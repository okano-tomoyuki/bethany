# 0043. リスト・コンボの選択・チェックの 3 状態・Application(Tier A の 3 バッチ目)

- 状態: 承認
- 日付: 2026-09-29

## 背景

[member-coverage.md](../member-coverage.md) の Tier A の A10〜A12。これで Tier A をすべて終える。

## 決定

- **A10 リストの選択**:
  - `TCustomListBox` に `MultiSelect`・`ExtendedSelect`・`Selected[i]`・`SelCount`・`ClearSelection()`・`SelectAll()`・`Sorted`・`TopIndex`・
    `ItemAtPos(Pos, Existing)` と、イベント `OnSelectionChange`(`TSelectionChangeEvent`: Sender と、利用者の操作によるものかの User)を加える。
  - `ItemAtPos` は、項目の位置を求めるのにウィンドウのハンドルが要るため、DLL がハンドルを作ってから呼ぶ(ADR 0042 の編集の操作と同じ)。
    LCL の `ItemAtPos` は、項目が無ければ `Existing` によらず -1 を返す(`Existing` は VCL との互換のために受け取る)。
  - `TCustomComboBox` に `Style`(`TComboBoxStyle`)・`DropDownCount`・`Sorted`・`ReadOnly`・`DroppedDown`・`AutoComplete` と、イベント
    `OnSelect`・`OnDropDown`・`OnCloseUp` を加える。
- **A11 チェックの 3 状態**: `TCustomCheckBox`(TCheckBox・TRadioButton・TToggleBox の基底)に `State`(`TCheckBoxState`)・`AllowGrayed` と
  イベント `OnChange` を加える。
- **A12 Application**: `ExeName`・`ShowHint`・`HintPause`・`HintHidePause`・`Minimize()`・`Restore()`・`BringToFront()` と、イベント
  `OnIdle`(`TIdleEvent`: `bool& Done`)・`OnException`(`TExceptionEvent`: `const Exception& E`)を加える。
  - `OnException` は、イベントのハンドラから送出された例外([ADR 0031](0031-exceptions-across-dll.md) で DLL が送出し直すもの)を、
    既定のエラーのダイアログの代わりに受ける。クラス名は元の例外のもの(DLL が送出し直したときのクラスではなく)を渡す。
    Python では `BethError`(`E.Message`・`E.ClassName()`)で受け、Python の例外ならそのクラス名(`ValueError` 等)になる。
  - LCL は例外によっては Sender を nil で渡す(タイマーの例外は `HandleException(nil)`)。そのため、Application のイベントは Sender によらず
    Application のハンドラを呼ぶ。C++ の `OnException` の Sender は LCL が渡したもの(nil のことがある)で、Python では Application。
- 新しいイベントの形に合わせて、DLL のコールバックの型 `bool_callback_t`(OnSelectionChange)・`exception_callback_t`(クラス名と
  メッセージの 2 つの文字列)を加える。`OnIdle` は `OnCloseQuery` と同じ形なので、既存の `close_query_callback_t` を使う。
- デザイナーでは、published のもの(リストボックスの `MultiSelect`・`ExtendedSelect`・`Sorted`、コンボボックスの `Style`・`DropDownCount`・
  `Sorted`・`ReadOnly`・`AutoComplete`、チェックボックスの `State`・`AllowGrayed`、各イベント)を出し、キャンバスにチェックボックスの
  中間の状態を描く。実行時に使う `TopIndex`・`DroppedDown` と、TRadioButton の `State`・`AllowGrayed` は出さない。

## 影響

- Tier A はこれで終わる。次は Tier B を、項目ごとに ADR を書いて進める。
