#include "no_vcl.hpp"

#include <cstdlib>

namespace no_vcl
{

namespace
{

// イベントの Setter で共通の処理: ハンドラを保持し、最初に空でないハンドラが設定されたときだけブリッジを登録する。
template<typename Event, typename Callback>
void SetSimpleEvent(no_vcl_obj_t handle, Event& slot, bool& hooked, const Event& value,
                     void (NO_VCL_CALL *setOn)(no_vcl_obj_t, Callback, void*), Callback trampoline)
{
    slot = value;
    if (value && !hooked)
    {
        setOn(handle, trampoline, nullptr);
        hooked = true;
    }
}

// ハンドラの中でハンドラ自身を差し替えても実行中の std::function が破棄されないよう、コピーしてから呼ぶ。
void CallNotify(const TNotifyEvent& handler, TObject* sender)
{
    if (handler)
        handler(sender);
}

} // namespace

/* ---------------- TComponent ---------------- */

TComponent::TComponent(no_vcl_obj_t handle)
    : TPersistent(handle)
{
    static bool callbackInstalled = false;
    if (!callbackInstalled)
    {
        no_vcl_FreeNotify_SetCallback(&TComponent::FreeNotifyTrampoline, nullptr);
        callbackInstalled = true;
    }

    if (handle_)
        Registry()[handle_] = this;
}

// ラッパーは通常、LCL オブジェクトの破棄通知(FreeNotifyTrampoline)からしか delete されず、
// そのとき LCL オブジェクトは既に破棄の途中にある。
// 例外は派生クラスのコンストラクタが例外を投げた場合で、このときは LCL オブジェクトだけが取り残されるため、ここで破棄する。
// 先にレジストリから外すので、この破棄に伴う自身への通知(破棄通知・OnDestroy 等)はラッパーに届かない。
TComponent::~TComponent()
{
    Registry().erase(handle_);
    if (!freedByLcl_ && handle_)
        no_vcl_TComponent_Destroy(handle_);
}

void TComponent::Free()
{
    no_vcl_TComponent_Destroy(handle_);
}

void NO_VCL_CALL TComponent::FreeNotifyTrampoline(no_vcl_obj_t handle, void*)
{
    std::unordered_map<no_vcl_obj_t, TComponent*>& registry = Registry();
    auto it = registry.find(handle);
    if (it == registry.end())
        return;

    TComponent* self = it->second;
    registry.erase(it);
    self->freedByLcl_ = true;
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
    , OnClick(this, &TControl::GetOnClickImpl, &TControl::SetOnClickImpl)
    , OnDblClick(this, &TControl::GetOnDblClickImpl, &TControl::SetOnDblClickImpl)
    , OnResize(this, &TControl::GetOnResizeImpl, &TControl::SetOnResizeImpl)
    , OnMouseDown(this, &TControl::GetOnMouseDownImpl, &TControl::SetOnMouseDownImpl)
    , OnMouseUp(this, &TControl::GetOnMouseUpImpl, &TControl::SetOnMouseUpImpl)
    , OnMouseMove(this, &TControl::GetOnMouseMoveImpl, &TControl::SetOnMouseMoveImpl)
    , OnMouseEnter(this, &TControl::GetOnMouseEnterImpl, &TControl::SetOnMouseEnterImpl)
    , OnMouseLeave(this, &TControl::GetOnMouseLeaveImpl, &TControl::SetOnMouseLeaveImpl)
    , OnMouseWheel(this, &TControl::GetOnMouseWheelImpl, &TControl::SetOnMouseWheelImpl)
    , Text(this, &TControl::GetTextImpl, &TControl::SetTextImpl)
{}

void TControl::Show() { no_vcl_TControl_Show(handle_); }
void TControl::Hide() { no_vcl_TControl_Hide(handle_); }

// イベントの実装はどれも同じ形:
//   Setter  - ハンドラを保持し、最初に空でないハンドラが設定されたときだけ Pascal 側のブリッジを登録する。
//   トランポリン - ハンドルからラッパーを引き、ラッパー自身を Sender として渡す。ハンドラの中で
//             ハンドラ自身を差し替えても実行中の std::function が破棄されないよう、コピーしてから呼ぶ。

TNotifyEvent TControl::GetOnClickImpl(TObject* owner)
{
    return static_cast<TControl*>(owner)->onClick_;
}

void TControl::SetOnClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onClick_ = value;
    if (value && !self->onClickHooked_)
    {
        no_vcl_TControl_SetOnClick(self->handle_, &TControl::ClickTrampoline, nullptr);
        self->onClickHooked_ = true;
    }
}

void NO_VCL_CALL TControl::ClickTrampoline(no_vcl_obj_t sender, void*)
{
    TControl* self = static_cast<TControl*>(FromHandle(sender));
    if (!self || !self->onClick_)
        return;
    TNotifyEvent handler = self->onClick_;
    handler(self);
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

TNotifyEvent TControl::GetOnDblClickImpl(TObject* owner)   { return static_cast<TControl*>(owner)->onDblClick_; }
TNotifyEvent TControl::GetOnResizeImpl(TObject* owner)     { return static_cast<TControl*>(owner)->onResize_; }
TNotifyEvent TControl::GetOnMouseEnterImpl(TObject* owner) { return static_cast<TControl*>(owner)->onMouseEnter_; }
TNotifyEvent TControl::GetOnMouseLeaveImpl(TObject* owner) { return static_cast<TControl*>(owner)->onMouseLeave_; }
TMouseEvent  TControl::GetOnMouseDownImpl(TObject* owner)  { return static_cast<TControl*>(owner)->onMouseDown_; }
TMouseEvent  TControl::GetOnMouseUpImpl(TObject* owner)    { return static_cast<TControl*>(owner)->onMouseUp_; }
TMouseMoveEvent  TControl::GetOnMouseMoveImpl(TObject* owner)  { return static_cast<TControl*>(owner)->onMouseMove_; }
TMouseWheelEvent TControl::GetOnMouseWheelImpl(TObject* owner) { return static_cast<TControl*>(owner)->onMouseWheel_; }

void TControl::SetOnDblClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    SetSimpleEvent(self->handle_, self->onDblClick_, self->onDblClickHooked_, value,
                   &no_vcl_TControl_SetOnDblClick, &TControl::DblClickTrampoline);
}

void TControl::SetOnResizeImpl(TObject* owner, const TNotifyEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    SetSimpleEvent(self->handle_, self->onResize_, self->onResizeHooked_, value,
                   &no_vcl_TControl_SetOnResize, &TControl::ResizeTrampoline);
}

void TControl::SetOnMouseEnterImpl(TObject* owner, const TNotifyEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    SetSimpleEvent(self->handle_, self->onMouseEnter_, self->onMouseEnterHooked_, value,
                   &no_vcl_TControl_SetOnMouseEnter, &TControl::MouseEnterTrampoline);
}

void TControl::SetOnMouseLeaveImpl(TObject* owner, const TNotifyEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    SetSimpleEvent(self->handle_, self->onMouseLeave_, self->onMouseLeaveHooked_, value,
                   &no_vcl_TControl_SetOnMouseLeave, &TControl::MouseLeaveTrampoline);
}

void TControl::SetOnMouseDownImpl(TObject* owner, const TMouseEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseDown_ = value;
    if (value && !self->onMouseDownHooked_)
    {
        no_vcl_TControl_SetOnMouseDown(self->handle_, &TControl::MouseDownTrampoline, nullptr);
        self->onMouseDownHooked_ = true;
    }
}

void TControl::SetOnMouseUpImpl(TObject* owner, const TMouseEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseUp_ = value;
    if (value && !self->onMouseUpHooked_)
    {
        no_vcl_TControl_SetOnMouseUp(self->handle_, &TControl::MouseUpTrampoline, nullptr);
        self->onMouseUpHooked_ = true;
    }
}

void TControl::SetOnMouseMoveImpl(TObject* owner, const TMouseMoveEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseMove_ = value;
    if (value && !self->onMouseMoveHooked_)
    {
        no_vcl_TControl_SetOnMouseMove(self->handle_, &TControl::MouseMoveTrampoline, nullptr);
        self->onMouseMoveHooked_ = true;
    }
}

void TControl::SetOnMouseWheelImpl(TObject* owner, const TMouseWheelEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseWheel_ = value;
    if (value && !self->onMouseWheelHooked_)
    {
        no_vcl_TControl_SetOnMouseWheel(self->handle_, &TControl::MouseWheelTrampoline, nullptr);
        self->onMouseWheelHooked_ = true;
    }
}

void NO_VCL_CALL TControl::DblClickTrampoline(no_vcl_obj_t sender, void*)
{
    if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
        CallNotify(self->onDblClick_, self);
}

void NO_VCL_CALL TControl::ResizeTrampoline(no_vcl_obj_t sender, void*)
{
    if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
        CallNotify(self->onResize_, self);
}

void NO_VCL_CALL TControl::MouseEnterTrampoline(no_vcl_obj_t sender, void*)
{
    if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
        CallNotify(self->onMouseEnter_, self);
}

void NO_VCL_CALL TControl::MouseLeaveTrampoline(no_vcl_obj_t sender, void*)
{
    if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
        CallNotify(self->onMouseLeave_, self);
}

void NO_VCL_CALL TControl::MouseDownTrampoline(no_vcl_obj_t sender, no_vcl_int_t button, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void*)
{
    TControl* self = static_cast<TControl*>(FromHandle(sender));
    if (!self || !self->onMouseDown_)
        return;
    TMouseEvent handler = self->onMouseDown_;
    handler(self, static_cast<TMouseButton>(button), static_cast<TShiftState>(shift), x, y);
}

void NO_VCL_CALL TControl::MouseUpTrampoline(no_vcl_obj_t sender, no_vcl_int_t button, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void*)
{
    TControl* self = static_cast<TControl*>(FromHandle(sender));
    if (!self || !self->onMouseUp_)
        return;
    TMouseEvent handler = self->onMouseUp_;
    handler(self, static_cast<TMouseButton>(button), static_cast<TShiftState>(shift), x, y);
}

void NO_VCL_CALL TControl::MouseMoveTrampoline(no_vcl_obj_t sender, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void*)
{
    TControl* self = static_cast<TControl*>(FromHandle(sender));
    if (!self || !self->onMouseMove_)
        return;
    TMouseMoveEvent handler = self->onMouseMove_;
    handler(self, static_cast<TShiftState>(shift), x, y);
}

void NO_VCL_CALL TControl::MouseWheelTrampoline(no_vcl_obj_t sender, no_vcl_int_t shift, no_vcl_int_t wheelDelta, no_vcl_int_t x, no_vcl_int_t y, no_vcl_bool_t* handled, void*)
{
    TControl* self = static_cast<TControl*>(FromHandle(sender));
    if (!self || !self->onMouseWheel_)
        return;
    TMouseWheelEvent handler = self->onMouseWheel_;
    bool handledValue = *handled != 0;
    handler(self, static_cast<TShiftState>(shift), wheelDelta, x, y, handledValue);
    *handled = handledValue ? 1 : 0;
}

/* ---------------- TWinControl ---------------- */

TWinControl::TWinControl(no_vcl_obj_t handle)
    : TControl(handle)
    , OnKeyDown(this, &TWinControl::GetOnKeyDownImpl, &TWinControl::SetOnKeyDownImpl)
    , OnKeyUp(this, &TWinControl::GetOnKeyUpImpl, &TWinControl::SetOnKeyUpImpl)
    , OnKeyPress(this, &TWinControl::GetOnKeyPressImpl, &TWinControl::SetOnKeyPressImpl)
{}

TKeyEvent TWinControl::GetOnKeyDownImpl(TObject* owner) { return static_cast<TWinControl*>(owner)->onKeyDown_; }
TKeyEvent TWinControl::GetOnKeyUpImpl(TObject* owner)   { return static_cast<TWinControl*>(owner)->onKeyUp_; }
TKeyPressEvent TWinControl::GetOnKeyPressImpl(TObject* owner) { return static_cast<TWinControl*>(owner)->onKeyPress_; }

void TWinControl::SetOnKeyDownImpl(TObject* owner, const TKeyEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onKeyDown_ = value;
    if (value && !self->onKeyDownHooked_)
    {
        no_vcl_TWinControl_SetOnKeyDown(self->handle_, &TWinControl::KeyDownTrampoline, nullptr);
        self->onKeyDownHooked_ = true;
    }
}

void TWinControl::SetOnKeyUpImpl(TObject* owner, const TKeyEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onKeyUp_ = value;
    if (value && !self->onKeyUpHooked_)
    {
        no_vcl_TWinControl_SetOnKeyUp(self->handle_, &TWinControl::KeyUpTrampoline, nullptr);
        self->onKeyUpHooked_ = true;
    }
}

void TWinControl::SetOnKeyPressImpl(TObject* owner, const TKeyPressEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onKeyPress_ = value;
    if (value && !self->onKeyPressHooked_)
    {
        no_vcl_TWinControl_SetOnKeyPress(self->handle_, &TWinControl::KeyPressTrampoline, nullptr);
        self->onKeyPressHooked_ = true;
    }
}

void NO_VCL_CALL TWinControl::KeyDownTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, no_vcl_int_t shift, void*)
{
    TWinControl* self = static_cast<TWinControl*>(FromHandle(sender));
    if (!self || !self->onKeyDown_)
        return;
    TKeyEvent handler = self->onKeyDown_;
    int keyValue = *key;
    handler(self, keyValue, static_cast<TShiftState>(shift));
    *key = keyValue;
}

void NO_VCL_CALL TWinControl::KeyUpTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, no_vcl_int_t shift, void*)
{
    TWinControl* self = static_cast<TWinControl*>(FromHandle(sender));
    if (!self || !self->onKeyUp_)
        return;
    TKeyEvent handler = self->onKeyUp_;
    int keyValue = *key;
    handler(self, keyValue, static_cast<TShiftState>(shift));
    *key = keyValue;
}

void NO_VCL_CALL TWinControl::KeyPressTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, void*)
{
    TWinControl* self = static_cast<TWinControl*>(FromHandle(sender));
    if (!self || !self->onKeyPress_)
        return;
    TKeyPressEvent handler = self->onKeyPress_;
    char keyValue = static_cast<char>(*key);
    handler(self, keyValue);
    *key = static_cast<unsigned char>(keyValue);
}

TScrollBox::TScrollBox(TComponent* AOwner)
    : TScrollingWinControl(no_vcl_TScrollBox_Create(HandleOf(AOwner)))
{}

/* ---------------- Form ---------------- */

void TCustomForm::Show()      { no_vcl_TCustomForm_Show(handle_); }
void TCustomForm::Hide()      { no_vcl_TCustomForm_Hide(handle_); }
int  TCustomForm::ShowModal() { return no_vcl_TCustomForm_ShowModal(handle_); }
void TCustomForm::Close()     { no_vcl_TCustomForm_Close(handle_); }
void TCustomForm::Release()   { no_vcl_TCustomForm_Release(handle_); }

TCustomForm::TCustomForm(no_vcl_obj_t handle)
    : TScrollingWinControl(handle)
    , OnCreate(this, &TCustomForm::GetOnCreateImpl, &TCustomForm::SetOnCreateImpl)
    , OnShow(this, &TCustomForm::GetOnShowImpl, &TCustomForm::SetOnShowImpl)
    , OnHide(this, &TCustomForm::GetOnHideImpl, &TCustomForm::SetOnHideImpl)
    , OnActivate(this, &TCustomForm::GetOnActivateImpl, &TCustomForm::SetOnActivateImpl)
    , OnDeactivate(this, &TCustomForm::GetOnDeactivateImpl, &TCustomForm::SetOnDeactivateImpl)
    , OnCloseQuery(this, &TCustomForm::GetOnCloseQueryImpl, &TCustomForm::SetOnCloseQueryImpl)
    , OnClose(this, &TCustomForm::GetOnCloseImpl, &TCustomForm::SetOnCloseImpl)
    , OnDestroy(this, &TCustomForm::GetOnDestroyImpl, &TCustomForm::SetOnDestroyImpl)
{
    // OnShow のブリッジは常に登録する。new で直接生成したフォームの OnCreate を、最初の表示の直前に呼ぶため。
    no_vcl_TCustomForm_SetOnShow(handle_, &TCustomForm::ShowTrampoline, nullptr);
}

void TCustomForm::DoCreate()
{
    if (created_)
        return;
    created_ = true;
    TNotifyEvent handler = onCreate_;
    if (handler)
        handler(this);
}

void NO_VCL_CALL TCustomForm::ShowTrampoline(no_vcl_obj_t sender, void*)
{
    TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender));
    if (!self)
        return;
    self->DoCreate();
    TNotifyEvent handler = self->onShow_;
    if (handler)
        handler(self);
}

void NO_VCL_CALL TCustomForm::CloseTrampoline(no_vcl_obj_t sender, no_vcl_int_t* action, void*)
{
    TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender));
    if (!self || !self->onClose_)
        return;
    TCloseEvent handler = self->onClose_;
    TCloseAction value = static_cast<TCloseAction>(*action);
    handler(self, value);
    *action = static_cast<no_vcl_int_t>(value);
}

TNotifyEvent TCustomForm::GetOnCreateImpl(TObject* owner) { return static_cast<TCustomForm*>(owner)->onCreate_; }
void TCustomForm::SetOnCreateImpl(TObject* owner, const TNotifyEvent& value) { static_cast<TCustomForm*>(owner)->onCreate_ = value; }
TNotifyEvent TCustomForm::GetOnShowImpl(TObject* owner) { return static_cast<TCustomForm*>(owner)->onShow_; }
void TCustomForm::SetOnShowImpl(TObject* owner, const TNotifyEvent& value) { static_cast<TCustomForm*>(owner)->onShow_ = value; }
TCloseEvent TCustomForm::GetOnCloseImpl(TObject* owner) { return static_cast<TCustomForm*>(owner)->onClose_; }

void TCustomForm::SetOnCloseImpl(TObject* owner, const TCloseEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    self->onClose_ = value;
    if (value && !self->onCloseHooked_)
    {
        no_vcl_TCustomForm_SetOnClose(self->handle_, &TCustomForm::CloseTrampoline, nullptr);
        self->onCloseHooked_ = true;
    }
}

void NO_VCL_CALL TCustomForm::HideTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
        CallNotify(self->onHide_, self);
}

void NO_VCL_CALL TCustomForm::ActivateTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
        CallNotify(self->onActivate_, self);
}

void NO_VCL_CALL TCustomForm::DeactivateTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
        CallNotify(self->onDeactivate_, self);
}

void NO_VCL_CALL TCustomForm::DestroyTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
        CallNotify(self->onDestroy_, self);
}

void NO_VCL_CALL TCustomForm::CloseQueryTrampoline(no_vcl_obj_t sender, no_vcl_bool_t* canClose, void*)
{
    TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender));
    if (!self || !self->onCloseQuery_)
        return;
    TCloseQueryEvent handler = self->onCloseQuery_;
    bool value = *canClose != 0;
    handler(self, value);
    *canClose = value ? 1 : 0;
}

TNotifyEvent TCustomForm::GetOnHideImpl(TObject* owner)       { return static_cast<TCustomForm*>(owner)->onHide_; }
TNotifyEvent TCustomForm::GetOnActivateImpl(TObject* owner)   { return static_cast<TCustomForm*>(owner)->onActivate_; }
TNotifyEvent TCustomForm::GetOnDeactivateImpl(TObject* owner) { return static_cast<TCustomForm*>(owner)->onDeactivate_; }
TNotifyEvent TCustomForm::GetOnDestroyImpl(TObject* owner)    { return static_cast<TCustomForm*>(owner)->onDestroy_; }
TCloseQueryEvent TCustomForm::GetOnCloseQueryImpl(TObject* owner) { return static_cast<TCustomForm*>(owner)->onCloseQuery_; }

void TCustomForm::SetOnHideImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onHide_, self->onHideHooked_, value,
                 &no_vcl_TCustomForm_SetOnHide, &TCustomForm::HideTrampoline);
}

void TCustomForm::SetOnActivateImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onActivate_, self->onActivateHooked_, value,
                 &no_vcl_TCustomForm_SetOnActivate, &TCustomForm::ActivateTrampoline);
}

void TCustomForm::SetOnDeactivateImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onDeactivate_, self->onDeactivateHooked_, value,
                 &no_vcl_TCustomForm_SetOnDeactivate, &TCustomForm::DeactivateTrampoline);
}

void TCustomForm::SetOnDestroyImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onDestroy_, self->onDestroyHooked_, value,
                 &no_vcl_TCustomForm_SetOnDestroy, &TCustomForm::DestroyTrampoline);
}

void TCustomForm::SetOnCloseQueryImpl(TObject* owner, const TCloseQueryEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onCloseQuery_, self->onCloseQueryHooked_, value,
                 &no_vcl_TCustomForm_SetOnCloseQuery, &TCustomForm::CloseQueryTrampoline);
}

no_vcl_obj_t TForm::pendingHandle_ = nullptr;

TForm::TForm(TComponent* AOwner)
    : TCustomForm(CreateHandle(AOwner))
{}

no_vcl_obj_t TForm::CreateHandle(TComponent* AOwner)
{
    if (pendingHandle_ && AOwner && AOwner == Application)
    {
        no_vcl_obj_t handle = pendingHandle_;
        pendingHandle_ = nullptr;
        return handle;
    }
    return no_vcl_TForm_Create(HandleOf(AOwner));
}

/* ---------------- TApplication ---------------- */

TApplication* NewApplication()
{
    return new TApplication(no_vcl_GetApplication());
}

TApplication* Application = NewApplication();

TApplication::TApplication(no_vcl_obj_t handle)
    : TComponent(handle)
    , MainForm(this, &TApplication::GetMainFormImpl)
    , Terminated(this, &TApplication::GetTerminatedImpl)
    , Title(this, &TApplication::GetTitleImpl, &TApplication::SetTitleImpl)
    , ShowMainForm(this, &TApplication::GetShowMainFormImpl, &TApplication::SetShowMainFormImpl)
{
    // 基底の TComponent のコンストラクタでレジストリ(関数内 static)が構築済みのため、
    // ここで登録した終了処理はレジストリの破棄より先に呼ばれる。
    std::atexit(&TApplication::Shutdown);
}

// main から戻った後(C++ の実行環境がまだ有効なうち)に、Application が所有するフォームを破棄する。
// 破棄通知によってラッパーのデストラクタも呼ばれる。その後の DLL の切り離しでは通知は来ない。
// Application 自身のラッパーは解放しない(LCL の Application は DLL の切り離しまで生きている)。
void TApplication::Shutdown()
{
    if (Application)
        no_vcl_TComponent_DestroyComponents(Application->handle_);
    no_vcl_FreeNotify_SetCallback(nullptr, nullptr);
}

void TApplication::BeginCreateForm()
{
    // 前回のハンドルが引き取られていなければ破棄する(T のコンストラクタが Application 以外を Owner にした場合)。
    if (TForm::pendingHandle_)
        no_vcl_TComponent_Destroy(TForm::pendingHandle_);
    TForm::pendingHandle_ = no_vcl_TApplication_CreateForm(handle_);
}

void TApplication::EndCreateForm()
{
    if (TForm::pendingHandle_)
    {
        no_vcl_TComponent_Destroy(TForm::pendingHandle_);
        TForm::pendingHandle_ = nullptr;
    }
}

void TApplication::Run()             { no_vcl_TApplication_Run(handle_); }
void TApplication::ProcessMessages() { no_vcl_TApplication_ProcessMessages(handle_); }
void TApplication::Terminate()       { no_vcl_TApplication_Terminate(handle_); }

TForm* TApplication::GetMainFormImpl(TObject* owner)
{
    return dynamic_cast<TForm*>(FromHandle(no_vcl_TApplication_GetMainForm(owner->Handle())));
}

bool TApplication::GetTerminatedImpl(TObject* owner)
{
    return no_vcl_TApplication_GetTerminated(owner->Handle()) != 0;
}

std::string TApplication::GetTitleImpl(TObject* owner)
{
    return std::string(no_vcl_TApplication_GetTitle(owner->Handle()));
}

void TApplication::SetTitleImpl(TObject* owner, const std::string& value)
{
    no_vcl_TApplication_SetTitle(owner->Handle(), value.c_str());
}

bool TApplication::GetShowMainFormImpl(TObject* owner)
{
    return no_vcl_TApplication_GetShowMainForm(owner->Handle()) != 0;
}

void TApplication::SetShowMainFormImpl(TObject* owner, const bool& value)
{
    no_vcl_TApplication_SetShowMainForm(owner->Handle(), value ? 1 : 0);
}

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

TBevel::TBevel(TComponent* AOwner)
    : TGraphicControl(no_vcl_TBevel_Create(HandleOf(AOwner)))
    , Shape(this, &TBevel::GetShapeImpl, &TBevel::SetShapeImpl)
    , Style(this, &TBevel::GetStyleImpl, &TBevel::SetStyleImpl)
{}

TBevelShape TBevel::GetShapeImpl(TObject* owner) { return static_cast<TBevelShape>(no_vcl_TBevel_GetShape(owner->Handle())); }
void TBevel::SetShapeImpl(TObject* owner, const TBevelShape& value) { no_vcl_TBevel_SetShape(owner->Handle(), value); }
TBevelStyle TBevel::GetStyleImpl(TObject* owner) { return static_cast<TBevelStyle>(no_vcl_TBevel_GetStyle(owner->Handle())); }
void TBevel::SetStyleImpl(TObject* owner, const TBevelStyle& value) { no_vcl_TBevel_SetStyle(owner->Handle(), value); }

TCustomShape::TCustomShape(no_vcl_obj_t handle)
    : TGraphicControl(handle)
    , Pen(no_vcl_TCustomShape_GetPen(handle))
    , Brush(no_vcl_TCustomShape_GetBrush(handle))
    , Shape(this, &TCustomShape::GetShapeImpl, &TCustomShape::SetShapeImpl)
{}

TShapeType TCustomShape::GetShapeImpl(TObject* owner) { return static_cast<TShapeType>(no_vcl_TCustomShape_GetShape(owner->Handle())); }
void TCustomShape::SetShapeImpl(TObject* owner, const TShapeType& value) { no_vcl_TCustomShape_SetShape(owner->Handle(), value); }

TShape::TShape(TComponent* AOwner)
    : TCustomShape(no_vcl_TShape_Create(HandleOf(AOwner)))
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

TToggleBox::TToggleBox(TComponent* AOwner)
    : TCustomCheckBox(no_vcl_TToggleBox_Create(HandleOf(AOwner)))
{}

/* ---------------- Edit / Memo ---------------- */

TCustomEdit::TCustomEdit(no_vcl_obj_t handle)
    : TWinControl(handle)
    , MaxLength(this, &TCustomEdit::GetMaxLengthImpl, &TCustomEdit::SetMaxLengthImpl)
    , ReadOnly(this, &TCustomEdit::GetReadOnlyImpl, &TCustomEdit::SetReadOnlyImpl)
    , OnChange(this, &TCustomEdit::GetOnChangeImpl, &TCustomEdit::SetOnChangeImpl)
{}

TNotifyEvent TCustomEdit::GetOnChangeImpl(TObject* owner)
{
    return static_cast<TCustomEdit*>(owner)->onChange_;
}

void TCustomEdit::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomEdit* self = static_cast<TCustomEdit*>(owner);
    self->onChange_ = value;
    if (value && !self->onChangeHooked_)
    {
        no_vcl_TCustomEdit_SetOnChange(self->handle_, &TCustomEdit::ChangeTrampoline, nullptr);
        self->onChangeHooked_ = true;
    }
}

void NO_VCL_CALL TCustomEdit::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    TCustomEdit* self = static_cast<TCustomEdit*>(FromHandle(sender));
    if (!self || !self->onChange_)
        return;
    TNotifyEvent handler = self->onChange_;
    handler(self);
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
    , OnChange(this, &TComboBox::GetOnChangeImpl, &TComboBox::SetOnChangeImpl)
{}

TNotifyEvent TComboBox::GetOnChangeImpl(TObject* owner)
{
    return static_cast<TComboBox*>(owner)->onChange_;
}

void TComboBox::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TComboBox* self = static_cast<TComboBox*>(owner);
    self->onChange_ = value;
    if (value && !self->onChangeHooked_)
    {
        no_vcl_TComboBox_SetOnChange(self->handle_, &TComboBox::ChangeTrampoline, nullptr);
        self->onChangeHooked_ = true;
    }
}

void NO_VCL_CALL TComboBox::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    TComboBox* self = static_cast<TComboBox*>(FromHandle(sender));
    if (!self || !self->onChange_)
        return;
    TNotifyEvent handler = self->onChange_;
    handler(self);
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

TCustomStaticText::TCustomStaticText(no_vcl_obj_t handle)
    : TWinControl(handle)
    , BorderStyle(this, &TCustomStaticText::GetBorderStyleImpl, &TCustomStaticText::SetBorderStyleImpl)
{}

TStaticBorderStyle TCustomStaticText::GetBorderStyleImpl(TObject* owner)
{
    return static_cast<TStaticBorderStyle>(no_vcl_TCustomStaticText_GetBorderStyle(owner->Handle()));
}

void TCustomStaticText::SetBorderStyleImpl(TObject* owner, const TStaticBorderStyle& value)
{
    no_vcl_TCustomStaticText_SetBorderStyle(owner->Handle(), value);
}

TStaticText::TStaticText(TComponent* AOwner)
    : TCustomStaticText(no_vcl_TStaticText_Create(HandleOf(AOwner)))
{}

TStatusBar::TStatusBar(TComponent* AOwner)
    : TWinControl(no_vcl_TStatusBar_Create(HandleOf(AOwner)))
    , SimpleText(this, &TStatusBar::GetSimpleTextImpl, &TStatusBar::SetSimpleTextImpl)
    , SimplePanel(this, &TStatusBar::GetSimplePanelImpl, &TStatusBar::SetSimplePanelImpl)
{}

std::string TStatusBar::GetSimpleTextImpl(TObject* owner)
{
    return std::string(no_vcl_TStatusBar_GetSimpleText(owner->Handle()));
}

void TStatusBar::SetSimpleTextImpl(TObject* owner, const std::string& value)
{
    no_vcl_TStatusBar_SetSimpleText(owner->Handle(), value.c_str());
}

bool TStatusBar::GetSimplePanelImpl(TObject* owner) { return no_vcl_TStatusBar_GetSimplePanel(owner->Handle()) != 0; }
void TStatusBar::SetSimplePanelImpl(TObject* owner, const bool& value) { no_vcl_TStatusBar_SetSimplePanel(owner->Handle(), value ? 1 : 0); }

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
    , OnPaint(this, &TPaintBox::GetOnPaintImpl, &TPaintBox::SetOnPaintImpl)
{}

TNotifyEvent TPaintBox::GetOnPaintImpl(TObject* owner)
{
    return static_cast<TPaintBox*>(owner)->onPaint_;
}

void TPaintBox::SetOnPaintImpl(TObject* owner, const TNotifyEvent& value)
{
    TPaintBox* self = static_cast<TPaintBox*>(owner);
    self->onPaint_ = value;
    if (value && !self->onPaintHooked_)
    {
        no_vcl_TPaintBox_SetOnPaint(self->handle_, &TPaintBox::PaintTrampoline, nullptr);
        self->onPaintHooked_ = true;
    }
}

void NO_VCL_CALL TPaintBox::PaintTrampoline(no_vcl_obj_t sender, void*)
{
    TPaintBox* self = static_cast<TPaintBox*>(FromHandle(sender));
    if (!self || !self->onPaint_)
        return;
    TNotifyEvent handler = self->onPaint_;
    handler(self);
}

/* ---------------- Timer ---------------- */

TCustomTimer::TCustomTimer(no_vcl_obj_t handle)
    : TComponent(handle)
    , Interval(this, &TCustomTimer::GetIntervalImpl, &TCustomTimer::SetIntervalImpl)
    , Enabled(this, &TCustomTimer::GetEnabledImpl, &TCustomTimer::SetEnabledImpl)
    , OnTimer(this, &TCustomTimer::GetOnTimerImpl, &TCustomTimer::SetOnTimerImpl)
{}

TNotifyEvent TCustomTimer::GetOnTimerImpl(TObject* owner)
{
    return static_cast<TCustomTimer*>(owner)->onTimer_;
}

void TCustomTimer::SetOnTimerImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomTimer* self = static_cast<TCustomTimer*>(owner);
    self->onTimer_ = value;
    if (value && !self->onTimerHooked_)
    {
        no_vcl_TCustomTimer_SetOnTimer(self->handle_, &TCustomTimer::TimerTrampoline, nullptr);
        self->onTimerHooked_ = true;
    }
}

void NO_VCL_CALL TCustomTimer::TimerTrampoline(no_vcl_obj_t sender, void*)
{
    TCustomTimer* self = static_cast<TCustomTimer*>(FromHandle(sender));
    if (!self || !self->onTimer_)
        return;
    TNotifyEvent handler = self->onTimer_;
    handler(self);
}

int  TCustomTimer::GetIntervalImpl(TObject* owner)                   { return no_vcl_TCustomTimer_GetInterval(owner->Handle()); }
void TCustomTimer::SetIntervalImpl(TObject* owner, const int& value) { no_vcl_TCustomTimer_SetInterval(owner->Handle(), value); }
bool TCustomTimer::GetEnabledImpl(TObject* owner)                    { return no_vcl_TCustomTimer_GetEnabled(owner->Handle()) != 0; }
void TCustomTimer::SetEnabledImpl(TObject* owner, const bool& value) { no_vcl_TCustomTimer_SetEnabled(owner->Handle(), value ? 1 : 0); }

TTimer::TTimer(TComponent* AOwner)
    : TCustomTimer(no_vcl_TTimer_Create(HandleOf(AOwner)))
{}

} // namespace no_vcl
