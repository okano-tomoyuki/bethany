# クラス階層

no_vcl のクラス階層と、各メンバをどの階層に置くかの対応表。
方針の背景は [ADR 0007](adr/0007-lcl-faithful-hierarchy.md) を参照。

## 1. LCL の継承関係(ソースで確認済み)

Lazarus 同梱の LCL(`lcl/controls.pp`・`stdctrls.pp`・`extctrls.pp`・`forms.pp`・`menus.pp`・`customtimer.pas`・`graphics.pp`)と
FPC の `fcl-image/src/fpcanvas.pp` のクラス宣言から確認した継承関係(2026-09-26)。
TreeView・Grid 等の未実装クラスは確認していない。

```
TObject
└── TPersistent
    ├── TComponent
    │   ├── TCustomApplication ── TApplication              ※TCustomApplication は FCL 固有
    │   ├── TCustomTimer ── TTimer
    │   └── TLCLComponent                                  ※LCL 固有
    │       ├── TMenu ── TMainMenu / TPopupMenu             ※TControl ではない
    │       └── TControl
    │           ├── TGraphicControl
    │           │   ├── TCustomLabel ── TLabel
    │           │   ├── TCustomShape ── TShape
    │           │   ├── TBevel
    │           │   ├── TCustomImage ── TImage
    │           │   └── TPaintBox
    │           └── TWinControl
    │               ├── TCustomControl
    │               │   ├── TCustomPanel ── TPanel
    │               │   └── TScrollingWinControl
    │               │       └── TCustomDesignControl         ※LCL 固有
    │               │           └── TCustomForm ── TForm
    │               ├── TCustomEdit ── TEdit
    │               │   └── TCustomMemo ── TMemo
    │               ├── TCustomComboBox ── TComboBox
    │               ├── TCustomListBox ── TListBox
    │               ├── TCustomGroupBox ── TGroupBox
    │               └── TButtonControl
    │                   ├── TCustomButton ── TButton
    │                   └── TCustomCheckBox ── TCheckBox / TRadioButton
    ├── TFPCanvasHelper                                    ※FPC 固有
    │   └── TFPCustomPen / TFPCustomBrush / TFPCustomFont
    │       └── TPen / TBrush / TFont
    ├── TFPCustomCanvas ── TCanvas                         ※FPC 固有
    ├── TGraphic ── TRasterImage ── TCustomBitmap
    │   └── TFPImageBitmap                                 ※LCL 固有
    │       └── TBitmap / TPortableNetworkGraphic / TJPEGImage
    └── TPicture
```

注意点:

- TImage・TPaintBox は **TGraphicControl** の派生(ウィンドウハンドルを持たない)。親(Parent)にはなれない。
- TMainMenu・TPopupMenu は **TControl ではない**(非ビジュアルコンポーネント)。
- TTimer は TLCLComponent を経由せず **TComponent 直下**。
- TMemo は **TCustomEdit の派生**で、Text・ReadOnly・MaxLength・OnChange を TEdit と共有する。
- TPen・TBrush・TFont・TCanvas は TComponent ではなく **TPersistent** の派生。
- TGraphic の派生(TBitmap 等)と TPicture も **TPersistent** の派生(2026-09-27 に `graphics.pp` で確認。[ADR 0029](adr/0029-graphics-picture-image-glyph.md))。

## 2. no_vcl の階層(実装済みのクラス)

LCL の継承関係の**部分列**にする(途中の階層を省くことはあっても、LCL に無い継承関係は作らない)。
省くのは「LCL/FPC 固有で、C++Builder に存在せず、公開するメンバも持たないクラス」
(TLCLComponent・TCustomDesignControl・TFPCanvasHelper・TFPCustomPen/Brush/Font・TFPCustomCanvas)。
TCustomApplication も FCL 固有で C++Builder に無いため省き、Run・Terminate・Title 等は TApplication に置く。

```
TObject
└── TPersistent
    ├── TComponent
    │   ├── TApplication
    │   ├── TCustomTimer ── TTimer
    │   └── TControl
    │       ├── TGraphicControl
    │       │   ├── TCustomLabel ── TLabel
    │       │   ├── TCustomImage ── TImage
    │       │   └── TPaintBox
    │       └── TWinControl
    │           ├── TCustomControl
    │           │   ├── TCustomPanel ── TPanel
    │           │   └── TScrollingWinControl ── TCustomForm ── TForm
    │           ├── TCustomEdit ── TEdit
    │           │   └── TCustomMemo ── TMemo
    │           ├── TCustomComboBox ── TComboBox
    │           ├── TCustomListBox ── TListBox
    │           ├── TCustomGroupBox ── TGroupBox
    │           └── TButtonControl
    │               ├── TCustomButton ── TButton
    │               └── TCustomCheckBox ── TCheckBox / TRadioButton
    ├── TPen / TBrush / TFont
    ├── TCanvas
    ├── TGraphic ── TRasterImage ── TCustomBitmap ── TBitmap / TPortableNetworkGraphic / TJPEGImage
    └── TPicture
```

TFPImageBitmap(LCL 固有で、公開するメンバを持たない)は省いた。

文字列を返す C API(`no_vcl_TControl_GetCaption` 等)の戻り値は、DLL 内のスレッドごとのバッファを指し、
同じスレッドで次に文字列を返す関数を呼ぶまで有効([ADR 0013](adr/0013-string-return-bridge-reuse-ctor-exception.md))。
保持する場合や 2 つの戻り値を同時に使う場合は呼び出し側でコピーすること。

具象クラス(TForm・TButton 等)だけが public なコンストラクタ `(TComponent* AOwner)` を持つ。
TCustomXxx 等の中間クラスのコンストラクタは protected で、直接は生成できない。
TApplication はグローバル変数 `Application` の 1 つだけで、利用者は生成できない([ADR 0010](adr/0010-application-object.md))。

## 3. メンバの配置

LCL でそのメンバが **公開(public/published)される階層** に置く。
LCL で protected のメンバを派生クラスが公開している場合は、C++ でも基底では protected にし、
公開する派生クラスで `using` する(例: `TCheckBox` の `using TButtonControl::Checked;`)。

C API(`no_vcl_c.h`)の関数名も同じ規則で、公開される階層のクラス名を使う。
ただし兄弟クラスがそれぞれ公開していて関数が重複する場合は、宣言元の共通祖先の名前で 1 本にし、
Pascal 側は protected hack(`TControlAccess = class(TControl)` のような同一ユニット内の派生クラス経由)でアクセスする。

| メンバ | LCL の宣言元(公開範囲) | LCL で公開しているクラス | no_vcl C++ | C API |
|---|---|---|---|---|
| Parent / Left / Top / Width / Height / Visible / Enabled / Caption | TControl(public/published) | TControl | TControl(public) | `TControl_*` |
| Align | TControl(public。既定値は TStatusBar が alBottom、TCustomSplitter が alLeft に上書き) | TControl | TControl(public) | `TControl_GetAlign` / `SetAlign`([ADR 0016](adr/0016-control-align-and-splitter.md)) |
| AutoSize | TControl(public) | TControl | TControl(public) | `TControl_GetAutoSize` / `SetAutoSize`([ADR 0029](adr/0029-graphics-picture-image-glyph.md)) |
| Show / Hide | TControl(public) | TControl | TControl(public) | `TControl_Show` / `Hide` |
| OnClick | TControl(public) | TControl | `TControl::OnClick` | `TControl_SetOnClick` |
| Text | TControl(protected) | TCustomEdit / TCustomComboBox | TControl(protected)、TCustomEdit / TCustomComboBox で `using` | `TControl_GetText` / `SetText`(protected hack) |
| OnResize | TControl(public) | TControl | TControl(public) | `TControl_SetOnResize` |
| OnDblClick / OnMouseEnter / OnMouseLeave | TControl(protected) | TControl | TControl(public。protected hack で登録) | `TControl_SetOn*`(protected hack) |
| OnMouseDown / OnMouseUp / OnMouseMove / OnMouseWheel | TControl(protected) | TControl | TControl(public。protected hack で登録) | `TControl_SetOn*`(protected hack、[ADR 0014](adr/0014-control-key-mouse-events.md)) |
| OnKeyDown / OnKeyUp / OnKeyPress | TWinControl(public) | TWinControl | TWinControl(public) | `TWinControl_SetOn*`([ADR 0014](adr/0014-control-key-mouse-events.md)) |
| Show / Hide / ShowModal / Close | TCustomForm(public) | TCustomForm | TCustomForm(public。Show/Hide は TControl のものを隠す) | `TCustomForm_*` |
| Release / OnClose / OnShow | TCustomForm(public) | TCustomForm | TCustomForm(public) | `TCustomForm_Release` / `SetOnClose` / `SetOnShow` |
| OnCloseQuery / OnHide / OnActivate / OnDeactivate / OnDestroy | TCustomForm(public) | TCustomForm | TCustomForm(public) | `TCustomForm_SetOn*`([ADR 0012](adr/0012-remaining-form-events.md)) |
| OnCreate | TCustomForm(public) | TCustomForm | TCustomForm(public。発火は C++ 側、[ADR 0011](adr/0011-form-release-onclose-oncreate.md)) | (なし) |
| Checked | TButtonControl(protected) | TCheckBox / TRadioButton | TButtonControl(protected)、TCheckBox / TRadioButton で `using` | `TButtonControl_GetChecked` / `SetChecked`(protected hack) |
| MaxLength / ReadOnly / OnChange | TCustomEdit(public) | TCustomEdit | TCustomEdit(public) | `TCustomEdit_*` |
| EditLabel / LabelPosition / LabelSpacing | TCustomLabeledEdit(public) | TCustomLabeledEdit | TCustomLabeledEdit(public。EditLabel は `WrapExisting<TBoundLabel>`) | `TCustomLabeledEdit_*`([ADR 0028](adr/0028-labelededit-and-stringlist.md)) |
| Lines / ScrollBars | TCustomMemo(public) | TCustomMemo | TCustomMemo(public。Lines は `ReadOnlyProperty<TStrings*>`。ADR 0027) | `TCustomMemo_*` |
| Items / ItemIndex | TCustomComboBox(public) | TCustomComboBox | TCustomComboBox(public。Items は `ReadOnlyProperty<TStrings*>`。ADR 0027) | `TCustomComboBox_*` |
| OnChange(ComboBox) | TCustomComboBox(protected) | TComboBox のみ | TComboBox(public) | `TComboBox_SetOnChange` |
| Items / ItemIndex | TCustomListBox(public) | TCustomListBox | TCustomListBox(public。Items は `ReadOnlyProperty<TStrings*>`。ADR 0027) | `TCustomListBox_*` |
| Count / Strings[i] / Objects[i] / Text / CommaText / Add / AddObject / Insert / Delete / Clear / IndexOf / Exchange / Move / BeginUpdate / EndUpdate / Assign / AddStrings | TStrings(public。TPersistent) | TStrings | TStrings(public。所有者から中身を都度取得するビュー) | `TStrings_*`([ADR 0027](adr/0027-tstrings.md)) |
| Names[i] / Values[name] / ValueFromIndex[i] / IndexOfName / Delimiter / StrictDelimiter / DelimitedText / LoadFromFile / SaveToFile | TStrings(public) | TStrings | TStrings(public。Values は `IndexedProperty<std::string, std::string>`) | `TStrings_*`(ADR 0028) |
| Sorted / Duplicates / CaseSensitive / Sort / Find | TStringList(public。TStrings の派生) | TStringList | TStringList(public。new / delete で生成・破棄) | `TStringList_*`(ADR 0028) |
| Canvas / OnPaint | TPaintBox(public/published) | TPaintBox | TPaintBox(public) | `TPaintBox_*` |
| Pixels / FillRect / Draw / StretchDraw | TCanvas(public) | TCanvas | TCanvas(public。Pixels は `IndexedProperty2<TColor>`) | `TCanvas_*`(ADR 0029) |
| Width / Height / Empty / Transparent / LoadFromFile / SaveToFile / Assign / Clear | TGraphic(public。TPersistent) | TGraphic | TGraphic(public。利用者が new / delete するものと、所有者から中身を都度取得するビューがある) | `TGraphic_*`(ADR 0029) |
| Canvas / PixelFormat / TransparentColor / TransparentMode | TRasterImage(public) | TRasterImage | TRasterImage(public。Canvas は `ReadOnlyProperty<TCanvas*>`) | `TRasterImage_*`(ADR 0029) |
| SetSize | TCustomBitmap(public) | TCustomBitmap | TCustomBitmap(public) | `TCustomBitmap_SetSize`(ADR 0029) |
| CompressionQuality | TJPEGImage(public) | TJPEGImage | TJPEGImage(public) | `TJPEGImage_*`(ADR 0029) |
| Graphic / Bitmap / PNG / Jpeg / Width / Height / LoadFromFile / SaveToFile / Assign / Clear | TPicture(public。TPersistent) | TPicture | TPicture(public。Graphic・Bitmap 等は代入で内容を写す `Property<T*>`) | `TPicture_*`(ADR 0029) |
| Picture / Canvas / HasGraphic / Center / Stretch / StretchOutEnabled / StretchInEnabled / Proportional / Transparent / OnPictureChanged | TCustomImage(public) | TCustomImage | TCustomImage(public) | `TCustomImage_*`(ADR 0029) |
| Glyph / NumGlyphs / Layout / Margin / Spacing | TCustomBitBtn・TCustomSpeedButton(public) | TCustomBitBtn・TCustomSpeedButton | 同左(public。Glyph は代入で内容を写す `Property<TBitmap*>`) | `TCustomBitBtn_*` / `TCustomSpeedButton_*`(ADR 0029) |
| AutoSnap / Beveled / MinSize / ResizeAnchor / ResizeStyle / OnMoved / Get・SetSplitterPosition | TCustomSplitter(public) | TCustomSplitter | TCustomSplitter(public) | `TCustomSplitter_*`([ADR 0016](adr/0016-control-align-and-splitter.md)) |
| PopupMenu | TControl(public) | TControl | TControl(public) | `TControl_GetPopupMenu` / `SetPopupMenu`([ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md)) |
| Menu | TCustomForm(public。TForm が published) | TCustomForm | TCustomForm(public) | `TCustomForm_GetMenu` / `SetMenu`(ADR 0017) |
| Caption / Checked / Enabled / Visible / AutoCheck / RadioItem / GroupIndex / Default / ShortCut / Hint / OnClick | TMenuItem(published) | TMenuItem | TMenuItem(public。TControl とは別に宣言) | `TMenuItem_*`(ADR 0017) |
| Count / Items[i] / Parent / Add / Insert / Delete / Remove / Clear / IndexOf / AddSeparator / IsLine / Click | TMenuItem(public) | TMenuItem | TMenuItem(public。Items[i] は `ReadOnlyIndexedProperty`。ADR 0023) | `TMenuItem_*`(ADR 0017) |
| Items | TMenu(published。LCL が内部で生成するルート項目) | TMenu | TMenu(public。`WrapExisting` でラップ) | `TMenu_GetItems`(ADR 0017) |
| AutoPopup / PopupComponent / OnPopup / OnClose / Popup | TPopupMenu(public/published) | TPopupMenu | TPopupMenu(public) | `TPopupMenu_*`(ADR 0017) |
| PageCount / MultiLine / ShowTabs / TabPosition / OnChanging | TCustomTabControl(public) | TCustomTabControl | TCustomTabControl(public。TTabControl でも使える) | `TCustomTabControl_*`([ADR 0018](adr/0018-pagecontrol-and-tabsheet.md)) |
| TabIndex / OnChange(PageControl) | TCustomTabControl(protected) | TPageControl(TTabControl は独自のフィールドで再宣言) | TPageControl(public) | `TPageControl_GetTabIndex` / `SetTabIndex` / `SetOnChange`(ADR 0018) |
| ActivePage / ActivePageIndex / Pages[i] / AddTabSheet / Clear / SelectNextPage | TPageControl(public/published) | TPageControl | TPageControl(public。Pages[i] は `ReadOnlyIndexedProperty`。ADR 0023) | `TPageControl_*`(ADR 0018) |
| PageIndex / TabVisible / OnShow / OnHide | TCustomPage(public) | TCustomPage | TCustomPage(public) | `TCustomPage_*`(ADR 0018) |
| PageControl / TabIndex | TTabSheet(public) | TTabSheet | TTabSheet(public) | `TTabSheet_*`(ADR 0018) |
| Items / Selected / FullExpand / FullCollapse / AlphaSort / GetNodeAt | TCustomTreeView(public) | TCustomTreeView | TCustomTreeView(public) | `TCustomTreeView_*`([ADR 0019](adr/0019-treeview-and-non-component-items.md)) |
| ReadOnly / ShowLines / ShowRoot / ShowButtons / AutoExpand / HideSelection / RowSelect / OnChange / OnChanging / OnExpanding / OnExpanded / OnCollapsing / OnCollapsed / OnDeletion | TCustomTreeView(protected) | TTreeView | TTreeView(public) | `TTreeView_*`(ADR 0019) |
| Add / AddFirst / AddChild / AddChildFirst / Insert / Clear / Delete / Count / Item[i] / GetFirstNode / FindNodeWithText / BeginUpdate / EndUpdate | TTreeNodes(public。TPersistent) | TTreeNodes | TTreeNodes(public。Item[i] は `ReadOnlyIndexedProperty`。ADR 0023) | `TTreeNodes_*`(ADR 0019) |
| Text / Expanded / Selected / HasChildren / Data / Count / Index / Level / AbsoluteIndex / Parent / TreeView / Items[i] / GetFirstChild 等 / Expand / Collapse / Delete / DeleteChildren / MakeVisible / MoveTo | TTreeNode(public。TPersistent) | TTreeNode | TTreeNode(public。Items[i] は `ReadOnlyIndexedProperty`。ADR 0023) | `TTreeNode_*`(ADR 0019) |
| Items / Selected / ItemIndex / SelCount / Checkboxes / GridLines / MultiSelect / ReadOnly / RowSelect / Clear / BeginUpdate / EndUpdate / GetItemAt / ClearSelection / SelectAll | TCustomListView(public) | TCustomListView | TCustomListView(public) | `TCustomListView_*`([ADR 0020](adr/0020-listview-and-shared-item-registry.md)) |
| Columns / ViewStyle / HideSelection / SortType / SortColumn / SortDirection / OnSelectItem / OnChange / OnDeletion / OnItemChecked / OnColumnClick | TCustomListView(protected) | TListView | TListView(public) | `TListView_*`(ADR 0020) |
| Add / Insert / Delete / Clear / Count / Item[i] / IndexOf / FindCaption / Exchange / Move / BeginUpdate / EndUpdate | TListItems(public。TPersistent) | TListItems | TListItems(public。Item[i] は `ReadOnlyIndexedProperty`。ADR 0023) | `TListItems_*`(ADR 0020) |
| Caption / Checked / Selected / Focused / Data / Index / ListView / SubItems / Delete / MakeVisible | TListItem(public。TPersistent) | TListItem | TListItem(public。SubItems は `ReadOnlyProperty<TStrings*>`。ADR 0027) | `TListItem_*`(ADR 0020) |
| Add / Count / Items[i] / Delete / Clear | TListColumns(public。TCollection) | TListColumns | TListColumns(public。Items[i] は `ReadOnlyIndexedProperty`。ADR 0023) | `TListColumns_*`(ADR 0020) |
| Caption / Width / Alignment / AutoSize / Visible / Index | TListColumn(published。TCollectionItem) | TListColumn | TListColumn(public) | `TListColumn_*`(ADR 0020) |
| BeginUpdate / EndUpdate / Clear / CellRect / MouseToCell | TCustomGrid(public) | TCustomGrid | TCustomGrid(public) | `TCustomGrid_*`([ADR 0021](adr/0021-drawgrid-and-stringgrid.md)) |
| ColCount / RowCount / FixedCols / FixedRows / Col / Row / DefaultColWidth / DefaultRowHeight / ColWidths[] / RowHeights[] / Options / Selection / LeftCol / TopRow / DefaultDrawing / FixedColor / EditorMode / Canvas / OnDrawCell / OnSelection | TCustomGrid(protected) | TCustomDrawGrid(public) | TCustomDrawGrid(public。ColWidths[i]・RowHeights[i] は `IndexedProperty<int>`) | `TCustomDrawGrid_*`(ADR 0021・0022) |
| OnSelectCell / OnHeaderClick / InsertColRow / DeleteColRow / MoveColRow / SortColRow | TCustomDrawGrid(public) | TCustomDrawGrid | TCustomDrawGrid(public) | `TCustomDrawGrid_*`(ADR 0021) |
| Cells[c, r] / Clean / AutoSizeColumns / AutoSizeColumn | TCustomStringGrid(public) | TCustomStringGrid | TCustomStringGrid(public。Cells[c][r] は `IndexedProperty2<std::string>`) | `TCustomStringGrid_*`(ADR 0021・0022) |
| Sections / DragReorder / OnSectionClick / OnSectionResize / OnSectionTrack / OnSectionDrag / OnSectionEndDrag / OnSectionSeparatorDblClick | TCustomHeaderControl(published) | TCustomHeaderControl | TCustomHeaderControl(public) | `TCustomHeaderControl_*`([ADR 0024](adr/0024-headercontrol.md)) |
| GetSectionAt / SectionFromOriginalIndex[i] | TCustomHeaderControl(public) | TCustomHeaderControl | TCustomHeaderControl(public。SectionFromOriginalIndex[i] は `ReadOnlyIndexedProperty`) | `TCustomHeaderControl_*`(ADR 0024) |
| Add / Insert / Delete / Clear / Count / Items[i] / BeginUpdate / EndUpdate | THeaderSections(public。TCollection) | THeaderSections | THeaderSections(public。Items[i] は `ReadOnlyIndexedProperty`) | `THeaderSections_*`(ADR 0024) |
| Text / Width / MinWidth / MaxWidth / Alignment / Visible / Index / Left / Right / OriginalIndex | THeaderSection(public/published。TCollectionItem) | THeaderSection | THeaderSection(public) | `THeaderSection_*`(ADR 0024) |
| EdgeBorders / EdgeInner / EdgeOuter / BeginUpdate / EndUpdate | TToolWindow(public) | TToolWindow | TToolWindow(public。EdgeBorders はビット集合) | `TToolWindow_*`([ADR 0025](adr/0025-toolbar-and-toolbutton.md)) |
| Bands / FixedSize / FixedOrder / GrabStyle / GrabWidth / HorizontalSpacing / VerticalSpacing / ShowText / Themed / Vertical / OnChange / AutosizeBands / MouseToBandPos | TCustomCoolBar(public) | TCoolBar | TCustomCoolBar(public) | `TCustomCoolBar_*`([ADR 0026](adr/0026-coolbar-and-item-free-observer.md)) |
| Add / Delete / Clear / Count / Items[i] / FindBand / FindBandIndex / BeginUpdate / EndUpdate | TCoolBands(public。TCollection) | TCoolBands | TCoolBands(public。Items[i] は `ReadOnlyIndexedProperty`) | `TCoolBands_*`(ADR 0026) |
| Text / Width / MinWidth / MinHeight / Break / Visible / FixedSize / FixedBackground / HorizontalOnly / Color / ParentColor / Index / Control / Left / Top / Right / Height / AutosizeWidth | TCoolBand(public/published。TCollectionItem) | TCoolBand | TCoolBand(public) | `TCoolBand_*`(ADR 0026) |
| ButtonCount / Buttons[i] / RowCount / ButtonHeight / ButtonWidth / DropDownWidth / Indent / Flat / List / ShowCaptions / Transparent / Wrapable / SetButtonSize | TToolBar(public/published) | TToolBar | TToolBar(public。Buttons[i] は `ReadOnlyIndexedProperty`) | `TToolBar_*`(ADR 0025) |
| AllowAllUp / Down / Grouped / Indeterminate / Marked / ShowCaption / Wrap / Style / DropdownMenu / MenuItem / OnArrowClick / Index / Click / ArrowClick / PointInArrow | TToolButton(public/published) | TToolButton | TToolButton(public) | `TToolButton_*`(ADR 0025) |
| Interval / Enabled / OnTimer | TCustomTimer(public) | TCustomTimer | TCustomTimer(public) | `TCustomTimer_*` |
| Run / Terminate / Terminated / Title | TCustomApplication(public。Run・Terminate・Title は TApplication で再宣言) | TApplication | TApplication(public) | `TApplication_*` |
| CreateForm / MainForm / ProcessMessages / ShowMainForm | TApplication(public) | TApplication | TApplication(public。CreateForm は型を引数から推論するテンプレート) | `TApplication_*`(CreateForm は素の TForm を返す) |
| DestroyComponents | TComponent(public) | TComponent | (C API のみ) | `TComponent_DestroyComponents` |

破棄は種類によらず `TComponent_Destroy`(C++ では `TComponent::Free()`)で行う。

修飾キー・マウスボタンの状態(TShiftState)は、C API・C++ とも `no_vcl_ss*` / `ssShift` 等のビット定数を OR した
単純な整数のビット集合として表す(TColor と同様、Pascal の集合型を素の整数として扱う)。
マウスボタンは `TMouseButton` の序数と同じ整数(`no_vcl_mb*` / `TMouseButton`)([ADR 0014](adr/0014-control-key-mouse-events.md))。

イベント(OnClick・OnChange・OnPaint・OnTimer・OnShow 等)は C++ では `Property<TNotifyEvent>`(`TNotifyEvent = std::function<void(TObject* Sender)>`)、
C API では `no_vcl_Txxx_SetOnXxx(obj, callback, data)` で登録する([ADR 0009](adr/0009-events-as-properties-with-sender.md))。
OnClose は `Property<TCloseEvent>`(`std::function<void(TObject* Sender, TCloseAction& Action)>`)で、
C API のコールバックは Action へのポインタを受け取る([ADR 0011](adr/0011-form-release-onclose-oncreate.md))。
OnCloseQuery も同じ形で、`Property<TCloseQueryEvent>`(`std::function<void(TObject* Sender, bool& CanClose)>`)、
C API のコールバックは CanClose へのポインタを受け取る([ADR 0012](adr/0012-remaining-form-events.md))。

## 4. 生存期間([ADR 0008](adr/0008-wrapper-lifetime-follows-lcl.md))

- C++ ラッパーの寿命は LCL オブジェクトの寿命と一致する。LCL オブジェクトが破棄されると
  (`Free()` でも Owner による連鎖破棄でも)、破棄通知を受けてラッパーも delete される。
- コンポーネントは `new` で生成し、`delete` ではなく `Free()` で破棄する。Owner を持つものは Owner に任せてよい。
  フォーム自身やその子のイベントハンドラの中でフォームを破棄するときは `Release()` を使う(OnClose で `Action = caFree` としても同じ)。
- `TObject` から具象クラスまで、コンポーネント系の全クラスのデストラクタは protected
  (スタック生成・`delete`・`unique_ptr` はコンパイルエラー)。派生クラスを作る場合もデストラクタを protected で宣言する。
- 非所有の `TCanvas`/`TPen`/`TBrush`/`TFont` は値メンバとして持つため、デストラクタは public。
- グラフィック(`TBitmap` 等)と `TPicture` は TComponent ではなく、`TStringList` と同じく利用者が `new` / `delete` する
  (スタックや値メンバにも置ける)。`Image1->Picture->Bitmap`・`BitBtn1->Glyph` 等は所有者の値メンバのビューで、
  中身を操作のたびに所有者から取得する。グラフィック・TImage の `Canvas` のラッパーは、中身が作り直されたときに作り直す
  (ポインタを保存しない。[ADR 0029](adr/0029-graphics-picture-image-glyph.md))。
- LCL が内部で生成したコンポーネント(`TMenu::Items` のルート項目、`AddSeparator` の区切り線)は、
  C++ で初めて取得した時点でラッパーが作られ(`TComponent::WrapExisting`)、以降は他のコンポーネントと同じく
  破棄通知で delete される。Pascal 側はそれを返す関数の中で破棄通知の対象に登録する
  ([ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md))。
- ツリービューのノード(`TTreeNode`。TComponent ではない)は、初めて取得したときにラッパーが作られ、ノードの削除
  (ツリービューの破棄に伴う削除も含む)の通知で delete される。Pascal 側は `TCustomTreeView.Delete` を上書きした
  内部クラス(`TNoVclTreeView`)で、OnDeletion の後に通知する。`TTreeNodes` は TCanvas と同じくツリービューの値メンバ
  ([ADR 0019](adr/0019-treeview-and-non-component-items.md))。
- TComponent ではない項目(ツリービューのノード・リストビューの項目と列)のラッパーは `ItemRegistry` で共通に管理し、
  DLL の項目の破棄通知(`no_vcl_ItemFree_SetCallback`)で delete される。リストビューの項目は `TCustomListView.DoDeletion` の上書き、
  列は no_vcl の Delete・Clear とリストビューの破棄で通知される([ADR 0020](adr/0020-listview-and-shared-item-registry.md))。
- ラッパーのレジストリは、atexit で登録した終了処理(`TApplication::Shutdown`)の中でも使われるため、関数内 static の値ではなく
  破棄しないオブジェクトにする(初回の構築が atexit 登録より後だと、Shutdown より先に破棄されてしまう。ADR 0019)。
- Application が所有するフォームは、main から戻った後の C++ の終了処理でまとめて破棄される
  (デストラクタも呼ばれる)。DLL の切り離し時には破棄通知は呼ばれない([ADR 0010](adr/0010-application-object.md))。
