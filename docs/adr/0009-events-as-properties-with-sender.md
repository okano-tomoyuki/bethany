# 0009. イベントは Sender を受け取るプロパティとし、C API のコールバックに利用者データを渡す

- 状態: 承認
- 日付: 2026-09-26

## 背景

これまで C++ ラッパーのイベントは `button->SetOnClick(std::function<void()>)` という形で、
ハンドラはイベントを発生させたオブジェクト(Sender)を受け取れなかった。
C++Builder では `Button1->OnClick = Button1Click;` のようにイベントはプロパティであり、
ハンドラは `void __fastcall Button1Click(TObject* Sender)` のように Sender を受け取る。
複数のコントロールで 1 つのハンドラを共有し、Sender で区別するのは C++Builder の定番の書き方であり、
デザイナーが生成するコードの形も直接決まるため、コントロールを増やす前に揃えることにした。

C API のコールバックは既に Sender を渡していたが、利用者データを渡す引数が無く、
C から使う場合はグローバル変数で状態を持ち回るしかなかった。

## 検討した選択肢

- イベントの型: `std::function<void(TObject*)>` / メンバ関数ポインタとオブジェクトを組にした独自のデリゲート型
- 公開の形: `SetOnXxx()` メソッドのまま / プロパティ(`OnXxx = ...`)
- Sender: LCL のハンドル(`beth_obj_t`)のまま渡す / C++ ラッパー(`TObject*`)に変換して渡す
- C API のコールバック: 現状のまま / 利用者データ(`void* data`)を追加する

## 決定

1. **イベントの型は `using TNotifyEvent = std::function<void(TObject* Sender)>;`** とする(C++Builder と同じ型名)。
   本家の `__closure` は標準 C++ に無いため、メンバ関数は
   `Button1->OnClick = [this](TObject* Sender) { Button1Click(Sender); };` のようにラムダで割り当てる。
   デザイナーの生成コードもこの形にする。今後、別のシグネチャのイベント(OnClose 等)も本家の型名(`TCloseEvent` 等)で追加する。
2. **イベントはプロパティ(`Property<TNotifyEvent>`)として公開する。** `SetOnXxx` は削除した。
   配置は [docs/class-hierarchy.md](../class-hierarchy.md) の表のとおり(OnClick は TControl、OnChange は TCustomEdit と TComboBox 等)。
   `nullptr` を代入するとハンドラを解除できる。
3. **Sender は C++ ラッパーの `TObject*`。** Pascal から通知されたハンドルを共通レジストリでラッパーに変換して渡す。
   `TObject` は仮想デストラクタを持つため、本家と同じく `dynamic_cast<TButton*>(Sender)` も使える。
4. **C API のコールバックは `void (*)(beth_obj_t sender, void* data)`** とし、登録関数(`beth_TControl_SetOnClick` 等、
   `beth_FreeNotify_SetCallback` も含む)に `data` 引数を追加する。`data` は登録時の値がそのまま返る。
   C++ ラッパーは共通レジストリを使うので `data` は使わない。
   戻り値や書き換え可能な引数を持つイベント(OnClose の Action 等)の C API での表し方は、
   C++ 側の実装を優先し、そのイベントを実装するときに決める。

あわせて次を行った。

- **ハンドラはコピーしてから呼ぶ。** ハンドラの中でハンドラ自身を差し替えても、実行中の `std::function` が破棄されないようにするため。
- **Pascal 側のブリッジは、最初に空でないハンドラが設定されたときだけ登録する**(従来どおり 1 度だけ)。
- **`Property<T>` 同士の代入を値のコピーとして扱う。** 従来はコピー代入を削除していたため、
  `RadioButton2->OnClick = RadioButton1->OnClick;` や `Label1->Caption = Label2->Caption;` がコンパイルエラーになっていた。
  代入は「代入元の値を読んで代入先に書く」だけで所有者は付け替えないため、コピー構築の禁止(所有者の取り違え防止)とは両立する。

## 影響

- C++ の既存コードは `SetOnClick([]() { ... })` から `OnClick = [](TObject* Sender) { ... }` に書き換えが必要(テストは追随済み)。
- C API の既存コードは、コールバックに `void* data` 引数を追加し、登録関数に `data` を渡すよう書き換えが必要(テストは追随済み)。
- `test/main.cpp` を、デザイナーが生成することを想定した形(`TForm` の派生クラスが生ポインタメンバを持ち、
  メンバ関数のハンドラを `[this]` のラムダで割り当てる)に書き直し、Sender の判定・ハンドラの共有・
  `dynamic_cast` による種類の判定・ハンドラの解除を確認できるようにした。
