#ifndef NO_VCL_HPP
#define NO_VCL_HPP

#include <functional>
#include <string>
#include <unordered_map>

#include "no_vcl_c.h"

namespace no_vcl
{

// 所有者への生ポインタ + 固定のGetter/Setter関数ポインタを持つ軽量プロキシ。
// std::functionを使わないためヒープ確保がなく、TFormやTButtonのコピー/ムーブは
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

    // Owner(メモリ管理)とParent(表示上の親)は同じFormにまとめている。
    // 別々に指定したくなったら引数を分ければよい。
    explicit TButton(TForm& parent);
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

} // namespace no_vcl

#endif // NO_VCL_HPP
