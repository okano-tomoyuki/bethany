# 0033. Tier 4 としてダイアログを追加し、結果を適用する先の TControl.Color・Font を追加する

- 状態: 承認
- 日付: 2026-09-28

## 背景

[component-coverage.md](../component-coverage.md) の Tier 4(ダイアログ)が未着手のまま残っていた。
VCL のアプリでは、ファイルの読み書き・色やフォントの選択・検索と置換のために、コモンダイアログをほぼ必ず使う。

- 形は VCL と同じく「プロパティを設定して `Execute()` を呼び、結果を bool で受け取る」。
- ダイアログの結果は、コントロールの Color・Font に適用することが多い
  (`Panel1->Color = ColorDialog1->Color;`・`Memo1->Font->Assign(FontDialog1->Font);`)。
  しかし Bethany の TControl には Color も Font も無く、TFont も Name・Size・Color だけだった(Style・Assign が無い)。

LCL のソース(`dialogs.pp`・`include/filedialog.inc`・`finddialog.inc`・`replacedialog.inc`・
`interfaces/win32/win32wsdialogs.pp`)で確認したこと:

- **継承関係**:
  - TCommonDialog は TLCLComponent の派生で、TControl ではない。
  - TSaveDialog・TSelectDirectoryDialog は **具象クラスの TOpenDialog の派生**、TReplaceDialog は TFindDialog の派生。
- **Execute**:
  - TOpenDialog 等・TColorDialog・TFontDialog は、閉じるまで戻らない(Win32 では OS のコモンダイアログ)。
  - TFindDialog・TReplaceDialog は LCL のフォームで作ったモードレスのダイアログで、表示してすぐ True を返す。
    ボタンが押されると、Options の frFindNext・frReplace・frReplaceAll を設定してから OnFind・OnReplace を呼ぶ。
- **公開範囲**:
  - ReplaceText・OnReplace は TFindDialog の protected で、TReplaceDialog が published にしている。
  - TControl の Color・Font は TControl の public。
- **既定値**:
  - TOpenDialog.Options は `[ofEnableSizing, ofViewDetail]`、TColorDialog.Options は `[cdFullOpen]`(VCL は空)、
    TFontDialog.Options は `[fdEffects]`、TFindDialog.Options は `[frDown]`。
  - TReplaceDialog は生成時に `frReplace, frReplaceAll, frHidePromptOnReplace` を加える。
  - TColorDialog.CustomColors は ColorA〜ColorT の 20 行(`ColorA=000000` の形)。
  - TFileDialog.SetDefaultExt は先頭に `.` を補う(VCL は補わない)。
- **Win32 のダイアログの親**: 表示中のフォーム(Screen.ActiveForm)か MainForm を親にする。
  DLL で `WidgetSet.AppHandle` が 0 になる問題([ADR 0015](0015-tier1-batch1-and-statusbar-issue.md))とは関係しない。

印刷のダイアログ(TPrintDialog・TPrinterSetupDialog)の具象クラスは、LCL 本体ではなく別パッケージ
(`components/printers` の PrintersDlgs)にあり、現在のビルドの `-Fu` に無い。

## 検討した選択肢

対象の範囲:

- 選択肢A: 優先度の高いもの(ファイル・色・フォント)だけ。
- 選択肢B: 選択肢A に検索・置換も加える。検索・置換は LCL 本体にあり、LCL のフォームなので OS に依存しない。
- 選択肢C: 選択肢B に印刷も加える。印刷の基盤(TPrinter)が Bethany に無く、ビルドのパスの追加が前提になる。

ダイアログの結果を適用する先:

- 選択肢A: ダイアログだけを追加する。`Panel1->Color = ...` のような VCL のコードは書き換えが要る。
- 選択肢B: TControl に Color・Font を、TFont に Style・Assign を足す。

派生を持つ具象クラス(TOpenDialog・TFindDialog)のコンストラクタ:

- 選択肢A: protected のコンストラクタ `(ObjectHandle handle)` を足す。
  `ObjectHandle` は `void*` なので、`new TOpenDialog(nullptr)`(VCL でよく書く、Owner の無い一時的なダイアログ)が
  public の `(TComponent* AOwner)` と曖昧になり、コンパイルエラーになる。
- 選択肢B: protected のコンストラクタの引数に、目印の型(`DerivedTag`)を足して区別する。

## 決定

- **対象**: 選択肢B。TOpenDialog・TSaveDialog・TSelectDirectoryDialog・TColorDialog・TFontDialog・TFindDialog・TReplaceDialog。
  印刷のダイアログは見送る(印刷の基盤と合わせて、必要になった時点で別の ADR で扱う)。
- **C++ の階層**: LCL の部分列として、TComponent → TCommonDialog → TFileDialog → TOpenDialog → TSaveDialog / TSelectDirectoryDialog、
  TCommonDialog → TColorDialog / TFontDialog / TFindDialog → TReplaceDialog(TLCLComponent は省く)。
  TComponent なので寿命は既存の仕組み(new で生成し、Owner に任せるか Free())。
- **メンバ**:
  - TCommonDialog: `Execute()`・Title・OnShow・OnClose・OnCanClose(`TCloseQueryEvent`。フォームの OnCloseQuery と同じブリッジ)。
  - TFileDialog: FileName・Filter・FilterIndex・InitialDir・DefaultExt・Files(`ReadOnlyProperty<TStrings*>`)。
  - TOpenDialog: Options。
  - TColorDialog: Color・CustomColors(`ReadOnlyProperty<TStrings*>`)・Options。
  - TFontDialog: Font・MinFontSize・MaxFontSize・Options。
  - TFindDialog: FindText・Options・Left・Top・OnFind・`CloseDialog()`。ReplaceText・OnReplace は protected に置き、
    TReplaceDialog で `using` する(DLL の関数は宣言元の `TFindDialog_*` で、protected hack でアクセスする)。
  - Options はいずれも TGridOptions と同じく、ビットを OR した整数(`TOpenOptions`・`TColorDialogOptions`・
    `TFontDialogOptions`・`TFindOptions`)。
- **結果を適用する先**: 選択肢B。
  - TControl: Color(`Property<TColor>`)・Font(`Property<TFont*>`)。
  - TFont: Style(`TFontStyles`。fsBold 等のビット集合)・`Assign(const TFont*)`。
  - Font(TControl・TFontDialog)は所有者が持つ TFont の値メンバのビューで、所有者と寿命が一致する
    (LCL の Font は所有者の生成時に作られ、差し替わらない)。代入は LCL の SetFont と同じく内容のコピー
    (`Memo1->Font = FontDialog1->Font;` は `Memo1->Font->Assign(FontDialog1->Font);` と同じ)。
  - あわせて `clNone`・`clDefault` を足した(コントロールの Color の既定値が clDefault のため)。
- **派生を持つ具象クラスのコンストラクタ**: 選択肢B。TCommonDialog に protected の `struct DerivedTag {}` を置き、
  TOpenDialog・TFindDialog は protected の `(ObjectHandle, DerivedTag)` を持つ。
- **Python**: いずれも命名規則どおりなので、手書き(`_mixins`)は要らず、gen_api.py の生成だけで使える。
- 見送ったもの:
  - TFileDialog.OnTypeChange、TOpenDialog.OnSelectionChange・OnFolderChange・OptionsEx、TFontDialog.OnApplyClicked・PreviewText。
  - TCommonDialog.Width・Height・Close・UserChoice・HelpContext、TFindDialog.Position・OnHelpClicked。
  - 印刷のダイアログ(上記)。

## 実装して分かったこと

- **Execute(Win32)**:
  - DLL から呼んでも、各ダイアログが表示され、OK で True、キャンセルで False が返った。
  - モーダルの間も LCL のタイマーは動く。検証では、タイマーから開いているダイアログに IDOK・IDCANCEL を送って閉じた
    (C++ ラッパー・Python の公開 API の両方)。この自動化は Win32 固有のため、リポジトリのテストには入れず、
    C++・Python のテストの "Dialogs" メニューから手で試す形にした。
  - `new TOpenDialog(nullptr)` は曖昧にならずにコンパイルでき、Owner の無いダイアログとして使えた。
- **イベント(Win32)**:
  - OnShow・OnClose は、どのダイアログでも Sender がそのダイアログのラッパーで呼ばれた。
  - OnCanClose はファイルのダイアログでだけ呼ばれた(TColorDialog・TFontDialog では呼ばれない)。
    CanClose を false にすると、ダイアログは閉じずに残った。
- **Title**: 日本語(UTF-8)のタイトルがそのまま表示された。TFontDialog では Title は使われず、OS の既定のタイトルになった。
- **フォント**:
  - LCL の既定のフォントは Name が `default`、Size が 0。
  - TFontDialog で Arial・14・太字斜体・赤を設定して OK で閉じると、同じ値が返った。
  - コントロールの Font への代入は内容のコピーで、代入後に元を変えても写した先は変わらない。Font のハンドルは代入の前後で同じ。
- **既定値**: 上記の「既定値」のとおり(TSaveDialog の DefaultExt に `txt` を設定すると `.txt` が返る)。
- **例外**: CustomColors の範囲外の添字は、他の TStrings と同じく `Exception`(EStringListError)になった([ADR 0031](0031-exceptions-across-dll.md))。
- **Linux(GTK2、WSLg)**: ビルドが通り、Run の前のプロパティの確認と TFindDialog の Execute・CloseDialog は Windows と同じ結果になった。
  GTK2 のモーダルのダイアログの Execute は、自動では確かめていない。
- **検索のダイアログの文字列**: タイトル・ボタンは英語(LCL の既定の文字列。タイトルは Find)で表示された。

## 影響

- Tier 4 は印刷を除いて完了した。VCL の `if (OpenDialog1->Execute()) ...`・`ColorDialog1->Color`・
  `Memo1->Font->Assign(FontDialog1->Font)` のようなコードを、書き換えずに移植できる。
- ただし LCL と VCL の違いが 2 つある: DefaultExt は先頭に `.` が付いて返る、TColorDialog.Options の既定値は cdFullOpen。
- TControl.Color・Font が入ったことで、ダイアログ以外でも色・フォントを変えられるようになった
  (ParentColor・ParentFont は未対応)。
- 具象クラスが派生を持つ場合の形(`DerivedTag`)は、今後同じ形のクラス(LCL の具象クラスの派生)を足すときにも使う。
- 印刷のダイアログは、印刷の基盤(TPrinter・Printers パッケージのビルドのパス)と合わせて扱う。
