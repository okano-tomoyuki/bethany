# 0041. ModalResult・メッセージのダイアログ・フォームの表示・既定のボタン(Tier A の 1 バッチ目)

- 状態: 承認
- 日付: 2026-09-29

## 背景

[member-coverage.md](../member-coverage.md) の Tier A の A1〜A4。C++Builder でダイアログを作る基本の書き方
(ボタンの `ModalResult` でフォームを閉じて `ShowModal()` の戻り値で分ける、`MessageDlg` で確かめる、`Position = poMainFormCenter`、
Enter・Esc で押される既定のボタン)ができず、サンプルのメモ帳([example/](../../example/README.md))では自前のメンバで代わりに書いていた。

## 決定

### ModalResult(A1)

- `TModalResult` は整数の別名(`using TModalResult = std::int32_t;`)とし、`mrNone`・`mrOk`・`mrCancel`・`mrAbort`・`mrRetry`・`mrIgnore`・
  `mrYes`・`mrNo`・`mrAll`・`mrNoToAll`・`mrYesToAll`・`mrClose` を定数にする(値は System.UITypes と同じ)。列挙型にしないのは、
  VCL と同じく利用者が `mrOk + 1` のような独自の値を使えるようにするため。
- `TCustomForm.ModalResult` と `TCustomButton.ModalResult` を加える。`ShowModal()` の戻り値の型を `TModalResult` にする(中身は同じ int)。
  ボタンの ModalResult が mrNone でなければ、押したときに LCL がフォームの ModalResult を設定し、モーダルのフォームが閉じる。
- デザイナーでは、ボタンの `ModalResult` をプロパティに出す。値は定数の名前(`"mrOk"`)で書き、生成するコードも `ModalResult = mrOk` になる
  (デザイナーの整数の別名の扱いを、TCursor だけでなく定数を持つ別名一般に広げる)。フォームの ModalResult は実行時に使うもので、LCL でも published ではない。

### メッセージのダイアログ(A2)

- グローバルな関数 `ShowMessage`・`MessageDlg`・`InputBox`・`InputQuery`・`PasswordBox` と、`Application->MessageBox` を加える。
- `MessageDlg(Msg, DlgType, Buttons, HelpCtx)` と、題名を付ける `MessageDlg(Caption, Msg, DlgType, Buttons, HelpCtx)`。
  `TMsgDlgType`(`mtWarning`・`mtError`・`mtInformation`・`mtConfirmation`・`mtCustom`)は列挙型、`TMsgDlgButtons` は `TMsgDlgBtn` の集合
  (`Set<TMsgDlgBtn>`。`TMsgDlgButtons() << mbYes << mbNo`)とし、VCL と同じ定数 `mbYesNo`・`mbYesNoCancel`・`mbOKCancel`・`mbAbortRetryIgnore` を置く。
  戻り値は `TModalResult`。
- `InputQuery` の値は、C++ は参照(`std::string&`)、Python は `Ref`(イベントの `CanClose` と同じ)で受け渡す。
- `Application->MessageBox(Text, Caption, Flags)` の Flags と戻り値は Windows の `MB_…`・`ID…` の値。C++ では `<windows.h>` の定数を使う
  (beth.hpp で同じ名前を定義すると、`<windows.h>` のマクロと衝突するため)。Python では `beth` に定数を置く。
- `QuestionDlg`(ボタンの文言を自由に決める)は、ボタンの配列の渡し方の設計が要るため、このバッチでは扱わない。

### フォームの表示(A3)と既定のボタン(A4)

- `TCustomForm` に `BorderStyle`(`TFormBorderStyle`: `bsNone`・`bsSingle`・`bsSizeable`・`bsDialog`・`bsToolWindow`・`bsSizeToolWin`)・
  `Position`(`TPosition`: `poDesigned` 〜 `poWorkAreaCenter`)・`WindowState`(`TWindowState`)・`BorderIcons`(`TBorderIcons` = `Set<TBorderIcon>`)・
  `FormStyle`(`TFormStyle`)・`KeyPreview`・`ActiveControl` を加える。列挙型の値の順は LCL と同じ。
- `TCustomButton` に `Default`・`Cancel` を加える。
- デザイナーでは、フォームの `BorderStyle`・`Position`・`WindowState`・`BorderIcons`・`FormStyle`・`KeyPreview`・`ActiveControl` と、ボタンの
  `Default`・`Cancel`・`ModalResult` をプロパティに出す(カタログを抽出し直す)。

## 影響

- メモ帳のサンプルの `ConfirmSaveForm` は `MessageDlg` に、`AboutForm` の閉じ方はボタンの `ModalResult` に置き換えられる。
- `ShowModal()` の戻り値の型の名前が `int` から `TModalResult` に変わるが、中身は同じ int なので既存のコードはそのまま通る。
- 後の B17(コントロールの枠)で `TBorderStyle`(`bsNone`・`bsSingle`)を加えるときは、LCL と同じく `TFormBorderStyle` の一部として扱う。
