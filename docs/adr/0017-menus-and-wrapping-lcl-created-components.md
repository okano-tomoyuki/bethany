# 0017. メニュー(TMenuItem・TMainMenu・TPopupMenu)を追加し、LCL が内部で生成したコンポーネントを後からラップする仕組みを入れる

- 状態: 承認(一部置換→0023。インデックス付きプロパティは添字で書く)
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の Tier 5(メニュー)に着手した。VCL アプリの UI にほぼ必須だが、
TMenuItem が子の項目を持つツリー構造の TComponent であるため、これまでの「プロパティの Get/Set とイベント」だけでは
表せず、項目のコレクション操作(Add・Insert・Delete・Count・Items[i])を C API・C++ の両方で設計する必要があった。

LCL のソース(`menus.pp`・`include/menu.inc`・`include/menuitem.inc`)で確認したこと:

- 継承関係は `TLCLComponent → TMenuItem`、`TLCLComponent → TMenu → TMainMenu / TPopupMenu`。どれも TControl ではない。
- **`TMenu.Items`(メニューのルート項目)は、TMenu のコンストラクタの中で LCL が `TMenuItem.Create(Self)` で生成する。**
  no_vcl の `*_Create` を経由しないため、C++ のラッパーも破棄通知の登録も無い。`TMenuItem.AddSeparator` が追加する
  区切り線も同じく LCL の内部で生成される。
- `TMenuItem.Delete` / `Remove` は子から外すだけで破棄しない(VCL と同じ)。`Clear` はすべての子を破棄する。
- `TMenuItem.Destroy` は、Owner が誰であっても子の項目をすべて破棄する。
- `TCustomForm.Menu`(TMainMenu)は TCustomForm の public、`TControl.PopupMenu` は TControl の public。

2 つ目の点は、component-coverage.md の cross-cutting な課題「LCL が内部で生成する子コンポーネントをラップできない」
(TLabeledEdit.EditLabel で見送った問題)そのものである。メニューのルート項目は `MainMenu1->Items->Add(...)` のように
必ず使うため、今回この課題を解決する必要があった。

## 検討した選択肢

LCL が内部で生成したコンポーネントの扱い:

- 選択肢A: ルート項目を C++ では隠し、`TMenu::ItemsAdd(TMenuItem*)` のような転送メソッドだけを用意する。
  仕組みが要らない代わりに、VCL の `MainMenu1->Items->Add(...)`・`Items->Count` の書き方ができない。
  区切り線(AddSeparator)や、ルートを親に持つ項目の `Parent` も表せない。TLabeledEdit.EditLabel にも使えない。
- 選択肢B: ハンドルを返す C API の側で破棄通知に登録し、C++ 側は初めて受け取ったハンドルにその場でラッパーを作る。
  VCL と同じ書き方ができ、TLabeledEdit.EditLabel 等にも同じ形で使える。
- 選択肢C: C++ の TMenu のコンストラクタでルート項目のラッパーを必ず作る。
  ルート項目には使えるが、区切り線のように後から LCL が作るものには使えない。

項目の参照の形:

- LCL の既定の配列プロパティ `Items[Index]` / `Count` は、C++ では `GetItem(Index)` と `ReadOnlyProperty<int> Count` にする
  (TCheckGroup の `GetChecked(Index)` と同じく、インデックス付きプロパティは Get/Set メソッドで表す既存の方針)。

## 決定

選択肢B を採る。

- **Pascal 側**: LCL が内部で生成したコンポーネントを返しうる関数(`TMenu_GetItems`・`TMenuItem_GetItem`・
  `TMenuItem_GetParent`)は、返す前に `Watch`(`FreeNotification`)で破棄通知の対象に登録する。
  `FreeNotification` は同じ相手に何度呼んでも 1 回分しか登録されないため、*_Create で生成した項目を返す場合も害は無い。
- **C++ 側**: `TComponent::WrapExisting<T>(handle)` を追加した。レジストリに既にラッパーがあればそれを返し、
  無ければその場で `new T(handle)` してレジストリに登録する(以降は同じラッパーを返す)。以後は通常のラッパーと同じく、
  LCL オブジェクトの破棄通知で delete される。T のハンドルを受け取るコンストラクタは private にし、
  `friend class TComponent` で WrapExisting からだけ呼べるようにする(利用者が既存のハンドルから二重にラッパーを作れないように)。
- **クラス**: C++ は `TComponent → TMenuItem`、`TComponent → TMenu → TMainMenu / TPopupMenu`(TLCLComponent は
  [ADR 0007](0007-lcl-faithful-hierarchy.md) の方針どおり省く)。
  - TMenuItem: Caption・Checked・Enabled・Visible・AutoCheck・RadioItem・GroupIndex・Default・ShortCut・Hint・OnClick、
    Count・Parent(読み取り専用)、GetItem・Add・Insert・Delete・Remove・Clear・IndexOf・AddSeparator・IsLine・Click。
  - TMenu: Items(読み取り専用。ルート項目)。
  - TPopupMenu: AutoPopup・PopupComponent・OnPopup・OnClose・Popup(X, Y)。
  - `TCustomForm::Menu`(`Property<TMainMenu*>`)と `TControl::PopupMenu`(`Property<TPopupMenu*>`)。
- **ショートカットキー**: VCL と同じく `TShortCut`(仮想キーコード + scShift/scCtrl/scAlt)とし、Menus ユニットの
  `ShortCut(Key, Shift)`・`TextToShortCut`・`ShortCutToText` を C++ の自由関数(C API は `no_vcl_ShortCut_*`)として用意する。
- 見送ったもの: Bitmap・ImageIndex(Tier 3 の TBitmap/TImageList 待ち)、Action、OnDrawItem/OnMeasureItem(オーナードロー)、
  TMainMenu.Merge、TPopupMenu.Alignment/TrackButton。

## 実装して分かったこと

- **メニューは Application->Run() の前に生成・割り当ててよい。** TStatusBar([ADR 0015](0015-tier1-batch1-and-statusbar-issue.md))の
  ような、DLL で AppHandle が無いことによる問題は起きなかった。フォームのコンストラクタで組んだメニューが、
  Win32 の `GetMenu` で実際にウィンドウに付いていることを確認した。
- 実行中のウィンドウに `WM_COMMAND`(項目の ID)を送ると OnClick が呼ばれ、`WM_CONTEXTMENU` を送ると
  PopupMenu が開いて OnPopup の PopupComponent が右クリックしたパネルになることを確認した。
- `Click()` は AutoCheck を反映してから OnClick を呼ぶ。RadioItem は同じ GroupIndex の他の項目の Checked を外す。
- C 版テストの破棄数には、取得した時点で破棄通知の対象になったルート項目(2 つ)と区切り線(1 つ)も数えられる。
  取得していない内部生成の項目は破棄通知の対象にならない(破棄はされる)。
- Linux/GTK2(WSL)でも同じ結果になることを確認した。

## 影響

- `MainMenu1->Items->Add(FileMenu)` のような VCL と同じ書き方で、メニューバーと右クリックメニューを組めるようになった。
- `WrapExisting` により、component-coverage.md の cross-cutting な課題「LCL が内部で生成する子コンポーネントをラップできない」は
  仕組みとしては解決した。TLabeledEdit.EditLabel も、`EditLabel` を返す C API で Watch し、C++ で `WrapExisting<TBoundLabel>` する
  同じ形で実装できる。
- 内部生成のコンポーネントのラッパーは、取得するまで作られない。取得前に LCL オブジェクトが破棄された場合はラッパーも無いので問題ない。
- 注意: C++ ラッパーを介さずに作られたコンポーネント(C API だけで作ったもの等)を `Parent`・`PopupMenu`・`Menu` 等の
  「利用者が作るもの」を返す Getter で取得すると、従来どおり nullptr になる。WrapExisting を使うのは、
  LCL が内部で生成することが分かっているものを返す Getter だけに限る(型が確定していないハンドルにラッパーを作らないため)。
