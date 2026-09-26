#ifndef NO_VCL_HPP
#define NO_VCL_HPP

#include <cstdint>
#include <functional>
#include <string>
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
    virtual ~TObject() = default;

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

    Property(const Property&) = delete;
    Property& operator=(const Property&) = delete;

    Property& operator=(const T& value)
    {
        setter_(owner_, value);
        return *this;
    }

    operator T() const
    {
        return getter_(owner_);
    }

private:
    TObject* owner_;
    Getter   getter_;
    Setter   setter_;
};

class TPersistent : public TObject
{
protected:
    explicit TPersistent(no_vcl_obj_t handle) : TObject(handle) {}
};

// LCL のコンポーネント(Create/Destroy を持つオブジェクト)。
// ハンドルと C++ ラッパーの対応を共通のレジストリで管理し、
// コールバックのトランポリンや Parent の Getter から C++ ラッパーを引けるようにする。
class TComponent : public TPersistent
{
public:
    ~TComponent() override;

protected:
    explicit TComponent(no_vcl_obj_t handle);

    static no_vcl_obj_t HandleOf(const TObject* obj) { return obj ? obj->Handle() : nullptr; }
    static TComponent*  FromHandle(no_vcl_obj_t handle);

private:
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

    void Show();
    void Hide();

    void SetOnClick(std::function<void()> handler);

protected:
    explicit TControl(no_vcl_obj_t handle);

    // LCL では TControl の protected。TCustomEdit / TCustomComboBox が公開する。
    Property<std::string>  Text;

private:
    std::function<void()> onClick_;
    bool                  onClickHooked_ = false;
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender);

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
};

class TGraphicControl : public TControl
{
protected:
    explicit TGraphicControl(no_vcl_obj_t handle) : TControl(handle) {}
};

class TCustomControl : public TWinControl
{
protected:
    explicit TCustomControl(no_vcl_obj_t handle) : TWinControl(handle) {}
};

/* ---------------- Form ---------------- */

class TScrollingWinControl : public TCustomControl
{
protected:
    explicit TScrollingWinControl(no_vcl_obj_t handle) : TCustomControl(handle) {}
};

class TCustomForm : public TScrollingWinControl
{
public:
    // LCL の TCustomForm は Show/Hide を独自に宣言している(TControl のものを隠す)。
    void Show();
    void Hide();
    int  ShowModal();
    void Close();

protected:
    explicit TCustomForm(no_vcl_obj_t handle) : TScrollingWinControl(handle) {}
};

class TForm : public TCustomForm
{
public:
    explicit TForm(TComponent* AOwner);
};

/* ---------------- Panel / GroupBox / Label ---------------- */

class TCustomPanel : public TCustomControl
{
protected:
    explicit TCustomPanel(no_vcl_obj_t handle) : TCustomControl(handle) {}
};

class TPanel : public TCustomPanel
{
public:
    explicit TPanel(TComponent* AOwner);
};

class TCustomGroupBox : public TWinControl
{
protected:
    explicit TCustomGroupBox(no_vcl_obj_t handle) : TWinControl(handle) {}
};

class TGroupBox : public TCustomGroupBox
{
public:
    explicit TGroupBox(TComponent* AOwner);
};

class TCustomLabel : public TGraphicControl
{
protected:
    explicit TCustomLabel(no_vcl_obj_t handle) : TGraphicControl(handle) {}
};

class TLabel : public TCustomLabel
{
public:
    explicit TLabel(TComponent* AOwner);
};

/* ---------------- Button / CheckBox / RadioButton ---------------- */

class TButtonControl : public TWinControl
{
protected:
    explicit TButtonControl(no_vcl_obj_t handle);

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
};

class TButton : public TCustomButton
{
public:
    explicit TButton(TComponent* AOwner);
};

class TCustomCheckBox : public TButtonControl
{
protected:
    explicit TCustomCheckBox(no_vcl_obj_t handle) : TButtonControl(handle) {}
};

class TCheckBox : public TCustomCheckBox
{
public:
    using TButtonControl::Checked;

    explicit TCheckBox(TComponent* AOwner);
};

class TRadioButton : public TCustomCheckBox
{
public:
    using TButtonControl::Checked;

    explicit TRadioButton(TComponent* AOwner);
};

/* ---------------- Edit / Memo ---------------- */

class TCustomEdit : public TWinControl
{
public:
    using TControl::Text;
    Property<int>  MaxLength;
    Property<bool> ReadOnly;

    void SetOnChange(std::function<void()> handler);

protected:
    explicit TCustomEdit(no_vcl_obj_t handle);

private:
    std::function<void()> onChange_;
    bool                  onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender);

    static int  GetMaxLengthImpl(TObject* owner);
    static void SetMaxLengthImpl(TObject* owner, const int& value);
    static bool GetReadOnlyImpl(TObject* owner);
    static void SetReadOnlyImpl(TObject* owner, const bool& value);
};

class TEdit : public TCustomEdit
{
public:
    explicit TEdit(TComponent* AOwner);
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

private:
    static int  GetScrollBarsImpl(TObject* owner);
    static void SetScrollBarsImpl(TObject* owner, const int& value);
};

class TMemo : public TCustomMemo
{
public:
    explicit TMemo(TComponent* AOwner);
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

private:
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);
};

class TComboBox : public TCustomComboBox
{
public:
    explicit TComboBox(TComponent* AOwner);

    // LCL では TCustomComboBox の protected で、公開しているのは TComboBox だけ。
    void SetOnChange(std::function<void()> handler);

private:
    std::function<void()> onChange_;
    bool                  onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender);
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

private:
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);
};

class TListBox : public TCustomListBox
{
public:
    explicit TListBox(TComponent* AOwner);
};

/* ---------------- Canvas ---------------- */

// TPen/TBrush/TFont/TCanvas は LCL でも TComponent ではなく TPersistent の派生であり、
// Canvas を持つコントロールが内部で保持するオブジェクトへの非所有(non-owning)ラッパー。
// 自前で Create/Destroy は行わない(取得元のコントロールが破棄されれば一緒に破棄される)。

class TPen : public TPersistent
{
public:
    Property<TColor> Color;
    Property<int>    Width;

    explicit TPen(no_vcl_obj_t handle);

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

    void MoveTo(int x, int y);
    void LineTo(int x, int y);
    void Rectangle(int x1, int y1, int x2, int y2);
    void Ellipse(int x1, int y1, int x2, int y2);
    void TextOut(int x, int y, const std::string& text);
};

class TPaintBox : public TGraphicControl
{
public:
    TCanvas Canvas;

    explicit TPaintBox(TComponent* AOwner);

    void SetOnPaint(std::function<void()> handler);

private:
    std::function<void()> onPaint_;
    bool                  onPaintHooked_ = false;
    static void NO_VCL_CALL PaintTrampoline(no_vcl_obj_t sender);
};

/* ---------------- Timer ---------------- */

class TCustomTimer : public TComponent
{
public:
    Property<int>  Interval;
    Property<bool> Enabled;

    void SetOnTimer(std::function<void()> handler);

protected:
    explicit TCustomTimer(no_vcl_obj_t handle);

private:
    std::function<void()> onTimer_;
    bool                  onTimerHooked_ = false;
    static void NO_VCL_CALL TimerTrampoline(no_vcl_obj_t sender);

    static int  GetIntervalImpl(TObject* owner);
    static void SetIntervalImpl(TObject* owner, const int& value);
    static bool GetEnabledImpl(TObject* owner);
    static void SetEnabledImpl(TObject* owner, const bool& value);
};

class TTimer : public TCustomTimer
{
public:
    explicit TTimer(TComponent* AOwner);
};

} // namespace no_vcl

#endif // NO_VCL_HPP
