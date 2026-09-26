#ifndef NO_VCL_HPP
#define NO_VCL_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "no_vcl_c.h"

// クラス階層は LCL の継承関係の「部分列」になっている(途中の階層を省くことはあっても、
// LCL に無い継承関係は作らない)。これにより、C++ 上で基底クラスとして扱えるオブジェクトは
// Pascal 側でもその基底クラスとして扱えることが保証される。
// メンバの公開範囲も LCL に合わせ、LCL で protected のメンバを派生クラスが公開している箇所は
// using で公開している。詳細は docs/class-hierarchy.md を参照。

namespace no_vcl
{

// DelphiのTColorに合わせ $00BBGGRR 順のパック整数として表す。
using TColor = std::int32_t;

const TColor clBlack  = 0x000000;
const TColor clWhite  = 0xFFFFFF;
const TColor clRed    = 0x0000FF;
const TColor clGreen  = 0x008000;
const TColor clBlue   = 0xFF0000;
const TColor clYellow = 0x00FFFF;

class TObject
{
public:
    TObject(const TObject&) = delete;
    TObject& operator=(const TObject&) = delete;
    TObject(TObject&&) = delete;
    TObject& operator=(TObject&&) = delete;

    no_vcl_obj_t Handle() const { return handle_; }

protected:
    // 各クラスは、生成した(または取得した)ハンドルを基底クラスの初期化の時点で渡す。
    // これにより、派生クラスのメンバ(Property や TPaintBox::Canvas)を構築する時点で
    // handle_ が有効であることが保証される。
    explicit TObject(no_vcl_obj_t handle) : handle_(handle) {}

    // TObject* 経由の delete でコンポーネントの寿命管理(TComponent 参照)を迂回できないよう protected にする。
    virtual ~TObject() = default;

    no_vcl_obj_t handle_;
};

// 所有者(TObject派生インスタンス)への生ポインタ + 固定のGetter/Setter関数ポインタを持つ
// 軽量プロキシ。std::functionを使わないためヒープ確保がなく、所有者のコピー/ムーブは
// TObjectで禁止しているためownerが指す実体が入れ替わることもない。
template<typename T>
class Property
{
public:
    using Getter = T    (*)(TObject*);
    using Setter = void (*)(TObject*, const T&);

    Property(TObject* owner, Getter getter, Setter setter)
        : owner_(owner)
        , getter_(getter)
        , setter_(setter)
    {}

    // コピー構築は所有者を取り違えるので禁止する。一方、プロパティ同士の代入
    // (Label1->Caption = Edit1->Text; / RadioButton2->OnClick = RadioButton1->OnClick;)は
    // 値のコピーとして扱う(所有者は付け替えない)。
    Property(const Property&) = delete;
    Property& operator=(const Property& other)
    {
        setter_(owner_, other.getter_(other.owner_));
        return *this;
    }

    Property& operator=(const T& value)
    {
        setter_(owner_, value);
        return *this;
    }

    operator T() const
    {
        return getter_(owner_);
    }

    // ポインタ型のプロパティ(Parent 等)で、Button1->Parent->Caption のようにメンバへ直接たどれるようにする。
    // ポインタ以外の型では使うとコンパイルエラーになる。
    T operator->() const
    {
        return getter_(owner_);
    }

private:
    TObject* owner_;
    Getter   getter_;
    Setter   setter_;
};

// 読み取り専用のプロパティ(Application->MainForm / Terminated 等)。代入はコンパイルエラーになる。
template<typename T>
class ReadOnlyProperty
{
public:
    using Getter = T (*)(TObject*);

    ReadOnlyProperty(TObject* owner, Getter getter)
        : owner_(owner)
        , getter_(getter)
    {}

    ReadOnlyProperty(const ReadOnlyProperty&) = delete;
    ReadOnlyProperty& operator=(const ReadOnlyProperty&) = delete;

    operator T() const
    {
        return getter_(owner_);
    }

    T operator->() const
    {
        return getter_(owner_);
    }

private:
    TObject* owner_;
    Getter   getter_;
};

// イベントハンドラの型。C++Builder の TNotifyEvent に合わせ、イベントを発生させたオブジェクトを
// Sender として受け取る(static_cast / dynamic_cast で具体的な型に戻して使う)。
// 本家の __closure は標準 C++ に無いため、メンバ関数は [this](TObject* Sender) { Button1Click(Sender); }
// のようにラムダで割り当てる。nullptr を代入するとハンドラを解除できる。
using TNotifyEvent = std::function<void(TObject* Sender)>;

// フォームを閉じるときの動作(C++Builder / LCL の TCloseAction と同じ値)。
enum TCloseAction
{
    caNone,      // 閉じない
    caHide,      // 隠す(MainForm 以外の既定値)
    caFree,      // 破棄する(MainForm の既定値。MainForm ならアプリケーションを終了する)
    caMinimize   // 最小化する
};

// OnClose の型。Action には既定の動作が入っており、書き換えると Close の動作が変わる。
using TCloseEvent = std::function<void(TObject* Sender, TCloseAction& Action)>;

class TPersistent : public TObject
{
protected:
    explicit TPersistent(no_vcl_obj_t handle) : TObject(handle) {}
    ~TPersistent() override = default;
};

// LCL のコンポーネント(Create/Destroy を持つオブジェクト)。
// ハンドルと C++ ラッパーの対応を共通のレジストリで管理し、
// コールバックのトランポリンや Parent の Getter から C++ ラッパーを引けるようにする。
//
// C++ ラッパーの寿命は LCL オブジェクトの寿命と一致する。LCL オブジェクトが破棄されると
// (Free() でも、Owner による連鎖破棄でも)破棄通知を受けてラッパーも delete される。
// このため、コンポーネントは必ず new で生成し、delete ではなく Free() で破棄する
// (Owner を持つものは Owner に任せてよい)。スタック上に置いたり delete したりできないよう、
// コンポーネント系の全クラスでデストラクタを protected にしている
// (派生クラスを作る場合も、デストラクタを protected で宣言すること)。
class TComponent : public TPersistent
{
public:
    // LCL オブジェクトを破棄する。破棄通知によってこのラッパーも delete されるため、
    // 呼び出し後にこのオブジェクトへ触れてはならない。
    void Free();

protected:
    explicit TComponent(no_vcl_obj_t handle);
    ~TComponent() override;

    static no_vcl_obj_t HandleOf(const TObject* obj) { return obj ? obj->Handle() : nullptr; }
    static TComponent*  FromHandle(no_vcl_obj_t handle);

private:
    static void NO_VCL_CALL FreeNotifyTrampoline(no_vcl_obj_t handle, void* data);
    static std::unordered_map<no_vcl_obj_t, TComponent*>& Registry();
};

class TWinControl;

class TControl : public TComponent
{
public:
    Property<TWinControl*> Parent;
    Property<int>          Left;
    Property<int>          Top;
    Property<int>          Width;
    Property<int>          Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string>  Caption;
    Property<TNotifyEvent> OnClick;

    void Show();
    void Hide();

protected:
    explicit TControl(no_vcl_obj_t handle);
    ~TControl() override = default;

    // LCL では TControl の protected。TCustomEdit / TCustomComboBox が公開する。
    Property<std::string>  Text;

private:
    TNotifyEvent onClick_;
    bool         onClickHooked_ = false;
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnClickImpl(TObject* owner);
    static void         SetOnClickImpl(TObject* owner, const TNotifyEvent& value);

    static TWinControl* GetParentImpl(TObject* owner);
    static void         SetParentImpl(TObject* owner, TWinControl* const& value);
    static int          GetLeftImpl(TObject* owner);
    static void         SetLeftImpl(TObject* owner, const int& value);
    static int          GetTopImpl(TObject* owner);
    static void         SetTopImpl(TObject* owner, const int& value);
    static int          GetWidthImpl(TObject* owner);
    static void         SetWidthImpl(TObject* owner, const int& value);
    static int          GetHeightImpl(TObject* owner);
    static void         SetHeightImpl(TObject* owner, const int& value);
    static bool         GetVisibleImpl(TObject* owner);
    static void         SetVisibleImpl(TObject* owner, const bool& value);
    static bool         GetEnabledImpl(TObject* owner);
    static void         SetEnabledImpl(TObject* owner, const bool& value);
    static std::string  GetCaptionImpl(TObject* owner);
    static void         SetCaptionImpl(TObject* owner, const std::string& value);
    static std::string  GetTextImpl(TObject* owner);
    static void         SetTextImpl(TObject* owner, const std::string& value);
};

class TWinControl : public TControl
{
protected:
    explicit TWinControl(no_vcl_obj_t handle) : TControl(handle) {}
    ~TWinControl() override = default;
};

class TGraphicControl : public TControl
{
protected:
    explicit TGraphicControl(no_vcl_obj_t handle) : TControl(handle) {}
    ~TGraphicControl() override = default;
};

class TCustomControl : public TWinControl
{
protected:
    explicit TCustomControl(no_vcl_obj_t handle) : TWinControl(handle) {}
    ~TCustomControl() override = default;
};

/* ---------------- Form ---------------- */

class TScrollingWinControl : public TCustomControl
{
protected:
    explicit TScrollingWinControl(no_vcl_obj_t handle) : TCustomControl(handle) {}
    ~TScrollingWinControl() override = default;
};

class TCustomForm : public TScrollingWinControl
{
public:
    // LCL の TCustomForm は Show/Hide を独自に宣言している(TControl のものを隠す)。
    void Show();
    void Hide();
    int  ShowModal();
    void Close();

    // 保留中のメッセージを処理し終えてから破棄する(破棄後はラッパーも delete される)。
    // Free() と違い、フォーム自身やその子のイベントハンドラの中からでも安全に呼べる。
    void Release();

    // フォームの生成が完了したときに 1 度だけ呼ばれる。
    // Application->CreateForm で生成した場合は、派生クラスのコンストラクタの完了直後(C++Builder と同じ時点)。
    // new で直接生成した場合は、C++ ではコンストラクタの完了を検知できないため、最初に表示される直前になる。
    // どちらの場合もコンストラクタの中で設定すればよい。
    Property<TNotifyEvent> OnCreate;
    Property<TNotifyEvent> OnShow;
    Property<TCloseEvent>  OnClose;

protected:
    explicit TCustomForm(no_vcl_obj_t handle);
    ~TCustomForm() override = default;

private:
    friend class TApplication;

    TNotifyEvent onCreate_;
    TNotifyEvent onShow_;
    TCloseEvent  onClose_;
    bool         created_ = false;
    bool         onCloseHooked_ = false;

    // OnCreate がまだ呼ばれていなければ呼ぶ。
    void DoCreate();

    static void NO_VCL_CALL ShowTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL CloseTrampoline(no_vcl_obj_t sender, no_vcl_int_t* action, void* data);

    static TNotifyEvent GetOnCreateImpl(TObject* owner);
    static void         SetOnCreateImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnShowImpl(TObject* owner);
    static void         SetOnShowImpl(TObject* owner, const TNotifyEvent& value);
    static TCloseEvent  GetOnCloseImpl(TObject* owner);
    static void         SetOnCloseImpl(TObject* owner, const TCloseEvent& value);
};

class TForm : public TCustomForm
{
public:
    explicit TForm(TComponent* AOwner);

protected:
    ~TForm() override = default;

private:
    friend class TApplication;

    // TApplication::CreateForm が LCL の CreateForm で生成済みの、コンストラクタに引き取られるのを待つハンドル。
    // Owner が Application のときだけ、新たに生成せずこれを使う。
    static no_vcl_obj_t pendingHandle_;
    static no_vcl_obj_t CreateHandle(TComponent* AOwner);
};

// C++Builder の TApplication。LCL の TApplication は FCL の TCustomApplication の派生だが、
// C++Builder に合わせて TComponent 直下に置く。インスタンスはグローバル変数 Application の 1 つだけ。
//
// Application が所有するフォーム(CreateForm や new TForm(Application) で生成したもの)は、
// プログラムの終了時(main から戻った後)にまとめて破棄され、ラッパーのデストラクタも呼ばれる。
class TApplication : public TComponent
{
public:
    // CreateForm で最初に生成したフォーム。Run はこれを表示し、これが閉じられると戻る。
    ReadOnlyProperty<TForm*> MainForm;
    ReadOnlyProperty<bool>   Terminated;
    Property<std::string>    Title;
    Property<bool>           ShowMainForm;

    // LCL 側は DLL の読み込み時に初期化済みのため何もしない(C++Builder のコードとの互換のために置く)。
    void Initialize() {}

    // C++Builder の Application->CreateForm(__classid(TForm1), &Form1) に相当する。
    // __classid は標準 C++ に無いため、型は引数から推論する: Application->CreateForm(&Form1);
    // T は TForm の派生で、(TComponent* AOwner) を受け取って TForm(AOwner) に渡すコンストラクタを持つこと。
    // 本家と違い、Reference への代入はコンストラクタの完了後になる。
    template<typename T>
    void CreateForm(T** Reference);

    void Run();
    void ProcessMessages();
    void Terminate();

protected:
    ~TApplication() override = default;

private:
    friend TApplication* NewApplication();
    explicit TApplication(no_vcl_obj_t handle);

    void BeginCreateForm();
    void EndCreateForm();
    static void Shutdown();

    static TForm*      GetMainFormImpl(TObject* owner);
    static bool        GetTerminatedImpl(TObject* owner);
    static std::string GetTitleImpl(TObject* owner);
    static void        SetTitleImpl(TObject* owner, const std::string& value);
    static bool        GetShowMainFormImpl(TObject* owner);
    static void        SetShowMainFormImpl(TObject* owner, const bool& value);
};

// C++Builder と同じく、アプリケーションに 1 つのグローバル変数として公開する。
// 静的初期化の順序は規定されないため、他の翻訳単位のグローバル変数の初期化子からは使わないこと。
extern TApplication* Application;

template<typename T>
void TApplication::CreateForm(T** Reference)
{
    static_assert(std::is_base_of<TForm, T>::value, "CreateForm can only create classes derived from TForm");

    BeginCreateForm();
    try
    {
        *Reference = new T(this);
    }
    catch (...)
    {
        EndCreateForm();
        throw;
    }
    EndCreateForm();

    // C++Builder では OnCreate は最派生クラスのコンストラクタの完了後(AfterConstruction)に呼ばれる。
    static_cast<TCustomForm*>(*Reference)->DoCreate();
}

/* ---------------- Panel / GroupBox / Label ---------------- */

class TCustomPanel : public TCustomControl
{
protected:
    explicit TCustomPanel(no_vcl_obj_t handle) : TCustomControl(handle) {}
    ~TCustomPanel() override = default;
};

class TPanel : public TCustomPanel
{
public:
    explicit TPanel(TComponent* AOwner);

protected:
    ~TPanel() override = default;
};

class TCustomGroupBox : public TWinControl
{
protected:
    explicit TCustomGroupBox(no_vcl_obj_t handle) : TWinControl(handle) {}
    ~TCustomGroupBox() override = default;
};

class TGroupBox : public TCustomGroupBox
{
public:
    explicit TGroupBox(TComponent* AOwner);

protected:
    ~TGroupBox() override = default;
};

class TCustomLabel : public TGraphicControl
{
protected:
    explicit TCustomLabel(no_vcl_obj_t handle) : TGraphicControl(handle) {}
    ~TCustomLabel() override = default;
};

class TLabel : public TCustomLabel
{
public:
    explicit TLabel(TComponent* AOwner);

protected:
    ~TLabel() override = default;
};

/* ---------------- Button / CheckBox / RadioButton ---------------- */

class TButtonControl : public TWinControl
{
protected:
    explicit TButtonControl(no_vcl_obj_t handle);
    ~TButtonControl() override = default;

    // LCL では TButtonControl の protected。TCheckBox / TRadioButton が公開する。
    Property<bool> Checked;

private:
    static bool GetCheckedImpl(TObject* owner);
    static void SetCheckedImpl(TObject* owner, const bool& value);
};

class TCustomButton : public TButtonControl
{
protected:
    explicit TCustomButton(no_vcl_obj_t handle) : TButtonControl(handle) {}
    ~TCustomButton() override = default;
};

class TButton : public TCustomButton
{
public:
    explicit TButton(TComponent* AOwner);

protected:
    ~TButton() override = default;
};

class TCustomCheckBox : public TButtonControl
{
protected:
    explicit TCustomCheckBox(no_vcl_obj_t handle) : TButtonControl(handle) {}
    ~TCustomCheckBox() override = default;
};

class TCheckBox : public TCustomCheckBox
{
public:
    using TButtonControl::Checked;

    explicit TCheckBox(TComponent* AOwner);

protected:
    ~TCheckBox() override = default;
};

class TRadioButton : public TCustomCheckBox
{
public:
    using TButtonControl::Checked;

    explicit TRadioButton(TComponent* AOwner);

protected:
    ~TRadioButton() override = default;
};

/* ---------------- Edit / Memo ---------------- */

class TCustomEdit : public TWinControl
{
public:
    using TControl::Text;
    Property<int>          MaxLength;
    Property<bool>         ReadOnly;
    Property<TNotifyEvent> OnChange;

protected:
    explicit TCustomEdit(no_vcl_obj_t handle);
    ~TCustomEdit() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);

    static int  GetMaxLengthImpl(TObject* owner);
    static void SetMaxLengthImpl(TObject* owner, const int& value);
    static bool GetReadOnlyImpl(TObject* owner);
    static void SetReadOnlyImpl(TObject* owner, const bool& value);
};

class TEdit : public TCustomEdit
{
public:
    explicit TEdit(TComponent* AOwner);

protected:
    ~TEdit() override = default;
};

class TCustomMemo : public TCustomEdit
{
public:
    Property<int> ScrollBars;

    void        LinesAdd(const std::string& text);
    void        LinesClear();
    int         LinesCount() const;
    std::string LinesGetText(int index) const;

protected:
    explicit TCustomMemo(no_vcl_obj_t handle);
    ~TCustomMemo() override = default;

private:
    static int  GetScrollBarsImpl(TObject* owner);
    static void SetScrollBarsImpl(TObject* owner, const int& value);
};

class TMemo : public TCustomMemo
{
public:
    explicit TMemo(TComponent* AOwner);

protected:
    ~TMemo() override = default;
};

/* ---------------- ComboBox / ListBox ---------------- */

class TCustomComboBox : public TWinControl
{
public:
    using TControl::Text;
    Property<int> ItemIndex;

    void        ItemsAdd(const std::string& text);
    void        ItemsClear();
    int         ItemsCount() const;
    std::string ItemsGetText(int index) const;

protected:
    explicit TCustomComboBox(no_vcl_obj_t handle);
    ~TCustomComboBox() override = default;

private:
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);
};

class TComboBox : public TCustomComboBox
{
public:
    // LCL では TCustomComboBox の protected で、公開しているのは TComboBox だけ。
    Property<TNotifyEvent> OnChange;

    explicit TComboBox(TComponent* AOwner);

protected:
    ~TComboBox() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

class TCustomListBox : public TWinControl
{
public:
    Property<int> ItemIndex;

    void        ItemsAdd(const std::string& text);
    void        ItemsClear();
    int         ItemsCount() const;
    std::string ItemsGetText(int index) const;

protected:
    explicit TCustomListBox(no_vcl_obj_t handle);
    ~TCustomListBox() override = default;

private:
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);
};

class TListBox : public TCustomListBox
{
public:
    explicit TListBox(TComponent* AOwner);

protected:
    ~TListBox() override = default;
};

/* ---------------- Canvas ---------------- */

// TPen/TBrush/TFont/TCanvas は LCL でも TComponent ではなく TPersistent の派生であり、
// Canvas を持つコントロールが内部で保持するオブジェクトへの非所有(non-owning)ラッパー。
// 自前で Create/Destroy は行わない(取得元のコントロールが破棄されれば一緒に破棄される)。
// TPaintBox::Canvas のように値メンバとして持つため、これらのデストラクタは public にしている。

class TPen : public TPersistent
{
public:
    Property<TColor> Color;
    Property<int>    Width;

    explicit TPen(no_vcl_obj_t handle);
    ~TPen() override = default;

private:
    static TColor GetColorImpl(TObject* owner);
    static void   SetColorImpl(TObject* owner, const TColor& value);
    static int    GetWidthImpl(TObject* owner);
    static void   SetWidthImpl(TObject* owner, const int& value);
};

class TBrush : public TPersistent
{
public:
    Property<TColor> Color;

    explicit TBrush(no_vcl_obj_t handle);
    ~TBrush() override = default;

private:
    static TColor GetColorImpl(TObject* owner);
    static void   SetColorImpl(TObject* owner, const TColor& value);
};

class TFont : public TPersistent
{
public:
    Property<std::string> Name;
    Property<int>         Size;
    Property<TColor>      Color;

    explicit TFont(no_vcl_obj_t handle);
    ~TFont() override = default;

private:
    static std::string GetNameImpl(TObject* owner);
    static void        SetNameImpl(TObject* owner, const std::string& value);
    static int         GetSizeImpl(TObject* owner);
    static void        SetSizeImpl(TObject* owner, const int& value);
    static TColor      GetColorImpl(TObject* owner);
    static void        SetColorImpl(TObject* owner, const TColor& value);
};

class TCanvas : public TPersistent
{
public:
    TPen   Pen;
    TBrush Brush;
    TFont  Font;

    explicit TCanvas(no_vcl_obj_t handle);
    ~TCanvas() override = default;

    void MoveTo(int x, int y);
    void LineTo(int x, int y);
    void Rectangle(int x1, int y1, int x2, int y2);
    void Ellipse(int x1, int y1, int x2, int y2);
    void TextOut(int x, int y, const std::string& text);
};

class TPaintBox : public TGraphicControl
{
public:
    TCanvas                Canvas;
    Property<TNotifyEvent> OnPaint;

    explicit TPaintBox(TComponent* AOwner);

protected:
    ~TPaintBox() override = default;

private:
    TNotifyEvent onPaint_;
    bool         onPaintHooked_ = false;
    static void NO_VCL_CALL PaintTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnPaintImpl(TObject* owner);
    static void         SetOnPaintImpl(TObject* owner, const TNotifyEvent& value);
};

/* ---------------- Timer ---------------- */

class TCustomTimer : public TComponent
{
public:
    Property<int>          Interval;
    Property<bool>         Enabled;
    Property<TNotifyEvent> OnTimer;

protected:
    explicit TCustomTimer(no_vcl_obj_t handle);
    ~TCustomTimer() override = default;

private:
    TNotifyEvent onTimer_;
    bool         onTimerHooked_ = false;
    static void NO_VCL_CALL TimerTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnTimerImpl(TObject* owner);
    static void         SetOnTimerImpl(TObject* owner, const TNotifyEvent& value);

    static int  GetIntervalImpl(TObject* owner);
    static void SetIntervalImpl(TObject* owner, const int& value);
    static bool GetEnabledImpl(TObject* owner);
    static void SetEnabledImpl(TObject* owner, const bool& value);
};

class TTimer : public TCustomTimer
{
public:
    explicit TTimer(TComponent* AOwner);

protected:
    ~TTimer() override = default;
};

} // namespace no_vcl

#endif // NO_VCL_HPP
