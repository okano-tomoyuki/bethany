# メンバの網羅性(実装済みのクラスに足りないメンバ・グローバルな関数)

[component-coverage.md](component-coverage.md) は**クラス単位**の棚卸しで、Tier 1〜5 はほぼ完了している。
本書は、実装済みのクラスに足りない**メンバ**(プロパティ・メソッド・イベント)と、クラスに属さない**グローバルな関数・オブジェクト**
(`MessageDlg`・`Screen` 等)の棚卸しと、その優先順位(Tier)を扱う。

きっかけは、サンプルのメモ帳([example/](../example/README.md))を書いたときに、C++Builder の基本的な書き方
(ボタンの `ModalResult` でダイアログを閉じる、`MessageDlg` で確認する、`Memo1->WordWrap`、`Position = poMainFormCenter` 等)が
使えず、代わりの書き方が要ったこと。

## 1. 洗い出しの方法

`python py/member_gap.py`(Lazarus 4.8 の LCL のソースと beth.hpp を比べる)の出力を元に、手で分類した。

- LCL のソースの interface 部から、実装済みの各クラスの published のプロパティ・イベントと、public のメンバを継承をたどって集め、
  beth.hpp のメンバ(継承を含む)と比べる。
- published の差分は、LCL で宣言したクラスごとにまとめる(TControl のものを派生のクラスごとに重ねて数えない)。
  ただし、LCL が派生のクラスで published にし直すもの(`OnMouseWheelDown` 等)は、その派生のクラスごとに出る。
- public は LCL の内部向けのもの(`AddHandler…`・`AutoSizeDelayed` 等)が多いため、VCL のアプリでよく使うものだけを照合する。
- グローバルな関数・オブジェクトは、LCL の `dialogs.pp`・`forms.pp`・`controls.pp`・`clipbrd.pp` から手で拾った。

| 区分 | 件数 |
|---|---|
| 未実装の published のメンバ(宣言したクラスで 95) | 606 |
| 対象外に分けた published のメンバ(ドッキング・ドラッグ・右から左・ヘルプ・高 DPI 等。§4) | 390 |
| VCL でよく使う public のメンバのうち未実装のもの(照合した 10 クラス) | 94 |
| グローバルな関数・オブジェクト(`ShortCut` 系の 3 つのほかは無い) | §3 |

件数は重複を含む(`OnContextPopup`・`OnMouseWheelDown` 等は、LCL が派生のクラスごとに published にし直すため、クラスの数だけ数える)。
解析は正規表現による簡易なもので、条件付きコンパイル(IFDEF)の中のメンバは正確でないことがある。

## 2. 実装のコストの目安

どれも DLL(`beth.pas` の export)・内部層(`include/bethany/internal/funcs.h`)・C++ のクラス(`beth.hpp`・`beth.cpp`)・
テスト(`test/`)の変更を伴う。Python の公開 API は `py/gen_api.py` が beth.hpp から生成するので、手で書く部分は少ない。
published のものはデザイナーのカタログ(`designer/tools/catalog/extract.py`)にも反映し、デザイナーのプロパティ・イベントに出す。

| コスト | 内容 |
|---|---|
| **S** | 既存の型(int・bool・string・既存の列挙型)のプロパティ、引数の少ないメソッド、既存の型(TNotifyEvent 等)のイベント。今の生成の仕組みでそのまま書ける |
| **M** | 新しい列挙型・集合型(`TFormBorderStyle`・`TMsgDlgButtons` 等)、新しい引数の形のイベント、ダイアログを出すグローバル関数、添字つきのプロパティ |
| **L** | 新しいクラス・コレクション・寿命の設計が要るもの(TStatusPanels・TScreen・TAction 等)。ADR を書いてから着手する |

## 3. Tier 表

### Tier A — 基本的なアプリに必須(最優先)

C++Builder の入門的なアプリ(ダイアログ・入力のフォーム・テキストの編集)をそのままの書き方で作れるようにするもの。
メモ帳のサンプルで代わりの書き方が要ったものは、すべてここに入る。✅ は実装済み(A1〜A4 は [ADR 0041](adr/0041-modal-result-and-message-dialogs.md)。`QuestionDlg` は未対応。A5〜A9 は [ADR 0042](adr/0042-focus-edit-memo-label.md)、A10〜A12 は [ADR 0043](adr/0043-list-combo-check-application.md)。Tier A はすべて実装済み)。

| # | 項目 | 対象 | 種類 | コスト | 備考 |
|---|---|---|---|---|---|
| A1 | ✅ **ModalResult** | `TCustomForm.ModalResult`・`TCustomButton.ModalResult`・`TModalResult` と `mrOk`・`mrCancel`・`mrYes`・`mrNo` 等 | P・定数 | M | ボタンの ModalResult を設定すれば、押したときに LCL がフォームを閉じて `ShowModal()` がその値を返す。`ShowModal()` は既に int を返している |
| A2 | ✅ **メッセージのダイアログ** | `ShowMessage`・`MessageDlg`(`TMsgDlgType`・`TMsgDlgButtons`)・`QuestionDlg`・`InputBox`・`InputQuery`・`PasswordBox`・`Application->MessageBox` | 関数 | M | 新しい列挙型と集合型。戻り値は A1 の `TModalResult` |
| A3 | ✅ **フォームの表示** | `TCustomForm` の `BorderStyle`(bsDialog 等)・`Position`(poMainFormCenter 等)・`WindowState`・`BorderIcons`・`FormStyle`(fsStayOnTop)・`KeyPreview`・`ActiveControl` | P | S〜M | 新しい列挙型・集合型。デザイナーに出す |
| A4 | ✅ **既定のボタン** | `TCustomButton.Default`・`Cancel`(Enter・Esc で押す) | P | S | A1 と組み合わせてダイアログを作る |
| A5 | ✅ **フォーカス** | `TWinControl.SetFocus`・`CanFocus`・`Focused`・`OnEnter`・`OnExit` | M・E | S | |
| A6 | ✅ **表示の更新・位置** | `TControl.Invalidate`・`Repaint`・`Refresh`・`Update`・`BringToFront`・`SendToBack`・`SetBounds`・`ClientWidth`・`ClientHeight` | M・P | S | |
| A7 | ✅ **テキストの編集** | `TCustomEdit` の `SelStart`・`SelLength`・`SelText`・`SelectAll`・`ClearSelection`・`Clear`・`CopyToClipboard`・`CutToClipboard`・`PasteFromClipboard`・`Undo`・`CanUndo`・`Modified`・`PasswordChar`・`EchoMode`・`CharCase`・`Alignment`・`TextHint`・`NumbersOnly`・`AutoSelect`・`HideSelection` | P・M | S〜M | 検索・置換(TFindDialog の OnFind で選択する)に要る |
| A8 | ✅ **TMemo** | `WordWrap`・`WantReturns`・`WantTabs`・`CaretPos`・`Append` | P・M | S | `TPoint` は既にある |
| A9 | ✅ **TLabel** | `Alignment`・`Layout`・`WordWrap`・`Transparent`・`FocusControl`・`ShowAccelChar` | P | S〜M | `TTextLayout` は新しい列挙型 |
| A10 | ✅ **リストの選択** | `TCustomListBox` の `MultiSelect`・`ExtendedSelect`・`Selected[i]`・`SelCount`・`ClearSelection`・`SelectAll`・`Sorted`・`TopIndex`・`ItemAtPos`・`OnSelectionChange`、`TCustomComboBox` の `Style`(csDropDownList)・`DropDownCount`・`Sorted`・`ReadOnly`・`DroppedDown`・`AutoComplete`・`OnSelect`・`OnDropDown`・`OnCloseUp` | P・M・E | S〜M | コンボボックスの `Style` は、選択だけのコンボボックスに要る |
| A11 | ✅ **チェックの 3 状態** | `TCustomCheckBox.State`・`AllowGrayed`・`OnChange` | P・E | S〜M | `TCheckBoxState` は新しい列挙型 |
| A12 | ✅ **Application** | `ExeName`・`OnException`・`OnIdle`・`Minimize`・`Restore`・`BringToFront`・`ShowHint`・`HintPause`・`HintHidePause` | P・M・E | S〜M | `OnIdle` は `Done` を参照で受けるイベント(`OnCloseQuery` の `CanClose` と同じ形) |

### Tier B — よく使う(中〜高コスト)

実用的なアプリでよく使うが、新しい型やコレクション・グローバルなオブジェクトの設計が要るもの。✅ は実装済み(B1 は [ADR 0044](adr/0044-statusbar-panels-and-designer-collections.md))。

| # | 項目 | 対象 | コスト | 備考 |
|---|---|---|---|---|
| B1 | ✅ **ステータスバーのパネル** | `TStatusBar.Panels`(`TStatusPanels`・`TStatusPanel` の `Text`・`Width`・`Alignment`・`Style`)・`SizeGrip`・`AutoHint`・`OnDrawPanel` | L | コレクション。TCoolBar の Bands・THeaderControl の Sections と同じ作り([ADR 0024](adr/0024-headercontrol.md)・[0026](adr/0026-coolbar-and-item-free-observer.md)) |
| B2 | **Screen** | `TScreen`(`Screen`)の `Cursor`・`Width`・`Height`・`WorkArea…`・`Forms`・`FormCount`・`ActiveForm`・`ActiveControl` | L | グローバルなオブジェクト。`Application` と同じく DLL の読み込み時に作る |
| B3 | **Clipboard** | `TClipboard`(`Clipboard()`)の `AsText`・`HasFormat`・`Clear`、画像の `Assign` | M〜L | A7 の `CopyToClipboard` 等とは別に、プログラムからクリップボードを読み書きするもの |
| B4 | **Action** | `TActionList`・`TAction`(`Caption`・`Enabled`・`Checked`・`ShortCut`・`OnExecute`・`OnUpdate`)と、コントロール・メニュー項目の `Action` | L | C++Builder のアプリでよく使う(メニューとツールボタンの状態をまとめる)。デザイナーにも非ビジュアルコンポーネントとして要る。ADR を書く |
| B5 | **フォームへの描画** | `TCustomControl`(TForm・TPanel 等)の `Canvas`・`OnPaint` | M | 今は `TPaintBox`・`TImage` にだけ Canvas がある |
| B6 | **Canvas の描画の関数** | `TCanvas` の `TextWidth`・`TextHeight`・`TextRect`・`Polygon`・`Polyline`・`RoundRect`・`Arc`・`Pie`・`FrameRect`・`CopyRect`、`TPen.Style`・`Mode`、`TBrush.Style`、`TFont.Height`・`Orientation`・`Quality` | S〜M | 配列(Polygon の点)を渡す形を決める |
| B7 | **TPanel の縁** | `BevelOuter`・`BevelInner`・`BevelWidth`・`BevelColor`・`Alignment`・`VerticalAlignment`・`WordWrap` | S〜M | `TPanelBevel` は新しい列挙型。デザイナーに出す |
| B8 | **オーナードロー** | `TListBox`・`TComboBox` の `Style`(lbOwnerDrawFixed 等)・`ItemHeight`・`OnDrawItem`・`OnMeasureItem`、メニューの `OwnerDraw`・`OnDrawItem` | M | `TOwnerDrawState`(集合)と TRect を受けるイベント |
| B9 | **スクロール** | `ScrollBars`(`TScrollStyle`: `ssNone`・`ssHorizontal`・`ssVertical`・`ssBoth`・`ssAutoHorizontal`・`ssAutoVertical`・`ssAutoBoth`)を TStringGrid・TDrawGrid・TTreeView に加え、実装済みの TMemo の `ScrollBars`(今は `int`)も `TScrollStyle` にする。`TScrollingWinControl` の `HorzScrollBar`・`VertScrollBar`(`TControlScrollBar`)、TForm・TScrollBox の `AutoScroll` | M | グリッドの既定は `ssAutoBoth`(必要なときだけ出る)なので、今も既定の動作では使える。TMemo の型を変えるとデザイナーのフォームのファイルの値(`"ScrollBars": 3`)も `"ssBoth"` に変わるため、古い数値も読めるようにする |
| B10 | **範囲のコントロールの細部** | `TTrackBar`(`Orientation`・`Frequency`・`TickMarks`・`TickStyle`・`LineSize`・`PageSize`・`SelStart`・`SelEnd`・`Reversed`)、`TProgressBar`(`Orientation`・`Smooth`・`Step`・`Style`・`BarShowText`)、`TScrollBar`(`LargeChange`・`SmallChange`・`OnScroll`)、`TUpDown`(`Orientation`・`Wrap`・`ArrowKeys`・`Thousands`・`AlignButton`) | S〜M | 列挙型がいくつか要る。デザイナーに出す |
| B11 | **グループの列** | `TRadioGroup`・`TCheckGroup` の `Columns`・`ColumnLayout`・`AutoFill`・`OnItemClick`・`OnSelectionChanged` | S〜M | |
| B12 | **TTreeView の細部** | `MultiSelect`・`MultiSelectStyle`・`Selections`、`SortType`・`OnCompare`・`AlphaSort`、ラベルの編集(`OnEditing`・`OnEdited`)、`Indent`・`HotTrack`・`RightClickSelect`・`ToolTips`・`Options`、`OnCustomDrawItem` | M〜L | 複数選択とラベルの編集は [component-coverage.md](component-coverage.md) でも未対応とした |
| B13 | **TListView の細部** | `ShowColumnHeaders`・`ColumnClick`・`SortType`・`OnCompare`・`AlphaSort`、仮想モード(`OwnerData`・`OnData`)、ラベルの編集、`OnCustomDrawItem`・`OwnerDraw`・`OnDrawItem`、`ToolTips` | M〜L | |
| B14 | **グリッドの細部** | 編集(`OnGetEditText`・`OnSetEditText`・`OnValidateEntry`・`AutoEdit`)、`OnPrepareCanvas`、`Columns`(`TGridColumns`)、`Objects`・`Cols`・`Rows`、`FocusColor`・`GridLineColor`・`GridLineWidth`・`AlternateColor`・`TitleFont`・`AutoFillColumns`・`OnTopLeftChanged`、セルの編集の部品(`OnSelectEditor`・`OnButtonClick`・`OnPickListSelect`)、並べ替え(`OnCompareCells`・`ColumnClickSorts`)、行・列の挿入・削除・移動の通知(`OnColRowInserted`・`OnColRowDeleted`・`OnColRowMoved`)、チェックボックスの列(`OnGetCheckboxState`・`OnSetCheckboxState`) | M〜L | Columns はコレクション |
| B15 | **アイコン** | `TIcon`、`TForm.Icon`・`Application->Icon` | M | グラフィックの基盤([ADR 0029](adr/0029-graphics-picture-image-glyph.md))に TIcon を足す |
| B16 | **ファイルのドロップ** | `TCustomForm.AllowDropFiles`・`OnDropFiles` | M | ファイル名の配列を受けるイベント |
| B17 | **コントロールの枠** | `BorderStyle`(`bsNone`・`bsSingle`)を TCustomEdit(TEdit・TMemo 等)・TCustomListBox・TCustomComboBox・TCustomListView・TCustomControl に、`TWinControl.BorderWidth` | S〜M | 枠を消した入力欄などに使う。フォームの `BorderStyle`(A3)とは型が違う(LCL では同じ `TBorderStyle` の一部) |

### Tier C — 低優先・特殊(必要になった時点で個別に対応)

| 項目 | 備考 |
|---|---|
| `OnContextPopup`(右クリックのメニューを出す前)・`OnMouseWheelDown`・`OnMouseWheelUp`・`OnMouseWheelHorz` 等、`OnChangeBounds`・`OnShowHint`・`OnConstrainedResize`・`OnEditingDone`・`OnShortCut`・`OnWindowStateChange` | 実装済みの `PopupMenu`・`OnMouseWheel`・`OnResize` で足りることが多い |
| `AlphaBlend`・`AlphaBlendValue`・`ShowInTaskBar`・`PopupMode`・`PopupParent` | |
| TMaskEdit の `SpaceChar`・`OnValidationError`・`ValidationErrorMode`・`EnableSets`、TFloatSpinEdit の `EditorEnabled` | |
| TTimer の `OnStartTimer`・`OnStopTimer` | |
| メニューの細部(`RightJustify`・`ShortCutKey2`・`ShowAlwaysCheckable`・`AutoLineReduction`・`Find`、TPopupMenu の `Alignment`・`TrackButton`) | |
| TToolBar・TCoolBar・THeaderControl・TImageList・TBitBtn・TSpeedButton の細部(`OnPaintButton`・`BandMaximize`・`AllocBy`・`ShareImages`・`ShowCaption`・`Transparent` 等) | |
| TOpenDialog の `OnFolderChange`・`OnSelectionChange`・`OptionsEx`、TFontDialog の `OnApplyClicked`・`PreviewText`、`SelectDirectory`(関数)・`TTaskDialog` | |
| クラス単位の Tier 6(TTrayIcon・TDateTimePicker・TCalendar 等) | [component-coverage.md](component-coverage.md) の Tier 6 |
| その他の細部(TTreeView・グリッドの色やカーソル、TTabControl の `TabHeight`・`TabWidth`、画像の添字の `HotImageIndex` 等、TPen の `EndCap`・`JoinStyle` 等) | 付録の一覧を参照。必要になった時点で Tier に入れる |

## 4. 対象外(意図して扱わない)

| 項目 | 理由 |
|---|---|
| ドッキング(`Dock…`・`OnDock…`・`UseDockManager` 等) | 現代のアプリではほとんど使わず、LCL の Win32 でも実装が限られる |
| コントロールのドラッグ&ドロップ(`DragMode`・`DragKind`・`OnDragDrop`・`OnDragOver`・`OnStartDrag`・`OnEndDrag`) | 使い道が限られる。必要になれば Tier C として再検討する(ファイルのドロップは B16) |
| 右から左の表示(`BiDiMode` 等)・ヘルプ(`HelpContext`・`HelpKeyword`・`OnHelp` 等)・アクセシビリティ(`Accessible…`) | 対象の利用者(日本語・英語の Windows のアプリ)で使わない |
| `AnchorSide…`・`ChildSizing` | 配置は `Align`・`Anchors`・`BorderSpacing`・`Constraints` で行う([ADR 0034](adr/0034-designer-common-properties.md)) |
| 高 DPI の画像の幅(`ImagesWidth` 等)・設計時だけのもの(`DesignTimePPI`・`LCLVersion`・`PixelsPerInch`・`Scaled`・`SessionProperties`) | |
| `DoubleBuffered`・`OnUTF8KeyPress` | Win32 の LCL では効果が限られる。C++・Python の文字列は UTF-8 のため、`OnKeyPress` で足りる |

## 5. 進め方

- **Tier A をバッチに分けて進める。** 1 バッチごとに ADR を書き、DLL・内部層・C++・テストを変え、Python の API とデザイナーのカタログを生成し直す。
  1. **A1〜A4**(ダイアログの作り方): `ModalResult`・メッセージのダイアログ・フォームの表示・既定のボタン。
     メモ帳のサンプルの `ConfirmSaveForm` を `MessageDlg` に、`AboutForm` の閉じ方をボタンの `ModalResult` に書き直して確かめる。
  2. **A5〜A9**(入力と表示): フォーカス・表示の更新・テキストの編集・TMemo・TLabel。サンプルに検索(TFindDialog)を加えて確かめる。
  3. **A10〜A12**(リスト・チェック・Application)。
- **Tier B は、項目ごとに設計(ADR)を切る。** B1(ステータスバーのパネル)・B4(Action)・B5(フォームへの描画)は、C++Builder のアプリで特によく使う。
- **デザイナー**: published のプロパティ・イベントは、カタログを抽出し直せばデザイナーのプロパティ・イベントのタブに出る。
  新しい列挙型・集合型は、カタログの型とデザイナーの入力部品(選択肢・チェック)に対応させる。
  `ModalResult`・`Default`・`Cancel` のように、デザイナーでの見た目(既定のボタンの枠)に関わるものはキャンバスの描画も直す。
- **棚卸しの更新**: 実装したら `python py/member_gap.py` の出力と本書の Tier 表を見直す。

## 付録: 洗い出しの出力(2026-09-29)

Tier 表は、この出力を手で分類したもの。Tier 表に無い項目も、ここには載っている。

<details>
<summary><code>python py/member_gap.py</code> の出力</summary>

### 未実装の published のメンバ(宣言した LCL のクラスごと)

- TBevel: OnMouseWheelDown, OnMouseWheelUp, OnPaint
- TBitBtn: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TBoundLabel: Alignment, Layout, OnMouseWheelDown, OnMouseWheelUp, ShowAccelChar, WordWrap
- TBrush: Style
- TButton: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TCanvas: AntialiasingMode, AutoRedraw, CopyMode, Height, OnChange, OnChanging, Region, Width
- TCheckBox: OnContextPopup, OnEditingDone, OnMouseWheelDown, OnMouseWheelUp
- TCheckGroup: OnMouseWheelDown, OnMouseWheelUp
- TCheckListBox: OnContextPopup, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight
- TComboBox: BorderStyle, ItemHeight, ItemWidth, MaxLength, OnCloseUp, OnContextPopup, OnDrawItem, OnDropDown, OnEditingDone, OnGetItems, OnMeasureItem, OnMouseWheelDown, OnMouseWheelUp, OnSelect, Sorted
- TControl: Action, ClientHeight, ClientWidth, OnChangeBounds, OnShowHint
- TControlBorderSpacing: CellAlignHorizontal, CellAlignVertical, OnChange
- TCoolBand: BorderStyle, ParentBitmap
- TCoolBar: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TCustomBitBtn: DefaultCaption, DisabledImageIndex, GlyphShowMode, HotImageIndex, PressedImageIndex
- TCustomButton: Cancel, Default, ModalResult
- TCustomCheckBox: Alignment, AllowGrayed, OnChange, State
- TCustomCheckGroup: AutoFill, ColumnLayout, Columns, OnItemClick
- TCustomCheckListBox: AllowGrayed, HeaderBackgroundColor, HeaderColor, OnItemClick
- TCustomComboBox: ArrowKeysTraverseList, AutoComplete, AutoCompleteText, AutoDropDown, AutoSelect, CharCase, DropDownCount, ReadOnly, Style, TextHint
- TCustomControl: BorderStyle, OnPaint
- TCustomCoolBar: BandBorderStyle, BandMaximize
- TCustomDrawGrid: AutoAdvance, AutoFillColumns, Columns, FadeUnfocusedSelection, Flat, FocusColor, FocusRectVisible, GridLineColor, GridLineStyle, GridLineWidth, OnAfterSelection, OnBeforeSelection, OnButtonClick, OnColRowDeleted, OnColRowExchanged, OnColRowInserted, OnColRowMoved, OnCompareCells, OnContextPopup, OnEditButtonClick, OnGetEditMask, OnGetEditText, OnHeaderSized, OnHeaderSizing, OnMouseWheelDown, OnMouseWheelUp, OnPickListSelect, OnPrepareCanvas, OnSelectEditor, OnSetEditText, OnTopleftChanged, OnValidateEntry, Options2, ScrollBars, TabAdvance, UseXORFeatures
- TCustomEdit: Alignment, BorderStyle, CharCase, EchoMode, HideSelection, NumbersOnly, PasswordChar, TextHint
- TCustomFloatSpinEdit: EditorEnabled
- TCustomForm: ActiveControl, AllowDropFiles, AlphaBlend, AlphaBlendValue, AutoScroll, BorderIcons, FormStyle, Icon, KeyPreview, OnDropFiles, OnWindowStateChange, PopupMode, PopupParent, Position, ShowInTaskBar, WindowState
- TCustomGroupBox: ParentBackground
- TCustomImage: AntialiasingMode, KeepOriginXWhenClipped, KeepOriginYWhenClipped, OnMouseWheelDown, OnMouseWheelUp, OnPaintBackground
- TCustomImageList: AllocBy, BlendColor, ImageType, ShareImages
- TCustomListBox: BorderStyle, ClickOnSelChange, Columns, ExtendedSelect, IntegralHeight, ItemHeight, MultiSelect, OnDrawItem, OnMeasureItem, OnMouseWheelDown, OnMouseWheelUp, OnSelectionChange, Options, ScrollWidth, Sorted, Style, TopIndex
- TCustomListView: BorderStyle, IconOptions, OwnerData
- TCustomMaskEdit: EnableSets, OnValidationError, ValidationErrorMode
- TCustomMemo: WantReturns, WantTabs, WordWrap
- TCustomPanel: Alignment, BevelColor, BevelInner, BevelOuter, BevelWidth, FullRepaint, ParentBackground
- TCustomProgressBar: BarShowText, Orientation, Smooth, Step, Style
- TCustomRadioGroup: AutoFill, ColumnLayout, Columns, OnItemEnter, OnItemExit, OnSelectionChanged
- TCustomScrollBar: LargeChange, OnScroll, SmallChange
- TCustomShape: OnShapeClick, OnShapePoints
- TCustomSpeedButton: Alignment, DisabledImageIndex, HotImageIndex, PressedImageIndex, SelectedImageIndex, ShowCaption, Transparent
- TCustomSplitter: OnCanOffset, OnCanResize
- TCustomStaticText: Alignment, FocusControl, ShowAccelChar, Transparent
- TCustomStringGrid: ExtendedSelect
- TCustomTabControl: HotTrack, MultiSelect, OnCloseTabClicked, OnGetImageIndex, Options, OwnerDraw, RaggedRight, ScrollOpposite, Style, TabHeight, TabWidth
- TCustomTrackBar: Frequency, LineSize, Orientation, PageSize, Reversed, ScalePos, SelEnd, SelStart, ShowSelRange, TickMarks, TickStyle
- TCustomTreeView: BackgroundColor, DefaultItemHeight, ExpandSignColor, ExpandSignSize, ExpandSignType, MultiSelectStyle, Options, ScrollBars, SelectionColor, SelectionFontColor, SelectionFontColorUsed, SeparatorColor, TreeLineColor, TreeLinePenStyle
- TDrawGrid: AlternateColor, AutoEdit, ColRowDragIndicatorColor, ColRowDraggingCursor, ColSizingCursor, ColumnClickSorts, ExtendedSelect, HeaderHotZones, HeaderPushZones, ImageIndexSortAsc, ImageIndexSortDesc, MouseWheelOption, OnCheckboxToggled, OnEditingDone, OnGetCellHint, OnGetCheckboxState, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnSetCheckboxState, OnUserCheckboxBitmap, OnUserCheckboxImage, RangeSelectMode, RowSizingCursor, TitleFont, TitleImageList, TitleImageListWidth, TitleStyle
- TEdit: AutoSelect, OnContextPopup, OnEditingDone, OnMouseWheelDown, OnMouseWheelUp
- TFileDialog: OnTypeChange
- TFloatSpinEdit: AutoSelect, OnEditingDone, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp
- TFont: CharSet, Height, Orientation, Pitch, Quality
- TFontDialog: OnApplyClicked, PreviewText
- TForm: OnConstrainedResize, OnContextPopup, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp, OnShortCut
- TGroupBox: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- THeaderControl: OnContextPopup, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp
- TImage: OnContextPopup, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnPaint
- TLabel: Alignment, FocusControl, Layout, OnContextPopup, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp, OptimalFill, ShowAccelChar, Transparent, WordWrap
- TLabeledEdit: AutoSelect, OnContextPopup, OnEditingDone, OnMouseWheelDown, OnMouseWheelUp
- TListBox: OnContextPopup, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight
- TListColumn: MaxWidth, MinWidth, SortIndicator, Tag
- TListView: AllocBy, AutoSort, AutoSortIndicator, AutoWidthLastColumn, ColumnClick, OnAdvancedCustomDraw, OnAdvancedCustomDrawItem, OnAdvancedCustomDrawSubItem, OnChanging, OnCompare, OnContextPopup, OnCustomDraw, OnCustomDrawItem, OnCustomDrawSubItem, OnData, OnDataFind, OnDataHint, OnDataStateChange, OnDrawItem, OnEdited, OnEditing, OnInsert, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp, OwnerDraw, ScrollBars, ShowColumnHeaders, ToolTips
- TMainMenu: OnChange
- TMaskEdit: AutoSelect, OnEditingDone, OnMouseWheelDown, OnMouseWheelUp, SpaceChar
- TMemo: OnContextPopup, OnEditingDone, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp
- TMenu: AutoLineReduction, OnDrawItem, OnMeasureItem, OwnerDraw
- TMenuItem: Action, AutoLineReduction, GlyphShowMode, OnDrawItem, OnMeasureItem, RightJustify, ShortCutKey2, ShowAlwaysCheckable
- TOpenDialog: OnFolderChange, OnSelectionChange, OptionsEx
- TPageControl: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TPaintBox: OnContextPopup, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp
- TPanel: OnContextPopup, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp, ShowAccelChar, VerticalAlignment, Wordwrap
- TPen: Cosmetic, EndCap, JoinStyle, Mode, Style
- TPopupMenu: Alignment, TrackButton
- TProgressBar: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TRadioButton: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TRadioGroup: OnMouseWheelDown, OnMouseWheelUp
- TScrollBar: OnContextPopup
- TScrollBox: AutoScroll, OnConstrainedResize, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp, ParentBackground
- TScrollingWinControl: HorzScrollBar, VertScrollBar
- TShape: OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp, OnPaint
- TSizeConstraints: OnChange
- TSpeedButton: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp, OnPaint
- TSpinEdit: AutoSelect, OnEditingDone, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp
- TSplitter: OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp
- TStaticText: OnContextPopup, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp
- TStatusBar: AutoHint, OnContextPopup, OnDrawPanel, OnHint, OnMouseWheelDown, OnMouseWheelUp, Panels, SizeGrip, UseSystemFont
- TStringGrid: AlternateColor, AutoEdit, CellHintPriority, ColRowDragIndicatorColor, ColRowDraggingCursor, ColSizingCursor, ColumnClickSorts, HeaderHotZones, HeaderPushZones, ImageIndexSortAsc, ImageIndexSortDesc, MouseWheelOption, OnCellProcess, OnCheckboxToggled, OnEditingDone, OnGetCellHint, OnGetCheckboxState, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnSetCheckboxState, OnTopLeftChanged, OnUserCheckboxBitmap, OnUserCheckboxImage, RangeSelectMode, RowSizingCursor, TitleFont, TitleImageList, TitleStyle
- TTabControl: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TTabSheet: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TTimer: OnStartTimer, OnStopTimer
- TToggleBox: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TToolBar: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp, OnPaintButton
- TToolButton: OnContextPopup, OnMouseWheelDown, OnMouseWheelUp
- TTrackBar: OnContextPopup, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp
- TTreeView: DisabledFontColor, HotTrack, HotTrackColor, Indent, MultiSelect, OnAddition, OnAdvancedCustomDraw, OnAdvancedCustomDrawItem, OnCompare, OnContextPopup, OnCustomCreateItem, OnCustomDraw, OnCustomDrawArrow, OnCustomDrawItem, OnEdited, OnEditing, OnEditingEnd, OnGetImageIndex, OnGetSelectedIndex, OnHasChildren, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp, OnNodeChanged, OnSelectionChanged, RightClickSelect, ShowSeparators, SortType, ToolTips
- TUpDown: AlignButton, ArrowKeys, Flat, MinRepeatInterval, OnChanging, OnChangingEx, OnContextPopup, OnMouseWheelDown, OnMouseWheelHorz, OnMouseWheelLeft, OnMouseWheelRight, OnMouseWheelUp, Orientation, Thousands, Wrap
- TWinControl: BorderWidth, OnEnter, OnExit

### 対象外(ドッキング・ドラッグ・右から左・ヘルプ・高 DPI 等)

- TBitBtn: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TBoundLabel: DragCursor, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag
- TButton: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TCheckBox: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TCheckGroup: DragCursor, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag
- TCheckListBox: BidiMode, DragCursor, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TComboBox: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TCommonDialog: HelpContext
- TControl: AnchorSideBottom, AnchorSideLeft, AnchorSideRight, AnchorSideTop, BiDiMode, HelpContext, HelpKeyword, HelpType, ParentBiDiMode
- TCoolBar: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnGetSiteInfo, OnStartDock, OnStartDrag
- TCustomBitBtn: ImageWidth
- TCustomCoolBar: ImagesWidth
- TCustomDesignControl: DesignTimePPI, PixelsPerInch, Scaled
- TCustomDrawGrid: OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnStartDock, OnStartDrag
- TCustomForm: DefaultMonitor, HelpFile, OnHelp, ScreenSnap, SnapBuffer, SnapOptions
- TCustomHeaderControl: ImagesWidth, OnCreateSectionClass
- TCustomImage: ImageWidth
- TCustomImageList: OnGetWidthForPPI, Scaled
- TCustomSpeedButton: ImageWidth
- TCustomTabControl: ImagesWidth
- TCustomTreeView: ImagesWidth, StateImagesWidth
- TDrawGrid: DragCursor, DragKind, DragMode
- TEdit: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TFileDialog: OnHelpClicked
- TFindDialog: OnHelpClicked
- TForm: DragKind, DragMode, LCLVersion, OnDragDrop, OnDragOver, OnEndDock, OnGetSiteInfo, OnStartDock, SessionProperties
- TGroupBox: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnGetSiteInfo, OnStartDock, OnStartDrag, ParentBidiMode
- THeaderControl: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag
- TImage: DragCursor, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag
- TLabel: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TLabeledEdit: BidiMode, DragCursor, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TListBox: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TListView: DragCursor, DragKind, DragMode, LargeImagesWidth, OnCreateItemClass, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnStartDock, OnStartDrag, SmallImagesWidth, StateImagesWidth
- TMaskEdit: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnStartDock, OnStartDrag
- TMemo: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TMenu: BidiMode, ImagesWidth, ParentBidiMode
- TMenuItem: HelpContext, SubMenuImagesWidth
- TPageControl: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnGetDockCaption, OnGetSiteInfo, OnStartDock, OnStartDrag
- TPaintBox: DragCursor, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag
- TPanel: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnGetDockCaption, OnGetSiteInfo, OnStartDock, OnStartDrag, ParentBidiMode
- TPopupMenu: HelpContext
- TProgressBar: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDock, OnStartDrag
- TRadioButton: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TRadioGroup: BidiMode, DragCursor, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TScrollBar: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TScrollBox: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnGetSiteInfo, OnStartDock, OnStartDrag
- TShape: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnStartDock, OnStartDrag
- TSpeedButton: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TStaticText: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TStatusBar: DragCursor, DragKind, DragMode, OnCreatePanelClass, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnStartDock, OnStartDrag
- TStringGrid: DragCursor, DragKind, DragMode
- TTabControl: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnGetSiteInfo, OnStartDock, OnStartDrag
- TTabSheet: OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag
- TToggleBox: BidiMode, DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag, ParentBidiMode
- TToolBar: DragCursor, DragKind, DragMode, ImagesWidth, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag
- TToolButton: DragCursor, DragKind, DragMode, OnDragDrop, OnDragOver, OnEndDock, OnEndDrag, OnStartDock, OnStartDrag
- TTrackBar: DragCursor, DragMode, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag
- TTreeView: DragCursor, DragKind, DragMode, OnCreateNodeClass, OnDragDrop, OnDragOver, OnEndDrag, OnStartDrag
- TWinControl: ChildSizing, DockSite, DoubleBuffered, OnDockDrop, OnDockOver, OnUTF8KeyPress, OnUnDock, ParentDoubleBuffered, UseDockManager

### VCL でよく使う public のメンバのうち未実装のもの

- TControl: Refresh, Repaint, Invalidate, Update, BringToFront, SendToBack, SetBounds, ClientToScreen, ScreenToClient, ClientWidth, ClientHeight, BoundsRect
- TWinControl: SetFocus, CanFocus, Focused, Controls, ControlCount, OnEnter, OnExit
- TCustomEdit: SelStart, SelLength, SelText, SelectAll, ClearSelection, CopyToClipboard, CutToClipboard, PasteFromClipboard, Undo, CanUndo, Clear, Modified, CaretPos
- TCustomMemo: Append, WordWrap, WantReturns, WantTabs
- TCustomForm: ModalResult, ActiveControl, WindowState, BorderStyle, BorderIcons, Position, FormStyle, KeyPreview, Icon, CloseQuery
- TCustomButton: ModalResult, Default, Cancel, Click
- TCustomListBox: Selected, Sorted, TopIndex, ItemAtPos, MultiSelect, SelCount, ClearSelection, SelectAll, Style, OnDrawItem, ItemHeight
- TCustomComboBox: Style, DropDownCount, Sorted, SelectAll, SelStart, SelLength, SelText, AutoComplete, DroppedDown, OnSelect, OnDropDown, OnCloseUp
- TApplication: MessageBox, Icon, ExeName, OnException, OnIdle, Minimize, Restore, BringToFront, HintPause, HintHidePause, ShowHint
- TCanvas: TextWidth, TextHeight, TextRect, Polygon, Polyline, RoundRect, Arc, Pie, FrameRect, CopyRect

</details>
