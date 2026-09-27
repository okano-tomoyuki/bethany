#include "no_vcl.hpp"

#include <cstdlib>

#include "no_vcl_funcs.h"

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

// 直前のエラー(no_vcl_c.h)があれば、Exception として送出する(docs/adr/0031)。
void ThrowIfLastError()
{
    if (!no_vcl_HasLastError())
        return;
    Exception e(no_vcl_GetLastErrorClassName(), no_vcl_GetLastErrorMessage());
    no_vcl_ClearLastError();
    throw e;
}

template<typename R>
struct Checked
{
    template<typename F>
    static R Call(F f)
    {
        R result = f();
        ThrowIfLastError();
        return result;
    }
};

template<>
struct Checked<void>
{
    template<typename F>
    static void Call(F f)
    {
        f();
        ThrowIfLastError();
    }
};

// DLL から呼ばれるコールバック(トランポリン)の本体を包む。ハンドラから送出された例外は DLL の関数をまたいで伝えられないため、
// ここで捕まえて no_vcl_SetCallbackError で知らせ、DLL 側で送出し直させる(docs/adr/0031)。
// 正常に戻ったときは、ハンドラの中の呼び出しが残した直前のエラーをクリアする(外側の呼び出しの失敗と取り違えないため)。
template<typename F>
void GuardCallback(F f)
{
    try
    {
        f();
    }
    catch (const Exception& e)
    {
        no_vcl_SetCallbackError(e.ClassName().c_str(), e.Message.c_str());
        return;
    }
    catch (const std::exception& e)
    {
        no_vcl_SetCallbackError("std::exception", e.what());
        return;
    }
    catch (...)
    {
        no_vcl_SetCallbackError("", "unknown C++ exception");
        return;
    }
    no_vcl_ClearLastError();
}

} // namespace

// C API の各関数を、呼び出しの後に直前のエラーを確かめる版にしたもの(no_vcl_funcs.h の一覧から生成する)。
// このファイルでは C API を nv:: 経由で呼ぶ。生の C API を使うのは、例外を送出してはならないデストラクタと、
// TStrings 等の取得関数(Accessor)として関数ポインタを渡す箇所だけ。
namespace nv
{
#define NO_VCL_CHECKED(ret, name, params, args) \
    inline ret name params { return Checked<ret>::Call([&]() { return ::no_vcl_##name args; }); }
NO_VCL_FUNCS(NO_VCL_CHECKED)
#undef NO_VCL_CHECKED
} // namespace nv

namespace
{

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
    , Names(this, &TStrings::GetNamesImpl)
    , Values(this, &TStrings::GetValuesImpl, &TStrings::SetValuesImpl)
    , ValueFromIndex(this, &TStrings::GetValueFromIndexImpl, &TStrings::SetValueFromIndexImpl)
    , Delimiter(this, &TStrings::GetDelimiterImpl, &TStrings::SetDelimiterImpl)
    , StrictDelimiter(this, &TStrings::GetStrictDelimiterImpl, &TStrings::SetStrictDelimiterImpl)
    , DelimitedText(this, &TStrings::GetDelimitedTextImpl, &TStrings::SetDelimitedTextImpl)
    , owner_(owner)
    , accessor_(accessor)
{}

// TStringList は自分のハンドルを持ち、Current() はそれをそのまま返す(所有者が自分、取得関数が恒等関数)。
TStrings::TStrings(no_vcl_obj_t handle)
    : TStrings(this, &TStrings::SelfAccessor)
{
    handle_ = handle;
}

int  TStrings::Add(const std::string& S)                     { return nv::TStrings_Add(Current(), S.c_str()); }
int  TStrings::AddObject(const std::string& S, void* AObject) { return nv::TStrings_AddObject(Current(), S.c_str(), AObject); }
void TStrings::Insert(int Index, const std::string& S)       { nv::TStrings_Insert(Current(), Index, S.c_str()); }
void TStrings::Delete(int Index)                             { nv::TStrings_Delete(Current(), Index); }
void TStrings::Clear()                                       { nv::TStrings_Clear(Current()); }
int  TStrings::IndexOf(const std::string& S) const           { return nv::TStrings_IndexOf(Current(), S.c_str()); }
void TStrings::Exchange(int Index1, int Index2)              { nv::TStrings_Exchange(Current(), Index1, Index2); }
void TStrings::Move(int CurIndex, int NewIndex)              { nv::TStrings_Move(Current(), CurIndex, NewIndex); }
void TStrings::BeginUpdate()                                 { nv::TStrings_BeginUpdate(Current()); }
void TStrings::EndUpdate()                                   { nv::TStrings_EndUpdate(Current()); }
void TStrings::Assign(const TStrings* Source)                { nv::TStrings_Assign(Current(), Source ? Source->Current() : nullptr); }
void TStrings::AddStrings(const TStrings* Source)            { if (Source) nv::TStrings_AddStrings(Current(), Source->Current()); }
int  TStrings::IndexOfName(const std::string& Name) const     { return nv::TStrings_IndexOfName(Current(), Name.c_str()); }
void TStrings::LoadFromFile(const std::string& FileName)     { nv::TStrings_LoadFromFile(Current(), FileName.c_str()); }
void TStrings::SaveToFile(const std::string& FileName) const { nv::TStrings_SaveToFile(Current(), FileName.c_str()); }

int TStrings::GetCountImpl(TObject* owner) { return nv::TStrings_GetCount(static_cast<TStrings*>(owner)->Current()); }
std::string TStrings::GetStringsImpl(TObject* owner, int Index)
{
    return std::string(nv::TStrings_GetStrings(static_cast<TStrings*>(owner)->Current(), Index));
}
void TStrings::SetStringsImpl(TObject* owner, int Index, const std::string& value)
{
    nv::TStrings_SetStrings(static_cast<TStrings*>(owner)->Current(), Index, value.c_str());
}
void* TStrings::GetObjectsImpl(TObject* owner, int Index) { return nv::TStrings_GetObjects(static_cast<TStrings*>(owner)->Current(), Index); }
void TStrings::SetObjectsImpl(TObject* owner, int Index, void* const& value)
{
    nv::TStrings_SetObjects(static_cast<TStrings*>(owner)->Current(), Index, value);
}
std::string TStrings::GetTextImpl(TObject* owner) { return std::string(nv::TStrings_GetText(static_cast<TStrings*>(owner)->Current())); }
void TStrings::SetTextImpl(TObject* owner, const std::string& value) { nv::TStrings_SetText(static_cast<TStrings*>(owner)->Current(), value.c_str()); }
std::string TStrings::GetCommaTextImpl(TObject* owner) { return std::string(nv::TStrings_GetCommaText(static_cast<TStrings*>(owner)->Current())); }
void TStrings::SetCommaTextImpl(TObject* owner, const std::string& value) { nv::TStrings_SetCommaText(static_cast<TStrings*>(owner)->Current(), value.c_str()); }
std::string TStrings::GetNamesImpl(TObject* owner, int Index)
{
    return std::string(nv::TStrings_GetNames(static_cast<TStrings*>(owner)->Current(), Index));
}
std::string TStrings::GetValuesImpl(TObject* owner, std::string Name)
{
    return std::string(nv::TStrings_GetValues(static_cast<TStrings*>(owner)->Current(), Name.c_str()));
}
void TStrings::SetValuesImpl(TObject* owner, std::string Name, const std::string& value)
{
    nv::TStrings_SetValues(static_cast<TStrings*>(owner)->Current(), Name.c_str(), value.c_str());
}
std::string TStrings::GetValueFromIndexImpl(TObject* owner, int Index)
{
    return std::string(nv::TStrings_GetValueFromIndex(static_cast<TStrings*>(owner)->Current(), Index));
}
void TStrings::SetValueFromIndexImpl(TObject* owner, int Index, const std::string& value)
{
    nv::TStrings_SetValueFromIndex(static_cast<TStrings*>(owner)->Current(), Index, value.c_str());
}
char TStrings::GetDelimiterImpl(TObject* owner) { return nv::TStrings_GetDelimiter(static_cast<TStrings*>(owner)->Current()); }
void TStrings::SetDelimiterImpl(TObject* owner, const char& value) { nv::TStrings_SetDelimiter(static_cast<TStrings*>(owner)->Current(), value); }
bool TStrings::GetStrictDelimiterImpl(TObject* owner) { return nv::TStrings_GetStrictDelimiter(static_cast<TStrings*>(owner)->Current()) != 0; }
void TStrings::SetStrictDelimiterImpl(TObject* owner, const bool& value)
{
    nv::TStrings_SetStrictDelimiter(static_cast<TStrings*>(owner)->Current(), value ? 1 : 0);
}
std::string TStrings::GetDelimitedTextImpl(TObject* owner) { return std::string(nv::TStrings_GetDelimitedText(static_cast<TStrings*>(owner)->Current())); }
void TStrings::SetDelimitedTextImpl(TObject* owner, const std::string& value)
{
    nv::TStrings_SetDelimitedText(static_cast<TStrings*>(owner)->Current(), value.c_str());
}

/* ---------------- TStringList ---------------- */

TStringList::TStringList()
    : TStrings(nv::TStringList_Create())
    , Sorted(this, &TStringList::GetSortedImpl, &TStringList::SetSortedImpl)
    , Duplicates(this, &TStringList::GetDuplicatesImpl, &TStringList::SetDuplicatesImpl)
    , CaseSensitive(this, &TStringList::GetCaseSensitiveImpl, &TStringList::SetCaseSensitiveImpl)
{}

TStringList::~TStringList()
{
    no_vcl_TStringList_Destroy(handle_);
}

void TStringList::Sort() { nv::TStringList_Sort(handle_); }

bool TStringList::Find(const std::string& S, int& Index) const
{
    no_vcl_int_t index = -1;
    bool found = nv::TStringList_Find(handle_, S.c_str(), &index) != 0;
    Index = index;
    return found;
}

bool TStringList::GetSortedImpl(TObject* owner) { return nv::TStringList_GetSorted(owner->Handle()) != 0; }
void TStringList::SetSortedImpl(TObject* owner, const bool& value) { nv::TStringList_SetSorted(owner->Handle(), value ? 1 : 0); }
TDuplicates TStringList::GetDuplicatesImpl(TObject* owner) { return static_cast<TDuplicates>(nv::TStringList_GetDuplicates(owner->Handle())); }
void TStringList::SetDuplicatesImpl(TObject* owner, const TDuplicates& value) { nv::TStringList_SetDuplicates(owner->Handle(), value); }
bool TStringList::GetCaseSensitiveImpl(TObject* owner) { return nv::TStringList_GetCaseSensitive(owner->Handle()) != 0; }
void TStringList::SetCaseSensitiveImpl(TObject* owner, const bool& value) { nv::TStringList_SetCaseSensitive(owner->Handle(), value ? 1 : 0); }

/* ---------------- TComponent ---------------- */

TComponent::TComponent(no_vcl_obj_t handle)
    : TPersistent(handle)
{
    static bool callbackInstalled = false;
    if (!callbackInstalled)
    {
        nv::FreeNotify_SetCallback(&TComponent::FreeNotifyTrampoline, nullptr);
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
    nv::TComponent_Destroy(handle_);
}

void NO_VCL_CALL TComponent::FreeNotifyTrampoline(no_vcl_obj_t handle, void*)
{
    GuardCallback([&] {
        std::unordered_map<no_vcl_obj_t, TComponent*>& registry = Registry();
        auto it = registry.find(handle);
        if (it == registry.end())
            return;

        TComponent* self = it->second;
        registry.erase(it);
        self->freedByLcl_ = true;
        delete self;
    });
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
    , AutoSize(this, &TControl::GetAutoSizeImpl, &TControl::SetAutoSizeImpl)
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

void TControl::Show() { nv::TControl_Show(handle_); }
void TControl::Hide() { nv::TControl_Hide(handle_); }

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
        nv::TControl_SetOnClick(self->handle_, &TControl::ClickTrampoline, nullptr);
        self->onClickHooked_ = true;
    }
}

void NO_VCL_CALL TControl::ClickTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        TControl* self = static_cast<TControl*>(FromHandle(sender));
        if (!self || !self->onClick_)
            return;
        TNotifyEvent handler = self->onClick_;
        handler(self);
    });
}

TWinControl* TControl::GetParentImpl(TObject* owner)
{
    // LCL の Parent は常に TWinControl 派生で、C++ 側の階層も LCL の部分列なので、
    // そのハンドルに登録されたラッパーは TWinControl 派生であることが保証される。
    // C++ ラッパーを介さずに作られた親の場合は nullptr になる。
    return static_cast<TWinControl*>(FromHandle(nv::TControl_GetParent(owner->Handle())));
}

void TControl::SetParentImpl(TObject* owner, TWinControl* const& value)
{
    nv::TControl_SetParent(owner->Handle(), HandleOf(value));
}

int  TControl::GetLeftImpl(TObject* owner)                      { return nv::TControl_GetLeft(owner->Handle()); }
void TControl::SetLeftImpl(TObject* owner, const int& value)    { nv::TControl_SetLeft(owner->Handle(), value); }
int  TControl::GetTopImpl(TObject* owner)                       { return nv::TControl_GetTop(owner->Handle()); }
void TControl::SetTopImpl(TObject* owner, const int& value)     { nv::TControl_SetTop(owner->Handle(), value); }
int  TControl::GetWidthImpl(TObject* owner)                     { return nv::TControl_GetWidth(owner->Handle()); }
void TControl::SetWidthImpl(TObject* owner, const int& value)   { nv::TControl_SetWidth(owner->Handle(), value); }
int  TControl::GetHeightImpl(TObject* owner)                    { return nv::TControl_GetHeight(owner->Handle()); }
void TControl::SetHeightImpl(TObject* owner, const int& value)  { nv::TControl_SetHeight(owner->Handle(), value); }
bool TControl::GetVisibleImpl(TObject* owner)                   { return nv::TControl_GetVisible(owner->Handle()) != 0; }
void TControl::SetVisibleImpl(TObject* owner, const bool& value){ nv::TControl_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TControl::GetEnabledImpl(TObject* owner)                   { return nv::TControl_GetEnabled(owner->Handle()) != 0; }
void TControl::SetEnabledImpl(TObject* owner, const bool& value){ nv::TControl_SetEnabled(owner->Handle(), value ? 1 : 0); }

std::string TControl::GetCaptionImpl(TObject* owner)
{
    return std::string(nv::TControl_GetCaption(owner->Handle()));
}

void TControl::SetCaptionImpl(TObject* owner, const std::string& value)
{
    nv::TControl_SetCaption(owner->Handle(), value.c_str());
}

TAlign TControl::GetAlignImpl(TObject* owner)                  { return static_cast<TAlign>(nv::TControl_GetAlign(owner->Handle())); }
void   TControl::SetAlignImpl(TObject* owner, const TAlign& value) { nv::TControl_SetAlign(owner->Handle(), value); }
bool   TControl::GetAutoSizeImpl(TObject* owner)                  { return nv::TControl_GetAutoSize(owner->Handle()) != 0; }
void   TControl::SetAutoSizeImpl(TObject* owner, const bool& value) { nv::TControl_SetAutoSize(owner->Handle(), value ? 1 : 0); }

TPopupMenu* TControl::GetPopupMenuImpl(TObject* owner)
{
    return static_cast<TPopupMenu*>(FromHandle(nv::TControl_GetPopupMenu(owner->Handle())));
}

void TControl::SetPopupMenuImpl(TObject* owner, TPopupMenu* const& value)
{
    nv::TControl_SetPopupMenu(owner->Handle(), HandleOf(value));
}

std::string TControl::GetTextImpl(TObject* owner)
{
    return std::string(nv::TControl_GetText(owner->Handle()));
}

void TControl::SetTextImpl(TObject* owner, const std::string& value)
{
    nv::TControl_SetText(owner->Handle(), value.c_str());
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
        nv::TControl_SetOnMouseDown(self->handle_, &TControl::MouseDownTrampoline, nullptr);
        self->onMouseDownHooked_ = true;
    }
}

void TControl::SetOnMouseUpImpl(TObject* owner, const TMouseEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseUp_ = value;
    if (value && !self->onMouseUpHooked_)
    {
        nv::TControl_SetOnMouseUp(self->handle_, &TControl::MouseUpTrampoline, nullptr);
        self->onMouseUpHooked_ = true;
    }
}

void TControl::SetOnMouseMoveImpl(TObject* owner, const TMouseMoveEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseMove_ = value;
    if (value && !self->onMouseMoveHooked_)
    {
        nv::TControl_SetOnMouseMove(self->handle_, &TControl::MouseMoveTrampoline, nullptr);
        self->onMouseMoveHooked_ = true;
    }
}

void TControl::SetOnMouseWheelImpl(TObject* owner, const TMouseWheelEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseWheel_ = value;
    if (value && !self->onMouseWheelHooked_)
    {
        nv::TControl_SetOnMouseWheel(self->handle_, &TControl::MouseWheelTrampoline, nullptr);
        self->onMouseWheelHooked_ = true;
    }
}

void NO_VCL_CALL TControl::DblClickTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
            CallNotify(self->onDblClick_, self);
    });
}

void NO_VCL_CALL TControl::ResizeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
            CallNotify(self->onResize_, self);
    });
}

void NO_VCL_CALL TControl::MouseEnterTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
            CallNotify(self->onMouseEnter_, self);
    });
}

void NO_VCL_CALL TControl::MouseLeaveTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
            CallNotify(self->onMouseLeave_, self);
    });
}

void NO_VCL_CALL TControl::MouseDownTrampoline(no_vcl_obj_t sender, no_vcl_int_t button, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void*)
{
    GuardCallback([&] {
        TControl* self = static_cast<TControl*>(FromHandle(sender));
        if (!self || !self->onMouseDown_)
            return;
        TMouseEvent handler = self->onMouseDown_;
        handler(self, static_cast<TMouseButton>(button), static_cast<TShiftState>(shift), x, y);
    });
}

void NO_VCL_CALL TControl::MouseUpTrampoline(no_vcl_obj_t sender, no_vcl_int_t button, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void*)
{
    GuardCallback([&] {
        TControl* self = static_cast<TControl*>(FromHandle(sender));
        if (!self || !self->onMouseUp_)
            return;
        TMouseEvent handler = self->onMouseUp_;
        handler(self, static_cast<TMouseButton>(button), static_cast<TShiftState>(shift), x, y);
    });
}

void NO_VCL_CALL TControl::MouseMoveTrampoline(no_vcl_obj_t sender, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void*)
{
    GuardCallback([&] {
        TControl* self = static_cast<TControl*>(FromHandle(sender));
        if (!self || !self->onMouseMove_)
            return;
        TMouseMoveEvent handler = self->onMouseMove_;
        handler(self, static_cast<TShiftState>(shift), x, y);
    });
}

void NO_VCL_CALL TControl::MouseWheelTrampoline(no_vcl_obj_t sender, no_vcl_int_t shift, no_vcl_int_t wheelDelta, no_vcl_int_t x, no_vcl_int_t y, no_vcl_bool_t* handled, void*)
{
    GuardCallback([&] {
        TControl* self = static_cast<TControl*>(FromHandle(sender));
        if (!self || !self->onMouseWheel_)
            return;
        TMouseWheelEvent handler = self->onMouseWheel_;
        bool handledValue = *handled != 0;
        handler(self, static_cast<TShiftState>(shift), wheelDelta, x, y, handledValue);
        *handled = handledValue ? 1 : 0;
    });
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
        nv::TWinControl_SetOnKeyDown(self->handle_, &TWinControl::KeyDownTrampoline, nullptr);
        self->onKeyDownHooked_ = true;
    }
}

void TWinControl::SetOnKeyUpImpl(TObject* owner, const TKeyEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onKeyUp_ = value;
    if (value && !self->onKeyUpHooked_)
    {
        nv::TWinControl_SetOnKeyUp(self->handle_, &TWinControl::KeyUpTrampoline, nullptr);
        self->onKeyUpHooked_ = true;
    }
}

void TWinControl::SetOnKeyPressImpl(TObject* owner, const TKeyPressEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onKeyPress_ = value;
    if (value && !self->onKeyPressHooked_)
    {
        nv::TWinControl_SetOnKeyPress(self->handle_, &TWinControl::KeyPressTrampoline, nullptr);
        self->onKeyPressHooked_ = true;
    }
}

void NO_VCL_CALL TWinControl::KeyDownTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, no_vcl_int_t shift, void*)
{
    GuardCallback([&] {
        TWinControl* self = static_cast<TWinControl*>(FromHandle(sender));
        if (!self || !self->onKeyDown_)
            return;
        TKeyEvent handler = self->onKeyDown_;
        int keyValue = *key;
        handler(self, keyValue, static_cast<TShiftState>(shift));
        *key = keyValue;
    });
}

void NO_VCL_CALL TWinControl::KeyUpTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, no_vcl_int_t shift, void*)
{
    GuardCallback([&] {
        TWinControl* self = static_cast<TWinControl*>(FromHandle(sender));
        if (!self || !self->onKeyUp_)
            return;
        TKeyEvent handler = self->onKeyUp_;
        int keyValue = *key;
        handler(self, keyValue, static_cast<TShiftState>(shift));
        *key = keyValue;
    });
}

void NO_VCL_CALL TWinControl::KeyPressTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, void*)
{
    GuardCallback([&] {
        TWinControl* self = static_cast<TWinControl*>(FromHandle(sender));
        if (!self || !self->onKeyPress_)
            return;
        TKeyPressEvent handler = self->onKeyPress_;
        char keyValue = static_cast<char>(*key);
        handler(self, keyValue);
        *key = static_cast<unsigned char>(keyValue);
    });
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

TScrollBarKind TCustomScrollBar::GetKindImpl(TObject* owner) { return static_cast<TScrollBarKind>(nv::TCustomScrollBar_GetKind(owner->Handle())); }
void TCustomScrollBar::SetKindImpl(TObject* owner, const TScrollBarKind& value) { nv::TCustomScrollBar_SetKind(owner->Handle(), value); }
int  TCustomScrollBar::GetMinImpl(TObject* owner)      { return nv::TCustomScrollBar_GetMin(owner->Handle()); }
void TCustomScrollBar::SetMinImpl(TObject* owner, const int& value)      { nv::TCustomScrollBar_SetMin(owner->Handle(), value); }
int  TCustomScrollBar::GetMaxImpl(TObject* owner)      { return nv::TCustomScrollBar_GetMax(owner->Handle()); }
void TCustomScrollBar::SetMaxImpl(TObject* owner, const int& value)      { nv::TCustomScrollBar_SetMax(owner->Handle(), value); }
int  TCustomScrollBar::GetPositionImpl(TObject* owner) { return nv::TCustomScrollBar_GetPosition(owner->Handle()); }
void TCustomScrollBar::SetPositionImpl(TObject* owner, const int& value) { nv::TCustomScrollBar_SetPosition(owner->Handle(), value); }
int  TCustomScrollBar::GetPageSizeImpl(TObject* owner) { return nv::TCustomScrollBar_GetPageSize(owner->Handle()); }
void TCustomScrollBar::SetPageSizeImpl(TObject* owner, const int& value) { nv::TCustomScrollBar_SetPageSize(owner->Handle(), value); }
TNotifyEvent TCustomScrollBar::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomScrollBar*>(owner)->onChange_; }

void TCustomScrollBar::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomScrollBar* self = static_cast<TCustomScrollBar*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TCustomScrollBar_SetOnChange, &TCustomScrollBar::ChangeTrampoline);
}

void NO_VCL_CALL TCustomScrollBar::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomScrollBar* self = static_cast<TCustomScrollBar*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TScrollBar::TScrollBar(TComponent* AOwner)
    : TCustomScrollBar(nv::TScrollBar_Create(HandleOf(AOwner)))
{}

TCustomTrackBar::TCustomTrackBar(no_vcl_obj_t handle)
    : TWinControl(handle)
    , Min(this, &TCustomTrackBar::GetMinImpl, &TCustomTrackBar::SetMinImpl)
    , Max(this, &TCustomTrackBar::GetMaxImpl, &TCustomTrackBar::SetMaxImpl)
    , Position(this, &TCustomTrackBar::GetPositionImpl, &TCustomTrackBar::SetPositionImpl)
    , OnChange(this, &TCustomTrackBar::GetOnChangeImpl, &TCustomTrackBar::SetOnChangeImpl)
{}

int  TCustomTrackBar::GetMinImpl(TObject* owner)      { return nv::TCustomTrackBar_GetMin(owner->Handle()); }
void TCustomTrackBar::SetMinImpl(TObject* owner, const int& value)      { nv::TCustomTrackBar_SetMin(owner->Handle(), value); }
int  TCustomTrackBar::GetMaxImpl(TObject* owner)      { return nv::TCustomTrackBar_GetMax(owner->Handle()); }
void TCustomTrackBar::SetMaxImpl(TObject* owner, const int& value)      { nv::TCustomTrackBar_SetMax(owner->Handle(), value); }
int  TCustomTrackBar::GetPositionImpl(TObject* owner) { return nv::TCustomTrackBar_GetPosition(owner->Handle()); }
void TCustomTrackBar::SetPositionImpl(TObject* owner, const int& value) { nv::TCustomTrackBar_SetPosition(owner->Handle(), value); }
TNotifyEvent TCustomTrackBar::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomTrackBar*>(owner)->onChange_; }

void TCustomTrackBar::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomTrackBar* self = static_cast<TCustomTrackBar*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TCustomTrackBar_SetOnChange, &TCustomTrackBar::ChangeTrampoline);
}

void NO_VCL_CALL TCustomTrackBar::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomTrackBar* self = static_cast<TCustomTrackBar*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TTrackBar::TTrackBar(TComponent* AOwner)
    : TCustomTrackBar(nv::TTrackBar_Create(HandleOf(AOwner)))
{}

TCustomProgressBar::TCustomProgressBar(no_vcl_obj_t handle)
    : TWinControl(handle)
    , Min(this, &TCustomProgressBar::GetMinImpl, &TCustomProgressBar::SetMinImpl)
    , Max(this, &TCustomProgressBar::GetMaxImpl, &TCustomProgressBar::SetMaxImpl)
    , Position(this, &TCustomProgressBar::GetPositionImpl, &TCustomProgressBar::SetPositionImpl)
{}

int  TCustomProgressBar::GetMinImpl(TObject* owner)      { return nv::TCustomProgressBar_GetMin(owner->Handle()); }
void TCustomProgressBar::SetMinImpl(TObject* owner, const int& value)      { nv::TCustomProgressBar_SetMin(owner->Handle(), value); }
int  TCustomProgressBar::GetMaxImpl(TObject* owner)      { return nv::TCustomProgressBar_GetMax(owner->Handle()); }
void TCustomProgressBar::SetMaxImpl(TObject* owner, const int& value)      { nv::TCustomProgressBar_SetMax(owner->Handle(), value); }
int  TCustomProgressBar::GetPositionImpl(TObject* owner) { return nv::TCustomProgressBar_GetPosition(owner->Handle()); }
void TCustomProgressBar::SetPositionImpl(TObject* owner, const int& value) { nv::TCustomProgressBar_SetPosition(owner->Handle(), value); }

TProgressBar::TProgressBar(TComponent* AOwner)
    : TCustomProgressBar(nv::TProgressBar_Create(HandleOf(AOwner)))
{}

TScrollBox::TScrollBox(TComponent* AOwner)
    : TScrollingWinControl(nv::TScrollBox_Create(HandleOf(AOwner)))
{}

TUpDown::TUpDown(TComponent* AOwner)
    : TCustomControl(nv::TUpDown_Create(HandleOf(AOwner)))
    , Min(this, &TUpDown::GetMinImpl, &TUpDown::SetMinImpl)
    , Max(this, &TUpDown::GetMaxImpl, &TUpDown::SetMaxImpl)
    , Position(this, &TUpDown::GetPositionImpl, &TUpDown::SetPositionImpl)
    , Increment(this, &TUpDown::GetIncrementImpl, &TUpDown::SetIncrementImpl)
    , Associate(this, &TUpDown::GetAssociateImpl, &TUpDown::SetAssociateImpl)
{}

int  TUpDown::GetMinImpl(TObject* owner)       { return nv::TUpDown_GetMin(owner->Handle()); }
void TUpDown::SetMinImpl(TObject* owner, const int& value)       { nv::TUpDown_SetMin(owner->Handle(), value); }
int  TUpDown::GetMaxImpl(TObject* owner)       { return nv::TUpDown_GetMax(owner->Handle()); }
void TUpDown::SetMaxImpl(TObject* owner, const int& value)       { nv::TUpDown_SetMax(owner->Handle(), value); }
int  TUpDown::GetPositionImpl(TObject* owner)  { return nv::TUpDown_GetPosition(owner->Handle()); }
void TUpDown::SetPositionImpl(TObject* owner, const int& value)  { nv::TUpDown_SetPosition(owner->Handle(), value); }
int  TUpDown::GetIncrementImpl(TObject* owner) { return nv::TUpDown_GetIncrement(owner->Handle()); }
void TUpDown::SetIncrementImpl(TObject* owner, const int& value) { nv::TUpDown_SetIncrement(owner->Handle(), value); }

TWinControl* TUpDown::GetAssociateImpl(TObject* owner)
{
    return static_cast<TWinControl*>(FromHandle(nv::TUpDown_GetAssociate(owner->Handle())));
}

void TUpDown::SetAssociateImpl(TObject* owner, TWinControl* const& value)
{
    nv::TUpDown_SetAssociate(owner->Handle(), HandleOf(value));
}

/* ---------------- Form ---------------- */

void TCustomForm::Show()      { nv::TCustomForm_Show(handle_); }
void TCustomForm::Hide()      { nv::TCustomForm_Hide(handle_); }
int  TCustomForm::ShowModal() { return nv::TCustomForm_ShowModal(handle_); }
void TCustomForm::Close()     { nv::TCustomForm_Close(handle_); }
void TCustomForm::Release()   { nv::TCustomForm_Release(handle_); }

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
    nv::TCustomForm_SetOnShow(handle_, &TCustomForm::ShowTrampoline, nullptr);
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
    GuardCallback([&] {
        TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender));
        if (!self)
            return;
        self->DoCreate();
        TNotifyEvent handler = self->onShow_;
        if (handler)
            handler(self);
    });
}

void NO_VCL_CALL TCustomForm::CloseTrampoline(no_vcl_obj_t sender, no_vcl_int_t* action, void*)
{
    GuardCallback([&] {
        TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender));
        if (!self || !self->onClose_)
            return;
        TCloseEvent handler = self->onClose_;
        TCloseAction value = static_cast<TCloseAction>(*action);
        handler(self, value);
        *action = static_cast<no_vcl_int_t>(value);
    });
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
        nv::TCustomForm_SetOnClose(self->handle_, &TCustomForm::CloseTrampoline, nullptr);
        self->onCloseHooked_ = true;
    }
}

void NO_VCL_CALL TCustomForm::HideTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
            CallNotify(self->onHide_, self);
    });
}

void NO_VCL_CALL TCustomForm::ActivateTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
            CallNotify(self->onActivate_, self);
    });
}

void NO_VCL_CALL TCustomForm::DeactivateTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
            CallNotify(self->onDeactivate_, self);
    });
}

void NO_VCL_CALL TCustomForm::DestroyTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
            CallNotify(self->onDestroy_, self);
    });
}

void NO_VCL_CALL TCustomForm::CloseQueryTrampoline(no_vcl_obj_t sender, no_vcl_bool_t* canClose, void*)
{
    GuardCallback([&] {
        TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender));
        if (!self || !self->onCloseQuery_)
            return;
        TCloseQueryEvent handler = self->onCloseQuery_;
        bool value = *canClose != 0;
        handler(self, value);
        *canClose = value ? 1 : 0;
    });
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
    return static_cast<TMainMenu*>(FromHandle(nv::TCustomForm_GetMenu(owner->Handle())));
}

void TCustomForm::SetMenuImpl(TObject* owner, TMainMenu* const& value)
{
    nv::TCustomForm_SetMenu(owner->Handle(), HandleOf(value));
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
    return nv::TForm_Create(HandleOf(AOwner));
}

/* ---------------- TApplication ---------------- */

TApplication* NewApplication()
{
    return new TApplication(nv::GetApplication());
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
        nv::TComponent_DestroyComponents(Application->handle_);
    nv::FreeNotify_SetCallback(nullptr, nullptr);
    nv::ItemFree_SetCallback(nullptr, nullptr);
}

void TApplication::BeginCreateForm()
{
    // 前回のハンドルが引き取られていなければ破棄する(T のコンストラクタが Application 以外を Owner にした場合)。
    if (TForm::pendingHandle_)
        nv::TComponent_Destroy(TForm::pendingHandle_);
    TForm::pendingHandle_ = nv::TApplication_CreateForm(handle_);
}

void TApplication::EndCreateForm()
{
    if (TForm::pendingHandle_)
    {
        nv::TComponent_Destroy(TForm::pendingHandle_);
        TForm::pendingHandle_ = nullptr;
    }
}

void TApplication::Run()             { nv::TApplication_Run(handle_); }
void TApplication::ProcessMessages() { nv::TApplication_ProcessMessages(handle_); }
void TApplication::Terminate()       { nv::TApplication_Terminate(handle_); }

TForm* TApplication::GetMainFormImpl(TObject* owner)
{
    return dynamic_cast<TForm*>(FromHandle(nv::TApplication_GetMainForm(owner->Handle())));
}

bool TApplication::GetTerminatedImpl(TObject* owner)
{
    return nv::TApplication_GetTerminated(owner->Handle()) != 0;
}

std::string TApplication::GetTitleImpl(TObject* owner)
{
    return std::string(nv::TApplication_GetTitle(owner->Handle()));
}

void TApplication::SetTitleImpl(TObject* owner, const std::string& value)
{
    nv::TApplication_SetTitle(owner->Handle(), value.c_str());
}

bool TApplication::GetShowMainFormImpl(TObject* owner)
{
    return nv::TApplication_GetShowMainForm(owner->Handle()) != 0;
}

void TApplication::SetShowMainFormImpl(TObject* owner, const bool& value)
{
    nv::TApplication_SetShowMainForm(owner->Handle(), value ? 1 : 0);
}

/* ---------------- Panel / GroupBox / Label ---------------- */

TPanel::TPanel(TComponent* AOwner)
    : TCustomPanel(nv::TPanel_Create(HandleOf(AOwner)))
{}

TGroupBox::TGroupBox(TComponent* AOwner)
    : TCustomGroupBox(nv::TGroupBox_Create(HandleOf(AOwner)))
{}

TCustomRadioGroup::TCustomRadioGroup(no_vcl_obj_t handle)
    : TCustomGroupBox(handle)
    , ItemIndex(this, &TCustomRadioGroup::GetItemIndexImpl, &TCustomRadioGroup::SetItemIndexImpl)
    , OnClick(this, &TCustomRadioGroup::GetOnClickImpl, &TCustomRadioGroup::SetOnClickImpl)
    , Items(this, &TCustomRadioGroup::GetItemsImpl)
    , items_(this, &no_vcl_TCustomRadioGroup_GetItems)
{}

TStrings* TCustomRadioGroup::GetItemsImpl(TObject* owner) { return &static_cast<TCustomRadioGroup*>(owner)->items_; }

int  TCustomRadioGroup::GetItemIndexImpl(TObject* owner)                   { return nv::TCustomRadioGroup_GetItemIndex(owner->Handle()); }
void TCustomRadioGroup::SetItemIndexImpl(TObject* owner, const int& value) { nv::TCustomRadioGroup_SetItemIndex(owner->Handle(), value); }
TNotifyEvent TCustomRadioGroup::GetOnClickImpl(TObject* owner) { return static_cast<TCustomRadioGroup*>(owner)->onClick_; }

void TCustomRadioGroup::SetOnClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomRadioGroup* self = static_cast<TCustomRadioGroup*>(owner);
    SetSimpleEvent(self->handle_, self->onClick_, self->onClickHooked_, value,
                   &no_vcl_TCustomRadioGroup_SetOnClick, &TCustomRadioGroup::ClickTrampoline);
}

void NO_VCL_CALL TCustomRadioGroup::ClickTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomRadioGroup* self = static_cast<TCustomRadioGroup*>(FromHandle(sender)))
            CallNotify(self->onClick_, self);
    });
}

TRadioGroup::TRadioGroup(TComponent* AOwner)
    : TCustomRadioGroup(nv::TRadioGroup_Create(HandleOf(AOwner)))
{}

TStrings* TCustomCheckGroup::GetItemsImpl(TObject* owner) { return &static_cast<TCustomCheckGroup*>(owner)->items_; }

TCustomCheckGroup::TCustomCheckGroup(no_vcl_obj_t handle)
    : TCustomGroupBox(handle)
    , Checked(this, &TCustomCheckGroup::GetCheckedImpl, &TCustomCheckGroup::SetCheckedImpl)
    , Items(this, &TCustomCheckGroup::GetItemsImpl)
    , items_(this, &no_vcl_TCustomCheckGroup_GetItems)
{}

bool TCustomCheckGroup::GetCheckedImpl(TObject* owner, int index)                     { return nv::TCustomCheckGroup_GetChecked(owner->Handle(), index) != 0; }
void TCustomCheckGroup::SetCheckedImpl(TObject* owner, int index, const bool& value)  { nv::TCustomCheckGroup_SetChecked(owner->Handle(), index, value ? 1 : 0); }

TCheckGroup::TCheckGroup(TComponent* AOwner)
    : TCustomCheckGroup(nv::TCheckGroup_Create(HandleOf(AOwner)))
{}

TLabel::TLabel(TComponent* AOwner)
    : TCustomLabel(nv::TLabel_Create(HandleOf(AOwner)))
{}

TBevel::TBevel(TComponent* AOwner)
    : TGraphicControl(nv::TBevel_Create(HandleOf(AOwner)))
    , Shape(this, &TBevel::GetShapeImpl, &TBevel::SetShapeImpl)
    , Style(this, &TBevel::GetStyleImpl, &TBevel::SetStyleImpl)
{}

TBevelShape TBevel::GetShapeImpl(TObject* owner) { return static_cast<TBevelShape>(nv::TBevel_GetShape(owner->Handle())); }
void TBevel::SetShapeImpl(TObject* owner, const TBevelShape& value) { nv::TBevel_SetShape(owner->Handle(), value); }
TBevelStyle TBevel::GetStyleImpl(TObject* owner) { return static_cast<TBevelStyle>(nv::TBevel_GetStyle(owner->Handle())); }
void TBevel::SetStyleImpl(TObject* owner, const TBevelStyle& value) { nv::TBevel_SetStyle(owner->Handle(), value); }

TCustomShape::TCustomShape(no_vcl_obj_t handle)
    : TGraphicControl(handle)
    , Pen(nv::TCustomShape_GetPen(handle))
    , Brush(nv::TCustomShape_GetBrush(handle))
    , Shape(this, &TCustomShape::GetShapeImpl, &TCustomShape::SetShapeImpl)
{}

TShapeType TCustomShape::GetShapeImpl(TObject* owner) { return static_cast<TShapeType>(nv::TCustomShape_GetShape(owner->Handle())); }
void TCustomShape::SetShapeImpl(TObject* owner, const TShapeType& value) { nv::TCustomShape_SetShape(owner->Handle(), value); }

TShape::TShape(TComponent* AOwner)
    : TCustomShape(nv::TShape_Create(HandleOf(AOwner)))
{}

TCustomSpeedButton::TCustomSpeedButton(no_vcl_obj_t handle)
    : TGraphicControl(handle)
    , Down(this, &TCustomSpeedButton::GetDownImpl, &TCustomSpeedButton::SetDownImpl)
    , GroupIndex(this, &TCustomSpeedButton::GetGroupIndexImpl, &TCustomSpeedButton::SetGroupIndexImpl)
    , Flat(this, &TCustomSpeedButton::GetFlatImpl, &TCustomSpeedButton::SetFlatImpl)
    , AllowAllUp(this, &TCustomSpeedButton::GetAllowAllUpImpl, &TCustomSpeedButton::SetAllowAllUpImpl)
    , Glyph(this, &TCustomSpeedButton::GetGlyphImpl, &TCustomSpeedButton::SetGlyphImpl)
    , NumGlyphs(this, &TCustomSpeedButton::GetNumGlyphsImpl, &TCustomSpeedButton::SetNumGlyphsImpl)
    , Layout(this, &TCustomSpeedButton::GetLayoutImpl, &TCustomSpeedButton::SetLayoutImpl)
    , Margin(this, &TCustomSpeedButton::GetMarginImpl, &TCustomSpeedButton::SetMarginImpl)
    , Spacing(this, &TCustomSpeedButton::GetSpacingImpl, &TCustomSpeedButton::SetSpacingImpl)
    , Images(this, &TCustomSpeedButton::GetImagesImpl, &TCustomSpeedButton::SetImagesImpl)
    , ImageIndex(this, &TCustomSpeedButton::GetImageIndexImpl, &TCustomSpeedButton::SetImageIndexImpl)
    , glyph_(this, &no_vcl_TCustomSpeedButton_GetGlyph)
{}

TBitmap* TCustomSpeedButton::GetGlyphImpl(TObject* owner) { return &static_cast<TCustomSpeedButton*>(owner)->glyph_; }
void TCustomSpeedButton::SetGlyphImpl(TObject* owner, TBitmap* const& value)
{
    nv::TCustomSpeedButton_SetGlyph(owner->Handle(), value ? value->Current() : nullptr);
}
int  TCustomSpeedButton::GetNumGlyphsImpl(TObject* owner) { return nv::TCustomSpeedButton_GetNumGlyphs(owner->Handle()); }
void TCustomSpeedButton::SetNumGlyphsImpl(TObject* owner, const int& value) { nv::TCustomSpeedButton_SetNumGlyphs(owner->Handle(), value); }
TButtonLayout TCustomSpeedButton::GetLayoutImpl(TObject* owner)
{
    return static_cast<TButtonLayout>(nv::TCustomSpeedButton_GetLayout(owner->Handle()));
}
void TCustomSpeedButton::SetLayoutImpl(TObject* owner, const TButtonLayout& value) { nv::TCustomSpeedButton_SetLayout(owner->Handle(), value); }
int  TCustomSpeedButton::GetMarginImpl(TObject* owner)    { return nv::TCustomSpeedButton_GetMargin(owner->Handle()); }
void TCustomSpeedButton::SetMarginImpl(TObject* owner, const int& value) { nv::TCustomSpeedButton_SetMargin(owner->Handle(), value); }
int  TCustomSpeedButton::GetSpacingImpl(TObject* owner)   { return nv::TCustomSpeedButton_GetSpacing(owner->Handle()); }
void TCustomSpeedButton::SetSpacingImpl(TObject* owner, const int& value) { nv::TCustomSpeedButton_SetSpacing(owner->Handle(), value); }

bool TCustomSpeedButton::GetDownImpl(TObject* owner)       { return nv::TCustomSpeedButton_GetDown(owner->Handle()) != 0; }
void TCustomSpeedButton::SetDownImpl(TObject* owner, const bool& value)       { nv::TCustomSpeedButton_SetDown(owner->Handle(), value ? 1 : 0); }
int  TCustomSpeedButton::GetGroupIndexImpl(TObject* owner) { return nv::TCustomSpeedButton_GetGroupIndex(owner->Handle()); }
void TCustomSpeedButton::SetGroupIndexImpl(TObject* owner, const int& value) { nv::TCustomSpeedButton_SetGroupIndex(owner->Handle(), value); }
bool TCustomSpeedButton::GetFlatImpl(TObject* owner)       { return nv::TCustomSpeedButton_GetFlat(owner->Handle()) != 0; }
void TCustomSpeedButton::SetFlatImpl(TObject* owner, const bool& value)       { nv::TCustomSpeedButton_SetFlat(owner->Handle(), value ? 1 : 0); }
bool TCustomSpeedButton::GetAllowAllUpImpl(TObject* owner) { return nv::TCustomSpeedButton_GetAllowAllUp(owner->Handle()) != 0; }
void TCustomSpeedButton::SetAllowAllUpImpl(TObject* owner, const bool& value) { nv::TCustomSpeedButton_SetAllowAllUp(owner->Handle(), value ? 1 : 0); }

TSpeedButton::TSpeedButton(TComponent* AOwner)
    : TCustomSpeedButton(nv::TSpeedButton_Create(HandleOf(AOwner)))
{}

/* ---------------- Button / CheckBox / RadioButton ---------------- */

TButtonControl::TButtonControl(no_vcl_obj_t handle)
    : TWinControl(handle)
    , Checked(this, &TButtonControl::GetCheckedImpl, &TButtonControl::SetCheckedImpl)
{}

bool TButtonControl::GetCheckedImpl(TObject* owner)
{
    return nv::TButtonControl_GetChecked(owner->Handle()) != 0;
}

void TButtonControl::SetCheckedImpl(TObject* owner, const bool& value)
{
    nv::TButtonControl_SetChecked(owner->Handle(), value ? 1 : 0);
}

TButton::TButton(TComponent* AOwner)
    : TCustomButton(nv::TButton_Create(HandleOf(AOwner)))
{}

TCustomBitBtn::TCustomBitBtn(no_vcl_obj_t handle)
    : TCustomButton(handle)
    , Kind(this, &TCustomBitBtn::GetKindImpl, &TCustomBitBtn::SetKindImpl)
    , Glyph(this, &TCustomBitBtn::GetGlyphImpl, &TCustomBitBtn::SetGlyphImpl)
    , NumGlyphs(this, &TCustomBitBtn::GetNumGlyphsImpl, &TCustomBitBtn::SetNumGlyphsImpl)
    , Layout(this, &TCustomBitBtn::GetLayoutImpl, &TCustomBitBtn::SetLayoutImpl)
    , Margin(this, &TCustomBitBtn::GetMarginImpl, &TCustomBitBtn::SetMarginImpl)
    , Spacing(this, &TCustomBitBtn::GetSpacingImpl, &TCustomBitBtn::SetSpacingImpl)
    , Images(this, &TCustomBitBtn::GetImagesImpl, &TCustomBitBtn::SetImagesImpl)
    , ImageIndex(this, &TCustomBitBtn::GetImageIndexImpl, &TCustomBitBtn::SetImageIndexImpl)
    , glyph_(this, &no_vcl_TCustomBitBtn_GetGlyph)
{}

TBitBtnKind TCustomBitBtn::GetKindImpl(TObject* owner) { return static_cast<TBitBtnKind>(nv::TCustomBitBtn_GetKind(owner->Handle())); }
void TCustomBitBtn::SetKindImpl(TObject* owner, const TBitBtnKind& value) { nv::TCustomBitBtn_SetKind(owner->Handle(), value); }
TBitmap* TCustomBitBtn::GetGlyphImpl(TObject* owner) { return &static_cast<TCustomBitBtn*>(owner)->glyph_; }
void TCustomBitBtn::SetGlyphImpl(TObject* owner, TBitmap* const& value)
{
    nv::TCustomBitBtn_SetGlyph(owner->Handle(), value ? value->Current() : nullptr);
}
int  TCustomBitBtn::GetNumGlyphsImpl(TObject* owner) { return nv::TCustomBitBtn_GetNumGlyphs(owner->Handle()); }
void TCustomBitBtn::SetNumGlyphsImpl(TObject* owner, const int& value) { nv::TCustomBitBtn_SetNumGlyphs(owner->Handle(), value); }
TButtonLayout TCustomBitBtn::GetLayoutImpl(TObject* owner) { return static_cast<TButtonLayout>(nv::TCustomBitBtn_GetLayout(owner->Handle())); }
void TCustomBitBtn::SetLayoutImpl(TObject* owner, const TButtonLayout& value) { nv::TCustomBitBtn_SetLayout(owner->Handle(), value); }
int  TCustomBitBtn::GetMarginImpl(TObject* owner)    { return nv::TCustomBitBtn_GetMargin(owner->Handle()); }
void TCustomBitBtn::SetMarginImpl(TObject* owner, const int& value) { nv::TCustomBitBtn_SetMargin(owner->Handle(), value); }
int  TCustomBitBtn::GetSpacingImpl(TObject* owner)   { return nv::TCustomBitBtn_GetSpacing(owner->Handle()); }
void TCustomBitBtn::SetSpacingImpl(TObject* owner, const int& value) { nv::TCustomBitBtn_SetSpacing(owner->Handle(), value); }

TBitBtn::TBitBtn(TComponent* AOwner)
    : TCustomBitBtn(nv::TBitBtn_Create(HandleOf(AOwner)))
{}

TCheckBox::TCheckBox(TComponent* AOwner)
    : TCustomCheckBox(nv::TCheckBox_Create(HandleOf(AOwner)))
{}

TRadioButton::TRadioButton(TComponent* AOwner)
    : TCustomCheckBox(nv::TRadioButton_Create(HandleOf(AOwner)))
{}

TToggleBox::TToggleBox(TComponent* AOwner)
    : TCustomCheckBox(nv::TToggleBox_Create(HandleOf(AOwner)))
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
        nv::TCustomEdit_SetOnChange(self->handle_, &TCustomEdit::ChangeTrampoline, nullptr);
        self->onChangeHooked_ = true;
    }
}

void NO_VCL_CALL TCustomEdit::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        TCustomEdit* self = static_cast<TCustomEdit*>(FromHandle(sender));
        if (!self || !self->onChange_)
            return;
        TNotifyEvent handler = self->onChange_;
        handler(self);
    });
}

int  TCustomEdit::GetMaxLengthImpl(TObject* owner)                   { return nv::TCustomEdit_GetMaxLength(owner->Handle()); }
void TCustomEdit::SetMaxLengthImpl(TObject* owner, const int& value) { nv::TCustomEdit_SetMaxLength(owner->Handle(), value); }
bool TCustomEdit::GetReadOnlyImpl(TObject* owner)                    { return nv::TCustomEdit_GetReadOnly(owner->Handle()) != 0; }
void TCustomEdit::SetReadOnlyImpl(TObject* owner, const bool& value) { nv::TCustomEdit_SetReadOnly(owner->Handle(), value ? 1 : 0); }

TEdit::TEdit(TComponent* AOwner)
    : TCustomEdit(nv::TEdit_Create(HandleOf(AOwner)))
{}

TCustomFloatSpinEdit::TCustomFloatSpinEdit(no_vcl_obj_t handle)
    : TCustomEdit(handle)
    , Value(this, &TCustomFloatSpinEdit::GetValueImpl, &TCustomFloatSpinEdit::SetValueImpl)
    , MinValue(this, &TCustomFloatSpinEdit::GetMinValueImpl, &TCustomFloatSpinEdit::SetMinValueImpl)
    , MaxValue(this, &TCustomFloatSpinEdit::GetMaxValueImpl, &TCustomFloatSpinEdit::SetMaxValueImpl)
    , Increment(this, &TCustomFloatSpinEdit::GetIncrementImpl, &TCustomFloatSpinEdit::SetIncrementImpl)
    , DecimalPlaces(this, &TCustomFloatSpinEdit::GetDecimalPlacesImpl, &TCustomFloatSpinEdit::SetDecimalPlacesImpl)
{}

double TCustomFloatSpinEdit::GetValueImpl(TObject* owner)     { return nv::TCustomFloatSpinEdit_GetValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetValueImpl(TObject* owner, const double& value)     { nv::TCustomFloatSpinEdit_SetValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetMinValueImpl(TObject* owner)  { return nv::TCustomFloatSpinEdit_GetMinValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetMinValueImpl(TObject* owner, const double& value)  { nv::TCustomFloatSpinEdit_SetMinValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetMaxValueImpl(TObject* owner)  { return nv::TCustomFloatSpinEdit_GetMaxValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetMaxValueImpl(TObject* owner, const double& value)  { nv::TCustomFloatSpinEdit_SetMaxValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetIncrementImpl(TObject* owner) { return nv::TCustomFloatSpinEdit_GetIncrement(owner->Handle()); }
void   TCustomFloatSpinEdit::SetIncrementImpl(TObject* owner, const double& value) { nv::TCustomFloatSpinEdit_SetIncrement(owner->Handle(), value); }
int    TCustomFloatSpinEdit::GetDecimalPlacesImpl(TObject* owner) { return nv::TCustomFloatSpinEdit_GetDecimalPlaces(owner->Handle()); }
void   TCustomFloatSpinEdit::SetDecimalPlacesImpl(TObject* owner, const int& value) { nv::TCustomFloatSpinEdit_SetDecimalPlaces(owner->Handle(), value); }

TFloatSpinEdit::TFloatSpinEdit(TComponent* AOwner)
    : TCustomFloatSpinEdit(nv::TFloatSpinEdit_Create(HandleOf(AOwner)))
{}

TCustomSpinEdit::TCustomSpinEdit(no_vcl_obj_t handle)
    : TCustomFloatSpinEdit(handle)
    , Value(this, &TCustomSpinEdit::GetValueImpl, &TCustomSpinEdit::SetValueImpl)
    , MinValue(this, &TCustomSpinEdit::GetMinValueImpl, &TCustomSpinEdit::SetMinValueImpl)
    , MaxValue(this, &TCustomSpinEdit::GetMaxValueImpl, &TCustomSpinEdit::SetMaxValueImpl)
    , Increment(this, &TCustomSpinEdit::GetIncrementImpl, &TCustomSpinEdit::SetIncrementImpl)
{}

int  TCustomSpinEdit::GetValueImpl(TObject* owner)     { return nv::TCustomSpinEdit_GetValue(owner->Handle()); }
void TCustomSpinEdit::SetValueImpl(TObject* owner, const int& value)     { nv::TCustomSpinEdit_SetValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetMinValueImpl(TObject* owner)  { return nv::TCustomSpinEdit_GetMinValue(owner->Handle()); }
void TCustomSpinEdit::SetMinValueImpl(TObject* owner, const int& value)  { nv::TCustomSpinEdit_SetMinValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetMaxValueImpl(TObject* owner)  { return nv::TCustomSpinEdit_GetMaxValue(owner->Handle()); }
void TCustomSpinEdit::SetMaxValueImpl(TObject* owner, const int& value)  { nv::TCustomSpinEdit_SetMaxValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetIncrementImpl(TObject* owner) { return nv::TCustomSpinEdit_GetIncrement(owner->Handle()); }
void TCustomSpinEdit::SetIncrementImpl(TObject* owner, const int& value) { nv::TCustomSpinEdit_SetIncrement(owner->Handle(), value); }

TSpinEdit::TSpinEdit(TComponent* AOwner)
    : TCustomSpinEdit(nv::TSpinEdit_Create(HandleOf(AOwner)))
{}

TMaskEdit::TMaskEdit(TComponent* AOwner)
    : TCustomEdit(nv::TMaskEdit_Create(HandleOf(AOwner)))
    , EditMask(this, &TMaskEdit::GetEditMaskImpl, &TMaskEdit::SetEditMaskImpl)
{}

std::string TMaskEdit::GetEditMaskImpl(TObject* owner) { return std::string(nv::TMaskEdit_GetEditMask(owner->Handle())); }
void TMaskEdit::SetEditMaskImpl(TObject* owner, const std::string& value) { nv::TMaskEdit_SetEditMask(owner->Handle(), value.c_str()); }

TCustomLabeledEdit::TCustomLabeledEdit(no_vcl_obj_t handle)
    : TCustomEdit(handle)
    , EditLabel(this, &TCustomLabeledEdit::GetEditLabelImpl)
    , LabelPosition(this, &TCustomLabeledEdit::GetLabelPositionImpl, &TCustomLabeledEdit::SetLabelPositionImpl)
    , LabelSpacing(this, &TCustomLabeledEdit::GetLabelSpacingImpl, &TCustomLabeledEdit::SetLabelSpacingImpl)
{}

TBoundLabel* TCustomLabeledEdit::GetEditLabelImpl(TObject* owner)
{
    return WrapExisting<TBoundLabel>(nv::TCustomLabeledEdit_GetEditLabel(owner->Handle()));
}
TLabelPosition TCustomLabeledEdit::GetLabelPositionImpl(TObject* owner)
{
    return static_cast<TLabelPosition>(nv::TCustomLabeledEdit_GetLabelPosition(owner->Handle()));
}
void TCustomLabeledEdit::SetLabelPositionImpl(TObject* owner, const TLabelPosition& value)
{
    nv::TCustomLabeledEdit_SetLabelPosition(owner->Handle(), value);
}
int  TCustomLabeledEdit::GetLabelSpacingImpl(TObject* owner) { return nv::TCustomLabeledEdit_GetLabelSpacing(owner->Handle()); }
void TCustomLabeledEdit::SetLabelSpacingImpl(TObject* owner, const int& value) { nv::TCustomLabeledEdit_SetLabelSpacing(owner->Handle(), value); }

TLabeledEdit::TLabeledEdit(TComponent* AOwner)
    : TCustomLabeledEdit(nv::TLabeledEdit_Create(HandleOf(AOwner)))
{}

TTabControl::TTabControl(TComponent* AOwner)
    : TCustomTabControl(nv::TTabControl_Create(HandleOf(AOwner)))
    , TabIndex(this, &TTabControl::GetTabIndexImpl, &TTabControl::SetTabIndexImpl)
    , OnChange(this, &TTabControl::GetOnChangeImpl, &TTabControl::SetOnChangeImpl)
    , Tabs(this, &TTabControl::GetTabsImpl)
    , tabs_(this, &no_vcl_TTabControl_GetTabs)
{}

TStrings* TTabControl::GetTabsImpl(TObject* owner) { return &static_cast<TTabControl*>(owner)->tabs_; }

int  TTabControl::GetTabIndexImpl(TObject* owner)                   { return nv::TTabControl_GetTabIndex(owner->Handle()); }
void TTabControl::SetTabIndexImpl(TObject* owner, const int& value) { nv::TTabControl_SetTabIndex(owner->Handle(), value); }
TNotifyEvent TTabControl::GetOnChangeImpl(TObject* owner) { return static_cast<TTabControl*>(owner)->onChange_; }

void TTabControl::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TTabControl* self = static_cast<TTabControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TTabControl_SetOnChange, &TTabControl::ChangeTrampoline);
}

void NO_VCL_CALL TTabControl::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TTabControl* self = static_cast<TTabControl*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TCustomTabControl::TCustomTabControl(no_vcl_obj_t handle)
    : TWinControl(handle)
    , PageCount(this, &TCustomTabControl::GetPageCountImpl)
    , MultiLine(this, &TCustomTabControl::GetMultiLineImpl, &TCustomTabControl::SetMultiLineImpl)
    , ShowTabs(this, &TCustomTabControl::GetShowTabsImpl, &TCustomTabControl::SetShowTabsImpl)
    , TabPosition(this, &TCustomTabControl::GetTabPositionImpl, &TCustomTabControl::SetTabPositionImpl)
    , OnChanging(this, &TCustomTabControl::GetOnChangingImpl, &TCustomTabControl::SetOnChangingImpl)
    , Images(this, &TCustomTabControl::GetImagesImpl, &TCustomTabControl::SetImagesImpl)
{}

int  TCustomTabControl::GetPageCountImpl(TObject* owner)                 { return nv::TCustomTabControl_GetPageCount(owner->Handle()); }
bool TCustomTabControl::GetMultiLineImpl(TObject* owner)                 { return nv::TCustomTabControl_GetMultiLine(owner->Handle()) != 0; }
void TCustomTabControl::SetMultiLineImpl(TObject* owner, const bool& value) { nv::TCustomTabControl_SetMultiLine(owner->Handle(), value ? 1 : 0); }
bool TCustomTabControl::GetShowTabsImpl(TObject* owner)                  { return nv::TCustomTabControl_GetShowTabs(owner->Handle()) != 0; }
void TCustomTabControl::SetShowTabsImpl(TObject* owner, const bool& value)  { nv::TCustomTabControl_SetShowTabs(owner->Handle(), value ? 1 : 0); }
TTabPosition TCustomTabControl::GetTabPositionImpl(TObject* owner) { return static_cast<TTabPosition>(nv::TCustomTabControl_GetTabPosition(owner->Handle())); }
void TCustomTabControl::SetTabPositionImpl(TObject* owner, const TTabPosition& value) { nv::TCustomTabControl_SetTabPosition(owner->Handle(), value); }

TTabChangingEvent TCustomTabControl::GetOnChangingImpl(TObject* owner) { return static_cast<TCustomTabControl*>(owner)->onChanging_; }

void TCustomTabControl::SetOnChangingImpl(TObject* owner, const TTabChangingEvent& value)
{
    TCustomTabControl* self = static_cast<TCustomTabControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChanging_, self->onChangingHooked_, value,
                   &no_vcl_TCustomTabControl_SetOnChanging, &TCustomTabControl::ChangingTrampoline);
}

void NO_VCL_CALL TCustomTabControl::ChangingTrampoline(no_vcl_obj_t sender, no_vcl_bool_t* allowChange, void*)
{
    GuardCallback([&] {
        TCustomTabControl* self = static_cast<TCustomTabControl*>(FromHandle(sender));
        if (!self || !self->onChanging_)
            return;
        TTabChangingEvent handler = self->onChanging_;
        bool value = *allowChange != 0;
        handler(self, value);
        *allowChange = value ? 1 : 0;
    });
}

TPageControl::TPageControl(TComponent* AOwner)
    : TCustomTabControl(nv::TPageControl_Create(HandleOf(AOwner)))
    , ActivePage(this, &TPageControl::GetActivePageImpl, &TPageControl::SetActivePageImpl)
    , ActivePageIndex(this, &TPageControl::GetActivePageIndexImpl, &TPageControl::SetActivePageIndexImpl)
    , TabIndex(this, &TPageControl::GetTabIndexImpl, &TPageControl::SetTabIndexImpl)
    , OnChange(this, &TPageControl::GetOnChangeImpl, &TPageControl::SetOnChangeImpl)
    , Pages(this, &TPageControl::GetPagesImpl)
{}

TTabSheet* TPageControl::GetPagesImpl(TObject* owner, int Index) { return WrapExisting<TTabSheet>(nv::TPageControl_GetPage(owner->Handle(), Index)); }
TTabSheet* TPageControl::AddTabSheet()            { return WrapExisting<TTabSheet>(nv::TPageControl_AddTabSheet(handle_)); }
void TPageControl::Clear()                        { nv::TPageControl_Clear(handle_); }
void TPageControl::SelectNextPage(bool GoForward) { nv::TPageControl_SelectNextPage(handle_, GoForward ? 1 : 0); }

TTabSheet* TPageControl::GetActivePageImpl(TObject* owner) { return WrapExisting<TTabSheet>(nv::TPageControl_GetActivePage(owner->Handle())); }
void TPageControl::SetActivePageImpl(TObject* owner, TTabSheet* const& value) { nv::TPageControl_SetActivePage(owner->Handle(), HandleOf(value)); }
int  TPageControl::GetActivePageIndexImpl(TObject* owner)                  { return nv::TPageControl_GetActivePageIndex(owner->Handle()); }
void TPageControl::SetActivePageIndexImpl(TObject* owner, const int& value) { nv::TPageControl_SetActivePageIndex(owner->Handle(), value); }
int  TPageControl::GetTabIndexImpl(TObject* owner)                         { return nv::TPageControl_GetTabIndex(owner->Handle()); }
void TPageControl::SetTabIndexImpl(TObject* owner, const int& value)        { nv::TPageControl_SetTabIndex(owner->Handle(), value); }

TNotifyEvent TPageControl::GetOnChangeImpl(TObject* owner) { return static_cast<TPageControl*>(owner)->onChange_; }

void TPageControl::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TPageControl* self = static_cast<TPageControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TPageControl_SetOnChange, &TPageControl::ChangeTrampoline);
}

void NO_VCL_CALL TPageControl::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TPageControl* self = static_cast<TPageControl*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TCustomPage::TCustomPage(no_vcl_obj_t handle)
    : TWinControl(handle)
    , PageIndex(this, &TCustomPage::GetPageIndexImpl, &TCustomPage::SetPageIndexImpl)
    , TabVisible(this, &TCustomPage::GetTabVisibleImpl, &TCustomPage::SetTabVisibleImpl)
    , OnShow(this, &TCustomPage::GetOnShowImpl, &TCustomPage::SetOnShowImpl)
    , OnHide(this, &TCustomPage::GetOnHideImpl, &TCustomPage::SetOnHideImpl)
    , ImageIndex(this, &TCustomPage::GetImageIndexImpl, &TCustomPage::SetImageIndexImpl)
{}

int  TCustomPage::GetPageIndexImpl(TObject* owner)                   { return nv::TCustomPage_GetPageIndex(owner->Handle()); }
void TCustomPage::SetPageIndexImpl(TObject* owner, const int& value)  { nv::TCustomPage_SetPageIndex(owner->Handle(), value); }
bool TCustomPage::GetTabVisibleImpl(TObject* owner)                  { return nv::TCustomPage_GetTabVisible(owner->Handle()) != 0; }
void TCustomPage::SetTabVisibleImpl(TObject* owner, const bool& value) { nv::TCustomPage_SetTabVisible(owner->Handle(), value ? 1 : 0); }

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
    GuardCallback([&] {
        if (TCustomPage* self = static_cast<TCustomPage*>(FromHandle(sender)))
            CallNotify(self->onShow_, self);
    });
}

void NO_VCL_CALL TCustomPage::HideTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomPage* self = static_cast<TCustomPage*>(FromHandle(sender)))
            CallNotify(self->onHide_, self);
    });
}

TTabSheet::TTabSheet(TComponent* AOwner)
    : TTabSheet(nv::TTabSheet_Create(HandleOf(AOwner)))
{}

TTabSheet::TTabSheet(no_vcl_obj_t handle)
    : TCustomPage(handle)
    , PageControl(this, &TTabSheet::GetPageControlImpl, &TTabSheet::SetPageControlImpl)
    , TabIndex(this, &TTabSheet::GetTabIndexImpl)
{}

TPageControl* TTabSheet::GetPageControlImpl(TObject* owner)
{
    return static_cast<TPageControl*>(FromHandle(nv::TTabSheet_GetPageControl(owner->Handle())));
}

void TTabSheet::SetPageControlImpl(TObject* owner, TPageControl* const& value)
{
    nv::TTabSheet_SetPageControl(owner->Handle(), HandleOf(value));
}

int TTabSheet::GetTabIndexImpl(TObject* owner) { return nv::TTabSheet_GetTabIndex(owner->Handle()); }

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
        nv::ItemFree_SetCallback(&ItemRegistry::FreeTrampoline, nullptr);
        installed = true;
    }
}

void NO_VCL_CALL ItemRegistry::FreeTrampoline(no_vcl_obj_t handle, void*)
{
    GuardCallback([&] {
        std::unordered_map<no_vcl_obj_t, TPersistent*>& registry = Registry();
        auto it = registry.find(handle);
        if (it == registry.end())
            return;
        TPersistent* item = it->second;
        registry.erase(it);
        delete item;
    });
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
    , ImageIndex(this, &TTreeNode::GetImageIndexImpl, &TTreeNode::SetImageIndexImpl)
    , SelectedIndex(this, &TTreeNode::GetSelectedIndexImpl, &TTreeNode::SetSelectedIndexImpl)
    , StateIndex(this, &TTreeNode::GetStateIndexImpl, &TTreeNode::SetStateIndexImpl)
    , OverlayIndex(this, &TTreeNode::GetOverlayIndexImpl, &TTreeNode::SetOverlayIndexImpl)
{}

TTreeNode* TTreeNode::GetItemsImpl(TObject* owner, int Index) { return Wrap(nv::TTreeNode_GetItem(owner->Handle(), Index)); }
TTreeNode* TTreeNode::GetFirstChild() const     { return Wrap(nv::TTreeNode_GetFirstChild(handle_)); }
TTreeNode* TTreeNode::GetLastChild() const      { return Wrap(nv::TTreeNode_GetLastChild(handle_)); }
TTreeNode* TTreeNode::GetNextSibling() const    { return Wrap(nv::TTreeNode_GetNextSibling(handle_)); }
TTreeNode* TTreeNode::GetPrevSibling() const    { return Wrap(nv::TTreeNode_GetPrevSibling(handle_)); }
TTreeNode* TTreeNode::GetNext() const           { return Wrap(nv::TTreeNode_GetNext(handle_)); }
TTreeNode* TTreeNode::GetPrev() const           { return Wrap(nv::TTreeNode_GetPrev(handle_)); }
int  TTreeNode::IndexOf(TTreeNode* Node) const  { return nv::TTreeNode_IndexOf(handle_, Node ? Node->Handle() : nullptr); }
void TTreeNode::Expand(bool Recurse)            { nv::TTreeNode_Expand(handle_, Recurse ? 1 : 0); }
void TTreeNode::Collapse(bool Recurse)          { nv::TTreeNode_Collapse(handle_, Recurse ? 1 : 0); }
void TTreeNode::Delete()                        { nv::TTreeNode_Delete(handle_); }
void TTreeNode::DeleteChildren()                { nv::TTreeNode_DeleteChildren(handle_); }
void TTreeNode::MakeVisible()                   { nv::TTreeNode_MakeVisible(handle_); }

void TTreeNode::MoveTo(TTreeNode* Destination, TNodeAttachMode Mode)
{
    nv::TTreeNode_MoveTo(handle_, Destination ? Destination->Handle() : nullptr, Mode);
}

std::string TTreeNode::GetTextImpl(TObject* owner) { return std::string(nv::TTreeNode_GetText(owner->Handle())); }
void TTreeNode::SetTextImpl(TObject* owner, const std::string& value) { nv::TTreeNode_SetText(owner->Handle(), value.c_str()); }
bool TTreeNode::GetExpandedImpl(TObject* owner)                      { return nv::TTreeNode_GetExpanded(owner->Handle()) != 0; }
void TTreeNode::SetExpandedImpl(TObject* owner, const bool& value)    { nv::TTreeNode_SetExpanded(owner->Handle(), value ? 1 : 0); }
bool TTreeNode::GetSelectedImpl(TObject* owner)                      { return nv::TTreeNode_GetSelected(owner->Handle()) != 0; }
void TTreeNode::SetSelectedImpl(TObject* owner, const bool& value)    { nv::TTreeNode_SetSelected(owner->Handle(), value ? 1 : 0); }
bool TTreeNode::GetHasChildrenImpl(TObject* owner)                   { return nv::TTreeNode_GetHasChildren(owner->Handle()) != 0; }
void TTreeNode::SetHasChildrenImpl(TObject* owner, const bool& value) { nv::TTreeNode_SetHasChildren(owner->Handle(), value ? 1 : 0); }
void* TTreeNode::GetDataImpl(TObject* owner)                         { return nv::TTreeNode_GetData(owner->Handle()); }
void TTreeNode::SetDataImpl(TObject* owner, void* const& value)       { nv::TTreeNode_SetData(owner->Handle(), value); }
int  TTreeNode::GetCountImpl(TObject* owner)                         { return nv::TTreeNode_GetCount(owner->Handle()); }
int  TTreeNode::GetIndexImpl(TObject* owner)                         { return nv::TTreeNode_GetIndex(owner->Handle()); }
int  TTreeNode::GetLevelImpl(TObject* owner)                         { return nv::TTreeNode_GetLevel(owner->Handle()); }
int  TTreeNode::GetAbsoluteIndexImpl(TObject* owner)                 { return nv::TTreeNode_GetAbsoluteIndex(owner->Handle()); }
TTreeNode* TTreeNode::GetParentImpl(TObject* owner)                  { return Wrap(nv::TTreeNode_GetParent(owner->Handle())); }

TCustomTreeView* TTreeNode::GetTreeViewImpl(TObject* owner)
{
    // ツリービューは TComponent で、C++ ラッパーを介して作られていればレジストリにある。
    return static_cast<TCustomTreeView*>(TCustomTreeView::FromHandle(nv::TTreeNode_GetTreeView(owner->Handle())));
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
    return TTreeNode::Wrap(nv::TTreeNodes_Add(handle_, NodeHandle(Sibling), S.c_str()));
}

TTreeNode* TTreeNodes::AddFirst(TTreeNode* Sibling, const std::string& S)
{
    return TTreeNode::Wrap(nv::TTreeNodes_AddFirst(handle_, NodeHandle(Sibling), S.c_str()));
}

TTreeNode* TTreeNodes::AddChild(TTreeNode* Parent, const std::string& S)
{
    return TTreeNode::Wrap(nv::TTreeNodes_AddChild(handle_, NodeHandle(Parent), S.c_str()));
}

TTreeNode* TTreeNodes::AddChildFirst(TTreeNode* Parent, const std::string& S)
{
    return TTreeNode::Wrap(nv::TTreeNodes_AddChildFirst(handle_, NodeHandle(Parent), S.c_str()));
}

TTreeNode* TTreeNodes::Insert(TTreeNode* NextNode, const std::string& S)
{
    return TTreeNode::Wrap(nv::TTreeNodes_Insert(handle_, NodeHandle(NextNode), S.c_str()));
}

void TTreeNodes::Clear()                   { nv::TTreeNodes_Clear(handle_); }
void TTreeNodes::Delete(TTreeNode* Node)   { nv::TTreeNodes_Delete(handle_, NodeHandle(Node)); }
TTreeNode* TTreeNodes::GetItemImpl(TObject* owner, int Index) { return TTreeNode::Wrap(nv::TTreeNodes_GetItem(owner->Handle(), Index)); }
TTreeNode* TTreeNodes::GetFirstNode() const     { return TTreeNode::Wrap(nv::TTreeNodes_GetFirstNode(handle_)); }

TTreeNode* TTreeNodes::FindNodeWithText(const std::string& S) const
{
    return TTreeNode::Wrap(nv::TTreeNodes_FindNodeWithText(handle_, S.c_str()));
}

void TTreeNodes::BeginUpdate()             { nv::TTreeNodes_BeginUpdate(handle_); }
void TTreeNodes::EndUpdate()               { nv::TTreeNodes_EndUpdate(handle_); }
int  TTreeNodes::GetCountImpl(TObject* owner) { return nv::TTreeNodes_GetCount(owner->Handle()); }

TCustomTreeView::TCustomTreeView(no_vcl_obj_t handle)
    : TCustomControl(handle)
    , Items(this, &TCustomTreeView::GetItemsImpl)
    , Selected(this, &TCustomTreeView::GetSelectedImpl, &TCustomTreeView::SetSelectedImpl)
    , Images(this, &TCustomTreeView::GetImagesImpl, &TCustomTreeView::SetImagesImpl)
    , StateImages(this, &TCustomTreeView::GetStateImagesImpl, &TCustomTreeView::SetStateImagesImpl)
    , items_(nv::TCustomTreeView_GetItems(handle))
{}

void TCustomTreeView::FullExpand()   { nv::TCustomTreeView_FullExpand(handle_); }
void TCustomTreeView::FullCollapse() { nv::TCustomTreeView_FullCollapse(handle_); }
bool TCustomTreeView::AlphaSort()    { return nv::TCustomTreeView_AlphaSort(handle_) != 0; }

TTreeNode* TCustomTreeView::GetNodeAt(int X, int Y) const
{
    return TTreeNode::Wrap(nv::TCustomTreeView_GetNodeAt(handle_, X, Y));
}

TTreeNodes* TCustomTreeView::GetItemsImpl(TObject* owner) { return &static_cast<TCustomTreeView*>(owner)->items_; }
TTreeNode*  TCustomTreeView::GetSelectedImpl(TObject* owner) { return TTreeNode::Wrap(nv::TCustomTreeView_GetSelected(owner->Handle())); }

void TCustomTreeView::SetSelectedImpl(TObject* owner, TTreeNode* const& value)
{
    nv::TCustomTreeView_SetSelected(owner->Handle(), NodeHandle(value));
}

TTreeView::TTreeView(TComponent* AOwner)
    : TCustomTreeView(nv::TTreeView_Create(HandleOf(AOwner)))
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

bool TTreeView::GetReadOnlyImpl(TObject* owner)                     { return nv::TTreeView_GetReadOnly(owner->Handle()) != 0; }
void TTreeView::SetReadOnlyImpl(TObject* owner, const bool& value)   { nv::TTreeView_SetReadOnly(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowLinesImpl(TObject* owner)                    { return nv::TTreeView_GetShowLines(owner->Handle()) != 0; }
void TTreeView::SetShowLinesImpl(TObject* owner, const bool& value)  { nv::TTreeView_SetShowLines(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowRootImpl(TObject* owner)                     { return nv::TTreeView_GetShowRoot(owner->Handle()) != 0; }
void TTreeView::SetShowRootImpl(TObject* owner, const bool& value)   { nv::TTreeView_SetShowRoot(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowButtonsImpl(TObject* owner)                  { return nv::TTreeView_GetShowButtons(owner->Handle()) != 0; }
void TTreeView::SetShowButtonsImpl(TObject* owner, const bool& value) { nv::TTreeView_SetShowButtons(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetAutoExpandImpl(TObject* owner)                   { return nv::TTreeView_GetAutoExpand(owner->Handle()) != 0; }
void TTreeView::SetAutoExpandImpl(TObject* owner, const bool& value) { nv::TTreeView_SetAutoExpand(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetHideSelectionImpl(TObject* owner)                { return nv::TTreeView_GetHideSelection(owner->Handle()) != 0; }
void TTreeView::SetHideSelectionImpl(TObject* owner, const bool& value) { nv::TTreeView_SetHideSelection(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetRowSelectImpl(TObject* owner)                    { return nv::TTreeView_GetRowSelect(owner->Handle()) != 0; }
void TTreeView::SetRowSelectImpl(TObject* owner, const bool& value)  { nv::TTreeView_SetRowSelect(owner->Handle(), value ? 1 : 0); }

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

void NO_VCL_CALL TTreeView::ChangeTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, void*)    { GuardCallback([&] { DispatchNode(s, n, &TTreeView::onChange_); }); }
void NO_VCL_CALL TTreeView::ExpandedTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, void*)  { GuardCallback([&] { DispatchNode(s, n, &TTreeView::onExpanded_); }); }
void NO_VCL_CALL TTreeView::CollapsedTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, void*) { GuardCallback([&] { DispatchNode(s, n, &TTreeView::onCollapsed_); }); }
void NO_VCL_CALL TTreeView::DeletionTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, void*)  { GuardCallback([&] { DispatchNode(s, n, &TTreeView::onDeletion_); }); }

void NO_VCL_CALL TTreeView::ChangingTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, no_vcl_bool_t* a, void*)
{
    GuardCallback([&] {
        DispatchNodeAllow(s, n, a, &TTreeView::onChanging_);
    });
}

void NO_VCL_CALL TTreeView::ExpandingTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, no_vcl_bool_t* a, void*)
{
    GuardCallback([&] {
        DispatchNodeAllow(s, n, a, &TTreeView::onExpanding_);
    });
}

void NO_VCL_CALL TTreeView::CollapsingTrampoline(no_vcl_obj_t s, no_vcl_obj_t n, no_vcl_bool_t* a, void*)
{
    GuardCallback([&] {
        DispatchNodeAllow(s, n, a, &TTreeView::onCollapsing_);
    });
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
    , ImageIndex(this, &TListItem::GetImageIndexImpl, &TListItem::SetImageIndexImpl)
    , StateIndex(this, &TListItem::GetStateIndexImpl, &TListItem::SetStateIndexImpl)
    , subItems_(this, &no_vcl_TListItem_GetSubItems)
{}

TStrings* TListItem::GetSubItemsImpl(TObject* owner) { return &static_cast<TListItem*>(owner)->subItems_; }
void TListItem::Delete()                               { nv::TListItem_Delete(handle_); }
void TListItem::MakeVisible(bool PartialOK)            { nv::TListItem_MakeVisible(handle_, PartialOK ? 1 : 0); }

std::string TListItem::GetCaptionImpl(TObject* owner) { return std::string(nv::TListItem_GetCaption(owner->Handle())); }
void TListItem::SetCaptionImpl(TObject* owner, const std::string& value) { nv::TListItem_SetCaption(owner->Handle(), value.c_str()); }
bool TListItem::GetCheckedImpl(TObject* owner)                     { return nv::TListItem_GetChecked(owner->Handle()) != 0; }
void TListItem::SetCheckedImpl(TObject* owner, const bool& value)   { nv::TListItem_SetChecked(owner->Handle(), value ? 1 : 0); }
bool TListItem::GetSelectedImpl(TObject* owner)                    { return nv::TListItem_GetSelected(owner->Handle()) != 0; }
void TListItem::SetSelectedImpl(TObject* owner, const bool& value)  { nv::TListItem_SetSelected(owner->Handle(), value ? 1 : 0); }
bool TListItem::GetFocusedImpl(TObject* owner)                     { return nv::TListItem_GetFocused(owner->Handle()) != 0; }
void TListItem::SetFocusedImpl(TObject* owner, const bool& value)   { nv::TListItem_SetFocused(owner->Handle(), value ? 1 : 0); }
void* TListItem::GetDataImpl(TObject* owner)                       { return nv::TListItem_GetData(owner->Handle()); }
void TListItem::SetDataImpl(TObject* owner, void* const& value)     { nv::TListItem_SetData(owner->Handle(), value); }
int  TListItem::GetIndexImpl(TObject* owner)                       { return nv::TListItem_GetIndex(owner->Handle()); }

TCustomListView* TListItem::GetListViewImpl(TObject* owner)
{
    return static_cast<TCustomListView*>(TCustomListView::FromHandle(nv::TListItem_GetListView(owner->Handle())));
}

TListItems::TListItems(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &TListItems::GetCountImpl)
    , Item(this, &TListItems::GetItemImpl)
{}

TListItem* TListItems::Add()                     { return TListItem::Wrap(nv::TListItems_Add(handle_)); }
TListItem* TListItems::Insert(int Index)         { return TListItem::Wrap(nv::TListItems_Insert(handle_, Index)); }
void TListItems::Delete(int Index)               { nv::TListItems_Delete(handle_, Index); }
void TListItems::Clear()                         { nv::TListItems_Clear(handle_); }
TListItem* TListItems::GetItemImpl(TObject* owner, int Index) { return TListItem::Wrap(nv::TListItems_GetItem(owner->Handle(), Index)); }
int  TListItems::IndexOf(TListItem* Item) const  { return nv::TListItems_IndexOf(handle_, ItemHandle(Item)); }

TListItem* TListItems::FindCaption(int StartIndex, const std::string& Value, bool Partial, bool Inclusive, bool Wrap) const
{
    return TListItem::Wrap(nv::TListItems_FindCaption(handle_, StartIndex, Value.c_str(),
                                                        Partial ? 1 : 0, Inclusive ? 1 : 0, Wrap ? 1 : 0));
}

void TListItems::Exchange(int Index1, int Index2) { nv::TListItems_Exchange(handle_, Index1, Index2); }
void TListItems::Move(int FromIndex, int ToIndex) { nv::TListItems_Move(handle_, FromIndex, ToIndex); }
void TListItems::BeginUpdate()                    { nv::TListItems_BeginUpdate(handle_); }
void TListItems::EndUpdate()                      { nv::TListItems_EndUpdate(handle_); }
int  TListItems::GetCountImpl(TObject* owner)     { return nv::TListItems_GetCount(owner->Handle()); }

TListColumn::TListColumn(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Caption(this, &TListColumn::GetCaptionImpl, &TListColumn::SetCaptionImpl)
    , Width(this, &TListColumn::GetWidthImpl, &TListColumn::SetWidthImpl)
    , Alignment(this, &TListColumn::GetAlignmentImpl, &TListColumn::SetAlignmentImpl)
    , AutoSize(this, &TListColumn::GetAutoSizeImpl, &TListColumn::SetAutoSizeImpl)
    , Visible(this, &TListColumn::GetVisibleImpl, &TListColumn::SetVisibleImpl)
    , Index(this, &TListColumn::GetIndexImpl, &TListColumn::SetIndexImpl)
    , ImageIndex(this, &TListColumn::GetImageIndexImpl, &TListColumn::SetImageIndexImpl)
{}

std::string TListColumn::GetCaptionImpl(TObject* owner) { return std::string(nv::TListColumn_GetCaption(owner->Handle())); }
void TListColumn::SetCaptionImpl(TObject* owner, const std::string& value) { nv::TListColumn_SetCaption(owner->Handle(), value.c_str()); }
int  TListColumn::GetWidthImpl(TObject* owner)                    { return nv::TListColumn_GetWidth(owner->Handle()); }
void TListColumn::SetWidthImpl(TObject* owner, const int& value)   { nv::TListColumn_SetWidth(owner->Handle(), value); }
TAlignment TListColumn::GetAlignmentImpl(TObject* owner) { return static_cast<TAlignment>(nv::TListColumn_GetAlignment(owner->Handle())); }
void TListColumn::SetAlignmentImpl(TObject* owner, const TAlignment& value) { nv::TListColumn_SetAlignment(owner->Handle(), value); }
bool TListColumn::GetAutoSizeImpl(TObject* owner)                 { return nv::TListColumn_GetAutoSize(owner->Handle()) != 0; }
void TListColumn::SetAutoSizeImpl(TObject* owner, const bool& value) { nv::TListColumn_SetAutoSize(owner->Handle(), value ? 1 : 0); }
bool TListColumn::GetVisibleImpl(TObject* owner)                  { return nv::TListColumn_GetVisible(owner->Handle()) != 0; }
void TListColumn::SetVisibleImpl(TObject* owner, const bool& value) { nv::TListColumn_SetVisible(owner->Handle(), value ? 1 : 0); }
int  TListColumn::GetIndexImpl(TObject* owner)                    { return nv::TListColumn_GetIndex(owner->Handle()); }
void TListColumn::SetIndexImpl(TObject* owner, const int& value)   { nv::TListColumn_SetIndex(owner->Handle(), value); }

TListColumns::TListColumns(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &TListColumns::GetCountImpl)
    , Items(this, &TListColumns::GetItemsImpl)
{}

TListColumn* TListColumns::Add()                    { return TListColumn::Wrap(nv::TListColumns_Add(handle_)); }
TListColumn* TListColumns::GetItemsImpl(TObject* owner, int Index) { return TListColumn::Wrap(nv::TListColumns_GetItem(owner->Handle(), Index)); }
void TListColumns::Delete(int Index)                { nv::TListColumns_Delete(handle_, Index); }
void TListColumns::Clear()                          { nv::TListColumns_Clear(handle_); }
int  TListColumns::GetCountImpl(TObject* owner)     { return nv::TListColumns_GetCount(owner->Handle()); }

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
    , items_(nv::TCustomListView_GetItems(handle))
{}

void TCustomListView::Clear()          { nv::TCustomListView_Clear(handle_); }
void TCustomListView::BeginUpdate()    { nv::TCustomListView_BeginUpdate(handle_); }
void TCustomListView::EndUpdate()      { nv::TCustomListView_EndUpdate(handle_); }
void TCustomListView::ClearSelection() { nv::TCustomListView_ClearSelection(handle_); }
void TCustomListView::SelectAll()      { nv::TCustomListView_SelectAll(handle_); }

TListItem* TCustomListView::GetItemAt(int X, int Y) const
{
    return TListItem::Wrap(nv::TCustomListView_GetItemAt(handle_, X, Y));
}

TListItems* TCustomListView::GetItemsImpl(TObject* owner)   { return &static_cast<TCustomListView*>(owner)->items_; }
TListItem*  TCustomListView::GetSelectedImpl(TObject* owner) { return TListItem::Wrap(nv::TCustomListView_GetSelected(owner->Handle())); }
void TCustomListView::SetSelectedImpl(TObject* owner, TListItem* const& value) { nv::TCustomListView_SetSelected(owner->Handle(), ItemHandle(value)); }
int  TCustomListView::GetItemIndexImpl(TObject* owner)                   { return nv::TCustomListView_GetItemIndex(owner->Handle()); }
void TCustomListView::SetItemIndexImpl(TObject* owner, const int& value)  { nv::TCustomListView_SetItemIndex(owner->Handle(), value); }
int  TCustomListView::GetSelCountImpl(TObject* owner)                    { return nv::TCustomListView_GetSelCount(owner->Handle()); }
bool TCustomListView::GetCheckboxesImpl(TObject* owner)                  { return nv::TCustomListView_GetCheckboxes(owner->Handle()) != 0; }
void TCustomListView::SetCheckboxesImpl(TObject* owner, const bool& value) { nv::TCustomListView_SetCheckboxes(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetGridLinesImpl(TObject* owner)                   { return nv::TCustomListView_GetGridLines(owner->Handle()) != 0; }
void TCustomListView::SetGridLinesImpl(TObject* owner, const bool& value) { nv::TCustomListView_SetGridLines(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetMultiSelectImpl(TObject* owner)                 { return nv::TCustomListView_GetMultiSelect(owner->Handle()) != 0; }
void TCustomListView::SetMultiSelectImpl(TObject* owner, const bool& value) { nv::TCustomListView_SetMultiSelect(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetReadOnlyImpl(TObject* owner)                    { return nv::TCustomListView_GetReadOnly(owner->Handle()) != 0; }
void TCustomListView::SetReadOnlyImpl(TObject* owner, const bool& value)  { nv::TCustomListView_SetReadOnly(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetRowSelectImpl(TObject* owner)                   { return nv::TCustomListView_GetRowSelect(owner->Handle()) != 0; }
void TCustomListView::SetRowSelectImpl(TObject* owner, const bool& value) { nv::TCustomListView_SetRowSelect(owner->Handle(), value ? 1 : 0); }

TListView::TListView(TComponent* AOwner)
    : TCustomListView(nv::TListView_Create(HandleOf(AOwner)))
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
    , LargeImages(this, &TListView::GetLargeImagesImpl, &TListView::SetLargeImagesImpl)
    , SmallImages(this, &TListView::GetSmallImagesImpl, &TListView::SetSmallImagesImpl)
    , StateImages(this, &TListView::GetStateImagesImpl, &TListView::SetStateImagesImpl)
    , columns_(nv::TListView_GetColumns(handle_))
{}

TListColumns* TListView::GetColumnsImpl(TObject* owner) { return &static_cast<TListView*>(owner)->columns_; }
TViewStyle TListView::GetViewStyleImpl(TObject* owner) { return static_cast<TViewStyle>(nv::TListView_GetViewStyle(owner->Handle())); }
void TListView::SetViewStyleImpl(TObject* owner, const TViewStyle& value) { nv::TListView_SetViewStyle(owner->Handle(), value); }
bool TListView::GetHideSelectionImpl(TObject* owner)                    { return nv::TListView_GetHideSelection(owner->Handle()) != 0; }
void TListView::SetHideSelectionImpl(TObject* owner, const bool& value)  { nv::TListView_SetHideSelection(owner->Handle(), value ? 1 : 0); }
TSortType TListView::GetSortTypeImpl(TObject* owner) { return static_cast<TSortType>(nv::TListView_GetSortType(owner->Handle())); }
void TListView::SetSortTypeImpl(TObject* owner, const TSortType& value) { nv::TListView_SetSortType(owner->Handle(), value); }
int  TListView::GetSortColumnImpl(TObject* owner)                       { return nv::TListView_GetSortColumn(owner->Handle()); }
void TListView::SetSortColumnImpl(TObject* owner, const int& value)      { nv::TListView_SetSortColumn(owner->Handle(), value); }
TSortDirection TListView::GetSortDirectionImpl(TObject* owner) { return static_cast<TSortDirection>(nv::TListView_GetSortDirection(owner->Handle())); }
void TListView::SetSortDirectionImpl(TObject* owner, const TSortDirection& value) { nv::TListView_SetSortDirection(owner->Handle(), value); }

// リストビューの破棄では、リストビュー自身のラッパーが delete された後に項目が破棄される(LCL の順序)。
// そのときの OnDeletion は FromHandle が nullptr を返すため、ハンドラは呼ばれない。
void NO_VCL_CALL TListView::SelectItemTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, no_vcl_int_t selected, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onSelectItem_)
            return;
        TLVSelectItemEvent handler = self->onSelectItem_;
        handler(self, TListItem::Wrap(item), selected != 0);
    });
}

void NO_VCL_CALL TListView::ChangeTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, no_vcl_int_t change, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onChange_)
            return;
        TLVChangeEvent handler = self->onChange_;
        handler(self, TListItem::Wrap(item), static_cast<TItemChange>(change));
    });
}

void NO_VCL_CALL TListView::DeletionTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onDeletion_)
            return;
        TLVDeletedEvent handler = self->onDeletion_;
        handler(self, TListItem::Wrap(item));
    });
}

void NO_VCL_CALL TListView::ItemCheckedTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onItemChecked_)
            return;
        TLVCheckedItemEvent handler = self->onItemChecked_;
        handler(self, TListItem::Wrap(item));
    });
}

void NO_VCL_CALL TListView::ColumnClickTrampoline(no_vcl_obj_t sender, no_vcl_obj_t column, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onColumnClick_)
            return;
        TLVColumnClickEvent handler = self->onColumnClick_;
        handler(self, TListColumn::Wrap(column));
    });
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

int  TCustomSplitter::GetSplitterPosition() const  { return nv::TCustomSplitter_GetSplitterPosition(handle_); }
void TCustomSplitter::SetSplitterPosition(int pos) { nv::TCustomSplitter_SetSplitterPosition(handle_, pos); }

bool TCustomSplitter::GetAutoSnapImpl(TObject* owner)                   { return nv::TCustomSplitter_GetAutoSnap(owner->Handle()) != 0; }
void TCustomSplitter::SetAutoSnapImpl(TObject* owner, const bool& value) { nv::TCustomSplitter_SetAutoSnap(owner->Handle(), value ? 1 : 0); }
bool TCustomSplitter::GetBeveledImpl(TObject* owner)                    { return nv::TCustomSplitter_GetBeveled(owner->Handle()) != 0; }
void TCustomSplitter::SetBeveledImpl(TObject* owner, const bool& value)  { nv::TCustomSplitter_SetBeveled(owner->Handle(), value ? 1 : 0); }
int  TCustomSplitter::GetMinSizeImpl(TObject* owner)                    { return nv::TCustomSplitter_GetMinSize(owner->Handle()); }
void TCustomSplitter::SetMinSizeImpl(TObject* owner, const int& value)   { nv::TCustomSplitter_SetMinSize(owner->Handle(), value); }

TAnchorKind TCustomSplitter::GetResizeAnchorImpl(TObject* owner) { return static_cast<TAnchorKind>(nv::TCustomSplitter_GetResizeAnchor(owner->Handle())); }
void TCustomSplitter::SetResizeAnchorImpl(TObject* owner, const TAnchorKind& value) { nv::TCustomSplitter_SetResizeAnchor(owner->Handle(), value); }
TResizeStyle TCustomSplitter::GetResizeStyleImpl(TObject* owner) { return static_cast<TResizeStyle>(nv::TCustomSplitter_GetResizeStyle(owner->Handle())); }
void TCustomSplitter::SetResizeStyleImpl(TObject* owner, const TResizeStyle& value) { nv::TCustomSplitter_SetResizeStyle(owner->Handle(), value); }

TNotifyEvent TCustomSplitter::GetOnMovedImpl(TObject* owner) { return static_cast<TCustomSplitter*>(owner)->onMoved_; }

void TCustomSplitter::SetOnMovedImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomSplitter* self = static_cast<TCustomSplitter*>(owner);
    SetSimpleEvent(self->handle_, self->onMoved_, self->onMovedHooked_, value,
                   &no_vcl_TCustomSplitter_SetOnMoved, &TCustomSplitter::MovedTrampoline);
}

void NO_VCL_CALL TCustomSplitter::MovedTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomSplitter* self = static_cast<TCustomSplitter*>(FromHandle(sender)))
            CallNotify(self->onMoved_, self);
    });
}

TSplitter::TSplitter(TComponent* AOwner)
    : TCustomSplitter(nv::TSplitter_Create(HandleOf(AOwner)))
{}

TCustomMemo::TCustomMemo(no_vcl_obj_t handle)
    : TCustomEdit(handle)
    , ScrollBars(this, &TCustomMemo::GetScrollBarsImpl, &TCustomMemo::SetScrollBarsImpl)
    , Lines(this, &TCustomMemo::GetLinesImpl)
    , lines_(this, &no_vcl_TCustomMemo_GetLines)
{}

TStrings* TCustomMemo::GetLinesImpl(TObject* owner) { return &static_cast<TCustomMemo*>(owner)->lines_; }

int  TCustomMemo::GetScrollBarsImpl(TObject* owner)                   { return nv::TCustomMemo_GetScrollBars(owner->Handle()); }
void TCustomMemo::SetScrollBarsImpl(TObject* owner, const int& value) { nv::TCustomMemo_SetScrollBars(owner->Handle(), value); }

TMemo::TMemo(TComponent* AOwner)
    : TCustomMemo(nv::TMemo_Create(HandleOf(AOwner)))
{}

/* ---------------- ComboBox / ListBox ---------------- */

TCustomComboBox::TCustomComboBox(no_vcl_obj_t handle)
    : TWinControl(handle)
    , ItemIndex(this, &TCustomComboBox::GetItemIndexImpl, &TCustomComboBox::SetItemIndexImpl)
    , Items(this, &TCustomComboBox::GetItemsImpl)
    , items_(this, &no_vcl_TCustomComboBox_GetItems)
{}

TStrings* TCustomComboBox::GetItemsImpl(TObject* owner) { return &static_cast<TCustomComboBox*>(owner)->items_; }

int  TCustomComboBox::GetItemIndexImpl(TObject* owner)                   { return nv::TCustomComboBox_GetItemIndex(owner->Handle()); }
void TCustomComboBox::SetItemIndexImpl(TObject* owner, const int& value) { nv::TCustomComboBox_SetItemIndex(owner->Handle(), value); }

TComboBox::TComboBox(TComponent* AOwner)
    : TCustomComboBox(nv::TComboBox_Create(HandleOf(AOwner)))
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
        nv::TComboBox_SetOnChange(self->handle_, &TComboBox::ChangeTrampoline, nullptr);
        self->onChangeHooked_ = true;
    }
}

void NO_VCL_CALL TComboBox::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        TComboBox* self = static_cast<TComboBox*>(FromHandle(sender));
        if (!self || !self->onChange_)
            return;
        TNotifyEvent handler = self->onChange_;
        handler(self);
    });
}

TCustomListBox::TCustomListBox(no_vcl_obj_t handle)
    : TWinControl(handle)
    , ItemIndex(this, &TCustomListBox::GetItemIndexImpl, &TCustomListBox::SetItemIndexImpl)
    , Items(this, &TCustomListBox::GetItemsImpl)
    , items_(this, &no_vcl_TCustomListBox_GetItems)
{}

TStrings* TCustomListBox::GetItemsImpl(TObject* owner) { return &static_cast<TCustomListBox*>(owner)->items_; }

int  TCustomListBox::GetItemIndexImpl(TObject* owner)                   { return nv::TCustomListBox_GetItemIndex(owner->Handle()); }
void TCustomListBox::SetItemIndexImpl(TObject* owner, const int& value) { nv::TCustomListBox_SetItemIndex(owner->Handle(), value); }

TListBox::TListBox(TComponent* AOwner)
    : TCustomListBox(nv::TListBox_Create(HandleOf(AOwner)))
{}

TCustomCheckListBox::TCustomCheckListBox(no_vcl_obj_t handle)
    : TCustomListBox(handle)
    , OnClickCheck(this, &TCustomCheckListBox::GetOnClickCheckImpl, &TCustomCheckListBox::SetOnClickCheckImpl)
    , Checked(this, &TCustomCheckListBox::GetCheckedImpl, &TCustomCheckListBox::SetCheckedImpl)
{}

bool TCustomCheckListBox::GetCheckedImpl(TObject* owner, int index)                    { return nv::TCustomCheckListBox_GetChecked(owner->Handle(), index) != 0; }
void TCustomCheckListBox::SetCheckedImpl(TObject* owner, int index, const bool& value) { nv::TCustomCheckListBox_SetChecked(owner->Handle(), index, value ? 1 : 0); }

TNotifyEvent TCustomCheckListBox::GetOnClickCheckImpl(TObject* owner) { return static_cast<TCustomCheckListBox*>(owner)->onClickCheck_; }

void TCustomCheckListBox::SetOnClickCheckImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomCheckListBox* self = static_cast<TCustomCheckListBox*>(owner);
    SetSimpleEvent(self->handle_, self->onClickCheck_, self->onClickCheckHooked_, value,
                   &no_vcl_TCustomCheckListBox_SetOnClickCheck, &TCustomCheckListBox::ClickCheckTrampoline);
}

void NO_VCL_CALL TCustomCheckListBox::ClickCheckTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomCheckListBox* self = static_cast<TCustomCheckListBox*>(FromHandle(sender)))
            CallNotify(self->onClickCheck_, self);
    });
}

TCheckListBox::TCheckListBox(TComponent* AOwner)
    : TCustomCheckListBox(nv::TCheckListBox_Create(HandleOf(AOwner)))
{}

TCustomStaticText::TCustomStaticText(no_vcl_obj_t handle)
    : TWinControl(handle)
    , BorderStyle(this, &TCustomStaticText::GetBorderStyleImpl, &TCustomStaticText::SetBorderStyleImpl)
{}

TStaticBorderStyle TCustomStaticText::GetBorderStyleImpl(TObject* owner)
{
    return static_cast<TStaticBorderStyle>(nv::TCustomStaticText_GetBorderStyle(owner->Handle()));
}

void TCustomStaticText::SetBorderStyleImpl(TObject* owner, const TStaticBorderStyle& value)
{
    nv::TCustomStaticText_SetBorderStyle(owner->Handle(), value);
}

TStaticText::TStaticText(TComponent* AOwner)
    : TCustomStaticText(nv::TStaticText_Create(HandleOf(AOwner)))
{}

TStatusBar::TStatusBar(TComponent* AOwner)
    : TWinControl(nv::TStatusBar_Create(HandleOf(AOwner)))
    , SimpleText(this, &TStatusBar::GetSimpleTextImpl, &TStatusBar::SetSimpleTextImpl)
    , SimplePanel(this, &TStatusBar::GetSimplePanelImpl, &TStatusBar::SetSimplePanelImpl)
{}

std::string TStatusBar::GetSimpleTextImpl(TObject* owner)
{
    return std::string(nv::TStatusBar_GetSimpleText(owner->Handle()));
}

void TStatusBar::SetSimpleTextImpl(TObject* owner, const std::string& value)
{
    nv::TStatusBar_SetSimpleText(owner->Handle(), value.c_str());
}

bool TStatusBar::GetSimplePanelImpl(TObject* owner) { return nv::TStatusBar_GetSimplePanel(owner->Handle()) != 0; }
void TStatusBar::SetSimplePanelImpl(TObject* owner, const bool& value) { nv::TStatusBar_SetSimplePanel(owner->Handle(), value ? 1 : 0); }

/* ---------------- Canvas ---------------- */

TPen::TPen(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Color(this, &TPen::GetColorImpl, &TPen::SetColorImpl)
    , Width(this, &TPen::GetWidthImpl, &TPen::SetWidthImpl)
{}

TColor TPen::GetColorImpl(TObject* owner)                      { return static_cast<TColor>(nv::TPen_GetColor(owner->Handle())); }
void   TPen::SetColorImpl(TObject* owner, const TColor& value) { nv::TPen_SetColor(owner->Handle(), value); }
int    TPen::GetWidthImpl(TObject* owner)                      { return nv::TPen_GetWidth(owner->Handle()); }
void   TPen::SetWidthImpl(TObject* owner, const int& value)    { nv::TPen_SetWidth(owner->Handle(), value); }

TBrush::TBrush(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Color(this, &TBrush::GetColorImpl, &TBrush::SetColorImpl)
{}

TColor TBrush::GetColorImpl(TObject* owner)                      { return static_cast<TColor>(nv::TBrush_GetColor(owner->Handle())); }
void   TBrush::SetColorImpl(TObject* owner, const TColor& value) { nv::TBrush_SetColor(owner->Handle(), value); }

TFont::TFont(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Name(this, &TFont::GetNameImpl, &TFont::SetNameImpl)
    , Size(this, &TFont::GetSizeImpl, &TFont::SetSizeImpl)
    , Color(this, &TFont::GetColorImpl, &TFont::SetColorImpl)
{}

std::string TFont::GetNameImpl(TObject* owner)
{
    return std::string(nv::TFont_GetName(owner->Handle()));
}

void TFont::SetNameImpl(TObject* owner, const std::string& value)
{
    nv::TFont_SetName(owner->Handle(), value.c_str());
}

int    TFont::GetSizeImpl(TObject* owner)                      { return nv::TFont_GetSize(owner->Handle()); }
void   TFont::SetSizeImpl(TObject* owner, const int& value)    { nv::TFont_SetSize(owner->Handle(), value); }
TColor TFont::GetColorImpl(TObject* owner)                     { return static_cast<TColor>(nv::TFont_GetColor(owner->Handle())); }
void   TFont::SetColorImpl(TObject* owner, const TColor& value){ nv::TFont_SetColor(owner->Handle(), value); }

TCanvas::TCanvas(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Pen(nv::TCanvas_GetPen(handle))
    , Brush(nv::TCanvas_GetBrush(handle))
    , Font(nv::TCanvas_GetFont(handle))
    , Pixels(this, &TCanvas::GetPixelsImpl, &TCanvas::SetPixelsImpl)
{}

void TCanvas::MoveTo(int x, int y)                        { nv::TCanvas_MoveTo(handle_, x, y); }
void TCanvas::LineTo(int x, int y)                        { nv::TCanvas_LineTo(handle_, x, y); }
void TCanvas::Rectangle(int x1, int y1, int x2, int y2)   { nv::TCanvas_Rectangle(handle_, x1, y1, x2, y2); }
void TCanvas::Ellipse(int x1, int y1, int x2, int y2)     { nv::TCanvas_Ellipse(handle_, x1, y1, x2, y2); }
void TCanvas::TextOut(int x, int y, const std::string& t) { nv::TCanvas_TextOut(handle_, x, y, t.c_str()); }
void TCanvas::FillRect(const TRect& Rect)                 { nv::TCanvas_FillRect(handle_, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom); }

void TCanvas::Draw(int X, int Y, const TGraphic* Graphic)
{
    if (Graphic)
        nv::TCanvas_Draw(handle_, X, Y, Graphic->Current());
}

void TCanvas::StretchDraw(const TRect& Rect, const TGraphic* Graphic)
{
    if (Graphic)
        nv::TCanvas_StretchDraw(handle_, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom, Graphic->Current());
}

TColor TCanvas::GetPixelsImpl(TObject* owner, int X, int Y) { return static_cast<TColor>(nv::TCanvas_GetPixels(owner->Handle(), X, Y)); }
void   TCanvas::SetPixelsImpl(TObject* owner, int X, int Y, const TColor& value) { nv::TCanvas_SetPixels(owner->Handle(), X, Y, value); }

/* ---------------- Graphics ---------------- */

TCanvas* CanvasHolder::Get(no_vcl_obj_t canvas)
{
    if (!canvas)
        return nullptr;
    // 同じアドレスに別の Canvas が作られることもあるため、Pen・Brush・Font のハンドルも確かめる。
    if (!canvas_ || canvas_->Handle() != canvas
        || canvas_->Pen.Handle() != nv::TCanvas_GetPen(canvas)
        || canvas_->Brush.Handle() != nv::TCanvas_GetBrush(canvas)
        || canvas_->Font.Handle() != nv::TCanvas_GetFont(canvas))
        canvas_.reset(new TCanvas(canvas));
    return canvas_.get();
}

TGraphic::TGraphic(no_vcl_obj_t handle)
    : TGraphic(this, &TGraphic::SelfAccessor)
{
    handle_ = handle;
    owns_ = true;
}

TGraphic::TGraphic(TObject* owner, Accessor accessor)
    : TPersistent(nullptr)
    , Width(this, &TGraphic::GetWidthImpl, &TGraphic::SetWidthImpl)
    , Height(this, &TGraphic::GetHeightImpl, &TGraphic::SetHeightImpl)
    , Empty(this, &TGraphic::GetEmptyImpl)
    , Transparent(this, &TGraphic::GetTransparentImpl, &TGraphic::SetTransparentImpl)
    , owner_(owner)
    , accessor_(accessor)
    , owns_(false)
{}

TGraphic::~TGraphic()
{
    if (owns_)
        no_vcl_TGraphic_Destroy(handle_);
}

void TGraphic::LoadFromFile(const std::string& FileName)     { nv::TGraphic_LoadFromFile(Current(), FileName.c_str()); }
void TGraphic::SaveToFile(const std::string& FileName) const { nv::TGraphic_SaveToFile(Current(), FileName.c_str()); }
void TGraphic::Assign(const TGraphic* Source)                { nv::TGraphic_Assign(Current(), Source ? Source->Current() : nullptr); }
void TGraphic::Clear()                                       { nv::TGraphic_Clear(Current()); }

int  TGraphic::GetWidthImpl(TObject* owner)                   { return nv::TGraphic_GetWidth(static_cast<TGraphic*>(owner)->Current()); }
void TGraphic::SetWidthImpl(TObject* owner, const int& value) { nv::TGraphic_SetWidth(static_cast<TGraphic*>(owner)->Current(), value); }
int  TGraphic::GetHeightImpl(TObject* owner)                  { return nv::TGraphic_GetHeight(static_cast<TGraphic*>(owner)->Current()); }
void TGraphic::SetHeightImpl(TObject* owner, const int& value) { nv::TGraphic_SetHeight(static_cast<TGraphic*>(owner)->Current(), value); }
bool TGraphic::GetEmptyImpl(TObject* owner)                   { return nv::TGraphic_GetEmpty(static_cast<TGraphic*>(owner)->Current()) != 0; }
bool TGraphic::GetTransparentImpl(TObject* owner)             { return nv::TGraphic_GetTransparent(static_cast<TGraphic*>(owner)->Current()) != 0; }
void TGraphic::SetTransparentImpl(TObject* owner, const bool& value)
{
    nv::TGraphic_SetTransparent(static_cast<TGraphic*>(owner)->Current(), value ? 1 : 0);
}

TRasterImage::TRasterImage(no_vcl_obj_t handle)
    : TGraphic(handle)
    , Canvas(this, &TRasterImage::GetCanvasImpl)
    , PixelFormat(this, &TRasterImage::GetPixelFormatImpl, &TRasterImage::SetPixelFormatImpl)
    , TransparentColor(this, &TRasterImage::GetTransparentColorImpl, &TRasterImage::SetTransparentColorImpl)
    , TransparentMode(this, &TRasterImage::GetTransparentModeImpl, &TRasterImage::SetTransparentModeImpl)
{}

TRasterImage::TRasterImage(TObject* owner, Accessor accessor)
    : TGraphic(owner, accessor)
    , Canvas(this, &TRasterImage::GetCanvasImpl)
    , PixelFormat(this, &TRasterImage::GetPixelFormatImpl, &TRasterImage::SetPixelFormatImpl)
    , TransparentColor(this, &TRasterImage::GetTransparentColorImpl, &TRasterImage::SetTransparentColorImpl)
    , TransparentMode(this, &TRasterImage::GetTransparentModeImpl, &TRasterImage::SetTransparentModeImpl)
{}

TCanvas* TRasterImage::GetCanvasImpl(TObject* owner)
{
    TRasterImage* self = static_cast<TRasterImage*>(owner);
    return self->canvas_.Get(nv::TRasterImage_GetCanvas(self->Current()));
}

TPixelFormat TRasterImage::GetPixelFormatImpl(TObject* owner)
{
    return static_cast<TPixelFormat>(nv::TRasterImage_GetPixelFormat(static_cast<TRasterImage*>(owner)->Current()));
}
void TRasterImage::SetPixelFormatImpl(TObject* owner, const TPixelFormat& value)
{
    nv::TRasterImage_SetPixelFormat(static_cast<TRasterImage*>(owner)->Current(), value);
}
TColor TRasterImage::GetTransparentColorImpl(TObject* owner)
{
    return static_cast<TColor>(nv::TRasterImage_GetTransparentColor(static_cast<TRasterImage*>(owner)->Current()));
}
void TRasterImage::SetTransparentColorImpl(TObject* owner, const TColor& value)
{
    nv::TRasterImage_SetTransparentColor(static_cast<TRasterImage*>(owner)->Current(), value);
}
TTransparentMode TRasterImage::GetTransparentModeImpl(TObject* owner)
{
    return static_cast<TTransparentMode>(nv::TRasterImage_GetTransparentMode(static_cast<TRasterImage*>(owner)->Current()));
}
void TRasterImage::SetTransparentModeImpl(TObject* owner, const TTransparentMode& value)
{
    nv::TRasterImage_SetTransparentMode(static_cast<TRasterImage*>(owner)->Current(), value);
}

void TCustomBitmap::SetSize(int AWidth, int AHeight) { nv::TCustomBitmap_SetSize(Current(), AWidth, AHeight); }

TBitmap::TBitmap() : TCustomBitmap(nv::TBitmap_Create()) {}

TPortableNetworkGraphic::TPortableNetworkGraphic() : TCustomBitmap(nv::TPortableNetworkGraphic_Create()) {}

TJPEGImage::TJPEGImage()
    : TCustomBitmap(nv::TJPEGImage_Create())
    , CompressionQuality(this, &TJPEGImage::GetCompressionQualityImpl, &TJPEGImage::SetCompressionQualityImpl)
{}

TJPEGImage::TJPEGImage(TObject* owner, Accessor accessor)
    : TCustomBitmap(owner, accessor)
    , CompressionQuality(this, &TJPEGImage::GetCompressionQualityImpl, &TJPEGImage::SetCompressionQualityImpl)
{}

int TJPEGImage::GetCompressionQualityImpl(TObject* owner)
{
    return nv::TJPEGImage_GetCompressionQuality(static_cast<TJPEGImage*>(owner)->Current());
}
void TJPEGImage::SetCompressionQualityImpl(TObject* owner, const int& value)
{
    nv::TJPEGImage_SetCompressionQuality(static_cast<TJPEGImage*>(owner)->Current(), value);
}

TPicture::TPicture() : TPicture(nv::TPicture_Create(), true) {}

TPicture::TPicture(no_vcl_obj_t handle, bool owns)
    : TPersistent(handle)
    , Graphic(this, &TPicture::GetGraphicImpl, &TPicture::SetGraphicImpl)
    , Bitmap(this, &TPicture::GetBitmapImpl, &TPicture::SetBitmapImpl)
    , PNG(this, &TPicture::GetPNGImpl, &TPicture::SetPNGImpl)
    , Jpeg(this, &TPicture::GetJpegImpl, &TPicture::SetJpegImpl)
    , Width(this, &TPicture::GetWidthImpl)
    , Height(this, &TPicture::GetHeightImpl)
    , owns_(owns)
    , graphic_(this, &no_vcl_TPicture_GetGraphic)
    , bitmap_(this, &no_vcl_TPicture_GetBitmap)
    , png_(this, &no_vcl_TPicture_GetPNG)
    , jpeg_(this, &no_vcl_TPicture_GetJpeg)
{}

TPicture::~TPicture()
{
    if (owns_)
        no_vcl_TPicture_Destroy(handle_);
}

void TPicture::LoadFromFile(const std::string& FileName)     { nv::TPicture_LoadFromFile(handle_, FileName.c_str()); }
void TPicture::SaveToFile(const std::string& FileName) const { nv::TPicture_SaveToFile(handle_, FileName.c_str()); }
void TPicture::Assign(const TPicture* Source)                { nv::TPicture_Assign(handle_, Source ? Source->Handle() : nullptr); }
void TPicture::Clear()                                       { nv::TPicture_Clear(handle_); }

// 空の TPicture の Graphic は nullptr(VCL と同じ)。それ以外は、クラスを問わないビューを返す。
TGraphic* TPicture::GetGraphicImpl(TObject* owner)
{
    TPicture* self = static_cast<TPicture*>(owner);
    return nv::TPicture_GetGraphic(self->handle_) ? &self->graphic_ : nullptr;
}
void TPicture::SetGraphicImpl(TObject* owner, TGraphic* const& value)
{
    nv::TPicture_SetGraphic(owner->Handle(), value ? value->Current() : nullptr);
}
// Bitmap・PNG・Jpeg は、ビューを返すだけで中身には触れない(変換はビューを操作したときに LCL が行う)。
TBitmap* TPicture::GetBitmapImpl(TObject* owner) { return &static_cast<TPicture*>(owner)->bitmap_; }
void TPicture::SetBitmapImpl(TObject* owner, TBitmap* const& value)
{
    nv::TPicture_SetGraphic(owner->Handle(), value ? value->Current() : nullptr);
}
TPortableNetworkGraphic* TPicture::GetPNGImpl(TObject* owner) { return &static_cast<TPicture*>(owner)->png_; }
void TPicture::SetPNGImpl(TObject* owner, TPortableNetworkGraphic* const& value)
{
    nv::TPicture_SetGraphic(owner->Handle(), value ? value->Current() : nullptr);
}
TJPEGImage* TPicture::GetJpegImpl(TObject* owner) { return &static_cast<TPicture*>(owner)->jpeg_; }
void TPicture::SetJpegImpl(TObject* owner, TJPEGImage* const& value)
{
    nv::TPicture_SetGraphic(owner->Handle(), value ? value->Current() : nullptr);
}
int TPicture::GetWidthImpl(TObject* owner)  { return nv::TPicture_GetWidth(owner->Handle()); }
int TPicture::GetHeightImpl(TObject* owner) { return nv::TPicture_GetHeight(owner->Handle()); }

TPaintBox::TPaintBox(TComponent* AOwner)
    : TGraphicControl(nv::TPaintBox_Create(HandleOf(AOwner)))
    , Canvas(nv::TPaintBox_GetCanvas(handle_))
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
        nv::TPaintBox_SetOnPaint(self->handle_, &TPaintBox::PaintTrampoline, nullptr);
        self->onPaintHooked_ = true;
    }
}

void NO_VCL_CALL TPaintBox::PaintTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        TPaintBox* self = static_cast<TPaintBox*>(FromHandle(sender));
        if (!self || !self->onPaint_)
            return;
        TNotifyEvent handler = self->onPaint_;
        handler(self);
    });
}

/* ---------------- Image ---------------- */

TCustomImage::TCustomImage(no_vcl_obj_t handle)
    : TGraphicControl(handle)
    , Picture(this, &TCustomImage::GetPictureImpl, &TCustomImage::SetPictureImpl)
    , Canvas(this, &TCustomImage::GetCanvasImpl)
    , HasGraphic(this, &TCustomImage::GetHasGraphicImpl)
    , Center(this, &TCustomImage::GetCenterImpl, &TCustomImage::SetCenterImpl)
    , Stretch(this, &TCustomImage::GetStretchImpl, &TCustomImage::SetStretchImpl)
    , StretchOutEnabled(this, &TCustomImage::GetStretchOutEnabledImpl, &TCustomImage::SetStretchOutEnabledImpl)
    , StretchInEnabled(this, &TCustomImage::GetStretchInEnabledImpl, &TCustomImage::SetStretchInEnabledImpl)
    , Proportional(this, &TCustomImage::GetProportionalImpl, &TCustomImage::SetProportionalImpl)
    , Transparent(this, &TCustomImage::GetTransparentImpl, &TCustomImage::SetTransparentImpl)
    , OnPictureChanged(this, &TCustomImage::GetOnPictureChangedImpl, &TCustomImage::SetOnPictureChangedImpl)
    , Images(this, &TCustomImage::GetImagesImpl, &TCustomImage::SetImagesImpl)
    , ImageIndex(this, &TCustomImage::GetImageIndexImpl, &TCustomImage::SetImageIndexImpl)
    , picture_(nv::TCustomImage_GetPicture(handle), false)
{}

TPicture* TCustomImage::GetPictureImpl(TObject* owner) { return &static_cast<TCustomImage*>(owner)->picture_; }
void TCustomImage::SetPictureImpl(TObject* owner, TPicture* const& value)
{
    nv::TCustomImage_SetPicture(owner->Handle(), value ? value->Handle() : nullptr);
}
TCanvas* TCustomImage::GetCanvasImpl(TObject* owner)
{
    TCustomImage* self = static_cast<TCustomImage*>(owner);
    return self->canvas_.Get(nv::TCustomImage_GetCanvas(self->handle_));
}
bool TCustomImage::GetHasGraphicImpl(TObject* owner) { return nv::TCustomImage_GetHasGraphic(owner->Handle()) != 0; }
bool TCustomImage::GetCenterImpl(TObject* owner)     { return nv::TCustomImage_GetCenter(owner->Handle()) != 0; }
void TCustomImage::SetCenterImpl(TObject* owner, const bool& value) { nv::TCustomImage_SetCenter(owner->Handle(), value ? 1 : 0); }
bool TCustomImage::GetStretchImpl(TObject* owner)    { return nv::TCustomImage_GetStretch(owner->Handle()) != 0; }
void TCustomImage::SetStretchImpl(TObject* owner, const bool& value) { nv::TCustomImage_SetStretch(owner->Handle(), value ? 1 : 0); }
bool TCustomImage::GetStretchOutEnabledImpl(TObject* owner) { return nv::TCustomImage_GetStretchOutEnabled(owner->Handle()) != 0; }
void TCustomImage::SetStretchOutEnabledImpl(TObject* owner, const bool& value)
{
    nv::TCustomImage_SetStretchOutEnabled(owner->Handle(), value ? 1 : 0);
}
bool TCustomImage::GetStretchInEnabledImpl(TObject* owner) { return nv::TCustomImage_GetStretchInEnabled(owner->Handle()) != 0; }
void TCustomImage::SetStretchInEnabledImpl(TObject* owner, const bool& value)
{
    nv::TCustomImage_SetStretchInEnabled(owner->Handle(), value ? 1 : 0);
}
bool TCustomImage::GetProportionalImpl(TObject* owner) { return nv::TCustomImage_GetProportional(owner->Handle()) != 0; }
void TCustomImage::SetProportionalImpl(TObject* owner, const bool& value) { nv::TCustomImage_SetProportional(owner->Handle(), value ? 1 : 0); }
bool TCustomImage::GetTransparentImpl(TObject* owner)  { return nv::TCustomImage_GetTransparent(owner->Handle()) != 0; }
void TCustomImage::SetTransparentImpl(TObject* owner, const bool& value) { nv::TCustomImage_SetTransparent(owner->Handle(), value ? 1 : 0); }

void NO_VCL_CALL TCustomImage::PictureChangedTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomImage* self = static_cast<TCustomImage*>(FromHandle(sender)))
            CallNotify(self->onPictureChanged_, self);
    });
}

TNotifyEvent TCustomImage::GetOnPictureChangedImpl(TObject* owner) { return static_cast<TCustomImage*>(owner)->onPictureChanged_; }

void TCustomImage::SetOnPictureChangedImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomImage* self = static_cast<TCustomImage*>(owner);
    SetSimpleEvent(self->handle_, self->onPictureChanged_, self->onPictureChangedHooked_, value,
                   &no_vcl_TCustomImage_SetOnPictureChanged, &TCustomImage::PictureChangedTrampoline);
}

TImage::TImage(TComponent* AOwner)
    : TCustomImage(nv::TImage_Create(HandleOf(AOwner)))
{}

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
        nv::TCustomTimer_SetOnTimer(self->handle_, &TCustomTimer::TimerTrampoline, nullptr);
        self->onTimerHooked_ = true;
    }
}

void NO_VCL_CALL TCustomTimer::TimerTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        TCustomTimer* self = static_cast<TCustomTimer*>(FromHandle(sender));
        if (!self || !self->onTimer_)
            return;
        TNotifyEvent handler = self->onTimer_;
        handler(self);
    });
}

int  TCustomTimer::GetIntervalImpl(TObject* owner)                   { return nv::TCustomTimer_GetInterval(owner->Handle()); }
void TCustomTimer::SetIntervalImpl(TObject* owner, const int& value) { nv::TCustomTimer_SetInterval(owner->Handle(), value); }
bool TCustomTimer::GetEnabledImpl(TObject* owner)                    { return nv::TCustomTimer_GetEnabled(owner->Handle()) != 0; }
void TCustomTimer::SetEnabledImpl(TObject* owner, const bool& value) { nv::TCustomTimer_SetEnabled(owner->Handle(), value ? 1 : 0); }

TTimer::TTimer(TComponent* AOwner)
    : TCustomTimer(nv::TTimer_Create(HandleOf(AOwner)))
{}

/* ---------------- ImageList ---------------- */

TCustomImageList::TCustomImageList(no_vcl_obj_t handle)
    : TComponent(handle)
    , Width(this, &TCustomImageList::GetWidthImpl, &TCustomImageList::SetWidthImpl)
    , Height(this, &TCustomImageList::GetHeightImpl, &TCustomImageList::SetHeightImpl)
    , Count(this, &TCustomImageList::GetCountImpl)
    , Masked(this, &TCustomImageList::GetMaskedImpl, &TCustomImageList::SetMaskedImpl)
    , BkColor(this, &TCustomImageList::GetBkColorImpl, &TCustomImageList::SetBkColorImpl)
    , DrawingStyle(this, &TCustomImageList::GetDrawingStyleImpl, &TCustomImageList::SetDrawingStyleImpl)
    , OnChange(this, &TCustomImageList::GetOnChangeImpl, &TCustomImageList::SetOnChangeImpl)
{}

int TCustomImageList::Add(const TCustomBitmap* Image, const TCustomBitmap* Mask)
{
    return nv::TCustomImageList_Add(handle_, Image ? Image->Current() : nullptr, Mask ? Mask->Current() : nullptr);
}
int TCustomImageList::AddSliced(const TCustomBitmap* Image, int AHorizontalCount, int AVerticalCount)
{
    return nv::TCustomImageList_AddSliced(handle_, Image ? Image->Current() : nullptr, AHorizontalCount, AVerticalCount);
}
int TCustomImageList::AddMasked(const TBitmap* Image, TColor MaskColor)
{
    return nv::TCustomImageList_AddMasked(handle_, Image ? Image->Current() : nullptr, MaskColor);
}
void TCustomImageList::Insert(int Index, const TCustomBitmap* Image, const TCustomBitmap* Mask)
{
    nv::TCustomImageList_Insert(handle_, Index, Image ? Image->Current() : nullptr, Mask ? Mask->Current() : nullptr);
}
void TCustomImageList::Replace(int Index, const TCustomBitmap* Image, const TCustomBitmap* Mask)
{
    nv::TCustomImageList_Replace(handle_, Index, Image ? Image->Current() : nullptr, Mask ? Mask->Current() : nullptr);
}
void TCustomImageList::Delete(int Index)                  { nv::TCustomImageList_Delete(handle_, Index); }
void TCustomImageList::Clear()                            { nv::TCustomImageList_Clear(handle_); }
void TCustomImageList::Move(int CurIndex, int NewIndex)   { nv::TCustomImageList_Move(handle_, CurIndex, NewIndex); }
void TCustomImageList::GetBitmap(int Index, TCustomBitmap* Image) const
{
    if (Image)
        nv::TCustomImageList_GetBitmap(handle_, Index, Image->Current());
}
void TCustomImageList::Draw(TCanvas* Canvas, int X, int Y, int Index, bool Enabled) const
{
    if (Canvas)
        nv::TCustomImageList_Draw(handle_, Canvas->Handle(), X, Y, Index, Enabled ? 1 : 0);
}
void TCustomImageList::BeginUpdate() { nv::TCustomImageList_BeginUpdate(handle_); }
void TCustomImageList::EndUpdate()   { nv::TCustomImageList_EndUpdate(handle_); }

int    TCustomImageList::GetWidthImpl(TObject* owner)                   { return nv::TCustomImageList_GetWidth(owner->Handle()); }
void   TCustomImageList::SetWidthImpl(TObject* owner, const int& value) { nv::TCustomImageList_SetWidth(owner->Handle(), value); }
int    TCustomImageList::GetHeightImpl(TObject* owner)                  { return nv::TCustomImageList_GetHeight(owner->Handle()); }
void   TCustomImageList::SetHeightImpl(TObject* owner, const int& value) { nv::TCustomImageList_SetHeight(owner->Handle(), value); }
int    TCustomImageList::GetCountImpl(TObject* owner)                   { return nv::TCustomImageList_GetCount(owner->Handle()); }
bool   TCustomImageList::GetMaskedImpl(TObject* owner)                  { return nv::TCustomImageList_GetMasked(owner->Handle()) != 0; }
void   TCustomImageList::SetMaskedImpl(TObject* owner, const bool& value) { nv::TCustomImageList_SetMasked(owner->Handle(), value ? 1 : 0); }
TColor TCustomImageList::GetBkColorImpl(TObject* owner)                 { return static_cast<TColor>(nv::TCustomImageList_GetBkColor(owner->Handle())); }
void   TCustomImageList::SetBkColorImpl(TObject* owner, const TColor& value) { nv::TCustomImageList_SetBkColor(owner->Handle(), value); }
TDrawingStyle TCustomImageList::GetDrawingStyleImpl(TObject* owner)
{
    return static_cast<TDrawingStyle>(nv::TCustomImageList_GetDrawingStyle(owner->Handle()));
}
void TCustomImageList::SetDrawingStyleImpl(TObject* owner, const TDrawingStyle& value)
{
    nv::TCustomImageList_SetDrawingStyle(owner->Handle(), value);
}

void NO_VCL_CALL TCustomImageList::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomImageList* self = static_cast<TCustomImageList*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TNotifyEvent TCustomImageList::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomImageList*>(owner)->onChange_; }

void TCustomImageList::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomImageList* self = static_cast<TCustomImageList*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TCustomImageList_SetOnChange, &TCustomImageList::ChangeTrampoline);
}

TImageList::TImageList(TComponent* AOwner)
    : TCustomImageList(nv::TImageList_Create(HandleOf(AOwner)))
{}

/* ---------------- メニュー ---------------- */

TShortCut ShortCut(unsigned short Key, TShiftState Shift)
{
    return static_cast<TShortCut>(nv::ShortCut_Make(Key, static_cast<no_vcl_int_t>(Shift)));
}

TShortCut TextToShortCut(const std::string& Text)
{
    return static_cast<TShortCut>(nv::ShortCut_FromText(Text.c_str()));
}

std::string ShortCutToText(TShortCut ShortCut)
{
    return std::string(nv::ShortCut_ToText(ShortCut));
}

TMenuItem::TMenuItem(TComponent* AOwner)
    : TMenuItem(nv::TMenuItem_Create(HandleOf(AOwner)))
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
    , ImageIndex(this, &TMenuItem::GetImageIndexImpl, &TMenuItem::SetImageIndexImpl)
    , SubMenuImages(this, &TMenuItem::GetSubMenuImagesImpl, &TMenuItem::SetSubMenuImagesImpl)
    , Bitmap(this, &TMenuItem::GetBitmapImpl, &TMenuItem::SetBitmapImpl)
    , bitmap_(this, &no_vcl_TMenuItem_GetBitmap)
{}

TMenuItem* TMenuItem::GetItemsImpl(TObject* owner, int Index) { return WrapExisting<TMenuItem>(nv::TMenuItem_GetItem(owner->Handle(), Index)); }
void TMenuItem::Add(TMenuItem* Item)              { nv::TMenuItem_Add(handle_, HandleOf(Item)); }
void TMenuItem::Insert(int Index, TMenuItem* Item) { nv::TMenuItem_Insert(handle_, Index, HandleOf(Item)); }
void TMenuItem::Delete(int Index)                 { nv::TMenuItem_Delete(handle_, Index); }
void TMenuItem::Remove(TMenuItem* Item)           { nv::TMenuItem_Remove(handle_, HandleOf(Item)); }
void TMenuItem::Clear()                           { nv::TMenuItem_Clear(handle_); }
int  TMenuItem::IndexOf(TMenuItem* Item) const    { return nv::TMenuItem_IndexOf(handle_, HandleOf(Item)); }
void TMenuItem::AddSeparator()                    { nv::TMenuItem_AddSeparator(handle_); }
bool TMenuItem::IsLine() const                    { return nv::TMenuItem_IsLine(handle_) != 0; }
void TMenuItem::Click()                           { nv::TMenuItem_Click(handle_); }

std::string TMenuItem::GetCaptionImpl(TObject* owner) { return std::string(nv::TMenuItem_GetCaption(owner->Handle())); }
void TMenuItem::SetCaptionImpl(TObject* owner, const std::string& value) { nv::TMenuItem_SetCaption(owner->Handle(), value.c_str()); }
bool TMenuItem::GetCheckedImpl(TObject* owner)                     { return nv::TMenuItem_GetChecked(owner->Handle()) != 0; }
void TMenuItem::SetCheckedImpl(TObject* owner, const bool& value)   { nv::TMenuItem_SetChecked(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetEnabledImpl(TObject* owner)                     { return nv::TMenuItem_GetEnabled(owner->Handle()) != 0; }
void TMenuItem::SetEnabledImpl(TObject* owner, const bool& value)   { nv::TMenuItem_SetEnabled(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetVisibleImpl(TObject* owner)                     { return nv::TMenuItem_GetVisible(owner->Handle()) != 0; }
void TMenuItem::SetVisibleImpl(TObject* owner, const bool& value)   { nv::TMenuItem_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetAutoCheckImpl(TObject* owner)                   { return nv::TMenuItem_GetAutoCheck(owner->Handle()) != 0; }
void TMenuItem::SetAutoCheckImpl(TObject* owner, const bool& value) { nv::TMenuItem_SetAutoCheck(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetRadioItemImpl(TObject* owner)                   { return nv::TMenuItem_GetRadioItem(owner->Handle()) != 0; }
void TMenuItem::SetRadioItemImpl(TObject* owner, const bool& value) { nv::TMenuItem_SetRadioItem(owner->Handle(), value ? 1 : 0); }
int  TMenuItem::GetGroupIndexImpl(TObject* owner)                  { return nv::TMenuItem_GetGroupIndex(owner->Handle()); }
void TMenuItem::SetGroupIndexImpl(TObject* owner, const int& value) { nv::TMenuItem_SetGroupIndex(owner->Handle(), value); }
bool TMenuItem::GetDefaultImpl(TObject* owner)                     { return nv::TMenuItem_GetDefault(owner->Handle()) != 0; }
void TMenuItem::SetDefaultImpl(TObject* owner, const bool& value)   { nv::TMenuItem_SetDefault(owner->Handle(), value ? 1 : 0); }
TShortCut TMenuItem::GetShortCutImpl(TObject* owner) { return static_cast<TShortCut>(nv::TMenuItem_GetShortCut(owner->Handle())); }
void TMenuItem::SetShortCutImpl(TObject* owner, const TShortCut& value) { nv::TMenuItem_SetShortCut(owner->Handle(), value); }
std::string TMenuItem::GetHintImpl(TObject* owner) { return std::string(nv::TMenuItem_GetHint(owner->Handle())); }
void TMenuItem::SetHintImpl(TObject* owner, const std::string& value) { nv::TMenuItem_SetHint(owner->Handle(), value.c_str()); }
int  TMenuItem::GetCountImpl(TObject* owner) { return nv::TMenuItem_GetCount(owner->Handle()); }
TMenuItem* TMenuItem::GetParentImpl(TObject* owner) { return WrapExisting<TMenuItem>(nv::TMenuItem_GetParent(owner->Handle())); }

TNotifyEvent TMenuItem::GetOnClickImpl(TObject* owner) { return static_cast<TMenuItem*>(owner)->onClick_; }

void TMenuItem::SetOnClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TMenuItem* self = static_cast<TMenuItem*>(owner);
    SetSimpleEvent(self->handle_, self->onClick_, self->onClickHooked_, value,
                   &no_vcl_TMenuItem_SetOnClick, &TMenuItem::ClickTrampoline);
}

void NO_VCL_CALL TMenuItem::ClickTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TMenuItem* self = static_cast<TMenuItem*>(FromHandle(sender)))
            CallNotify(self->onClick_, self);
    });
}

TMenu::TMenu(no_vcl_obj_t handle)
    : TComponent(handle)
    , Items(this, &TMenu::GetItemsImpl)
    , Images(this, &TMenu::GetImagesImpl, &TMenu::SetImagesImpl)
{}

TMenuItem* TMenu::GetItemsImpl(TObject* owner) { return WrapExisting<TMenuItem>(nv::TMenu_GetItems(owner->Handle())); }

TMainMenu::TMainMenu(TComponent* AOwner)
    : TMenu(nv::TMainMenu_Create(HandleOf(AOwner)))
{}

TPopupMenu::TPopupMenu(TComponent* AOwner)
    : TMenu(nv::TPopupMenu_Create(HandleOf(AOwner)))
    , AutoPopup(this, &TPopupMenu::GetAutoPopupImpl, &TPopupMenu::SetAutoPopupImpl)
    , PopupComponent(this, &TPopupMenu::GetPopupComponentImpl, &TPopupMenu::SetPopupComponentImpl)
    , OnPopup(this, &TPopupMenu::GetOnPopupImpl, &TPopupMenu::SetOnPopupImpl)
    , OnClose(this, &TPopupMenu::GetOnCloseImpl, &TPopupMenu::SetOnCloseImpl)
{}

void TPopupMenu::Popup(int X, int Y) { nv::TPopupMenu_Popup(handle_, X, Y); }

bool TPopupMenu::GetAutoPopupImpl(TObject* owner)                   { return nv::TPopupMenu_GetAutoPopup(owner->Handle()) != 0; }
void TPopupMenu::SetAutoPopupImpl(TObject* owner, const bool& value) { nv::TPopupMenu_SetAutoPopup(owner->Handle(), value ? 1 : 0); }
TComponent* TPopupMenu::GetPopupComponentImpl(TObject* owner) { return FromHandle(nv::TPopupMenu_GetPopupComponent(owner->Handle())); }
void TPopupMenu::SetPopupComponentImpl(TObject* owner, TComponent* const& value) { nv::TPopupMenu_SetPopupComponent(owner->Handle(), HandleOf(value)); }

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
    GuardCallback([&] {
        if (TPopupMenu* self = static_cast<TPopupMenu*>(FromHandle(sender)))
            CallNotify(self->onPopup_, self);
    });
}

void NO_VCL_CALL TPopupMenu::CloseTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TPopupMenu* self = static_cast<TPopupMenu*>(FromHandle(sender)))
            CallNotify(self->onClose_, self);
    });
}

/* ---------------- Grid ---------------- */

void TCustomGrid::BeginUpdate() { nv::TCustomGrid_BeginUpdate(handle_); }
void TCustomGrid::EndUpdate()   { nv::TCustomGrid_EndUpdate(handle_); }
void TCustomGrid::Clear()       { nv::TCustomGrid_Clear(handle_); }

TRect TCustomGrid::CellRect(int ACol, int ARow) const
{
    TRect r{};
    nv::TCustomGrid_CellRect(handle_, ACol, ARow, &r.Left, &r.Top, &r.Right, &r.Bottom);
    return r;
}

void TCustomGrid::MouseToCell(int X, int Y, int& ACol, int& ARow) const
{
    nv::TCustomGrid_MouseToCell(handle_, X, Y, &ACol, &ARow);
}

TCustomDrawGrid::TCustomDrawGrid(no_vcl_obj_t handle)
    : TCustomGrid(handle)
    , Canvas(nv::TCustomDrawGrid_GetCanvas(handle))
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

int  TCustomDrawGrid::GetColWidthsImpl(TObject* owner, int ACol)                     { return nv::TCustomDrawGrid_GetColWidths(owner->Handle(), ACol); }
void TCustomDrawGrid::SetColWidthsImpl(TObject* owner, int ACol, const int& value)   { nv::TCustomDrawGrid_SetColWidths(owner->Handle(), ACol, value); }
int  TCustomDrawGrid::GetRowHeightsImpl(TObject* owner, int ARow)                    { return nv::TCustomDrawGrid_GetRowHeights(owner->Handle(), ARow); }
void TCustomDrawGrid::SetRowHeightsImpl(TObject* owner, int ARow, const int& value)  { nv::TCustomDrawGrid_SetRowHeights(owner->Handle(), ARow, value); }

void TCustomDrawGrid::InsertColRow(bool IsColumn, int Index) { nv::TCustomDrawGrid_InsertColRow(handle_, IsColumn ? 1 : 0, Index); }
void TCustomDrawGrid::DeleteColRow(bool IsColumn, int Index) { nv::TCustomDrawGrid_DeleteColRow(handle_, IsColumn ? 1 : 0, Index); }
void TCustomDrawGrid::SortColRow(bool IsColumn, int Index)   { nv::TCustomDrawGrid_SortColRow(handle_, IsColumn ? 1 : 0, Index); }

void TCustomDrawGrid::MoveColRow(bool IsColumn, int FromIndex, int ToIndex)
{
    nv::TCustomDrawGrid_MoveColRow(handle_, IsColumn ? 1 : 0, FromIndex, ToIndex);
}

int  TCustomDrawGrid::GetColCountImpl(TObject* owner)                        { return nv::TCustomDrawGrid_GetColCount(owner->Handle()); }
void TCustomDrawGrid::SetColCountImpl(TObject* owner, const int& value)       { nv::TCustomDrawGrid_SetColCount(owner->Handle(), value); }
int  TCustomDrawGrid::GetRowCountImpl(TObject* owner)                        { return nv::TCustomDrawGrid_GetRowCount(owner->Handle()); }
void TCustomDrawGrid::SetRowCountImpl(TObject* owner, const int& value)       { nv::TCustomDrawGrid_SetRowCount(owner->Handle(), value); }
int  TCustomDrawGrid::GetFixedColsImpl(TObject* owner)                       { return nv::TCustomDrawGrid_GetFixedCols(owner->Handle()); }
void TCustomDrawGrid::SetFixedColsImpl(TObject* owner, const int& value)      { nv::TCustomDrawGrid_SetFixedCols(owner->Handle(), value); }
int  TCustomDrawGrid::GetFixedRowsImpl(TObject* owner)                       { return nv::TCustomDrawGrid_GetFixedRows(owner->Handle()); }
void TCustomDrawGrid::SetFixedRowsImpl(TObject* owner, const int& value)      { nv::TCustomDrawGrid_SetFixedRows(owner->Handle(), value); }
int  TCustomDrawGrid::GetColImpl(TObject* owner)                             { return nv::TCustomDrawGrid_GetCol(owner->Handle()); }
void TCustomDrawGrid::SetColImpl(TObject* owner, const int& value)            { nv::TCustomDrawGrid_SetCol(owner->Handle(), value); }
int  TCustomDrawGrid::GetRowImpl(TObject* owner)                             { return nv::TCustomDrawGrid_GetRow(owner->Handle()); }
void TCustomDrawGrid::SetRowImpl(TObject* owner, const int& value)            { nv::TCustomDrawGrid_SetRow(owner->Handle(), value); }
int  TCustomDrawGrid::GetDefaultColWidthImpl(TObject* owner)                 { return nv::TCustomDrawGrid_GetDefaultColWidth(owner->Handle()); }
void TCustomDrawGrid::SetDefaultColWidthImpl(TObject* owner, const int& value) { nv::TCustomDrawGrid_SetDefaultColWidth(owner->Handle(), value); }
int  TCustomDrawGrid::GetDefaultRowHeightImpl(TObject* owner)                { return nv::TCustomDrawGrid_GetDefaultRowHeight(owner->Handle()); }
void TCustomDrawGrid::SetDefaultRowHeightImpl(TObject* owner, const int& value) { nv::TCustomDrawGrid_SetDefaultRowHeight(owner->Handle(), value); }
TGridOptions TCustomDrawGrid::GetOptionsImpl(TObject* owner)                  { return nv::TCustomDrawGrid_GetOptions(owner->Handle()); }
void TCustomDrawGrid::SetOptionsImpl(TObject* owner, const TGridOptions& value) { nv::TCustomDrawGrid_SetOptions(owner->Handle(), value); }
int  TCustomDrawGrid::GetLeftColImpl(TObject* owner)                         { return nv::TCustomDrawGrid_GetLeftCol(owner->Handle()); }
void TCustomDrawGrid::SetLeftColImpl(TObject* owner, const int& value)        { nv::TCustomDrawGrid_SetLeftCol(owner->Handle(), value); }
int  TCustomDrawGrid::GetTopRowImpl(TObject* owner)                          { return nv::TCustomDrawGrid_GetTopRow(owner->Handle()); }
void TCustomDrawGrid::SetTopRowImpl(TObject* owner, const int& value)         { nv::TCustomDrawGrid_SetTopRow(owner->Handle(), value); }
bool TCustomDrawGrid::GetDefaultDrawingImpl(TObject* owner)                  { return nv::TCustomDrawGrid_GetDefaultDrawing(owner->Handle()) != 0; }
void TCustomDrawGrid::SetDefaultDrawingImpl(TObject* owner, const bool& value) { nv::TCustomDrawGrid_SetDefaultDrawing(owner->Handle(), value ? 1 : 0); }
TColor TCustomDrawGrid::GetFixedColorImpl(TObject* owner)                    { return nv::TCustomDrawGrid_GetFixedColor(owner->Handle()); }
void TCustomDrawGrid::SetFixedColorImpl(TObject* owner, const TColor& value)  { nv::TCustomDrawGrid_SetFixedColor(owner->Handle(), value); }
bool TCustomDrawGrid::GetEditorModeImpl(TObject* owner)                      { return nv::TCustomDrawGrid_GetEditorMode(owner->Handle()) != 0; }
void TCustomDrawGrid::SetEditorModeImpl(TObject* owner, const bool& value)    { nv::TCustomDrawGrid_SetEditorMode(owner->Handle(), value ? 1 : 0); }

TGridRect TCustomDrawGrid::GetSelectionImpl(TObject* owner)
{
    TGridRect r{};
    nv::TCustomDrawGrid_GetSelection(owner->Handle(), &r.Left, &r.Top, &r.Right, &r.Bottom);
    return r;
}

void TCustomDrawGrid::SetSelectionImpl(TObject* owner, const TGridRect& value)
{
    nv::TCustomDrawGrid_SetSelection(owner->Handle(), value.Left, value.Top, value.Right, value.Bottom);
}

void NO_VCL_CALL TCustomDrawGrid::DrawCellTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row,
                                                     no_vcl_int_t left, no_vcl_int_t top, no_vcl_int_t right, no_vcl_int_t bottom,
                                                     no_vcl_uint_t state, void*)
{
    GuardCallback([&] {
        TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
        if (!self || !self->onDrawCell_)
            return;
        TOnDrawCell handler = self->onDrawCell_;
        handler(self, col, row, TRect{left, top, right, bottom}, state);
    });
}

void NO_VCL_CALL TCustomDrawGrid::SelectCellTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row, no_vcl_bool_t* canSelect, void*)
{
    GuardCallback([&] {
        TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
        if (!self || !self->onSelectCell_)
            return;
        TOnSelectCellEvent handler = self->onSelectCell_;
        bool value = *canSelect != 0;
        handler(self, col, row, value);
        *canSelect = value ? 1 : 0;
    });
}

void NO_VCL_CALL TCustomDrawGrid::SelectionTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row, void*)
{
    GuardCallback([&] {
        TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
        if (!self || !self->onSelection_)
            return;
        TOnSelectEvent handler = self->onSelection_;
        handler(self, col, row);
    });
}

void NO_VCL_CALL TCustomDrawGrid::HeaderClickTrampoline(no_vcl_obj_t sender, no_vcl_int_t isColumn, no_vcl_int_t index, void*)
{
    GuardCallback([&] {
        TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
        if (!self || !self->onHeaderClick_)
            return;
        THdrEvent handler = self->onHeaderClick_;
        handler(self, isColumn != 0, index);
    });
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
    : TCustomDrawGrid(nv::TDrawGrid_Create(HandleOf(AOwner)))
{}

TCustomStringGrid::TCustomStringGrid(no_vcl_obj_t handle)
    : TCustomDrawGrid(handle)
    , Cells(this, &TCustomStringGrid::GetCellsImpl, &TCustomStringGrid::SetCellsImpl)
{}

std::string TCustomStringGrid::GetCellsImpl(TObject* owner, int ACol, int ARow)
{
    return std::string(nv::TCustomStringGrid_GetCells(owner->Handle(), ACol, ARow));
}

void TCustomStringGrid::SetCellsImpl(TObject* owner, int ACol, int ARow, const std::string& value)
{
    nv::TCustomStringGrid_SetCells(owner->Handle(), ACol, ARow, value.c_str());
}

void TCustomStringGrid::Clean()                   { nv::TCustomStringGrid_Clean(handle_); }
void TCustomStringGrid::AutoSizeColumns()         { nv::TCustomStringGrid_AutoSizeColumns(handle_); }
void TCustomStringGrid::AutoSizeColumn(int ACol)  { nv::TCustomStringGrid_AutoSizeColumn(handle_, ACol); }

TStringGrid::TStringGrid(TComponent* AOwner)
    : TCustomStringGrid(nv::TStringGrid_Create(HandleOf(AOwner)))
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
    , ImageIndex(this, &THeaderSection::GetImageIndexImpl, &THeaderSection::SetImageIndexImpl)
{}

std::string THeaderSection::GetTextImpl(TObject* owner) { return std::string(nv::THeaderSection_GetText(owner->Handle())); }
void THeaderSection::SetTextImpl(TObject* owner, const std::string& value) { nv::THeaderSection_SetText(owner->Handle(), value.c_str()); }
int  THeaderSection::GetWidthImpl(TObject* owner)                     { return nv::THeaderSection_GetWidth(owner->Handle()); }
void THeaderSection::SetWidthImpl(TObject* owner, const int& value)    { nv::THeaderSection_SetWidth(owner->Handle(), value); }
int  THeaderSection::GetMinWidthImpl(TObject* owner)                  { return nv::THeaderSection_GetMinWidth(owner->Handle()); }
void THeaderSection::SetMinWidthImpl(TObject* owner, const int& value) { nv::THeaderSection_SetMinWidth(owner->Handle(), value); }
int  THeaderSection::GetMaxWidthImpl(TObject* owner)                  { return nv::THeaderSection_GetMaxWidth(owner->Handle()); }
void THeaderSection::SetMaxWidthImpl(TObject* owner, const int& value) { nv::THeaderSection_SetMaxWidth(owner->Handle(), value); }
TAlignment THeaderSection::GetAlignmentImpl(TObject* owner) { return static_cast<TAlignment>(nv::THeaderSection_GetAlignment(owner->Handle())); }
void THeaderSection::SetAlignmentImpl(TObject* owner, const TAlignment& value) { nv::THeaderSection_SetAlignment(owner->Handle(), value); }
bool THeaderSection::GetVisibleImpl(TObject* owner)                   { return nv::THeaderSection_GetVisible(owner->Handle()) != 0; }
void THeaderSection::SetVisibleImpl(TObject* owner, const bool& value) { nv::THeaderSection_SetVisible(owner->Handle(), value ? 1 : 0); }
int  THeaderSection::GetIndexImpl(TObject* owner)                     { return nv::THeaderSection_GetIndex(owner->Handle()); }
void THeaderSection::SetIndexImpl(TObject* owner, const int& value)    { nv::THeaderSection_SetIndex(owner->Handle(), value); }
int  THeaderSection::GetLeftImpl(TObject* owner)                      { return nv::THeaderSection_GetLeft(owner->Handle()); }
int  THeaderSection::GetRightImpl(TObject* owner)                     { return nv::THeaderSection_GetRight(owner->Handle()); }
int  THeaderSection::GetOriginalIndexImpl(TObject* owner)             { return nv::THeaderSection_GetOriginalIndex(owner->Handle()); }

THeaderSections::THeaderSections(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &THeaderSections::GetCountImpl)
    , Items(this, &THeaderSections::GetItemsImpl)
{}

THeaderSection* THeaderSections::Add()             { return THeaderSection::Wrap(nv::THeaderSections_Add(handle_)); }
THeaderSection* THeaderSections::Insert(int Index) { return THeaderSection::Wrap(nv::THeaderSections_Insert(handle_, Index)); }
void THeaderSections::Delete(int Index)            { nv::THeaderSections_Delete(handle_, Index); }
void THeaderSections::Clear()                      { nv::THeaderSections_Clear(handle_); }
void THeaderSections::BeginUpdate()                { nv::THeaderSections_BeginUpdate(handle_); }
void THeaderSections::EndUpdate()                  { nv::THeaderSections_EndUpdate(handle_); }
THeaderSection* THeaderSections::GetItemsImpl(TObject* owner, int Index) { return THeaderSection::Wrap(nv::THeaderSections_GetItem(owner->Handle(), Index)); }
int  THeaderSections::GetCountImpl(TObject* owner) { return nv::THeaderSections_GetCount(owner->Handle()); }

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
    , Images(this, &TCustomHeaderControl::GetImagesImpl, &TCustomHeaderControl::SetImagesImpl)
    , sections_(nv::TCustomHeaderControl_GetSections(handle_))
{}

int TCustomHeaderControl::GetSectionAt(const TPoint& P) const
{
    return nv::TCustomHeaderControl_GetSectionAt(handle_, P.X, P.Y);
}

THeaderSections* TCustomHeaderControl::GetSectionsImpl(TObject* owner) { return &static_cast<TCustomHeaderControl*>(owner)->sections_; }
bool TCustomHeaderControl::GetDragReorderImpl(TObject* owner) { return nv::TCustomHeaderControl_GetDragReorder(owner->Handle()) != 0; }
void TCustomHeaderControl::SetDragReorderImpl(TObject* owner, const bool& value) { nv::TCustomHeaderControl_SetDragReorder(owner->Handle(), value ? 1 : 0); }
THeaderSection* TCustomHeaderControl::GetSectionFromOriginalIndexImpl(TObject* owner, int OriginalIndex)
{
    return THeaderSection::Wrap(nv::TCustomHeaderControl_GetSectionFromOriginalIndex(owner->Handle(), OriginalIndex));
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
    GuardCallback([&] {
        if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
            CallSectionNotify(self, self->onSectionClick_, THeaderSection::Wrap(section));
    });
}

void NO_VCL_CALL TCustomHeaderControl::SectionResizeTrampoline(no_vcl_obj_t sender, no_vcl_obj_t section, void*)
{
    GuardCallback([&] {
        if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
            CallSectionNotify(self, self->onSectionResize_, THeaderSection::Wrap(section));
    });
}

void NO_VCL_CALL TCustomHeaderControl::SectionSeparatorDblClickTrampoline(no_vcl_obj_t sender, no_vcl_obj_t section, void*)
{
    GuardCallback([&] {
        if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
            CallSectionNotify(self, self->onSectionSeparatorDblClick_, THeaderSection::Wrap(section));
    });
}

void NO_VCL_CALL TCustomHeaderControl::SectionTrackTrampoline(no_vcl_obj_t sender, no_vcl_obj_t section,
                                                              no_vcl_int_t width, no_vcl_int_t state, void*)
{
    GuardCallback([&] {
        TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender));
        if (!self || !self->onSectionTrack_)
            return;
        TCustomSectionTrackEvent handler = self->onSectionTrack_;
        handler(self, THeaderSection::Wrap(section), width, static_cast<TSectionTrackState>(state));
    });
}

void NO_VCL_CALL TCustomHeaderControl::SectionDragTrampoline(no_vcl_obj_t sender, no_vcl_obj_t fromSection,
                                                             no_vcl_obj_t toSection, no_vcl_bool_t* allow, void*)
{
    GuardCallback([&] {
        TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender));
        if (!self || !self->onSectionDrag_)
            return;
        TSectionDragEvent handler = self->onSectionDrag_;
        bool allowDrag = *allow != 0;
        handler(self, THeaderSection::Wrap(fromSection), THeaderSection::Wrap(toSection), allowDrag);
        *allow = allowDrag ? 1 : 0;
    });
}

void NO_VCL_CALL TCustomHeaderControl::SectionEndDragTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
            CallNotify(self->onSectionEndDrag_, self);
    });
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
    : TCustomHeaderControl(nv::THeaderControl_Create(HandleOf(AOwner)))
{}

/* ---------------- ToolBar ---------------- */

TToolWindow::TToolWindow(no_vcl_obj_t handle)
    : TCustomControl(handle)
    , EdgeBorders(this, &TToolWindow::GetEdgeBordersImpl, &TToolWindow::SetEdgeBordersImpl)
    , EdgeInner(this, &TToolWindow::GetEdgeInnerImpl, &TToolWindow::SetEdgeInnerImpl)
    , EdgeOuter(this, &TToolWindow::GetEdgeOuterImpl, &TToolWindow::SetEdgeOuterImpl)
{}

void TToolWindow::BeginUpdate() { nv::TToolWindow_BeginUpdate(handle_); }
void TToolWindow::EndUpdate()   { nv::TToolWindow_EndUpdate(handle_); }
TEdgeBorders TToolWindow::GetEdgeBordersImpl(TObject* owner) { return nv::TToolWindow_GetEdgeBorders(owner->Handle()); }
void TToolWindow::SetEdgeBordersImpl(TObject* owner, const TEdgeBorders& value) { nv::TToolWindow_SetEdgeBorders(owner->Handle(), value); }
TEdgeStyle TToolWindow::GetEdgeInnerImpl(TObject* owner) { return static_cast<TEdgeStyle>(nv::TToolWindow_GetEdgeInner(owner->Handle())); }
void TToolWindow::SetEdgeInnerImpl(TObject* owner, const TEdgeStyle& value) { nv::TToolWindow_SetEdgeInner(owner->Handle(), value); }
TEdgeStyle TToolWindow::GetEdgeOuterImpl(TObject* owner) { return static_cast<TEdgeStyle>(nv::TToolWindow_GetEdgeOuter(owner->Handle())); }
void TToolWindow::SetEdgeOuterImpl(TObject* owner, const TEdgeStyle& value) { nv::TToolWindow_SetEdgeOuter(owner->Handle(), value); }

TToolBar::TToolBar(TComponent* AOwner)
    : TToolWindow(nv::TToolBar_Create(HandleOf(AOwner)))
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
    , Images(this, &TToolBar::GetImagesImpl, &TToolBar::SetImagesImpl)
    , HotImages(this, &TToolBar::GetHotImagesImpl, &TToolBar::SetHotImagesImpl)
    , DisabledImages(this, &TToolBar::GetDisabledImagesImpl, &TToolBar::SetDisabledImagesImpl)
{}

void TToolBar::SetButtonSize(int NewButtonWidth, int NewButtonHeight)
{
    nv::TToolBar_SetButtonSize(handle_, NewButtonWidth, NewButtonHeight);
}

int  TToolBar::GetButtonCountImpl(TObject* owner) { return nv::TToolBar_GetButtonCount(owner->Handle()); }
// ボタンは利用者が生成したコンポーネントなので、ラッパーは必ずある。
TToolButton* TToolBar::GetButtonsImpl(TObject* owner, int Index)
{
    return static_cast<TToolButton*>(FromHandle(nv::TToolBar_GetButton(owner->Handle(), Index)));
}
int  TToolBar::GetRowCountImpl(TObject* owner)                         { return nv::TToolBar_GetRowCount(owner->Handle()); }
int  TToolBar::GetButtonHeightImpl(TObject* owner)                     { return nv::TToolBar_GetButtonHeight(owner->Handle()); }
void TToolBar::SetButtonHeightImpl(TObject* owner, const int& value)    { nv::TToolBar_SetButtonHeight(owner->Handle(), value); }
int  TToolBar::GetButtonWidthImpl(TObject* owner)                      { return nv::TToolBar_GetButtonWidth(owner->Handle()); }
void TToolBar::SetButtonWidthImpl(TObject* owner, const int& value)     { nv::TToolBar_SetButtonWidth(owner->Handle(), value); }
int  TToolBar::GetDropDownWidthImpl(TObject* owner)                    { return nv::TToolBar_GetDropDownWidth(owner->Handle()); }
void TToolBar::SetDropDownWidthImpl(TObject* owner, const int& value)   { nv::TToolBar_SetDropDownWidth(owner->Handle(), value); }
int  TToolBar::GetIndentImpl(TObject* owner)                           { return nv::TToolBar_GetIndent(owner->Handle()); }
void TToolBar::SetIndentImpl(TObject* owner, const int& value)          { nv::TToolBar_SetIndent(owner->Handle(), value); }
bool TToolBar::GetFlatImpl(TObject* owner)                             { return nv::TToolBar_GetFlat(owner->Handle()) != 0; }
void TToolBar::SetFlatImpl(TObject* owner, const bool& value)           { nv::TToolBar_SetFlat(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetListImpl(TObject* owner)                             { return nv::TToolBar_GetList(owner->Handle()) != 0; }
void TToolBar::SetListImpl(TObject* owner, const bool& value)           { nv::TToolBar_SetList(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetShowCaptionsImpl(TObject* owner)                     { return nv::TToolBar_GetShowCaptions(owner->Handle()) != 0; }
void TToolBar::SetShowCaptionsImpl(TObject* owner, const bool& value)   { nv::TToolBar_SetShowCaptions(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetTransparentImpl(TObject* owner)                      { return nv::TToolBar_GetTransparent(owner->Handle()) != 0; }
void TToolBar::SetTransparentImpl(TObject* owner, const bool& value)    { nv::TToolBar_SetTransparent(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetWrapableImpl(TObject* owner)                         { return nv::TToolBar_GetWrapable(owner->Handle()) != 0; }
void TToolBar::SetWrapableImpl(TObject* owner, const bool& value)       { nv::TToolBar_SetWrapable(owner->Handle(), value ? 1 : 0); }

TToolButton::TToolButton(TComponent* AOwner)
    : TGraphicControl(nv::TToolButton_Create(HandleOf(AOwner)))
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
    , ImageIndex(this, &TToolButton::GetImageIndexImpl, &TToolButton::SetImageIndexImpl)
{}

void TToolButton::Click()      { nv::TToolButton_Click(handle_); }
void TToolButton::ArrowClick() { nv::TToolButton_ArrowClick(handle_); }
bool TToolButton::PointInArrow(int X, int Y) const { return nv::TToolButton_PointInArrow(handle_, X, Y) != 0; }

bool TToolButton::GetAllowAllUpImpl(TObject* owner)                      { return nv::TToolButton_GetAllowAllUp(owner->Handle()) != 0; }
void TToolButton::SetAllowAllUpImpl(TObject* owner, const bool& value)    { nv::TToolButton_SetAllowAllUp(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetDownImpl(TObject* owner)                            { return nv::TToolButton_GetDown(owner->Handle()) != 0; }
void TToolButton::SetDownImpl(TObject* owner, const bool& value)          { nv::TToolButton_SetDown(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetGroupedImpl(TObject* owner)                         { return nv::TToolButton_GetGrouped(owner->Handle()) != 0; }
void TToolButton::SetGroupedImpl(TObject* owner, const bool& value)       { nv::TToolButton_SetGrouped(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetIndeterminateImpl(TObject* owner)                   { return nv::TToolButton_GetIndeterminate(owner->Handle()) != 0; }
void TToolButton::SetIndeterminateImpl(TObject* owner, const bool& value) { nv::TToolButton_SetIndeterminate(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetMarkedImpl(TObject* owner)                          { return nv::TToolButton_GetMarked(owner->Handle()) != 0; }
void TToolButton::SetMarkedImpl(TObject* owner, const bool& value)        { nv::TToolButton_SetMarked(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetShowCaptionImpl(TObject* owner)                     { return nv::TToolButton_GetShowCaption(owner->Handle()) != 0; }
void TToolButton::SetShowCaptionImpl(TObject* owner, const bool& value)   { nv::TToolButton_SetShowCaption(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetWrapImpl(TObject* owner)                            { return nv::TToolButton_GetWrap(owner->Handle()) != 0; }
void TToolButton::SetWrapImpl(TObject* owner, const bool& value)          { nv::TToolButton_SetWrap(owner->Handle(), value ? 1 : 0); }
TToolButtonStyle TToolButton::GetStyleImpl(TObject* owner) { return static_cast<TToolButtonStyle>(nv::TToolButton_GetStyle(owner->Handle())); }
void TToolButton::SetStyleImpl(TObject* owner, const TToolButtonStyle& value) { nv::TToolButton_SetStyle(owner->Handle(), value); }
TPopupMenu* TToolButton::GetDropdownMenuImpl(TObject* owner)
{
    return static_cast<TPopupMenu*>(FromHandle(nv::TToolButton_GetDropdownMenu(owner->Handle())));
}
void TToolButton::SetDropdownMenuImpl(TObject* owner, TPopupMenu* const& value) { nv::TToolButton_SetDropdownMenu(owner->Handle(), HandleOf(value)); }
// メニュー項目は LCL が内部で生成したもの(メニューのルート項目等)もありうるため WrapExisting で引く。
TMenuItem* TToolButton::GetMenuItemImpl(TObject* owner) { return WrapExisting<TMenuItem>(nv::TToolButton_GetMenuItem(owner->Handle())); }
void TToolButton::SetMenuItemImpl(TObject* owner, TMenuItem* const& value) { nv::TToolButton_SetMenuItem(owner->Handle(), HandleOf(value)); }
int  TToolButton::GetIndexImpl(TObject* owner) { return nv::TToolButton_GetIndex(owner->Handle()); }

void NO_VCL_CALL TToolButton::ArrowClickTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TToolButton* self = static_cast<TToolButton*>(FromHandle(sender)))
            CallNotify(self->onArrowClick_, self);
    });
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
    , ImageIndex(this, &TCoolBand::GetImageIndexImpl, &TCoolBand::SetImageIndexImpl)
    , Bitmap(this, &TCoolBand::GetBitmapImpl, &TCoolBand::SetBitmapImpl)
    , bitmap_(this, &no_vcl_TCoolBand_GetBitmap)
{}

void TCoolBand::AutosizeWidth() { nv::TCoolBand_AutosizeWidth(handle_); }

std::string TCoolBand::GetTextImpl(TObject* owner) { return std::string(nv::TCoolBand_GetText(owner->Handle())); }
void TCoolBand::SetTextImpl(TObject* owner, const std::string& value) { nv::TCoolBand_SetText(owner->Handle(), value.c_str()); }
int  TCoolBand::GetWidthImpl(TObject* owner)                            { return nv::TCoolBand_GetWidth(owner->Handle()); }
void TCoolBand::SetWidthImpl(TObject* owner, const int& value)           { nv::TCoolBand_SetWidth(owner->Handle(), value); }
int  TCoolBand::GetMinWidthImpl(TObject* owner)                         { return nv::TCoolBand_GetMinWidth(owner->Handle()); }
void TCoolBand::SetMinWidthImpl(TObject* owner, const int& value)        { nv::TCoolBand_SetMinWidth(owner->Handle(), value); }
int  TCoolBand::GetMinHeightImpl(TObject* owner)                        { return nv::TCoolBand_GetMinHeight(owner->Handle()); }
void TCoolBand::SetMinHeightImpl(TObject* owner, const int& value)       { nv::TCoolBand_SetMinHeight(owner->Handle(), value); }
bool TCoolBand::GetBreakImpl(TObject* owner)                            { return nv::TCoolBand_GetBreak(owner->Handle()) != 0; }
void TCoolBand::SetBreakImpl(TObject* owner, const bool& value)          { nv::TCoolBand_SetBreak(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetVisibleImpl(TObject* owner)                          { return nv::TCoolBand_GetVisible(owner->Handle()) != 0; }
void TCoolBand::SetVisibleImpl(TObject* owner, const bool& value)        { nv::TCoolBand_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetFixedSizeImpl(TObject* owner)                        { return nv::TCoolBand_GetFixedSize(owner->Handle()) != 0; }
void TCoolBand::SetFixedSizeImpl(TObject* owner, const bool& value)      { nv::TCoolBand_SetFixedSize(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetFixedBackgroundImpl(TObject* owner)                  { return nv::TCoolBand_GetFixedBackground(owner->Handle()) != 0; }
void TCoolBand::SetFixedBackgroundImpl(TObject* owner, const bool& value) { nv::TCoolBand_SetFixedBackground(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetHorizontalOnlyImpl(TObject* owner)                   { return nv::TCoolBand_GetHorizontalOnly(owner->Handle()) != 0; }
void TCoolBand::SetHorizontalOnlyImpl(TObject* owner, const bool& value) { nv::TCoolBand_SetHorizontalOnly(owner->Handle(), value ? 1 : 0); }
TColor TCoolBand::GetColorImpl(TObject* owner)                          { return static_cast<TColor>(nv::TCoolBand_GetColor(owner->Handle())); }
void TCoolBand::SetColorImpl(TObject* owner, const TColor& value)        { nv::TCoolBand_SetColor(owner->Handle(), static_cast<no_vcl_int_t>(value)); }
bool TCoolBand::GetParentColorImpl(TObject* owner)                      { return nv::TCoolBand_GetParentColor(owner->Handle()) != 0; }
void TCoolBand::SetParentColorImpl(TObject* owner, const bool& value)    { nv::TCoolBand_SetParentColor(owner->Handle(), value ? 1 : 0); }
int  TCoolBand::GetIndexImpl(TObject* owner)                            { return nv::TCoolBand_GetIndex(owner->Handle()); }
void TCoolBand::SetIndexImpl(TObject* owner, const int& value)           { nv::TCoolBand_SetIndex(owner->Handle(), value); }
// バンドに置くコントロールは利用者が生成したコンポーネントなので、ラッパーは必ずある。
TControl* TCoolBand::GetControlImpl(TObject* owner)
{
    return static_cast<TControl*>(TControl::FromHandle(nv::TCoolBand_GetControl(owner->Handle())));
}
void TCoolBand::SetControlImpl(TObject* owner, TControl* const& value)   { nv::TCoolBand_SetControl(owner->Handle(), value ? value->Handle() : nullptr); }
int  TCoolBand::GetLeftImpl(TObject* owner)                             { return nv::TCoolBand_GetLeft(owner->Handle()); }
int  TCoolBand::GetTopImpl(TObject* owner)                              { return nv::TCoolBand_GetTop(owner->Handle()); }
int  TCoolBand::GetRightImpl(TObject* owner)                            { return nv::TCoolBand_GetRight(owner->Handle()); }
int  TCoolBand::GetHeightImpl(TObject* owner)                           { return nv::TCoolBand_GetHeight(owner->Handle()); }

TCoolBands::TCoolBands(no_vcl_obj_t handle)
    : TPersistent(handle)
    , Count(this, &TCoolBands::GetCountImpl)
    , Items(this, &TCoolBands::GetItemsImpl)
{}

TCoolBand* TCoolBands::Add()          { return TCoolBand::Wrap(nv::TCoolBands_Add(handle_)); }
void TCoolBands::Delete(int Index)    { nv::TCoolBands_Delete(handle_, Index); }
void TCoolBands::Clear()              { nv::TCoolBands_Clear(handle_); }
void TCoolBands::BeginUpdate()        { nv::TCoolBands_BeginUpdate(handle_); }
void TCoolBands::EndUpdate()          { nv::TCoolBands_EndUpdate(handle_); }
TCoolBand* TCoolBands::FindBand(TControl* AControl) const { return TCoolBand::Wrap(nv::TCoolBands_FindBand(handle_, AControl ? AControl->Handle() : nullptr)); }
int  TCoolBands::FindBandIndex(TControl* AControl) const  { return nv::TCoolBands_FindBandIndex(handle_, AControl ? AControl->Handle() : nullptr); }
TCoolBand* TCoolBands::GetItemsImpl(TObject* owner, int Index) { return TCoolBand::Wrap(nv::TCoolBands_GetItem(owner->Handle(), Index)); }
int  TCoolBands::GetCountImpl(TObject* owner) { return nv::TCoolBands_GetCount(owner->Handle()); }

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
    , Images(this, &TCustomCoolBar::GetImagesImpl, &TCustomCoolBar::SetImagesImpl)
    , Bitmap(this, &TCustomCoolBar::GetBitmapImpl, &TCustomCoolBar::SetBitmapImpl)
    , bands_(nv::TCustomCoolBar_GetBands(handle_))
    , bitmap_(this, &no_vcl_TCustomCoolBar_GetBitmap)
{}

void TCustomCoolBar::AutosizeBands() { nv::TCustomCoolBar_AutosizeBands(handle_); }

void TCustomCoolBar::MouseToBandPos(int X, int Y, int& ABand, bool& AGrabber) const
{
    no_vcl_int_t band = -1;
    no_vcl_bool_t grabber = 0;
    nv::TCustomCoolBar_MouseToBandPos(handle_, X, Y, &band, &grabber);
    ABand = band;
    AGrabber = grabber != 0;
}

TCoolBands* TCustomCoolBar::GetBandsImpl(TObject* owner) { return &static_cast<TCustomCoolBar*>(owner)->bands_; }
bool TCustomCoolBar::GetFixedSizeImpl(TObject* owner)                        { return nv::TCustomCoolBar_GetFixedSize(owner->Handle()) != 0; }
void TCustomCoolBar::SetFixedSizeImpl(TObject* owner, const bool& value)      { nv::TCustomCoolBar_SetFixedSize(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetFixedOrderImpl(TObject* owner)                       { return nv::TCustomCoolBar_GetFixedOrder(owner->Handle()) != 0; }
void TCustomCoolBar::SetFixedOrderImpl(TObject* owner, const bool& value)     { nv::TCustomCoolBar_SetFixedOrder(owner->Handle(), value ? 1 : 0); }
TGrabStyle TCustomCoolBar::GetGrabStyleImpl(TObject* owner) { return static_cast<TGrabStyle>(nv::TCustomCoolBar_GetGrabStyle(owner->Handle())); }
void TCustomCoolBar::SetGrabStyleImpl(TObject* owner, const TGrabStyle& value) { nv::TCustomCoolBar_SetGrabStyle(owner->Handle(), value); }
int  TCustomCoolBar::GetGrabWidthImpl(TObject* owner)                        { return nv::TCustomCoolBar_GetGrabWidth(owner->Handle()); }
void TCustomCoolBar::SetGrabWidthImpl(TObject* owner, const int& value)       { nv::TCustomCoolBar_SetGrabWidth(owner->Handle(), value); }
int  TCustomCoolBar::GetHorizontalSpacingImpl(TObject* owner)                { return nv::TCustomCoolBar_GetHorizontalSpacing(owner->Handle()); }
void TCustomCoolBar::SetHorizontalSpacingImpl(TObject* owner, const int& value) { nv::TCustomCoolBar_SetHorizontalSpacing(owner->Handle(), value); }
int  TCustomCoolBar::GetVerticalSpacingImpl(TObject* owner)                  { return nv::TCustomCoolBar_GetVerticalSpacing(owner->Handle()); }
void TCustomCoolBar::SetVerticalSpacingImpl(TObject* owner, const int& value) { nv::TCustomCoolBar_SetVerticalSpacing(owner->Handle(), value); }
bool TCustomCoolBar::GetShowTextImpl(TObject* owner)                         { return nv::TCustomCoolBar_GetShowText(owner->Handle()) != 0; }
void TCustomCoolBar::SetShowTextImpl(TObject* owner, const bool& value)       { nv::TCustomCoolBar_SetShowText(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetThemedImpl(TObject* owner)                           { return nv::TCustomCoolBar_GetThemed(owner->Handle()) != 0; }
void TCustomCoolBar::SetThemedImpl(TObject* owner, const bool& value)         { nv::TCustomCoolBar_SetThemed(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetVerticalImpl(TObject* owner)                         { return nv::TCustomCoolBar_GetVertical(owner->Handle()) != 0; }
void TCustomCoolBar::SetVerticalImpl(TObject* owner, const bool& value)       { nv::TCustomCoolBar_SetVertical(owner->Handle(), value ? 1 : 0); }

void NO_VCL_CALL TCustomCoolBar::ChangeTrampoline(no_vcl_obj_t sender, void*)
{
    GuardCallback([&] {
        if (TCustomCoolBar* self = static_cast<TCustomCoolBar*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TNotifyEvent TCustomCoolBar::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomCoolBar*>(owner)->onChange_; }

void TCustomCoolBar::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomCoolBar* self = static_cast<TCustomCoolBar*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &no_vcl_TCustomCoolBar_SetOnChange, &TCustomCoolBar::ChangeTrampoline);
}

TCoolBar::TCoolBar(TComponent* AOwner)
    : TCustomCoolBar(nv::TCoolBar_Create(HandleOf(AOwner)))
{}


/* ---------------- Images・ImageIndex・Bitmap(docs/adr/0030) ---------------- */

TCustomImageList* TCustomImage::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TCustomImage_GetImages(owner->Handle()))); }
void TCustomImage::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TCustomImage_SetImages(owner->Handle(), HandleOf(value)); }
int  TCustomImage::GetImageIndexImpl(TObject* owner) { return nv::TCustomImage_GetImageIndex(owner->Handle()); }
void TCustomImage::SetImageIndexImpl(TObject* owner, const int& value) { nv::TCustomImage_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomBitBtn::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TCustomBitBtn_GetImages(owner->Handle()))); }
void TCustomBitBtn::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TCustomBitBtn_SetImages(owner->Handle(), HandleOf(value)); }
int  TCustomBitBtn::GetImageIndexImpl(TObject* owner) { return nv::TCustomBitBtn_GetImageIndex(owner->Handle()); }
void TCustomBitBtn::SetImageIndexImpl(TObject* owner, const int& value) { nv::TCustomBitBtn_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomSpeedButton::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TCustomSpeedButton_GetImages(owner->Handle()))); }
void TCustomSpeedButton::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TCustomSpeedButton_SetImages(owner->Handle(), HandleOf(value)); }
int  TCustomSpeedButton::GetImageIndexImpl(TObject* owner) { return nv::TCustomSpeedButton_GetImageIndex(owner->Handle()); }
void TCustomSpeedButton::SetImageIndexImpl(TObject* owner, const int& value) { nv::TCustomSpeedButton_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomTabControl::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TCustomTabControl_GetImages(owner->Handle()))); }
void TCustomTabControl::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TCustomTabControl_SetImages(owner->Handle(), HandleOf(value)); }

int  TCustomPage::GetImageIndexImpl(TObject* owner) { return nv::TCustomPage_GetImageIndex(owner->Handle()); }
void TCustomPage::SetImageIndexImpl(TObject* owner, const int& value) { nv::TCustomPage_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomTreeView::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TCustomTreeView_GetImages(owner->Handle()))); }
void TCustomTreeView::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TCustomTreeView_SetImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TCustomTreeView::GetStateImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TCustomTreeView_GetStateImages(owner->Handle()))); }
void TCustomTreeView::SetStateImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TCustomTreeView_SetStateImages(owner->Handle(), HandleOf(value)); }

int  TTreeNode::GetImageIndexImpl(TObject* owner) { return nv::TTreeNode_GetImageIndex(owner->Handle()); }
void TTreeNode::SetImageIndexImpl(TObject* owner, const int& value) { nv::TTreeNode_SetImageIndex(owner->Handle(), value); }
int  TTreeNode::GetSelectedIndexImpl(TObject* owner) { return nv::TTreeNode_GetSelectedIndex(owner->Handle()); }
void TTreeNode::SetSelectedIndexImpl(TObject* owner, const int& value) { nv::TTreeNode_SetSelectedIndex(owner->Handle(), value); }
int  TTreeNode::GetStateIndexImpl(TObject* owner) { return nv::TTreeNode_GetStateIndex(owner->Handle()); }
void TTreeNode::SetStateIndexImpl(TObject* owner, const int& value) { nv::TTreeNode_SetStateIndex(owner->Handle(), value); }
int  TTreeNode::GetOverlayIndexImpl(TObject* owner) { return nv::TTreeNode_GetOverlayIndex(owner->Handle()); }
void TTreeNode::SetOverlayIndexImpl(TObject* owner, const int& value) { nv::TTreeNode_SetOverlayIndex(owner->Handle(), value); }

TCustomImageList* TListView::GetLargeImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TListView_GetLargeImages(owner->Handle()))); }
void TListView::SetLargeImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TListView_SetLargeImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TListView::GetSmallImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TListView_GetSmallImages(owner->Handle()))); }
void TListView::SetSmallImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TListView_SetSmallImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TListView::GetStateImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TListView_GetStateImages(owner->Handle()))); }
void TListView::SetStateImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TListView_SetStateImages(owner->Handle(), HandleOf(value)); }

int  TListItem::GetImageIndexImpl(TObject* owner) { return nv::TListItem_GetImageIndex(owner->Handle()); }
void TListItem::SetImageIndexImpl(TObject* owner, const int& value) { nv::TListItem_SetImageIndex(owner->Handle(), value); }
int  TListItem::GetStateIndexImpl(TObject* owner) { return nv::TListItem_GetStateIndex(owner->Handle()); }
void TListItem::SetStateIndexImpl(TObject* owner, const int& value) { nv::TListItem_SetStateIndex(owner->Handle(), value); }

int  TListColumn::GetImageIndexImpl(TObject* owner) { return nv::TListColumn_GetImageIndex(owner->Handle()); }
void TListColumn::SetImageIndexImpl(TObject* owner, const int& value) { nv::TListColumn_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TToolBar::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TToolBar_GetImages(owner->Handle()))); }
void TToolBar::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TToolBar_SetImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TToolBar::GetHotImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TToolBar_GetHotImages(owner->Handle()))); }
void TToolBar::SetHotImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TToolBar_SetHotImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TToolBar::GetDisabledImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TToolBar_GetDisabledImages(owner->Handle()))); }
void TToolBar::SetDisabledImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TToolBar_SetDisabledImages(owner->Handle(), HandleOf(value)); }

int  TToolButton::GetImageIndexImpl(TObject* owner) { return nv::TToolButton_GetImageIndex(owner->Handle()); }
void TToolButton::SetImageIndexImpl(TObject* owner, const int& value) { nv::TToolButton_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomHeaderControl::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TCustomHeaderControl_GetImages(owner->Handle()))); }
void TCustomHeaderControl::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TCustomHeaderControl_SetImages(owner->Handle(), HandleOf(value)); }

int  THeaderSection::GetImageIndexImpl(TObject* owner) { return nv::THeaderSection_GetImageIndex(owner->Handle()); }
void THeaderSection::SetImageIndexImpl(TObject* owner, const int& value) { nv::THeaderSection_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomCoolBar::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TCustomCoolBar_GetImages(owner->Handle()))); }
void TCustomCoolBar::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TCustomCoolBar_SetImages(owner->Handle(), HandleOf(value)); }
TBitmap* TCustomCoolBar::GetBitmapImpl(TObject* owner) { return &static_cast<TCustomCoolBar*>(owner)->bitmap_; }
void TCustomCoolBar::SetBitmapImpl(TObject* owner, TBitmap* const& value)
{
    nv::TCustomCoolBar_SetBitmap(owner->Handle(), value ? value->Current() : nullptr);
}

int  TCoolBand::GetImageIndexImpl(TObject* owner) { return nv::TCoolBand_GetImageIndex(owner->Handle()); }
void TCoolBand::SetImageIndexImpl(TObject* owner, const int& value) { nv::TCoolBand_SetImageIndex(owner->Handle(), value); }
TBitmap* TCoolBand::GetBitmapImpl(TObject* owner) { return &static_cast<TCoolBand*>(owner)->bitmap_; }
void TCoolBand::SetBitmapImpl(TObject* owner, TBitmap* const& value)
{
    nv::TCoolBand_SetBitmap(owner->Handle(), value ? value->Current() : nullptr);
}

TCustomImageList* TMenu::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TMenu_GetImages(owner->Handle()))); }
void TMenu::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TMenu_SetImages(owner->Handle(), HandleOf(value)); }

int  TMenuItem::GetImageIndexImpl(TObject* owner) { return nv::TMenuItem_GetImageIndex(owner->Handle()); }
void TMenuItem::SetImageIndexImpl(TObject* owner, const int& value) { nv::TMenuItem_SetImageIndex(owner->Handle(), value); }
TCustomImageList* TMenuItem::GetSubMenuImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(nv::TMenuItem_GetSubMenuImages(owner->Handle()))); }
void TMenuItem::SetSubMenuImagesImpl(TObject* owner, TCustomImageList* const& value) { nv::TMenuItem_SetSubMenuImages(owner->Handle(), HandleOf(value)); }
TBitmap* TMenuItem::GetBitmapImpl(TObject* owner) { return &static_cast<TMenuItem*>(owner)->bitmap_; }
void TMenuItem::SetBitmapImpl(TObject* owner, TBitmap* const& value)
{
    nv::TMenuItem_SetBitmap(owner->Handle(), value ? value->Current() : nullptr);
}

} // namespace no_vcl
