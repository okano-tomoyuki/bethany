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

/* ---------------- TStrings ---------------- */

TStrings::TStrings(TObject* owner, Accessor accessor)
    : TPersistent(nullptr)
    , Count(this, &TStrings::GetCountImpl)
    , Strings(this, &TStrings::GetStringsImpl, &TStrings::SetStringsImpl)
    , Objects(this, &TStrings::GetObjectsImpl, &TStrings::SetObjectsImpl)
    , Text(this, &TStrings::GetTextImpl, &TStrings::SetTextImpl)
    , CommaText(this, &TStrings::GetCommaTextImpl, &TStrings::SetCommaTextImpl)
    , owner_(owner)
    , accessor_(accessor)
{}

int  TStrings::Add(const std::string& S)                     { return no_vcl_TStrings_Add(Current(), S.c_str()); }
int  TStrings::AddObject(const std::string& S, void* AObject) { return no_vcl_TStrings_AddObject(Current(), S.c_str(), AObject); }
void TStrings::Insert(int Index, const std::string& S)       { no_vcl_TStrings_Insert(Current(), Index, S.c_str()); }
void TStrings::Delete(int Index)                             { no_vcl_TStrings_Delete(Current(), Index); }
void TStrings::Clear()                                       { no_vcl_TStrings_Clear(Current()); }
int  TStrings::IndexOf(const std::string& S) const           { return no_vcl_TStrings_IndexOf(Current(), S.c_str()); }
void TStrings::Exchange(int Index1, int Index2)              { no_vcl_TStrings_Exchange(Current(), Index1, Index2); }
void TStrings::Move(int CurIndex, int NewIndex)              { no_vcl_TStrings_Move(Current(), CurIndex, NewIndex); }
void TStrings::BeginUpdate()                                 { no_vcl_TStrings_BeginUpdate(Current()); }
void TStrings::EndUpdate()                                   { no_vcl_TStrings_EndUpdate(Current()); }
void TStrings::Assign(const TStrings* Source)                { no_vcl_TStrings_Assign(Current(), Source ? Source->Current() : nullptr); }
void TStrings::AddStrings(const TStrings* Source)            { if (Source) no_vcl_TStrings_AddStrings(Current(), Source->Current()); }

int TStrings::GetCountImpl(TObject* owner) { return no_vcl_TStrings_GetCount(static_cast<TStrings*>(owner)->Current()); }
std::string TStrings::GetStringsImpl(TObject* owner, int Index)
{
    return std::string(no_vcl_TStrings_GetStrings(static_cast<TStrings*>(owner)->Current(), Index));
}
void TStrings::SetStringsImpl(TObject* owner, int Index, const std::string& value)
{
    no_vcl_TStrings_SetStrings(static_cast<TStrings*>(owner)->Current(), Index, value.c_str());
}
void* TStrings::GetObjectsImpl(TObject* owner, int Index) { return no_vcl_TStrings_GetObjects(static_cast<TStrings*>(owner)->Current(), Index); }
void TStrings::SetObjectsImpl(TObject* owner, int Index, void* const& value)
{
    no_vcl_TStrings_SetObjects(static_cast<TStrings*>(owner)->Current(), Index, value);
}
std::string TStrings::GetTextImpl(TObject* owner) { return std::string(no_vcl_TStrings_GetText(static_cast<TStrings*>(owner)->Current())); }
void TStrings::SetTextImpl(TObject* owner, const std::string& value) { no_vcl_TStrings_SetText(static_cast<TStrings*>(owner)->Current(), value.c_str()); }
std::string TStrings::GetCommaTextImpl(TObject* owner) { return std::string(no_vcl_TStrings_GetCommaText(static_cast<TStrings*>(owner)->Current())); }
void TStrings::SetCommaTextImpl(TObject* owner, const std::string& value) { no_vcl_TStrings_SetCommaText(static_cast<TStrings*>(owner)->Current(), value.c_str()); }

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
    , Align(this, &TControl::GetAlignImpl, &TControl::SetAlignImpl)
    , PopupMenu(this, &TControl::GetPopupMenuImpl, &TControl::SetPopupMenuImpl)
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

TAlign TControl::GetAlignImpl(TObject* owner)                  { return static_cast<TAlign>(no_vcl_TControl_GetAlign(owner->Handle())); }
void   TControl::SetAlignImpl(TObject* owner, const TAlign& value) { no_vcl_TControl_SetAlign(owner->Handle(), value); }

TPopupMenu* TControl::GetPopupMenuImpl(TObject* owner)
{
    return static_cast<TPopupMenu*>(FromHandle(no_vcl_TControl_GetPopupMenu(owner->Handle())));
}

void TControl::SetPopupMenuImpl(TObject* owner, TPopupMenu* const& value)
{
    no_vcl_TControl_SetPopupMenu(owner->Handle(), HandleOf(value));
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

TCustomScrollBar::TCustomScrollBar(no_vcl_obj_t handle)
    : TWinControl(handle)
    , Kind(this, &TCustomScrollBar::GetKindImpl, &TCustomScrollBar::SetKindImpl)
    , Min(this, &TCustomScrollBar::GetMinImpl, &TCustomScrollBar::SetMinImpl)
    , Max(this, &TCustomScrollBar::GetMaxImpl, &TCustomScrollBar::SetMaxImpl)
    , Position(this, &TCustomScrollBar::GetPositionImpl, &TCustomScrollBar::SetPositionImpl)
    , PageSize(this, &TCustomScrollBar::GetPageSizeImpl, &TCustomScrollBar::SetPageSizeImpl)
    , OnChange(this, &TCustomScrollBar::GetOnChangeImpl, &TCustomScrollBar::SetOnChangeImpl)
{}

TScrollBarKind TCustomScrollBar::GetKindImpl(TObject* owner) { return static_cast<TScrollBarKind>(no_vcl_TCustomScrollBar_GetKind(owner->Handle())); }
void TCustomScrollBar::SetKindImpl(TObject* owner, const TScrollBarKind& value) { no_vcl_TCustomScrollBar_SetKind(owner->Handle(), value); }
int  TCustomScrollBar::GetMinImpl(TObject* owner)      { return no_vcl_TCustomScrollBar_GetMin(owner->Handle()); }
void TCustomScrollBar::SetMinImpl(TObject* owner, const int& value)      { no_vcl_TCustomScrollBar_SetMin(owner->Handle(), value); }
int  TCustomScrollBar::GetMaxImpl(TObject* owner)      { return no_vcl_TCustomScrollBar_GetMax(owner->Handle()); }
void TCustomScrollBar::SetMaxImpl(TObject* owner, const int& value)      { no_vcl_TCustomScrollBar_SetMax(owner->Handle(), value); }
int  TCustomScrollBar::GetPositionImpl(TObject* owner) { return no_vcl_TCustomScrollBar_GetPosition(owner->Handle()); }
void TCustomScrollBar::SetPositionImpl(TObject* owner, const int& value) { no_vcl_TCustomScrollBar_SetPosition(owner->Handle(), value); }
int  TCustomScrollBar::GetPageSizeImpl(TObject* owner) { return no_vcl_TCustomScrollBar_GetPageSize(owner->Handle()); }
void TCustomScrollBar::SetPageSizeImpl(TObject* owner, const int& value) { no_vcl_TCustomScrollBar_SetPageSize(owner->Handle(), value); }
TNotifyEvent TCustomScrollBar::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomScrollBar*>(owner)->onChange_; }

void TCustomScrollBar::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomScrollBar* self = static_cast<TCustomScrollBar*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TCustomScrollBar_SetOnChange, &TCustomScrollBar::ChangeTrampoline);
}

void NO_VCL_CALL TCustomScrollBar::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomScrollBar* self = static_cast<TCustomScrollBar*>(FromHandle(sender)))
        CallNotify(self->onChange_, self);
}

TScrollBar::TScrollBar(TComponent* AOwner)
    : TCustomScrollBar(no_vcl_TScrollBar_Create(HandleOf(AOwner)))
{}

TCustomTrackBar::TCustomTrackBar(no_vcl_obj_t handle)
    : TWinControl(handle)
    , Min(this, &TCustomTrackBar::GetMinImpl, &TCustomTrackBar::SetMinImpl)
    , Max(this, &TCustomTrackBar::GetMaxImpl, &TCustomTrackBar::SetMaxImpl)
    , Position(this, &TCustomTrackBar::GetPositionImpl, &TCustomTrackBar::SetPositionImpl)
    , OnChange(this, &TCustomTrackBar::GetOnChangeImpl, &TCustomTrackBar::SetOnChangeImpl)
{}

int  TCustomTrackBar::GetMinImpl(TObject* owner)      { return no_vcl_TCustomTrackBar_GetMin(owner->Handle()); }
void TCustomTrackBar::SetMinImpl(TObject* owner, const int& value)      { no_vcl_TCustomTrackBar_SetMin(owner->Handle(), value); }
int  TCustomTrackBar::GetMaxImpl(TObject* owner)      { return no_vcl_TCustomTrackBar_GetMax(owner->Handle()); }
void TCustomTrackBar::SetMaxImpl(TObject* owner, const int& value)      { no_vcl_TCustomTrackBar_SetMax(owner->Handle(), value); }
int  TCustomTrackBar::GetPositionImpl(TObject* owner) { return no_vcl_TCustomTrackBar_GetPosition(owner->Handle()); }
void TCustomTrackBar::SetPositionImpl(TObject* owner, const int& value) { no_vcl_TCustomTrackBar_SetPosition(owner->Handle(), value); }
TNotifyEvent TCustomTrackBar::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomTrackBar*>(owner)->onChange_; }

void TCustomTrackBar::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomTrackBar* self = static_cast<TCustomTrackBar*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TCustomTrackBar_SetOnChange, &TCustomTrackBar::ChangeTrampoline);
}

void NO_VCL_CALL TCustomTrackBar::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomTrackBar* self = static_cast<TCustomTrackBar*>(FromHandle(sender)))
        CallNotify(self->onChange_, self);
}

TTrackBar::TTrackBar(TComponent* AOwner)
    : TCustomTrackBar(no_vcl_TTrackBar_Create(HandleOf(AOwner)))
{}

TCustomProgressBar::TCustomProgressBar(no_vcl_obj_t handle)
    : TWinControl(handle)
    , Min(this, &TCustomProgressBar::GetMinImpl, &TCustomProgressBar::SetMinImpl)
    , Max(this, &TCustomProgressBar::GetMaxImpl, &TCustomProgressBar::SetMaxImpl)
    , Position(this, &TCustomProgressBar::GetPositionImpl, &TCustomProgressBar::SetPositionImpl)
{}

int  TCustomProgressBar::GetMinImpl(TObject* owner)      { return no_vcl_TCustomProgressBar_GetMin(owner->Handle()); }
void TCustomProgressBar::SetMinImpl(TObject* owner, const int& value)      { no_vcl_TCustomProgressBar_SetMin(owner->Handle(), value); }
int  TCustomProgressBar::GetMaxImpl(TObject* owner)      { return no_vcl_TCustomProgressBar_GetMax(owner->Handle()); }
void TCustomProgressBar::SetMaxImpl(TObject* owner, const int& value)      { no_vcl_TCustomProgressBar_SetMax(owner->Handle(), value); }
int  TCustomProgressBar::GetPositionImpl(TObject* owner) { return no_vcl_TCustomProgressBar_GetPosition(owner->Handle()); }
void TCustomProgressBar::SetPositionImpl(TObject* owner, const int& value) { no_vcl_TCustomProgressBar_SetPosition(owner->Handle(), value); }

TProgressBar::TProgressBar(TComponent* AOwner)
    : TCustomProgressBar(no_vcl_TProgressBar_Create(HandleOf(AOwner)))
{}

TScrollBox::TScrollBox(TComponent* AOwner)
    : TScrollingWinControl(no_vcl_TScrollBox_Create(HandleOf(AOwner)))
{}

TUpDown::TUpDown(TComponent* AOwner)
    : TCustomControl(no_vcl_TUpDown_Create(HandleOf(AOwner)))
    , Min(this, &TUpDown::GetMinImpl, &TUpDown::SetMinImpl)
    , Max(this, &TUpDown::GetMaxImpl, &TUpDown::SetMaxImpl)
    , Position(this, &TUpDown::GetPositionImpl, &TUpDown::SetPositionImpl)
    , Increment(this, &TUpDown::GetIncrementImpl, &TUpDown::SetIncrementImpl)
    , Associate(this, &TUpDown::GetAssociateImpl, &TUpDown::SetAssociateImpl)
{}

int  TUpDown::GetMinImpl(TObject* owner)       { return no_vcl_TUpDown_GetMin(owner->Handle()); }
void TUpDown::SetMinImpl(TObject* owner, const int& value)       { no_vcl_TUpDown_SetMin(owner->Handle(), value); }
int  TUpDown::GetMaxImpl(TObject* owner)       { return no_vcl_TUpDown_GetMax(owner->Handle()); }
void TUpDown::SetMaxImpl(TObject* owner, const int& value)       { no_vcl_TUpDown_SetMax(owner->Handle(), value); }
int  TUpDown::GetPositionImpl(TObject* owner)  { return no_vcl_TUpDown_GetPosition(owner->Handle()); }
void TUpDown::SetPositionImpl(TObject* owner, const int& value)  { no_vcl_TUpDown_SetPosition(owner->Handle(), value); }
int  TUpDown::GetIncrementImpl(TObject* owner) { return no_vcl_TUpDown_GetIncrement(owner->Handle()); }
void TUpDown::SetIncrementImpl(TObject* owner, const int& value) { no_vcl_TUpDown_SetIncrement(owner->Handle(), value); }

TWinControl* TUpDown::GetAssociateImpl(TObject* owner)
{
    return static_cast<TWinControl*>(FromHandle(no_vcl_TUpDown_GetAssociate(owner->Handle())));
}

void TUpDown::SetAssociateImpl(TObject* owner, TWinControl* const& value)
{
    no_vcl_TUpDown_SetAssociate(owner->Handle(), HandleOf(value));
}

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
    , Menu(this, &TCustomForm::GetMenuImpl, &TCustomForm::SetMenuImpl)
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

TMainMenu* TCustomForm::GetMenuImpl(TObject* owner)
{
    return static_cast<TMainMenu*>(FromHandle(no_vcl_TCustomForm_GetMenu(owner->Handle())));
}

void TCustomForm::SetMenuImpl(TObject* owner, TMainMenu* const& value)
{
    no_vcl_TCustomForm_SetMenu(owner->Handle(), HandleOf(value));
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
    no_vcl_ItemFree_SetCallback(nullptr, nullptr);
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

TCustomRadioGroup::TCustomRadioGroup(no_vcl_obj_t handle)
    : TCustomGroupBox(handle)
    , ItemIndex(this, &TCustomRadioGroup::GetItemIndexImpl, &TCustomRadioGroup::SetItemIndexImpl)
    , OnClick(this, &TCustomRadioGroup::GetOnClickImpl, &TCustomRadioGroup::SetOnClickImpl)
    , Items(this, &TCustomRadioGroup::GetItemsImpl)
    , items_(this, &no_vcl_TCustomRadioGroup_GetItems)
{}

TStrings* TCustomRadioGroup::GetItemsImpl(TObject* owner) { return &static_cast<TCustomRadioGroup*>(owner)->items_; }

int  TCustomRadioGroup::GetItemIndexImpl(TObject* owner)                   { return no_vcl_TCustomRadioGroup_GetItemIndex(owner->Handle()); }
void TCustomRadioGroup::SetItemIndexImpl(TObject* owner, const int& value) { no_vcl_TCustomRadioGroup_SetItemIndex(owner->Handle(), value); }
TNotifyEvent TCustomRadioGroup::GetOnClickImpl(TObject* owner) { return static_cast<TCustomRadioGroup*>(owner)->onClick_; }

void TCustomRadioGroup::SetOnClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomRadioGroup* self = static_cast<TCustomRadioGroup*>(owner);
    SetSimpleEvent(self->handle_, self->onClick_, self->onClickHooked_, value,
                   &no_vcl_TCustomRadioGroup_SetOnClick, &TCustomRadioGroup::ClickTrampoline);
}

void NO_VCL_CALL TCustomRadioGroup::ClickTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomRadioGroup* self = static_cast<TCustomRadioGroup*>(FromHandle(sender)))
        CallNotify(self->onClick_, self);
}

TRadioGroup::TRadioGroup(TComponent* AOwner)
    : TCustomRadioGroup(no_vcl_TRadioGroup_Create(HandleOf(AOwner)))
{}

TStrings* TCustomCheckGroup::GetItemsImpl(TObject* owner) { return &static_cast<TCustomCheckGroup*>(owner)->items_; }

TCustomCheckGroup::TCustomCheckGroup(no_vcl_obj_t handle)
    : TCustomGroupBox(handle)
    , Checked(this, &TCustomCheckGroup::GetCheckedImpl, &TCustomCheckGroup::SetCheckedImpl)
    , Items(this, &TCustomCheckGroup::GetItemsImpl)
    , items_(this, &no_vcl_TCustomCheckGroup_GetItems)
{}

bool TCustomCheckGroup::GetCheckedImpl(TObject* owner, int index)                     { return no_vcl_TCustomCheckGroup_GetChecked(owner->Handle(), index) != 0; }
void TCustomCheckGroup::SetCheckedImpl(TObject* owner, int index, const bool& value)  { no_vcl_TCustomCheckGroup_SetChecked(owner->Handle(), index, value ? 1 : 0); }

TCheckGroup::TCheckGroup(TComponent* AOwner)
    : TCustomCheckGroup(no_vcl_TCheckGroup_Create(HandleOf(AOwner)))
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

TCustomSpeedButton::TCustomSpeedButton(no_vcl_obj_t handle)
    : TGraphicControl(handle)
    , Down(this, &TCustomSpeedButton::GetDownImpl, &TCustomSpeedButton::SetDownImpl)
    , GroupIndex(this, &TCustomSpeedButton::GetGroupIndexImpl, &TCustomSpeedButton::SetGroupIndexImpl)
    , Flat(this, &TCustomSpeedButton::GetFlatImpl, &TCustomSpeedButton::SetFlatImpl)
    , AllowAllUp(this, &TCustomSpeedButton::GetAllowAllUpImpl, &TCustomSpeedButton::SetAllowAllUpImpl)
{}

bool TCustomSpeedButton::GetDownImpl(TObject* owner)       { return no_vcl_TCustomSpeedButton_GetDown(owner->Handle()) != 0; }
void TCustomSpeedButton::SetDownImpl(TObject* owner, const bool& value)       { no_vcl_TCustomSpeedButton_SetDown(owner->Handle(), value ? 1 : 0); }
int  TCustomSpeedButton::GetGroupIndexImpl(TObject* owner) { return no_vcl_TCustomSpeedButton_GetGroupIndex(owner->Handle()); }
void TCustomSpeedButton::SetGroupIndexImpl(TObject* owner, const int& value) { no_vcl_TCustomSpeedButton_SetGroupIndex(owner->Handle(), value); }
bool TCustomSpeedButton::GetFlatImpl(TObject* owner)       { return no_vcl_TCustomSpeedButton_GetFlat(owner->Handle()) != 0; }
void TCustomSpeedButton::SetFlatImpl(TObject* owner, const bool& value)       { no_vcl_TCustomSpeedButton_SetFlat(owner->Handle(), value ? 1 : 0); }
bool TCustomSpeedButton::GetAllowAllUpImpl(TObject* owner) { return no_vcl_TCustomSpeedButton_GetAllowAllUp(owner->Handle()) != 0; }
void TCustomSpeedButton::SetAllowAllUpImpl(TObject* owner, const bool& value) { no_vcl_TCustomSpeedButton_SetAllowAllUp(owner->Handle(), value ? 1 : 0); }

TSpeedButton::TSpeedButton(TComponent* AOwner)
    : TCustomSpeedButton(no_vcl_TSpeedButton_Create(HandleOf(AOwner)))
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

TCustomBitBtn::TCustomBitBtn(no_vcl_obj_t handle)
    : TCustomButton(handle)
    , Kind(this, &TCustomBitBtn::GetKindImpl, &TCustomBitBtn::SetKindImpl)
{}

TBitBtnKind TCustomBitBtn::GetKindImpl(TObject* owner) { return static_cast<TBitBtnKind>(no_vcl_TCustomBitBtn_GetKind(owner->Handle())); }
void TCustomBitBtn::SetKindImpl(TObject* owner, const TBitBtnKind& value) { no_vcl_TCustomBitBtn_SetKind(owner->Handle(), value); }

TBitBtn::TBitBtn(TComponent* AOwner)
    : TCustomBitBtn(no_vcl_TBitBtn_Create(HandleOf(AOwner)))
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

TCustomFloatSpinEdit::TCustomFloatSpinEdit(no_vcl_obj_t handle)
    : TCustomEdit(handle)
    , Value(this, &TCustomFloatSpinEdit::GetValueImpl, &TCustomFloatSpinEdit::SetValueImpl)
    , MinValue(this, &TCustomFloatSpinEdit::GetMinValueImpl, &TCustomFloatSpinEdit::SetMinValueImpl)
    , MaxValue(this, &TCustomFloatSpinEdit::GetMaxValueImpl, &TCustomFloatSpinEdit::SetMaxValueImpl)
    , Increment(this, &TCustomFloatSpinEdit::GetIncrementImpl, &TCustomFloatSpinEdit::SetIncrementImpl)
    , DecimalPlaces(this, &TCustomFloatSpinEdit::GetDecimalPlacesImpl, &TCustomFloatSpinEdit::SetDecimalPlacesImpl)
{}

double TCustomFloatSpinEdit::GetValueImpl(TObject* owner)     { return no_vcl_TCustomFloatSpinEdit_GetValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetValueImpl(TObject* owner, const double& value)     { no_vcl_TCustomFloatSpinEdit_SetValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetMinValueImpl(TObject* owner)  { return no_vcl_TCustomFloatSpinEdit_GetMinValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetMinValueImpl(TObject* owner, const double& value)  { no_vcl_TCustomFloatSpinEdit_SetMinValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetMaxValueImpl(TObject* owner)  { return no_vcl_TCustomFloatSpinEdit_GetMaxValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetMaxValueImpl(TObject* owner, const double& value)  { no_vcl_TCustomFloatSpinEdit_SetMaxValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetIncrementImpl(TObject* owner) { return no_vcl_TCustomFloatSpinEdit_GetIncrement(owner->Handle()); }
void   TCustomFloatSpinEdit::SetIncrementImpl(TObject* owner, const double& value) { no_vcl_TCustomFloatSpinEdit_SetIncrement(owner->Handle(), value); }
int    TCustomFloatSpinEdit::GetDecimalPlacesImpl(TObject* owner) { return no_vcl_TCustomFloatSpinEdit_GetDecimalPlaces(owner->Handle()); }
void   TCustomFloatSpinEdit::SetDecimalPlacesImpl(TObject* owner, const int& value) { no_vcl_TCustomFloatSpinEdit_SetDecimalPlaces(owner->Handle(), value); }

TFloatSpinEdit::TFloatSpinEdit(TComponent* AOwner)
    : TCustomFloatSpinEdit(no_vcl_TFloatSpinEdit_Create(HandleOf(AOwner)))
{}

TCustomSpinEdit::TCustomSpinEdit(no_vcl_obj_t handle)
    : TCustomFloatSpinEdit(handle)
    , Value(this, &TCustomSpinEdit::GetValueImpl, &TCustomSpinEdit::SetValueImpl)
    , MinValue(this, &TCustomSpinEdit::GetMinValueImpl, &TCustomSpinEdit::SetMinValueImpl)
    , MaxValue(this, &TCustomSpinEdit::GetMaxValueImpl, &TCustomSpinEdit::SetMaxValueImpl)
    , Increment(this, &TCustomSpinEdit::GetIncrementImpl, &TCustomSpinEdit::SetIncrementImpl)
{}

int  TCustomSpinEdit::GetValueImpl(TObject* owner)     { return no_vcl_TCustomSpinEdit_GetValue(owner->Handle()); }
void TCustomSpinEdit::SetValueImpl(TObject* owner, const int& value)     { no_vcl_TCustomSpinEdit_SetValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetMinValueImpl(TObject* owner)  { return no_vcl_TCustomSpinEdit_GetMinValue(owner->Handle()); }
void TCustomSpinEdit::SetMinValueImpl(TObject* owner, const int& value)  { no_vcl_TCustomSpinEdit_SetMinValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetMaxValueImpl(TObject* owner)  { return no_vcl_TCustomSpinEdit_GetMaxValue(owner->Handle()); }
void TCustomSpinEdit::SetMaxValueImpl(TObject* owner, const int& value)  { no_vcl_TCustomSpinEdit_SetMaxValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetIncrementImpl(TObject* owner) { return no_vcl_TCustomSpinEdit_GetIncrement(owner->Handle()); }
void TCustomSpinEdit::SetIncrementImpl(TObject* owner, const int& value) { no_vcl_TCustomSpinEdit_SetIncrement(owner->Handle(), value); }

TSpinEdit::TSpinEdit(TComponent* AOwner)
    : TCustomSpinEdit(no_vcl_TSpinEdit_Create(HandleOf(AOwner)))
{}

TMaskEdit::TMaskEdit(TComponent* AOwner)
    : TCustomEdit(no_vcl_TMaskEdit_Create(HandleOf(AOwner)))
    , EditMask(this, &TMaskEdit::GetEditMaskImpl, &TMaskEdit::SetEditMaskImpl)
{}

std::string TMaskEdit::GetEditMaskImpl(TObject* owner) { return std::string(no_vcl_TMaskEdit_GetEditMask(owner->Handle())); }
void TMaskEdit::SetEditMaskImpl(TObject* owner, const std::string& value) { no_vcl_TMaskEdit_SetEditMask(owner->Handle(), value.c_str()); }

TTabControl::TTabControl(TComponent* AOwner)
    : TCustomTabControl(no_vcl_TTabControl_Create(HandleOf(AOwner)))
    , TabIndex(this, &TTabControl::GetTabIndexImpl, &TTabControl::SetTabIndexImpl)
    , OnChange(this, &TTabControl::GetOnChangeImpl, &TTabControl::SetOnChangeImpl)
    , Tabs(this, &TTabControl::GetTabsImpl)
    , tabs_(this, &no_vcl_TTabControl_GetTabs)
{}

TStrings* TTabControl::GetTabsImpl(TObject* owner) { return &static_cast<TTabControl*>(owner)->tabs_; }

int  TTabControl::GetTabIndexImpl(TObject* owner)                   { return no_vcl_TTabControl_GetTabIndex(owner->Handle()); }
void TTabControl::SetTabIndexImpl(TObject* owner, const int& value) { no_vcl_TTabControl_SetTabIndex(owner->Handle(), value); }
TNotifyEvent TTabControl::GetOnChangeImpl(TObject* owner) { return static_cast<TTabControl*>(owner)->onChange_; }

void TTabControl::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TTabControl* self = static_cast<TTabControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TTabControl_SetOnChange, &TTabControl::ChangeTrampoline);
}

void NO_VCL_CALL TTabControl::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    if (TTabControl* self = static_cast<TTabControl*>(FromHandle(sender)))
        CallNotify(self->onChange_, self);
}

TCustomTabControl::TCustomTabControl(no_vcl_obj_t handle)
    : TWinControl(handle)
    , PageCount(this, &TCustomTabControl::GetPageCountImpl)
    , MultiLine(this, &TCustomTabControl::GetMultiLineImpl, &TCustomTabControl::SetMultiLineImpl)
    , ShowTabs(this, &TCustomTabControl::GetShowTabsImpl, &TCustomTabControl::SetShowTabsImpl)
    , TabPosition(this, &TCustomTabControl::GetTabPositionImpl, &TCustomTabControl::SetTabPositionImpl)
    , OnChanging(this, &TCustomTabControl::GetOnChangingImpl, &TCustomTabControl::SetOnChangingImpl)
{}

int  TCustomTabControl::GetPageCountImpl(TObject* owner)                 { return no_vcl_TCustomTabControl_GetPageCount(owner->Handle()); }
bool TCustomTabControl::GetMultiLineImpl(TObject* owner)                 { return no_vcl_TCustomTabControl_GetMultiLine(owner->Handle()) != 0; }
void TCustomTabControl::SetMultiLineImpl(TObject* owner, const bool& value) { no_vcl_TCustomTabControl_SetMultiLine(owner->Handle(), value ? 1 : 0); }
bool TCustomTabControl::GetShowTabsImpl(TObject* owner)                  { return no_vcl_TCustomTabControl_GetShowTabs(owner->Handle()) != 0; }
void TCustomTabControl::SetShowTabsImpl(TObject* owner, const bool& value)  { no_vcl_TCustomTabControl_SetShowTabs(owner->Handle(), value ? 1 : 0); }
TTabPosition TCustomTabControl::GetTabPositionImpl(TObject* owner) { return static_cast<TTabPosition>(no_vcl_TCustomTabControl_GetTabPosition(owner->Handle())); }
void TCustomTabControl::SetTabPositionImpl(TObject* owner, const TTabPosition& value) { no_vcl_TCustomTabControl_SetTabPosition(owner->Handle(), value); }

TTabChangingEvent TCustomTabControl::GetOnChangingImpl(TObject* owner) { return static_cast<TCustomTabControl*>(owner)->onChanging_; }

void TCustomTabControl::SetOnChangingImpl(TObject* owner, const TTabChangingEvent& value)
{
    TCustomTabControl* self = static_cast<TCustomTabControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChanging_, self->onChangingHooked_, value,
                   &no_vcl_TCustomTabControl_SetOnChanging, &TCustomTabControl::ChangingTrampoline);
}

void NO_VCL_CALL TCustomTabControl::ChangingTrampoline(no_vcl_obj_t sender, no_vcl_bool_t* allowChange, void*)
{
    TCustomTabControl* self = static_cast<TCustomTabControl*>(FromHandle(sender));
    if (!self || !self->onChanging_)
        return;
    TTabChangingEvent handler = self->onChanging_;
    bool value = *allowChange != 0;
    handler(self, value);
    *allowChange = value ? 1 : 0;
}

TPageControl::TPageControl(TComponent* AOwner)
    : TCustomTabControl(no_vcl_TPageControl_Create(HandleOf(AOwner)))
    , ActivePage(this, &TPageControl::GetActivePageImpl, &TPageControl::SetActivePageImpl)
    , ActivePageIndex(this, &TPageControl::GetActivePageIndexImpl, &TPageControl::SetActivePageIndexImpl)
    , TabIndex(this, &TPageControl::GetTabIndexImpl, &TPageControl::SetTabIndexImpl)
    , OnChange(this, &TPageControl::GetOnChangeImpl, &TPageControl::SetOnChangeImpl)
    , Pages(this, &TPageControl::GetPagesImpl)
{}

TTabSheet* TPageControl::GetPagesImpl(TObject* owner, int Index) { return WrapExisting<TTabSheet>(no_vcl_TPageControl_GetPage(owner->Handle(), Index)); }
TTabSheet* TPageControl::AddTabSheet()            { return WrapExisting<TTabSheet>(no_vcl_TPageControl_AddTabSheet(handle_)); }
void TPageControl::Clear()                        { no_vcl_TPageControl_Clear(handle_); }
void TPageControl::SelectNextPage(bool GoForward) { no_vcl_TPageControl_SelectNextPage(handle_, GoForward ? 1 : 0); }

TTabSheet* TPageControl::GetActivePageImpl(TObject* owner) { return WrapExisting<TTabSheet>(no_vcl_TPageControl_GetActivePage(owner->Handle())); }
void TPageControl::SetActivePageImpl(TObject* owner, TTabSheet* const& value) { no_vcl_TPageControl_SetActivePage(owner->Handle(), HandleOf(value)); }
int  TPageControl::GetActivePageIndexImpl(TObject* owner)                  { return no_vcl_TPageControl_GetActivePageIndex(owner->Handle()); }
void TPageControl::SetActivePageIndexImpl(TObject* owner, const int& value) { no_vcl_TPageControl_SetActivePageIndex(owner->Handle(), value); }
int  TPageControl::GetTabIndexImpl(TObject* owner)                         { return no_vcl_TPageControl_GetTabIndex(owner->Handle()); }
void TPageControl::SetTabIndexImpl(TObject* owner, const int& value)        { no_vcl_TPageControl_SetTabIndex(owner->Handle(), value); }

TNotifyEvent TPageControl::GetOnChangeImpl(TObject* owner) { return static_cast<TPageControl*>(owner)->onChange_; }

void TPageControl::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TPageControl* self = static_cast<TPageControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TPageControl_SetOnChange, &TPageControl::ChangeTrampoline);
}

void NO_VCL_CALL TPageControl::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    if (TPageControl* self = static_cast<TPageControl*>(FromHandle(sender)))
        CallNotify(self->onChange_, self);
}

TCustomPage::TCustomPage(no_vcl_obj_t handle)
    : TWinControl(handle)
    , PageIndex(this, &TCustomPage::GetPageIndexImpl, &TCustomPage::SetPageIndexImpl)
    , TabVisible(this, &TCustomPage::GetTabVisibleImpl, &TCustomPage::SetTabVisibleImpl)
    , OnShow(this, &TCustomPage::GetOnShowImpl, &TCustomPage::SetOnShowImpl)
    , OnHide(this, &TCustomPage::GetOnHideImpl, &TCustomPage::SetOnHideImpl)
{}

int  TCustomPage::GetPageIndexImpl(TObject* owner)                   { return no_vcl_TCustomPage_GetPageIndex(owner->Handle()); }
void TCustomPage::SetPageIndexImpl(TObject* owner, const int& value)  { no_vcl_TCustomPage_SetPageIndex(owner->Handle(), value); }
bool TCustomPage::GetTabVisibleImpl(TObject* owner)                  { return no_vcl_TCustomPage_GetTabVisible(owner->Handle()) != 0; }
void TCustomPage::SetTabVisibleImpl(TObject* owner, const bool& value) { no_vcl_TCustomPage_SetTabVisible(owner->Handle(), value ? 1 : 0); }

TNotifyEvent TCustomPage::GetOnShowImpl(TObject* owner) { return static_cast<TCustomPage*>(owner)->onShow_; }
TNotifyEvent TCustomPage::GetOnHideImpl(TObject* owner) { return static_cast<TCustomPage*>(owner)->onHide_; }

void TCustomPage::SetOnShowImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomPage* self = static_cast<TCustomPage*>(owner);
    SetSimpleEvent(self->handle_, self->onShow_, self->onShowHooked_, value,
                   &no_vcl_TCustomPage_SetOnShow, &TCustomPage::ShowTrampoline);
}

void TCustomPage::SetOnHideImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomPage* self = static_cast<TCustomPage*>(owner);
    SetSimpleEvent(self->handle_, self->onHide_, self->onHideHooked_, value,
                   &no_vcl_TCustomPage_SetOnHide, &TCustomPage::HideTrampoline);
}

void NO_VCL_CALL TCustomPage::ShowTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomPage* self = static_cast<TCustomPage*>(FromHandle(sender)))
        CallNotify(self->onShow_, self);
}

void NO_VCL_CALL TCustomPage::HideTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomPage* self = static_cast<TCustomPage*>(FromHandle(sender)))
        CallNotify(self->onHide_, self);
}

TTabSheet::TTabSheet(TComponent* AOwner)
    : TTabSheet(no_vcl_TTabSheet_Create(HandleOf(AOwner)))
{}

TTabSheet::TTabSheet(no_vcl_obj_t handle)
    : TCustomPage(handle)
    , PageControl(this, &TTabSheet::GetPageControlImpl, &TTabSheet::SetPageControlImpl)
    , TabIndex(this, &TTabSheet::GetTabIndexImpl)
{}

TPageControl* TTabSheet::GetPageControlImpl(TObject* owner)
{
    return static_cast<TPageControl*>(FromHandle(no_vcl_TTabSheet_GetPageControl(owner->Handle())));
}

void TTabSheet::SetPageControlImpl(TObject* owner, TPageControl* const& value)
{
    no_vcl_TTabSheet_SetPageControl(owner->Handle(), HandleOf(value));
}

int TTabSheet::GetTabIndexImpl(TObject* owner) { return no_vcl_TTabSheet_GetTabIndex(owner->Handle()); }

/* ---------------- ItemRegistry ---------------- */

// 項目の破棄は Pascal 側で、ハンドルを渡すときに付けた観察者(TPersistent.Destroy の ooFree)から通知される。
// 意図的に破棄しない(new したまま)。関数内 static の値にすると、初回の呼び出し(フォームの生成中)より前に
// 登録された TApplication::Shutdown(atexit)よりも先に破棄されてしまい、Shutdown でフォームを破棄する際の
// 項目の削除通知(FreeTrampoline)が、破棄済みのレジストリに触れることになる(未定義動作。実際に終了が数秒遅れた。ADR 0019)。
std::unordered_map<no_vcl_obj_t, TPersistent*>& ItemRegistry::Registry()
{
    static std::unordered_map<no_vcl_obj_t, TPersistent*>* registry = new std::unordered_map<no_vcl_obj_t, TPersistent*>();
    return *registry;
}

void ItemRegistry::InstallCallback()
{
    static bool installed = false;
    if (!installed)
    {
        no_vcl_ItemFree_SetCallback(&ItemRegistry::FreeTrampoline, nullptr);
        installed = true;
    }
}

void NO_VCL_CALL ItemRegistry::FreeTrampoline(no_vcl_obj_t handle, void*)
{
    std::unordered_map<no_vcl_obj_t, TPersistent*>& registry = Registry();
    auto it = registry.find(handle);
    if (it == registry.end())
        return;
    TPersistent* item = it->second;
    registry.erase(it);
    delete item;
}

/* ---------------- TreeView ---------------- */

TTreeNode::TTreeNode(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Text(this, &TTreeNode::GetTextImpl, &TTreeNode::SetTextImpl)
    , Expanded(this, &TTreeNode::GetExpandedImpl, &TTreeNode::SetExpandedImpl)
    , Selected(this, &TTreeNode::GetSelectedImpl, &TTreeNode::SetSelectedImpl)
    , HasChildren(this, &TTreeNode::GetHasChildrenImpl, &TTreeNode::SetHasChildrenImpl)
    , Data(this, &TTreeNode::GetDataImpl, &TTreeNode::SetDataImpl)
    , Count(this, &TTreeNode::GetCountImpl)
    , Index(this, &TTreeNode::GetIndexImpl)
    , Level(this, &TTreeNode::GetLevelImpl)
    , AbsoluteIndex(this, &TTreeNode::GetAbsoluteIndexImpl)
    , Parent(this, &TTreeNode::GetParentImpl)
    , TreeView(this, &TTreeNode::GetTreeViewImpl)
    , Items(this, &TTreeNode::GetItemsImpl)
{}

TTreeNode* TTreeNode::GetItemsImpl(TObject* owner, int Index) { return Wrap(no_vcl_TTreeNode_GetItem(owner->Handle(), Index)); }
TTreeNode* TTreeNode::GetFirstChild() const     { return Wrap(no_vcl_TTreeNode_GetFirstChild(handle_)); }
TTreeNode* TTreeNode::GetLastChild() const      { return Wrap(no_vcl_TTreeNode_GetLastChild(handle_)); }
TTreeNode* TTreeNode::GetNextSibling() const    { return Wrap(no_vcl_TTreeNode_GetNextSibling(handle_)); }
TTreeNode* TTreeNode::GetPrevSibling() const    { return Wrap(no_vcl_TTreeNode_GetPrevSibling(handle_)); }
TTreeNode* TTreeNode::GetNext() const           { return Wrap(no_vcl_TTreeNode_GetNext(handle_)); }
TTreeNode* TTreeNode::GetPrev() const           { return Wrap(no_vcl_TTreeNode_GetPrev(handle_)); }
int  TTreeNode::IndexOf(TTreeNode* Node) const  { return no_vcl_TTreeNode_IndexOf(handle_, Node ? Node->Handle() : nullptr); }
void TTreeNode::Expand(bool Recurse)            { no_vcl_TTreeNode_Expand(handle_, Recurse ? 1 : 0); }
void TTreeNode::Collapse(bool Recurse)          { no_vcl_TTreeNode_Collapse(handle_, Recurse ? 1 : 0); }
void TTreeNode::Delete()                        { no_vcl_TTreeNode_Delete(handle_); }
void TTreeNode::DeleteChildren()                { no_vcl_TTreeNode_DeleteChildren(handle_); }
void TTreeNode::MakeVisible()                   { no_vcl_TTreeNode_MakeVisible(handle_); }

void TTreeNode::MoveTo(TTreeNode* Destination, TNodeAttachMode Mode)
{
    no_vcl_TTreeNode_MoveTo(handle_, Destination ? Destination->Handle() : nullptr, Mode);
}

std::string TTreeNode::GetTextImpl(TObject* owner) { return std::string(no_vcl_TTreeNode_GetText(owner->Handle())); }
void TTreeNode::SetTextImpl(TObject* owner, const std::string& value) { no_vcl_TTreeNode_SetText(owner->Handle(), value.c_str()); }
bool TTreeNode::GetExpandedImpl(TObject* owner)                      { return no_vcl_TTreeNode_GetExpanded(owner->Handle()) != 0; }
void TTreeNode::SetExpandedImpl(TObject* owner, const bool& value)    { no_vcl_TTreeNode_SetExpanded(owner->Handle(), value ? 1 : 0); }
bool TTreeNode::GetSelectedImpl(TObject* owner)                      { return no_vcl_TTreeNode_GetSelected(owner->Handle()) != 0; }
void TTreeNode::SetSelectedImpl(TObject* owner, const bool& value)    { no_vcl_TTreeNode_SetSelected(owner->Handle(), value ? 1 : 0); }
bool TTreeNode::GetHasChildrenImpl(TObject* owner)                   { return no_vcl_TTreeNode_GetHasChildren(owner->Handle()) != 0; }
void TTreeNode::SetHasChildrenImpl(TObject* owner, const bool& value) { no_vcl_TTreeNode_SetHasChildren(owner->Handle(), value ? 1 : 0); }
void* TTreeNode::GetDataImpl(TObject* owner)                         { return no_vcl_TTreeNode_GetData(owner->Handle()); }
void TTreeNode::SetDataImpl(TObject* owner, void* const& value)       { no_vcl_TTreeNode_SetData(owner->Handle(), value); }
int  TTreeNode::GetCountImpl(TObject* owner)                         { return no_vcl_TTreeNode_GetCount(owner->Handle()); }
int  TTreeNode::GetIndexImpl(TObject* owner)                         { return no_vcl_TTreeNode_GetIndex(owner->Handle()); }
int  TTreeNode::GetLevelImpl(TObject* owner)                         { return no_vcl_TTreeNode_GetLevel(owner->Handle()); }
int  TTreeNode::GetAbsoluteIndexImpl(TObject* owner)                 { return no_vcl_TTreeNode_GetAbsoluteIndex(owner->Handle()); }
TTreeNode* TTreeNode::GetParentImpl(TObject* owner)                  { return Wrap(no_vcl_TTreeNode_GetParent(owner->Handle())); }

TCustomTreeView* TTreeNode::GetTreeViewImpl(TObject* owner)
{
    // ツリービューは TComponent で、C++ ラッパーを介して作られていればレジストリにある。
    return static_cast<TCustomTreeView*>(TCustomTreeView::FromHandle(no_vcl_TTreeNode_GetTreeView(owner->Handle())));
}

TTreeNodes::TTreeNodes(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &TTreeNodes::GetCountImpl)
    , Item(this, &TTreeNodes::GetItemImpl)
{}

namespace
{
no_vcl_obj_t NodeHandle(const TTreeNode* node) { return node ? node->Handle() : nullptr; }
}

TTreeNode* TTreeNodes::Add(TTreeNode* Sibling, const std::string& S)
{
    return TTreeNode::Wrap(no_vcl_TTreeNodes_Add(handle_, NodeHandle(Sibling), S.c_str()));
}

TTreeNode* TTreeNodes::AddFirst(TTreeNode* Sibling, const std::string& S)
{
    return TTreeNode::Wrap(no_vcl_TTreeNodes_AddFirst(handle_, NodeHandle(Sibling), S.c_str()));
}

TTreeNode* TTreeNodes::AddChild(TTreeNode* Parent, const std::string& S)
{
    return TTreeNode::Wrap(no_vcl_TTreeNodes_AddChild(handle_, NodeHandle(Parent), S.c_str()));
}

TTreeNode* TTreeNodes::AddChildFirst(TTreeNode* Parent, const std::string& S)
{
    return TTreeNode::Wrap(no_vcl_TTreeNodes_AddChildFirst(handle_, NodeHandle(Parent), S.c_str()));
}

TTreeNode* TTreeNodes::Insert(TTreeNode* NextNode, const std::string& S)
{
    return TTreeNode::Wrap(no_vcl_TTreeNodes_Insert(handle_, NodeHandle(NextNode), S.c_str()));
}

void TTreeNodes::Clear()                   { no_vcl_TTreeNodes_Clear(handle_); }
void TTreeNodes::Delete(TTreeNode* Node)   { no_vcl_TTreeNodes_Delete(handle_, NodeHandle(Node)); }
TTreeNode* TTreeNodes::GetItemImpl(TObject* owner, int Index) { return TTreeNode::Wrap(no_vcl_TTreeNodes_GetItem(owner->Handle(), Index)); }
TTreeNode* TTreeNodes::GetFirstNode() const     { return TTreeNode::Wrap(no_vcl_TTreeNodes_GetFirstNode(handle_)); }

TTreeNode* TTreeNodes::FindNodeWithText(const std::string& S) const
{
    return TTreeNode::Wrap(no_vcl_TTreeNodes_FindNodeWithText(handle_, S.c_str()));
}

void TTreeNodes::BeginUpdate()             { no_vcl_TTreeNodes_BeginUpdate(handle_); }
void TTreeNodes::EndUpdate()               { no_vcl_TTreeNodes_EndUpdate(handle_); }
int  TTreeNodes::GetCountImpl(TObject* owner) { return no_vcl_TTreeNodes_GetCount(owner->Handle()); }

TCustomTreeView::TCustomTreeView(no_vcl_obj_t handle)
    : TCustomControl(handle)
    , Items(this, &TCustomTreeView::GetItemsImpl)
    , Selected(this, &TCustomTreeView::GetSelectedImpl, &TCustomTreeView::SetSelectedImpl)
    , items_(no_vcl_TCustomTreeView_GetItems(handle))
{}

void TCustomTreeView::FullExpand()   { no_vcl_TCustomTreeView_FullExpand(handle_); }
void TCustomTreeView::FullCollapse() { no_vcl_TCustomTreeView_FullCollapse(handle_); }
bool TCustomTreeView::AlphaSort()    { return no_vcl_TCustomTreeView_AlphaSort(handle_) != 0; }

TTreeNode* TCustomTreeView::GetNodeAt(int X, int Y) const
{
    return TTreeNode::Wrap(no_vcl_TCustomTreeView_GetNodeAt(handle_, X, Y));
}

TTreeNodes* TCustomTreeView::GetItemsImpl(TObject* owner) { return &static_cast<TCustomTreeView*>(owner)->items_; }
TTreeNode*  TCustomTreeView::GetSelectedImpl(TObject* owner) { return TTreeNode::Wrap(no_vcl_TCustomTreeView_GetSelected(owner->Handle())); }

void TCustomTreeView::SetSelectedImpl(TObject* owner, TTreeNode* const& value)
{
    no_vcl_TCustomTreeView_SetSelected(owner->Handle(), NodeHandle(value));
}

TTreeView::TTreeView(TComponent* AOwner)
    : TCustomTreeView(no_vcl_TTreeView_Create(HandleOf(AOwner)))
    , ReadOnly(this, &TTreeView::GetReadOnlyImpl, &TTreeView::SetReadOnlyImpl)
    , ShowLines(this, &TTreeView::GetShowLinesImpl, &TTreeView::SetShowLinesImpl)
    , ShowRoot(this, &TTreeView::GetShowRootImpl, &TTreeView::SetShowRootImpl)
    , ShowButtons(this, &TTreeView::GetShowButtonsImpl, &TTreeView::SetShowButtonsImpl)
    , AutoExpand(this, &TTreeView::GetAutoExpandImpl, &TTreeView::SetAutoExpandImpl)
    , HideSelection(this, &TTreeView::GetHideSelectionImpl, &TTreeView::SetHideSelectionImpl)
    , RowSelect(this, &TTreeView::GetRowSelectImpl, &TTreeView::SetRowSelectImpl)
    , OnChange(this, &TTreeView::GetOnChangeImpl, &TTreeView::SetOnChangeImpl)
    , OnChanging(this, &TTreeView::GetOnChangingImpl, &TTreeView::SetOnChangingImpl)
    , OnExpanding(this, &TTreeView::GetOnExpandingImpl, &TTreeView::SetOnExpandingImpl)
    , OnExpanded(this, &TTreeView::GetOnExpandedImpl, &TTreeView::SetOnExpandedImpl)
    , OnCollapsing(this, &TTreeView::GetOnCollapsingImpl, &TTreeView::SetOnCollapsingImpl)
    , OnCollapsed(this, &TTreeView::GetOnCollapsedImpl, &TTreeView::SetOnCollapsedImpl)
    , OnDeletion(this, &TTreeView::GetOnDeletionImpl, &TTreeView::SetOnDeletionImpl)
{}

bool TTreeView::GetReadOnlyImpl(TObject* owner)                     { return no_vcl_TTreeView_GetReadOnly(owner->Handle()) != 0; }
void TTreeView::SetReadOnlyImpl(TObject* owner, const bool& value)   { no_vcl_TTreeView_SetReadOnly(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowLinesImpl(TObject* owner)                    { return no_vcl_TTreeView_GetShowLines(owner->Handle()) != 0; }
void TTreeView::SetShowLinesImpl(TObject* owner, const bool& value)  { no_vcl_TTreeView_SetShowLines(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowRootImpl(TObject* owner)                     { return no_vcl_TTreeView_GetShowRoot(owner->Handle()) != 0; }
void TTreeView::SetShowRootImpl(TObject* owner, const bool& value)   { no_vcl_TTreeView_SetShowRoot(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowButtonsImpl(TObject* owner)                  { return no_vcl_TTreeView_GetShowButtons(owner->Handle()) != 0; }
void TTreeView::SetShowButtonsImpl(TObject* owner, const bool& value) { no_vcl_TTreeView_SetShowButtons(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetAutoExpandImpl(TObject* owner)                   { return no_vcl_TTreeView_GetAutoExpand(owner->Handle()) != 0; }
void TTreeView::SetAutoExpandImpl(TObject* owner, const bool& value) { no_vcl_TTreeView_SetAutoExpand(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetHideSelectionImpl(TObject* owner)                { return no_vcl_TTreeView_GetHideSelection(owner->Handle()) != 0; }
void TTreeView::SetHideSelectionImpl(TObject* owner, const bool& value) { no_vcl_TTreeView_SetHideSelection(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetRowSelectImpl(TObject* owner)                    { return no_vcl_TTreeView_GetRowSelect(owner->Handle()) != 0; }
void TTreeView::SetRowSelectImpl(TObject* owner, const bool& value)  { no_vcl_TTreeView_SetRowSelect(owner->Handle(), value ? 1 : 0); }

// ノードを対象とするイベントのトランポリンは、スロット(メンバへのポインタ)だけが異なるため共通化する。
template<typename Event>
void TTreeView::DispatchNode(no_vcl_obj_t sender, no_vcl_obj_t node, Event TTreeView::*slot)
{
    TTreeView* self = static_cast<TTreeView*>(FromHandle(sender));
    if (!self || !(self->*slot))
        return;
    Event handler = self->*slot;
    handler(self, TTreeNode::Wrap(node));
}

template<typename Event>
void TTreeView::DispatchNodeAllow(no_vcl_obj_t sender, no_vcl_obj_t node, no_vcl_bool_t* allow, Event TTreeView::*slot)
{
    TTreeView* self = static_cast<TTreeView*>(FromHandle(sender));
    if (!self || !(self->*slot))
        return;
    Event handler = self->*slot;
    bool value = *allow != 0;
    handler(self, TTreeNode::Wrap(node), value);
    *allow = value ? 1 : 0;
}

void NO_VCL_CALL TTreeView::ChangeTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, void*)    { DispatchNode(s, n, &TTreeView::onChange_); }
void NO_VCL_CALL TTreeView::ExpandedTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, void*)  { DispatchNode(s, n, &TTreeView::onExpanded_); }
void NO_VCL_CALL TTreeView::CollapsedTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, void*) { DispatchNode(s, n, &TTreeView::onCollapsed_); }
void NO_VCL_CALL TTreeView::DeletionTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, void*)  { DispatchNode(s, n, &TTreeView::onDeletion_); }

void NO_VCL_CALL TTreeView::ChangingTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, no_vcl_bool_t* a, void*)
{
    DispatchNodeAllow(s, n, a, &TTreeView::onChanging_);
}

void NO_VCL_CALL TTreeView::ExpandingTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, no_vcl_bool_t* a, void*)
{
    DispatchNodeAllow(s, n, a, &TTreeView::onExpanding_);
}

void NO_VCL_CALL TTreeView::CollapsingTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, no_vcl_bool_t* a, void*)
{
    DispatchNodeAllow(s, n, a, &TTreeView::onCollapsing_);
}

TTVChangedEvent    TTreeView::GetOnChangeImpl(TObject* owner)     { return static_cast<TTreeView*>(owner)->onChange_; }
TTVChangingEvent   TTreeView::GetOnChangingImpl(TObject* owner)   { return static_cast<TTreeView*>(owner)->onChanging_; }
TTVExpandingEvent  TTreeView::GetOnExpandingImpl(TObject* owner)  { return static_cast<TTreeView*>(owner)->onExpanding_; }
TTVExpandedEvent   TTreeView::GetOnExpandedImpl(TObject* owner)   { return static_cast<TTreeView*>(owner)->onExpanded_; }
TTVCollapsingEvent TTreeView::GetOnCollapsingImpl(TObject* owner) { return static_cast<TTreeView*>(owner)->onCollapsing_; }
TTVExpandedEvent   TTreeView::GetOnCollapsedImpl(TObject* owner)  { return static_cast<TTreeView*>(owner)->onCollapsed_; }
TTVExpandedEvent   TTreeView::GetOnDeletionImpl(TObject* owner)   { return static_cast<TTreeView*>(owner)->onDeletion_; }

void TTreeView::SetOnChangeImpl(TObject* owner, const TTVChangedEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TTreeView_SetOnChange, &TTreeView::ChangeTrampoline);
}

void TTreeView::SetOnChangingImpl(TObject* owner, const TTVChangingEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onChanging_, self->onChangingHooked_, value,
                   &no_vcl_TTreeView_SetOnChanging, &TTreeView::ChangingTrampoline);
}

void TTreeView::SetOnExpandingImpl(TObject* owner, const TTVExpandingEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onExpanding_, self->onExpandingHooked_, value,
                   &no_vcl_TTreeView_SetOnExpanding, &TTreeView::ExpandingTrampoline);
}

void TTreeView::SetOnExpandedImpl(TObject* owner, const TTVExpandedEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onExpanded_, self->onExpandedHooked_, value,
                   &no_vcl_TTreeView_SetOnExpanded, &TTreeView::ExpandedTrampoline);
}

void TTreeView::SetOnCollapsingImpl(TObject* owner, const TTVCollapsingEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onCollapsing_, self->onCollapsingHooked_, value,
                   &no_vcl_TTreeView_SetOnCollapsing, &TTreeView::CollapsingTrampoline);
}

void TTreeView::SetOnCollapsedImpl(TObject* owner, const TTVExpandedEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onCollapsed_, self->onCollapsedHooked_, value,
                   &no_vcl_TTreeView_SetOnCollapsed, &TTreeView::CollapsedTrampoline);
}

void TTreeView::SetOnDeletionImpl(TObject* owner, const TTVExpandedEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onDeletion_, self->onDeletionHooked_, value,
                   &no_vcl_TTreeView_SetOnDeletion, &TTreeView::DeletionTrampoline);
}

/* ---------------- ListView ---------------- */

namespace
{
no_vcl_obj_t ItemHandle(const TObject* item) { return item ? item->Handle() : nullptr; }
}

TListItem::TListItem(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Caption(this, &TListItem::GetCaptionImpl, &TListItem::SetCaptionImpl)
    , Checked(this, &TListItem::GetCheckedImpl, &TListItem::SetCheckedImpl)
    , Selected(this, &TListItem::GetSelectedImpl, &TListItem::SetSelectedImpl)
    , Focused(this, &TListItem::GetFocusedImpl, &TListItem::SetFocusedImpl)
    , Data(this, &TListItem::GetDataImpl, &TListItem::SetDataImpl)
    , Index(this, &TListItem::GetIndexImpl)
    , ListView(this, &TListItem::GetListViewImpl)
    , SubItems(this, &TListItem::GetSubItemsImpl)
    , subItems_(this, &no_vcl_TListItem_GetSubItems)
{}

TStrings* TListItem::GetSubItemsImpl(TObject* owner) { return &static_cast<TListItem*>(owner)->subItems_; }
void TListItem::Delete()                               { no_vcl_TListItem_Delete(handle_); }
void TListItem::MakeVisible(bool PartialOK)            { no_vcl_TListItem_MakeVisible(handle_, PartialOK ? 1 : 0); }

std::string TListItem::GetCaptionImpl(TObject* owner) { return std::string(no_vcl_TListItem_GetCaption(owner->Handle())); }
void TListItem::SetCaptionImpl(TObject* owner, const std::string& value) { no_vcl_TListItem_SetCaption(owner->Handle(), value.c_str()); }
bool TListItem::GetCheckedImpl(TObject* owner)                     { return no_vcl_TListItem_GetChecked(owner->Handle()) != 0; }
void TListItem::SetCheckedImpl(TObject* owner, const bool& value)   { no_vcl_TListItem_SetChecked(owner->Handle(), value ? 1 : 0); }
bool TListItem::GetSelectedImpl(TObject* owner)                    { return no_vcl_TListItem_GetSelected(owner->Handle()) != 0; }
void TListItem::SetSelectedImpl(TObject* owner, const bool& value)  { no_vcl_TListItem_SetSelected(owner->Handle(), value ? 1 : 0); }
bool TListItem::GetFocusedImpl(TObject* owner)                     { return no_vcl_TListItem_GetFocused(owner->Handle()) != 0; }
void TListItem::SetFocusedImpl(TObject* owner, const bool& value)   { no_vcl_TListItem_SetFocused(owner->Handle(), value ? 1 : 0); }
void* TListItem::GetDataImpl(TObject* owner)                       { return no_vcl_TListItem_GetData(owner->Handle()); }
void TListItem::SetDataImpl(TObject* owner, void* const& value)     { no_vcl_TListItem_SetData(owner->Handle(), value); }
int  TListItem::GetIndexImpl(TObject* owner)                       { return no_vcl_TListItem_GetIndex(owner->Handle()); }

TCustomListView* TListItem::GetListViewImpl(TObject* owner)
{
    return static_cast<TCustomListView*>(TCustomListView::FromHandle(no_vcl_TListItem_GetListView(owner->Handle())));
}

TListItems::TListItems(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &TListItems::GetCountImpl)
    , Item(this, &TListItems::GetItemImpl)
{}

TListItem* TListItems::Add()                     { return TListItem::Wrap(no_vcl_TListItems_Add(handle_)); }
TListItem* TListItems::Insert(int Index)         { return TListItem::Wrap(no_vcl_TListItems_Insert(handle_, Index)); }
void TListItems::Delete(int Index)               { no_vcl_TListItems_Delete(handle_, Index); }
void TListItems::Clear()                         { no_vcl_TListItems_Clear(handle_); }
TListItem* TListItems::GetItemImpl(TObject* owner, int Index) { return TListItem::Wrap(no_vcl_TListItems_GetItem(owner->Handle(), Index)); }
int  TListItems::IndexOf(TListItem* Item) const  { return no_vcl_TListItems_IndexOf(handle_, ItemHandle(Item)); }

TListItem* TListItems::FindCaption(int StartIndex, const std::string& Value, bool Partial, bool Inclusive, bool Wrap) const
{
    return TListItem::Wrap(no_vcl_TListItems_FindCaption(handle_, StartIndex, Value.c_str(),
                                                        Partial ? 1 : 0, Inclusive ? 1 : 0, Wrap ? 1 : 0));
}

void TListItems::Exchange(int Index1, int Index2) { no_vcl_TListItems_Exchange(handle_, Index1, Index2); }
void TListItems::Move(int FromIndex, int ToIndex) { no_vcl_TListItems_Move(handle_, FromIndex, ToIndex); }
void TListItems::BeginUpdate()                    { no_vcl_TListItems_BeginUpdate(handle_); }
void TListItems::EndUpdate()                      { no_vcl_TListItems_EndUpdate(handle_); }
int  TListItems::GetCountImpl(TObject* owner)     { return no_vcl_TListItems_GetCount(owner->Handle()); }

TListColumn::TListColumn(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Caption(this, &TListColumn::GetCaptionImpl, &TListColumn::SetCaptionImpl)
    , Width(this, &TListColumn::GetWidthImpl, &TListColumn::SetWidthImpl)
    , Alignment(this, &TListColumn::GetAlignmentImpl, &TListColumn::SetAlignmentImpl)
    , AutoSize(this, &TListColumn::GetAutoSizeImpl, &TListColumn::SetAutoSizeImpl)
    , Visible(this, &TListColumn::GetVisibleImpl, &TListColumn::SetVisibleImpl)
    , Index(this, &TListColumn::GetIndexImpl, &TListColumn::SetIndexImpl)
{}

std::string TListColumn::GetCaptionImpl(TObject* owner) { return std::string(no_vcl_TListColumn_GetCaption(owner->Handle())); }
void TListColumn::SetCaptionImpl(TObject* owner, const std::string& value) { no_vcl_TListColumn_SetCaption(owner->Handle(), value.c_str()); }
int  TListColumn::GetWidthImpl(TObject* owner)                    { return no_vcl_TListColumn_GetWidth(owner->Handle()); }
void TListColumn::SetWidthImpl(TObject* owner, const int& value)   { no_vcl_TListColumn_SetWidth(owner->Handle(), value); }
TAlignment TListColumn::GetAlignmentImpl(TObject* owner) { return static_cast<TAlignment>(no_vcl_TListColumn_GetAlignment(owner->Handle())); }
void TListColumn::SetAlignmentImpl(TObject* owner, const TAlignment& value) { no_vcl_TListColumn_SetAlignment(owner->Handle(), value); }
bool TListColumn::GetAutoSizeImpl(TObject* owner)                 { return no_vcl_TListColumn_GetAutoSize(owner->Handle()) != 0; }
void TListColumn::SetAutoSizeImpl(TObject* owner, const bool& value) { no_vcl_TListColumn_SetAutoSize(owner->Handle(), value ? 1 : 0); }
bool TListColumn::GetVisibleImpl(TObject* owner)                  { return no_vcl_TListColumn_GetVisible(owner->Handle()) != 0; }
void TListColumn::SetVisibleImpl(TObject* owner, const bool& value) { no_vcl_TListColumn_SetVisible(owner->Handle(), value ? 1 : 0); }
int  TListColumn::GetIndexImpl(TObject* owner)                    { return no_vcl_TListColumn_GetIndex(owner->Handle()); }
void TListColumn::SetIndexImpl(TObject* owner, const int& value)   { no_vcl_TListColumn_SetIndex(owner->Handle(), value); }

TListColumns::TListColumns(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &TListColumns::GetCountImpl)
    , Items(this, &TListColumns::GetItemsImpl)
{}

TListColumn* TListColumns::Add()                    { return TListColumn::Wrap(no_vcl_TListColumns_Add(handle_)); }
TListColumn* TListColumns::GetItemsImpl(TObject* owner, int Index) { return TListColumn::Wrap(no_vcl_TListColumns_GetItem(owner->Handle(), Index)); }
void TListColumns::Delete(int Index)                { no_vcl_TListColumns_Delete(handle_, Index); }
void TListColumns::Clear()                          { no_vcl_TListColumns_Clear(handle_); }
int  TListColumns::GetCountImpl(TObject* owner)     { return no_vcl_TListColumns_GetCount(owner->Handle()); }

TCustomListView::TCustomListView(no_vcl_obj_t handle)
    : TWinControl(handle)
    , Items(this, &TCustomListView::GetItemsImpl)
    , Selected(this, &TCustomListView::GetSelectedImpl, &TCustomListView::SetSelectedImpl)
    , ItemIndex(this, &TCustomListView::GetItemIndexImpl, &TCustomListView::SetItemIndexImpl)
    , SelCount(this, &TCustomListView::GetSelCountImpl)
    , Checkboxes(this, &TCustomListView::GetCheckboxesImpl, &TCustomListView::SetCheckboxesImpl)
    , GridLines(this, &TCustomListView::GetGridLinesImpl, &TCustomListView::SetGridLinesImpl)
    , MultiSelect(this, &TCustomListView::GetMultiSelectImpl, &TCustomListView::SetMultiSelectImpl)
    , ReadOnly(this, &TCustomListView::GetReadOnlyImpl, &TCustomListView::SetReadOnlyImpl)
    , RowSelect(this, &TCustomListView::GetRowSelectImpl, &TCustomListView::SetRowSelectImpl)
    , items_(no_vcl_TCustomListView_GetItems(handle))
{}

void TCustomListView::Clear()          { no_vcl_TCustomListView_Clear(handle_); }
void TCustomListView::BeginUpdate()    { no_vcl_TCustomListView_BeginUpdate(handle_); }
void TCustomListView::EndUpdate()      { no_vcl_TCustomListView_EndUpdate(handle_); }
void TCustomListView::ClearSelection() { no_vcl_TCustomListView_ClearSelection(handle_); }
void TCustomListView::SelectAll()      { no_vcl_TCustomListView_SelectAll(handle_); }

TListItem* TCustomListView::GetItemAt(int X, int Y) const
{
    return TListItem::Wrap(no_vcl_TCustomListView_GetItemAt(handle_, X, Y));
}

TListItems* TCustomListView::GetItemsImpl(TObject* owner)   { return &static_cast<TCustomListView*>(owner)->items_; }
TListItem*  TCustomListView::GetSelectedImpl(TObject* owner) { return TListItem::Wrap(no_vcl_TCustomListView_GetSelected(owner->Handle())); }
void TCustomListView::SetSelectedImpl(TObject* owner, TListItem* const& value) { no_vcl_TCustomListView_SetSelected(owner->Handle(), ItemHandle(value)); }
int  TCustomListView::GetItemIndexImpl(TObject* owner)                   { return no_vcl_TCustomListView_GetItemIndex(owner->Handle()); }
void TCustomListView::SetItemIndexImpl(TObject* owner, const int& value)  { no_vcl_TCustomListView_SetItemIndex(owner->Handle(), value); }
int  TCustomListView::GetSelCountImpl(TObject* owner)                    { return no_vcl_TCustomListView_GetSelCount(owner->Handle()); }
bool TCustomListView::GetCheckboxesImpl(TObject* owner)                  { return no_vcl_TCustomListView_GetCheckboxes(owner->Handle()) != 0; }
void TCustomListView::SetCheckboxesImpl(TObject* owner, const bool& value) { no_vcl_TCustomListView_SetCheckboxes(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetGridLinesImpl(TObject* owner)                   { return no_vcl_TCustomListView_GetGridLines(owner->Handle()) != 0; }
void TCustomListView::SetGridLinesImpl(TObject* owner, const bool& value) { no_vcl_TCustomListView_SetGridLines(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetMultiSelectImpl(TObject* owner)                 { return no_vcl_TCustomListView_GetMultiSelect(owner->Handle()) != 0; }
void TCustomListView::SetMultiSelectImpl(TObject* owner, const bool& value) { no_vcl_TCustomListView_SetMultiSelect(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetReadOnlyImpl(TObject* owner)                    { return no_vcl_TCustomListView_GetReadOnly(owner->Handle()) != 0; }
void TCustomListView::SetReadOnlyImpl(TObject* owner, const bool& value)  { no_vcl_TCustomListView_SetReadOnly(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetRowSelectImpl(TObject* owner)                   { return no_vcl_TCustomListView_GetRowSelect(owner->Handle()) != 0; }
void TCustomListView::SetRowSelectImpl(TObject* owner, const bool& value) { no_vcl_TCustomListView_SetRowSelect(owner->Handle(), value ? 1 : 0); }

TListView::TListView(TComponent* AOwner)
    : TCustomListView(no_vcl_TListView_Create(HandleOf(AOwner)))
    , Columns(this, &TListView::GetColumnsImpl)
    , ViewStyle(this, &TListView::GetViewStyleImpl, &TListView::SetViewStyleImpl)
    , HideSelection(this, &TListView::GetHideSelectionImpl, &TListView::SetHideSelectionImpl)
    , SortType(this, &TListView::GetSortTypeImpl, &TListView::SetSortTypeImpl)
    , SortColumn(this, &TListView::GetSortColumnImpl, &TListView::SetSortColumnImpl)
    , SortDirection(this, &TListView::GetSortDirectionImpl, &TListView::SetSortDirectionImpl)
    , OnSelectItem(this, &TListView::GetOnSelectItemImpl, &TListView::SetOnSelectItemImpl)
    , OnChange(this, &TListView::GetOnChangeImpl, &TListView::SetOnChangeImpl)
    , OnDeletion(this, &TListView::GetOnDeletionImpl, &TListView::SetOnDeletionImpl)
    , OnItemChecked(this, &TListView::GetOnItemCheckedImpl, &TListView::SetOnItemCheckedImpl)
    , OnColumnClick(this, &TListView::GetOnColumnClickImpl, &TListView::SetOnColumnClickImpl)
    , columns_(no_vcl_TListView_GetColumns(handle_))
{}

TListColumns* TListView::GetColumnsImpl(TObject* owner) { return &static_cast<TListView*>(owner)->columns_; }
TViewStyle TListView::GetViewStyleImpl(TObject* owner) { return static_cast<TViewStyle>(no_vcl_TListView_GetViewStyle(owner->Handle())); }
void TListView::SetViewStyleImpl(TObject* owner, const TViewStyle& value) { no_vcl_TListView_SetViewStyle(owner->Handle(), value); }
bool TListView::GetHideSelectionImpl(TObject* owner)                    { return no_vcl_TListView_GetHideSelection(owner->Handle()) != 0; }
void TListView::SetHideSelectionImpl(TObject* owner, const bool& value)  { no_vcl_TListView_SetHideSelection(owner->Handle(), value ? 1 : 0); }
TSortType TListView::GetSortTypeImpl(TObject* owner) { return static_cast<TSortType>(no_vcl_TListView_GetSortType(owner->Handle())); }
void TListView::SetSortTypeImpl(TObject* owner, const TSortType& value) { no_vcl_TListView_SetSortType(owner->Handle(), value); }
int  TListView::GetSortColumnImpl(TObject* owner)                       { return no_vcl_TListView_GetSortColumn(owner->Handle()); }
void TListView::SetSortColumnImpl(TObject* owner, const int& value)      { no_vcl_TListView_SetSortColumn(owner->Handle(), value); }
TSortDirection TListView::GetSortDirectionImpl(TObject* owner) { return static_cast<TSortDirection>(no_vcl_TListView_GetSortDirection(owner->Handle())); }
void TListView::SetSortDirectionImpl(TObject* owner, const TSortDirection& value) { no_vcl_TListView_SetSortDirection(owner->Handle(), value); }

// リストビューの破棄では、リストビュー自身のラッパーが delete された後に項目が破棄される(LCL の順序)。
// そのときの OnDeletion は FromHandle が nullptr を返すため、ハンドラは呼ばれない。
void NO_VCL_CALL TListView::SelectItemTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, no_vcl_int_t selected, void*)
{
    TListView* self = static_cast<TListView*>(FromHandle(sender));
    if (!self || !self->onSelectItem_)
        return;
    TLVSelectItemEvent handler = self->onSelectItem_;
    handler(self, TListItem::Wrap(item), selected != 0);
}

void NO_VCL_CALL TListView::ChangeTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, no_vcl_int_t change, void*)
{
    TListView* self = static_cast<TListView*>(FromHandle(sender));
    if (!self || !self->onChange_)
        return;
    TLVChangeEvent handler = self->onChange_;
    handler(self, TListItem::Wrap(item), static_cast<TItemChange>(change));
}

void NO_VCL_CALL TListView::DeletionTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, void*)
{
    TListView* self = static_cast<TListView*>(FromHandle(sender));
    if (!self || !self->onDeletion_)
        return;
    TLVDeletedEvent handler = self->onDeletion_;
    handler(self, TListItem::Wrap(item));
}

void NO_VCL_CALL TListView::ItemCheckedTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, void*)
{
    TListView* self = static_cast<TListView*>(FromHandle(sender));
    if (!self || !self->onItemChecked_)
        return;
    TLVCheckedItemEvent handler = self->onItemChecked_;
    handler(self, TListItem::Wrap(item));
}

void NO_VCL_CALL TListView::ColumnClickTrampoline(no_vcl_obj_t sender, no_vcl_obj_t column, void*)
{
    TListView* self = static_cast<TListView*>(FromHandle(sender));
    if (!self || !self->onColumnClick_)
        return;
    TLVColumnClickEvent handler = self->onColumnClick_;
    handler(self, TListColumn::Wrap(column));
}

TLVSelectItemEvent  TListView::GetOnSelectItemImpl(TObject* owner)  { return static_cast<TListView*>(owner)->onSelectItem_; }
TLVChangeEvent      TListView::GetOnChangeImpl(TObject* owner)      { return static_cast<TListView*>(owner)->onChange_; }
TLVDeletedEvent     TListView::GetOnDeletionImpl(TObject* owner)    { return static_cast<TListView*>(owner)->onDeletion_; }
TLVCheckedItemEvent TListView::GetOnItemCheckedImpl(TObject* owner) { return static_cast<TListView*>(owner)->onItemChecked_; }
TLVColumnClickEvent TListView::GetOnColumnClickImpl(TObject* owner) { return static_cast<TListView*>(owner)->onColumnClick_; }

void TListView::SetOnSelectItemImpl(TObject* owner, const TLVSelectItemEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onSelectItem_, self->onSelectItemHooked_, value,
                   &no_vcl_TListView_SetOnSelectItem, &TListView::SelectItemTrampoline);
}

void TListView::SetOnChangeImpl(TObject* owner, const TLVChangeEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TListView_SetOnChange, &TListView::ChangeTrampoline);
}

void TListView::SetOnDeletionImpl(TObject* owner, const TLVDeletedEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onDeletion_, self->onDeletionHooked_, value,
                   &no_vcl_TListView_SetOnDeletion, &TListView::DeletionTrampoline);
}

void TListView::SetOnItemCheckedImpl(TObject* owner, const TLVCheckedItemEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onItemChecked_, self->onItemCheckedHooked_, value,
                   &no_vcl_TListView_SetOnItemChecked, &TListView::ItemCheckedTrampoline);
}

void TListView::SetOnColumnClickImpl(TObject* owner, const TLVColumnClickEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onColumnClick_, self->onColumnClickHooked_, value,
                   &no_vcl_TListView_SetOnColumnClick, &TListView::ColumnClickTrampoline);
}

TCustomSplitter::TCustomSplitter(no_vcl_obj_t handle)
    : TCustomControl(handle)
    , AutoSnap(this, &TCustomSplitter::GetAutoSnapImpl, &TCustomSplitter::SetAutoSnapImpl)
    , Beveled(this, &TCustomSplitter::GetBeveledImpl, &TCustomSplitter::SetBeveledImpl)
    , MinSize(this, &TCustomSplitter::GetMinSizeImpl, &TCustomSplitter::SetMinSizeImpl)
    , ResizeAnchor(this, &TCustomSplitter::GetResizeAnchorImpl, &TCustomSplitter::SetResizeAnchorImpl)
    , ResizeStyle(this, &TCustomSplitter::GetResizeStyleImpl, &TCustomSplitter::SetResizeStyleImpl)
    , OnMoved(this, &TCustomSplitter::GetOnMovedImpl, &TCustomSplitter::SetOnMovedImpl)
{}

int  TCustomSplitter::GetSplitterPosition() const  { return no_vcl_TCustomSplitter_GetSplitterPosition(handle_); }
void TCustomSplitter::SetSplitterPosition(int pos) { no_vcl_TCustomSplitter_SetSplitterPosition(handle_, pos); }

bool TCustomSplitter::GetAutoSnapImpl(TObject* owner)                   { return no_vcl_TCustomSplitter_GetAutoSnap(owner->Handle()) != 0; }
void TCustomSplitter::SetAutoSnapImpl(TObject* owner, const bool& value) { no_vcl_TCustomSplitter_SetAutoSnap(owner->Handle(), value ? 1 : 0); }
bool TCustomSplitter::GetBeveledImpl(TObject* owner)                    { return no_vcl_TCustomSplitter_GetBeveled(owner->Handle()) != 0; }
void TCustomSplitter::SetBeveledImpl(TObject* owner, const bool& value)  { no_vcl_TCustomSplitter_SetBeveled(owner->Handle(), value ? 1 : 0); }
int  TCustomSplitter::GetMinSizeImpl(TObject* owner)                    { return no_vcl_TCustomSplitter_GetMinSize(owner->Handle()); }
void TCustomSplitter::SetMinSizeImpl(TObject* owner, const int& value)   { no_vcl_TCustomSplitter_SetMinSize(owner->Handle(), value); }

TAnchorKind TCustomSplitter::GetResizeAnchorImpl(TObject* owner) { return static_cast<TAnchorKind>(no_vcl_TCustomSplitter_GetResizeAnchor(owner->Handle())); }
void TCustomSplitter::SetResizeAnchorImpl(TObject* owner, const TAnchorKind& value) { no_vcl_TCustomSplitter_SetResizeAnchor(owner->Handle(), value); }
TResizeStyle TCustomSplitter::GetResizeStyleImpl(TObject* owner) { return static_cast<TResizeStyle>(no_vcl_TCustomSplitter_GetResizeStyle(owner->Handle())); }
void TCustomSplitter::SetResizeStyleImpl(TObject* owner, const TResizeStyle& value) { no_vcl_TCustomSplitter_SetResizeStyle(owner->Handle(), value); }

TNotifyEvent TCustomSplitter::GetOnMovedImpl(TObject* owner) { return static_cast<TCustomSplitter*>(owner)->onMoved_; }

void TCustomSplitter::SetOnMovedImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomSplitter* self = static_cast<TCustomSplitter*>(owner);
    SetSimpleEvent(self->handle_, self->onMoved_, self->onMovedHooked_, value,
                   &no_vcl_TCustomSplitter_SetOnMoved, &TCustomSplitter::MovedTrampoline);
}

void NO_VCL_CALL TCustomSplitter::MovedTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomSplitter* self = static_cast<TCustomSplitter*>(FromHandle(sender)))
        CallNotify(self->onMoved_, self);
}

TSplitter::TSplitter(TComponent* AOwner)
    : TCustomSplitter(no_vcl_TSplitter_Create(HandleOf(AOwner)))
{}

TCustomMemo::TCustomMemo(no_vcl_obj_t handle)
    : TCustomEdit(handle)
    , ScrollBars(this, &TCustomMemo::GetScrollBarsImpl, &TCustomMemo::SetScrollBarsImpl)
    , Lines(this, &TCustomMemo::GetLinesImpl)
    , lines_(this, &no_vcl_TCustomMemo_GetLines)
{}

TStrings* TCustomMemo::GetLinesImpl(TObject* owner) { return &static_cast<TCustomMemo*>(owner)->lines_; }

int  TCustomMemo::GetScrollBarsImpl(TObject* owner)                   { return no_vcl_TCustomMemo_GetScrollBars(owner->Handle()); }
void TCustomMemo::SetScrollBarsImpl(TObject* owner, const int& value) { no_vcl_TCustomMemo_SetScrollBars(owner->Handle(), value); }

TMemo::TMemo(TComponent* AOwner)
    : TCustomMemo(no_vcl_TMemo_Create(HandleOf(AOwner)))
{}

/* ---------------- ComboBox / ListBox ---------------- */

TCustomComboBox::TCustomComboBox(no_vcl_obj_t handle)
    : TWinControl(handle)
    , ItemIndex(this, &TCustomComboBox::GetItemIndexImpl, &TCustomComboBox::SetItemIndexImpl)
    , Items(this, &TCustomComboBox::GetItemsImpl)
    , items_(this, &no_vcl_TCustomComboBox_GetItems)
{}

TStrings* TCustomComboBox::GetItemsImpl(TObject* owner) { return &static_cast<TCustomComboBox*>(owner)->items_; }

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
    , Items(this, &TCustomListBox::GetItemsImpl)
    , items_(this, &no_vcl_TCustomListBox_GetItems)
{}

TStrings* TCustomListBox::GetItemsImpl(TObject* owner) { return &static_cast<TCustomListBox*>(owner)->items_; }

int  TCustomListBox::GetItemIndexImpl(TObject* owner)                   { return no_vcl_TCustomListBox_GetItemIndex(owner->Handle()); }
void TCustomListBox::SetItemIndexImpl(TObject* owner, const int& value) { no_vcl_TCustomListBox_SetItemIndex(owner->Handle(), value); }

TListBox::TListBox(TComponent* AOwner)
    : TCustomListBox(no_vcl_TListBox_Create(HandleOf(AOwner)))
{}

TCustomCheckListBox::TCustomCheckListBox(no_vcl_obj_t handle)
    : TCustomListBox(handle)
    , OnClickCheck(this, &TCustomCheckListBox::GetOnClickCheckImpl, &TCustomCheckListBox::SetOnClickCheckImpl)
    , Checked(this, &TCustomCheckListBox::GetCheckedImpl, &TCustomCheckListBox::SetCheckedImpl)
{}

bool TCustomCheckListBox::GetCheckedImpl(TObject* owner, int index)                    { return no_vcl_TCustomCheckListBox_GetChecked(owner->Handle(), index) != 0; }
void TCustomCheckListBox::SetCheckedImpl(TObject* owner, int index, const bool& value) { no_vcl_TCustomCheckListBox_SetChecked(owner->Handle(), index, value ? 1 : 0); }

TNotifyEvent TCustomCheckListBox::GetOnClickCheckImpl(TObject* owner) { return static_cast<TCustomCheckListBox*>(owner)->onClickCheck_; }

void TCustomCheckListBox::SetOnClickCheckImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomCheckListBox* self = static_cast<TCustomCheckListBox*>(owner);
    SetSimpleEvent(self->handle_, self->onClickCheck_, self->onClickCheckHooked_, value,
                   &no_vcl_TCustomCheckListBox_SetOnClickCheck, &TCustomCheckListBox::ClickCheckTrampoline);
}

void NO_VCL_CALL TCustomCheckListBox::ClickCheckTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomCheckListBox* self = static_cast<TCustomCheckListBox*>(FromHandle(sender)))
        CallNotify(self->onClickCheck_, self);
}

TCheckListBox::TCheckListBox(TComponent* AOwner)
    : TCustomCheckListBox(no_vcl_TCheckListBox_Create(HandleOf(AOwner)))
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

/* ---------------- メニュー ---------------- */

TShortCut ShortCut(unsigned short Key, TShiftState Shift)
{
    return static_cast<TShortCut>(no_vcl_ShortCut_Make(Key, static_cast<no_vcl_int_t>(Shift)));
}

TShortCut TextToShortCut(const std::string& Text)
{
    return static_cast<TShortCut>(no_vcl_ShortCut_FromText(Text.c_str()));
}

std::string ShortCutToText(TShortCut ShortCut)
{
    return std::string(no_vcl_ShortCut_ToText(ShortCut));
}

TMenuItem::TMenuItem(TComponent* AOwner)
    : TMenuItem(no_vcl_TMenuItem_Create(HandleOf(AOwner)))
{}

TMenuItem::TMenuItem(no_vcl_obj_t handle)
    : TComponent(handle)
    , Caption(this, &TMenuItem::GetCaptionImpl, &TMenuItem::SetCaptionImpl)
    , Checked(this, &TMenuItem::GetCheckedImpl, &TMenuItem::SetCheckedImpl)
    , Enabled(this, &TMenuItem::GetEnabledImpl, &TMenuItem::SetEnabledImpl)
    , Visible(this, &TMenuItem::GetVisibleImpl, &TMenuItem::SetVisibleImpl)
    , AutoCheck(this, &TMenuItem::GetAutoCheckImpl, &TMenuItem::SetAutoCheckImpl)
    , RadioItem(this, &TMenuItem::GetRadioItemImpl, &TMenuItem::SetRadioItemImpl)
    , GroupIndex(this, &TMenuItem::GetGroupIndexImpl, &TMenuItem::SetGroupIndexImpl)
    , Default(this, &TMenuItem::GetDefaultImpl, &TMenuItem::SetDefaultImpl)
    , ShortCut(this, &TMenuItem::GetShortCutImpl, &TMenuItem::SetShortCutImpl)
    , Hint(this, &TMenuItem::GetHintImpl, &TMenuItem::SetHintImpl)
    , OnClick(this, &TMenuItem::GetOnClickImpl, &TMenuItem::SetOnClickImpl)
    , Count(this, &TMenuItem::GetCountImpl)
    , Parent(this, &TMenuItem::GetParentImpl)
    , Items(this, &TMenuItem::GetItemsImpl)
{}

TMenuItem* TMenuItem::GetItemsImpl(TObject* owner, int Index) { return WrapExisting<TMenuItem>(no_vcl_TMenuItem_GetItem(owner->Handle(), Index)); }
void TMenuItem::Add(TMenuItem* Item)              { no_vcl_TMenuItem_Add(handle_, HandleOf(Item)); }
void TMenuItem::Insert(int Index, TMenuItem* Item) { no_vcl_TMenuItem_Insert(handle_, Index, HandleOf(Item)); }
void TMenuItem::Delete(int Index)                 { no_vcl_TMenuItem_Delete(handle_, Index); }
void TMenuItem::Remove(TMenuItem* Item)           { no_vcl_TMenuItem_Remove(handle_, HandleOf(Item)); }
void TMenuItem::Clear()                           { no_vcl_TMenuItem_Clear(handle_); }
int  TMenuItem::IndexOf(TMenuItem* Item) const    { return no_vcl_TMenuItem_IndexOf(handle_, HandleOf(Item)); }
void TMenuItem::AddSeparator()                    { no_vcl_TMenuItem_AddSeparator(handle_); }
bool TMenuItem::IsLine() const                    { return no_vcl_TMenuItem_IsLine(handle_) != 0; }
void TMenuItem::Click()                           { no_vcl_TMenuItem_Click(handle_); }

std::string TMenuItem::GetCaptionImpl(TObject* owner) { return std::string(no_vcl_TMenuItem_GetCaption(owner->Handle())); }
void TMenuItem::SetCaptionImpl(TObject* owner, const std::string& value) { no_vcl_TMenuItem_SetCaption(owner->Handle(), value.c_str()); }
bool TMenuItem::GetCheckedImpl(TObject* owner)                     { return no_vcl_TMenuItem_GetChecked(owner->Handle()) != 0; }
void TMenuItem::SetCheckedImpl(TObject* owner, const bool& value)   { no_vcl_TMenuItem_SetChecked(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetEnabledImpl(TObject* owner)                     { return no_vcl_TMenuItem_GetEnabled(owner->Handle()) != 0; }
void TMenuItem::SetEnabledImpl(TObject* owner, const bool& value)   { no_vcl_TMenuItem_SetEnabled(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetVisibleImpl(TObject* owner)                     { return no_vcl_TMenuItem_GetVisible(owner->Handle()) != 0; }
void TMenuItem::SetVisibleImpl(TObject* owner, const bool& value)   { no_vcl_TMenuItem_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetAutoCheckImpl(TObject* owner)                   { return no_vcl_TMenuItem_GetAutoCheck(owner->Handle()) != 0; }
void TMenuItem::SetAutoCheckImpl(TObject* owner, const bool& value) { no_vcl_TMenuItem_SetAutoCheck(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetRadioItemImpl(TObject* owner)                   { return no_vcl_TMenuItem_GetRadioItem(owner->Handle()) != 0; }
void TMenuItem::SetRadioItemImpl(TObject* owner, const bool& value) { no_vcl_TMenuItem_SetRadioItem(owner->Handle(), value ? 1 : 0); }
int  TMenuItem::GetGroupIndexImpl(TObject* owner)                  { return no_vcl_TMenuItem_GetGroupIndex(owner->Handle()); }
void TMenuItem::SetGroupIndexImpl(TObject* owner, const int& value) { no_vcl_TMenuItem_SetGroupIndex(owner->Handle(), value); }
bool TMenuItem::GetDefaultImpl(TObject* owner)                     { return no_vcl_TMenuItem_GetDefault(owner->Handle()) != 0; }
void TMenuItem::SetDefaultImpl(TObject* owner, const bool& value)   { no_vcl_TMenuItem_SetDefault(owner->Handle(), value ? 1 : 0); }
TShortCut TMenuItem::GetShortCutImpl(TObject* owner) { return static_cast<TShortCut>(no_vcl_TMenuItem_GetShortCut(owner->Handle())); }
void TMenuItem::SetShortCutImpl(TObject* owner, const TShortCut& value) { no_vcl_TMenuItem_SetShortCut(owner->Handle(), value); }
std::string TMenuItem::GetHintImpl(TObject* owner) { return std::string(no_vcl_TMenuItem_GetHint(owner->Handle())); }
void TMenuItem::SetHintImpl(TObject* owner, const std::string& value) { no_vcl_TMenuItem_SetHint(owner->Handle(), value.c_str()); }
int  TMenuItem::GetCountImpl(TObject* owner) { return no_vcl_TMenuItem_GetCount(owner->Handle()); }
TMenuItem* TMenuItem::GetParentImpl(TObject* owner) { return WrapExisting<TMenuItem>(no_vcl_TMenuItem_GetParent(owner->Handle())); }

TNotifyEvent TMenuItem::GetOnClickImpl(TObject* owner) { return static_cast<TMenuItem*>(owner)->onClick_; }

void TMenuItem::SetOnClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TMenuItem* self = static_cast<TMenuItem*>(owner);
    SetSimpleEvent(self->handle_, self->onClick_, self->onClickHooked_, value,
                   &no_vcl_TMenuItem_SetOnClick, &TMenuItem::ClickTrampoline);
}

void NO_VCL_CALL TMenuItem::ClickTrampoline(no_vcl_obj_t sender, void*)
{
    if (TMenuItem* self = static_cast<TMenuItem*>(FromHandle(sender)))
        CallNotify(self->onClick_, self);
}

TMenu::TMenu(no_vcl_obj_t handle)
    : TComponent(handle)
    , Items(this, &TMenu::GetItemsImpl)
{}

TMenuItem* TMenu::GetItemsImpl(TObject* owner) { return WrapExisting<TMenuItem>(no_vcl_TMenu_GetItems(owner->Handle())); }

TMainMenu::TMainMenu(TComponent* AOwner)
    : TMenu(no_vcl_TMainMenu_Create(HandleOf(AOwner)))
{}

TPopupMenu::TPopupMenu(TComponent* AOwner)
    : TMenu(no_vcl_TPopupMenu_Create(HandleOf(AOwner)))
    , AutoPopup(this, &TPopupMenu::GetAutoPopupImpl, &TPopupMenu::SetAutoPopupImpl)
    , PopupComponent(this, &TPopupMenu::GetPopupComponentImpl, &TPopupMenu::SetPopupComponentImpl)
    , OnPopup(this, &TPopupMenu::GetOnPopupImpl, &TPopupMenu::SetOnPopupImpl)
    , OnClose(this, &TPopupMenu::GetOnCloseImpl, &TPopupMenu::SetOnCloseImpl)
{}

void TPopupMenu::Popup(int X, int Y) { no_vcl_TPopupMenu_Popup(handle_, X, Y); }

bool TPopupMenu::GetAutoPopupImpl(TObject* owner)                   { return no_vcl_TPopupMenu_GetAutoPopup(owner->Handle()) != 0; }
void TPopupMenu::SetAutoPopupImpl(TObject* owner, const bool& value) { no_vcl_TPopupMenu_SetAutoPopup(owner->Handle(), value ? 1 : 0); }
TComponent* TPopupMenu::GetPopupComponentImpl(TObject* owner) { return FromHandle(no_vcl_TPopupMenu_GetPopupComponent(owner->Handle())); }
void TPopupMenu::SetPopupComponentImpl(TObject* owner, TComponent* const& value) { no_vcl_TPopupMenu_SetPopupComponent(owner->Handle(), HandleOf(value)); }

TNotifyEvent TPopupMenu::GetOnPopupImpl(TObject* owner) { return static_cast<TPopupMenu*>(owner)->onPopup_; }
TNotifyEvent TPopupMenu::GetOnCloseImpl(TObject* owner) { return static_cast<TPopupMenu*>(owner)->onClose_; }

void TPopupMenu::SetOnPopupImpl(TObject* owner, const TNotifyEvent& value)
{
    TPopupMenu* self = static_cast<TPopupMenu*>(owner);
    SetSimpleEvent(self->handle_, self->onPopup_, self->onPopupHooked_, value,
                   &no_vcl_TPopupMenu_SetOnPopup, &TPopupMenu::PopupTrampoline);
}

void TPopupMenu::SetOnCloseImpl(TObject* owner, const TNotifyEvent& value)
{
    TPopupMenu* self = static_cast<TPopupMenu*>(owner);
    SetSimpleEvent(self->handle_, self->onClose_, self->onCloseHooked_, value,
                   &no_vcl_TPopupMenu_SetOnClose, &TPopupMenu::CloseTrampoline);
}

void NO_VCL_CALL TPopupMenu::PopupTrampoline(no_vcl_obj_t sender, void*)
{
    if (TPopupMenu* self = static_cast<TPopupMenu*>(FromHandle(sender)))
        CallNotify(self->onPopup_, self);
}

void NO_VCL_CALL TPopupMenu::CloseTrampoline(no_vcl_obj_t sender, void*)
{
    if (TPopupMenu* self = static_cast<TPopupMenu*>(FromHandle(sender)))
        CallNotify(self->onClose_, self);
}

/* ---------------- Grid ---------------- */

void TCustomGrid::BeginUpdate() { no_vcl_TCustomGrid_BeginUpdate(handle_); }
void TCustomGrid::EndUpdate()   { no_vcl_TCustomGrid_EndUpdate(handle_); }
void TCustomGrid::Clear()       { no_vcl_TCustomGrid_Clear(handle_); }

TRect TCustomGrid::CellRect(int ACol, int ARow) const
{
    TRect r{};
    no_vcl_TCustomGrid_CellRect(handle_, ACol, ARow, &r.Left, &r.Top, &r.Right, &r.Bottom);
    return r;
}

void TCustomGrid::MouseToCell(int X, int Y, int& ACol, int& ARow) const
{
    no_vcl_TCustomGrid_MouseToCell(handle_, X, Y, &ACol, &ARow);
}

TCustomDrawGrid::TCustomDrawGrid(no_vcl_obj_t handle)
    : TCustomGrid(handle)
    , Canvas(no_vcl_TCustomDrawGrid_GetCanvas(handle))
    , ColCount(this, &TCustomDrawGrid::GetColCountImpl, &TCustomDrawGrid::SetColCountImpl)
    , RowCount(this, &TCustomDrawGrid::GetRowCountImpl, &TCustomDrawGrid::SetRowCountImpl)
    , FixedCols(this, &TCustomDrawGrid::GetFixedColsImpl, &TCustomDrawGrid::SetFixedColsImpl)
    , FixedRows(this, &TCustomDrawGrid::GetFixedRowsImpl, &TCustomDrawGrid::SetFixedRowsImpl)
    , Col(this, &TCustomDrawGrid::GetColImpl, &TCustomDrawGrid::SetColImpl)
    , Row(this, &TCustomDrawGrid::GetRowImpl, &TCustomDrawGrid::SetRowImpl)
    , DefaultColWidth(this, &TCustomDrawGrid::GetDefaultColWidthImpl, &TCustomDrawGrid::SetDefaultColWidthImpl)
    , DefaultRowHeight(this, &TCustomDrawGrid::GetDefaultRowHeightImpl, &TCustomDrawGrid::SetDefaultRowHeightImpl)
    , Options(this, &TCustomDrawGrid::GetOptionsImpl, &TCustomDrawGrid::SetOptionsImpl)
    , Selection(this, &TCustomDrawGrid::GetSelectionImpl, &TCustomDrawGrid::SetSelectionImpl)
    , LeftCol(this, &TCustomDrawGrid::GetLeftColImpl, &TCustomDrawGrid::SetLeftColImpl)
    , TopRow(this, &TCustomDrawGrid::GetTopRowImpl, &TCustomDrawGrid::SetTopRowImpl)
    , DefaultDrawing(this, &TCustomDrawGrid::GetDefaultDrawingImpl, &TCustomDrawGrid::SetDefaultDrawingImpl)
    , FixedColor(this, &TCustomDrawGrid::GetFixedColorImpl, &TCustomDrawGrid::SetFixedColorImpl)
    , EditorMode(this, &TCustomDrawGrid::GetEditorModeImpl, &TCustomDrawGrid::SetEditorModeImpl)
    , OnDrawCell(this, &TCustomDrawGrid::GetOnDrawCellImpl, &TCustomDrawGrid::SetOnDrawCellImpl)
    , OnSelectCell(this, &TCustomDrawGrid::GetOnSelectCellImpl, &TCustomDrawGrid::SetOnSelectCellImpl)
    , OnSelection(this, &TCustomDrawGrid::GetOnSelectionImpl, &TCustomDrawGrid::SetOnSelectionImpl)
    , OnHeaderClick(this, &TCustomDrawGrid::GetOnHeaderClickImpl, &TCustomDrawGrid::SetOnHeaderClickImpl)
    , ColWidths(this, &TCustomDrawGrid::GetColWidthsImpl, &TCustomDrawGrid::SetColWidthsImpl)
    , RowHeights(this, &TCustomDrawGrid::GetRowHeightsImpl, &TCustomDrawGrid::SetRowHeightsImpl)
{}

int  TCustomDrawGrid::GetColWidthsImpl(TObject* owner, int ACol)                     { return no_vcl_TCustomDrawGrid_GetColWidths(owner->Handle(), ACol); }
void TCustomDrawGrid::SetColWidthsImpl(TObject* owner, int ACol, const int& value)   { no_vcl_TCustomDrawGrid_SetColWidths(owner->Handle(), ACol, value); }
int  TCustomDrawGrid::GetRowHeightsImpl(TObject* owner, int ARow)                    { return no_vcl_TCustomDrawGrid_GetRowHeights(owner->Handle(), ARow); }
void TCustomDrawGrid::SetRowHeightsImpl(TObject* owner, int ARow, const int& value)  { no_vcl_TCustomDrawGrid_SetRowHeights(owner->Handle(), ARow, value); }

void TCustomDrawGrid::InsertColRow(bool IsColumn, int Index) { no_vcl_TCustomDrawGrid_InsertColRow(handle_, IsColumn ? 1 : 0, Index); }
void TCustomDrawGrid::DeleteColRow(bool IsColumn, int Index) { no_vcl_TCustomDrawGrid_DeleteColRow(handle_, IsColumn ? 1 : 0, Index); }
void TCustomDrawGrid::SortColRow(bool IsColumn, int Index)   { no_vcl_TCustomDrawGrid_SortColRow(handle_, IsColumn ? 1 : 0, Index); }

void TCustomDrawGrid::MoveColRow(bool IsColumn, int FromIndex, int ToIndex)
{
    no_vcl_TCustomDrawGrid_MoveColRow(handle_, IsColumn ? 1 : 0, FromIndex, ToIndex);
}

int  TCustomDrawGrid::GetColCountImpl(TObject* owner)                        { return no_vcl_TCustomDrawGrid_GetColCount(owner->Handle()); }
void TCustomDrawGrid::SetColCountImpl(TObject* owner, const int& value)       { no_vcl_TCustomDrawGrid_SetColCount(owner->Handle(), value); }
int  TCustomDrawGrid::GetRowCountImpl(TObject* owner)                        { return no_vcl_TCustomDrawGrid_GetRowCount(owner->Handle()); }
void TCustomDrawGrid::SetRowCountImpl(TObject* owner, const int& value)       { no_vcl_TCustomDrawGrid_SetRowCount(owner->Handle(), value); }
int  TCustomDrawGrid::GetFixedColsImpl(TObject* owner)                       { return no_vcl_TCustomDrawGrid_GetFixedCols(owner->Handle()); }
void TCustomDrawGrid::SetFixedColsImpl(TObject* owner, const int& value)      { no_vcl_TCustomDrawGrid_SetFixedCols(owner->Handle(), value); }
int  TCustomDrawGrid::GetFixedRowsImpl(TObject* owner)                       { return no_vcl_TCustomDrawGrid_GetFixedRows(owner->Handle()); }
void TCustomDrawGrid::SetFixedRowsImpl(TObject* owner, const int& value)      { no_vcl_TCustomDrawGrid_SetFixedRows(owner->Handle(), value); }
int  TCustomDrawGrid::GetColImpl(TObject* owner)                             { return no_vcl_TCustomDrawGrid_GetCol(owner->Handle()); }
void TCustomDrawGrid::SetColImpl(TObject* owner, const int& value)            { no_vcl_TCustomDrawGrid_SetCol(owner->Handle(), value); }
int  TCustomDrawGrid::GetRowImpl(TObject* owner)                             { return no_vcl_TCustomDrawGrid_GetRow(owner->Handle()); }
void TCustomDrawGrid::SetRowImpl(TObject* owner, const int& value)            { no_vcl_TCustomDrawGrid_SetRow(owner->Handle(), value); }
int  TCustomDrawGrid::GetDefaultColWidthImpl(TObject* owner)                 { return no_vcl_TCustomDrawGrid_GetDefaultColWidth(owner->Handle()); }
void TCustomDrawGrid::SetDefaultColWidthImpl(TObject* owner, const int& value) { no_vcl_TCustomDrawGrid_SetDefaultColWidth(owner->Handle(), value); }
int  TCustomDrawGrid::GetDefaultRowHeightImpl(TObject* owner)                { return no_vcl_TCustomDrawGrid_GetDefaultRowHeight(owner->Handle()); }
void TCustomDrawGrid::SetDefaultRowHeightImpl(TObject* owner, const int& value) { no_vcl_TCustomDrawGrid_SetDefaultRowHeight(owner->Handle(), value); }
TGridOptions TCustomDrawGrid::GetOptionsImpl(TObject* owner)                  { return no_vcl_TCustomDrawGrid_GetOptions(owner->Handle()); }
void TCustomDrawGrid::SetOptionsImpl(TObject* owner, const TGridOptions& value) { no_vcl_TCustomDrawGrid_SetOptions(owner->Handle(), value); }
int  TCustomDrawGrid::GetLeftColImpl(TObject* owner)                         { return no_vcl_TCustomDrawGrid_GetLeftCol(owner->Handle()); }
void TCustomDrawGrid::SetLeftColImpl(TObject* owner, const int& value)        { no_vcl_TCustomDrawGrid_SetLeftCol(owner->Handle(), value); }
int  TCustomDrawGrid::GetTopRowImpl(TObject* owner)                          { return no_vcl_TCustomDrawGrid_GetTopRow(owner->Handle()); }
void TCustomDrawGrid::SetTopRowImpl(TObject* owner, const int& value)         { no_vcl_TCustomDrawGrid_SetTopRow(owner->Handle(), value); }
bool TCustomDrawGrid::GetDefaultDrawingImpl(TObject* owner)                  { return no_vcl_TCustomDrawGrid_GetDefaultDrawing(owner->Handle()) != 0; }
void TCustomDrawGrid::SetDefaultDrawingImpl(TObject* owner, const bool& value) { no_vcl_TCustomDrawGrid_SetDefaultDrawing(owner->Handle(), value ? 1 : 0); }
TColor TCustomDrawGrid::GetFixedColorImpl(TObject* owner)                    { return no_vcl_TCustomDrawGrid_GetFixedColor(owner->Handle()); }
void TCustomDrawGrid::SetFixedColorImpl(TObject* owner, const TColor& value)  { no_vcl_TCustomDrawGrid_SetFixedColor(owner->Handle(), value); }
bool TCustomDrawGrid::GetEditorModeImpl(TObject* owner)                      { return no_vcl_TCustomDrawGrid_GetEditorMode(owner->Handle()) != 0; }
void TCustomDrawGrid::SetEditorModeImpl(TObject* owner, const bool& value)    { no_vcl_TCustomDrawGrid_SetEditorMode(owner->Handle(), value ? 1 : 0); }

TGridRect TCustomDrawGrid::GetSelectionImpl(TObject* owner)
{
    TGridRect r{};
    no_vcl_TCustomDrawGrid_GetSelection(owner->Handle(), &r.Left, &r.Top, &r.Right, &r.Bottom);
    return r;
}

void TCustomDrawGrid::SetSelectionImpl(TObject* owner, const TGridRect& value)
{
    no_vcl_TCustomDrawGrid_SetSelection(owner->Handle(), value.Left, value.Top, value.Right, value.Bottom);
}

void NO_VCL_CALL TCustomDrawGrid::DrawCellTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row,
                                                     no_vcl_int_t left, no_vcl_int_t top, no_vcl_int_t right, no_vcl_int_t bottom,
                                                     no_vcl_uint_t state, void*)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
    if (!self || !self->onDrawCell_)
        return;
    TOnDrawCell handler = self->onDrawCell_;
    handler(self, col, row, TRect{left, top, right, bottom}, state);
}

void NO_VCL_CALL TCustomDrawGrid::SelectCellTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row, no_vcl_bool_t* canSelect, void*)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
    if (!self || !self->onSelectCell_)
        return;
    TOnSelectCellEvent handler = self->onSelectCell_;
    bool value = *canSelect != 0;
    handler(self, col, row, value);
    *canSelect = value ? 1 : 0;
}

void NO_VCL_CALL TCustomDrawGrid::SelectionTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row, void*)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
    if (!self || !self->onSelection_)
        return;
    TOnSelectEvent handler = self->onSelection_;
    handler(self, col, row);
}

void NO_VCL_CALL TCustomDrawGrid::HeaderClickTrampoline(no_vcl_obj_t sender, no_vcl_int_t isColumn, no_vcl_int_t index, void*)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
    if (!self || !self->onHeaderClick_)
        return;
    THdrEvent handler = self->onHeaderClick_;
    handler(self, isColumn != 0, index);
}

TOnDrawCell        TCustomDrawGrid::GetOnDrawCellImpl(TObject* owner)    { return static_cast<TCustomDrawGrid*>(owner)->onDrawCell_; }
TOnSelectCellEvent TCustomDrawGrid::GetOnSelectCellImpl(TObject* owner)  { return static_cast<TCustomDrawGrid*>(owner)->onSelectCell_; }
TOnSelectEvent     TCustomDrawGrid::GetOnSelectionImpl(TObject* owner)   { return static_cast<TCustomDrawGrid*>(owner)->onSelection_; }
THdrEvent          TCustomDrawGrid::GetOnHeaderClickImpl(TObject* owner) { return static_cast<TCustomDrawGrid*>(owner)->onHeaderClick_; }

void TCustomDrawGrid::SetOnDrawCellImpl(TObject* owner, const TOnDrawCell& value)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(owner);
    SetSimpleEvent(self->handle_, self->onDrawCell_, self->onDrawCellHooked_, value,
                   &no_vcl_TCustomDrawGrid_SetOnDrawCell, &TCustomDrawGrid::DrawCellTrampoline);
}

void TCustomDrawGrid::SetOnSelectCellImpl(TObject* owner, const TOnSelectCellEvent& value)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(owner);
    SetSimpleEvent(self->handle_, self->onSelectCell_, self->onSelectCellHooked_, value,
                   &no_vcl_TCustomDrawGrid_SetOnSelectCell, &TCustomDrawGrid::SelectCellTrampoline);
}

void TCustomDrawGrid::SetOnSelectionImpl(TObject* owner, const TOnSelectEvent& value)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(owner);
    SetSimpleEvent(self->handle_, self->onSelection_, self->onSelectionHooked_, value,
                   &no_vcl_TCustomDrawGrid_SetOnSelection, &TCustomDrawGrid::SelectionTrampoline);
}

void TCustomDrawGrid::SetOnHeaderClickImpl(TObject* owner, const THdrEvent& value)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(owner);
    SetSimpleEvent(self->handle_, self->onHeaderClick_, self->onHeaderClickHooked_, value,
                   &no_vcl_TCustomDrawGrid_SetOnHeaderClick, &TCustomDrawGrid::HeaderClickTrampoline);
}

TDrawGrid::TDrawGrid(TComponent* AOwner)
    : TCustomDrawGrid(no_vcl_TDrawGrid_Create(HandleOf(AOwner)))
{}

TCustomStringGrid::TCustomStringGrid(no_vcl_obj_t handle)
    : TCustomDrawGrid(handle)
    , Cells(this, &TCustomStringGrid::GetCellsImpl, &TCustomStringGrid::SetCellsImpl)
{}

std::string TCustomStringGrid::GetCellsImpl(TObject* owner, int ACol, int ARow)
{
    return std::string(no_vcl_TCustomStringGrid_GetCells(owner->Handle(), ACol, ARow));
}

void TCustomStringGrid::SetCellsImpl(TObject* owner, int ACol, int ARow, const std::string& value)
{
    no_vcl_TCustomStringGrid_SetCells(owner->Handle(), ACol, ARow, value.c_str());
}

void TCustomStringGrid::Clean()                   { no_vcl_TCustomStringGrid_Clean(handle_); }
void TCustomStringGrid::AutoSizeColumns()         { no_vcl_TCustomStringGrid_AutoSizeColumns(handle_); }
void TCustomStringGrid::AutoSizeColumn(int ACol)  { no_vcl_TCustomStringGrid_AutoSizeColumn(handle_, ACol); }

TStringGrid::TStringGrid(TComponent* AOwner)
    : TCustomStringGrid(no_vcl_TStringGrid_Create(HandleOf(AOwner)))
{}

/* ---------------- HeaderControl ---------------- */

THeaderSection::THeaderSection(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Text(this, &THeaderSection::GetTextImpl, &THeaderSection::SetTextImpl)
    , Width(this, &THeaderSection::GetWidthImpl, &THeaderSection::SetWidthImpl)
    , MinWidth(this, &THeaderSection::GetMinWidthImpl, &THeaderSection::SetMinWidthImpl)
    , MaxWidth(this, &THeaderSection::GetMaxWidthImpl, &THeaderSection::SetMaxWidthImpl)
    , Alignment(this, &THeaderSection::GetAlignmentImpl, &THeaderSection::SetAlignmentImpl)
    , Visible(this, &THeaderSection::GetVisibleImpl, &THeaderSection::SetVisibleImpl)
    , Index(this, &THeaderSection::GetIndexImpl, &THeaderSection::SetIndexImpl)
    , Left(this, &THeaderSection::GetLeftImpl)
    , Right(this, &THeaderSection::GetRightImpl)
    , OriginalIndex(this, &THeaderSection::GetOriginalIndexImpl)
{}

std::string THeaderSection::GetTextImpl(TObject* owner) { return std::string(no_vcl_THeaderSection_GetText(owner->Handle())); }
void THeaderSection::SetTextImpl(TObject* owner, const std::string& value) { no_vcl_THeaderSection_SetText(owner->Handle(), value.c_str()); }
int  THeaderSection::GetWidthImpl(TObject* owner)                     { return no_vcl_THeaderSection_GetWidth(owner->Handle()); }
void THeaderSection::SetWidthImpl(TObject* owner, const int& value)    { no_vcl_THeaderSection_SetWidth(owner->Handle(), value); }
int  THeaderSection::GetMinWidthImpl(TObject* owner)                  { return no_vcl_THeaderSection_GetMinWidth(owner->Handle()); }
void THeaderSection::SetMinWidthImpl(TObject* owner, const int& value) { no_vcl_THeaderSection_SetMinWidth(owner->Handle(), value); }
int  THeaderSection::GetMaxWidthImpl(TObject* owner)                  { return no_vcl_THeaderSection_GetMaxWidth(owner->Handle()); }
void THeaderSection::SetMaxWidthImpl(TObject* owner, const int& value) { no_vcl_THeaderSection_SetMaxWidth(owner->Handle(), value); }
TAlignment THeaderSection::GetAlignmentImpl(TObject* owner) { return static_cast<TAlignment>(no_vcl_THeaderSection_GetAlignment(owner->Handle())); }
void THeaderSection::SetAlignmentImpl(TObject* owner, const TAlignment& value) { no_vcl_THeaderSection_SetAlignment(owner->Handle(), value); }
bool THeaderSection::GetVisibleImpl(TObject* owner)                   { return no_vcl_THeaderSection_GetVisible(owner->Handle()) != 0; }
void THeaderSection::SetVisibleImpl(TObject* owner, const bool& value) { no_vcl_THeaderSection_SetVisible(owner->Handle(), value ? 1 : 0); }
int  THeaderSection::GetIndexImpl(TObject* owner)                     { return no_vcl_THeaderSection_GetIndex(owner->Handle()); }
void THeaderSection::SetIndexImpl(TObject* owner, const int& value)    { no_vcl_THeaderSection_SetIndex(owner->Handle(), value); }
int  THeaderSection::GetLeftImpl(TObject* owner)                      { return no_vcl_THeaderSection_GetLeft(owner->Handle()); }
int  THeaderSection::GetRightImpl(TObject* owner)                     { return no_vcl_THeaderSection_GetRight(owner->Handle()); }
int  THeaderSection::GetOriginalIndexImpl(TObject* owner)             { return no_vcl_THeaderSection_GetOriginalIndex(owner->Handle()); }

THeaderSections::THeaderSections(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &THeaderSections::GetCountImpl)
    , Items(this, &THeaderSections::GetItemsImpl)
{}

THeaderSection* THeaderSections::Add()             { return THeaderSection::Wrap(no_vcl_THeaderSections_Add(handle_)); }
THeaderSection* THeaderSections::Insert(int Index) { return THeaderSection::Wrap(no_vcl_THeaderSections_Insert(handle_, Index)); }
void THeaderSections::Delete(int Index)            { no_vcl_THeaderSections_Delete(handle_, Index); }
void THeaderSections::Clear()                      { no_vcl_THeaderSections_Clear(handle_); }
void THeaderSections::BeginUpdate()                { no_vcl_THeaderSections_BeginUpdate(handle_); }
void THeaderSections::EndUpdate()                  { no_vcl_THeaderSections_EndUpdate(handle_); }
THeaderSection* THeaderSections::GetItemsImpl(TObject* owner, int Index) { return THeaderSection::Wrap(no_vcl_THeaderSections_GetItem(owner->Handle(), Index)); }
int  THeaderSections::GetCountImpl(TObject* owner) { return no_vcl_THeaderSections_GetCount(owner->Handle()); }

TCustomHeaderControl::TCustomHeaderControl(no_vcl_obj_t handle)
    : TCustomControl(handle)
    , Sections(this, &TCustomHeaderControl::GetSectionsImpl)
    , DragReorder(this, &TCustomHeaderControl::GetDragReorderImpl, &TCustomHeaderControl::SetDragReorderImpl)
    , SectionFromOriginalIndex(this, &TCustomHeaderControl::GetSectionFromOriginalIndexImpl)
    , OnSectionClick(this, &TCustomHeaderControl::GetOnSectionClickImpl, &TCustomHeaderControl::SetOnSectionClickImpl)
    , OnSectionResize(this, &TCustomHeaderControl::GetOnSectionResizeImpl, &TCustomHeaderControl::SetOnSectionResizeImpl)
    , OnSectionSeparatorDblClick(this, &TCustomHeaderControl::GetOnSectionSeparatorDblClickImpl, &TCustomHeaderControl::SetOnSectionSeparatorDblClickImpl)
    , OnSectionTrack(this, &TCustomHeaderControl::GetOnSectionTrackImpl, &TCustomHeaderControl::SetOnSectionTrackImpl)
    , OnSectionDrag(this, &TCustomHeaderControl::GetOnSectionDragImpl, &TCustomHeaderControl::SetOnSectionDragImpl)
    , OnSectionEndDrag(this, &TCustomHeaderControl::GetOnSectionEndDragImpl, &TCustomHeaderControl::SetOnSectionEndDragImpl)
    , sections_(no_vcl_TCustomHeaderControl_GetSections(handle_))
{}

int TCustomHeaderControl::GetSectionAt(const TPoint& P) const
{
    return no_vcl_TCustomHeaderControl_GetSectionAt(handle_, P.X, P.Y);
}

THeaderSections* TCustomHeaderControl::GetSectionsImpl(TObject* owner) { return &static_cast<TCustomHeaderControl*>(owner)->sections_; }
bool TCustomHeaderControl::GetDragReorderImpl(TObject* owner) { return no_vcl_TCustomHeaderControl_GetDragReorder(owner->Handle()) != 0; }
void TCustomHeaderControl::SetDragReorderImpl(TObject* owner, const bool& value) { no_vcl_TCustomHeaderControl_SetDragReorder(owner->Handle(), value ? 1 : 0); }
THeaderSection* TCustomHeaderControl::GetSectionFromOriginalIndexImpl(TObject* owner, int OriginalIndex)
{
    return THeaderSection::Wrap(no_vcl_TCustomHeaderControl_GetSectionFromOriginalIndex(owner->Handle(), OriginalIndex));
}

namespace
{

// OnSectionClick・OnSectionResize・OnSectionSeparatorDblClick の共通の呼び出し(ハンドラはコピーしてから呼ぶ)。
void CallSectionNotify(TCustomHeaderControl* self, TCustomSectionNotifyEvent handler, THeaderSection* section)
{
    if (handler)
        handler(self, section);
}

} // namespace

void NO_VCL_CALL TCustomHeaderControl::SectionClickTrampoline(no_vcl_obj_t sender, no_vcl_obj_t section, void*)
{
    if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
        CallSectionNotify(self, self->onSectionClick_, THeaderSection::Wrap(section));
}

void NO_VCL_CALL TCustomHeaderControl::SectionResizeTrampoline(no_vcl_obj_t sender, no_vcl_obj_t section, void*)
{
    if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
        CallSectionNotify(self, self->onSectionResize_, THeaderSection::Wrap(section));
}

void NO_VCL_CALL TCustomHeaderControl::SectionSeparatorDblClickTrampoline(no_vcl_obj_t sender, no_vcl_obj_t section, void*)
{
    if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
        CallSectionNotify(self, self->onSectionSeparatorDblClick_, THeaderSection::Wrap(section));
}

void NO_VCL_CALL TCustomHeaderControl::SectionTrackTrampoline(no_vcl_obj_t sender, no_vcl_obj_t section,
                                                              no_vcl_int_t width, no_vcl_int_t state, void*)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender));
    if (!self || !self->onSectionTrack_)
        return;
    TCustomSectionTrackEvent handler = self->onSectionTrack_;
    handler(self, THeaderSection::Wrap(section), width, static_cast<TSectionTrackState>(state));
}

void NO_VCL_CALL TCustomHeaderControl::SectionDragTrampoline(no_vcl_obj_t sender, no_vcl_obj_t fromSection,
                                                             no_vcl_obj_t toSection, no_vcl_bool_t* allow, void*)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender));
    if (!self || !self->onSectionDrag_)
        return;
    TSectionDragEvent handler = self->onSectionDrag_;
    bool allowDrag = *allow != 0;
    handler(self, THeaderSection::Wrap(fromSection), THeaderSection::Wrap(toSection), allowDrag);
    *allow = allowDrag ? 1 : 0;
}

void NO_VCL_CALL TCustomHeaderControl::SectionEndDragTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
        CallNotify(self->onSectionEndDrag_, self);
}

TCustomSectionNotifyEvent TCustomHeaderControl::GetOnSectionClickImpl(TObject* owner) { return static_cast<TCustomHeaderControl*>(owner)->onSectionClick_; }
TCustomSectionNotifyEvent TCustomHeaderControl::GetOnSectionResizeImpl(TObject* owner) { return static_cast<TCustomHeaderControl*>(owner)->onSectionResize_; }
TCustomSectionNotifyEvent TCustomHeaderControl::GetOnSectionSeparatorDblClickImpl(TObject* owner) { return static_cast<TCustomHeaderControl*>(owner)->onSectionSeparatorDblClick_; }
TCustomSectionTrackEvent  TCustomHeaderControl::GetOnSectionTrackImpl(TObject* owner) { return static_cast<TCustomHeaderControl*>(owner)->onSectionTrack_; }
TSectionDragEvent         TCustomHeaderControl::GetOnSectionDragImpl(TObject* owner) { return static_cast<TCustomHeaderControl*>(owner)->onSectionDrag_; }
TNotifyEvent              TCustomHeaderControl::GetOnSectionEndDragImpl(TObject* owner) { return static_cast<TCustomHeaderControl*>(owner)->onSectionEndDrag_; }

void TCustomHeaderControl::SetOnSectionClickImpl(TObject* owner, const TCustomSectionNotifyEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionClick_, self->onSectionClickHooked_, value,
                   &no_vcl_TCustomHeaderControl_SetOnSectionClick, &TCustomHeaderControl::SectionClickTrampoline);
}

void TCustomHeaderControl::SetOnSectionResizeImpl(TObject* owner, const TCustomSectionNotifyEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionResize_, self->onSectionResizeHooked_, value,
                   &no_vcl_TCustomHeaderControl_SetOnSectionResize, &TCustomHeaderControl::SectionResizeTrampoline);
}

void TCustomHeaderControl::SetOnSectionSeparatorDblClickImpl(TObject* owner, const TCustomSectionNotifyEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionSeparatorDblClick_, self->onSectionSeparatorDblClickHooked_, value,
                   &no_vcl_TCustomHeaderControl_SetOnSectionSeparatorDblClick, &TCustomHeaderControl::SectionSeparatorDblClickTrampoline);
}

void TCustomHeaderControl::SetOnSectionTrackImpl(TObject* owner, const TCustomSectionTrackEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionTrack_, self->onSectionTrackHooked_, value,
                   &no_vcl_TCustomHeaderControl_SetOnSectionTrack, &TCustomHeaderControl::SectionTrackTrampoline);
}

void TCustomHeaderControl::SetOnSectionDragImpl(TObject* owner, const TSectionDragEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionDrag_, self->onSectionDragHooked_, value,
                   &no_vcl_TCustomHeaderControl_SetOnSectionDrag, &TCustomHeaderControl::SectionDragTrampoline);
}

void TCustomHeaderControl::SetOnSectionEndDragImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionEndDrag_, self->onSectionEndDragHooked_, value,
                   &no_vcl_TCustomHeaderControl_SetOnSectionEndDrag, &TCustomHeaderControl::SectionEndDragTrampoline);
}

THeaderControl::THeaderControl(TComponent* AOwner)
    : TCustomHeaderControl(no_vcl_THeaderControl_Create(HandleOf(AOwner)))
{}

/* ---------------- ToolBar ---------------- */

TToolWindow::TToolWindow(no_vcl_obj_t handle)
    : TCustomControl(handle)
    , EdgeBorders(this, &TToolWindow::GetEdgeBordersImpl, &TToolWindow::SetEdgeBordersImpl)
    , EdgeInner(this, &TToolWindow::GetEdgeInnerImpl, &TToolWindow::SetEdgeInnerImpl)
    , EdgeOuter(this, &TToolWindow::GetEdgeOuterImpl, &TToolWindow::SetEdgeOuterImpl)
{}

void TToolWindow::BeginUpdate() { no_vcl_TToolWindow_BeginUpdate(handle_); }
void TToolWindow::EndUpdate()   { no_vcl_TToolWindow_EndUpdate(handle_); }
TEdgeBorders TToolWindow::GetEdgeBordersImpl(TObject* owner) { return no_vcl_TToolWindow_GetEdgeBorders(owner->Handle()); }
void TToolWindow::SetEdgeBordersImpl(TObject* owner, const TEdgeBorders& value) { no_vcl_TToolWindow_SetEdgeBorders(owner->Handle(), value); }
TEdgeStyle TToolWindow::GetEdgeInnerImpl(TObject* owner) { return static_cast<TEdgeStyle>(no_vcl_TToolWindow_GetEdgeInner(owner->Handle())); }
void TToolWindow::SetEdgeInnerImpl(TObject* owner, const TEdgeStyle& value) { no_vcl_TToolWindow_SetEdgeInner(owner->Handle(), value); }
TEdgeStyle TToolWindow::GetEdgeOuterImpl(TObject* owner) { return static_cast<TEdgeStyle>(no_vcl_TToolWindow_GetEdgeOuter(owner->Handle())); }
void TToolWindow::SetEdgeOuterImpl(TObject* owner, const TEdgeStyle& value) { no_vcl_TToolWindow_SetEdgeOuter(owner->Handle(), value); }

TToolBar::TToolBar(TComponent* AOwner)
    : TToolWindow(no_vcl_TToolBar_Create(HandleOf(AOwner)))
    , ButtonCount(this, &TToolBar::GetButtonCountImpl)
    , Buttons(this, &TToolBar::GetButtonsImpl)
    , RowCount(this, &TToolBar::GetRowCountImpl)
    , ButtonHeight(this, &TToolBar::GetButtonHeightImpl, &TToolBar::SetButtonHeightImpl)
    , ButtonWidth(this, &TToolBar::GetButtonWidthImpl, &TToolBar::SetButtonWidthImpl)
    , DropDownWidth(this, &TToolBar::GetDropDownWidthImpl, &TToolBar::SetDropDownWidthImpl)
    , Indent(this, &TToolBar::GetIndentImpl, &TToolBar::SetIndentImpl)
    , Flat(this, &TToolBar::GetFlatImpl, &TToolBar::SetFlatImpl)
    , List(this, &TToolBar::GetListImpl, &TToolBar::SetListImpl)
    , ShowCaptions(this, &TToolBar::GetShowCaptionsImpl, &TToolBar::SetShowCaptionsImpl)
    , Transparent(this, &TToolBar::GetTransparentImpl, &TToolBar::SetTransparentImpl)
    , Wrapable(this, &TToolBar::GetWrapableImpl, &TToolBar::SetWrapableImpl)
{}

void TToolBar::SetButtonSize(int NewButtonWidth, int NewButtonHeight)
{
    no_vcl_TToolBar_SetButtonSize(handle_, NewButtonWidth, NewButtonHeight);
}

int  TToolBar::GetButtonCountImpl(TObject* owner) { return no_vcl_TToolBar_GetButtonCount(owner->Handle()); }
// ボタンは利用者が生成したコンポーネントなので、ラッパーは必ずある。
TToolButton* TToolBar::GetButtonsImpl(TObject* owner, int Index)
{
    return static_cast<TToolButton*>(FromHandle(no_vcl_TToolBar_GetButton(owner->Handle(), Index)));
}
int  TToolBar::GetRowCountImpl(TObject* owner)                         { return no_vcl_TToolBar_GetRowCount(owner->Handle()); }
int  TToolBar::GetButtonHeightImpl(TObject* owner)                     { return no_vcl_TToolBar_GetButtonHeight(owner->Handle()); }
void TToolBar::SetButtonHeightImpl(TObject* owner, const int& value)    { no_vcl_TToolBar_SetButtonHeight(owner->Handle(), value); }
int  TToolBar::GetButtonWidthImpl(TObject* owner)                      { return no_vcl_TToolBar_GetButtonWidth(owner->Handle()); }
void TToolBar::SetButtonWidthImpl(TObject* owner, const int& value)     { no_vcl_TToolBar_SetButtonWidth(owner->Handle(), value); }
int  TToolBar::GetDropDownWidthImpl(TObject* owner)                    { return no_vcl_TToolBar_GetDropDownWidth(owner->Handle()); }
void TToolBar::SetDropDownWidthImpl(TObject* owner, const int& value)   { no_vcl_TToolBar_SetDropDownWidth(owner->Handle(), value); }
int  TToolBar::GetIndentImpl(TObject* owner)                           { return no_vcl_TToolBar_GetIndent(owner->Handle()); }
void TToolBar::SetIndentImpl(TObject* owner, const int& value)          { no_vcl_TToolBar_SetIndent(owner->Handle(), value); }
bool TToolBar::GetFlatImpl(TObject* owner)                             { return no_vcl_TToolBar_GetFlat(owner->Handle()) != 0; }
void TToolBar::SetFlatImpl(TObject* owner, const bool& value)           { no_vcl_TToolBar_SetFlat(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetListImpl(TObject* owner)                             { return no_vcl_TToolBar_GetList(owner->Handle()) != 0; }
void TToolBar::SetListImpl(TObject* owner, const bool& value)           { no_vcl_TToolBar_SetList(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetShowCaptionsImpl(TObject* owner)                     { return no_vcl_TToolBar_GetShowCaptions(owner->Handle()) != 0; }
void TToolBar::SetShowCaptionsImpl(TObject* owner, const bool& value)   { no_vcl_TToolBar_SetShowCaptions(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetTransparentImpl(TObject* owner)                      { return no_vcl_TToolBar_GetTransparent(owner->Handle()) != 0; }
void TToolBar::SetTransparentImpl(TObject* owner, const bool& value)    { no_vcl_TToolBar_SetTransparent(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetWrapableImpl(TObject* owner)                         { return no_vcl_TToolBar_GetWrapable(owner->Handle()) != 0; }
void TToolBar::SetWrapableImpl(TObject* owner, const bool& value)       { no_vcl_TToolBar_SetWrapable(owner->Handle(), value ? 1 : 0); }

TToolButton::TToolButton(TComponent* AOwner)
    : TGraphicControl(no_vcl_TToolButton_Create(HandleOf(AOwner)))
    , AllowAllUp(this, &TToolButton::GetAllowAllUpImpl, &TToolButton::SetAllowAllUpImpl)
    , Down(this, &TToolButton::GetDownImpl, &TToolButton::SetDownImpl)
    , Grouped(this, &TToolButton::GetGroupedImpl, &TToolButton::SetGroupedImpl)
    , Indeterminate(this, &TToolButton::GetIndeterminateImpl, &TToolButton::SetIndeterminateImpl)
    , Marked(this, &TToolButton::GetMarkedImpl, &TToolButton::SetMarkedImpl)
    , ShowCaption(this, &TToolButton::GetShowCaptionImpl, &TToolButton::SetShowCaptionImpl)
    , Wrap(this, &TToolButton::GetWrapImpl, &TToolButton::SetWrapImpl)
    , Style(this, &TToolButton::GetStyleImpl, &TToolButton::SetStyleImpl)
    , DropdownMenu(this, &TToolButton::GetDropdownMenuImpl, &TToolButton::SetDropdownMenuImpl)
    , MenuItem(this, &TToolButton::GetMenuItemImpl, &TToolButton::SetMenuItemImpl)
    , OnArrowClick(this, &TToolButton::GetOnArrowClickImpl, &TToolButton::SetOnArrowClickImpl)
    , Index(this, &TToolButton::GetIndexImpl)
{}

void TToolButton::Click()      { no_vcl_TToolButton_Click(handle_); }
void TToolButton::ArrowClick() { no_vcl_TToolButton_ArrowClick(handle_); }
bool TToolButton::PointInArrow(int X, int Y) const { return no_vcl_TToolButton_PointInArrow(handle_, X, Y) != 0; }

bool TToolButton::GetAllowAllUpImpl(TObject* owner)                      { return no_vcl_TToolButton_GetAllowAllUp(owner->Handle()) != 0; }
void TToolButton::SetAllowAllUpImpl(TObject* owner, const bool& value)    { no_vcl_TToolButton_SetAllowAllUp(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetDownImpl(TObject* owner)                            { return no_vcl_TToolButton_GetDown(owner->Handle()) != 0; }
void TToolButton::SetDownImpl(TObject* owner, const bool& value)          { no_vcl_TToolButton_SetDown(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetGroupedImpl(TObject* owner)                         { return no_vcl_TToolButton_GetGrouped(owner->Handle()) != 0; }
void TToolButton::SetGroupedImpl(TObject* owner, const bool& value)       { no_vcl_TToolButton_SetGrouped(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetIndeterminateImpl(TObject* owner)                   { return no_vcl_TToolButton_GetIndeterminate(owner->Handle()) != 0; }
void TToolButton::SetIndeterminateImpl(TObject* owner, const bool& value) { no_vcl_TToolButton_SetIndeterminate(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetMarkedImpl(TObject* owner)                          { return no_vcl_TToolButton_GetMarked(owner->Handle()) != 0; }
void TToolButton::SetMarkedImpl(TObject* owner, const bool& value)        { no_vcl_TToolButton_SetMarked(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetShowCaptionImpl(TObject* owner)                     { return no_vcl_TToolButton_GetShowCaption(owner->Handle()) != 0; }
void TToolButton::SetShowCaptionImpl(TObject* owner, const bool& value)   { no_vcl_TToolButton_SetShowCaption(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetWrapImpl(TObject* owner)                            { return no_vcl_TToolButton_GetWrap(owner->Handle()) != 0; }
void TToolButton::SetWrapImpl(TObject* owner, const bool& value)          { no_vcl_TToolButton_SetWrap(owner->Handle(), value ? 1 : 0); }
TToolButtonStyle TToolButton::GetStyleImpl(TObject* owner) { return static_cast<TToolButtonStyle>(no_vcl_TToolButton_GetStyle(owner->Handle())); }
void TToolButton::SetStyleImpl(TObject* owner, const TToolButtonStyle& value) { no_vcl_TToolButton_SetStyle(owner->Handle(), value); }
TPopupMenu* TToolButton::GetDropdownMenuImpl(TObject* owner)
{
    return static_cast<TPopupMenu*>(FromHandle(no_vcl_TToolButton_GetDropdownMenu(owner->Handle())));
}
void TToolButton::SetDropdownMenuImpl(TObject* owner, TPopupMenu* const& value) { no_vcl_TToolButton_SetDropdownMenu(owner->Handle(), HandleOf(value)); }
// メニュー項目は LCL が内部で生成したもの(メニューのルート項目等)もありうるため WrapExisting で引く。
TMenuItem* TToolButton::GetMenuItemImpl(TObject* owner) { return WrapExisting<TMenuItem>(no_vcl_TToolButton_GetMenuItem(owner->Handle())); }
void TToolButton::SetMenuItemImpl(TObject* owner, TMenuItem* const& value) { no_vcl_TToolButton_SetMenuItem(owner->Handle(), HandleOf(value)); }
int  TToolButton::GetIndexImpl(TObject* owner) { return no_vcl_TToolButton_GetIndex(owner->Handle()); }

void NO_VCL_CALL TToolButton::ArrowClickTrampoline(no_vcl_obj_t sender, void*)
{
    if (TToolButton* self = static_cast<TToolButton*>(FromHandle(sender)))
        CallNotify(self->onArrowClick_, self);
}

TNotifyEvent TToolButton::GetOnArrowClickImpl(TObject* owner) { return static_cast<TToolButton*>(owner)->onArrowClick_; }

void TToolButton::SetOnArrowClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TToolButton* self = static_cast<TToolButton*>(owner);
    SetSimpleEvent(self->handle_, self->onArrowClick_, self->onArrowClickHooked_, value,
                   &no_vcl_TToolButton_SetOnArrowClick, &TToolButton::ArrowClickTrampoline);
}

/* ---------------- CoolBar ---------------- */

TCoolBand::TCoolBand(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Text(this, &TCoolBand::GetTextImpl, &TCoolBand::SetTextImpl)
    , Width(this, &TCoolBand::GetWidthImpl, &TCoolBand::SetWidthImpl)
    , MinWidth(this, &TCoolBand::GetMinWidthImpl, &TCoolBand::SetMinWidthImpl)
    , MinHeight(this, &TCoolBand::GetMinHeightImpl, &TCoolBand::SetMinHeightImpl)
    , Break(this, &TCoolBand::GetBreakImpl, &TCoolBand::SetBreakImpl)
    , Visible(this, &TCoolBand::GetVisibleImpl, &TCoolBand::SetVisibleImpl)
    , FixedSize(this, &TCoolBand::GetFixedSizeImpl, &TCoolBand::SetFixedSizeImpl)
    , FixedBackground(this, &TCoolBand::GetFixedBackgroundImpl, &TCoolBand::SetFixedBackgroundImpl)
    , HorizontalOnly(this, &TCoolBand::GetHorizontalOnlyImpl, &TCoolBand::SetHorizontalOnlyImpl)
    , Color(this, &TCoolBand::GetColorImpl, &TCoolBand::SetColorImpl)
    , ParentColor(this, &TCoolBand::GetParentColorImpl, &TCoolBand::SetParentColorImpl)
    , Index(this, &TCoolBand::GetIndexImpl, &TCoolBand::SetIndexImpl)
    , Control(this, &TCoolBand::GetControlImpl, &TCoolBand::SetControlImpl)
    , Left(this, &TCoolBand::GetLeftImpl)
    , Top(this, &TCoolBand::GetTopImpl)
    , Right(this, &TCoolBand::GetRightImpl)
    , Height(this, &TCoolBand::GetHeightImpl)
{}

void TCoolBand::AutosizeWidth() { no_vcl_TCoolBand_AutosizeWidth(handle_); }

std::string TCoolBand::GetTextImpl(TObject* owner) { return std::string(no_vcl_TCoolBand_GetText(owner->Handle())); }
void TCoolBand::SetTextImpl(TObject* owner, const std::string& value) { no_vcl_TCoolBand_SetText(owner->Handle(), value.c_str()); }
int  TCoolBand::GetWidthImpl(TObject* owner)                            { return no_vcl_TCoolBand_GetWidth(owner->Handle()); }
void TCoolBand::SetWidthImpl(TObject* owner, const int& value)           { no_vcl_TCoolBand_SetWidth(owner->Handle(), value); }
int  TCoolBand::GetMinWidthImpl(TObject* owner)                         { return no_vcl_TCoolBand_GetMinWidth(owner->Handle()); }
void TCoolBand::SetMinWidthImpl(TObject* owner, const int& value)        { no_vcl_TCoolBand_SetMinWidth(owner->Handle(), value); }
int  TCoolBand::GetMinHeightImpl(TObject* owner)                        { return no_vcl_TCoolBand_GetMinHeight(owner->Handle()); }
void TCoolBand::SetMinHeightImpl(TObject* owner, const int& value)       { no_vcl_TCoolBand_SetMinHeight(owner->Handle(), value); }
bool TCoolBand::GetBreakImpl(TObject* owner)                            { return no_vcl_TCoolBand_GetBreak(owner->Handle()) != 0; }
void TCoolBand::SetBreakImpl(TObject* owner, const bool& value)          { no_vcl_TCoolBand_SetBreak(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetVisibleImpl(TObject* owner)                          { return no_vcl_TCoolBand_GetVisible(owner->Handle()) != 0; }
void TCoolBand::SetVisibleImpl(TObject* owner, const bool& value)        { no_vcl_TCoolBand_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetFixedSizeImpl(TObject* owner)                        { return no_vcl_TCoolBand_GetFixedSize(owner->Handle()) != 0; }
void TCoolBand::SetFixedSizeImpl(TObject* owner, const bool& value)      { no_vcl_TCoolBand_SetFixedSize(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetFixedBackgroundImpl(TObject* owner)                  { return no_vcl_TCoolBand_GetFixedBackground(owner->Handle()) != 0; }
void TCoolBand::SetFixedBackgroundImpl(TObject* owner, const bool& value) { no_vcl_TCoolBand_SetFixedBackground(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetHorizontalOnlyImpl(TObject* owner)                   { return no_vcl_TCoolBand_GetHorizontalOnly(owner->Handle()) != 0; }
void TCoolBand::SetHorizontalOnlyImpl(TObject* owner, const bool& value) { no_vcl_TCoolBand_SetHorizontalOnly(owner->Handle(), value ? 1 : 0); }
TColor TCoolBand::GetColorImpl(TObject* owner)                          { return static_cast<TColor>(no_vcl_TCoolBand_GetColor(owner->Handle())); }
void TCoolBand::SetColorImpl(TObject* owner, const TColor& value)        { no_vcl_TCoolBand_SetColor(owner->Handle(), static_cast<no_vcl_int_t>(value)); }
bool TCoolBand::GetParentColorImpl(TObject* owner)                      { return no_vcl_TCoolBand_GetParentColor(owner->Handle()) != 0; }
void TCoolBand::SetParentColorImpl(TObject* owner, const bool& value)    { no_vcl_TCoolBand_SetParentColor(owner->Handle(), value ? 1 : 0); }
int  TCoolBand::GetIndexImpl(TObject* owner)                            { return no_vcl_TCoolBand_GetIndex(owner->Handle()); }
void TCoolBand::SetIndexImpl(TObject* owner, const int& value)           { no_vcl_TCoolBand_SetIndex(owner->Handle(), value); }
// バンドに置くコントロールは利用者が生成したコンポーネントなので、ラッパーは必ずある。
TControl* TCoolBand::GetControlImpl(TObject* owner)
{
    return static_cast<TControl*>(TControl::FromHandle(no_vcl_TCoolBand_GetControl(owner->Handle())));
}
void TCoolBand::SetControlImpl(TObject* owner, TControl* const& value)   { no_vcl_TCoolBand_SetControl(owner->Handle(), value ? value->Handle() : nullptr); }
int  TCoolBand::GetLeftImpl(TObject* owner)                             { return no_vcl_TCoolBand_GetLeft(owner->Handle()); }
int  TCoolBand::GetTopImpl(TObject* owner)                              { return no_vcl_TCoolBand_GetTop(owner->Handle()); }
int  TCoolBand::GetRightImpl(TObject* owner)                            { return no_vcl_TCoolBand_GetRight(owner->Handle()); }
int  TCoolBand::GetHeightImpl(TObject* owner)                           { return no_vcl_TCoolBand_GetHeight(owner->Handle()); }

TCoolBands::TCoolBands(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &TCoolBands::GetCountImpl)
    , Items(this, &TCoolBands::GetItemsImpl)
{}

TCoolBand* TCoolBands::Add()          { return TCoolBand::Wrap(no_vcl_TCoolBands_Add(handle_)); }
void TCoolBands::Delete(int Index)    { no_vcl_TCoolBands_Delete(handle_, Index); }
void TCoolBands::Clear()              { no_vcl_TCoolBands_Clear(handle_); }
void TCoolBands::BeginUpdate()        { no_vcl_TCoolBands_BeginUpdate(handle_); }
void TCoolBands::EndUpdate()          { no_vcl_TCoolBands_EndUpdate(handle_); }
TCoolBand* TCoolBands::FindBand(TControl* AControl) const { return TCoolBand::Wrap(no_vcl_TCoolBands_FindBand(handle_, AControl ? AControl->Handle() : nullptr)); }
int  TCoolBands::FindBandIndex(TControl* AControl) const  { return no_vcl_TCoolBands_FindBandIndex(handle_, AControl ? AControl->Handle() : nullptr); }
TCoolBand* TCoolBands::GetItemsImpl(TObject* owner, int Index) { return TCoolBand::Wrap(no_vcl_TCoolBands_GetItem(owner->Handle(), Index)); }
int  TCoolBands::GetCountImpl(TObject* owner) { return no_vcl_TCoolBands_GetCount(owner->Handle()); }

TCustomCoolBar::TCustomCoolBar(no_vcl_obj_t handle)
    : TToolWindow(handle)
    , Bands(this, &TCustomCoolBar::GetBandsImpl)
    , FixedSize(this, &TCustomCoolBar::GetFixedSizeImpl, &TCustomCoolBar::SetFixedSizeImpl)
    , FixedOrder(this, &TCustomCoolBar::GetFixedOrderImpl, &TCustomCoolBar::SetFixedOrderImpl)
    , GrabStyle(this, &TCustomCoolBar::GetGrabStyleImpl, &TCustomCoolBar::SetGrabStyleImpl)
    , GrabWidth(this, &TCustomCoolBar::GetGrabWidthImpl, &TCustomCoolBar::SetGrabWidthImpl)
    , HorizontalSpacing(this, &TCustomCoolBar::GetHorizontalSpacingImpl, &TCustomCoolBar::SetHorizontalSpacingImpl)
    , VerticalSpacing(this, &TCustomCoolBar::GetVerticalSpacingImpl, &TCustomCoolBar::SetVerticalSpacingImpl)
    , ShowText(this, &TCustomCoolBar::GetShowTextImpl, &TCustomCoolBar::SetShowTextImpl)
    , Themed(this, &TCustomCoolBar::GetThemedImpl, &TCustomCoolBar::SetThemedImpl)
    , Vertical(this, &TCustomCoolBar::GetVerticalImpl, &TCustomCoolBar::SetVerticalImpl)
    , OnChange(this, &TCustomCoolBar::GetOnChangeImpl, &TCustomCoolBar::SetOnChangeImpl)
    , bands_(no_vcl_TCustomCoolBar_GetBands(handle_))
{}

void TCustomCoolBar::AutosizeBands() { no_vcl_TCustomCoolBar_AutosizeBands(handle_); }

void TCustomCoolBar::MouseToBandPos(int X, int Y, int& ABand, bool& AGrabber) const
{
    no_vcl_int_t band = -1;
    no_vcl_bool_t grabber = 0;
    no_vcl_TCustomCoolBar_MouseToBandPos(handle_, X, Y, &band, &grabber);
    ABand = band;
    AGrabber = grabber != 0;
}

TCoolBands* TCustomCoolBar::GetBandsImpl(TObject* owner) { return &static_cast<TCustomCoolBar*>(owner)->bands_; }
bool TCustomCoolBar::GetFixedSizeImpl(TObject* owner)                        { return no_vcl_TCustomCoolBar_GetFixedSize(owner->Handle()) != 0; }
void TCustomCoolBar::SetFixedSizeImpl(TObject* owner, const bool& value)      { no_vcl_TCustomCoolBar_SetFixedSize(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetFixedOrderImpl(TObject* owner)                       { return no_vcl_TCustomCoolBar_GetFixedOrder(owner->Handle()) != 0; }
void TCustomCoolBar::SetFixedOrderImpl(TObject* owner, const bool& value)     { no_vcl_TCustomCoolBar_SetFixedOrder(owner->Handle(), value ? 1 : 0); }
TGrabStyle TCustomCoolBar::GetGrabStyleImpl(TObject* owner) { return static_cast<TGrabStyle>(no_vcl_TCustomCoolBar_GetGrabStyle(owner->Handle())); }
void TCustomCoolBar::SetGrabStyleImpl(TObject* owner, const TGrabStyle& value) { no_vcl_TCustomCoolBar_SetGrabStyle(owner->Handle(), value); }
int  TCustomCoolBar::GetGrabWidthImpl(TObject* owner)                        { return no_vcl_TCustomCoolBar_GetGrabWidth(owner->Handle()); }
void TCustomCoolBar::SetGrabWidthImpl(TObject* owner, const int& value)       { no_vcl_TCustomCoolBar_SetGrabWidth(owner->Handle(), value); }
int  TCustomCoolBar::GetHorizontalSpacingImpl(TObject* owner)                { return no_vcl_TCustomCoolBar_GetHorizontalSpacing(owner->Handle()); }
void TCustomCoolBar::SetHorizontalSpacingImpl(TObject* owner, const int& value) { no_vcl_TCustomCoolBar_SetHorizontalSpacing(owner->Handle(), value); }
int  TCustomCoolBar::GetVerticalSpacingImpl(TObject* owner)                  { return no_vcl_TCustomCoolBar_GetVerticalSpacing(owner->Handle()); }
void TCustomCoolBar::SetVerticalSpacingImpl(TObject* owner, const int& value) { no_vcl_TCustomCoolBar_SetVerticalSpacing(owner->Handle(), value); }
bool TCustomCoolBar::GetShowTextImpl(TObject* owner)                         { return no_vcl_TCustomCoolBar_GetShowText(owner->Handle()) != 0; }
void TCustomCoolBar::SetShowTextImpl(TObject* owner, const bool& value)       { no_vcl_TCustomCoolBar_SetShowText(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetThemedImpl(TObject* owner)                           { return no_vcl_TCustomCoolBar_GetThemed(owner->Handle()) != 0; }
void TCustomCoolBar::SetThemedImpl(TObject* owner, const bool& value)         { no_vcl_TCustomCoolBar_SetThemed(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetVerticalImpl(TObject* owner)                         { return no_vcl_TCustomCoolBar_GetVertical(owner->Handle()) != 0; }
void TCustomCoolBar::SetVerticalImpl(TObject* owner, const bool& value)       { no_vcl_TCustomCoolBar_SetVertical(owner->Handle(), value ? 1 : 0); }

void NO_VCL_CALL TCustomCoolBar::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    if (TCustomCoolBar* self = static_cast<TCustomCoolBar*>(FromHandle(sender)))
        CallNotify(self->onChange_, self);
}

TNotifyEvent TCustomCoolBar::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomCoolBar*>(owner)->onChange_; }

void TCustomCoolBar::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomCoolBar* self = static_cast<TCustomCoolBar*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TCustomCoolBar_SetOnChange, &TCustomCoolBar::ChangeTrampoline);
}

TCoolBar::TCoolBar(TComponent* AOwner)
    : TCustomCoolBar(no_vcl_TCoolBar_Create(HandleOf(AOwner)))
{}

} // namespace no_vcl
