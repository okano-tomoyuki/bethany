#include "no_vcl.hpp"

#include <cassert>

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

TButton::TButton(TObject* parent)
    : Left(this, &TButton::GetLeftImpl, &TButton::SetLeftImpl)
    , Top(this, &TButton::GetTopImpl, &TButton::SetTopImpl)
    , Width(this, &TButton::GetWidthImpl, &TButton::SetWidthImpl)
    , Height(this, &TButton::GetHeightImpl, &TButton::SetHeightImpl)
    , Caption(this, &TButton::GetCaptionImpl, &TButton::SetCaptionImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TButton_Create(parent->Handle());
    no_vcl_TButton_SetParent(handle_, parent->Handle());

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

/* ---------------- TLabel ---------------- */

TLabel::TLabel(TObject* parent)
    : Left(this, &TLabel::GetLeftImpl, &TLabel::SetLeftImpl)
    , Top(this, &TLabel::GetTopImpl, &TLabel::SetTopImpl)
    , Width(this, &TLabel::GetWidthImpl, &TLabel::SetWidthImpl)
    , Height(this, &TLabel::GetHeightImpl, &TLabel::SetHeightImpl)
    , Visible(this, &TLabel::GetVisibleImpl, &TLabel::SetVisibleImpl)
    , Enabled(this, &TLabel::GetEnabledImpl, &TLabel::SetEnabledImpl)
    , Caption(this, &TLabel::GetCaptionImpl, &TLabel::SetCaptionImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TLabel_Create(parent->Handle());
    no_vcl_TLabel_SetParent(handle_, parent->Handle());
}

TLabel::~TLabel()
{
    no_vcl_TLabel_Destroy(handle_);
}

int TLabel::GetLeftImpl(void* owner)
{
    return no_vcl_TLabel_GetLeft(static_cast<TLabel*>(owner)->handle_);
}

void TLabel::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TLabel_SetLeft(static_cast<TLabel*>(owner)->handle_, value);
}

int TLabel::GetTopImpl(void* owner)
{
    return no_vcl_TLabel_GetTop(static_cast<TLabel*>(owner)->handle_);
}

void TLabel::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TLabel_SetTop(static_cast<TLabel*>(owner)->handle_, value);
}

int TLabel::GetWidthImpl(void* owner)
{
    return no_vcl_TLabel_GetWidth(static_cast<TLabel*>(owner)->handle_);
}

void TLabel::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TLabel_SetWidth(static_cast<TLabel*>(owner)->handle_, value);
}

int TLabel::GetHeightImpl(void* owner)
{
    return no_vcl_TLabel_GetHeight(static_cast<TLabel*>(owner)->handle_);
}

void TLabel::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TLabel_SetHeight(static_cast<TLabel*>(owner)->handle_, value);
}

bool TLabel::GetVisibleImpl(void* owner)
{
    return no_vcl_TLabel_GetVisible(static_cast<TLabel*>(owner)->handle_) != 0;
}

void TLabel::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TLabel_SetVisible(static_cast<TLabel*>(owner)->handle_, value ? 1 : 0);
}

bool TLabel::GetEnabledImpl(void* owner)
{
    return no_vcl_TLabel_GetEnabled(static_cast<TLabel*>(owner)->handle_) != 0;
}

void TLabel::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TLabel_SetEnabled(static_cast<TLabel*>(owner)->handle_, value ? 1 : 0);
}

std::string TLabel::GetCaptionImpl(void* owner)
{
    return std::string(no_vcl_TLabel_GetCaption(static_cast<TLabel*>(owner)->handle_));
}

void TLabel::SetCaptionImpl(void* owner, const std::string& value)
{
    no_vcl_TLabel_SetCaption(static_cast<TLabel*>(owner)->handle_, value.c_str());
}

/* ---------------- TEdit ---------------- */

std::unordered_map<no_vcl_obj_t, TEdit*> TEdit::s_registry;

TEdit::TEdit(TObject* parent)
    : Left(this, &TEdit::GetLeftImpl, &TEdit::SetLeftImpl)
    , Top(this, &TEdit::GetTopImpl, &TEdit::SetTopImpl)
    , Width(this, &TEdit::GetWidthImpl, &TEdit::SetWidthImpl)
    , Height(this, &TEdit::GetHeightImpl, &TEdit::SetHeightImpl)
    , Visible(this, &TEdit::GetVisibleImpl, &TEdit::SetVisibleImpl)
    , Enabled(this, &TEdit::GetEnabledImpl, &TEdit::SetEnabledImpl)
    , Text(this, &TEdit::GetTextImpl, &TEdit::SetTextImpl)
    , MaxLength(this, &TEdit::GetMaxLengthImpl, &TEdit::SetMaxLengthImpl)
    , ReadOnly(this, &TEdit::GetReadOnlyImpl, &TEdit::SetReadOnlyImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TEdit_Create(parent->Handle());
    no_vcl_TEdit_SetParent(handle_, parent->Handle());

    s_registry[handle_] = this;
    no_vcl_TEdit_SetOnChange(handle_, &TEdit::ChangeTrampoline);
}

TEdit::~TEdit()
{
    s_registry.erase(handle_);
    no_vcl_TEdit_Destroy(handle_);
}

void TEdit::SetOnChange(std::function<void()> handler)
{
    onChange_ = std::move(handler);
}

void NO_VCL_CALL TEdit::ChangeTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onChange_)
        it->second->onChange_();
}

int TEdit::GetLeftImpl(void* owner)
{
    return no_vcl_TEdit_GetLeft(static_cast<TEdit*>(owner)->handle_);
}

void TEdit::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TEdit_SetLeft(static_cast<TEdit*>(owner)->handle_, value);
}

int TEdit::GetTopImpl(void* owner)
{
    return no_vcl_TEdit_GetTop(static_cast<TEdit*>(owner)->handle_);
}

void TEdit::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TEdit_SetTop(static_cast<TEdit*>(owner)->handle_, value);
}

int TEdit::GetWidthImpl(void* owner)
{
    return no_vcl_TEdit_GetWidth(static_cast<TEdit*>(owner)->handle_);
}

void TEdit::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TEdit_SetWidth(static_cast<TEdit*>(owner)->handle_, value);
}

int TEdit::GetHeightImpl(void* owner)
{
    return no_vcl_TEdit_GetHeight(static_cast<TEdit*>(owner)->handle_);
}

void TEdit::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TEdit_SetHeight(static_cast<TEdit*>(owner)->handle_, value);
}

bool TEdit::GetVisibleImpl(void* owner)
{
    return no_vcl_TEdit_GetVisible(static_cast<TEdit*>(owner)->handle_) != 0;
}

void TEdit::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TEdit_SetVisible(static_cast<TEdit*>(owner)->handle_, value ? 1 : 0);
}

bool TEdit::GetEnabledImpl(void* owner)
{
    return no_vcl_TEdit_GetEnabled(static_cast<TEdit*>(owner)->handle_) != 0;
}

void TEdit::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TEdit_SetEnabled(static_cast<TEdit*>(owner)->handle_, value ? 1 : 0);
}

std::string TEdit::GetTextImpl(void* owner)
{
    return std::string(no_vcl_TEdit_GetText(static_cast<TEdit*>(owner)->handle_));
}

void TEdit::SetTextImpl(void* owner, const std::string& value)
{
    no_vcl_TEdit_SetText(static_cast<TEdit*>(owner)->handle_, value.c_str());
}

int TEdit::GetMaxLengthImpl(void* owner)
{
    return no_vcl_TEdit_GetMaxLength(static_cast<TEdit*>(owner)->handle_);
}

void TEdit::SetMaxLengthImpl(void* owner, const int& value)
{
    no_vcl_TEdit_SetMaxLength(static_cast<TEdit*>(owner)->handle_, value);
}

bool TEdit::GetReadOnlyImpl(void* owner)
{
    return no_vcl_TEdit_GetReadOnly(static_cast<TEdit*>(owner)->handle_) != 0;
}

void TEdit::SetReadOnlyImpl(void* owner, const bool& value)
{
    no_vcl_TEdit_SetReadOnly(static_cast<TEdit*>(owner)->handle_, value ? 1 : 0);
}

/* ---------------- TCheckBox ---------------- */

std::unordered_map<no_vcl_obj_t, TCheckBox*> TCheckBox::s_registry;

TCheckBox::TCheckBox(TObject* parent)
    : Left(this, &TCheckBox::GetLeftImpl, &TCheckBox::SetLeftImpl)
    , Top(this, &TCheckBox::GetTopImpl, &TCheckBox::SetTopImpl)
    , Width(this, &TCheckBox::GetWidthImpl, &TCheckBox::SetWidthImpl)
    , Height(this, &TCheckBox::GetHeightImpl, &TCheckBox::SetHeightImpl)
    , Visible(this, &TCheckBox::GetVisibleImpl, &TCheckBox::SetVisibleImpl)
    , Enabled(this, &TCheckBox::GetEnabledImpl, &TCheckBox::SetEnabledImpl)
    , Caption(this, &TCheckBox::GetCaptionImpl, &TCheckBox::SetCaptionImpl)
    , Checked(this, &TCheckBox::GetCheckedImpl, &TCheckBox::SetCheckedImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TCheckBox_Create(parent->Handle());
    no_vcl_TCheckBox_SetParent(handle_, parent->Handle());

    s_registry[handle_] = this;
    no_vcl_TCheckBox_SetOnClick(handle_, &TCheckBox::ClickTrampoline);
}

TCheckBox::~TCheckBox()
{
    s_registry.erase(handle_);
    no_vcl_TCheckBox_Destroy(handle_);
}

void TCheckBox::SetOnClick(std::function<void()> handler)
{
    onClick_ = std::move(handler);
}

void NO_VCL_CALL TCheckBox::ClickTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onClick_)
        it->second->onClick_();
}

int TCheckBox::GetLeftImpl(void* owner)
{
    return no_vcl_TCheckBox_GetLeft(static_cast<TCheckBox*>(owner)->handle_);
}

void TCheckBox::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TCheckBox_SetLeft(static_cast<TCheckBox*>(owner)->handle_, value);
}

int TCheckBox::GetTopImpl(void* owner)
{
    return no_vcl_TCheckBox_GetTop(static_cast<TCheckBox*>(owner)->handle_);
}

void TCheckBox::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TCheckBox_SetTop(static_cast<TCheckBox*>(owner)->handle_, value);
}

int TCheckBox::GetWidthImpl(void* owner)
{
    return no_vcl_TCheckBox_GetWidth(static_cast<TCheckBox*>(owner)->handle_);
}

void TCheckBox::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TCheckBox_SetWidth(static_cast<TCheckBox*>(owner)->handle_, value);
}

int TCheckBox::GetHeightImpl(void* owner)
{
    return no_vcl_TCheckBox_GetHeight(static_cast<TCheckBox*>(owner)->handle_);
}

void TCheckBox::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TCheckBox_SetHeight(static_cast<TCheckBox*>(owner)->handle_, value);
}

bool TCheckBox::GetVisibleImpl(void* owner)
{
    return no_vcl_TCheckBox_GetVisible(static_cast<TCheckBox*>(owner)->handle_) != 0;
}

void TCheckBox::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TCheckBox_SetVisible(static_cast<TCheckBox*>(owner)->handle_, value ? 1 : 0);
}

bool TCheckBox::GetEnabledImpl(void* owner)
{
    return no_vcl_TCheckBox_GetEnabled(static_cast<TCheckBox*>(owner)->handle_) != 0;
}

void TCheckBox::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TCheckBox_SetEnabled(static_cast<TCheckBox*>(owner)->handle_, value ? 1 : 0);
}

std::string TCheckBox::GetCaptionImpl(void* owner)
{
    return std::string(no_vcl_TCheckBox_GetCaption(static_cast<TCheckBox*>(owner)->handle_));
}

void TCheckBox::SetCaptionImpl(void* owner, const std::string& value)
{
    no_vcl_TCheckBox_SetCaption(static_cast<TCheckBox*>(owner)->handle_, value.c_str());
}

bool TCheckBox::GetCheckedImpl(void* owner)
{
    return no_vcl_TCheckBox_GetChecked(static_cast<TCheckBox*>(owner)->handle_) != 0;
}

void TCheckBox::SetCheckedImpl(void* owner, const bool& value)
{
    no_vcl_TCheckBox_SetChecked(static_cast<TCheckBox*>(owner)->handle_, value ? 1 : 0);
}

/* ---------------- TRadioButton ---------------- */

std::unordered_map<no_vcl_obj_t, TRadioButton*> TRadioButton::s_registry;

TRadioButton::TRadioButton(TObject* parent)
    : Left(this, &TRadioButton::GetLeftImpl, &TRadioButton::SetLeftImpl)
    , Top(this, &TRadioButton::GetTopImpl, &TRadioButton::SetTopImpl)
    , Width(this, &TRadioButton::GetWidthImpl, &TRadioButton::SetWidthImpl)
    , Height(this, &TRadioButton::GetHeightImpl, &TRadioButton::SetHeightImpl)
    , Visible(this, &TRadioButton::GetVisibleImpl, &TRadioButton::SetVisibleImpl)
    , Enabled(this, &TRadioButton::GetEnabledImpl, &TRadioButton::SetEnabledImpl)
    , Caption(this, &TRadioButton::GetCaptionImpl, &TRadioButton::SetCaptionImpl)
    , Checked(this, &TRadioButton::GetCheckedImpl, &TRadioButton::SetCheckedImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TRadioButton_Create(parent->Handle());
    no_vcl_TRadioButton_SetParent(handle_, parent->Handle());

    s_registry[handle_] = this;
    no_vcl_TRadioButton_SetOnClick(handle_, &TRadioButton::ClickTrampoline);
}

TRadioButton::~TRadioButton()
{
    s_registry.erase(handle_);
    no_vcl_TRadioButton_Destroy(handle_);
}

void TRadioButton::SetOnClick(std::function<void()> handler)
{
    onClick_ = std::move(handler);
}

void NO_VCL_CALL TRadioButton::ClickTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onClick_)
        it->second->onClick_();
}

int TRadioButton::GetLeftImpl(void* owner)
{
    return no_vcl_TRadioButton_GetLeft(static_cast<TRadioButton*>(owner)->handle_);
}

void TRadioButton::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TRadioButton_SetLeft(static_cast<TRadioButton*>(owner)->handle_, value);
}

int TRadioButton::GetTopImpl(void* owner)
{
    return no_vcl_TRadioButton_GetTop(static_cast<TRadioButton*>(owner)->handle_);
}

void TRadioButton::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TRadioButton_SetTop(static_cast<TRadioButton*>(owner)->handle_, value);
}

int TRadioButton::GetWidthImpl(void* owner)
{
    return no_vcl_TRadioButton_GetWidth(static_cast<TRadioButton*>(owner)->handle_);
}

void TRadioButton::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TRadioButton_SetWidth(static_cast<TRadioButton*>(owner)->handle_, value);
}

int TRadioButton::GetHeightImpl(void* owner)
{
    return no_vcl_TRadioButton_GetHeight(static_cast<TRadioButton*>(owner)->handle_);
}

void TRadioButton::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TRadioButton_SetHeight(static_cast<TRadioButton*>(owner)->handle_, value);
}

bool TRadioButton::GetVisibleImpl(void* owner)
{
    return no_vcl_TRadioButton_GetVisible(static_cast<TRadioButton*>(owner)->handle_) != 0;
}

void TRadioButton::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TRadioButton_SetVisible(static_cast<TRadioButton*>(owner)->handle_, value ? 1 : 0);
}

bool TRadioButton::GetEnabledImpl(void* owner)
{
    return no_vcl_TRadioButton_GetEnabled(static_cast<TRadioButton*>(owner)->handle_) != 0;
}

void TRadioButton::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TRadioButton_SetEnabled(static_cast<TRadioButton*>(owner)->handle_, value ? 1 : 0);
}

std::string TRadioButton::GetCaptionImpl(void* owner)
{
    return std::string(no_vcl_TRadioButton_GetCaption(static_cast<TRadioButton*>(owner)->handle_));
}

void TRadioButton::SetCaptionImpl(void* owner, const std::string& value)
{
    no_vcl_TRadioButton_SetCaption(static_cast<TRadioButton*>(owner)->handle_, value.c_str());
}

bool TRadioButton::GetCheckedImpl(void* owner)
{
    return no_vcl_TRadioButton_GetChecked(static_cast<TRadioButton*>(owner)->handle_) != 0;
}

void TRadioButton::SetCheckedImpl(void* owner, const bool& value)
{
    no_vcl_TRadioButton_SetChecked(static_cast<TRadioButton*>(owner)->handle_, value ? 1 : 0);
}

/* ---------------- TPanel ---------------- */

TPanel::TPanel(TObject* parent)
    : Left(this, &TPanel::GetLeftImpl, &TPanel::SetLeftImpl)
    , Top(this, &TPanel::GetTopImpl, &TPanel::SetTopImpl)
    , Width(this, &TPanel::GetWidthImpl, &TPanel::SetWidthImpl)
    , Height(this, &TPanel::GetHeightImpl, &TPanel::SetHeightImpl)
    , Visible(this, &TPanel::GetVisibleImpl, &TPanel::SetVisibleImpl)
    , Enabled(this, &TPanel::GetEnabledImpl, &TPanel::SetEnabledImpl)
    , Caption(this, &TPanel::GetCaptionImpl, &TPanel::SetCaptionImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TPanel_Create(parent->Handle());
    no_vcl_TPanel_SetParent(handle_, parent->Handle());
}

TPanel::~TPanel()
{
    no_vcl_TPanel_Destroy(handle_);
}

int TPanel::GetLeftImpl(void* owner)
{
    return no_vcl_TPanel_GetLeft(static_cast<TPanel*>(owner)->handle_);
}

void TPanel::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TPanel_SetLeft(static_cast<TPanel*>(owner)->handle_, value);
}

int TPanel::GetTopImpl(void* owner)
{
    return no_vcl_TPanel_GetTop(static_cast<TPanel*>(owner)->handle_);
}

void TPanel::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TPanel_SetTop(static_cast<TPanel*>(owner)->handle_, value);
}

int TPanel::GetWidthImpl(void* owner)
{
    return no_vcl_TPanel_GetWidth(static_cast<TPanel*>(owner)->handle_);
}

void TPanel::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TPanel_SetWidth(static_cast<TPanel*>(owner)->handle_, value);
}

int TPanel::GetHeightImpl(void* owner)
{
    return no_vcl_TPanel_GetHeight(static_cast<TPanel*>(owner)->handle_);
}

void TPanel::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TPanel_SetHeight(static_cast<TPanel*>(owner)->handle_, value);
}

bool TPanel::GetVisibleImpl(void* owner)
{
    return no_vcl_TPanel_GetVisible(static_cast<TPanel*>(owner)->handle_) != 0;
}

void TPanel::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TPanel_SetVisible(static_cast<TPanel*>(owner)->handle_, value ? 1 : 0);
}

bool TPanel::GetEnabledImpl(void* owner)
{
    return no_vcl_TPanel_GetEnabled(static_cast<TPanel*>(owner)->handle_) != 0;
}

void TPanel::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TPanel_SetEnabled(static_cast<TPanel*>(owner)->handle_, value ? 1 : 0);
}

std::string TPanel::GetCaptionImpl(void* owner)
{
    return std::string(no_vcl_TPanel_GetCaption(static_cast<TPanel*>(owner)->handle_));
}

void TPanel::SetCaptionImpl(void* owner, const std::string& value)
{
    no_vcl_TPanel_SetCaption(static_cast<TPanel*>(owner)->handle_, value.c_str());
}

/* ---------------- TGroupBox ---------------- */

TGroupBox::TGroupBox(TObject* parent)
    : Left(this, &TGroupBox::GetLeftImpl, &TGroupBox::SetLeftImpl)
    , Top(this, &TGroupBox::GetTopImpl, &TGroupBox::SetTopImpl)
    , Width(this, &TGroupBox::GetWidthImpl, &TGroupBox::SetWidthImpl)
    , Height(this, &TGroupBox::GetHeightImpl, &TGroupBox::SetHeightImpl)
    , Visible(this, &TGroupBox::GetVisibleImpl, &TGroupBox::SetVisibleImpl)
    , Enabled(this, &TGroupBox::GetEnabledImpl, &TGroupBox::SetEnabledImpl)
    , Caption(this, &TGroupBox::GetCaptionImpl, &TGroupBox::SetCaptionImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TGroupBox_Create(parent->Handle());
    no_vcl_TGroupBox_SetParent(handle_, parent->Handle());
}

TGroupBox::~TGroupBox()
{
    no_vcl_TGroupBox_Destroy(handle_);
}

int TGroupBox::GetLeftImpl(void* owner)
{
    return no_vcl_TGroupBox_GetLeft(static_cast<TGroupBox*>(owner)->handle_);
}

void TGroupBox::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TGroupBox_SetLeft(static_cast<TGroupBox*>(owner)->handle_, value);
}

int TGroupBox::GetTopImpl(void* owner)
{
    return no_vcl_TGroupBox_GetTop(static_cast<TGroupBox*>(owner)->handle_);
}

void TGroupBox::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TGroupBox_SetTop(static_cast<TGroupBox*>(owner)->handle_, value);
}

int TGroupBox::GetWidthImpl(void* owner)
{
    return no_vcl_TGroupBox_GetWidth(static_cast<TGroupBox*>(owner)->handle_);
}

void TGroupBox::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TGroupBox_SetWidth(static_cast<TGroupBox*>(owner)->handle_, value);
}

int TGroupBox::GetHeightImpl(void* owner)
{
    return no_vcl_TGroupBox_GetHeight(static_cast<TGroupBox*>(owner)->handle_);
}

void TGroupBox::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TGroupBox_SetHeight(static_cast<TGroupBox*>(owner)->handle_, value);
}

bool TGroupBox::GetVisibleImpl(void* owner)
{
    return no_vcl_TGroupBox_GetVisible(static_cast<TGroupBox*>(owner)->handle_) != 0;
}

void TGroupBox::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TGroupBox_SetVisible(static_cast<TGroupBox*>(owner)->handle_, value ? 1 : 0);
}

bool TGroupBox::GetEnabledImpl(void* owner)
{
    return no_vcl_TGroupBox_GetEnabled(static_cast<TGroupBox*>(owner)->handle_) != 0;
}

void TGroupBox::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TGroupBox_SetEnabled(static_cast<TGroupBox*>(owner)->handle_, value ? 1 : 0);
}

std::string TGroupBox::GetCaptionImpl(void* owner)
{
    return std::string(no_vcl_TGroupBox_GetCaption(static_cast<TGroupBox*>(owner)->handle_));
}

void TGroupBox::SetCaptionImpl(void* owner, const std::string& value)
{
    no_vcl_TGroupBox_SetCaption(static_cast<TGroupBox*>(owner)->handle_, value.c_str());
}

/* ---------------- TComboBox ---------------- */

std::unordered_map<no_vcl_obj_t, TComboBox*> TComboBox::s_registry;

TComboBox::TComboBox(TObject* parent)
    : Left(this, &TComboBox::GetLeftImpl, &TComboBox::SetLeftImpl)
    , Top(this, &TComboBox::GetTopImpl, &TComboBox::SetTopImpl)
    , Width(this, &TComboBox::GetWidthImpl, &TComboBox::SetWidthImpl)
    , Height(this, &TComboBox::GetHeightImpl, &TComboBox::SetHeightImpl)
    , Visible(this, &TComboBox::GetVisibleImpl, &TComboBox::SetVisibleImpl)
    , Enabled(this, &TComboBox::GetEnabledImpl, &TComboBox::SetEnabledImpl)
    , Text(this, &TComboBox::GetTextImpl, &TComboBox::SetTextImpl)
    , ItemIndex(this, &TComboBox::GetItemIndexImpl, &TComboBox::SetItemIndexImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TComboBox_Create(parent->Handle());
    no_vcl_TComboBox_SetParent(handle_, parent->Handle());

    s_registry[handle_] = this;
    no_vcl_TComboBox_SetOnChange(handle_, &TComboBox::ChangeTrampoline);
}

TComboBox::~TComboBox()
{
    s_registry.erase(handle_);
    no_vcl_TComboBox_Destroy(handle_);
}

void TComboBox::ItemsAdd(const std::string& text)
{
    no_vcl_TComboBox_Items_Add(handle_, text.c_str());
}

void TComboBox::ItemsClear()
{
    no_vcl_TComboBox_Items_Clear(handle_);
}

int TComboBox::ItemsCount() const
{
    return no_vcl_TComboBox_Items_Count(handle_);
}

std::string TComboBox::ItemsGetText(int index) const
{
    return std::string(no_vcl_TComboBox_Items_GetText(handle_, index));
}

void TComboBox::SetOnChange(std::function<void()> handler)
{
    onChange_ = std::move(handler);
}

void NO_VCL_CALL TComboBox::ChangeTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onChange_)
        it->second->onChange_();
}

int TComboBox::GetLeftImpl(void* owner)
{
    return no_vcl_TComboBox_GetLeft(static_cast<TComboBox*>(owner)->handle_);
}

void TComboBox::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TComboBox_SetLeft(static_cast<TComboBox*>(owner)->handle_, value);
}

int TComboBox::GetTopImpl(void* owner)
{
    return no_vcl_TComboBox_GetTop(static_cast<TComboBox*>(owner)->handle_);
}

void TComboBox::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TComboBox_SetTop(static_cast<TComboBox*>(owner)->handle_, value);
}

int TComboBox::GetWidthImpl(void* owner)
{
    return no_vcl_TComboBox_GetWidth(static_cast<TComboBox*>(owner)->handle_);
}

void TComboBox::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TComboBox_SetWidth(static_cast<TComboBox*>(owner)->handle_, value);
}

int TComboBox::GetHeightImpl(void* owner)
{
    return no_vcl_TComboBox_GetHeight(static_cast<TComboBox*>(owner)->handle_);
}

void TComboBox::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TComboBox_SetHeight(static_cast<TComboBox*>(owner)->handle_, value);
}

bool TComboBox::GetVisibleImpl(void* owner)
{
    return no_vcl_TComboBox_GetVisible(static_cast<TComboBox*>(owner)->handle_) != 0;
}

void TComboBox::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TComboBox_SetVisible(static_cast<TComboBox*>(owner)->handle_, value ? 1 : 0);
}

bool TComboBox::GetEnabledImpl(void* owner)
{
    return no_vcl_TComboBox_GetEnabled(static_cast<TComboBox*>(owner)->handle_) != 0;
}

void TComboBox::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TComboBox_SetEnabled(static_cast<TComboBox*>(owner)->handle_, value ? 1 : 0);
}

std::string TComboBox::GetTextImpl(void* owner)
{
    return std::string(no_vcl_TComboBox_GetText(static_cast<TComboBox*>(owner)->handle_));
}

void TComboBox::SetTextImpl(void* owner, const std::string& value)
{
    no_vcl_TComboBox_SetText(static_cast<TComboBox*>(owner)->handle_, value.c_str());
}

int TComboBox::GetItemIndexImpl(void* owner)
{
    return no_vcl_TComboBox_GetItemIndex(static_cast<TComboBox*>(owner)->handle_);
}

void TComboBox::SetItemIndexImpl(void* owner, const int& value)
{
    no_vcl_TComboBox_SetItemIndex(static_cast<TComboBox*>(owner)->handle_, value);
}

/* ---------------- TListBox ---------------- */

std::unordered_map<no_vcl_obj_t, TListBox*> TListBox::s_registry;

TListBox::TListBox(TObject* parent)
    : Left(this, &TListBox::GetLeftImpl, &TListBox::SetLeftImpl)
    , Top(this, &TListBox::GetTopImpl, &TListBox::SetTopImpl)
    , Width(this, &TListBox::GetWidthImpl, &TListBox::SetWidthImpl)
    , Height(this, &TListBox::GetHeightImpl, &TListBox::SetHeightImpl)
    , Visible(this, &TListBox::GetVisibleImpl, &TListBox::SetVisibleImpl)
    , Enabled(this, &TListBox::GetEnabledImpl, &TListBox::SetEnabledImpl)
    , ItemIndex(this, &TListBox::GetItemIndexImpl, &TListBox::SetItemIndexImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TListBox_Create(parent->Handle());
    no_vcl_TListBox_SetParent(handle_, parent->Handle());

    s_registry[handle_] = this;
    no_vcl_TListBox_SetOnClick(handle_, &TListBox::ClickTrampoline);
}

TListBox::~TListBox()
{
    s_registry.erase(handle_);
    no_vcl_TListBox_Destroy(handle_);
}

void TListBox::ItemsAdd(const std::string& text)
{
    no_vcl_TListBox_Items_Add(handle_, text.c_str());
}

void TListBox::ItemsClear()
{
    no_vcl_TListBox_Items_Clear(handle_);
}

int TListBox::ItemsCount() const
{
    return no_vcl_TListBox_Items_Count(handle_);
}

std::string TListBox::ItemsGetText(int index) const
{
    return std::string(no_vcl_TListBox_Items_GetText(handle_, index));
}

void TListBox::SetOnClick(std::function<void()> handler)
{
    onClick_ = std::move(handler);
}

void NO_VCL_CALL TListBox::ClickTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onClick_)
        it->second->onClick_();
}

int TListBox::GetLeftImpl(void* owner)
{
    return no_vcl_TListBox_GetLeft(static_cast<TListBox*>(owner)->handle_);
}

void TListBox::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TListBox_SetLeft(static_cast<TListBox*>(owner)->handle_, value);
}

int TListBox::GetTopImpl(void* owner)
{
    return no_vcl_TListBox_GetTop(static_cast<TListBox*>(owner)->handle_);
}

void TListBox::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TListBox_SetTop(static_cast<TListBox*>(owner)->handle_, value);
}

int TListBox::GetWidthImpl(void* owner)
{
    return no_vcl_TListBox_GetWidth(static_cast<TListBox*>(owner)->handle_);
}

void TListBox::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TListBox_SetWidth(static_cast<TListBox*>(owner)->handle_, value);
}

int TListBox::GetHeightImpl(void* owner)
{
    return no_vcl_TListBox_GetHeight(static_cast<TListBox*>(owner)->handle_);
}

void TListBox::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TListBox_SetHeight(static_cast<TListBox*>(owner)->handle_, value);
}

bool TListBox::GetVisibleImpl(void* owner)
{
    return no_vcl_TListBox_GetVisible(static_cast<TListBox*>(owner)->handle_) != 0;
}

void TListBox::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TListBox_SetVisible(static_cast<TListBox*>(owner)->handle_, value ? 1 : 0);
}

bool TListBox::GetEnabledImpl(void* owner)
{
    return no_vcl_TListBox_GetEnabled(static_cast<TListBox*>(owner)->handle_) != 0;
}

void TListBox::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TListBox_SetEnabled(static_cast<TListBox*>(owner)->handle_, value ? 1 : 0);
}

int TListBox::GetItemIndexImpl(void* owner)
{
    return no_vcl_TListBox_GetItemIndex(static_cast<TListBox*>(owner)->handle_);
}

void TListBox::SetItemIndexImpl(void* owner, const int& value)
{
    no_vcl_TListBox_SetItemIndex(static_cast<TListBox*>(owner)->handle_, value);
}

/* ---------------- TMemo ---------------- */

std::unordered_map<no_vcl_obj_t, TMemo*> TMemo::s_registry;

TMemo::TMemo(TObject* parent)
    : Left(this, &TMemo::GetLeftImpl, &TMemo::SetLeftImpl)
    , Top(this, &TMemo::GetTopImpl, &TMemo::SetTopImpl)
    , Width(this, &TMemo::GetWidthImpl, &TMemo::SetWidthImpl)
    , Height(this, &TMemo::GetHeightImpl, &TMemo::SetHeightImpl)
    , Visible(this, &TMemo::GetVisibleImpl, &TMemo::SetVisibleImpl)
    , Enabled(this, &TMemo::GetEnabledImpl, &TMemo::SetEnabledImpl)
    , ReadOnly(this, &TMemo::GetReadOnlyImpl, &TMemo::SetReadOnlyImpl)
    , ScrollBars(this, &TMemo::GetScrollBarsImpl, &TMemo::SetScrollBarsImpl)
{
    assert(parent != nullptr);
    handle_ = no_vcl_TMemo_Create(parent->Handle());
    no_vcl_TMemo_SetParent(handle_, parent->Handle());

    s_registry[handle_] = this;
    no_vcl_TMemo_SetOnChange(handle_, &TMemo::ChangeTrampoline);
}

TMemo::~TMemo()
{
    s_registry.erase(handle_);
    no_vcl_TMemo_Destroy(handle_);
}

void TMemo::LinesAdd(const std::string& text)
{
    no_vcl_TMemo_Lines_Add(handle_, text.c_str());
}

void TMemo::LinesClear()
{
    no_vcl_TMemo_Lines_Clear(handle_);
}

int TMemo::LinesCount() const
{
    return no_vcl_TMemo_Lines_Count(handle_);
}

std::string TMemo::LinesGetText(int index) const
{
    return std::string(no_vcl_TMemo_Lines_GetText(handle_, index));
}

void TMemo::SetOnChange(std::function<void()> handler)
{
    onChange_ = std::move(handler);
}

void NO_VCL_CALL TMemo::ChangeTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onChange_)
        it->second->onChange_();
}

int TMemo::GetLeftImpl(void* owner)
{
    return no_vcl_TMemo_GetLeft(static_cast<TMemo*>(owner)->handle_);
}

void TMemo::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TMemo_SetLeft(static_cast<TMemo*>(owner)->handle_, value);
}

int TMemo::GetTopImpl(void* owner)
{
    return no_vcl_TMemo_GetTop(static_cast<TMemo*>(owner)->handle_);
}

void TMemo::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TMemo_SetTop(static_cast<TMemo*>(owner)->handle_, value);
}

int TMemo::GetWidthImpl(void* owner)
{
    return no_vcl_TMemo_GetWidth(static_cast<TMemo*>(owner)->handle_);
}

void TMemo::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TMemo_SetWidth(static_cast<TMemo*>(owner)->handle_, value);
}

int TMemo::GetHeightImpl(void* owner)
{
    return no_vcl_TMemo_GetHeight(static_cast<TMemo*>(owner)->handle_);
}

void TMemo::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TMemo_SetHeight(static_cast<TMemo*>(owner)->handle_, value);
}

bool TMemo::GetVisibleImpl(void* owner)
{
    return no_vcl_TMemo_GetVisible(static_cast<TMemo*>(owner)->handle_) != 0;
}

void TMemo::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TMemo_SetVisible(static_cast<TMemo*>(owner)->handle_, value ? 1 : 0);
}

bool TMemo::GetEnabledImpl(void* owner)
{
    return no_vcl_TMemo_GetEnabled(static_cast<TMemo*>(owner)->handle_) != 0;
}

void TMemo::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TMemo_SetEnabled(static_cast<TMemo*>(owner)->handle_, value ? 1 : 0);
}

bool TMemo::GetReadOnlyImpl(void* owner)
{
    return no_vcl_TMemo_GetReadOnly(static_cast<TMemo*>(owner)->handle_) != 0;
}

void TMemo::SetReadOnlyImpl(void* owner, const bool& value)
{
    no_vcl_TMemo_SetReadOnly(static_cast<TMemo*>(owner)->handle_, value ? 1 : 0);
}

int TMemo::GetScrollBarsImpl(void* owner)
{
    return no_vcl_TMemo_GetScrollBars(static_cast<TMemo*>(owner)->handle_);
}

void TMemo::SetScrollBarsImpl(void* owner, const int& value)
{
    no_vcl_TMemo_SetScrollBars(static_cast<TMemo*>(owner)->handle_, value);
}

/* ---------------- TTimer ---------------- */

std::unordered_map<no_vcl_obj_t, TTimer*> TTimer::s_registry;

TTimer::TTimer(TObject* owner)
    : Interval(this, &TTimer::GetIntervalImpl, &TTimer::SetIntervalImpl)
    , Enabled(this, &TTimer::GetEnabledImpl, &TTimer::SetEnabledImpl)
{
    assert(owner != nullptr);
    handle_ = no_vcl_TTimer_Create(owner->Handle());

    s_registry[handle_] = this;
    no_vcl_TTimer_SetOnTimer(handle_, &TTimer::TimerTrampoline);
}

TTimer::~TTimer()
{
    s_registry.erase(handle_);
    no_vcl_TTimer_Destroy(handle_);
}

void TTimer::SetOnTimer(std::function<void()> handler)
{
    onTimer_ = std::move(handler);
}

void NO_VCL_CALL TTimer::TimerTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onTimer_)
        it->second->onTimer_();
}

int TTimer::GetIntervalImpl(void* owner)
{
    return no_vcl_TTimer_GetInterval(static_cast<TTimer*>(owner)->handle_);
}

void TTimer::SetIntervalImpl(void* owner, const int& value)
{
    no_vcl_TTimer_SetInterval(static_cast<TTimer*>(owner)->handle_, value);
}

bool TTimer::GetEnabledImpl(void* owner)
{
    return no_vcl_TTimer_GetEnabled(static_cast<TTimer*>(owner)->handle_) != 0;
}

void TTimer::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TTimer_SetEnabled(static_cast<TTimer*>(owner)->handle_, value ? 1 : 0);
}

/* ---------------- TPen ---------------- */

TPen::TPen(no_vcl_obj_t handle)
    : Color(this, &TPen::GetColorImpl, &TPen::SetColorImpl)
    , Width(this, &TPen::GetWidthImpl, &TPen::SetWidthImpl)
    , handle_(handle)
{}

TColor TPen::GetColorImpl(void* owner)
{
    return static_cast<TColor>(no_vcl_TPen_GetColor(static_cast<TPen*>(owner)->handle_));
}

void TPen::SetColorImpl(void* owner, const TColor& value)
{
    no_vcl_TPen_SetColor(static_cast<TPen*>(owner)->handle_, value);
}

int TPen::GetWidthImpl(void* owner)
{
    return no_vcl_TPen_GetWidth(static_cast<TPen*>(owner)->handle_);
}

void TPen::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TPen_SetWidth(static_cast<TPen*>(owner)->handle_, value);
}

/* ---------------- TBrush ---------------- */

TBrush::TBrush(no_vcl_obj_t handle)
    : Color(this, &TBrush::GetColorImpl, &TBrush::SetColorImpl)
    , handle_(handle)
{}

TColor TBrush::GetColorImpl(void* owner)
{
    return static_cast<TColor>(no_vcl_TBrush_GetColor(static_cast<TBrush*>(owner)->handle_));
}

void TBrush::SetColorImpl(void* owner, const TColor& value)
{
    no_vcl_TBrush_SetColor(static_cast<TBrush*>(owner)->handle_, value);
}

/* ---------------- TFont ---------------- */

TFont::TFont(no_vcl_obj_t handle)
    : Name(this, &TFont::GetNameImpl, &TFont::SetNameImpl)
    , Size(this, &TFont::GetSizeImpl, &TFont::SetSizeImpl)
    , Color(this, &TFont::GetColorImpl, &TFont::SetColorImpl)
    , handle_(handle)
{}

std::string TFont::GetNameImpl(void* owner)
{
    return std::string(no_vcl_TFont_GetName(static_cast<TFont*>(owner)->handle_));
}

void TFont::SetNameImpl(void* owner, const std::string& value)
{
    no_vcl_TFont_SetName(static_cast<TFont*>(owner)->handle_, value.c_str());
}

int TFont::GetSizeImpl(void* owner)
{
    return no_vcl_TFont_GetSize(static_cast<TFont*>(owner)->handle_);
}

void TFont::SetSizeImpl(void* owner, const int& value)
{
    no_vcl_TFont_SetSize(static_cast<TFont*>(owner)->handle_, value);
}

TColor TFont::GetColorImpl(void* owner)
{
    return static_cast<TColor>(no_vcl_TFont_GetColor(static_cast<TFont*>(owner)->handle_));
}

void TFont::SetColorImpl(void* owner, const TColor& value)
{
    no_vcl_TFont_SetColor(static_cast<TFont*>(owner)->handle_, value);
}

/* ---------------- TCanvas ---------------- */

TCanvas::TCanvas(no_vcl_obj_t handle)
    : handle_(handle)
    , Pen(no_vcl_TCanvas_GetPen(handle))
    , Brush(no_vcl_TCanvas_GetBrush(handle))
    , Font(no_vcl_TCanvas_GetFont(handle))
{}

void TCanvas::MoveTo(int x, int y)
{
    no_vcl_TCanvas_MoveTo(handle_, x, y);
}

void TCanvas::LineTo(int x, int y)
{
    no_vcl_TCanvas_LineTo(handle_, x, y);
}

void TCanvas::Rectangle(int x1, int y1, int x2, int y2)
{
    no_vcl_TCanvas_Rectangle(handle_, x1, y1, x2, y2);
}

void TCanvas::Ellipse(int x1, int y1, int x2, int y2)
{
    no_vcl_TCanvas_Ellipse(handle_, x1, y1, x2, y2);
}

void TCanvas::TextOut(int x, int y, const std::string& text)
{
    no_vcl_TCanvas_TextOut(handle_, x, y, text.c_str());
}

/* ---------------- TPaintBox ---------------- */

std::unordered_map<no_vcl_obj_t, TPaintBox*> TPaintBox::s_registry;

no_vcl_obj_t TPaintBox::MakeHandle(TObject* parent)
{
    assert(parent != nullptr);
    no_vcl_obj_t h = no_vcl_TPaintBox_Create(parent->Handle());
    no_vcl_TPaintBox_SetParent(h, parent->Handle());
    return h;
}

TPaintBox::TPaintBox(TObject* parent)
    : TObject(MakeHandle(parent))
    , Left(this, &TPaintBox::GetLeftImpl, &TPaintBox::SetLeftImpl)
    , Top(this, &TPaintBox::GetTopImpl, &TPaintBox::SetTopImpl)
    , Width(this, &TPaintBox::GetWidthImpl, &TPaintBox::SetWidthImpl)
    , Height(this, &TPaintBox::GetHeightImpl, &TPaintBox::SetHeightImpl)
    , Visible(this, &TPaintBox::GetVisibleImpl, &TPaintBox::SetVisibleImpl)
    , Enabled(this, &TPaintBox::GetEnabledImpl, &TPaintBox::SetEnabledImpl)
    , Canvas(no_vcl_TPaintBox_GetCanvas(handle_))
{
    s_registry[handle_] = this;
    no_vcl_TPaintBox_SetOnPaint(handle_, &TPaintBox::PaintTrampoline);
}

TPaintBox::~TPaintBox()
{
    s_registry.erase(handle_);
    no_vcl_TPaintBox_Destroy(handle_);
}

void TPaintBox::SetOnPaint(std::function<void()> handler)
{
    onPaint_ = std::move(handler);
}

void NO_VCL_CALL TPaintBox::PaintTrampoline(no_vcl_obj_t sender)
{
    auto it = s_registry.find(sender);
    if (it != s_registry.end() && it->second->onPaint_)
        it->second->onPaint_();
}

int TPaintBox::GetLeftImpl(void* owner)
{
    return no_vcl_TPaintBox_GetLeft(static_cast<TPaintBox*>(owner)->handle_);
}

void TPaintBox::SetLeftImpl(void* owner, const int& value)
{
    no_vcl_TPaintBox_SetLeft(static_cast<TPaintBox*>(owner)->handle_, value);
}

int TPaintBox::GetTopImpl(void* owner)
{
    return no_vcl_TPaintBox_GetTop(static_cast<TPaintBox*>(owner)->handle_);
}

void TPaintBox::SetTopImpl(void* owner, const int& value)
{
    no_vcl_TPaintBox_SetTop(static_cast<TPaintBox*>(owner)->handle_, value);
}

int TPaintBox::GetWidthImpl(void* owner)
{
    return no_vcl_TPaintBox_GetWidth(static_cast<TPaintBox*>(owner)->handle_);
}

void TPaintBox::SetWidthImpl(void* owner, const int& value)
{
    no_vcl_TPaintBox_SetWidth(static_cast<TPaintBox*>(owner)->handle_, value);
}

int TPaintBox::GetHeightImpl(void* owner)
{
    return no_vcl_TPaintBox_GetHeight(static_cast<TPaintBox*>(owner)->handle_);
}

void TPaintBox::SetHeightImpl(void* owner, const int& value)
{
    no_vcl_TPaintBox_SetHeight(static_cast<TPaintBox*>(owner)->handle_, value);
}

bool TPaintBox::GetVisibleImpl(void* owner)
{
    return no_vcl_TPaintBox_GetVisible(static_cast<TPaintBox*>(owner)->handle_) != 0;
}

void TPaintBox::SetVisibleImpl(void* owner, const bool& value)
{
    no_vcl_TPaintBox_SetVisible(static_cast<TPaintBox*>(owner)->handle_, value ? 1 : 0);
}

bool TPaintBox::GetEnabledImpl(void* owner)
{
    return no_vcl_TPaintBox_GetEnabled(static_cast<TPaintBox*>(owner)->handle_) != 0;
}

void TPaintBox::SetEnabledImpl(void* owner, const bool& value)
{
    no_vcl_TPaintBox_SetEnabled(static_cast<TPaintBox*>(owner)->handle_, value ? 1 : 0);
}

} // namespace no_vcl
