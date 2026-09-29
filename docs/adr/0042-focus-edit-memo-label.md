# 0042. フォーカス・表示の更新・テキストの編集・TMemo・TLabel(Tier A の 2 バッチ目)

- 状態: 承認
- 日付: 2026-09-29

## 背景

[member-coverage.md](../member-coverage.md) の Tier A の A5〜A9。入力のフォームやテキストの編集(検索・置換・クリップボード)で
C++Builder の書き方をそのまま使えるようにする。

## 決定

- **A5 フォーカス**: `TWinControl` に `SetFocus()`・`CanFocus()`・`Focused()` と、イベント `OnEnter`・`OnExit`(TNotifyEvent)を加える。
- **A6 表示の更新・位置**: `TControl` に `Invalidate()`・`Repaint()`・`Refresh()`・`Update()`・`BringToFront()`・`SendToBack()`・
  `SetBounds(ALeft, ATop, AWidth, AHeight)` と、`ClientWidth`・`ClientHeight` を加える。
- **A7 テキストの編集**: `TCustomEdit`(TEdit・TMemo・TMaskEdit・TLabeledEdit・スピンエディットの基底)に `SelStart`・`SelLength`・`SelText`・
  `Modified`・`CanUndo`(読み取り専用)・`PasswordChar`・`EchoMode`(`TEchoMode`)・`CharCase`(`TEditCharCase`)・`Alignment`・`TextHint`・
  `NumbersOnly`・`AutoSelect`・`HideSelection`・`CaretPos` と、`SelectAll()`・`ClearSelection()`・`Clear()`・`CopyToClipboard()`・
  `CutToClipboard()`・`PasteFromClipboard()`・`Undo()` を加える。
  - LCL では、選択の置き換え(`SelText` の設定)・`SelectAll`・`ClearSelection`・クリップボード・`Undo` は、ウィンドウのハンドルが無い
    (フォームを表示する前)と何もしない。VCL は必要になった時点でハンドルを作るため、表示の前でも効く。C++Builder と同じ使い心地にするため、
    DLL がこれらの前に `HandleNeeded` を呼ぶ(親のハンドルも作られる)。
  - `CharCase` の値は VCL と同じ綴り(`ecUpperCase`・`ecLowerCase`)にする(LCL は `ecUppercase`)。
- **A8 TMemo**: `TCustomMemo` に `WordWrap`・`WantReturns`・`WantTabs` と `Append(S)` を加える(`CaretPos` は LCL と同じく `TCustomEdit`)。
- **A9 TLabel**: `TCustomLabel` に `Alignment`・`Layout`(`TTextLayout`)・`WordWrap`・`Transparent`・`FocusControl`・`ShowAccelChar` を加える。
- `TAlignment` の宣言を TCustomLabel より前に移す(TListColumn 等と共用)。
- Python では、TPoint のプロパティ(`CaretPos`)を TRect のプロパティと同じく扱う(DLL は 2 つの出力引数で返し、2 つの引数で受け取る)。
- デザイナーでは、published のもの(TEdit の `PasswordChar`・`EchoMode`・`CharCase`・`Alignment`・`TextHint`・`NumbersOnly`・`AutoSelect`・
  `HideSelection`、TMemo の `WordWrap`・`WantReturns`・`WantTabs`、TLabel の各プロパティ、`OnEnter`・`OnExit`)をプロパティ・イベントに出し、
  キャンバスの描画に反映する(ラベルの揃えと折り返し、入力欄の TextHint と伏せ字、メモの折り返し)。
  実行時に使うもの(`SelStart`・`SelLength`・`SelText`・`Modified`・`ClientWidth`・`ClientHeight`)と、LCL の TMemo が published にしない
  `PasswordChar`・`EchoMode`・`NumbersOnly`・`TextHint` は、デザイナーに出さない。

## 影響

- 表示の前に編集の操作を呼ぶと、そのコントロール(と親)のハンドルがその時点で作られる。
