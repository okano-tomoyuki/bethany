# 0012. フォームの残りのイベント(OnCloseQuery・OnHide・OnActivate・OnDeactivate・OnDestroy)を追加し、DLL の切り離し中はイベントを送らない

- 状態: 承認
- 日付: 2026-09-26

## 背景

[ADR 0011](0011-form-release-onclose-oncreate.md) で OnCreate・OnShow・OnClose を追加したが、TCustomForm が公開する他のイベントが残っていた。
このうち次の 2 点は、実装にあたって決める必要があった。

- **OnCloseQuery** は `TCloseQueryEvent`(Sender と、書き換え可能な `bool& CanClose`)で、OnClose と同じく C API での表し方を決める必要がある。
- **OnDestroy** は破棄の途中で呼ばれるため、ラッパーの寿命([ADR 0008](0008-wrapper-lifetime-follows-lcl.md))との前後関係と、
  プロセス終了時(DLL の切り離し)の扱いを決める必要がある。LCL は DLL の切り離し時に Application が所有するフォームを破棄するが、
  その時点で呼び出し側(C++ の実行環境や、C のコールバックが参照するデータ)は既に破棄されている可能性がある。
  [ADR 0010](0010-application-object.md) で止めたのは破棄通知だけで、OnDestroy や、破棄に伴って呼ばれる OnHide は止めていなかった。

## 検討した選択肢

- OnCloseQuery の C API: 戻り値で CanClose を返す / CanClose へのポインタを渡して書き換えさせる
- DLL の切り離し中のイベント: 呼び出し側の責任とする / Pascal 側のブリッジで一律に止める

## 決定

1. **OnCloseQuery は `Property<TCloseQueryEvent>`**、`using TCloseQueryEvent = std::function<void(TObject* Sender, bool& CanClose)>;` とする。
   C API のコールバックは OnClose と同じ形で、`void (*)(beth_obj_t sender, beth_bool_t* canClose, void* data)`(ポインタ先を書き換えさせる)。
   Pascal 側では OnClose と OnCloseQuery のブリッジを、書き換え可能な引数を 1 つ持つイベント用の 1 つのクラスにまとめた。
2. **OnHide・OnActivate・OnDeactivate・OnDestroy は `Property<TNotifyEvent>`** とし、他のイベントと同じく最初に空でないハンドラが設定されたときにブリッジを登録する。
3. **OnDestroy は LCL のとおり破棄の最初(BeforeDestruction)に呼ばれる。** 破棄通知(ラッパーの delete)はその後に来るため、
   ハンドラには生きている Sender が渡り、子コントロールにもアクセスできる。
   Application が所有するフォームの OnDestroy は、main から戻った後の C++ の終了処理(ADR 0010)の中で呼ばれる。
4. **DLL の切り離し中は、Pascal 側のすべてのブリッジでイベントを送らない。** 切り離しフックでフラグを立て、
   ブリッジはフラグが立っていれば呼び出し側を呼ばない。C から使う場合、終了前に `beth_TComponent_DestroyComponents(app)` を呼ばなければ、
   Application が所有するフォームの OnDestroy は呼ばれない(破棄通知と同じ扱い)。

C++ 側では、フォームのイベントの Setter で共通の処理(ハンドラの保持と、初回のブリッジ登録)を beth.cpp 内の関数テンプレートにまとめた。

## 影響

- OnCloseQuery で閉じるのを取りやめると OnClose は呼ばれない(LCL・C++Builder と同じ順序)。
- 破棄の最初に LCL がフォームを隠すため、表示中のフォームを破棄すると OnDestroy の前に OnHide が呼ばれる。
- TForm が published にしている TControl 由来のイベント(OnResize・OnPaint・OnKeyDown 等)は未対応。
