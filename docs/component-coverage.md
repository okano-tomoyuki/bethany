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
| ✅ TStatusBar | comctrls.pp | TWinControl(実装済み) | SimpleText/SimplePanel のみ(Panels は未対応)。**既知の問題:** Application->Run() 開始前に生成すると Win32 エラー 1406 で失敗する。回避策と詳細は [ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md) |
| TSplitter | extctrls.pp(TCustomSplitter) | TCustomControl(実装済み) | `TControl.Align` が no_vcl に無いと実用にならないため保留(下記「cross-cutting な既知の課題」参照) |
| ✅ TScrollBar | stdctrls.pp(TCustomScrollBar) | TWinControl(実装済み) | Kind/Min/Max/Position/PageSize/OnChange(ADR 0015 の 2 バッチ目) |
| ✅ TRadioGroup | extctrls.pp(TCustomRadioGroup) | TCustomGroupBox(実装済み) | Items/ItemIndex/OnClick(ADR 0015 の 3 バッチ目)。OnClick は TControl のものとは別の独自フィールド |
| ✅ TCheckGroup | extctrls.pp(TCustomCheckGroup) | TCustomGroupBox(実装済み) | Items + インデックス付き Checked(ADR 0015 の 3 バッチ目) |
| ✅ TCheckListBox | checklst.pas(TCustomCheckListBox) | TCustomListBox(実装済み) | 基底の Items をそのまま使い、インデックス付き Checked と OnClickCheck を追加(ADR 0015 の 3 バッチ目) |
| TLabeledEdit | extctrls.pp(TCustomLabeledEdit) | TCustomEdit(実装済み) | EditLabel は LCL が内部で生成する子コンポーネントで、no_vcl の `*_Create` を経由しないため C++ ラッパーが無い。下記「cross-cutting な既知の課題」を解決してから着手する |
| ✅ TSpeedButton | buttons.pp(TCustomSpeedButton) | TGraphicControl(実装済み) | Down/GroupIndex/Flat/AllowAllUp(ADR 0015 の 4 バッチ目)。Glyph(ビットマップ)は未対応 |
| ✅ TBitBtn | buttons.pp(TCustomBitBtn) | TCustomButton(実装済み) | Kind(bkOK 等)のみ実装(ADR 0015 の 4 バッチ目)。Kind を設定すると LCL が既定の Caption を自動設定する。Glyph は Tier 3 まで保留 |
| ✅ TSpinEdit / TFloatSpinEdit | spin.pp | TCustomEdit(実装済み) | TCustomSpinEdit が Value 等を Integer で再宣言(TCustomFloatSpinEdit の Double 版を隠す)。C++ でも `Property<int>` で `Property<double>` を隠す形で再現した(ADR 0015 の 5 バッチ目) |
| ✅ TMaskEdit | maskedit.pp(TCustomMaskEdit) | TCustomEdit(実装済み) | EditMask(ADR 0015 の 5 バッチ目)。EditText 等は未対応 |
| ✅ TTrackBar | comctrls.pp(TCustomTrackBar) | TWinControl(実装済み) | Min/Max/Position/OnChange。Application->Run() 開始前の生成でも問題が無いことを確認済み(ADR 0015 の 2 バッチ目) |
| ✅ TProgressBar | comctrls.pp(TCustomProgressBar) | TWinControl(実装済み) | Min/Max/Position のみで表示専用。問題無しを確認済み(ADR 0015 の 2 バッチ目) |
| ✅ TUpDown | comctrls.pp(TCustomUpDown) | TCustomControl(実装済み) | Min/Max/Position/Increment/Associate(対象の Edit と連動)。問題無しを確認済み(ADR 0015 の 2 バッチ目) |
| TTabControl / TPageControl + TTabSheet | comctrls.pp(TCustomTabControl) | TWinControl(実装済み) | タブ切り替え UI に必須。TTabSheet は TCustomPage(TWinControl)なので所有ページの生成・破棄の設計が要る。着手時に TStatusBar と同じ問題が無いか確認する |

### Tier 2 — 中コスト(新しいコレクション型が必要)

| クラス | LCL 宣言ユニット | LCL での基底 | 必要になる補助 |
|---|---|---|---|
| TTreeView | comctrls.pp(TCustomTreeView) | TCustomControl(実装済み) | TTreeNodes/TTreeNode 相当のノード操作 API(Add/Delete/Text/Parent/Expanded 等) |
| TListView | comctrls.pp(TCustomListView) | TWinControl(実装済み) | TListItems/TListColumns 相当の行・列操作 API |
| TStringGrid / TDrawGrid | grids.pas(TCustomGrid → TCustomDrawGrid) | TCustomControl(実装済み) | セル単位の Get/Set、OnDrawCell/OnSelectCell 等の専用イベント |
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
| TMenuItem | menus.pp | TLCLComponent(no_vcl では TComponent 直下に置く) | Caption/Checked/Enabled/ShortCut/OnClick、子 MenuItem を持つツリー |
| TMainMenu | menus.pp(TMenu) | TLCLComponent → TComponent | フォームに割り当てる(TForm.Menu) |
| TPopupMenu | menus.pp(TMenu) | TLCLComponent → TComponent | コントロールに割り当てる(TControl.PopupMenu、今回未実装のプロパティ) |

### Tier 6 — 低優先・特殊

| クラス | LCL 宣言ユニット | 備考 |
|---|---|---|
| TCalendar | calendar.pp | TWinControl 直下で単純だが、実用では日付選択に TDateTimePicker を使うことが多い |
| TDateTimePicker / 月表示カレンダー | `components/datetimectrls`(別パッケージ) | 現在の build.sh の `-Fu` に無い別パッケージ。パス追加が前提 |
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

- **`TControl.Align` が無い。** レイアウトの基本(ツールバーを alTop、メインの領域を alClient 等)に
  使う、実用上ほぼ必須のプロパティだが、現状のどのクラスにも実装していない。TSplitter は Align が無いと
  意味を成さないため、Tier 1 から見送った。Align 自体は TForm を含む全コントロール共通の機能改善であり、
  特定のコントロールを追加する作業とは切り離して着手すべきもの。
- **LCL が内部で生成する子コンポーネントをラップできない。** `TCustomLabeledEdit.EditLabel` のように、
  コンポーネントが自分の子を Pascal 側だけで生成する場合、その子は no_vcl の `*_Create` を経由しないため
  C++ 側にラッパーが登録されない(`TComponent::FromHandle` が nullptr を返す)。この種のプロパティを
  公開するには、「既存のハンドルを受け取って、初回アクセス時に遅延でラッパーを生成する」ような仕組みが要る。
- **ComCtrls のネイティブコントロールに、生成タイミングに関する既知の問題がある(TStatusBar)。**
  他のコントロールにも同様の問題が無いか、追加のたびに確認する([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md))。

## 4. 推奨する着手順序

1. **Tier 1 の 1 バッチ目**(TScrollBox・TToggleBox・TBevel・TShape・TStaticText・TStatusBar)は実装済み
   ([ADR 0015](adr/0015-tier1-batch1-and-statusbar-issue.md))。続けて残りの Tier 1 に着手する。
   TTabControl/TPageControl は複雑度がやや上がるが、実用上の価値が高いため Tier 1 に含めている。
2. **`TControl.Align` の追加**(上記 cross-cutting な課題)を、TSplitter に着手する前に済ませる。
3. **Tier 5(メニュー)** は複雑度は中程度だが、実用アプリでほぼ必須のため Tier 2 より先に着手する価値がある。
4. **Tier 2** のうち TTreeView・TListView・TStringGrid は、それぞれ専用のコレクション API 設計 ADR を
   1 つ書いてから着手する(TStrings 的な List 操作の共通パターンを固められる可能性がある)。
5. **Tier 3(TBitmap/TPicture)** は、TImage 単体のためというより、Tier 1/2 のいくつか(Glyph・ImageList)の
   完成度を上げるために必要になる。着手するタイミングで独立した ADR を書く。
6. **Tier 4(ダイアログ)** は他とほぼ独立して進められるので、隙間で着手しやすい。
7. **Tier 6** は必要になった時点で個別に対応する。
