# 0025. Tier 2 の 6 バッチ目として TToolBar と TToolButton を追加する

- 状態: 承認
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の Tier 2、6 バッチ目として TToolBar・TToolButton に着手した。
LCL のソース(`comctrls.pp`・`toolwin.pp`・`include/toolbar.inc`・`include/toolbutton.inc`)で確認したこと:

- 継承関係:
  - ツールバーは `TCustomControl → TToolWindow → TToolBar`。
  - ボタンは `TGraphicControl → TToolButton`。
  - ツールバーもボタンも OS のツールバーではなく、LCL が描画する。
- **TToolButton は TComponent**。寿命は他のコントロールと同じく Owner と FreeNotification で管理でき、
  TTreeNode 等のような項目の仕組み([ADR 0020](0020-listview-and-shared-item-registry.md))は要らない。
- ツールバーへの追加:
  - ボタンの Parent をツールバーにすると、`TToolButton.SetParent` がツールバーのボタンの一覧の末尾に登録する。
  - 同じ仕組みで、ボタンを Free すると一覧から外れる。
- メンバの宣言場所:
  - `EdgeBorders`・`EdgeInner`・`EdgeOuter`・`BeginUpdate`・`EndUpdate` は、**TToolWindow の public**(TToolBar が published にしている)。
  - 残りは TToolBar・TToolButton 自身が宣言している。
- クリックの処理の場所:
  - tbsCheck の Down の切り替えと OnClick の呼び出しは、**`TToolButton.MouseUp`** で行う。
  - `Click` は `TControl.Click`(OnClick を呼ぶだけ)。

## 決定

- C API は宣言しているクラスの名前で、`beth_TToolWindow_*`・`beth_TToolBar_*`・`beth_TToolButton_*`。
  Caption・OnClick・Align は既存の `beth_TControl_*` を使う。
- C++ の各クラス:
  - `TToolWindow`(EdgeBorders・EdgeInner・EdgeOuter・BeginUpdate・EndUpdate)
  - `TToolBar`: ButtonCount・`Buttons[i]`・RowCount・ButtonHeight・ButtonWidth・DropDownWidth・Indent・Flat・List・
    ShowCaptions・Transparent・Wrapable・SetButtonSize。
  - `TToolButton`: AllowAllUp・Down・Grouped・Indeterminate・Marked・ShowCaption・Wrap・Style・DropdownMenu・MenuItem・
    OnArrowClick・Index と、Click・ArrowClick・PointInArrow。
- 集合と列挙型:
  - `TEdgeBorders` は、TShiftState と同じくビット集合(`unsigned int`、ビットの位置は TEdgeBorder の序数)。
  - `TEdgeStyle`・`TToolButtonStyle` は LCL と同じ値の列挙型。
- メンバの引き方:
  - `Buttons[i]` は `ReadOnlyIndexedProperty`([ADR 0023](0023-remaining-indexed-properties.md))。
    ボタンは利用者が生成したコンポーネントなので、レジストリから引く。
  - `MenuItem` の取得は、LCL が内部で生成したメニュー項目もありうるため `WrapExisting` で引く。
- 見送ったもの:
  - Images・HotImages・DisabledImages・ImagesWidth・ImageIndex(TImageList は Tier 3 待ち)
  - OnPaint・OnPaintButton(TCustomControl の Canvas をまだ公開していない)
  - ButtonList・ButtonDropWidth・Action

## 実装して分かったこと

- `TToolButton::Click()`:
  - OnClick を呼ぶだけで、tbsCheck の Down は変えない(VCL も同じ)。
  - Down が切り替わるのはマウスで押して離したとき。
- Grouped の tbsCheck のボタン:
  - プログラムから Down を true にしても、隣り合う他方が上がる。
- `RowCount` は行数ではない:
  - Wrapable が false のときに、Wrap を設定したボタンで強制的に折り返した回数を数える(折り返しが無ければ 0)。
  - Wrapable による自動の折り返しは数えない。
- MenuItem を設定したボタン:
  - その項目の Caption 等を写す。
  - マウスで押すと、その項目の OnClick を呼んでから、子の項目を一時的なポップアップメニューに移して表示する。
  - DropdownMenu と同じく、メニューを閉じるまで戻らない(同期の Popup)。そのため、自動テストでは押していない。
- 表示の確認:
  - ShowCaptions で文字の幅が ButtonWidth を超えると、ボタンの幅が広がる。
  - ツールバーの幅に収まらないボタンは次の行へ折り返されるが、ツールバーの高さ(AutoSize でなければ既定のまま)は変わらず、はみ出して見えなくなる。
- マウスのメッセージで確認したこと:
  - tbsButton のクリック → OnClick。
  - tbsCheck のクリック → Down が切り替わってから OnClick。Grouped なら他方が上がる。
  - tbsDropDown の本体 → OnClick、矢印 → OnArrowClick。
  - スクリーンショットでも押された状態の描画を確認した。
- Linux/GTK2(WSL)でも同じ結果になった。

## 影響

- メニューとツールバーを組み合わせた、一般的なアプリケーションの画面を組めるようになった。
- Tier 2 の残りは TCoolBar だけになった。
