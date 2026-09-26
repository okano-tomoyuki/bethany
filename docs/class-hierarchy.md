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
    └── TFPCustomCanvas ── TCanvas                         ※FPC 固有
```

注意点:

- TImage・TPaintBox は **TGraphicControl** の派生(ウィンドウハンドルを持たない)。親(Parent)にはなれない。
- TMainMenu・TPopupMenu は **TControl ではない**(非ビジュアルコンポーネント)。
- TTimer は TLCLComponent を経由せず **TComponent 直下**。
- TMemo は **TCustomEdit の派生**で、Text・ReadOnly・MaxLength・OnChange を TEdit と共有する。
- TPen・TBrush・TFont・TCanvas は TComponent ではなく **TPersistent** の派生。

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
    └── TCanvas
```

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
| Show / Hide | TControl(public) | TControl | TControl(public) | `TControl_Show` / `Hide` |
| OnClick | TControl(public) | TControl | `TControl::OnClick` | `TControl_SetOnClick` |
| Text | TControl(protected) | TCustomEdit / TCustomComboBox | TControl(protected)、TCustomEdit / TCustomComboBox で `using` | `TControl_GetText` / `SetText`(protected hack) |
| Show / Hide / ShowModal / Close | TCustomForm(public) | TCustomForm | TCustomForm(public。Show/Hide は TControl のものを隠す) | `TCustomForm_*` |
| Checked | TButtonControl(protected) | TCheckBox / TRadioButton | TButtonControl(protected)、TCheckBox / TRadioButton で `using` | `TButtonControl_GetChecked` / `SetChecked`(protected hack) |
| MaxLength / ReadOnly / OnChange | TCustomEdit(public) | TCustomEdit | TCustomEdit(public) | `TCustomEdit_*` |
| Lines / ScrollBars | TCustomMemo(public) | TCustomMemo | TCustomMemo(public) | `TCustomMemo_*` |
| Items / ItemIndex | TCustomComboBox(public) | TCustomComboBox | TCustomComboBox(public) | `TCustomComboBox_*` |
| OnChange(ComboBox) | TCustomComboBox(protected) | TComboBox のみ | TComboBox(public) | `TComboBox_SetOnChange` |
| Items / ItemIndex | TCustomListBox(public) | TCustomListBox | TCustomListBox(public) | `TCustomListBox_*` |
| Canvas / OnPaint | TPaintBox(public/published) | TPaintBox | TPaintBox(public) | `TPaintBox_*` |
| Interval / Enabled / OnTimer | TCustomTimer(public) | TCustomTimer | TCustomTimer(public) | `TCustomTimer_*` |
| Run / Terminate / Terminated / Title | TCustomApplication(public。Run・Terminate・Title は TApplication で再宣言) | TApplication | TApplication(public) | `TApplication_*` |
| CreateForm / MainForm / ProcessMessages / ShowMainForm | TApplication(public) | TApplication | TApplication(public。CreateForm は型を引数から推論するテンプレート) | `TApplication_*`(CreateForm は素の TForm を返す) |
| DestroyComponents | TComponent(public) | TComponent | (C API のみ) | `TComponent_DestroyComponents` |

破棄は種類によらず `TComponent_Destroy`(C++ では `TComponent::Free()`)で行う。

イベント(OnClick・OnChange・OnPaint・OnTimer)は C++ では `Property<TNotifyEvent>`(`TNotifyEvent = std::function<void(TObject* Sender)>`)、
C API では `no_vcl_Txxx_SetOnXxx(obj, callback, data)` で登録する([ADR 0009](adr/0009-events-as-properties-with-sender.md))。

## 4. 生存期間([ADR 0008](adr/0008-wrapper-lifetime-follows-lcl.md))

- C++ ラッパーの寿命は LCL オブジェクトの寿命と一致する。LCL オブジェクトが破棄されると
  (`Free()` でも Owner による連鎖破棄でも)、破棄通知を受けてラッパーも delete される。
- コンポーネントは `new` で生成し、`delete` ではなく `Free()` で破棄する。Owner を持つものは Owner に任せてよい。
- `TObject` から具象クラスまで、コンポーネント系の全クラスのデストラクタは protected
  (スタック生成・`delete`・`unique_ptr` はコンパイルエラー)。派生クラスを作る場合もデストラクタを protected で宣言する。
- 非所有の `TCanvas`/`TPen`/`TBrush`/`TFont` は値メンバとして持つため、デストラクタは public。
- Application が所有するフォームは、main から戻った後の C++ の終了処理でまとめて破棄される
  (デストラクタも呼ばれる)。DLL の切り離し時には破棄通知は呼ばれない([ADR 0010](adr/0010-application-object.md))。
