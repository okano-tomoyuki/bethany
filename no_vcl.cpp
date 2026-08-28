#include "no_vcl.hpp"

namespace no_vcl
{

/* ---------------- TForm ---------------- */

TForm::TForm()
    : Width(this, &TForm::GetWidthImpl, &TForm::SetWidthImpl)
    , Height(this, &TForm::GetHeightImpl, &TForm::SetHeightImpl)
    , Caption(this, &TForm::GetCaptionImpl, &TForm::SetCaptionImpl)
{
    handle_ = no_vcl_TForm_Create(nullptr);
}

TForm::~TForm()
{
    no_vcl_TForm_Destroy(handle_);
}

void TForm::Show()      { no_vcl_TForm_Show(handle_); }
int  TForm::ShowModal() { return no_vcl_TForm_ShowModal(handle_); }
void TForm::Hide()      { no_vcl_TForm_Hide(handle_); }
void TForm::Close()     { no_vcl_TForm_Close(handle_); }

int TForm::GetWidthImpl(void* owner)
{
    return no_vcl_TForm_GetWidth(static_cast<TForm*>(owner)->handle_);
}

void TForm::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TForm_SetWidth(static_cast<TForm*>(owner)->handle_, value);
}

int TForm::GetHeightImpl(void* owner)
{
    return no_vcl_TForm_GetHeight(static_cast<TForm*>(owner)->handle_);
}

void TForm::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TForm_SetHeight(static_cast<TForm*>(owner)->handle_, value);
}

std::string TForm::GetCaptionImpl(void* owner)
{
    return std::string(no_vcl_TForm_GetCaption(static_cast<TForm*>(owner)->handle_));
}

void TForm::SetCaptionImpl(void* owner, const std::string& value)
{
    no_vcl_TForm_SetCaption(static_cast<TForm*>(owner)->handle_, value.c_str());
}

/* ---------------- TButton ---------------- */

std::unordered_map<no_vcl_obj_t, TButton*> TButton::s_registry;

TButton::TButton(TForm& parent)
    : Left(this, &TButton::GetLeftImpl, &TButton::SetLeftImpl)
    , Top(this, &TButton::GetTopImpl, &TButton::SetTopImpl)
    , Width(this, &TButton::GetWidthImpl, &TButton::SetWidthImpl)
    , Height(this, &TButton::GetHeightImpl, &TButton::SetHeightImpl)
    , Caption(this, &TButton::GetCaptionImpl, &TButton::SetCaptionImpl)
{
    handle_ = no_vcl_TButton_Create(parent.Handle());
    no_vcl_TButton_SetParent(handle_, parent.Handle());

    s_registry[handle_] = this;
    no_vcl_TButton_SetOnClick(handle_, &TButton::ClickTrampoline);
}

TButton::~TButton()
{
    s_registry.erase(handle_);
    no_vcl_TButton_Destroy(handle_);
}

void TButton::SetOnClick(std::function<void()> handler)
{
    onClick_ = std::move(handler);
}

void NO_VCL_CALL TButton::ClickTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onClick_)
        it->second->onClick_();
}

int TButton::GetLeftImpl(void* owner)
{
    return no_vcl_TButton_GetLeft(static_cast<TButton*>(owner)->handle_);
}

void TButton::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TButton_SetLeft(static_cast<TButton*>(owner)->handle_, value);
}

int TButton::GetTopImpl(void* owner)
{
    return no_vcl_TButton_GetTop(static_cast<TButton*>(owner)->handle_);
}

void TButton::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TButton_SetTop(static_cast<TButton*>(owner)->handle_, value);
}

int TButton::GetWidthImpl(void* owner)
{
    return no_vcl_TButton_GetWidth(static_cast<TButton*>(owner)->handle_);
}

void TButton::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TButton_SetWidth(static_cast<TButton*>(owner)->handle_, value);
}

int TButton::GetHeightImpl(void* owner)
{
    return no_vcl_TButton_GetHeight(static_cast<TButton*>(owner)->handle_);
}

void TButton::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TButton_SetHeight(static_cast<TButton*>(owner)->handle_, value);
}

std::string TButton::GetCaptionImpl(void* owner)
{
    return std::string(no_vcl_TButton_GetCaption(static_cast<TButton*>(owner)->handle_));
}

void TButton::SetCaptionImpl(void* owner, const std::string& value)
{
    no_vcl_TButton_SetCaption(static_cast<TButton*>(owner)->handle_, value.c_str());
}

} // namespace no_vcl
