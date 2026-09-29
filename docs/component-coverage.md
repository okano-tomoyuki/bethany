# コントロールの網羅性(VCL 移行を見据えた棚卸し)

Bethany を「本格的に VCL から移行できるレベル」にするための、LCL の標準コントロール・非ビジュアルコンポーネントの
実装状況の棚卸し。2026-09-26 時点で LCL(`C:\tool\lazarus\lcl`)のソースを確認して作成した。
VCL と LCL はクラス名・プロパティ名がほぼ一致するため、基本的に LCL のクラス名がそのまま VCL 移行時の対応クラス名になる
(差異がある場合は備考に記載)。

方針は [ADR 0007](adr/0007-lcl-faithful-hierarchy.md) と同じ:
LCL の継承関係の部分列にする。実装するクラスの基底が Bethany に無ければ、その基底から実装する。

実装済みのクラスに足りないメンバ(`ModalResult`・`WordWrap` 等)とグローバルな関数(`MessageDlg` 等)は、[member-coverage.md](member-coverage.md) で扱う。

## 1. 実装済み(37 クラス)

TObject, TPersistent, TComponent, TControl, TWinControl, TGraphicControl, TCustomControl, TScrollingWinControl,
TCustomForm, TForm, TApplication, TCustomPanel, TPanel, TCustomGroupBox, TGroupBox, TCustomLabel, TLabel,
TButtonControl, TCustomButton, TButton, TCustomCheckBox, TCheckBox, TRadioButton, TCustomEdit, TEdit,
TCustomMemo, TMemo, TCustomComboBox, TComboBox, TCustomListBox, TListBox, TPen, TBrush, TFont, TCanvas,
TPaintBox, TCustomTimer, TTimer。

詳細は [class-hierarchy.md](class-hierarchy.md) を参照。

## 2. 未実装(優先度別)

複雑度の目安:
- **低**: 既存のプロパティ・イベントのパターン(Property<T> + protected hack + BridgeFor)をそのまま適用できる。
- **中**: TStrings 相当のコレクション操作や、専用のイベント形が新たに必要。
- **高**: 新しい補助クラス(TTreeNodes・TListItems 等)や、グラフィックス基盤(TBitmap・TPicture)が前提になる。

### Tier 1 — 低コスト・高価値(優先して着手)

これらは基底が TWinControl・TCustomControl・TGraphicControl・TCustomGroupBox・TCustomListBox・TCustomEdit・
TScrollingWinControl のいずれかで、いずれも実装済み。既存パターンの延長で追加できる。

| クラス | LCL 宣言ユニット | LCL での基底 | 備考 |
|---|---|---|---|
| ✅ TScrollBox | forms.pp | TScrollingWinControl(実装済み) | 追加のメンバ無し([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md)) |
| ✅ TToggleBox | stdctrls.pp | TCustomCheckBox(実装済み) | 追加のメンバ無し(Checked を共有、ADR 0015) |
| ✅ TBevel | extctrls.pp | TGraphicControl(実装済み) | Shape/Style(ADR 0015) |
| ✅ TShape | extctrls.pp(TCustomShape) | TGraphicControl(実装済み) | Shape/Pen/Brush(ADR 0015) |
| ✅ TStaticText | stdctrls.pp(TCustomStaticText) | TWinControl(実装済み) | BorderStyle(ADR 0015) |
| ✅ TStatusBar | comctrls.pp | TWinControl(実装済み) | SimpleText/SimplePanel と Panels([ADR 0044](adr/0044-statusbar-panels-and-designer-collections.md))。Run() 開始前の生成が失敗する LCL 側の問題は DLL 側で回避済み([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md)) |
| ✅ TSplitter | extctrls.pp(TCustomSplitter) | TCustomControl(実装済み) | `TControl.Align` の追加とあわせて実装([ADR 0016](adr/0016-control-align-and-splitter.md))。AutoSnap/Beveled/MinSize/ResizeAnchor/ResizeStyle/SplitterPosition/OnMoved。OnCanResize/OnCanOffset は未対応 |
| ✅ TScrollBar | stdctrls.pp(TCustomScrollBar) | TWinControl(実装済み) | Kind/Min/Max/Position/PageSize/OnChange(ADR 0015 の 2 バッチ目) |
| ✅ TRadioGroup | extctrls.pp(TCustomRadioGroup) | TCustomGroupBox(実装済み) | Items/ItemIndex/OnClick(ADR 0015 の 3 バッチ目)。OnClick は TControl のものとは別の独自フィールド |
| ✅ TCheckGroup | extctrls.pp(TCustomCheckGroup) | TCustomGroupBox(実装済み) | Items + インデックス付き Checked(ADR 0015 の 3 バッチ目) |
| ✅ TCheckListBox | checklst.pas(TCustomCheckListBox) | TCustomListBox(実装済み) | 基底の Items をそのまま使い、インデックス付き Checked と OnClickCheck を追加(ADR 0015 の 3 バッチ目) |
| ✅ TLabeledEdit | extctrls.pp(TCustomLabeledEdit) | TCustomEdit(実装済み) | EditLabel(LCL が内部で生成する TBoundLabel)は `WrapExisting` でラップする。LabelPosition/LabelSpacing([ADR 0028](adr/0028-labelededit-and-stringlist.md)) |
| ✅ TSpeedButton | buttons.pp(TCustomSpeedButton) | TGraphicControl(実装済み) | Down/GroupIndex/Flat/AllowAllUp(ADR 0015 の 4 バッチ目)。Glyph/NumGlyphs/Layout/Margin/Spacing は Tier 3 の 1 バッチ目で追加([ADR 0029](adr/0029-graphics-picture-image-glyph.md)) |
| ✅ TBitBtn | buttons.pp(TCustomBitBtn) | TCustomButton(実装済み) | Kind(bkOK 等)のみ実装(ADR 0015 の 4 バッチ目)。Kind を設定すると LCL が既定の Caption を自動設定する。Glyph/NumGlyphs/Layout/Margin/Spacing は Tier 3 の 1 バッチ目で追加(ADR 0029。Glyph を設定すると Kind は bkCustom に戻る) |
| ✅ TSpinEdit / TFloatSpinEdit | spin.pp | TCustomEdit(実装済み) | TCustomSpinEdit が Value 等を Integer で再宣言(TCustomFloatSpinEdit の Double 版を隠す)。C++ でも `Property<int>` で `Property<double>` を隠す形で再現した(ADR 0015 の 5 バッチ目) |
| ✅ TMaskEdit | maskedit.pp(TCustomMaskEdit) | TCustomEdit(実装済み) | EditMask(ADR 0015 の 5 バッチ目)。EditText 等は未対応 |
| ✅ TTrackBar | comctrls.pp(TCustomTrackBar) | TWinControl(実装済み) | Min/Max/Position/OnChange。Application->Run() 開始前の生成でも問題が無いことを確認済み(ADR 0015 の 2 バッチ目) |
| ✅ TProgressBar | comctrls.pp(TCustomProgressBar) | TWinControl(実装済み) | Min/Max/Position のみで表示専用。問題無しを確認済み(ADR 0015 の 2 バッチ目) |
| ✅ TUpDown | comctrls.pp(TCustomUpDown) | TCustomControl(実装済み) | Min/Max/Position/Increment/Associate(対象の Edit と連動)。問題無しを確認済み(ADR 0015 の 2 バッチ目) |
| ✅ TTabControl | comctrls.pp(TCustomTabControl) | TWinControl(実装済み) | Tabs/TabIndex/OnChange のみ(単純なタブ切り替え UI。ADR 0015 の 6 バッチ目) |
| TPageControl + TTabSheet | comctrls.pp | TWinControl | 所有ページ(TTabSheet)の生成・破棄の設計が要るため Tier 1 では見送り、Tier 2 に位置づけ直した(Tier 2 の 1 バッチ目で実装済み) |

### Tier 2 — 中コスト(新しいコレクション型が必要)

| クラス | LCL 宣言ユニット | LCL での基底 | 必要になる補助 |
|---|---|---|---|
| ✅ TPageControl + TTabSheet | comctrls.pp(TCustomTabControl 系) | TWinControl | Tier 2 の 1 バッチ目([ADR 0018](adr/0018-pagecontrol-and-tabsheet.md))。AddTabSheet のページは `WrapExisting` でラップ。Clear は LCL の遅延破棄。Images/ImageIndex は Tier 3 の 2 バッチ目で追加(ADR 0030)。Style/Options は未対応 |
| ✅ TTreeView | comctrls.pp(TCustomTreeView) | TCustomControl(実装済み) | Tier 2 の 2 バッチ目([ADR 0019](adr/0019-treeview-and-non-component-items.md))。TTreeNode(TPersistent)は `TTreeNode*` で扱い、削除通知(TCustomTreeView.Delete の上書き)でラッパーの寿命を管理。Images/StateImages とノードの ImageIndex/SelectedIndex/StateIndex/OverlayIndex は Tier 3 の 2 バッチ目で追加(ADR 0030)。複数選択・並べ替え・ラベルの編集・独自の描画は Tier B で追加([ADR 0051](adr/0051-treeview-details.md)) |
| ✅ TListView | comctrls.pp(TCustomListView) | TWinControl(実装済み) | Tier 2 の 3 バッチ目([ADR 0020](adr/0020-listview-and-shared-item-registry.md))。TListItem・TListColumn は TTreeNode と同じく削除通知で寿命を管理(`ItemRegistry` に共通化)。表示前の Selected は DLL 側で補正。LargeImages/SmallImages/StateImages と項目・列の ImageIndex は Tier 3 の 2 バッチ目で追加(ADR 0030)。OwnerData・ラベルの編集は未対応 |
| ✅ TStringGrid / TDrawGrid | grids.pas(TCustomGrid → TCustomDrawGrid) | TCustomControl(実装済み) | Tier 2 の 4 バッチ目([ADR 0021](adr/0021-drawgrid-and-stringgrid.md))。Cells[c][r]・ColWidths[i] は添字で書ける([ADR 0022](adr/0022-indexed-property-proxy.md))、Options はビット集合、OnDrawCell/OnSelectCell/OnSelection/OnHeaderClick。Objects・Cols/Rows・OnGetEditText/OnSetEditText・Columns は未対応 |
| ✅ THeaderControl | comctrls.pp(TCustomHeaderControl) | TCustomControl(実装済み) | Tier 2 の 5 バッチ目([ADR 0024](adr/0024-headercontrol.md))。Sections(Items[i]・Add・Insert・Delete・Clear)、DragReorder、OnSectionClick/Resize/Track/Drag/EndDrag/SeparatorDblClick。セクションの破棄は CreateSection を差し替えた派生セクションのデストラクタで通知。Images/ImageIndex は Tier 3 の 2 バッチ目で追加(ADR 0030) |
| ✅ TToolBar / TToolButton | comctrls.pp | TToolWindow(TCustomControl 系。実質は TCustomControl) | Tier 2 の 6 バッチ目([ADR 0025](adr/0025-toolbar-and-toolbutton.md))。ボタンは Parent をツールバーにして追加(TComponent なので寿命は既存の仕組み)。Style/Down/Grouped/DropdownMenu/MenuItem/OnArrowClick、Buttons[i]、EdgeBorders 等。Images/HotImages/DisabledImages/ImageIndex は Tier 3 の 2 バッチ目で追加(ADR 0030)、OnPaint/OnPaintButton は未対応 |
| ✅ TCoolBar | comctrls.pp(TCustomCoolBar) | TToolWindow(実装済み) | Tier 2 の 7 バッチ目([ADR 0026](adr/0026-coolbar-and-item-free-observer.md))。Bands(Items[i]・Add・Delete・Clear・FindBand)、バンドの Text/Width/Break/Control 等、GrabStyle・Vertical・OnChange。コントロールの Parent をクールバーにすると LCL がバンドを自動で追加する。Bitmap/Images とバンドの Bitmap/ImageIndex は Tier 3 の 2 バッチ目で追加(ADR 0030) |

### Tier 3 — グラフィックス基盤が前提

画像を扱うコントロールは、グラフィックス基盤(TBitmap・TPicture)を前提にする。
基盤は 1 バッチ目で入れた([ADR 0029](adr/0029-graphics-picture-image-glyph.md))。
TGraphic → TRasterImage → TCustomBitmap → TBitmap / TPortableNetworkGraphic / TJPEGImage と TPicture で、
利用者が new / delete するものと、所有者から中身を都度取得するビュー(TPicture は中身を作り直すため)がある。

| クラス | LCL 宣言ユニット | 必要な基盤 |
|---|---|---|
| ✅ TBitmap / TPortableNetworkGraphic / TJPEGImage / TPicture | graphics.pp | Tier 3 の 1 バッチ目(ADR 0029)。Width/Height/Empty/Transparent/LoadFromFile/SaveToFile/Assign/Clear、Canvas/PixelFormat/TransparentColor/TransparentMode/SetSize、TPicture の Graphic/Bitmap/PNG/Jpeg。あわせて TCanvas の Pixels/FillRect/Draw/StretchDraw と TControl.AutoSize を追加。TIcon・ストリーム・ScanLine は未対応 |
| ✅ TImage | extctrls.pp(TCustomImage) | Tier 3 の 1 バッチ目(ADR 0029)。Picture/Canvas/HasGraphic/Center/Stretch/StretchOutEnabled/StretchInEnabled/Proportional/Transparent/OnPictureChanged。Images/ImageIndex は 2 バッチ目で追加(ADR 0030) |
| ✅ TBitBtn の Glyph / TSpeedButton の Glyph | buttons.pp | Tier 3 の 1 バッチ目(ADR 0029)。Glyph/NumGlyphs/Layout/Margin/Spacing |
| ✅ TImageList | ImgList ユニット(TCustomImageList)・controls.pp(TImageList) | Tier 3 の 2 バッチ目([ADR 0030](adr/0030-imagelist-and-images.md))。Width/Height/Count/Masked/BkColor/DrawingStyle/OnChange、Add/AddSliced/AddMasked/Insert/Replace/Delete/Clear/Move/GetBitmap/Draw。あわせて各コントロール・項目の Images/ImageIndex(TImage・TBitBtn・TSpeedButton・TPageControl・TTabControl・TTreeView・TListView・TToolBar・THeaderControl・TCoolBar・TMainMenu/TPopupMenu)と、TMenuItem・TCoolBar・TCoolBand の Bitmap を追加した。LCL の Add は VCL と違い、幅が Width の倍数でも画像を分けない(分けるのは AddSliced)。ImagesWidth 等の高 DPI 向けのものは未対応 |

### Tier 4 — ダイアログ(非ビジュアル、Execute 呼び出しパターン)

VCL と同じく「プロパティを設定して Execute を呼び、結果を bool で受け取る」形。
Application と同様、C++ 側は値型ではなく生成・破棄が必要な TComponent 派生として扱う。

| クラス | LCL 宣言ユニット | 主なメンバ |
|---|---|---|
| ✅ TOpenDialog / TSaveDialog | dialogs.pp(TFileDialog) | Tier 4([ADR 0033](adr/0033-dialogs.md))。Execute/Title/OnShow/OnClose/OnCanClose(TCommonDialog)、FileName/Filter/FilterIndex/InitialDir/DefaultExt/Files(TFileDialog)、Options(ビット集合)。LCL の DefaultExt は先頭に `.` を補う。OnTypeChange/OnSelectionChange/OnFolderChange・OptionsEx は未対応 |
| ✅ TColorDialog | dialogs.pp | Color/CustomColors/Options(ADR 0033)。Options の既定値は cdFullOpen(VCL は空) |
| ✅ TFontDialog | dialogs.pp | Font(既存の TFont を流用。代入は内容のコピー)/MinFontSize/MaxFontSize/Options(ADR 0033)。あわせて TControl の Color・Font と、TFont の Style・Assign を追加した |
| ✅ TSelectDirectoryDialog | dialogs.pp(TOpenDialog 派生) | FileName(選択したディレクトリ)/Execute(ADR 0033) |
| ✅ TFindDialog / TReplaceDialog | dialogs.pp | FindText/ReplaceText/Options/Left/Top/OnFind/OnReplace/CloseDialog(ADR 0033)。モードレス(Execute はすぐ戻る) |
| TPrintDialog / TPrinterSetupDialog | printersdlgs.pp(`components/printers`。基底の TCustomPrintDialog 等は dialogs.pp) | 見送り(ADR 0033)。具象クラスが別パッケージにあり、ビルドの `-Fu` に無い。印刷基盤(TPrinter)とあわせて扱う |

### Tier 5 — メニュー

VCL アプリらしい UI に必須だが、TMenuItem がツリー構造の TComponent であるため、
コレクション操作(Items.Add / Items.Count / Items[i])を DLL の関数・C++ 双方で新たに設計する必要がある。

| クラス | LCL 宣言ユニット | LCL での基底 | 備考 |
|---|---|---|---|
| ✅ TMenuItem | menus.pp | TLCLComponent(Bethany では TComponent 直下に置く) | Caption/Checked/Enabled/Visible/AutoCheck/RadioItem/GroupIndex/Default/ShortCut/Hint/OnClick と、子の項目の操作(Items[i]/Count/Add/Insert/Delete/Remove/Clear/IndexOf/AddSeparator)([ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md))。Bitmap/ImageIndex/SubMenuImages と TMenu.Images は Tier 3 の 2 バッチ目で追加(ADR 0030) |
| ✅ TMainMenu | menus.pp(TMenu) | TLCLComponent → TComponent | フォームに割り当てる(TForm.Menu)。Items(ルート項目)は LCL が内部で生成するため、`WrapExisting` でラップする(ADR 0017)。Merge は未対応 |
| ✅ TPopupMenu | menus.pp(TMenu) | TLCLComponent → TComponent | コントロールに割り当てる(TControl.PopupMenu)。AutoPopup/PopupComponent/OnPopup/OnClose/Popup(X, Y)(ADR 0017) |

### Tier 6 — 低優先・特殊

| クラス | LCL 宣言ユニット | 備考 |
|---|---|---|
| TCalendar | calendar.pp | TWinControl 直下で単純だが、実用では日付選択に TDateTimePicker を使うことが多い |
| TDateTimePicker / 月表示カレンダー | `components/datetimectrls`(別パッケージ) | 現在の build-windows.sh / build-linux.sh の `-Fu` に無い別パッケージ。パス追加が前提 |
| TTrayIcon | extctrls.pp(TCustomTrayIcon) | 非ビジュアル。タスクトレイ常駐アプリ向け |
| TControlBar | extctrls.pp | 現代の UI ではほぼ使われない |
| TFlowPanel | extctrls.pp | TPanel で代替できることが多い |
| TNotebook / TPage(旧式ページ切替) | extctrls.pp | TPageControl に置き換えられた旧 API。移行元が古い VCL コードでなければ不要 |

### コントロールではないが VCL 移行でよく使うもの(参考)

`TAction`/`TActionList`(アクションベースの UI 更新)、`TScreen`(Application と対になるグローバル)、
`TClipboard` は「コントロール」ではないため今回の一覧から外しているが、
実際の VCL アプリの移行では必要になることが多い。着手する場合は別途 ADR で設計を切る。
単体の `TStringList` は、TStrings の派生として実装済み([ADR 0028](adr/0028-labelededit-and-stringlist.md))。

## 3. cross-cutting な既知の課題(特定のクラスではなく設計全体に関わるもの)

Tier 1 の 1 バッチ目([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md))を実装する過程で見つかった、
個別のクラスの追加では解決できない課題。

- ✅ **`TControl.Align` が無い。** → 解決済み([ADR 0016](adr/0016-control-align-and-splitter.md))。
  LCL と同じく TControl の public プロパティとして追加し、あわせて TSplitter を追加した。
  LCL では Align による配置がフォームの表示まで行われない(VCL と異なる)点に注意。
  AutoSize は [ADR 0029](adr/0029-graphics-picture-image-glyph.md)、Anchors・BorderSpacing・Constraints は
  [ADR 0034](adr/0034-designer-common-properties.md) で追加した(Anchors・BorderSpacing も表示まで配置に反映されない)。
- ✅ **LCL が内部で生成する子コンポーネントをラップできない。** → 仕組みとしては解決済み
  ([ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md))。ハンドルを返す DLL の関数の側で破棄通知に登録し、
  C++ 側は `TComponent::WrapExisting<T>` で初回アクセス時にラッパーを作る。TMenu.Items で初めて使った。
  以下は当初の記述。`TCustomLabeledEdit.EditLabel` のように、
  コンポーネントが自分の子を Pascal 側だけで生成する場合、その子は Bethany の `*_Create` を経由しないため
  C++ 側にラッパーが登録されない(`TComponent::FromHandle` が nullptr を返す)。この種のプロパティを
  公開するには、「既存のハンドルを受け取って、初回アクセス時に遅延でラッパーを生成する」ような仕組みが要る。
- **LCL の Win32 実装には、DLL(`IsLibrary`)のとき `WidgetSet.AppHandle` が 0 であることを前提にしていない
  箇所がある。** TStatusBar ではこれが原因で Run() 開始前の生成が失敗していた(DLL 側で回避済み)。
  新しいコントロールを追加する際は、Run() 開始前にフォームを表示しても問題が無いか確認する
  ([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md))。
- ✅ **LCL が送出した例外は、呼び出し側(C/C++)で捕捉できない。** → 解決済み([ADR 0031](adr/0031-exceptions-across-dll.md))。
  DLL の公開関数で例外を捕まえ、C はスレッドごとの直前のエラー(`beth_HasLastError` 等)、C++ は `beth::Exception` として受けられる。
  イベントのハンドラから送出した例外も、DLL 側で送出し直す(メッセージループの中なら LCL が処理する)。
  以下は当初の記述。範囲外の添字、ソートされた TStringList への Insert
  ([ADR 0028](adr/0028-labelededit-and-stringlist.md))、読み込めない画像ファイル([ADR 0029](adr/0029-graphics-picture-image-glyph.md))等が、
  呼び出し側で捕捉できなかった。

## 4. 推奨する着手順序

1. **Tier 1 は完了した**(19 クラス。[ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md))。
   TLabeledEdit(内部生成コンポーネントのラップ待ち。ADR 0017 で仕組みが入った後、[ADR 0028](adr/0028-labelededit-and-stringlist.md) で実装済み)・
   TPageControl+TTabSheet(所有ページの設計が必要、Tier 2 へ再分類)は cross-cutting な課題または
   複雑度の都合で見送った。
2. ✅ **`TControl.Align` の追加**(上記 cross-cutting な課題)と、それを待っていた TSplitter は完了した
   ([ADR 0016](adr/0016-control-align-and-splitter.md))。
3. ✅ **Tier 5(メニュー)** は完了した([ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md))。
   あわせて、内部生成コンポーネントのラップの仕組み(`WrapExisting`)が入った。
4. ✅ **Tier 2** はバッチに分けて進めた。1 バッチ目の TPageControl+TTabSheet([ADR 0018](adr/0018-pagecontrol-and-tabsheet.md))と、
   2 バッチ目の TTreeView([ADR 0019](adr/0019-treeview-and-non-component-items.md))、3 バッチ目の TListView
   ([ADR 0020](adr/0020-listview-and-shared-item-registry.md))、4 バッチ目の TDrawGrid/TStringGrid
   ([ADR 0021](adr/0021-drawgrid-and-stringgrid.md))、5 バッチ目の THeaderControl([ADR 0024](adr/0024-headercontrol.md))、6 バッチ目の TToolBar/TToolButton([ADR 0025](adr/0025-toolbar-and-toolbutton.md))、7 バッチ目の TCoolBar([ADR 0026](adr/0026-coolbar-and-item-free-observer.md))で完了した。TComponent ではない項目の寿命管理は
   `ItemRegistry` と項目の破棄通知(`ItemFree_SetCallback`)に共通化し、通知は TPersistent の観察者(`WatchItem`)から送る(ADR 0026)。
   あわせて、横断的な課題だったインデックス付きプロパティの添字の書き方([ADR 0022](adr/0022-indexed-property-proxy.md)・[0023](adr/0023-remaining-indexed-properties.md))と、
   TStrings(`Items->Add`・`Lines->Text` 等。[ADR 0027](adr/0027-tstrings.md))も VCL と同じ形にした。
5. **Tier 3(TBitmap/TPicture)** は、1 バッチ目でグラフィックス基盤・TImage・Glyph を入れた([ADR 0029](adr/0029-graphics-picture-image-glyph.md))。
   2 バッチ目で TImageList と各コントロールの Images/ImageIndex を入れ([ADR 0030](adr/0030-imagelist-and-images.md))、Tier 3 は完了した。以下は当初の記述。
   TImage 単体のためというより、Tier 1/2 のいくつか(Glyph・ImageList)の
   完成度を上げるために必要になる。着手するタイミングで独立した ADR を書く。
6. ✅ **Tier 4(ダイアログ)** は、印刷のダイアログを除いて完了した([ADR 0033](adr/0033-dialogs.md))。
   ダイアログの結果を適用する先として、TControl の Color・Font と TFont の Style・Assign もあわせて追加した。
7. **Tier 6** は必要になった時点で個別に対応する。
8. ✅ デザイナーアプリの設計の前提として、どのコントロールにもある共通のプロパティ(Anchors・BorderSpacing・Constraints・
   TabOrder・TabStop・Hint・ShowHint・Cursor・ParentColor・ParentFont・Tag)を追加した([ADR 0034](adr/0034-designer-common-properties.md))。
