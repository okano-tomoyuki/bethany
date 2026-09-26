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

// 所有者への生ポインタ + 固定のGetter/Setter関数ポインタを持つ軽量プロキシ。
// std::functionを使わないためヒープ確保がなく、各コントロールのコピー/ムーブは
// TObjectで禁止しているためownerが指す実体が入れ替わることもない。
template<typename T>
class Property
{
public:
    using Getter = T    (*)(void*);
    using Setter = void (*)(void*, const T&);

    Property(void* owner, Getter getter, Setter setter)
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
    void*  owner_;
    Getter getter_;
    Setter setter_;
};

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
    //  そのためにはbase classの構築が終わった時点でhandle_が有効である必要がある)。
    explicit TObject(no_vcl_obj_t handle) : handle_(handle) {}
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
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static std::string GetCaptionImpl(void* owner);
    static void        SetCaptionImpl(void* owner, const std::string& value);
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

    static int         GetLeftImpl(void* owner);
    static void        SetLeftImpl(void* owner, const int& value);
    static int         GetTopImpl(void* owner);
    static void        SetTopImpl(void* owner, const int& value);
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static std::string GetCaptionImpl(void* owner);
    static void        SetCaptionImpl(void* owner, const std::string& value);
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
    static int         GetLeftImpl(void* owner);
    static void        SetLeftImpl(void* owner, const int& value);
    static int         GetTopImpl(void* owner);
    static void        SetTopImpl(void* owner, const int& value);
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static bool        GetVisibleImpl(void* owner);
    static void        SetVisibleImpl(void* owner, const bool& value);
    static bool        GetEnabledImpl(void* owner);
    static void        SetEnabledImpl(void* owner, const bool& value);
    static std::string GetCaptionImpl(void* owner);
    static void        SetCaptionImpl(void* owner, const std::string& value);
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

    static int         GetLeftImpl(void* owner);
    static void        SetLeftImpl(void* owner, const int& value);
    static int         GetTopImpl(void* owner);
    static void        SetTopImpl(void* owner, const int& value);
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static bool        GetVisibleImpl(void* owner);
    static void        SetVisibleImpl(void* owner, const bool& value);
    static bool        GetEnabledImpl(void* owner);
    static void        SetEnabledImpl(void* owner, const bool& value);
    static std::string GetTextImpl(void* owner);
    static void        SetTextImpl(void* owner, const std::string& value);
    static int         GetMaxLengthImpl(void* owner);
    static void        SetMaxLengthImpl(void* owner, const int& value);
    static bool        GetReadOnlyImpl(void* owner);
    static void        SetReadOnlyImpl(void* owner, const bool& value);
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

    static int         GetLeftImpl(void* owner);
    static void        SetLeftImpl(void* owner, const int& value);
    static int         GetTopImpl(void* owner);
    static void        SetTopImpl(void* owner, const int& value);
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static bool        GetVisibleImpl(void* owner);
    static void        SetVisibleImpl(void* owner, const bool& value);
    static bool        GetEnabledImpl(void* owner);
    static void        SetEnabledImpl(void* owner, const bool& value);
    static std::string GetCaptionImpl(void* owner);
    static void        SetCaptionImpl(void* owner, const std::string& value);
    static bool        GetCheckedImpl(void* owner);
    static void        SetCheckedImpl(void* owner, const bool& value);
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

    static int         GetLeftImpl(void* owner);
    static void        SetLeftImpl(void* owner, const int& value);
    static int         GetTopImpl(void* owner);
    static void        SetTopImpl(void* owner, const int& value);
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static bool        GetVisibleImpl(void* owner);
    static void        SetVisibleImpl(void* owner, const bool& value);
    static bool        GetEnabledImpl(void* owner);
    static void        SetEnabledImpl(void* owner, const bool& value);
    static std::string GetCaptionImpl(void* owner);
    static void        SetCaptionImpl(void* owner, const std::string& value);
    static bool        GetCheckedImpl(void* owner);
    static void        SetCheckedImpl(void* owner, const bool& value);
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
    static int         GetLeftImpl(void* owner);
    static void        SetLeftImpl(void* owner, const int& value);
    static int         GetTopImpl(void* owner);
    static void        SetTopImpl(void* owner, const int& value);
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static bool        GetVisibleImpl(void* owner);
    static void        SetVisibleImpl(void* owner, const bool& value);
    static bool        GetEnabledImpl(void* owner);
    static void        SetEnabledImpl(void* owner, const bool& value);
    static std::string GetCaptionImpl(void* owner);
    static void        SetCaptionImpl(void* owner, const std::string& value);
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
    static int         GetLeftImpl(void* owner);
    static void        SetLeftImpl(void* owner, const int& value);
    static int         GetTopImpl(void* owner);
    static void        SetTopImpl(void* owner, const int& value);
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static bool        GetVisibleImpl(void* owner);
    static void        SetVisibleImpl(void* owner, const bool& value);
    static bool        GetEnabledImpl(void* owner);
    static void        SetEnabledImpl(void* owner, const bool& value);
    static std::string GetCaptionImpl(void* owner);
    static void        SetCaptionImpl(void* owner, const std::string& value);
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

    static int         GetLeftImpl(void* owner);
    static void        SetLeftImpl(void* owner, const int& value);
    static int         GetTopImpl(void* owner);
    static void        SetTopImpl(void* owner, const int& value);
    static int         GetWidthImpl(void* owner);
    static void        SetWidthImpl(void* owner, const int& value);
    static int         GetHeightImpl(void* owner);
    static void        SetHeightImpl(void* owner, const int& value);
    static bool        GetVisibleImpl(void* owner);
    static void        SetVisibleImpl(void* owner, const bool& value);
    static bool        GetEnabledImpl(void* owner);
    static void        SetEnabledImpl(void* owner, const bool& value);
    static std::string GetTextImpl(void* owner);
    static void        SetTextImpl(void* owner, const std::string& value);
    static int         GetItemIndexImpl(void* owner);
    static void        SetItemIndexImpl(void* owner, const int& value);
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

    static int  GetLeftImpl(void* owner);
    static void SetLeftImpl(void* owner, const int& value);
    static int  GetTopImpl(void* owner);
    static void SetTopImpl(void* owner, const int& value);
    static int  GetWidthImpl(void* owner);
    static void SetWidthImpl(void* owner, const int& value);
    static int  GetHeightImpl(void* owner);
    static void SetHeightImpl(void* owner, const int& value);
    static bool GetVisibleImpl(void* owner);
    static void SetVisibleImpl(void* owner, const bool& value);
    static bool GetEnabledImpl(void* owner);
    static void SetEnabledImpl(void* owner, const bool& value);
    static int  GetItemIndexImpl(void* owner);
    static void SetItemIndexImpl(void* owner, const int& value);
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

    static int  GetLeftImpl(void* owner);
    static void SetLeftImpl(void* owner, const int& value);
    static int  GetTopImpl(void* owner);
    static void SetTopImpl(void* owner, const int& value);
    static int  GetWidthImpl(void* owner);
    static void SetWidthImpl(void* owner, const int& value);
    static int  GetHeightImpl(void* owner);
    static void SetHeightImpl(void* owner, const int& value);
    static bool GetVisibleImpl(void* owner);
    static void SetVisibleImpl(void* owner, const bool& value);
    static bool GetEnabledImpl(void* owner);
    static void SetEnabledImpl(void* owner, const bool& value);
    static bool GetReadOnlyImpl(void* owner);
    static void SetReadOnlyImpl(void* owner, const bool& value);
    static int  GetScrollBarsImpl(void* owner);
    static void SetScrollBarsImpl(void* owner, const int& value);
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

    static int  GetIntervalImpl(void* owner);
    static void SetIntervalImpl(void* owner, const int& value);
    static bool GetEnabledImpl(void* owner);
    static void SetEnabledImpl(void* owner, const bool& value);
};

// TPen/TBrush/TFont/TCanvas は Canvas を持つコントロールが内部で保持するオブジェクトへの
// 非所有(non-owning)ラッパー。TObjectとは異なり自前でCreate/Destroyは行わない
// (取得元のコントロールが破棄されれば一緒に破棄される)。

class TPen
{
public:
    Property<TColor> Color;
    Property<int>    Width;

    explicit TPen(no_vcl_obj_t handle);

private:
    no_vcl_obj_t handle_;

    static TColor GetColorImpl(void* owner);
    static void   SetColorImpl(void* owner, const TColor& value);
    static int    GetWidthImpl(void* owner);
    static void   SetWidthImpl(void* owner, const int& value);
};

class TBrush
{
public:
    Property<TColor> Color;

    explicit TBrush(no_vcl_obj_t handle);

private:
    no_vcl_obj_t handle_;

    static TColor GetColorImpl(void* owner);
    static void   SetColorImpl(void* owner, const TColor& value);
};

class TFont
{
public:
    Property<std::string> Name;
    Property<int>         Size;
    Property<TColor>      Color;

    explicit TFont(no_vcl_obj_t handle);

private:
    no_vcl_obj_t handle_;

    static std::string GetNameImpl(void* owner);
    static void        SetNameImpl(void* owner, const std::string& value);
    static int         GetSizeImpl(void* owner);
    static void        SetSizeImpl(void* owner, const int& value);
    static TColor       GetColorImpl(void* owner);
    static void         SetColorImpl(void* owner, const TColor& value);
};

class TCanvas
{
private:
    no_vcl_obj_t handle_;

public:
    TPen   Pen;
    TBrush Brush;
    TFont  Font;

    explicit TCanvas(no_vcl_obj_t handle);
    TCanvas(const TCanvas&) = delete;
    TCanvas& operator=(const TCanvas&) = delete;

    void MoveTo(int x, int y);
    void LineTo(int x, int y);
    void Rectangle(int x1, int y1, int x2, int y2);
    void Ellipse(int x1, int y1, int x2, int y2);
    void TextOut(int x, int y, const std::string& text);

    no_vcl_obj_t Handle() const { return handle_; }
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

    static int  GetLeftImpl(void* owner);
    static void SetLeftImpl(void* owner, const int& value);
    static int  GetTopImpl(void* owner);
    static void SetTopImpl(void* owner, const int& value);
    static int  GetWidthImpl(void* owner);
    static void SetWidthImpl(void* owner, const int& value);
    static int  GetHeightImpl(void* owner);
    static void SetHeightImpl(void* owner, const int& value);
    static bool GetVisibleImpl(void* owner);
    static void SetVisibleImpl(void* owner, const bool& value);
    static bool GetEnabledImpl(void* owner);
    static void SetEnabledImpl(void* owner, const bool& value);
};

} // namespace no_vcl

#endif // NO_VCL_HPP
