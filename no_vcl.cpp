#include "no_vcl.hpp"

namespace no_vcl
{

/* ---------------- TComponent ---------------- */

TComponent::TComponent(no_vcl_obj_t handle)
    : TPersistent(handle)
{
    static bool callbackInstalled = false;
    if (!callbackInstalled)
    {
        no_vcl_FreeNotify_SetCallback(&TComponent::FreeNotifyTrampoline);
        callbackInstalled = true;
    }

    if (handle_)
        Registry()[handle_] = this;
}

// ラッパーは LCL オブジェクトの破棄通知(FreeNotifyTrampoline)からしか delete されないため、
// ここで LCL オブジェクトを破棄することはない(LCL オブジェクトは既に破棄の途中にある)。
TComponent::~TComponent()
{
    Registry().erase(handle_);
}

void TComponent::Free()
{
    no_vcl_TComponent_Destroy(handle_);
}

void NO_VCL_CALL TComponent::FreeNotifyTrampoline(no_vcl_obj_t handle)
{
    std::unordered_map<no_vcl_obj_t, TComponent*>& registry = Registry();
    auto it = registry.find(handle);
    if (it == registry.end())
        return;

    TComponent* self = it->second;
    registry.erase(it);
    delete self;
}

std::unordered_map<no_vcl_obj_t, TComponent*>& TComponent::Registry()
{
    static std::unordered_map<no_vcl_obj_t, TComponent*> registry;
    return registry;
}

TComponent* TComponent::FromHandle(no_vcl_obj_t handle)
{
    std::unordered_map<no_vcl_obj_t, TComponent*>& registry = Registry();
    auto it = registry.find(handle);
    return it != registry.end() ? it->second : nullptr;
}

/* ---------------- TControl ---------------- */

TControl::TControl(no_vcl_obj_t handle)
    : TComponent(handle)
    , Parent(this, &TControl::GetParentImpl, &TControl::SetParentImpl)
    , Left(this, &TControl::GetLeftImpl, &TControl::SetLeftImpl)
    , Top(this, &TControl::GetTopImpl, &TControl::SetTopImpl)
    , Width(this, &TControl::GetWidthImpl, &TControl::SetWidthImpl)
    , Height(this, &TControl::GetHeightImpl, &TControl::SetHeightImpl)
    , Visible(this, &TControl::GetVisibleImpl, &TControl::SetVisibleImpl)
    , Enabled(this, &TControl::GetEnabledImpl, &TControl::SetEnabledImpl)
    , Caption(this, &TControl::GetCaptionImpl, &TControl::SetCaptionImpl)
    , Text(this, &TControl::GetTextImpl, &TControl::SetTextImpl)
{}

void TControl::Show() { no_vcl_TControl_Show(handle_); }
void TControl::Hide() { no_vcl_TControl_Hide(handle_); }

void TControl::SetOnClick(std::function<void()> handler)
{
    onClick_ = std::move(handler);
    if (!onClickHooked_)
    {
        no_vcl_TControl_SetOnClick(handle_, &TControl::ClickTrampoline);
        onClickHooked_ = true;
    }
}

void NO_VCL_CALL TControl::ClickTrampoline(no_vcl_obj_t sender)
{
    TControl* self = static_cast<TControl*>(FromHandle(sender));
    if (self && self->onClick_)
        self->onClick_();
}

TWinControl* TControl::GetParentImpl(TObject* owner)
{
    // LCL の Parent は常に TWinControl 派生で、C++ 側の階層も LCL の部分列なので、
    // そのハンドルに登録されたラッパーは TWinControl 派生であることが保証される。
    // C++ ラッパーを介さずに作られた親の場合は nullptr になる。
    return static_cast<TWinControl*>(FromHandle(no_vcl_TControl_GetParent(owner->Handle())));
}

void TControl::SetParentImpl(TObject* owner, TWinControl* const& value)
{
    no_vcl_TControl_SetParent(owner->Handle(), HandleOf(value));
}

int  TControl::GetLeftImpl(TObject* owner)                      { return no_vcl_TControl_GetLeft(owner->Handle()); }
void TControl::SetLeftImpl(TObject* owner, const int& value)    { no_vcl_TControl_SetLeft(owner->Handle(), value); }
int  TControl::GetTopImpl(TObject* owner)                       { return no_vcl_TControl_GetTop(owner->Handle()); }
void TControl::SetTopImpl(TObject* owner, const int& value)     { no_vcl_TControl_SetTop(owner->Handle(), value); }
int  TControl::GetWidthImpl(TObject* owner)                     { return no_vcl_TControl_GetWidth(owner->Handle()); }
void TControl::SetWidthImpl(TObject* owner, const int& value)   { no_vcl_TControl_SetWidth(owner->Handle(), value); }
int  TControl::GetHeightImpl(TObject* owner)                    { return no_vcl_TControl_GetHeight(owner->Handle()); }
void TControl::SetHeightImpl(TObject* owner, const int& value)  { no_vcl_TControl_SetHeight(owner->Handle(), value); }
bool TControl::GetVisibleImpl(TObject* owner)                   { return no_vcl_TControl_GetVisible(owner->Handle()) != 0; }
void TControl::SetVisibleImpl(TObject* owner, const bool& value){ no_vcl_TControl_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TControl::GetEnabledImpl(TObject* owner)                   { return no_vcl_TControl_GetEnabled(owner->Handle()) != 0; }
void TControl::SetEnabledImpl(TObject* owner, const bool& value){ no_vcl_TControl_SetEnabled(owner->Handle(), value ? 1 : 0); }

std::string TControl::GetCaptionImpl(TObject* owner)
{
    return std::string(no_vcl_TControl_GetCaption(owner->Handle()));
}

void TControl::SetCaptionImpl(TObject* owner, const std::string& value)
{
    no_vcl_TControl_SetCaption(owner->Handle(), value.c_str());
}

std::string TControl::GetTextImpl(TObject* owner)
{
    return std::string(no_vcl_TControl_GetText(owner->Handle()));
}

void TControl::SetTextImpl(TObject* owner, const std::string& value)
{
    no_vcl_TControl_SetText(owner->Handle(), value.c_str());
}

/* ---------------- Form ---------------- */

void TCustomForm::Show()      { no_vcl_TCustomForm_Show(handle_); }
void TCustomForm::Hide()      { no_vcl_TCustomForm_Hide(handle_); }
int  TCustomForm::ShowModal() { return no_vcl_TCustomForm_ShowModal(handle_); }
void TCustomForm::Close()     { no_vcl_TCustomForm_Close(handle_); }

TForm::TForm(TComponent* AOwner)
    : TCustomForm(no_vcl_TForm_Create(HandleOf(AOwner)))
{}

/* ---------------- Panel / GroupBox / Label ---------------- */

TPanel::TPanel(TComponent* AOwner)
    : TCustomPanel(no_vcl_TPanel_Create(HandleOf(AOwner)))
{}

TGroupBox::TGroupBox(TComponent* AOwner)
    : TCustomGroupBox(no_vcl_TGroupBox_Create(HandleOf(AOwner)))
{}

TLabel::TLabel(TComponent* AOwner)
    : TCustomLabel(no_vcl_TLabel_Create(HandleOf(AOwner)))
{}

/* ---------------- Button / CheckBox / RadioButton ---------------- */

TButtonControl::TButtonControl(no_vcl_obj_t handle)
    : TWinControl(handle)
    , Checked(this, &TButtonControl::GetCheckedImpl, &TButtonControl::SetCheckedImpl)
{}

bool TButtonControl::GetCheckedImpl(TObject* owner)
{
    return no_vcl_TButtonControl_GetChecked(owner->Handle()) != 0;
}

void TButtonControl::SetCheckedImpl(TObject* owner, const bool& value)
{
    no_vcl_TButtonControl_SetChecked(owner->Handle(), value ? 1 : 0);
}

TButton::TButton(TComponent* AOwner)
    : TCustomButton(no_vcl_TButton_Create(HandleOf(AOwner)))
{}

TCheckBox::TCheckBox(TComponent* AOwner)
    : TCustomCheckBox(no_vcl_TCheckBox_Create(HandleOf(AOwner)))
{}

TRadioButton::TRadioButton(TComponent* AOwner)
    : TCustomCheckBox(no_vcl_TRadioButton_Create(HandleOf(AOwner)))
{}

/* ---------------- Edit / Memo ---------------- */

TCustomEdit::TCustomEdit(no_vcl_obj_t handle)
    : TWinControl(handle)
    , MaxLength(this, &TCustomEdit::GetMaxLengthImpl, &TCustomEdit::SetMaxLengthImpl)
    , ReadOnly(this, &TCustomEdit::GetReadOnlyImpl, &TCustomEdit::SetReadOnlyImpl)
{}

void TCustomEdit::SetOnChange(std::function<void()> handler)
{
    onChange_ = std::move(handler);
    if (!onChangeHooked_)
    {
        no_vcl_TCustomEdit_SetOnChange(handle_, &TCustomEdit::ChangeTrampoline);
        onChangeHooked_ = true;
    }
}

void NO_VCL_CALL TCustomEdit::ChangeTrampoline(no_vcl_obj_t sender)
{
    TCustomEdit* self = static_cast<TCustomEdit*>(FromHandle(sender));
    if (self && self->onChange_)
        self->onChange_();
}

int  TCustomEdit::GetMaxLengthImpl(TObject* owner)                   { return no_vcl_TCustomEdit_GetMaxLength(owner->Handle()); }
void TCustomEdit::SetMaxLengthImpl(TObject* owner, const int& value) { no_vcl_TCustomEdit_SetMaxLength(owner->Handle(), value); }
bool TCustomEdit::GetReadOnlyImpl(TObject* owner)                    { return no_vcl_TCustomEdit_GetReadOnly(owner->Handle()) != 0; }
void TCustomEdit::SetReadOnlyImpl(TObject* owner, const bool& value) { no_vcl_TCustomEdit_SetReadOnly(owner->Handle(), value ? 1 : 0); }

TEdit::TEdit(TComponent* AOwner)
    : TCustomEdit(no_vcl_TEdit_Create(HandleOf(AOwner)))
{}

TCustomMemo::TCustomMemo(no_vcl_obj_t handle)
    : TCustomEdit(handle)
    , ScrollBars(this, &TCustomMemo::GetScrollBarsImpl, &TCustomMemo::SetScrollBarsImpl)
{}

void TCustomMemo::LinesAdd(const std::string& text) { no_vcl_TCustomMemo_Lines_Add(handle_, text.c_str()); }
void TCustomMemo::LinesClear()                      { no_vcl_TCustomMemo_Lines_Clear(handle_); }
int  TCustomMemo::LinesCount() const                { return no_vcl_TCustomMemo_Lines_Count(handle_); }

std::string TCustomMemo::LinesGetText(int index) const
{
    return std::string(no_vcl_TCustomMemo_Lines_GetText(handle_, index));
}

int  TCustomMemo::GetScrollBarsImpl(TObject* owner)                   { return no_vcl_TCustomMemo_GetScrollBars(owner->Handle()); }
void TCustomMemo::SetScrollBarsImpl(TObject* owner, const int& value) { no_vcl_TCustomMemo_SetScrollBars(owner->Handle(), value); }

TMemo::TMemo(TComponent* AOwner)
    : TCustomMemo(no_vcl_TMemo_Create(HandleOf(AOwner)))
{}

/* ---------------- ComboBox / ListBox ---------------- */

TCustomComboBox::TCustomComboBox(no_vcl_obj_t handle)
    : TWinControl(handle)
    , ItemIndex(this, &TCustomComboBox::GetItemIndexImpl, &TCustomComboBox::SetItemIndexImpl)
{}

void TCustomComboBox::ItemsAdd(const std::string& text) { no_vcl_TCustomComboBox_Items_Add(handle_, text.c_str()); }
void TCustomComboBox::ItemsClear()                      { no_vcl_TCustomComboBox_Items_Clear(handle_); }
int  TCustomComboBox::ItemsCount() const                { return no_vcl_TCustomComboBox_Items_Count(handle_); }

std::string TCustomComboBox::ItemsGetText(int index) const
{
    return std::string(no_vcl_TCustomComboBox_Items_GetText(handle_, index));
}

int  TCustomComboBox::GetItemIndexImpl(TObject* owner)                   { return no_vcl_TCustomComboBox_GetItemIndex(owner->Handle()); }
void TCustomComboBox::SetItemIndexImpl(TObject* owner, const int& value) { no_vcl_TCustomComboBox_SetItemIndex(owner->Handle(), value); }

TComboBox::TComboBox(TComponent* AOwner)
    : TCustomComboBox(no_vcl_TComboBox_Create(HandleOf(AOwner)))
{}

void TComboBox::SetOnChange(std::function<void()> handler)
{
    onChange_ = std::move(handler);
    if (!onChangeHooked_)
    {
        no_vcl_TComboBox_SetOnChange(handle_, &TComboBox::ChangeTrampoline);
        onChangeHooked_ = true;
    }
}

void NO_VCL_CALL TComboBox::ChangeTrampoline(no_vcl_obj_t sender)
{
    TComboBox* self = static_cast<TComboBox*>(FromHandle(sender));
    if (self && self->onChange_)
        self->onChange_();
}

TCustomListBox::TCustomListBox(no_vcl_obj_t handle)
    : TWinControl(handle)
    , ItemIndex(this, &TCustomListBox::GetItemIndexImpl, &TCustomListBox::SetItemIndexImpl)
{}

void TCustomListBox::ItemsAdd(const std::string& text) { no_vcl_TCustomListBox_Items_Add(handle_, text.c_str()); }
void TCustomListBox::ItemsClear()                      { no_vcl_TCustomListBox_Items_Clear(handle_); }
int  TCustomListBox::ItemsCount() const                { return no_vcl_TCustomListBox_Items_Count(handle_); }

std::string TCustomListBox::ItemsGetText(int index) const
{
    return std::string(no_vcl_TCustomListBox_Items_GetText(handle_, index));
}

int  TCustomListBox::GetItemIndexImpl(TObject* owner)                   { return no_vcl_TCustomListBox_GetItemIndex(owner->Handle()); }
void TCustomListBox::SetItemIndexImpl(TObject* owner, const int& value) { no_vcl_TCustomListBox_SetItemIndex(owner->Handle(), value); }

TListBox::TListBox(TComponent* AOwner)
    : TCustomListBox(no_vcl_TListBox_Create(HandleOf(AOwner)))
{}

/* ---------------- Canvas ---------------- */

TPen::TPen(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Color(this, &TPen::GetColorImpl, &TPen::SetColorImpl)
    , Width(this, &TPen::GetWidthImpl, &TPen::SetWidthImpl)
{}

TColor TPen::GetColorImpl(TObject* owner)                      { return static_cast<TColor>(no_vcl_TPen_GetColor(owner->Handle())); }
void   TPen::SetColorImpl(TObject* owner, const TColor& value) { no_vcl_TPen_SetColor(owner->Handle(), value); }
int    TPen::GetWidthImpl(TObject* owner)                      { return no_vcl_TPen_GetWidth(owner->Handle()); }
void   TPen::SetWidthImpl(TObject* owner, const int& value)    { no_vcl_TPen_SetWidth(owner->Handle(), value); }

TBrush::TBrush(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Color(this, &TBrush::GetColorImpl, &TBrush::SetColorImpl)
{}

TColor TBrush::GetColorImpl(TObject* owner)                      { return static_cast<TColor>(no_vcl_TBrush_GetColor(owner->Handle())); }
void   TBrush::SetColorImpl(TObject* owner, const TColor& value) { no_vcl_TBrush_SetColor(owner->Handle(), value); }

TFont::TFont(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Name(this, &TFont::GetNameImpl, &TFont::SetNameImpl)
    , Size(this, &TFont::GetSizeImpl, &TFont::SetSizeImpl)
    , Color(this, &TFont::GetColorImpl, &TFont::SetColorImpl)
{}

std::string TFont::GetNameImpl(TObject* owner)
{
    return std::string(no_vcl_TFont_GetName(owner->Handle()));
}

void TFont::SetNameImpl(TObject* owner, const std::string& value)
{
    no_vcl_TFont_SetName(owner->Handle(), value.c_str());
}

int    TFont::GetSizeImpl(TObject* owner)                      { return no_vcl_TFont_GetSize(owner->Handle()); }
void   TFont::SetSizeImpl(TObject* owner, const int& value)    { no_vcl_TFont_SetSize(owner->Handle(), value); }
TColor TFont::GetColorImpl(TObject* owner)                     { return static_cast<TColor>(no_vcl_TFont_GetColor(owner->Handle())); }
void   TFont::SetColorImpl(TObject* owner, const TColor& value){ no_vcl_TFont_SetColor(owner->Handle(), value); }

TCanvas::TCanvas(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Pen(no_vcl_TCanvas_GetPen(handle))
    , Brush(no_vcl_TCanvas_GetBrush(handle))
    , Font(no_vcl_TCanvas_GetFont(handle))
{}

void TCanvas::MoveTo(int x, int y)                        { no_vcl_TCanvas_MoveTo(handle_, x, y); }
void TCanvas::LineTo(int x, int y)                        { no_vcl_TCanvas_LineTo(handle_, x, y); }
void TCanvas::Rectangle(int x1, int y1, int x2, int y2)   { no_vcl_TCanvas_Rectangle(handle_, x1, y1, x2, y2); }
void TCanvas::Ellipse(int x1, int y1, int x2, int y2)     { no_vcl_TCanvas_Ellipse(handle_, x1, y1, x2, y2); }
void TCanvas::TextOut(int x, int y, const std::string& t) { no_vcl_TCanvas_TextOut(handle_, x, y, t.c_str()); }

TPaintBox::TPaintBox(TComponent* AOwner)
    : TGraphicControl(no_vcl_TPaintBox_Create(HandleOf(AOwner)))
    , Canvas(no_vcl_TPaintBox_GetCanvas(handle_))
{}

void TPaintBox::SetOnPaint(std::function<void()> handler)
{
    onPaint_ = std::move(handler);
    if (!onPaintHooked_)
    {
        no_vcl_TPaintBox_SetOnPaint(handle_, &TPaintBox::PaintTrampoline);
        onPaintHooked_ = true;
    }
}

void NO_VCL_CALL TPaintBox::PaintTrampoline(no_vcl_obj_t sender)
{
    TPaintBox* self = static_cast<TPaintBox*>(FromHandle(sender));
    if (self && self->onPaint_)
        self->onPaint_();
}

/* ---------------- Timer ---------------- */

TCustomTimer::TCustomTimer(no_vcl_obj_t handle)
    : TComponent(handle)
    , Interval(this, &TCustomTimer::GetIntervalImpl, &TCustomTimer::SetIntervalImpl)
    , Enabled(this, &TCustomTimer::GetEnabledImpl, &TCustomTimer::SetEnabledImpl)
{}

void TCustomTimer::SetOnTimer(std::function<void()> handler)
{
    onTimer_ = std::move(handler);
    if (!onTimerHooked_)
    {
        no_vcl_TCustomTimer_SetOnTimer(handle_, &TCustomTimer::TimerTrampoline);
        onTimerHooked_ = true;
    }
}

void NO_VCL_CALL TCustomTimer::TimerTrampoline(no_vcl_obj_t sender)
{
    TCustomTimer* self = static_cast<TCustomTimer*>(FromHandle(sender));
    if (self && self->onTimer_)
        self->onTimer_();
}

int  TCustomTimer::GetIntervalImpl(TObject* owner)                   { return no_vcl_TCustomTimer_GetInterval(owner->Handle()); }
void TCustomTimer::SetIntervalImpl(TObject* owner, const int& value) { no_vcl_TCustomTimer_SetInterval(owner->Handle(), value); }
bool TCustomTimer::GetEnabledImpl(TObject* owner)                    { return no_vcl_TCustomTimer_GetEnabled(owner->Handle()) != 0; }
void TCustomTimer::SetEnabledImpl(TObject* owner, const bool& value) { no_vcl_TCustomTimer_SetEnabled(owner->Handle(), value ? 1 : 0); }

TTimer::TTimer(TComponent* AOwner)
    : TCustomTimer(no_vcl_TTimer_Create(HandleOf(AOwner)))
{}

} // namespace no_vcl
