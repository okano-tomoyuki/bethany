#ifndef NO_VCL_HPP
#define NO_VCL_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "no_vcl_c.h"

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
    TObject() = default;
    virtual ~TObject() = default;

    TObject(const TObject&) = delete;
    TObject& operator=(const TObject&) = delete;
    TObject(TObject&&) = delete;
    TObject& operator=(TObject&&) = delete;

    no_vcl_obj_t Handle() const { return handle_; }

protected:
    no_vcl_obj_t handle_ = nullptr;

    // 派生クラスがCreate直後のハンドルを基底クラス初期化の時点で持ちたい場合に使う
    // (例: TPaintBoxはCanvasメンバをメンバ初期化子リストで組み立てる必要があり、
    //  そのためにはbase classの構築が終わった時点でhandle_が有効である必要がある。
    //  TPen/TBrush/TFont/TCanvasのような非所有ラッパーも同じ仕組みで、
    //  コンストラクタ引数で渡された既存のハンドルをそのままhandle_に格納する)。
    explicit TObject(no_vcl_obj_t handle) : handle_(handle) {}
};

// 所有者(TObject派生インスタンス)への生ポインタ + 固定のGetter/Setter関数ポインタを持つ
// 軽量プロキシ。std::functionを使わないためヒープ確保がなく、各コントロールのコピー/ムーブは
// TObjectで禁止しているためownerが指す実体が入れ替わることもない。
// Getter/SetterがTObject*を受け取るのは、Property自体は「どの具象クラスのメンバか」を
// 知らなくても(TObjectさえ継承していれば)使い回せるようにするため。実際の型へは
// 各クラスのImpl関数内でstatic_castする(例: static_cast<TButton*>(owner)->handle_)。
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

class TForm : public TObject
{
public:
    Property<int>         Width;
    Property<int>         Height;
    Property<std::string> Caption;

    TForm();
    ~TForm() override;

    void Show();
    int  ShowModal();
    void Hide();
    void Close();

private:
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
};

class TButton : public TObject
{
public:
    Property<int>         Left;
    Property<int>         Top;
    Property<int>         Width;
    Property<int>         Height;
    Property<std::string> Caption;

    // Owner(メモリ管理)とParent(表示上の親)は同じオブジェクトにまとめている。
    // TForm/TPanel/TGroupBoxいずれの上にも配置できるよう、引数はTObject*で受ける。
    // C++BuilderのOwner引数(TComponent* Owner)に合わせ、参照ではなくポインタにしている。
    explicit TButton(TObject* parent);
    ~TButton() override;

    void SetOnClick(std::function<void()> handler);

private:
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TButton*> s_registry;

    std::function<void()> onClick_;

    static int         GetLeftImpl(TObject* owner);
    static void        SetLeftImpl(TObject* owner, const int& value);
    static int         GetTopImpl(TObject* owner);
    static void        SetTopImpl(TObject* owner, const int& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
};

class TLabel : public TObject
{
public:
    Property<int>         Left;
    Property<int>         Top;
    Property<int>         Width;
    Property<int>         Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string> Caption;

    explicit TLabel(TObject* parent);
    ~TLabel() override;

private:
    static int         GetLeftImpl(TObject* owner);
    static void        SetLeftImpl(TObject* owner, const int& value);
    static int         GetTopImpl(TObject* owner);
    static void        SetTopImpl(TObject* owner, const int& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static bool        GetEnabledImpl(TObject* owner);
    static void        SetEnabledImpl(TObject* owner, const bool& value);
    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
};

class TEdit : public TObject
{
public:
    Property<int>         Left;
    Property<int>         Top;
    Property<int>         Width;
    Property<int>         Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string> Text;
    Property<int>          MaxLength;
    Property<bool>          ReadOnly;

    explicit TEdit(TObject* parent);
    ~TEdit() override;

    void SetOnChange(std::function<void()> handler);

private:
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TEdit*> s_registry;

    std::function<void()> onChange_;

    static int         GetLeftImpl(TObject* owner);
    static void        SetLeftImpl(TObject* owner, const int& value);
    static int         GetTopImpl(TObject* owner);
    static void        SetTopImpl(TObject* owner, const int& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static bool        GetEnabledImpl(TObject* owner);
    static void        SetEnabledImpl(TObject* owner, const bool& value);
    static std::string GetTextImpl(TObject* owner);
    static void        SetTextImpl(TObject* owner, const std::string& value);
    static int         GetMaxLengthImpl(TObject* owner);
    static void        SetMaxLengthImpl(TObject* owner, const int& value);
    static bool        GetReadOnlyImpl(TObject* owner);
    static void        SetReadOnlyImpl(TObject* owner, const bool& value);
};

class TCheckBox : public TObject
{
public:
    Property<int>         Left;
    Property<int>         Top;
    Property<int>         Width;
    Property<int>         Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string> Caption;
    Property<bool>          Checked;

    explicit TCheckBox(TObject* parent);
    ~TCheckBox() override;

    void SetOnClick(std::function<void()> handler);

private:
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TCheckBox*> s_registry;

    std::function<void()> onClick_;

    static int         GetLeftImpl(TObject* owner);
    static void        SetLeftImpl(TObject* owner, const int& value);
    static int         GetTopImpl(TObject* owner);
    static void        SetTopImpl(TObject* owner, const int& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static bool        GetEnabledImpl(TObject* owner);
    static void        SetEnabledImpl(TObject* owner, const bool& value);
    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
    static bool        GetCheckedImpl(TObject* owner);
    static void        SetCheckedImpl(TObject* owner, const bool& value);
};

class TRadioButton : public TObject
{
public:
    Property<int>         Left;
    Property<int>         Top;
    Property<int>         Width;
    Property<int>         Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string> Caption;
    Property<bool>          Checked;

    explicit TRadioButton(TObject* parent);
    ~TRadioButton() override;

    void SetOnClick(std::function<void()> handler);

private:
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TRadioButton*> s_registry;

    std::function<void()> onClick_;

    static int         GetLeftImpl(TObject* owner);
    static void        SetLeftImpl(TObject* owner, const int& value);
    static int         GetTopImpl(TObject* owner);
    static void        SetTopImpl(TObject* owner, const int& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static bool        GetEnabledImpl(TObject* owner);
    static void        SetEnabledImpl(TObject* owner, const bool& value);
    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
    static bool        GetCheckedImpl(TObject* owner);
    static void        SetCheckedImpl(TObject* owner, const bool& value);
};

class TPanel : public TObject
{
public:
    Property<int>         Left;
    Property<int>         Top;
    Property<int>         Width;
    Property<int>         Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string> Caption;

    explicit TPanel(TObject* parent);
    ~TPanel() override;

private:
    static int         GetLeftImpl(TObject* owner);
    static void        SetLeftImpl(TObject* owner, const int& value);
    static int         GetTopImpl(TObject* owner);
    static void        SetTopImpl(TObject* owner, const int& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static bool        GetEnabledImpl(TObject* owner);
    static void        SetEnabledImpl(TObject* owner, const bool& value);
    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
};

class TGroupBox : public TObject
{
public:
    Property<int>         Left;
    Property<int>         Top;
    Property<int>         Width;
    Property<int>         Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string> Caption;

    explicit TGroupBox(TObject* parent);
    ~TGroupBox() override;

private:
    static int         GetLeftImpl(TObject* owner);
    static void        SetLeftImpl(TObject* owner, const int& value);
    static int         GetTopImpl(TObject* owner);
    static void        SetTopImpl(TObject* owner, const int& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static bool        GetEnabledImpl(TObject* owner);
    static void        SetEnabledImpl(TObject* owner, const bool& value);
    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
};

class TComboBox : public TObject
{
public:
    Property<int>         Left;
    Property<int>         Top;
    Property<int>         Width;
    Property<int>         Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string> Text;
    Property<int>          ItemIndex;

    explicit TComboBox(TObject* parent);
    ~TComboBox() override;

    void        ItemsAdd(const std::string& text);
    void        ItemsClear();
    int         ItemsCount() const;
    std::string ItemsGetText(int index) const;

    void SetOnChange(std::function<void()> handler);

private:
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TComboBox*> s_registry;

    std::function<void()> onChange_;

    static int         GetLeftImpl(TObject* owner);
    static void        SetLeftImpl(TObject* owner, const int& value);
    static int         GetTopImpl(TObject* owner);
    static void        SetTopImpl(TObject* owner, const int& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetHeightImpl(TObject* owner);
    static void        SetHeightImpl(TObject* owner, const int& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static bool        GetEnabledImpl(TObject* owner);
    static void        SetEnabledImpl(TObject* owner, const bool& value);
    static std::string GetTextImpl(TObject* owner);
    static void        SetTextImpl(TObject* owner, const std::string& value);
    static int         GetItemIndexImpl(TObject* owner);
    static void        SetItemIndexImpl(TObject* owner, const int& value);
};

class TListBox : public TObject
{
public:
    Property<int> Left;
    Property<int> Top;
    Property<int> Width;
    Property<int> Height;
    Property<bool> Visible;
    Property<bool> Enabled;
    Property<int> ItemIndex;

    explicit TListBox(TObject* parent);
    ~TListBox() override;

    void        ItemsAdd(const std::string& text);
    void        ItemsClear();
    int         ItemsCount() const;
    std::string ItemsGetText(int index) const;

    void SetOnClick(std::function<void()> handler);

private:
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TListBox*> s_registry;

    std::function<void()> onClick_;

    static int  GetLeftImpl(TObject* owner);
    static void SetLeftImpl(TObject* owner, const int& value);
    static int  GetTopImpl(TObject* owner);
    static void SetTopImpl(TObject* owner, const int& value);
    static int  GetWidthImpl(TObject* owner);
    static void SetWidthImpl(TObject* owner, const int& value);
    static int  GetHeightImpl(TObject* owner);
    static void SetHeightImpl(TObject* owner, const int& value);
    static bool GetVisibleImpl(TObject* owner);
    static void SetVisibleImpl(TObject* owner, const bool& value);
    static bool GetEnabledImpl(TObject* owner);
    static void SetEnabledImpl(TObject* owner, const bool& value);
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);
};

class TMemo : public TObject
{
public:
    Property<int>  Left;
    Property<int>  Top;
    Property<int>  Width;
    Property<int>  Height;
    Property<bool> Visible;
    Property<bool> Enabled;
    Property<bool> ReadOnly;
    Property<int>  ScrollBars;

    explicit TMemo(TObject* parent);
    ~TMemo() override;

    void        LinesAdd(const std::string& text);
    void        LinesClear();
    int         LinesCount() const;
    std::string LinesGetText(int index) const;

    void SetOnChange(std::function<void()> handler);

private:
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TMemo*> s_registry;

    std::function<void()> onChange_;

    static int  GetLeftImpl(TObject* owner);
    static void SetLeftImpl(TObject* owner, const int& value);
    static int  GetTopImpl(TObject* owner);
    static void SetTopImpl(TObject* owner, const int& value);
    static int  GetWidthImpl(TObject* owner);
    static void SetWidthImpl(TObject* owner, const int& value);
    static int  GetHeightImpl(TObject* owner);
    static void SetHeightImpl(TObject* owner, const int& value);
    static bool GetVisibleImpl(TObject* owner);
    static void SetVisibleImpl(TObject* owner, const bool& value);
    static bool GetEnabledImpl(TObject* owner);
    static void SetEnabledImpl(TObject* owner, const bool& value);
    static bool GetReadOnlyImpl(TObject* owner);
    static void SetReadOnlyImpl(TObject* owner, const bool& value);
    static int  GetScrollBarsImpl(TObject* owner);
    static void SetScrollBarsImpl(TObject* owner, const int& value);
};

class TTimer : public TObject
{
public:
    Property<int>  Interval;
    Property<bool> Enabled;

    // TTimerは非ビジュアルコンポーネントなのでParentは無く、Ownerのみ受け取る。
    explicit TTimer(TObject* owner);
    ~TTimer() override;

    void SetOnTimer(std::function<void()> handler);

private:
    static void NO_VCL_CALL TimerTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TTimer*> s_registry;

    std::function<void()> onTimer_;

    static int  GetIntervalImpl(TObject* owner);
    static void SetIntervalImpl(TObject* owner, const int& value);
    static bool GetEnabledImpl(TObject* owner);
    static void SetEnabledImpl(TObject* owner, const bool& value);
};

// TPen/TBrush/TFont/TCanvas は Canvas を持つコントロールが内部で保持するオブジェクトへの
// 非所有(non-owning)ラッパー。TObjectは継承するが(handle_/Handle()を再利用するため)、
// 自前でCreate/Destroyは行わない(取得元のコントロールが破棄されれば一緒に破棄される)。

class TPen : public TObject
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

class TBrush : public TObject
{
public:
    Property<TColor> Color;

    explicit TBrush(no_vcl_obj_t handle);

private:
    static TColor GetColorImpl(TObject* owner);
    static void   SetColorImpl(TObject* owner, const TColor& value);
};

class TFont : public TObject
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
    static TColor       GetColorImpl(TObject* owner);
    static void         SetColorImpl(TObject* owner, const TColor& value);
};

class TCanvas : public TObject
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

class TPaintBox : public TObject
{
public:
    Property<int>  Left;
    Property<int>  Top;
    Property<int>  Width;
    Property<int>  Height;
    Property<bool> Visible;
    Property<bool> Enabled;
    TCanvas        Canvas;

    explicit TPaintBox(TObject* parent);
    ~TPaintBox() override;

    void SetOnPaint(std::function<void()> handler);

private:
    static no_vcl_obj_t MakeHandle(TObject* parent);

    static void NO_VCL_CALL PaintTrampoline(no_vcl_obj_t sender);
    static std::unordered_map<no_vcl_obj_t, TPaintBox*> s_registry;

    std::function<void()> onPaint_;

    static int  GetLeftImpl(TObject* owner);
    static void SetLeftImpl(TObject* owner, const int& value);
    static int  GetTopImpl(TObject* owner);
    static void SetTopImpl(TObject* owner, const int& value);
    static int  GetWidthImpl(TObject* owner);
    static void SetWidthImpl(TObject* owner, const int& value);
    static int  GetHeightImpl(TObject* owner);
    static void SetHeightImpl(TObject* owner, const int& value);
    static bool GetVisibleImpl(TObject* owner);
    static void SetVisibleImpl(TObject* owner, const bool& value);
    static bool GetEnabledImpl(TObject* owner);
    static void SetEnabledImpl(TObject* owner, const bool& value);
};

} // namespace no_vcl

#endif // NO_VCL_HPP
