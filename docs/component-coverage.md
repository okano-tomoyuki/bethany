# コントロールの網羅性(VCL 移行を見据えた棚卸し)

no_vcl を「本格的に VCL から移行できるレベル」にするための、LCL の標準コントロール・非ビジュアルコンポーネントの
実装状況の棚卸し。2026-09-26 時点で LCL(`C:\tool\lazarus\lcl`)のソースを確認して作成した。
VCL と LCL はクラス名・プロパティ名がほぼ一致するため、基本的に LCL のクラス名がそのまま VCL 移行時の対応クラス名になる
(差異がある場合は備考に記載)。

方針は [ADR 0007](adr/0007-lcl-faithful-hierarchy.md) と同じ:
LCL の継承関係の部分列にする。実装するクラスの基底が no_vcl に無ければ、その基底から実装する。

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
| ✅ TStatusBar | comctrls.pp | TWinControl(実装済み) | SimpleText/SimplePanel のみ(Panels は未対応)。Run() 開始前の生成が失敗する LCL 側の問題は DLL 側で回避済み([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md)) |
| ✅ TSplitter | extctrls.pp(TCustomSplitter) | TCustomControl(実装済み) | `TControl.Align` の追加とあわせて実装([ADR 0016](adr/0016-control-align-and-splitter.md))。AutoSnap/Beveled/MinSize/ResizeAnchor/ResizeStyle/SplitterPosition/OnMoved。OnCanResize/OnCanOffset は未対応 |
| ✅ TScrollBar | stdctrls.pp(TCustomScrollBar) | TWinControl(実装済み) | Kind/Min/Max/Position/PageSize/OnChange(ADR 0015 の 2 バッチ目) |
| ✅ TRadioGroup | extctrls.pp(TCustomRadioGroup) | TCustomGroupBox(実装済み) | Items/ItemIndex/OnClick(ADR 0015 の 3 バッチ目)。OnClick は TControl のものとは別の独自フィールド |
| ✅ TCheckGroup | extctrls.pp(TCustomCheckGroup) | TCustomGroupBox(実装済み) | Items + インデックス付き Checked(ADR 0015 の 3 バッチ目) |
| ✅ TCheckListBox | checklst.pas(TCustomCheckListBox) | TCustomListBox(実装済み) | 基底の Items をそのまま使い、インデックス付き Checked と OnClickCheck を追加(ADR 0015 の 3 バッチ目) |
| TLabeledEdit | extctrls.pp(TCustomLabeledEdit) | TCustomEdit(実装済み) | EditLabel は LCL が内部で生成する子コンポーネント。ラップの仕組み(`WrapExisting`)は [ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md) で入ったため、着手可能になった |
| ✅ TSpeedButton | buttons.pp(TCustomSpeedButton) | TGraphicControl(実装済み) | Down/GroupIndex/Flat/AllowAllUp(ADR 0015 の 4 バッチ目)。Glyph(ビットマップ)は未対応 |
| ✅ TBitBtn | buttons.pp(TCustomBitBtn) | TCustomButton(実装済み) | Kind(bkOK 等)のみ実装(ADR 0015 の 4 バッチ目)。Kind を設定すると LCL が既定の Caption を自動設定する。Glyph は Tier 3 まで保留 |
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
| ✅ TPageControl + TTabSheet | comctrls.pp(TCustomTabControl 系) | TWinControl | Tier 2 の 1 バッチ目([ADR 0018](adr/0018-pagecontrol-and-tabsheet.md))。AddTabSheet のページは `WrapExisting` でラップ。Clear は LCL の遅延破棄。Images/Style/Options は未対応 |
| ✅ TTreeView | comctrls.pp(TCustomTreeView) | TCustomControl(実装済み) | Tier 2 の 2 バッチ目([ADR 0019](adr/0019-treeview-and-non-component-items.md))。TTreeNode(TPersistent)は `TTreeNode*` で扱い、削除通知(TCustomTreeView.Delete の上書き)でラッパーの寿命を管理。画像・複数選択・ラベルの編集は未対応 |
| ✅ TListView | comctrls.pp(TCustomListView) | TWinControl(実装済み) | Tier 2 の 3 バッチ目([ADR 0020](adr/0020-listview-and-shared-item-registry.md))。TListItem・TListColumn は TTreeNode と同じく削除通知で寿命を管理(`ItemRegistry` に共通化)。表示前の Selected は DLL 側で補正。画像・OwnerData・ラベルの編集は未対応 |
| ✅ TStringGrid / TDrawGrid | grids.pas(TCustomGrid → TCustomDrawGrid) | TCustomControl(実装済み) | Tier 2 の 4 バッチ目([ADR 0021](adr/0021-drawgrid-and-stringgrid.md))。Cells[c][r]・ColWidths[i] は添字で書ける([ADR 0022](adr/0022-indexed-property-proxy.md))、Options はビット集合、OnDrawCell/OnSelectCell/OnSelection/OnHeaderClick。Objects・Cols/Rows・OnGetEditText/OnSetEditText・Columns は未対応 |
| THeaderControl | comctrls.pp(TCustomHeaderControl) | TCustomControl(実装済み) | THeaderSections 相当のセクション操作 |
| TToolBar / TToolButton | comctrls.pp | TToolWindow(TCustomControl 系。実質は TCustomControl) | ToolButton の並び・スタイル管理 |
| TCoolBar | comctrls.pp(TCustomCoolBar) | TToolWindow | 優先度低め(現代の UI ではあまり使われない) |

### Tier 3 — グラフィックス基盤が前提

TBitmap・TPicture(TGraphic の派生を包む可変長データ型)を no_vcl にまだ持っていないため、
画像を扱うコントロールはそれらの設計を先に固める必要がある。

| クラス | LCL 宣言ユニット | 必要な基盤 |
|---|---|---|
| TImage | extctrls.pp(TCustomImage) | TPicture(TGraphic 派生を保持する可変クラス)。TCanvas 同様、非所有の値メンバとして持たせる想定 |
| TBitBtn の Glyph / TSpeedButton の Glyph | buttons.pp | TBitmap |
| TImageList | comctrls.pp や ImgList ユニット | TBitmap 一覧。TreeView・ListView・ToolBar のアイコン表示に使うが、アイコン無しでも各コントロール自体は動く |

### Tier 4 — ダイアログ(非ビジュアル、Execute 呼び出しパターン)

VCL と同じく「プロパティを設定して Execute を呼び、結果を bool で受け取る」形。
Application と同様、C++ 側は値型ではなく生成・破棄が必要な TComponent 派生として扱う。

| クラス | LCL 宣言ユニット | 主なメンバ |
|---|---|---|
| TOpenDialog / TSaveDialog | dialogs.pp(TFileDialog) | FileName/Filter/InitialDir/Execute |
| TColorDialog | dialogs.pp | Color/Execute |
| TFontDialog | dialogs.pp | Font(既存の TFont を流用)/Execute |
| TSelectDirectoryDialog | dialogs.pp(TOpenDialog 派生) | FileName(選択したディレクトリ)/Execute |
| TFindDialog / TReplaceDialog | dialogs.pp | 優先度低め(検索・置換 UI 自体は自作することが多い) |
| TPrintDialog / TCustomPrinterSetupDialog | dialogs.pp | 優先度低め(印刷基盤が no_vcl に無い) |

### Tier 5 — メニュー

VCL アプリらしい UI に必須だが、TMenuItem がツリー構造の TComponent であるため、
コレクション操作(Items.Add / Items.Count / Items[i])を C API・C++ 双方で新たに設計する必要がある。

| クラス | LCL 宣言ユニット | LCL での基底 | 備考 |
|---|---|---|---|
| ✅ TMenuItem | menus.pp | TLCLComponent(no_vcl では TComponent 直下に置く) | Caption/Checked/Enabled/Visible/AutoCheck/RadioItem/GroupIndex/Default/ShortCut/Hint/OnClick と、子の項目の操作(GetItem/Count/Add/Insert/Delete/Remove/Clear/IndexOf/AddSeparator)([ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md))。Bitmap/ImageIndex は Tier 3 待ち |
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
`TClipboard`、単体の `TStringList` は「コントロール」ではないため今回の一覧から外しているが、
実際の VCL アプリの移行では必要になることが多い。着手する場合は別途 ADR で設計を切る。

## 3. cross-cutting な既知の課題(特定のクラスではなく設計全体に関わるもの)

Tier 1 の 1 バッチ目([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md))を実装する過程で見つかった、
個別のクラスの追加では解決できない課題。

- ✅ **`TControl.Align` が無い。** → 解決済み([ADR 0016](adr/0016-control-align-and-splitter.md))。
  LCL と同じく TControl の public プロパティとして追加し、あわせて TSplitter を追加した。
  LCL では Align による配置がフォームの表示まで行われない(VCL と異なる)点に注意。
  Anchors・BorderSpacing・Constraints・AutoSize は未実装。
- ✅ **LCL が内部で生成する子コンポーネントをラップできない。** → 仕組みとしては解決済み
  ([ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md))。ハンドルを返す C API の側で破棄通知に登録し、
  C++ 側は `TComponent::WrapExisting<T>` で初回アクセス時にラッパーを作る。TMenu.Items で初めて使った。
  以下は当初の記述。`TCustomLabeledEdit.EditLabel` のように、
  コンポーネントが自分の子を Pascal 側だけで生成する場合、その子は no_vcl の `*_Create` を経由しないため
  C++ 側にラッパーが登録されない(`TComponent::FromHandle` が nullptr を返す)。この種のプロパティを
  公開するには、「既存のハンドルを受け取って、初回アクセス時に遅延でラッパーを生成する」ような仕組みが要る。
- **LCL の Win32 実装には、DLL(`IsLibrary`)のとき `WidgetSet.AppHandle` が 0 であることを前提にしていない
  箇所がある。** TStatusBar ではこれが原因で Run() 開始前の生成が失敗していた(DLL 側で回避済み)。
  新しいコントロールを追加する際は、Run() 開始前にフォームを表示しても問題が無いか確認する
  ([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md))。

## 4. 推奨する着手順序

1. **Tier 1 は完了した**(19 クラス。[ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md))。
   TLabeledEdit(内部生成コンポーネントのラップ待ち。ADR 0017 で仕組みが入ったため着手可能)・
   TPageControl+TTabSheet(所有ページの設計が必要、Tier 2 へ再分類)は cross-cutting な課題または
   複雑度の都合で見送った。
2. ✅ **`TControl.Align` の追加**(上記 cross-cutting な課題)と、それを待っていた TSplitter は完了した
   ([ADR 0016](adr/0016-control-align-and-splitter.md))。
3. ✅ **Tier 5(メニュー)** は完了した([ADR 0017](adr/0017-menus-and-wrapping-lcl-created-components.md))。
   あわせて、内部生成コンポーネントのラップの仕組み(`WrapExisting`)が入った。
4. **Tier 2** はバッチに分けて進めている。1 バッチ目の TPageControl+TTabSheet([ADR 0018](adr/0018-pagecontrol-and-tabsheet.md))と、
   2 バッチ目の TTreeView([ADR 0019](adr/0019-treeview-and-non-component-items.md))、3 バッチ目の TListView
   ([ADR 0020](adr/0020-listview-and-shared-item-registry.md))、4 バッチ目の TDrawGrid/TStringGrid
   ([ADR 0021](adr/0021-drawgrid-and-stringgrid.md))は完了した。TComponent ではない項目の寿命管理は
   `ItemRegistry` と項目の破棄通知(`ItemFree_SetCallback`)に共通化した。
   残りの THeaderControl・TToolBar/TToolButton・TCoolBar は、それぞれ専用の設計を確認してから着手する。
5. **Tier 3(TBitmap/TPicture)** は、TImage 単体のためというより、Tier 1/2 のいくつか(Glyph・ImageList)の
   完成度を上げるために必要になる。着手するタイミングで独立した ADR を書く。
6. **Tier 4(ダイアログ)** は他とほぼ独立して進められるので、隙間で着手しやすい。
7. **Tier 6** は必要になった時点で個別に対応する。
