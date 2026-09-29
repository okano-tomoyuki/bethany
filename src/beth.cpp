#include <bethany/beth.hpp>

#include <cstdlib>

namespace beth
{

namespace
{

// イベントの Setter で共通の処理: ハンドラを保持し、最初に空でないハンドラが設定されたときだけブリッジを登録する。
template<typename Event, typename Callback>
void SetSimpleEvent(ObjectHandle handle, Event& slot, bool& hooked, const Event& value,
                     void (*setOn)(ObjectHandle, Callback, void*), Callback trampoline)
{
    slot = value;
    if (value && !hooked)
    {
        setOn(handle, trampoline, nullptr);
        hooked = true;
    }
}

// DLL から呼ばれるコールバック(トランポリン)の本体を包む。ハンドラから送出された例外は DLL の関数をまたいで伝えられないため、
// ここで捕まえて SetCallbackError で知らせ、DLL 側で送出し直させる(docs/adr/0031)。
// デストラクタの中で LCL のオブジェクトを破棄する。例外がデストラクタから出ると std::terminate になるため、握りつぶす。
void DestroyNoThrow(void (*destroy)(ObjectHandle), ObjectHandle handle)
{
    try
    {
        destroy(handle);
    }
    catch (...)
    {
    }
}

template<typename F>
void GuardCallback(F f)
{
    try
    {
        f();
    }
    catch (const Exception& e)
    {
        internal::SetCallbackError(e.ClassName().c_str(), e.Message.c_str());
    }
    catch (const std::exception& e)
    {
        internal::SetCallbackError("std::exception", e.what());
    }
    catch (...)
    {
        internal::SetCallbackError("", "unknown C++ exception");
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
TStrings::TStrings(ObjectHandle handle)
    : TStrings(this, &TStrings::SelfAccessor)
{
    handle_ = handle;
}

int  TStrings::Add(const std::string& S)                     { return internal::TStrings_Add(Current(), S.c_str()); }
int  TStrings::AddObject(const std::string& S, void* AObject) { return internal::TStrings_AddObject(Current(), S.c_str(), AObject); }
void TStrings::Insert(int Index, const std::string& S)       { internal::TStrings_Insert(Current(), Index, S.c_str()); }
void TStrings::Delete(int Index)                             { internal::TStrings_Delete(Current(), Index); }
void TStrings::Clear()                                       { internal::TStrings_Clear(Current()); }
int  TStrings::IndexOf(const std::string& S) const           { return internal::TStrings_IndexOf(Current(), S.c_str()); }
void TStrings::Exchange(int Index1, int Index2)              { internal::TStrings_Exchange(Current(), Index1, Index2); }
void TStrings::Move(int CurIndex, int NewIndex)              { internal::TStrings_Move(Current(), CurIndex, NewIndex); }
void TStrings::BeginUpdate()                                 { internal::TStrings_BeginUpdate(Current()); }
void TStrings::EndUpdate()                                   { internal::TStrings_EndUpdate(Current()); }
void TStrings::Assign(const TStrings* Source)                { internal::TStrings_Assign(Current(), Source ? Source->Current() : nullptr); }
void TStrings::AddStrings(const TStrings* Source)            { if (Source) internal::TStrings_AddStrings(Current(), Source->Current()); }
int  TStrings::IndexOfName(const std::string& Name) const     { return internal::TStrings_IndexOfName(Current(), Name.c_str()); }
void TStrings::LoadFromFile(const std::string& FileName)     { internal::TStrings_LoadFromFile(Current(), FileName.c_str()); }
void TStrings::SaveToFile(const std::string& FileName) const { internal::TStrings_SaveToFile(Current(), FileName.c_str()); }

int TStrings::GetCountImpl(TObject* owner) { return internal::TStrings_GetCount(static_cast<TStrings*>(owner)->Current()); }
std::string TStrings::GetStringsImpl(TObject* owner, int Index)
{
    return std::string(internal::TStrings_GetStrings(static_cast<TStrings*>(owner)->Current(), Index));
}
void TStrings::SetStringsImpl(TObject* owner, int Index, const std::string& value)
{
    internal::TStrings_SetStrings(static_cast<TStrings*>(owner)->Current(), Index, value.c_str());
}
void* TStrings::GetObjectsImpl(TObject* owner, int Index) { return internal::TStrings_GetObjects(static_cast<TStrings*>(owner)->Current(), Index); }
void TStrings::SetObjectsImpl(TObject* owner, int Index, void* const& value)
{
    internal::TStrings_SetObjects(static_cast<TStrings*>(owner)->Current(), Index, value);
}
std::string TStrings::GetTextImpl(TObject* owner) { return std::string(internal::TStrings_GetText(static_cast<TStrings*>(owner)->Current())); }
void TStrings::SetTextImpl(TObject* owner, const std::string& value) { internal::TStrings_SetText(static_cast<TStrings*>(owner)->Current(), value.c_str()); }
std::string TStrings::GetCommaTextImpl(TObject* owner) { return std::string(internal::TStrings_GetCommaText(static_cast<TStrings*>(owner)->Current())); }
void TStrings::SetCommaTextImpl(TObject* owner, const std::string& value) { internal::TStrings_SetCommaText(static_cast<TStrings*>(owner)->Current(), value.c_str()); }
std::string TStrings::GetNamesImpl(TObject* owner, int Index)
{
    return std::string(internal::TStrings_GetNames(static_cast<TStrings*>(owner)->Current(), Index));
}
std::string TStrings::GetValuesImpl(TObject* owner, std::string Name)
{
    return std::string(internal::TStrings_GetValues(static_cast<TStrings*>(owner)->Current(), Name.c_str()));
}
void TStrings::SetValuesImpl(TObject* owner, std::string Name, const std::string& value)
{
    internal::TStrings_SetValues(static_cast<TStrings*>(owner)->Current(), Name.c_str(), value.c_str());
}
std::string TStrings::GetValueFromIndexImpl(TObject* owner, int Index)
{
    return std::string(internal::TStrings_GetValueFromIndex(static_cast<TStrings*>(owner)->Current(), Index));
}
void TStrings::SetValueFromIndexImpl(TObject* owner, int Index, const std::string& value)
{
    internal::TStrings_SetValueFromIndex(static_cast<TStrings*>(owner)->Current(), Index, value.c_str());
}
char TStrings::GetDelimiterImpl(TObject* owner) { return internal::TStrings_GetDelimiter(static_cast<TStrings*>(owner)->Current()); }
void TStrings::SetDelimiterImpl(TObject* owner, const char& value) { internal::TStrings_SetDelimiter(static_cast<TStrings*>(owner)->Current(), value); }
bool TStrings::GetStrictDelimiterImpl(TObject* owner) { return internal::TStrings_GetStrictDelimiter(static_cast<TStrings*>(owner)->Current()) != 0; }
void TStrings::SetStrictDelimiterImpl(TObject* owner, const bool& value)
{
    internal::TStrings_SetStrictDelimiter(static_cast<TStrings*>(owner)->Current(), value ? 1 : 0);
}
std::string TStrings::GetDelimitedTextImpl(TObject* owner) { return std::string(internal::TStrings_GetDelimitedText(static_cast<TStrings*>(owner)->Current())); }
void TStrings::SetDelimitedTextImpl(TObject* owner, const std::string& value)
{
    internal::TStrings_SetDelimitedText(static_cast<TStrings*>(owner)->Current(), value.c_str());
}

/* ---------------- TStringList ---------------- */

TStringList::TStringList()
    : TStrings(internal::TStringList_Create())
    , Sorted(this, &TStringList::GetSortedImpl, &TStringList::SetSortedImpl)
    , Duplicates(this, &TStringList::GetDuplicatesImpl, &TStringList::SetDuplicatesImpl)
    , CaseSensitive(this, &TStringList::GetCaseSensitiveImpl, &TStringList::SetCaseSensitiveImpl)
{}

TStringList::~TStringList()
{
    DestroyNoThrow(&internal::TStringList_Destroy, handle_);
}

void TStringList::Sort() { internal::TStringList_Sort(handle_); }

bool TStringList::Find(const std::string& S, int& Index) const
{
    internal::int_t index = -1;
    bool found = internal::TStringList_Find(handle_, S.c_str(), &index) != 0;
    Index = index;
    return found;
}

bool TStringList::GetSortedImpl(TObject* owner) { return internal::TStringList_GetSorted(owner->Handle()) != 0; }
void TStringList::SetSortedImpl(TObject* owner, const bool& value) { internal::TStringList_SetSorted(owner->Handle(), value ? 1 : 0); }
TDuplicates TStringList::GetDuplicatesImpl(TObject* owner) { return static_cast<TDuplicates>(internal::TStringList_GetDuplicates(owner->Handle())); }
void TStringList::SetDuplicatesImpl(TObject* owner, const TDuplicates& value) { internal::TStringList_SetDuplicates(owner->Handle(), value); }
bool TStringList::GetCaseSensitiveImpl(TObject* owner) { return internal::TStringList_GetCaseSensitive(owner->Handle()) != 0; }
void TStringList::SetCaseSensitiveImpl(TObject* owner, const bool& value) { internal::TStringList_SetCaseSensitive(owner->Handle(), value ? 1 : 0); }

/* ---------------- TComponent ---------------- */

TComponent::TComponent(ObjectHandle handle)
    : TPersistent(handle)
    , Tag(this, &TComponent::GetTagImpl, &TComponent::SetTagImpl)
{
    static bool callbackInstalled = false;
    if (!callbackInstalled)
    {
        internal::FreeNotify_SetCallback(&TComponent::FreeNotifyTrampoline, nullptr);
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
        DestroyNoThrow(&internal::TComponent_Destroy, handle_);
}

void TComponent::Free()
{
    internal::TComponent_Destroy(handle_);
}

std::intptr_t TComponent::GetTagImpl(TObject* owner)                             { return internal::TComponent_GetTag(owner->Handle()); }
void          TComponent::SetTagImpl(TObject* owner, const std::intptr_t& value) { internal::TComponent_SetTag(owner->Handle(), value); }

void BETH_CALL TComponent::FreeNotifyTrampoline(ObjectHandle handle, void*)
{
    GuardCallback([&] {
        std::unordered_map<ObjectHandle, TComponent*>& registry = Registry();
        auto it = registry.find(handle);
        if (it == registry.end())
            return;

        TComponent* self = it->second;
        registry.erase(it);
        self->freedByLcl_ = true;
        delete self;
    });
}

std::unordered_map<ObjectHandle, TComponent*>& TComponent::Registry()
{
    static std::unordered_map<ObjectHandle, TComponent*> registry;
    return registry;
}

TComponent* TComponent::FromHandle(ObjectHandle handle)
{
    std::unordered_map<ObjectHandle, TComponent*>& registry = Registry();
    auto it = registry.find(handle);
    return it != registry.end() ? it->second : nullptr;
}

/* ---------------- TControl ---------------- */

TControl::TControl(ObjectHandle handle)
    : TComponent(handle)
    , ClientWidth(this, &TControl::GetClientWidthImpl, &TControl::SetClientWidthImpl)
    , ClientHeight(this, &TControl::GetClientHeightImpl, &TControl::SetClientHeightImpl)
    , Action(this, &TControl::GetActionImpl, &TControl::SetActionImpl)
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
    , Color(this, &TControl::GetColorImpl, &TControl::SetColorImpl)
    , Font(this, &TControl::GetFontImpl, &TControl::SetFontImpl)
    , ParentColor(this, &TControl::GetParentColorImpl, &TControl::SetParentColorImpl)
    , ParentFont(this, &TControl::GetParentFontImpl, &TControl::SetParentFontImpl)
    , Anchors(this, &TControl::GetAnchorsImpl, &TControl::SetAnchorsImpl)
    , BorderSpacing(this, &TControl::GetBorderSpacingImpl, &TControl::SetBorderSpacingImpl)
    , Constraints(this, &TControl::GetConstraintsImpl, &TControl::SetConstraintsImpl)
    , Hint(this, &TControl::GetHintImpl, &TControl::SetHintImpl)
    , ShowHint(this, &TControl::GetShowHintImpl, &TControl::SetShowHintImpl)
    , ParentShowHint(this, &TControl::GetParentShowHintImpl, &TControl::SetParentShowHintImpl)
    , Cursor(this, &TControl::GetCursorImpl, &TControl::SetCursorImpl)
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
    , font_(handle ? internal::TControl_GetFont(handle) : nullptr)
    , borderSpacing_(handle ? internal::TControl_GetBorderSpacing(handle) : nullptr)
    , constraints_(handle ? internal::TControl_GetConstraints(handle) : nullptr)
{}

// ---- docs/adr/0042 ----

int TControl::GetClientWidthImpl(TObject* owner)
{
    return internal::TControl_GetClientWidth(owner->Handle());
}

void TControl::SetClientWidthImpl(TObject* owner, const int& value)
{
    internal::TControl_SetClientWidth(owner->Handle(), value);
}

int TControl::GetClientHeightImpl(TObject* owner)
{
    return internal::TControl_GetClientHeight(owner->Handle());
}

void TControl::SetClientHeightImpl(TObject* owner, const int& value)
{
    internal::TControl_SetClientHeight(owner->Handle(), value);
}

void TControl::Invalidate() { internal::TControl_Invalidate(handle_); }

void TControl::Repaint() { internal::TControl_Repaint(handle_); }

void TControl::Refresh() { internal::TControl_Refresh(handle_); }

void TControl::Update() { internal::TControl_Update(handle_); }

void TControl::BringToFront() { internal::TControl_BringToFront(handle_); }

void TControl::SendToBack() { internal::TControl_SendToBack(handle_); }

void TControl::SetBounds(int ALeft, int ATop, int AWidth, int AHeight)
{
    internal::TControl_SetBounds(handle_, ALeft, ATop, AWidth, AHeight);
}

void TControl::Show() { internal::TControl_Show(handle_); }
void TControl::Hide() { internal::TControl_Hide(handle_); }

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
        internal::TControl_SetOnClick(self->handle_, &TControl::ClickTrampoline, nullptr);
        self->onClickHooked_ = true;
    }
}

void BETH_CALL TControl::ClickTrampoline(ObjectHandle sender, void*)
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
    return static_cast<TWinControl*>(FromHandle(internal::TControl_GetParent(owner->Handle())));
}

void TControl::SetParentImpl(TObject* owner, TWinControl* const& value)
{
    internal::TControl_SetParent(owner->Handle(), HandleOf(value));
}

int  TControl::GetLeftImpl(TObject* owner)                      { return internal::TControl_GetLeft(owner->Handle()); }
void TControl::SetLeftImpl(TObject* owner, const int& value)    { internal::TControl_SetLeft(owner->Handle(), value); }
int  TControl::GetTopImpl(TObject* owner)                       { return internal::TControl_GetTop(owner->Handle()); }
void TControl::SetTopImpl(TObject* owner, const int& value)     { internal::TControl_SetTop(owner->Handle(), value); }
int  TControl::GetWidthImpl(TObject* owner)                     { return internal::TControl_GetWidth(owner->Handle()); }
void TControl::SetWidthImpl(TObject* owner, const int& value)   { internal::TControl_SetWidth(owner->Handle(), value); }
int  TControl::GetHeightImpl(TObject* owner)                    { return internal::TControl_GetHeight(owner->Handle()); }
void TControl::SetHeightImpl(TObject* owner, const int& value)  { internal::TControl_SetHeight(owner->Handle(), value); }
bool TControl::GetVisibleImpl(TObject* owner)                   { return internal::TControl_GetVisible(owner->Handle()) != 0; }
void TControl::SetVisibleImpl(TObject* owner, const bool& value){ internal::TControl_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TControl::GetEnabledImpl(TObject* owner)                   { return internal::TControl_GetEnabled(owner->Handle()) != 0; }
void TControl::SetEnabledImpl(TObject* owner, const bool& value){ internal::TControl_SetEnabled(owner->Handle(), value ? 1 : 0); }

std::string TControl::GetCaptionImpl(TObject* owner)
{
    return std::string(internal::TControl_GetCaption(owner->Handle()));
}

void TControl::SetCaptionImpl(TObject* owner, const std::string& value)
{
    internal::TControl_SetCaption(owner->Handle(), value.c_str());
}

TAlign TControl::GetAlignImpl(TObject* owner)                  { return static_cast<TAlign>(internal::TControl_GetAlign(owner->Handle())); }
void   TControl::SetAlignImpl(TObject* owner, const TAlign& value) { internal::TControl_SetAlign(owner->Handle(), value); }
bool   TControl::GetAutoSizeImpl(TObject* owner)                  { return internal::TControl_GetAutoSize(owner->Handle()) != 0; }
void   TControl::SetAutoSizeImpl(TObject* owner, const bool& value) { internal::TControl_SetAutoSize(owner->Handle(), value ? 1 : 0); }

TBasicAction* TControl::GetActionImpl(TObject* owner)
{
    return static_cast<TBasicAction*>(FromHandle(internal::TControl_GetAction(owner->Handle())));
}

void TControl::SetActionImpl(TObject* owner, TBasicAction* const& value)
{
    internal::TControl_SetAction(owner->Handle(), HandleOf(value));
}

TPopupMenu* TControl::GetPopupMenuImpl(TObject* owner)
{
    return static_cast<TPopupMenu*>(FromHandle(internal::TControl_GetPopupMenu(owner->Handle())));
}

void TControl::SetPopupMenuImpl(TObject* owner, TPopupMenu* const& value)
{
    internal::TControl_SetPopupMenu(owner->Handle(), HandleOf(value));
}

TColor TControl::GetColorImpl(TObject* owner)                      { return static_cast<TColor>(internal::TControl_GetColor(owner->Handle())); }
void   TControl::SetColorImpl(TObject* owner, const TColor& value) { internal::TControl_SetColor(owner->Handle(), value); }
TFont* TControl::GetFontImpl(TObject* owner)                       { return &static_cast<TControl*>(owner)->font_; }
void   TControl::SetFontImpl(TObject* owner, TFont* const& value)  { internal::TControl_SetFont(owner->Handle(), value ? value->Handle() : nullptr); }

bool TControl::GetParentColorImpl(TObject* owner)                     { return internal::TControl_GetParentColor(owner->Handle()) != 0; }
void TControl::SetParentColorImpl(TObject* owner, const bool& value)  { internal::TControl_SetParentColor(owner->Handle(), value ? 1 : 0); }
bool TControl::GetParentFontImpl(TObject* owner)                      { return internal::TControl_GetParentFont(owner->Handle()) != 0; }
void TControl::SetParentFontImpl(TObject* owner, const bool& value)   { internal::TControl_SetParentFont(owner->Handle(), value ? 1 : 0); }
TAnchors TControl::GetAnchorsImpl(TObject* owner)                        { return TAnchors::FromInt(internal::TControl_GetAnchors(owner->Handle())); }
void     TControl::SetAnchorsImpl(TObject* owner, const TAnchors& value) { internal::TControl_SetAnchors(owner->Handle(), value.ToInt()); }

TControlBorderSpacing* TControl::GetBorderSpacingImpl(TObject* owner) { return &static_cast<TControl*>(owner)->borderSpacing_; }
void TControl::SetBorderSpacingImpl(TObject* owner, TControlBorderSpacing* const& value)
{
    internal::TControl_SetBorderSpacing(owner->Handle(), value ? value->Handle() : nullptr);
}
TSizeConstraints* TControl::GetConstraintsImpl(TObject* owner) { return &static_cast<TControl*>(owner)->constraints_; }
void TControl::SetConstraintsImpl(TObject* owner, TSizeConstraints* const& value)
{
    internal::TControl_SetConstraints(owner->Handle(), value ? value->Handle() : nullptr);
}

std::string TControl::GetHintImpl(TObject* owner)                           { return std::string(internal::TControl_GetHint(owner->Handle())); }
void        TControl::SetHintImpl(TObject* owner, const std::string& value) { internal::TControl_SetHint(owner->Handle(), value.c_str()); }
bool TControl::GetShowHintImpl(TObject* owner)                          { return internal::TControl_GetShowHint(owner->Handle()) != 0; }
void TControl::SetShowHintImpl(TObject* owner, const bool& value)       { internal::TControl_SetShowHint(owner->Handle(), value ? 1 : 0); }
bool TControl::GetParentShowHintImpl(TObject* owner)                    { return internal::TControl_GetParentShowHint(owner->Handle()) != 0; }
void TControl::SetParentShowHintImpl(TObject* owner, const bool& value) { internal::TControl_SetParentShowHint(owner->Handle(), value ? 1 : 0); }
TCursor TControl::GetCursorImpl(TObject* owner)                       { return static_cast<TCursor>(internal::TControl_GetCursor(owner->Handle())); }
void    TControl::SetCursorImpl(TObject* owner, const TCursor& value) { internal::TControl_SetCursor(owner->Handle(), value); }

std::string TControl::GetTextImpl(TObject* owner)
{
    return std::string(internal::TControl_GetText(owner->Handle()));
}

void TControl::SetTextImpl(TObject* owner, const std::string& value)
{
    internal::TControl_SetText(owner->Handle(), value.c_str());
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
                   &internal::TControl_SetOnDblClick, &TControl::DblClickTrampoline);
}

void TControl::SetOnResizeImpl(TObject* owner, const TNotifyEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    SetSimpleEvent(self->handle_, self->onResize_, self->onResizeHooked_, value,
                   &internal::TControl_SetOnResize, &TControl::ResizeTrampoline);
}

void TControl::SetOnMouseEnterImpl(TObject* owner, const TNotifyEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    SetSimpleEvent(self->handle_, self->onMouseEnter_, self->onMouseEnterHooked_, value,
                   &internal::TControl_SetOnMouseEnter, &TControl::MouseEnterTrampoline);
}

void TControl::SetOnMouseLeaveImpl(TObject* owner, const TNotifyEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    SetSimpleEvent(self->handle_, self->onMouseLeave_, self->onMouseLeaveHooked_, value,
                   &internal::TControl_SetOnMouseLeave, &TControl::MouseLeaveTrampoline);
}

void TControl::SetOnMouseDownImpl(TObject* owner, const TMouseEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseDown_ = value;
    if (value && !self->onMouseDownHooked_)
    {
        internal::TControl_SetOnMouseDown(self->handle_, &TControl::MouseDownTrampoline, nullptr);
        self->onMouseDownHooked_ = true;
    }
}

void TControl::SetOnMouseUpImpl(TObject* owner, const TMouseEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseUp_ = value;
    if (value && !self->onMouseUpHooked_)
    {
        internal::TControl_SetOnMouseUp(self->handle_, &TControl::MouseUpTrampoline, nullptr);
        self->onMouseUpHooked_ = true;
    }
}

void TControl::SetOnMouseMoveImpl(TObject* owner, const TMouseMoveEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseMove_ = value;
    if (value && !self->onMouseMoveHooked_)
    {
        internal::TControl_SetOnMouseMove(self->handle_, &TControl::MouseMoveTrampoline, nullptr);
        self->onMouseMoveHooked_ = true;
    }
}

void TControl::SetOnMouseWheelImpl(TObject* owner, const TMouseWheelEvent& value)
{
    TControl* self = static_cast<TControl*>(owner);
    self->onMouseWheel_ = value;
    if (value && !self->onMouseWheelHooked_)
    {
        internal::TControl_SetOnMouseWheel(self->handle_, &TControl::MouseWheelTrampoline, nullptr);
        self->onMouseWheelHooked_ = true;
    }
}

void BETH_CALL TControl::DblClickTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
            CallNotify(self->onDblClick_, self);
    });
}

void BETH_CALL TControl::ResizeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
            CallNotify(self->onResize_, self);
    });
}

void BETH_CALL TControl::MouseEnterTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
            CallNotify(self->onMouseEnter_, self);
    });
}

void BETH_CALL TControl::MouseLeaveTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TControl* self = static_cast<TControl*>(FromHandle(sender)))
            CallNotify(self->onMouseLeave_, self);
    });
}

void BETH_CALL TControl::MouseDownTrampoline(ObjectHandle sender, internal::int_t button, internal::int_t shift, internal::int_t x, internal::int_t y, void*)
{
    GuardCallback([&] {
        TControl* self = static_cast<TControl*>(FromHandle(sender));
        if (!self || !self->onMouseDown_)
            return;
        TMouseEvent handler = self->onMouseDown_;
        handler(self, static_cast<TMouseButton>(button), static_cast<TShiftState>(shift), x, y);
    });
}

void BETH_CALL TControl::MouseUpTrampoline(ObjectHandle sender, internal::int_t button, internal::int_t shift, internal::int_t x, internal::int_t y, void*)
{
    GuardCallback([&] {
        TControl* self = static_cast<TControl*>(FromHandle(sender));
        if (!self || !self->onMouseUp_)
            return;
        TMouseEvent handler = self->onMouseUp_;
        handler(self, static_cast<TMouseButton>(button), static_cast<TShiftState>(shift), x, y);
    });
}

void BETH_CALL TControl::MouseMoveTrampoline(ObjectHandle sender, internal::int_t shift, internal::int_t x, internal::int_t y, void*)
{
    GuardCallback([&] {
        TControl* self = static_cast<TControl*>(FromHandle(sender));
        if (!self || !self->onMouseMove_)
            return;
        TMouseMoveEvent handler = self->onMouseMove_;
        handler(self, static_cast<TShiftState>(shift), x, y);
    });
}

void BETH_CALL TControl::MouseWheelTrampoline(ObjectHandle sender, internal::int_t shift, internal::int_t wheelDelta, internal::int_t x, internal::int_t y, internal::bool_t* handled, void*)
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

TWinControl::TWinControl(ObjectHandle handle)
    : TControl(handle)
    , OnEnter(this, &TWinControl::GetOnEnterImpl, &TWinControl::SetOnEnterImpl)
    , OnExit(this, &TWinControl::GetOnExitImpl, &TWinControl::SetOnExitImpl)
    , OnKeyDown(this, &TWinControl::GetOnKeyDownImpl, &TWinControl::SetOnKeyDownImpl)
    , OnKeyUp(this, &TWinControl::GetOnKeyUpImpl, &TWinControl::SetOnKeyUpImpl)
    , OnKeyPress(this, &TWinControl::GetOnKeyPressImpl, &TWinControl::SetOnKeyPressImpl)
    , TabOrder(this, &TWinControl::GetTabOrderImpl, &TWinControl::SetTabOrderImpl)
    , TabStop(this, &TWinControl::GetTabStopImpl, &TWinControl::SetTabStopImpl)
{}

// ---- docs/adr/0042 ----

void TWinControl::SetFocus() { internal::TWinControl_SetFocus(handle_); }

bool TWinControl::CanFocus() const { return internal::TWinControl_CanFocus(handle_) != 0; }

bool TWinControl::Focused() const { return internal::TWinControl_Focused(handle_) != 0; }

TNotifyEvent TWinControl::GetOnEnterImpl(TObject* owner)
{
    return static_cast<TWinControl*>(owner)->onEnter_;
}
void TWinControl::SetOnEnterImpl(TObject* owner, const TNotifyEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onEnter_ = value;
    if (value && !self->onEnterHooked_)
    {
        internal::TWinControl_SetOnEnter(self->handle_, &TWinControl::EnterTrampoline, nullptr);
        self->onEnterHooked_ = true;
    }
}
void BETH_CALL TWinControl::EnterTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TWinControl* self = static_cast<TWinControl*>(FromHandle(sender));
        if (!self || !self->onEnter_)
            return;
        TNotifyEvent handler = self->onEnter_;
        handler(self);
    });
}

TNotifyEvent TWinControl::GetOnExitImpl(TObject* owner)
{
    return static_cast<TWinControl*>(owner)->onExit_;
}
void TWinControl::SetOnExitImpl(TObject* owner, const TNotifyEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onExit_ = value;
    if (value && !self->onExitHooked_)
    {
        internal::TWinControl_SetOnExit(self->handle_, &TWinControl::ExitTrampoline, nullptr);
        self->onExitHooked_ = true;
    }
}
void BETH_CALL TWinControl::ExitTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TWinControl* self = static_cast<TWinControl*>(FromHandle(sender));
        if (!self || !self->onExit_)
            return;
        TNotifyEvent handler = self->onExit_;
        handler(self);
    });
}

int  TWinControl::GetTabOrderImpl(TObject* owner)                   { return internal::TWinControl_GetTabOrder(owner->Handle()); }
void TWinControl::SetTabOrderImpl(TObject* owner, const int& value)  { internal::TWinControl_SetTabOrder(owner->Handle(), value); }
bool TWinControl::GetTabStopImpl(TObject* owner)                    { return internal::TWinControl_GetTabStop(owner->Handle()) != 0; }
void TWinControl::SetTabStopImpl(TObject* owner, const bool& value) { internal::TWinControl_SetTabStop(owner->Handle(), value ? 1 : 0); }

TKeyEvent TWinControl::GetOnKeyDownImpl(TObject* owner) { return static_cast<TWinControl*>(owner)->onKeyDown_; }
TKeyEvent TWinControl::GetOnKeyUpImpl(TObject* owner)   { return static_cast<TWinControl*>(owner)->onKeyUp_; }
TKeyPressEvent TWinControl::GetOnKeyPressImpl(TObject* owner) { return static_cast<TWinControl*>(owner)->onKeyPress_; }

void TWinControl::SetOnKeyDownImpl(TObject* owner, const TKeyEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onKeyDown_ = value;
    if (value && !self->onKeyDownHooked_)
    {
        internal::TWinControl_SetOnKeyDown(self->handle_, &TWinControl::KeyDownTrampoline, nullptr);
        self->onKeyDownHooked_ = true;
    }
}

void TWinControl::SetOnKeyUpImpl(TObject* owner, const TKeyEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onKeyUp_ = value;
    if (value && !self->onKeyUpHooked_)
    {
        internal::TWinControl_SetOnKeyUp(self->handle_, &TWinControl::KeyUpTrampoline, nullptr);
        self->onKeyUpHooked_ = true;
    }
}

void TWinControl::SetOnKeyPressImpl(TObject* owner, const TKeyPressEvent& value)
{
    TWinControl* self = static_cast<TWinControl*>(owner);
    self->onKeyPress_ = value;
    if (value && !self->onKeyPressHooked_)
    {
        internal::TWinControl_SetOnKeyPress(self->handle_, &TWinControl::KeyPressTrampoline, nullptr);
        self->onKeyPressHooked_ = true;
    }
}

void BETH_CALL TWinControl::KeyDownTrampoline(ObjectHandle sender, internal::int_t* key, internal::int_t shift, void*)
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

void BETH_CALL TWinControl::KeyUpTrampoline(ObjectHandle sender, internal::int_t* key, internal::int_t shift, void*)
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

void BETH_CALL TWinControl::KeyPressTrampoline(ObjectHandle sender, internal::int_t* key, void*)
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

TCustomScrollBar::TCustomScrollBar(ObjectHandle handle)
    : TWinControl(handle)
    , Kind(this, &TCustomScrollBar::GetKindImpl, &TCustomScrollBar::SetKindImpl)
    , Min(this, &TCustomScrollBar::GetMinImpl, &TCustomScrollBar::SetMinImpl)
    , Max(this, &TCustomScrollBar::GetMaxImpl, &TCustomScrollBar::SetMaxImpl)
    , Position(this, &TCustomScrollBar::GetPositionImpl, &TCustomScrollBar::SetPositionImpl)
    , PageSize(this, &TCustomScrollBar::GetPageSizeImpl, &TCustomScrollBar::SetPageSizeImpl)
    , OnChange(this, &TCustomScrollBar::GetOnChangeImpl, &TCustomScrollBar::SetOnChangeImpl)
{}

TScrollBarKind TCustomScrollBar::GetKindImpl(TObject* owner) { return static_cast<TScrollBarKind>(internal::TCustomScrollBar_GetKind(owner->Handle())); }
void TCustomScrollBar::SetKindImpl(TObject* owner, const TScrollBarKind& value) { internal::TCustomScrollBar_SetKind(owner->Handle(), value); }
int  TCustomScrollBar::GetMinImpl(TObject* owner)      { return internal::TCustomScrollBar_GetMin(owner->Handle()); }
void TCustomScrollBar::SetMinImpl(TObject* owner, const int& value)      { internal::TCustomScrollBar_SetMin(owner->Handle(), value); }
int  TCustomScrollBar::GetMaxImpl(TObject* owner)      { return internal::TCustomScrollBar_GetMax(owner->Handle()); }
void TCustomScrollBar::SetMaxImpl(TObject* owner, const int& value)      { internal::TCustomScrollBar_SetMax(owner->Handle(), value); }
int  TCustomScrollBar::GetPositionImpl(TObject* owner) { return internal::TCustomScrollBar_GetPosition(owner->Handle()); }
void TCustomScrollBar::SetPositionImpl(TObject* owner, const int& value) { internal::TCustomScrollBar_SetPosition(owner->Handle(), value); }
int  TCustomScrollBar::GetPageSizeImpl(TObject* owner) { return internal::TCustomScrollBar_GetPageSize(owner->Handle()); }
void TCustomScrollBar::SetPageSizeImpl(TObject* owner, const int& value) { internal::TCustomScrollBar_SetPageSize(owner->Handle(), value); }
TNotifyEvent TCustomScrollBar::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomScrollBar*>(owner)->onChange_; }

void TCustomScrollBar::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomScrollBar* self = static_cast<TCustomScrollBar*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &internal::TCustomScrollBar_SetOnChange, &TCustomScrollBar::ChangeTrampoline);
}

void BETH_CALL TCustomScrollBar::ChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomScrollBar* self = static_cast<TCustomScrollBar*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TScrollBar::TScrollBar(TComponent* AOwner)
    : TCustomScrollBar(internal::TScrollBar_Create(HandleOf(AOwner)))
{}

TCustomTrackBar::TCustomTrackBar(ObjectHandle handle)
    : TWinControl(handle)
    , Min(this, &TCustomTrackBar::GetMinImpl, &TCustomTrackBar::SetMinImpl)
    , Max(this, &TCustomTrackBar::GetMaxImpl, &TCustomTrackBar::SetMaxImpl)
    , Position(this, &TCustomTrackBar::GetPositionImpl, &TCustomTrackBar::SetPositionImpl)
    , OnChange(this, &TCustomTrackBar::GetOnChangeImpl, &TCustomTrackBar::SetOnChangeImpl)
{}

int  TCustomTrackBar::GetMinImpl(TObject* owner)      { return internal::TCustomTrackBar_GetMin(owner->Handle()); }
void TCustomTrackBar::SetMinImpl(TObject* owner, const int& value)      { internal::TCustomTrackBar_SetMin(owner->Handle(), value); }
int  TCustomTrackBar::GetMaxImpl(TObject* owner)      { return internal::TCustomTrackBar_GetMax(owner->Handle()); }
void TCustomTrackBar::SetMaxImpl(TObject* owner, const int& value)      { internal::TCustomTrackBar_SetMax(owner->Handle(), value); }
int  TCustomTrackBar::GetPositionImpl(TObject* owner) { return internal::TCustomTrackBar_GetPosition(owner->Handle()); }
void TCustomTrackBar::SetPositionImpl(TObject* owner, const int& value) { internal::TCustomTrackBar_SetPosition(owner->Handle(), value); }
TNotifyEvent TCustomTrackBar::GetOnChangeImpl(TObject* owner) { return static_cast<TCustomTrackBar*>(owner)->onChange_; }

void TCustomTrackBar::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomTrackBar* self = static_cast<TCustomTrackBar*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &internal::TCustomTrackBar_SetOnChange, &TCustomTrackBar::ChangeTrampoline);
}

void BETH_CALL TCustomTrackBar::ChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomTrackBar* self = static_cast<TCustomTrackBar*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TTrackBar::TTrackBar(TComponent* AOwner)
    : TCustomTrackBar(internal::TTrackBar_Create(HandleOf(AOwner)))
{}

TCustomProgressBar::TCustomProgressBar(ObjectHandle handle)
    : TWinControl(handle)
    , Min(this, &TCustomProgressBar::GetMinImpl, &TCustomProgressBar::SetMinImpl)
    , Max(this, &TCustomProgressBar::GetMaxImpl, &TCustomProgressBar::SetMaxImpl)
    , Position(this, &TCustomProgressBar::GetPositionImpl, &TCustomProgressBar::SetPositionImpl)
{}

int  TCustomProgressBar::GetMinImpl(TObject* owner)      { return internal::TCustomProgressBar_GetMin(owner->Handle()); }
void TCustomProgressBar::SetMinImpl(TObject* owner, const int& value)      { internal::TCustomProgressBar_SetMin(owner->Handle(), value); }
int  TCustomProgressBar::GetMaxImpl(TObject* owner)      { return internal::TCustomProgressBar_GetMax(owner->Handle()); }
void TCustomProgressBar::SetMaxImpl(TObject* owner, const int& value)      { internal::TCustomProgressBar_SetMax(owner->Handle(), value); }
int  TCustomProgressBar::GetPositionImpl(TObject* owner) { return internal::TCustomProgressBar_GetPosition(owner->Handle()); }
void TCustomProgressBar::SetPositionImpl(TObject* owner, const int& value) { internal::TCustomProgressBar_SetPosition(owner->Handle(), value); }

TProgressBar::TProgressBar(TComponent* AOwner)
    : TCustomProgressBar(internal::TProgressBar_Create(HandleOf(AOwner)))
{}

TCustomControl::TCustomControl(ObjectHandle handle)
    : TWinControl(handle)
    , Canvas(internal::TCustomControl_GetCanvas(handle))
    , OnPaint(this, &TCustomControl::GetOnPaintImpl, &TCustomControl::SetOnPaintImpl)
{}

TNotifyEvent TCustomControl::GetOnPaintImpl(TObject* owner)
{
    return static_cast<TCustomControl*>(owner)->onPaint_;
}

void TCustomControl::SetOnPaintImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomControl* self = static_cast<TCustomControl*>(owner);
    SetSimpleEvent(self->handle_, self->onPaint_, self->onPaintHooked_, value,
                   &internal::TCustomControl_SetOnPaint, &TCustomControl::PaintTrampoline);
}

void BETH_CALL TCustomControl::PaintTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCustomControl* self = static_cast<TCustomControl*>(FromHandle(sender));
        if (!self || !self->onPaint_)
            return;
        TNotifyEvent handler = self->onPaint_;
        handler(self);
    });
}

TScrollBox::TScrollBox(TComponent* AOwner)
    : TScrollingWinControl(internal::TScrollBox_Create(HandleOf(AOwner)))
{}

TUpDown::TUpDown(TComponent* AOwner)
    : TCustomControl(internal::TUpDown_Create(HandleOf(AOwner)))
    , Min(this, &TUpDown::GetMinImpl, &TUpDown::SetMinImpl)
    , Max(this, &TUpDown::GetMaxImpl, &TUpDown::SetMaxImpl)
    , Position(this, &TUpDown::GetPositionImpl, &TUpDown::SetPositionImpl)
    , Increment(this, &TUpDown::GetIncrementImpl, &TUpDown::SetIncrementImpl)
    , Associate(this, &TUpDown::GetAssociateImpl, &TUpDown::SetAssociateImpl)
{}

int  TUpDown::GetMinImpl(TObject* owner)       { return internal::TUpDown_GetMin(owner->Handle()); }
void TUpDown::SetMinImpl(TObject* owner, const int& value)       { internal::TUpDown_SetMin(owner->Handle(), value); }
int  TUpDown::GetMaxImpl(TObject* owner)       { return internal::TUpDown_GetMax(owner->Handle()); }
void TUpDown::SetMaxImpl(TObject* owner, const int& value)       { internal::TUpDown_SetMax(owner->Handle(), value); }
int  TUpDown::GetPositionImpl(TObject* owner)  { return internal::TUpDown_GetPosition(owner->Handle()); }
void TUpDown::SetPositionImpl(TObject* owner, const int& value)  { internal::TUpDown_SetPosition(owner->Handle(), value); }
int  TUpDown::GetIncrementImpl(TObject* owner) { return internal::TUpDown_GetIncrement(owner->Handle()); }
void TUpDown::SetIncrementImpl(TObject* owner, const int& value) { internal::TUpDown_SetIncrement(owner->Handle(), value); }

TWinControl* TUpDown::GetAssociateImpl(TObject* owner)
{
    return static_cast<TWinControl*>(FromHandle(internal::TUpDown_GetAssociate(owner->Handle())));
}

void TUpDown::SetAssociateImpl(TObject* owner, TWinControl* const& value)
{
    internal::TUpDown_SetAssociate(owner->Handle(), HandleOf(value));
}

/* ---------------- Form ---------------- */

void TCustomForm::Show()      { internal::TCustomForm_Show(handle_); }
void TCustomForm::Hide()      { internal::TCustomForm_Hide(handle_); }
TModalResult TCustomForm::ShowModal() { return internal::TCustomForm_ShowModal(handle_); }
void TCustomForm::Close()     { internal::TCustomForm_Close(handle_); }
void TCustomForm::Release()   { internal::TCustomForm_Release(handle_); }

TCustomForm::TCustomForm(ObjectHandle handle)
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
    , ModalResult(this, &TCustomForm::GetModalResultImpl, &TCustomForm::SetModalResultImpl)
    , BorderStyle(this, &TCustomForm::GetBorderStyleImpl, &TCustomForm::SetBorderStyleImpl)
    , Position(this, &TCustomForm::GetPositionImpl, &TCustomForm::SetPositionImpl)
    , WindowState(this, &TCustomForm::GetWindowStateImpl, &TCustomForm::SetWindowStateImpl)
    , BorderIcons(this, &TCustomForm::GetBorderIconsImpl, &TCustomForm::SetBorderIconsImpl)
    , FormStyle(this, &TCustomForm::GetFormStyleImpl, &TCustomForm::SetFormStyleImpl)
    , KeyPreview(this, &TCustomForm::GetKeyPreviewImpl, &TCustomForm::SetKeyPreviewImpl)
    , ActiveControl(this, &TCustomForm::GetActiveControlImpl, &TCustomForm::SetActiveControlImpl)
    , Icon(this, &TCustomForm::GetIconImpl, &TCustomForm::SetIconImpl)
    , AllowDropFiles(this, &TCustomForm::GetAllowDropFilesImpl, &TCustomForm::SetAllowDropFilesImpl)
    , OnDropFiles(this, &TCustomForm::GetOnDropFilesImpl, &TCustomForm::SetOnDropFilesImpl)
    , icon_(this, &internal::TCustomForm_GetIcon)
{
    // OnShow のブリッジは常に登録する。new で直接生成したフォームの OnCreate を、最初の表示の直前に呼ぶため。
    internal::TCustomForm_SetOnShow(handle_, &TCustomForm::ShowTrampoline, nullptr);
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

void BETH_CALL TCustomForm::ShowTrampoline(ObjectHandle sender, void*)
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

void BETH_CALL TCustomForm::CloseTrampoline(ObjectHandle sender, internal::int_t* action, void*)
{
    GuardCallback([&] {
        TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender));
        if (!self || !self->onClose_)
            return;
        TCloseEvent handler = self->onClose_;
        TCloseAction value = static_cast<TCloseAction>(*action);
        handler(self, value);
        *action = static_cast<internal::int_t>(value);
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
        internal::TCustomForm_SetOnClose(self->handle_, &TCustomForm::CloseTrampoline, nullptr);
        self->onCloseHooked_ = true;
    }
}

void BETH_CALL TCustomForm::HideTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
            CallNotify(self->onHide_, self);
    });
}

void BETH_CALL TCustomForm::ActivateTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
            CallNotify(self->onActivate_, self);
    });
}

void BETH_CALL TCustomForm::DeactivateTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
            CallNotify(self->onDeactivate_, self);
    });
}

void BETH_CALL TCustomForm::DestroyTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomForm* self = dynamic_cast<TCustomForm*>(FromHandle(sender)))
            CallNotify(self->onDestroy_, self);
    });
}

void BETH_CALL TCustomForm::CloseQueryTrampoline(ObjectHandle sender, internal::bool_t* canClose, void*)
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
                 &internal::TCustomForm_SetOnHide, &TCustomForm::HideTrampoline);
}

void TCustomForm::SetOnActivateImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onActivate_, self->onActivateHooked_, value,
                 &internal::TCustomForm_SetOnActivate, &TCustomForm::ActivateTrampoline);
}

void TCustomForm::SetOnDeactivateImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onDeactivate_, self->onDeactivateHooked_, value,
                 &internal::TCustomForm_SetOnDeactivate, &TCustomForm::DeactivateTrampoline);
}

void TCustomForm::SetOnDestroyImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onDestroy_, self->onDestroyHooked_, value,
                 &internal::TCustomForm_SetOnDestroy, &TCustomForm::DestroyTrampoline);
}

TModalResult TCustomForm::GetModalResultImpl(TObject* owner)
{
    return internal::TCustomForm_GetModalResult(owner->Handle());
}
void TCustomForm::SetModalResultImpl(TObject* owner, const TModalResult& value)
{
    internal::TCustomForm_SetModalResult(owner->Handle(), value);
}
TFormBorderStyle TCustomForm::GetBorderStyleImpl(TObject* owner)
{
    return static_cast<TFormBorderStyle>(internal::TCustomForm_GetBorderStyle(owner->Handle()));
}
void TCustomForm::SetBorderStyleImpl(TObject* owner, const TFormBorderStyle& value)
{
    internal::TCustomForm_SetBorderStyle(owner->Handle(), value);
}
TPosition TCustomForm::GetPositionImpl(TObject* owner)
{
    return static_cast<TPosition>(internal::TCustomForm_GetPosition(owner->Handle()));
}
void TCustomForm::SetPositionImpl(TObject* owner, const TPosition& value)
{
    internal::TCustomForm_SetPosition(owner->Handle(), value);
}
TWindowState TCustomForm::GetWindowStateImpl(TObject* owner)
{
    return static_cast<TWindowState>(internal::TCustomForm_GetWindowState(owner->Handle()));
}
void TCustomForm::SetWindowStateImpl(TObject* owner, const TWindowState& value)
{
    internal::TCustomForm_SetWindowState(owner->Handle(), value);
}
TBorderIcons TCustomForm::GetBorderIconsImpl(TObject* owner)
{
    return TBorderIcons::FromInt(internal::TCustomForm_GetBorderIcons(owner->Handle()));
}
void TCustomForm::SetBorderIconsImpl(TObject* owner, const TBorderIcons& value)
{
    internal::TCustomForm_SetBorderIcons(owner->Handle(), value.ToInt());
}
TFormStyle TCustomForm::GetFormStyleImpl(TObject* owner)
{
    return static_cast<TFormStyle>(internal::TCustomForm_GetFormStyle(owner->Handle()));
}
void TCustomForm::SetFormStyleImpl(TObject* owner, const TFormStyle& value)
{
    internal::TCustomForm_SetFormStyle(owner->Handle(), value);
}
bool TCustomForm::GetKeyPreviewImpl(TObject* owner)
{
    return internal::TCustomForm_GetKeyPreview(owner->Handle()) != 0;
}
void TCustomForm::SetKeyPreviewImpl(TObject* owner, const bool& value)
{
    internal::TCustomForm_SetKeyPreview(owner->Handle(), value ? 1 : 0);
}
TWinControl* TCustomForm::GetActiveControlImpl(TObject* owner)
{
    return dynamic_cast<TWinControl*>(FromHandle(internal::TCustomForm_GetActiveControl(owner->Handle())));
}
void TCustomForm::SetActiveControlImpl(TObject* owner, TWinControl* const& value)
{
    internal::TCustomForm_SetActiveControl(owner->Handle(), value ? value->Handle() : nullptr);
}

// ---- docs/adr/0047 ----

TIcon* TCustomForm::GetIconImpl(TObject* owner) { return &static_cast<TCustomForm*>(owner)->icon_; }
void TCustomForm::SetIconImpl(TObject* owner, TIcon* const& value)
{
    internal::TCustomForm_SetIcon(owner->Handle(), value ? value->Current() : nullptr);
}

bool TCustomForm::GetAllowDropFilesImpl(TObject* owner)
{
    return internal::TCustomForm_GetAllowDropFiles(owner->Handle()) != 0;
}
void TCustomForm::SetAllowDropFilesImpl(TObject* owner, const bool& value)
{
    internal::TCustomForm_SetAllowDropFiles(owner->Handle(), value ? 1 : 0);
}

TDropFilesEvent TCustomForm::GetOnDropFilesImpl(TObject* owner) { return static_cast<TCustomForm*>(owner)->onDropFiles_; }
void TCustomForm::SetOnDropFilesImpl(TObject* owner, const TDropFilesEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onDropFiles_, self->onDropFilesHooked_, value,
                   &internal::TCustomForm_SetOnDropFiles, &TCustomForm::DropFilesTrampoline);
}

void BETH_CALL TCustomForm::DropFilesTrampoline(ObjectHandle sender, internal::int_t count, internal::str_t* fileNames, void*)
{
    GuardCallback([&] {
        TCustomForm* self = static_cast<TCustomForm*>(FromHandle(sender));
        if (!self || !self->onDropFiles_)
            return;
        TDropFilesEvent handler = self->onDropFiles_;
        std::vector<std::string> names;
        names.reserve(count);
        for (int i = 0; i < count; ++i)
            names.emplace_back(fileNames[i] ? fileNames[i] : "");
        handler(self, names);
    });
}

TMainMenu* TCustomForm::GetMenuImpl(TObject* owner)
{
    return static_cast<TMainMenu*>(FromHandle(internal::TCustomForm_GetMenu(owner->Handle())));
}

void TCustomForm::SetMenuImpl(TObject* owner, TMainMenu* const& value)
{
    internal::TCustomForm_SetMenu(owner->Handle(), HandleOf(value));
}

void TCustomForm::SetOnCloseQueryImpl(TObject* owner, const TCloseQueryEvent& value)
{
    TCustomForm* self = static_cast<TCustomForm*>(owner);
    SetSimpleEvent(self->handle_, self->onCloseQuery_, self->onCloseQueryHooked_, value,
                 &internal::TCustomForm_SetOnCloseQuery, &TCustomForm::CloseQueryTrampoline);
}

ObjectHandle TForm::pendingHandle_ = nullptr;

TForm::TForm(TComponent* AOwner)
    : TCustomForm(CreateHandle(AOwner))
{}

ObjectHandle TForm::CreateHandle(TComponent* AOwner)
{
    if (pendingHandle_ && AOwner && AOwner == Application)
    {
        ObjectHandle handle = pendingHandle_;
        pendingHandle_ = nullptr;
        return handle;
    }
    return internal::TForm_Create(HandleOf(AOwner));
}

/* ---------------- TApplication ---------------- */

TApplication* NewApplication()
{
    return new TApplication(internal::GetApplication());
}

TApplication* Application = NewApplication();

TApplication::TApplication(ObjectHandle handle)
    : TComponent(handle)
    , ExeName(this, &TApplication::GetExeNameImpl)
    , Hint(this, &TApplication::GetHintImpl, &TApplication::SetHintImpl)
    , ShowHint(this, &TApplication::GetShowHintImpl, &TApplication::SetShowHintImpl)
    , HintPause(this, &TApplication::GetHintPauseImpl, &TApplication::SetHintPauseImpl)
    , HintHidePause(this, &TApplication::GetHintHidePauseImpl, &TApplication::SetHintHidePauseImpl)
    , OnIdle(this, &TApplication::GetOnIdleImpl, &TApplication::SetOnIdleImpl)
    , OnException(this, &TApplication::GetOnExceptionImpl, &TApplication::SetOnExceptionImpl)
    , Icon(this, &TApplication::GetIconImpl, &TApplication::SetIconImpl)
    , MainForm(this, &TApplication::GetMainFormImpl)
    , Terminated(this, &TApplication::GetTerminatedImpl)
    , Title(this, &TApplication::GetTitleImpl, &TApplication::SetTitleImpl)
    , ShowMainForm(this, &TApplication::GetShowMainFormImpl, &TApplication::SetShowMainFormImpl)
    , icon_(this, &internal::TApplication_GetIcon)
{
    // 基底の TComponent のコンストラクタでレジストリ(関数内 static)が構築済みのため、
    // ここで登録した終了処理はレジストリの破棄より先に呼ばれる。
    std::atexit(&TApplication::Shutdown);
}

// ---- docs/adr/0043 ----

std::string TApplication::GetExeNameImpl(TObject* owner)
{
    return internal::TApplication_GetExeName(owner->Handle());
}

std::string TApplication::GetHintImpl(TObject* owner)
{
    return internal::TApplication_GetHint(owner->Handle());
}

void TApplication::SetHintImpl(TObject* owner, const std::string& value)
{
    internal::TApplication_SetHint(owner->Handle(), value.c_str());
}

bool TApplication::GetShowHintImpl(TObject* owner)
{
    return internal::TApplication_GetShowHint(owner->Handle()) != 0;
}

void TApplication::SetShowHintImpl(TObject* owner, const bool& value)
{
    internal::TApplication_SetShowHint(owner->Handle(), value ? 1 : 0);
}

int TApplication::GetHintPauseImpl(TObject* owner)
{
    return internal::TApplication_GetHintPause(owner->Handle());
}

void TApplication::SetHintPauseImpl(TObject* owner, const int& value)
{
    internal::TApplication_SetHintPause(owner->Handle(), value);
}

int TApplication::GetHintHidePauseImpl(TObject* owner)
{
    return internal::TApplication_GetHintHidePause(owner->Handle());
}

void TApplication::SetHintHidePauseImpl(TObject* owner, const int& value)
{
    internal::TApplication_SetHintHidePause(owner->Handle(), value);
}

void TApplication::Minimize() { internal::TApplication_Minimize(handle_); }

void TApplication::Restore() { internal::TApplication_Restore(handle_); }

void TApplication::BringToFront() { internal::TApplication_BringToFront(handle_); }

TIdleEvent TApplication::GetOnIdleImpl(TObject* owner)
{
    return static_cast<TApplication*>(owner)->onIdle_;
}
void TApplication::SetOnIdleImpl(TObject* owner, const TIdleEvent& value)
{
    TApplication* self = static_cast<TApplication*>(owner);
    self->onIdle_ = value;
    if (value && !self->onIdleHooked_)
    {
        internal::TApplication_SetOnIdle(self->handle_, &TApplication::IdleTrampoline, nullptr);
        self->onIdleHooked_ = true;
    }
}

void BETH_CALL TApplication::IdleTrampoline(ObjectHandle, internal::bool_t* done, void*)
{
    GuardCallback([&] {
        TApplication* self = Application;
        if (!self || !self->onIdle_)
            return;
        TIdleEvent handler = self->onIdle_;
        bool d = *done != 0;
        handler(self, d);
        *done = d ? 1 : 0;
    });
}

TExceptionEvent TApplication::GetOnExceptionImpl(TObject* owner)
{
    return static_cast<TApplication*>(owner)->onException_;
}
void TApplication::SetOnExceptionImpl(TObject* owner, const TExceptionEvent& value)
{
    TApplication* self = static_cast<TApplication*>(owner);
    self->onException_ = value;
    if (value && !self->onExceptionHooked_)
    {
        internal::TApplication_SetOnException(self->handle_, &TApplication::ExceptionTrampoline, nullptr);
        self->onExceptionHooked_ = true;
    }
}

void BETH_CALL TApplication::ExceptionTrampoline(ObjectHandle sender, internal::str_t className, internal::str_t message, void*)
{
    GuardCallback([&] {
        // Sender は例外を渡したもの(LCL のタイマーの例外では nil)なので、ハンドラは Application のものを呼ぶ
        TApplication* self = Application;
        if (!self || !self->onException_)
            return;
        TExceptionEvent handler = self->onException_;
        const Exception e(className ? className : "", message ? message : "");
        handler(sender ? FromHandle(sender) : nullptr, e);
    });
}

TIcon* TApplication::GetIconImpl(TObject* owner) { return &static_cast<TApplication*>(owner)->icon_; }
void TApplication::SetIconImpl(TObject* owner, TIcon* const& value)
{
    internal::TApplication_SetIcon(owner->Handle(), value ? value->Current() : nullptr);
}

// main から戻った後(C++ の実行環境がまだ有効なうち)に、Application が所有するフォームを破棄する。
// 破棄通知によってラッパーのデストラクタも呼ばれる。その後の DLL の切り離しでは通知は来ない。
// Application 自身のラッパーは解放しない(LCL の Application は DLL の切り離しまで生きている)。
void TApplication::Shutdown()
{
    if (Application)
        internal::TComponent_DestroyComponents(Application->handle_);
    internal::FreeNotify_SetCallback(nullptr, nullptr);
    internal::ItemFree_SetCallback(nullptr, nullptr);
}

/* ---------------- TScreen・TClipboard(docs/adr/0047) ---------------- */

TScreen* NewScreen()
{
    return new TScreen(internal::GetScreen());
}

TScreen* Screen = NewScreen();

TScreen::TScreen(ObjectHandle handle)
    : TComponent(handle)
    , Cursor(this, &TScreen::GetCursorImpl, &TScreen::SetCursorImpl)
    , Width(this, &TScreen::GetWidthImpl)
    , Height(this, &TScreen::GetHeightImpl)
    , DesktopLeft(this, &TScreen::GetDesktopLeftImpl)
    , DesktopTop(this, &TScreen::GetDesktopTopImpl)
    , DesktopWidth(this, &TScreen::GetDesktopWidthImpl)
    , DesktopHeight(this, &TScreen::GetDesktopHeightImpl)
    , WorkAreaLeft(this, &TScreen::GetWorkAreaLeftImpl)
    , WorkAreaTop(this, &TScreen::GetWorkAreaTopImpl)
    , WorkAreaWidth(this, &TScreen::GetWorkAreaWidthImpl)
    , WorkAreaHeight(this, &TScreen::GetWorkAreaHeightImpl)
    , WorkAreaRect(this, &TScreen::GetWorkAreaRectImpl)
    , PixelsPerInch(this, &TScreen::GetPixelsPerInchImpl)
    , MonitorCount(this, &TScreen::GetMonitorCountImpl)
    , FormCount(this, &TScreen::GetFormCountImpl)
    , Forms(this, &TScreen::GetFormsImpl)
    , ActiveForm(this, &TScreen::GetActiveFormImpl)
    , ActiveControl(this, &TScreen::GetActiveControlImpl)
    , Fonts(this, &TScreen::GetFontsImpl)
    , OnActiveFormChange(this, &TScreen::GetOnActiveFormChangeImpl, &TScreen::SetOnActiveFormChangeImpl)
    , OnActiveControlChange(this, &TScreen::GetOnActiveControlChangeImpl, &TScreen::SetOnActiveControlChangeImpl)
    , fonts_(this, &internal::TScreen_GetFonts)
{}

TCursor TScreen::GetCursorImpl(TObject* owner) { return internal::TScreen_GetCursor(owner->Handle()); }
void TScreen::SetCursorImpl(TObject* owner, const TCursor& value) { internal::TScreen_SetCursor(owner->Handle(), value); }
int TScreen::GetWidthImpl(TObject* owner) { return internal::TScreen_GetWidth(owner->Handle()); }
int TScreen::GetHeightImpl(TObject* owner) { return internal::TScreen_GetHeight(owner->Handle()); }
int TScreen::GetDesktopLeftImpl(TObject* owner) { return internal::TScreen_GetDesktopLeft(owner->Handle()); }
int TScreen::GetDesktopTopImpl(TObject* owner) { return internal::TScreen_GetDesktopTop(owner->Handle()); }
int TScreen::GetDesktopWidthImpl(TObject* owner) { return internal::TScreen_GetDesktopWidth(owner->Handle()); }
int TScreen::GetDesktopHeightImpl(TObject* owner) { return internal::TScreen_GetDesktopHeight(owner->Handle()); }
int TScreen::GetWorkAreaLeftImpl(TObject* owner) { return internal::TScreen_GetWorkAreaLeft(owner->Handle()); }
int TScreen::GetWorkAreaTopImpl(TObject* owner) { return internal::TScreen_GetWorkAreaTop(owner->Handle()); }
int TScreen::GetWorkAreaWidthImpl(TObject* owner) { return internal::TScreen_GetWorkAreaWidth(owner->Handle()); }
int TScreen::GetWorkAreaHeightImpl(TObject* owner) { return internal::TScreen_GetWorkAreaHeight(owner->Handle()); }
int TScreen::GetPixelsPerInchImpl(TObject* owner) { return internal::TScreen_GetPixelsPerInch(owner->Handle()); }
int TScreen::GetMonitorCountImpl(TObject* owner) { return internal::TScreen_GetMonitorCount(owner->Handle()); }
int TScreen::GetFormCountImpl(TObject* owner) { return internal::TScreen_GetFormCount(owner->Handle()); }

TRect TScreen::GetWorkAreaRectImpl(TObject* owner)
{
    internal::int_t l = 0, t = 0, r = 0, b = 0;
    internal::TScreen_GetWorkAreaRect(owner->Handle(), &l, &t, &r, &b);
    return TRect{l, t, r, b};
}

TForm* TScreen::GetFormsImpl(TObject* owner, int Index)
{
    return dynamic_cast<TForm*>(FromHandle(internal::TScreen_GetForms(owner->Handle(), Index)));
}

TForm* TScreen::GetActiveFormImpl(TObject* owner)
{
    return dynamic_cast<TForm*>(FromHandle(internal::TScreen_GetActiveForm(owner->Handle())));
}

TWinControl* TScreen::GetActiveControlImpl(TObject* owner)
{
    return dynamic_cast<TWinControl*>(FromHandle(internal::TScreen_GetActiveControl(owner->Handle())));
}

TStrings* TScreen::GetFontsImpl(TObject* owner) { return &static_cast<TScreen*>(owner)->fonts_; }

TNotifyEvent TScreen::GetOnActiveFormChangeImpl(TObject* owner) { return static_cast<TScreen*>(owner)->onActiveFormChange_; }
void TScreen::SetOnActiveFormChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TScreen* self = static_cast<TScreen*>(owner);
    SetSimpleEvent(self->handle_, self->onActiveFormChange_, self->onActiveFormChangeHooked_, value,
                   &internal::TScreen_SetOnActiveFormChange, &TScreen::ActiveFormChangeTrampoline);
}

TNotifyEvent TScreen::GetOnActiveControlChangeImpl(TObject* owner) { return static_cast<TScreen*>(owner)->onActiveControlChange_; }
void TScreen::SetOnActiveControlChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TScreen* self = static_cast<TScreen*>(owner);
    SetSimpleEvent(self->handle_, self->onActiveControlChange_, self->onActiveControlChangeHooked_, value,
                   &internal::TScreen_SetOnActiveControlChange, &TScreen::ActiveControlChangeTrampoline);
}

void BETH_CALL TScreen::ActiveFormChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TScreen* self = static_cast<TScreen*>(FromHandle(sender));
        if (!self || !self->onActiveFormChange_)
            return;
        TNotifyEvent handler = self->onActiveFormChange_;
        handler(self);
    });
}

void BETH_CALL TScreen::ActiveControlChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TScreen* self = static_cast<TScreen*>(FromHandle(sender));
        if (!self || !self->onActiveControlChange_)
            return;
        TNotifyEvent handler = self->onActiveControlChange_;
        handler(self);
    });
}

TClipboardFormat CF_Text()    { return internal::Clipboard_CF_Text(); }
TClipboardFormat CF_Bitmap()  { return internal::Clipboard_CF_Bitmap(); }
TClipboardFormat CF_Picture() { return internal::Clipboard_CF_Picture(); }

// LCL の Clipboard は最初に使うときに作られ、プログラムの終了まで同じもの。
TClipboard* Clipboard()
{
    static TClipboard* clipboard = new TClipboard(internal::GetClipboard());
    return clipboard;
}

TClipboard::TClipboard(ObjectHandle handle)
    : TPersistent(handle)
    , AsText(this, &TClipboard::GetAsTextImpl, &TClipboard::SetAsTextImpl)
    , FormatCount(this, &TClipboard::GetFormatCountImpl)
    , Formats(this, &TClipboard::GetFormatsImpl)
{}

bool TClipboard::HasFormat(TClipboardFormat Format) const { return internal::TClipboard_HasFormat(handle_, Format) != 0; }
bool TClipboard::HasPictureFormat() const { return internal::TClipboard_HasPictureFormat(handle_) != 0; }
void TClipboard::Clear() { internal::TClipboard_Clear(handle_); }
void TClipboard::Open() { internal::TClipboard_Open(handle_); }
void TClipboard::Close() { internal::TClipboard_Close(handle_); }
void TClipboard::Assign(const TPicture* Source)
{
    if (Source)
        internal::TClipboard_Assign(handle_, Source->Handle());
}
void TClipboard::Assign(const TGraphic* Source)
{
    if (Source)
        internal::TClipboard_Assign(handle_, Source->Current());
}

std::string TClipboard::GetAsTextImpl(TObject* owner) { return internal::TClipboard_GetAsText(owner->Handle()); }
void TClipboard::SetAsTextImpl(TObject* owner, const std::string& value)
{
    internal::TClipboard_SetAsText(owner->Handle(), value.c_str());
}
int TClipboard::GetFormatCountImpl(TObject* owner) { return internal::TClipboard_GetFormatCount(owner->Handle()); }
TClipboardFormat TClipboard::GetFormatsImpl(TObject* owner, int Index)
{
    return internal::TClipboard_GetFormats(owner->Handle(), Index);
}

void TApplication::BeginCreateForm()
{
    // 前回のハンドルが引き取られていなければ破棄する(T のコンストラクタが Application 以外を Owner にした場合)。
    if (TForm::pendingHandle_)
        internal::TComponent_Destroy(TForm::pendingHandle_);
    TForm::pendingHandle_ = internal::TApplication_CreateForm(handle_);
}

void TApplication::EndCreateForm()
{
    if (TForm::pendingHandle_)
    {
        internal::TComponent_Destroy(TForm::pendingHandle_);
        TForm::pendingHandle_ = nullptr;
    }
}

void TApplication::Run()             { internal::TApplication_Run(handle_); }
void TApplication::ProcessMessages() { internal::TApplication_ProcessMessages(handle_); }
void TApplication::Terminate()       { internal::TApplication_Terminate(handle_); }

int TApplication::MessageBoxImpl(const std::string& Text, const std::string& Caption, int Flags)
{
    return internal::TApplication_MessageBox(handle_, Text.c_str(), Caption.c_str(), Flags);
}

// ---- メッセージのダイアログ(docs/adr/0041) ----

const TMsgDlgButtons mbYesNo            = TMsgDlgButtons() << mbYes << mbNo;
const TMsgDlgButtons mbYesNoCancel      = TMsgDlgButtons() << mbYes << mbNo << mbCancel;
const TMsgDlgButtons mbOKCancel         = TMsgDlgButtons() << mbOK << mbCancel;
const TMsgDlgButtons mbAbortRetryIgnore = TMsgDlgButtons() << mbAbort << mbRetry << mbIgnore;

void ShowMessage(const std::string& Msg)
{
    internal::Dialogs_ShowMessage(Msg.c_str());
}

TModalResult MessageDlg(const std::string& Msg, TMsgDlgType DlgType, TMsgDlgButtons Buttons, int HelpCtx)
{
    return internal::Dialogs_MessageDlg("", Msg.c_str(), DlgType, Buttons.ToInt(), HelpCtx);
}

TModalResult MessageDlg(const std::string& Caption, const std::string& Msg, TMsgDlgType DlgType,
                        TMsgDlgButtons Buttons, int HelpCtx)
{
    return internal::Dialogs_MessageDlg(Caption.c_str(), Msg.c_str(), DlgType, Buttons.ToInt(), HelpCtx);
}

std::string InputBox(const std::string& ACaption, const std::string& APrompt, const std::string& ADefault)
{
    return internal::Dialogs_InputBox(ACaption.c_str(), APrompt.c_str(), ADefault.c_str());
}

std::string PasswordBox(const std::string& ACaption, const std::string& APrompt)
{
    return internal::Dialogs_PasswordBox(ACaption.c_str(), APrompt.c_str());
}

bool InputQuery(const std::string& ACaption, const std::string& APrompt, std::string& Value)
{
    internal::bool_t ok = 0;
    std::string result = internal::Dialogs_InputQuery(ACaption.c_str(), APrompt.c_str(), Value.c_str(), &ok);
    if (ok == 0)
        return false;
    Value = result;
    return true;
}

TForm* TApplication::GetMainFormImpl(TObject* owner)
{
    return dynamic_cast<TForm*>(FromHandle(internal::TApplication_GetMainForm(owner->Handle())));
}

bool TApplication::GetTerminatedImpl(TObject* owner)
{
    return internal::TApplication_GetTerminated(owner->Handle()) != 0;
}

std::string TApplication::GetTitleImpl(TObject* owner)
{
    return std::string(internal::TApplication_GetTitle(owner->Handle()));
}

void TApplication::SetTitleImpl(TObject* owner, const std::string& value)
{
    internal::TApplication_SetTitle(owner->Handle(), value.c_str());
}

bool TApplication::GetShowMainFormImpl(TObject* owner)
{
    return internal::TApplication_GetShowMainForm(owner->Handle()) != 0;
}

void TApplication::SetShowMainFormImpl(TObject* owner, const bool& value)
{
    internal::TApplication_SetShowMainForm(owner->Handle(), value ? 1 : 0);
}

/* ---------------- Panel / GroupBox / Label ---------------- */

TPanel::TPanel(TComponent* AOwner)
    : TCustomPanel(internal::TPanel_Create(HandleOf(AOwner)))
{}

TGroupBox::TGroupBox(TComponent* AOwner)
    : TCustomGroupBox(internal::TGroupBox_Create(HandleOf(AOwner)))
{}

TCustomRadioGroup::TCustomRadioGroup(ObjectHandle handle)
    : TCustomGroupBox(handle)
    , ItemIndex(this, &TCustomRadioGroup::GetItemIndexImpl, &TCustomRadioGroup::SetItemIndexImpl)
    , OnClick(this, &TCustomRadioGroup::GetOnClickImpl, &TCustomRadioGroup::SetOnClickImpl)
    , Items(this, &TCustomRadioGroup::GetItemsImpl)
    , items_(this, &internal::TCustomRadioGroup_GetItems)
{}

TStrings* TCustomRadioGroup::GetItemsImpl(TObject* owner) { return &static_cast<TCustomRadioGroup*>(owner)->items_; }

int  TCustomRadioGroup::GetItemIndexImpl(TObject* owner)                   { return internal::TCustomRadioGroup_GetItemIndex(owner->Handle()); }
void TCustomRadioGroup::SetItemIndexImpl(TObject* owner, const int& value) { internal::TCustomRadioGroup_SetItemIndex(owner->Handle(), value); }
TNotifyEvent TCustomRadioGroup::GetOnClickImpl(TObject* owner) { return static_cast<TCustomRadioGroup*>(owner)->onClick_; }

void TCustomRadioGroup::SetOnClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomRadioGroup* self = static_cast<TCustomRadioGroup*>(owner);
    SetSimpleEvent(self->handle_, self->onClick_, self->onClickHooked_, value,
                   &internal::TCustomRadioGroup_SetOnClick, &TCustomRadioGroup::ClickTrampoline);
}

void BETH_CALL TCustomRadioGroup::ClickTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomRadioGroup* self = static_cast<TCustomRadioGroup*>(FromHandle(sender)))
            CallNotify(self->onClick_, self);
    });
}

TRadioGroup::TRadioGroup(TComponent* AOwner)
    : TCustomRadioGroup(internal::TRadioGroup_Create(HandleOf(AOwner)))
{}

TStrings* TCustomCheckGroup::GetItemsImpl(TObject* owner) { return &static_cast<TCustomCheckGroup*>(owner)->items_; }

TCustomCheckGroup::TCustomCheckGroup(ObjectHandle handle)
    : TCustomGroupBox(handle)
    , Checked(this, &TCustomCheckGroup::GetCheckedImpl, &TCustomCheckGroup::SetCheckedImpl)
    , Items(this, &TCustomCheckGroup::GetItemsImpl)
    , items_(this, &internal::TCustomCheckGroup_GetItems)
{}

bool TCustomCheckGroup::GetCheckedImpl(TObject* owner, int index)                     { return internal::TCustomCheckGroup_GetChecked(owner->Handle(), index) != 0; }
void TCustomCheckGroup::SetCheckedImpl(TObject* owner, int index, const bool& value)  { internal::TCustomCheckGroup_SetChecked(owner->Handle(), index, value ? 1 : 0); }

TCheckGroup::TCheckGroup(TComponent* AOwner)
    : TCustomCheckGroup(internal::TCheckGroup_Create(HandleOf(AOwner)))
{}

TCustomLabel::TCustomLabel(ObjectHandle handle)
    : TGraphicControl(handle)
    , Alignment(this, &TCustomLabel::GetAlignmentImpl, &TCustomLabel::SetAlignmentImpl)
    , Layout(this, &TCustomLabel::GetLayoutImpl, &TCustomLabel::SetLayoutImpl)
    , WordWrap(this, &TCustomLabel::GetWordWrapImpl, &TCustomLabel::SetWordWrapImpl)
    , Transparent(this, &TCustomLabel::GetTransparentImpl, &TCustomLabel::SetTransparentImpl)
    , FocusControl(this, &TCustomLabel::GetFocusControlImpl, &TCustomLabel::SetFocusControlImpl)
    , ShowAccelChar(this, &TCustomLabel::GetShowAccelCharImpl, &TCustomLabel::SetShowAccelCharImpl)
{}

TAlignment TCustomLabel::GetAlignmentImpl(TObject* owner)
{
    return static_cast<TAlignment>(internal::TCustomLabel_GetAlignment(owner->Handle()));
}

void TCustomLabel::SetAlignmentImpl(TObject* owner, const TAlignment& value)
{
    internal::TCustomLabel_SetAlignment(owner->Handle(), value);
}

TTextLayout TCustomLabel::GetLayoutImpl(TObject* owner)
{
    return static_cast<TTextLayout>(internal::TCustomLabel_GetLayout(owner->Handle()));
}

void TCustomLabel::SetLayoutImpl(TObject* owner, const TTextLayout& value)
{
    internal::TCustomLabel_SetLayout(owner->Handle(), value);
}

bool TCustomLabel::GetWordWrapImpl(TObject* owner)
{
    return internal::TCustomLabel_GetWordWrap(owner->Handle()) != 0;
}

void TCustomLabel::SetWordWrapImpl(TObject* owner, const bool& value)
{
    internal::TCustomLabel_SetWordWrap(owner->Handle(), value ? 1 : 0);
}

bool TCustomLabel::GetTransparentImpl(TObject* owner)
{
    return internal::TCustomLabel_GetTransparent(owner->Handle()) != 0;
}

void TCustomLabel::SetTransparentImpl(TObject* owner, const bool& value)
{
    internal::TCustomLabel_SetTransparent(owner->Handle(), value ? 1 : 0);
}

TWinControl* TCustomLabel::GetFocusControlImpl(TObject* owner)
{
    return dynamic_cast<TWinControl*>(FromHandle(internal::TCustomLabel_GetFocusControl(owner->Handle())));
}

void TCustomLabel::SetFocusControlImpl(TObject* owner, TWinControl* const& value)
{
    internal::TCustomLabel_SetFocusControl(owner->Handle(), value ? value->Handle() : nullptr);
}

bool TCustomLabel::GetShowAccelCharImpl(TObject* owner)
{
    return internal::TCustomLabel_GetShowAccelChar(owner->Handle()) != 0;
}

void TCustomLabel::SetShowAccelCharImpl(TObject* owner, const bool& value)
{
    internal::TCustomLabel_SetShowAccelChar(owner->Handle(), value ? 1 : 0);
}

TLabel::TLabel(TComponent* AOwner)
    : TCustomLabel(internal::TLabel_Create(HandleOf(AOwner)))
{}

TBevel::TBevel(TComponent* AOwner)
    : TGraphicControl(internal::TBevel_Create(HandleOf(AOwner)))
    , Shape(this, &TBevel::GetShapeImpl, &TBevel::SetShapeImpl)
    , Style(this, &TBevel::GetStyleImpl, &TBevel::SetStyleImpl)
{}

TBevelShape TBevel::GetShapeImpl(TObject* owner) { return static_cast<TBevelShape>(internal::TBevel_GetShape(owner->Handle())); }
void TBevel::SetShapeImpl(TObject* owner, const TBevelShape& value) { internal::TBevel_SetShape(owner->Handle(), value); }
TBevelStyle TBevel::GetStyleImpl(TObject* owner) { return static_cast<TBevelStyle>(internal::TBevel_GetStyle(owner->Handle())); }
void TBevel::SetStyleImpl(TObject* owner, const TBevelStyle& value) { internal::TBevel_SetStyle(owner->Handle(), value); }

TCustomShape::TCustomShape(ObjectHandle handle)
    : TGraphicControl(handle)
    , Pen(internal::TCustomShape_GetPen(handle))
    , Brush(internal::TCustomShape_GetBrush(handle))
    , Shape(this, &TCustomShape::GetShapeImpl, &TCustomShape::SetShapeImpl)
{}

TShapeType TCustomShape::GetShapeImpl(TObject* owner) { return static_cast<TShapeType>(internal::TCustomShape_GetShape(owner->Handle())); }
void TCustomShape::SetShapeImpl(TObject* owner, const TShapeType& value) { internal::TCustomShape_SetShape(owner->Handle(), value); }

TShape::TShape(TComponent* AOwner)
    : TCustomShape(internal::TShape_Create(HandleOf(AOwner)))
{}

TCustomSpeedButton::TCustomSpeedButton(ObjectHandle handle)
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
    , glyph_(this, &internal::TCustomSpeedButton_GetGlyph)
{}

TBitmap* TCustomSpeedButton::GetGlyphImpl(TObject* owner) { return &static_cast<TCustomSpeedButton*>(owner)->glyph_; }
void TCustomSpeedButton::SetGlyphImpl(TObject* owner, TBitmap* const& value)
{
    internal::TCustomSpeedButton_SetGlyph(owner->Handle(), value ? value->Current() : nullptr);
}
int  TCustomSpeedButton::GetNumGlyphsImpl(TObject* owner) { return internal::TCustomSpeedButton_GetNumGlyphs(owner->Handle()); }
void TCustomSpeedButton::SetNumGlyphsImpl(TObject* owner, const int& value) { internal::TCustomSpeedButton_SetNumGlyphs(owner->Handle(), value); }
TButtonLayout TCustomSpeedButton::GetLayoutImpl(TObject* owner)
{
    return static_cast<TButtonLayout>(internal::TCustomSpeedButton_GetLayout(owner->Handle()));
}
void TCustomSpeedButton::SetLayoutImpl(TObject* owner, const TButtonLayout& value) { internal::TCustomSpeedButton_SetLayout(owner->Handle(), value); }
int  TCustomSpeedButton::GetMarginImpl(TObject* owner)    { return internal::TCustomSpeedButton_GetMargin(owner->Handle()); }
void TCustomSpeedButton::SetMarginImpl(TObject* owner, const int& value) { internal::TCustomSpeedButton_SetMargin(owner->Handle(), value); }
int  TCustomSpeedButton::GetSpacingImpl(TObject* owner)   { return internal::TCustomSpeedButton_GetSpacing(owner->Handle()); }
void TCustomSpeedButton::SetSpacingImpl(TObject* owner, const int& value) { internal::TCustomSpeedButton_SetSpacing(owner->Handle(), value); }

bool TCustomSpeedButton::GetDownImpl(TObject* owner)       { return internal::TCustomSpeedButton_GetDown(owner->Handle()) != 0; }
void TCustomSpeedButton::SetDownImpl(TObject* owner, const bool& value)       { internal::TCustomSpeedButton_SetDown(owner->Handle(), value ? 1 : 0); }
int  TCustomSpeedButton::GetGroupIndexImpl(TObject* owner) { return internal::TCustomSpeedButton_GetGroupIndex(owner->Handle()); }
void TCustomSpeedButton::SetGroupIndexImpl(TObject* owner, const int& value) { internal::TCustomSpeedButton_SetGroupIndex(owner->Handle(), value); }
bool TCustomSpeedButton::GetFlatImpl(TObject* owner)       { return internal::TCustomSpeedButton_GetFlat(owner->Handle()) != 0; }
void TCustomSpeedButton::SetFlatImpl(TObject* owner, const bool& value)       { internal::TCustomSpeedButton_SetFlat(owner->Handle(), value ? 1 : 0); }
bool TCustomSpeedButton::GetAllowAllUpImpl(TObject* owner) { return internal::TCustomSpeedButton_GetAllowAllUp(owner->Handle()) != 0; }
void TCustomSpeedButton::SetAllowAllUpImpl(TObject* owner, const bool& value) { internal::TCustomSpeedButton_SetAllowAllUp(owner->Handle(), value ? 1 : 0); }

TSpeedButton::TSpeedButton(TComponent* AOwner)
    : TCustomSpeedButton(internal::TSpeedButton_Create(HandleOf(AOwner)))
{}

/* ---------------- Button / CheckBox / RadioButton ---------------- */

TButtonControl::TButtonControl(ObjectHandle handle)
    : TWinControl(handle)
    , Checked(this, &TButtonControl::GetCheckedImpl, &TButtonControl::SetCheckedImpl)
{}

TCustomButton::TCustomButton(ObjectHandle handle)
    : TButtonControl(handle)
    , ModalResult(this, &TCustomButton::GetModalResultImpl, &TCustomButton::SetModalResultImpl)
    , Default(this, &TCustomButton::GetDefaultImpl, &TCustomButton::SetDefaultImpl)
    , Cancel(this, &TCustomButton::GetCancelImpl, &TCustomButton::SetCancelImpl)
{}

TModalResult TCustomButton::GetModalResultImpl(TObject* owner)
{
    return internal::TCustomButton_GetModalResult(owner->Handle());
}
void TCustomButton::SetModalResultImpl(TObject* owner, const TModalResult& value)
{
    internal::TCustomButton_SetModalResult(owner->Handle(), value);
}
bool TCustomButton::GetDefaultImpl(TObject* owner)
{
    return internal::TCustomButton_GetDefault(owner->Handle()) != 0;
}
void TCustomButton::SetDefaultImpl(TObject* owner, const bool& value)
{
    internal::TCustomButton_SetDefault(owner->Handle(), value ? 1 : 0);
}
bool TCustomButton::GetCancelImpl(TObject* owner)
{
    return internal::TCustomButton_GetCancel(owner->Handle()) != 0;
}
void TCustomButton::SetCancelImpl(TObject* owner, const bool& value)
{
    internal::TCustomButton_SetCancel(owner->Handle(), value ? 1 : 0);
}

bool TButtonControl::GetCheckedImpl(TObject* owner)
{
    return internal::TButtonControl_GetChecked(owner->Handle()) != 0;
}

void TButtonControl::SetCheckedImpl(TObject* owner, const bool& value)
{
    internal::TButtonControl_SetChecked(owner->Handle(), value ? 1 : 0);
}

TButton::TButton(TComponent* AOwner)
    : TCustomButton(internal::TButton_Create(HandleOf(AOwner)))
{}

TCustomBitBtn::TCustomBitBtn(ObjectHandle handle)
    : TCustomButton(handle)
    , Kind(this, &TCustomBitBtn::GetKindImpl, &TCustomBitBtn::SetKindImpl)
    , Glyph(this, &TCustomBitBtn::GetGlyphImpl, &TCustomBitBtn::SetGlyphImpl)
    , NumGlyphs(this, &TCustomBitBtn::GetNumGlyphsImpl, &TCustomBitBtn::SetNumGlyphsImpl)
    , Layout(this, &TCustomBitBtn::GetLayoutImpl, &TCustomBitBtn::SetLayoutImpl)
    , Margin(this, &TCustomBitBtn::GetMarginImpl, &TCustomBitBtn::SetMarginImpl)
    , Spacing(this, &TCustomBitBtn::GetSpacingImpl, &TCustomBitBtn::SetSpacingImpl)
    , Images(this, &TCustomBitBtn::GetImagesImpl, &TCustomBitBtn::SetImagesImpl)
    , ImageIndex(this, &TCustomBitBtn::GetImageIndexImpl, &TCustomBitBtn::SetImageIndexImpl)
    , glyph_(this, &internal::TCustomBitBtn_GetGlyph)
{}

TBitBtnKind TCustomBitBtn::GetKindImpl(TObject* owner) { return static_cast<TBitBtnKind>(internal::TCustomBitBtn_GetKind(owner->Handle())); }
void TCustomBitBtn::SetKindImpl(TObject* owner, const TBitBtnKind& value) { internal::TCustomBitBtn_SetKind(owner->Handle(), value); }
TBitmap* TCustomBitBtn::GetGlyphImpl(TObject* owner) { return &static_cast<TCustomBitBtn*>(owner)->glyph_; }
void TCustomBitBtn::SetGlyphImpl(TObject* owner, TBitmap* const& value)
{
    internal::TCustomBitBtn_SetGlyph(owner->Handle(), value ? value->Current() : nullptr);
}
int  TCustomBitBtn::GetNumGlyphsImpl(TObject* owner) { return internal::TCustomBitBtn_GetNumGlyphs(owner->Handle()); }
void TCustomBitBtn::SetNumGlyphsImpl(TObject* owner, const int& value) { internal::TCustomBitBtn_SetNumGlyphs(owner->Handle(), value); }
TButtonLayout TCustomBitBtn::GetLayoutImpl(TObject* owner) { return static_cast<TButtonLayout>(internal::TCustomBitBtn_GetLayout(owner->Handle())); }
void TCustomBitBtn::SetLayoutImpl(TObject* owner, const TButtonLayout& value) { internal::TCustomBitBtn_SetLayout(owner->Handle(), value); }
int  TCustomBitBtn::GetMarginImpl(TObject* owner)    { return internal::TCustomBitBtn_GetMargin(owner->Handle()); }
void TCustomBitBtn::SetMarginImpl(TObject* owner, const int& value) { internal::TCustomBitBtn_SetMargin(owner->Handle(), value); }
int  TCustomBitBtn::GetSpacingImpl(TObject* owner)   { return internal::TCustomBitBtn_GetSpacing(owner->Handle()); }
void TCustomBitBtn::SetSpacingImpl(TObject* owner, const int& value) { internal::TCustomBitBtn_SetSpacing(owner->Handle(), value); }

TBitBtn::TBitBtn(TComponent* AOwner)
    : TCustomBitBtn(internal::TBitBtn_Create(HandleOf(AOwner)))
{}

TCustomCheckBox::TCustomCheckBox(ObjectHandle handle)
    : TButtonControl(handle)
    , State(this, &TCustomCheckBox::GetStateImpl, &TCustomCheckBox::SetStateImpl)
    , AllowGrayed(this, &TCustomCheckBox::GetAllowGrayedImpl, &TCustomCheckBox::SetAllowGrayedImpl)
    , OnChange(this, &TCustomCheckBox::GetOnChangeImpl, &TCustomCheckBox::SetOnChangeImpl)
{}

TCheckBoxState TCustomCheckBox::GetStateImpl(TObject* owner)
{
    return static_cast<TCheckBoxState>(internal::TCustomCheckBox_GetState(owner->Handle()));
}

void TCustomCheckBox::SetStateImpl(TObject* owner, const TCheckBoxState& value)
{
    internal::TCustomCheckBox_SetState(owner->Handle(), value);
}

bool TCustomCheckBox::GetAllowGrayedImpl(TObject* owner)
{
    return internal::TCustomCheckBox_GetAllowGrayed(owner->Handle()) != 0;
}

void TCustomCheckBox::SetAllowGrayedImpl(TObject* owner, const bool& value)
{
    internal::TCustomCheckBox_SetAllowGrayed(owner->Handle(), value ? 1 : 0);
}

TNotifyEvent TCustomCheckBox::GetOnChangeImpl(TObject* owner)
{
    return static_cast<TCustomCheckBox*>(owner)->onChange_;
}
void TCustomCheckBox::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomCheckBox* self = static_cast<TCustomCheckBox*>(owner);
    self->onChange_ = value;
    if (value && !self->onChangeHooked_)
    {
        internal::TCustomCheckBox_SetOnChange(self->handle_, &TCustomCheckBox::ChangeTrampoline, nullptr);
        self->onChangeHooked_ = true;
    }
}

void BETH_CALL TCustomCheckBox::ChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCustomCheckBox* self = static_cast<TCustomCheckBox*>(FromHandle(sender));
        if (!self || !self->onChange_)
            return;
        TNotifyEvent handler = self->onChange_;
        handler(self);
    });
}

TCheckBox::TCheckBox(TComponent* AOwner)
    : TCustomCheckBox(internal::TCheckBox_Create(HandleOf(AOwner)))
{}

TRadioButton::TRadioButton(TComponent* AOwner)
    : TCustomCheckBox(internal::TRadioButton_Create(HandleOf(AOwner)))
{}

TToggleBox::TToggleBox(TComponent* AOwner)
    : TCustomCheckBox(internal::TToggleBox_Create(HandleOf(AOwner)))
{}

/* ---------------- Edit / Memo ---------------- */

TCustomEdit::TCustomEdit(ObjectHandle handle)
    : TWinControl(handle)
    , SelStart(this, &TCustomEdit::GetSelStartImpl, &TCustomEdit::SetSelStartImpl)
    , SelLength(this, &TCustomEdit::GetSelLengthImpl, &TCustomEdit::SetSelLengthImpl)
    , SelText(this, &TCustomEdit::GetSelTextImpl, &TCustomEdit::SetSelTextImpl)
    , Modified(this, &TCustomEdit::GetModifiedImpl, &TCustomEdit::SetModifiedImpl)
    , CanUndo(this, &TCustomEdit::GetCanUndoImpl)
    , PasswordChar(this, &TCustomEdit::GetPasswordCharImpl, &TCustomEdit::SetPasswordCharImpl)
    , EchoMode(this, &TCustomEdit::GetEchoModeImpl, &TCustomEdit::SetEchoModeImpl)
    , CharCase(this, &TCustomEdit::GetCharCaseImpl, &TCustomEdit::SetCharCaseImpl)
    , Alignment(this, &TCustomEdit::GetAlignmentImpl, &TCustomEdit::SetAlignmentImpl)
    , TextHint(this, &TCustomEdit::GetTextHintImpl, &TCustomEdit::SetTextHintImpl)
    , NumbersOnly(this, &TCustomEdit::GetNumbersOnlyImpl, &TCustomEdit::SetNumbersOnlyImpl)
    , AutoSelect(this, &TCustomEdit::GetAutoSelectImpl, &TCustomEdit::SetAutoSelectImpl)
    , HideSelection(this, &TCustomEdit::GetHideSelectionImpl, &TCustomEdit::SetHideSelectionImpl)
    , CaretPos(this, &TCustomEdit::GetCaretPosImpl, &TCustomEdit::SetCaretPosImpl)
    , MaxLength(this, &TCustomEdit::GetMaxLengthImpl, &TCustomEdit::SetMaxLengthImpl)
    , ReadOnly(this, &TCustomEdit::GetReadOnlyImpl, &TCustomEdit::SetReadOnlyImpl)
    , OnChange(this, &TCustomEdit::GetOnChangeImpl, &TCustomEdit::SetOnChangeImpl)
{}

// ---- docs/adr/0042 ----

int TCustomEdit::GetSelStartImpl(TObject* owner)
{
    return internal::TCustomEdit_GetSelStart(owner->Handle());
}

void TCustomEdit::SetSelStartImpl(TObject* owner, const int& value)
{
    internal::TCustomEdit_SetSelStart(owner->Handle(), value);
}

int TCustomEdit::GetSelLengthImpl(TObject* owner)
{
    return internal::TCustomEdit_GetSelLength(owner->Handle());
}

void TCustomEdit::SetSelLengthImpl(TObject* owner, const int& value)
{
    internal::TCustomEdit_SetSelLength(owner->Handle(), value);
}

std::string TCustomEdit::GetSelTextImpl(TObject* owner)
{
    return internal::TCustomEdit_GetSelText(owner->Handle());
}

void TCustomEdit::SetSelTextImpl(TObject* owner, const std::string& value)
{
    internal::TCustomEdit_SetSelText(owner->Handle(), value.c_str());
}

bool TCustomEdit::GetModifiedImpl(TObject* owner)
{
    return internal::TCustomEdit_GetModified(owner->Handle()) != 0;
}

void TCustomEdit::SetModifiedImpl(TObject* owner, const bool& value)
{
    internal::TCustomEdit_SetModified(owner->Handle(), value ? 1 : 0);
}

bool TCustomEdit::GetCanUndoImpl(TObject* owner)
{
    return internal::TCustomEdit_GetCanUndo(owner->Handle()) != 0;
}

char TCustomEdit::GetPasswordCharImpl(TObject* owner)
{
    return internal::TCustomEdit_GetPasswordChar(owner->Handle());
}

void TCustomEdit::SetPasswordCharImpl(TObject* owner, const char& value)
{
    internal::TCustomEdit_SetPasswordChar(owner->Handle(), value);
}

TEchoMode TCustomEdit::GetEchoModeImpl(TObject* owner)
{
    return static_cast<TEchoMode>(internal::TCustomEdit_GetEchoMode(owner->Handle()));
}

void TCustomEdit::SetEchoModeImpl(TObject* owner, const TEchoMode& value)
{
    internal::TCustomEdit_SetEchoMode(owner->Handle(), value);
}

TEditCharCase TCustomEdit::GetCharCaseImpl(TObject* owner)
{
    return static_cast<TEditCharCase>(internal::TCustomEdit_GetCharCase(owner->Handle()));
}

void TCustomEdit::SetCharCaseImpl(TObject* owner, const TEditCharCase& value)
{
    internal::TCustomEdit_SetCharCase(owner->Handle(), value);
}

TAlignment TCustomEdit::GetAlignmentImpl(TObject* owner)
{
    return static_cast<TAlignment>(internal::TCustomEdit_GetAlignment(owner->Handle()));
}

void TCustomEdit::SetAlignmentImpl(TObject* owner, const TAlignment& value)
{
    internal::TCustomEdit_SetAlignment(owner->Handle(), value);
}

std::string TCustomEdit::GetTextHintImpl(TObject* owner)
{
    return internal::TCustomEdit_GetTextHint(owner->Handle());
}

void TCustomEdit::SetTextHintImpl(TObject* owner, const std::string& value)
{
    internal::TCustomEdit_SetTextHint(owner->Handle(), value.c_str());
}

bool TCustomEdit::GetNumbersOnlyImpl(TObject* owner)
{
    return internal::TCustomEdit_GetNumbersOnly(owner->Handle()) != 0;
}

void TCustomEdit::SetNumbersOnlyImpl(TObject* owner, const bool& value)
{
    internal::TCustomEdit_SetNumbersOnly(owner->Handle(), value ? 1 : 0);
}

bool TCustomEdit::GetAutoSelectImpl(TObject* owner)
{
    return internal::TCustomEdit_GetAutoSelect(owner->Handle()) != 0;
}

void TCustomEdit::SetAutoSelectImpl(TObject* owner, const bool& value)
{
    internal::TCustomEdit_SetAutoSelect(owner->Handle(), value ? 1 : 0);
}

bool TCustomEdit::GetHideSelectionImpl(TObject* owner)
{
    return internal::TCustomEdit_GetHideSelection(owner->Handle()) != 0;
}

void TCustomEdit::SetHideSelectionImpl(TObject* owner, const bool& value)
{
    internal::TCustomEdit_SetHideSelection(owner->Handle(), value ? 1 : 0);
}

TPoint TCustomEdit::GetCaretPosImpl(TObject* owner)
{
    internal::int_t x = 0, y = 0;
    internal::TCustomEdit_GetCaretPos(owner->Handle(), &x, &y);
    TPoint p = {x, y};
    return p;
}

void TCustomEdit::SetCaretPosImpl(TObject* owner, const TPoint& value)
{
    internal::TCustomEdit_SetCaretPos(owner->Handle(), value.X, value.Y);
}

void TCustomEdit::SelectAll() { internal::TCustomEdit_SelectAll(handle_); }

void TCustomEdit::ClearSelection() { internal::TCustomEdit_ClearSelection(handle_); }

void TCustomEdit::Clear() { internal::TCustomEdit_Clear(handle_); }

void TCustomEdit::CopyToClipboard() { internal::TCustomEdit_CopyToClipboard(handle_); }

void TCustomEdit::CutToClipboard() { internal::TCustomEdit_CutToClipboard(handle_); }

void TCustomEdit::PasteFromClipboard() { internal::TCustomEdit_PasteFromClipboard(handle_); }

void TCustomEdit::Undo() { internal::TCustomEdit_Undo(handle_); }

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
        internal::TCustomEdit_SetOnChange(self->handle_, &TCustomEdit::ChangeTrampoline, nullptr);
        self->onChangeHooked_ = true;
    }
}

void BETH_CALL TCustomEdit::ChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCustomEdit* self = static_cast<TCustomEdit*>(FromHandle(sender));
        if (!self || !self->onChange_)
            return;
        TNotifyEvent handler = self->onChange_;
        handler(self);
    });
}

int  TCustomEdit::GetMaxLengthImpl(TObject* owner)                   { return internal::TCustomEdit_GetMaxLength(owner->Handle()); }
void TCustomEdit::SetMaxLengthImpl(TObject* owner, const int& value) { internal::TCustomEdit_SetMaxLength(owner->Handle(), value); }
bool TCustomEdit::GetReadOnlyImpl(TObject* owner)                    { return internal::TCustomEdit_GetReadOnly(owner->Handle()) != 0; }
void TCustomEdit::SetReadOnlyImpl(TObject* owner, const bool& value) { internal::TCustomEdit_SetReadOnly(owner->Handle(), value ? 1 : 0); }

TEdit::TEdit(TComponent* AOwner)
    : TCustomEdit(internal::TEdit_Create(HandleOf(AOwner)))
{}

TCustomFloatSpinEdit::TCustomFloatSpinEdit(ObjectHandle handle)
    : TCustomEdit(handle)
    , Value(this, &TCustomFloatSpinEdit::GetValueImpl, &TCustomFloatSpinEdit::SetValueImpl)
    , MinValue(this, &TCustomFloatSpinEdit::GetMinValueImpl, &TCustomFloatSpinEdit::SetMinValueImpl)
    , MaxValue(this, &TCustomFloatSpinEdit::GetMaxValueImpl, &TCustomFloatSpinEdit::SetMaxValueImpl)
    , Increment(this, &TCustomFloatSpinEdit::GetIncrementImpl, &TCustomFloatSpinEdit::SetIncrementImpl)
    , DecimalPlaces(this, &TCustomFloatSpinEdit::GetDecimalPlacesImpl, &TCustomFloatSpinEdit::SetDecimalPlacesImpl)
{}

double TCustomFloatSpinEdit::GetValueImpl(TObject* owner)     { return internal::TCustomFloatSpinEdit_GetValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetValueImpl(TObject* owner, const double& value)     { internal::TCustomFloatSpinEdit_SetValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetMinValueImpl(TObject* owner)  { return internal::TCustomFloatSpinEdit_GetMinValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetMinValueImpl(TObject* owner, const double& value)  { internal::TCustomFloatSpinEdit_SetMinValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetMaxValueImpl(TObject* owner)  { return internal::TCustomFloatSpinEdit_GetMaxValue(owner->Handle()); }
void   TCustomFloatSpinEdit::SetMaxValueImpl(TObject* owner, const double& value)  { internal::TCustomFloatSpinEdit_SetMaxValue(owner->Handle(), value); }
double TCustomFloatSpinEdit::GetIncrementImpl(TObject* owner) { return internal::TCustomFloatSpinEdit_GetIncrement(owner->Handle()); }
void   TCustomFloatSpinEdit::SetIncrementImpl(TObject* owner, const double& value) { internal::TCustomFloatSpinEdit_SetIncrement(owner->Handle(), value); }
int    TCustomFloatSpinEdit::GetDecimalPlacesImpl(TObject* owner) { return internal::TCustomFloatSpinEdit_GetDecimalPlaces(owner->Handle()); }
void   TCustomFloatSpinEdit::SetDecimalPlacesImpl(TObject* owner, const int& value) { internal::TCustomFloatSpinEdit_SetDecimalPlaces(owner->Handle(), value); }

TFloatSpinEdit::TFloatSpinEdit(TComponent* AOwner)
    : TCustomFloatSpinEdit(internal::TFloatSpinEdit_Create(HandleOf(AOwner)))
{}

TCustomSpinEdit::TCustomSpinEdit(ObjectHandle handle)
    : TCustomFloatSpinEdit(handle)
    , Value(this, &TCustomSpinEdit::GetValueImpl, &TCustomSpinEdit::SetValueImpl)
    , MinValue(this, &TCustomSpinEdit::GetMinValueImpl, &TCustomSpinEdit::SetMinValueImpl)
    , MaxValue(this, &TCustomSpinEdit::GetMaxValueImpl, &TCustomSpinEdit::SetMaxValueImpl)
    , Increment(this, &TCustomSpinEdit::GetIncrementImpl, &TCustomSpinEdit::SetIncrementImpl)
{}

int  TCustomSpinEdit::GetValueImpl(TObject* owner)     { return internal::TCustomSpinEdit_GetValue(owner->Handle()); }
void TCustomSpinEdit::SetValueImpl(TObject* owner, const int& value)     { internal::TCustomSpinEdit_SetValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetMinValueImpl(TObject* owner)  { return internal::TCustomSpinEdit_GetMinValue(owner->Handle()); }
void TCustomSpinEdit::SetMinValueImpl(TObject* owner, const int& value)  { internal::TCustomSpinEdit_SetMinValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetMaxValueImpl(TObject* owner)  { return internal::TCustomSpinEdit_GetMaxValue(owner->Handle()); }
void TCustomSpinEdit::SetMaxValueImpl(TObject* owner, const int& value)  { internal::TCustomSpinEdit_SetMaxValue(owner->Handle(), value); }
int  TCustomSpinEdit::GetIncrementImpl(TObject* owner) { return internal::TCustomSpinEdit_GetIncrement(owner->Handle()); }
void TCustomSpinEdit::SetIncrementImpl(TObject* owner, const int& value) { internal::TCustomSpinEdit_SetIncrement(owner->Handle(), value); }

TSpinEdit::TSpinEdit(TComponent* AOwner)
    : TCustomSpinEdit(internal::TSpinEdit_Create(HandleOf(AOwner)))
{}

TMaskEdit::TMaskEdit(TComponent* AOwner)
    : TCustomEdit(internal::TMaskEdit_Create(HandleOf(AOwner)))
    , EditMask(this, &TMaskEdit::GetEditMaskImpl, &TMaskEdit::SetEditMaskImpl)
{}

std::string TMaskEdit::GetEditMaskImpl(TObject* owner) { return std::string(internal::TMaskEdit_GetEditMask(owner->Handle())); }
void TMaskEdit::SetEditMaskImpl(TObject* owner, const std::string& value) { internal::TMaskEdit_SetEditMask(owner->Handle(), value.c_str()); }

TCustomLabeledEdit::TCustomLabeledEdit(ObjectHandle handle)
    : TCustomEdit(handle)
    , EditLabel(this, &TCustomLabeledEdit::GetEditLabelImpl)
    , LabelPosition(this, &TCustomLabeledEdit::GetLabelPositionImpl, &TCustomLabeledEdit::SetLabelPositionImpl)
    , LabelSpacing(this, &TCustomLabeledEdit::GetLabelSpacingImpl, &TCustomLabeledEdit::SetLabelSpacingImpl)
{}

TBoundLabel* TCustomLabeledEdit::GetEditLabelImpl(TObject* owner)
{
    return WrapExisting<TBoundLabel>(internal::TCustomLabeledEdit_GetEditLabel(owner->Handle()));
}
TLabelPosition TCustomLabeledEdit::GetLabelPositionImpl(TObject* owner)
{
    return static_cast<TLabelPosition>(internal::TCustomLabeledEdit_GetLabelPosition(owner->Handle()));
}
void TCustomLabeledEdit::SetLabelPositionImpl(TObject* owner, const TLabelPosition& value)
{
    internal::TCustomLabeledEdit_SetLabelPosition(owner->Handle(), value);
}
int  TCustomLabeledEdit::GetLabelSpacingImpl(TObject* owner) { return internal::TCustomLabeledEdit_GetLabelSpacing(owner->Handle()); }
void TCustomLabeledEdit::SetLabelSpacingImpl(TObject* owner, const int& value) { internal::TCustomLabeledEdit_SetLabelSpacing(owner->Handle(), value); }

TLabeledEdit::TLabeledEdit(TComponent* AOwner)
    : TCustomLabeledEdit(internal::TLabeledEdit_Create(HandleOf(AOwner)))
{}

TTabControl::TTabControl(TComponent* AOwner)
    : TCustomTabControl(internal::TTabControl_Create(HandleOf(AOwner)))
    , TabIndex(this, &TTabControl::GetTabIndexImpl, &TTabControl::SetTabIndexImpl)
    , OnChange(this, &TTabControl::GetOnChangeImpl, &TTabControl::SetOnChangeImpl)
    , Tabs(this, &TTabControl::GetTabsImpl)
    , tabs_(this, &internal::TTabControl_GetTabs)
{}

TStrings* TTabControl::GetTabsImpl(TObject* owner) { return &static_cast<TTabControl*>(owner)->tabs_; }

int  TTabControl::GetTabIndexImpl(TObject* owner)                   { return internal::TTabControl_GetTabIndex(owner->Handle()); }
void TTabControl::SetTabIndexImpl(TObject* owner, const int& value) { internal::TTabControl_SetTabIndex(owner->Handle(), value); }
TNotifyEvent TTabControl::GetOnChangeImpl(TObject* owner) { return static_cast<TTabControl*>(owner)->onChange_; }

void TTabControl::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TTabControl* self = static_cast<TTabControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &internal::TTabControl_SetOnChange, &TTabControl::ChangeTrampoline);
}

void BETH_CALL TTabControl::ChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TTabControl* self = static_cast<TTabControl*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TCustomTabControl::TCustomTabControl(ObjectHandle handle)
    : TWinControl(handle)
    , PageCount(this, &TCustomTabControl::GetPageCountImpl)
    , MultiLine(this, &TCustomTabControl::GetMultiLineImpl, &TCustomTabControl::SetMultiLineImpl)
    , ShowTabs(this, &TCustomTabControl::GetShowTabsImpl, &TCustomTabControl::SetShowTabsImpl)
    , TabPosition(this, &TCustomTabControl::GetTabPositionImpl, &TCustomTabControl::SetTabPositionImpl)
    , OnChanging(this, &TCustomTabControl::GetOnChangingImpl, &TCustomTabControl::SetOnChangingImpl)
    , Images(this, &TCustomTabControl::GetImagesImpl, &TCustomTabControl::SetImagesImpl)
{}

int  TCustomTabControl::GetPageCountImpl(TObject* owner)                 { return internal::TCustomTabControl_GetPageCount(owner->Handle()); }
bool TCustomTabControl::GetMultiLineImpl(TObject* owner)                 { return internal::TCustomTabControl_GetMultiLine(owner->Handle()) != 0; }
void TCustomTabControl::SetMultiLineImpl(TObject* owner, const bool& value) { internal::TCustomTabControl_SetMultiLine(owner->Handle(), value ? 1 : 0); }
bool TCustomTabControl::GetShowTabsImpl(TObject* owner)                  { return internal::TCustomTabControl_GetShowTabs(owner->Handle()) != 0; }
void TCustomTabControl::SetShowTabsImpl(TObject* owner, const bool& value)  { internal::TCustomTabControl_SetShowTabs(owner->Handle(), value ? 1 : 0); }
TTabPosition TCustomTabControl::GetTabPositionImpl(TObject* owner) { return static_cast<TTabPosition>(internal::TCustomTabControl_GetTabPosition(owner->Handle())); }
void TCustomTabControl::SetTabPositionImpl(TObject* owner, const TTabPosition& value) { internal::TCustomTabControl_SetTabPosition(owner->Handle(), value); }

TTabChangingEvent TCustomTabControl::GetOnChangingImpl(TObject* owner) { return static_cast<TCustomTabControl*>(owner)->onChanging_; }

void TCustomTabControl::SetOnChangingImpl(TObject* owner, const TTabChangingEvent& value)
{
    TCustomTabControl* self = static_cast<TCustomTabControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChanging_, self->onChangingHooked_, value,
                   &internal::TCustomTabControl_SetOnChanging, &TCustomTabControl::ChangingTrampoline);
}

void BETH_CALL TCustomTabControl::ChangingTrampoline(ObjectHandle sender, internal::bool_t* allowChange, void*)
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
    : TCustomTabControl(internal::TPageControl_Create(HandleOf(AOwner)))
    , ActivePage(this, &TPageControl::GetActivePageImpl, &TPageControl::SetActivePageImpl)
    , ActivePageIndex(this, &TPageControl::GetActivePageIndexImpl, &TPageControl::SetActivePageIndexImpl)
    , TabIndex(this, &TPageControl::GetTabIndexImpl, &TPageControl::SetTabIndexImpl)
    , OnChange(this, &TPageControl::GetOnChangeImpl, &TPageControl::SetOnChangeImpl)
    , Pages(this, &TPageControl::GetPagesImpl)
{}

TTabSheet* TPageControl::GetPagesImpl(TObject* owner, int Index) { return WrapExisting<TTabSheet>(internal::TPageControl_GetPage(owner->Handle(), Index)); }
TTabSheet* TPageControl::AddTabSheet()            { return WrapExisting<TTabSheet>(internal::TPageControl_AddTabSheet(handle_)); }
void TPageControl::Clear()                        { internal::TPageControl_Clear(handle_); }
void TPageControl::SelectNextPage(bool GoForward) { internal::TPageControl_SelectNextPage(handle_, GoForward ? 1 : 0); }

TTabSheet* TPageControl::GetActivePageImpl(TObject* owner) { return WrapExisting<TTabSheet>(internal::TPageControl_GetActivePage(owner->Handle())); }
void TPageControl::SetActivePageImpl(TObject* owner, TTabSheet* const& value) { internal::TPageControl_SetActivePage(owner->Handle(), HandleOf(value)); }
int  TPageControl::GetActivePageIndexImpl(TObject* owner)                  { return internal::TPageControl_GetActivePageIndex(owner->Handle()); }
void TPageControl::SetActivePageIndexImpl(TObject* owner, const int& value) { internal::TPageControl_SetActivePageIndex(owner->Handle(), value); }
int  TPageControl::GetTabIndexImpl(TObject* owner)                         { return internal::TPageControl_GetTabIndex(owner->Handle()); }
void TPageControl::SetTabIndexImpl(TObject* owner, const int& value)        { internal::TPageControl_SetTabIndex(owner->Handle(), value); }

TNotifyEvent TPageControl::GetOnChangeImpl(TObject* owner) { return static_cast<TPageControl*>(owner)->onChange_; }

void TPageControl::SetOnChangeImpl(TObject* owner, const TNotifyEvent& value)
{
    TPageControl* self = static_cast<TPageControl*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &internal::TPageControl_SetOnChange, &TPageControl::ChangeTrampoline);
}

void BETH_CALL TPageControl::ChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TPageControl* self = static_cast<TPageControl*>(FromHandle(sender)))
            CallNotify(self->onChange_, self);
    });
}

TCustomPage::TCustomPage(ObjectHandle handle)
    : TWinControl(handle)
    , PageIndex(this, &TCustomPage::GetPageIndexImpl, &TCustomPage::SetPageIndexImpl)
    , TabVisible(this, &TCustomPage::GetTabVisibleImpl, &TCustomPage::SetTabVisibleImpl)
    , OnShow(this, &TCustomPage::GetOnShowImpl, &TCustomPage::SetOnShowImpl)
    , OnHide(this, &TCustomPage::GetOnHideImpl, &TCustomPage::SetOnHideImpl)
    , ImageIndex(this, &TCustomPage::GetImageIndexImpl, &TCustomPage::SetImageIndexImpl)
{}

int  TCustomPage::GetPageIndexImpl(TObject* owner)                   { return internal::TCustomPage_GetPageIndex(owner->Handle()); }
void TCustomPage::SetPageIndexImpl(TObject* owner, const int& value)  { internal::TCustomPage_SetPageIndex(owner->Handle(), value); }
bool TCustomPage::GetTabVisibleImpl(TObject* owner)                  { return internal::TCustomPage_GetTabVisible(owner->Handle()) != 0; }
void TCustomPage::SetTabVisibleImpl(TObject* owner, const bool& value) { internal::TCustomPage_SetTabVisible(owner->Handle(), value ? 1 : 0); }

TNotifyEvent TCustomPage::GetOnShowImpl(TObject* owner) { return static_cast<TCustomPage*>(owner)->onShow_; }
TNotifyEvent TCustomPage::GetOnHideImpl(TObject* owner) { return static_cast<TCustomPage*>(owner)->onHide_; }

void TCustomPage::SetOnShowImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomPage* self = static_cast<TCustomPage*>(owner);
    SetSimpleEvent(self->handle_, self->onShow_, self->onShowHooked_, value,
                   &internal::TCustomPage_SetOnShow, &TCustomPage::ShowTrampoline);
}

void TCustomPage::SetOnHideImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomPage* self = static_cast<TCustomPage*>(owner);
    SetSimpleEvent(self->handle_, self->onHide_, self->onHideHooked_, value,
                   &internal::TCustomPage_SetOnHide, &TCustomPage::HideTrampoline);
}

void BETH_CALL TCustomPage::ShowTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomPage* self = static_cast<TCustomPage*>(FromHandle(sender)))
            CallNotify(self->onShow_, self);
    });
}

void BETH_CALL TCustomPage::HideTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomPage* self = static_cast<TCustomPage*>(FromHandle(sender)))
            CallNotify(self->onHide_, self);
    });
}

TTabSheet::TTabSheet(TComponent* AOwner)
    : TTabSheet(internal::TTabSheet_Create(HandleOf(AOwner)))
{}

TTabSheet::TTabSheet(ObjectHandle handle)
    : TCustomPage(handle)
    , PageControl(this, &TTabSheet::GetPageControlImpl, &TTabSheet::SetPageControlImpl)
    , TabIndex(this, &TTabSheet::GetTabIndexImpl)
{}

TPageControl* TTabSheet::GetPageControlImpl(TObject* owner)
{
    return static_cast<TPageControl*>(FromHandle(internal::TTabSheet_GetPageControl(owner->Handle())));
}

void TTabSheet::SetPageControlImpl(TObject* owner, TPageControl* const& value)
{
    internal::TTabSheet_SetPageControl(owner->Handle(), HandleOf(value));
}

int TTabSheet::GetTabIndexImpl(TObject* owner) { return internal::TTabSheet_GetTabIndex(owner->Handle()); }

/* ---------------- ItemRegistry ---------------- */

// 項目の破棄は Pascal 側で、ハンドルを渡すときに付けた観察者(TPersistent.Destroy の ooFree)から通知される。
// 意図的に破棄しない(new したまま)。関数内 static の値にすると、初回の呼び出し(フォームの生成中)より前に
// 登録された TApplication::Shutdown(atexit)よりも先に破棄されてしまい、Shutdown でフォームを破棄する際の
// 項目の削除通知(FreeTrampoline)が、破棄済みのレジストリに触れることになる(未定義動作。実際に終了が数秒遅れた。ADR 0019)。
std::unordered_map<ObjectHandle, TPersistent*>& ItemRegistry::Registry()
{
    static std::unordered_map<ObjectHandle, TPersistent*>* registry = new std::unordered_map<ObjectHandle, TPersistent*>();
    return *registry;
}

void ItemRegistry::InstallCallback()
{
    static bool installed = false;
    if (!installed)
    {
        internal::ItemFree_SetCallback(&ItemRegistry::FreeTrampoline, nullptr);
        installed = true;
    }
}

void BETH_CALL ItemRegistry::FreeTrampoline(ObjectHandle handle, void*)
{
    GuardCallback([&] {
        std::unordered_map<ObjectHandle, TPersistent*>& registry = Registry();
        auto it = registry.find(handle);
        if (it == registry.end())
            return;
        TPersistent* item = it->second;
        registry.erase(it);
        delete item;
    });
}

/* ---------------- TreeView ---------------- */

TTreeNode::TTreeNode(ObjectHandle handle)
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

TTreeNode* TTreeNode::GetItemsImpl(TObject* owner, int Index) { return Wrap(internal::TTreeNode_GetItem(owner->Handle(), Index)); }
TTreeNode* TTreeNode::GetFirstChild() const     { return Wrap(internal::TTreeNode_GetFirstChild(handle_)); }
TTreeNode* TTreeNode::GetLastChild() const      { return Wrap(internal::TTreeNode_GetLastChild(handle_)); }
TTreeNode* TTreeNode::GetNextSibling() const    { return Wrap(internal::TTreeNode_GetNextSibling(handle_)); }
TTreeNode* TTreeNode::GetPrevSibling() const    { return Wrap(internal::TTreeNode_GetPrevSibling(handle_)); }
TTreeNode* TTreeNode::GetNext() const           { return Wrap(internal::TTreeNode_GetNext(handle_)); }
TTreeNode* TTreeNode::GetPrev() const           { return Wrap(internal::TTreeNode_GetPrev(handle_)); }
int  TTreeNode::IndexOf(TTreeNode* Node) const  { return internal::TTreeNode_IndexOf(handle_, Node ? Node->Handle() : nullptr); }
void TTreeNode::Expand(bool Recurse)            { internal::TTreeNode_Expand(handle_, Recurse ? 1 : 0); }
void TTreeNode::Collapse(bool Recurse)          { internal::TTreeNode_Collapse(handle_, Recurse ? 1 : 0); }
void TTreeNode::Delete()                        { internal::TTreeNode_Delete(handle_); }
void TTreeNode::DeleteChildren()                { internal::TTreeNode_DeleteChildren(handle_); }
void TTreeNode::MakeVisible()                   { internal::TTreeNode_MakeVisible(handle_); }

void TTreeNode::MoveTo(TTreeNode* Destination, TNodeAttachMode Mode)
{
    internal::TTreeNode_MoveTo(handle_, Destination ? Destination->Handle() : nullptr, Mode);
}

std::string TTreeNode::GetTextImpl(TObject* owner) { return std::string(internal::TTreeNode_GetText(owner->Handle())); }
void TTreeNode::SetTextImpl(TObject* owner, const std::string& value) { internal::TTreeNode_SetText(owner->Handle(), value.c_str()); }
bool TTreeNode::GetExpandedImpl(TObject* owner)                      { return internal::TTreeNode_GetExpanded(owner->Handle()) != 0; }
void TTreeNode::SetExpandedImpl(TObject* owner, const bool& value)    { internal::TTreeNode_SetExpanded(owner->Handle(), value ? 1 : 0); }
bool TTreeNode::GetSelectedImpl(TObject* owner)                      { return internal::TTreeNode_GetSelected(owner->Handle()) != 0; }
void TTreeNode::SetSelectedImpl(TObject* owner, const bool& value)    { internal::TTreeNode_SetSelected(owner->Handle(), value ? 1 : 0); }
bool TTreeNode::GetHasChildrenImpl(TObject* owner)                   { return internal::TTreeNode_GetHasChildren(owner->Handle()) != 0; }
void TTreeNode::SetHasChildrenImpl(TObject* owner, const bool& value) { internal::TTreeNode_SetHasChildren(owner->Handle(), value ? 1 : 0); }
void* TTreeNode::GetDataImpl(TObject* owner)                         { return internal::TTreeNode_GetData(owner->Handle()); }
void TTreeNode::SetDataImpl(TObject* owner, void* const& value)       { internal::TTreeNode_SetData(owner->Handle(), value); }
int  TTreeNode::GetCountImpl(TObject* owner)                         { return internal::TTreeNode_GetCount(owner->Handle()); }
int  TTreeNode::GetIndexImpl(TObject* owner)                         { return internal::TTreeNode_GetIndex(owner->Handle()); }
int  TTreeNode::GetLevelImpl(TObject* owner)                         { return internal::TTreeNode_GetLevel(owner->Handle()); }
int  TTreeNode::GetAbsoluteIndexImpl(TObject* owner)                 { return internal::TTreeNode_GetAbsoluteIndex(owner->Handle()); }
TTreeNode* TTreeNode::GetParentImpl(TObject* owner)                  { return Wrap(internal::TTreeNode_GetParent(owner->Handle())); }

TCustomTreeView* TTreeNode::GetTreeViewImpl(TObject* owner)
{
    // ツリービューは TComponent で、C++ ラッパーを介して作られていればレジストリにある。
    return static_cast<TCustomTreeView*>(TCustomTreeView::FromHandle(internal::TTreeNode_GetTreeView(owner->Handle())));
}

TTreeNodes::TTreeNodes(ObjectHandle handle)
    : TPersistent(handle)
    , Count(this, &TTreeNodes::GetCountImpl)
    , Item(this, &TTreeNodes::GetItemImpl)
{}

namespace
{
ObjectHandle NodeHandle(const TTreeNode* node) { return node ? node->Handle() : nullptr; }
}

TTreeNode* TTreeNodes::Add(TTreeNode* Sibling, const std::string& S)
{
    return TTreeNode::Wrap(internal::TTreeNodes_Add(handle_, NodeHandle(Sibling), S.c_str()));
}

TTreeNode* TTreeNodes::AddFirst(TTreeNode* Sibling, const std::string& S)
{
    return TTreeNode::Wrap(internal::TTreeNodes_AddFirst(handle_, NodeHandle(Sibling), S.c_str()));
}

TTreeNode* TTreeNodes::AddChild(TTreeNode* Parent, const std::string& S)
{
    return TTreeNode::Wrap(internal::TTreeNodes_AddChild(handle_, NodeHandle(Parent), S.c_str()));
}

TTreeNode* TTreeNodes::AddChildFirst(TTreeNode* Parent, const std::string& S)
{
    return TTreeNode::Wrap(internal::TTreeNodes_AddChildFirst(handle_, NodeHandle(Parent), S.c_str()));
}

TTreeNode* TTreeNodes::Insert(TTreeNode* NextNode, const std::string& S)
{
    return TTreeNode::Wrap(internal::TTreeNodes_Insert(handle_, NodeHandle(NextNode), S.c_str()));
}

void TTreeNodes::Clear()                   { internal::TTreeNodes_Clear(handle_); }
void TTreeNodes::Delete(TTreeNode* Node)   { internal::TTreeNodes_Delete(handle_, NodeHandle(Node)); }
TTreeNode* TTreeNodes::GetItemImpl(TObject* owner, int Index) { return TTreeNode::Wrap(internal::TTreeNodes_GetItem(owner->Handle(), Index)); }
TTreeNode* TTreeNodes::GetFirstNode() const     { return TTreeNode::Wrap(internal::TTreeNodes_GetFirstNode(handle_)); }

TTreeNode* TTreeNodes::FindNodeWithText(const std::string& S) const
{
    return TTreeNode::Wrap(internal::TTreeNodes_FindNodeWithText(handle_, S.c_str()));
}

void TTreeNodes::BeginUpdate()             { internal::TTreeNodes_BeginUpdate(handle_); }
void TTreeNodes::EndUpdate()               { internal::TTreeNodes_EndUpdate(handle_); }
int  TTreeNodes::GetCountImpl(TObject* owner) { return internal::TTreeNodes_GetCount(owner->Handle()); }

TCustomTreeView::TCustomTreeView(ObjectHandle handle)
    : TCustomControl(handle)
    , Items(this, &TCustomTreeView::GetItemsImpl)
    , Selected(this, &TCustomTreeView::GetSelectedImpl, &TCustomTreeView::SetSelectedImpl)
    , Images(this, &TCustomTreeView::GetImagesImpl, &TCustomTreeView::SetImagesImpl)
    , StateImages(this, &TCustomTreeView::GetStateImagesImpl, &TCustomTreeView::SetStateImagesImpl)
    , items_(internal::TCustomTreeView_GetItems(handle))
{}

void TCustomTreeView::FullExpand()   { internal::TCustomTreeView_FullExpand(handle_); }
void TCustomTreeView::FullCollapse() { internal::TCustomTreeView_FullCollapse(handle_); }
bool TCustomTreeView::AlphaSort()    { return internal::TCustomTreeView_AlphaSort(handle_) != 0; }

TTreeNode* TCustomTreeView::GetNodeAt(int X, int Y) const
{
    return TTreeNode::Wrap(internal::TCustomTreeView_GetNodeAt(handle_, X, Y));
}

TTreeNodes* TCustomTreeView::GetItemsImpl(TObject* owner) { return &static_cast<TCustomTreeView*>(owner)->items_; }
TTreeNode*  TCustomTreeView::GetSelectedImpl(TObject* owner) { return TTreeNode::Wrap(internal::TCustomTreeView_GetSelected(owner->Handle())); }

void TCustomTreeView::SetSelectedImpl(TObject* owner, TTreeNode* const& value)
{
    internal::TCustomTreeView_SetSelected(owner->Handle(), NodeHandle(value));
}

TTreeView::TTreeView(TComponent* AOwner)
    : TCustomTreeView(internal::TTreeView_Create(HandleOf(AOwner)))
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

bool TTreeView::GetReadOnlyImpl(TObject* owner)                     { return internal::TTreeView_GetReadOnly(owner->Handle()) != 0; }
void TTreeView::SetReadOnlyImpl(TObject* owner, const bool& value)   { internal::TTreeView_SetReadOnly(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowLinesImpl(TObject* owner)                    { return internal::TTreeView_GetShowLines(owner->Handle()) != 0; }
void TTreeView::SetShowLinesImpl(TObject* owner, const bool& value)  { internal::TTreeView_SetShowLines(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowRootImpl(TObject* owner)                     { return internal::TTreeView_GetShowRoot(owner->Handle()) != 0; }
void TTreeView::SetShowRootImpl(TObject* owner, const bool& value)   { internal::TTreeView_SetShowRoot(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetShowButtonsImpl(TObject* owner)                  { return internal::TTreeView_GetShowButtons(owner->Handle()) != 0; }
void TTreeView::SetShowButtonsImpl(TObject* owner, const bool& value) { internal::TTreeView_SetShowButtons(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetAutoExpandImpl(TObject* owner)                   { return internal::TTreeView_GetAutoExpand(owner->Handle()) != 0; }
void TTreeView::SetAutoExpandImpl(TObject* owner, const bool& value) { internal::TTreeView_SetAutoExpand(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetHideSelectionImpl(TObject* owner)                { return internal::TTreeView_GetHideSelection(owner->Handle()) != 0; }
void TTreeView::SetHideSelectionImpl(TObject* owner, const bool& value) { internal::TTreeView_SetHideSelection(owner->Handle(), value ? 1 : 0); }
bool TTreeView::GetRowSelectImpl(TObject* owner)                    { return internal::TTreeView_GetRowSelect(owner->Handle()) != 0; }
void TTreeView::SetRowSelectImpl(TObject* owner, const bool& value)  { internal::TTreeView_SetRowSelect(owner->Handle(), value ? 1 : 0); }

// ノードを対象とするイベントのトランポリンは、スロット(メンバへのポインタ)だけが異なるため共通化する。
template<typename Event>
void TTreeView::DispatchNode(ObjectHandle sender, ObjectHandle node, Event TTreeView::*slot)
{
    TTreeView* self = static_cast<TTreeView*>(FromHandle(sender));
    if (!self || !(self->*slot))
        return;
    Event handler = self->*slot;
    handler(self, TTreeNode::Wrap(node));
}

template<typename Event>
void TTreeView::DispatchNodeAllow(ObjectHandle sender, ObjectHandle node, internal::bool_t* allow, Event TTreeView::*slot)
{
    TTreeView* self = static_cast<TTreeView*>(FromHandle(sender));
    if (!self || !(self->*slot))
        return;
    Event handler = self->*slot;
    bool value = *allow != 0;
    handler(self, TTreeNode::Wrap(node), value);
    *allow = value ? 1 : 0;
}

void BETH_CALL TTreeView::ChangeTrampoline(ObjectHandle s, ObjectHandle n, void*)    { GuardCallback([&] { DispatchNode(s, n, &TTreeView::onChange_); }); }
void BETH_CALL TTreeView::ExpandedTrampoline(ObjectHandle s, ObjectHandle n, void*)  { GuardCallback([&] { DispatchNode(s, n, &TTreeView::onExpanded_); }); }
void BETH_CALL TTreeView::CollapsedTrampoline(ObjectHandle s, ObjectHandle n, void*) { GuardCallback([&] { DispatchNode(s, n, &TTreeView::onCollapsed_); }); }
void BETH_CALL TTreeView::DeletionTrampoline(ObjectHandle s, ObjectHandle n, void*)  { GuardCallback([&] { DispatchNode(s, n, &TTreeView::onDeletion_); }); }

void BETH_CALL TTreeView::ChangingTrampoline(ObjectHandle s, ObjectHandle n, internal::bool_t* a, void*)
{
    GuardCallback([&] {
        DispatchNodeAllow(s, n, a, &TTreeView::onChanging_);
    });
}

void BETH_CALL TTreeView::ExpandingTrampoline(ObjectHandle s, ObjectHandle n, internal::bool_t* a, void*)
{
    GuardCallback([&] {
        DispatchNodeAllow(s, n, a, &TTreeView::onExpanding_);
    });
}

void BETH_CALL TTreeView::CollapsingTrampoline(ObjectHandle s, ObjectHandle n, internal::bool_t* a, void*)
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
                   &internal::TTreeView_SetOnChange, &TTreeView::ChangeTrampoline);
}

void TTreeView::SetOnChangingImpl(TObject* owner, const TTVChangingEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onChanging_, self->onChangingHooked_, value,
                   &internal::TTreeView_SetOnChanging, &TTreeView::ChangingTrampoline);
}

void TTreeView::SetOnExpandingImpl(TObject* owner, const TTVExpandingEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onExpanding_, self->onExpandingHooked_, value,
                   &internal::TTreeView_SetOnExpanding, &TTreeView::ExpandingTrampoline);
}

void TTreeView::SetOnExpandedImpl(TObject* owner, const TTVExpandedEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onExpanded_, self->onExpandedHooked_, value,
                   &internal::TTreeView_SetOnExpanded, &TTreeView::ExpandedTrampoline);
}

void TTreeView::SetOnCollapsingImpl(TObject* owner, const TTVCollapsingEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onCollapsing_, self->onCollapsingHooked_, value,
                   &internal::TTreeView_SetOnCollapsing, &TTreeView::CollapsingTrampoline);
}

void TTreeView::SetOnCollapsedImpl(TObject* owner, const TTVExpandedEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onCollapsed_, self->onCollapsedHooked_, value,
                   &internal::TTreeView_SetOnCollapsed, &TTreeView::CollapsedTrampoline);
}

void TTreeView::SetOnDeletionImpl(TObject* owner, const TTVExpandedEvent& value)
{
    TTreeView* self = static_cast<TTreeView*>(owner);
    SetSimpleEvent(self->handle_, self->onDeletion_, self->onDeletionHooked_, value,
                   &internal::TTreeView_SetOnDeletion, &TTreeView::DeletionTrampoline);
}

/* ---------------- ListView ---------------- */

namespace
{
ObjectHandle ItemHandle(const TObject* item) { return item ? item->Handle() : nullptr; }
}

TListItem::TListItem(ObjectHandle handle)
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
    , subItems_(this, &internal::TListItem_GetSubItems)
{}

TStrings* TListItem::GetSubItemsImpl(TObject* owner) { return &static_cast<TListItem*>(owner)->subItems_; }
void TListItem::Delete()                               { internal::TListItem_Delete(handle_); }
void TListItem::MakeVisible(bool PartialOK)            { internal::TListItem_MakeVisible(handle_, PartialOK ? 1 : 0); }

std::string TListItem::GetCaptionImpl(TObject* owner) { return std::string(internal::TListItem_GetCaption(owner->Handle())); }
void TListItem::SetCaptionImpl(TObject* owner, const std::string& value) { internal::TListItem_SetCaption(owner->Handle(), value.c_str()); }
bool TListItem::GetCheckedImpl(TObject* owner)                     { return internal::TListItem_GetChecked(owner->Handle()) != 0; }
void TListItem::SetCheckedImpl(TObject* owner, const bool& value)   { internal::TListItem_SetChecked(owner->Handle(), value ? 1 : 0); }
bool TListItem::GetSelectedImpl(TObject* owner)                    { return internal::TListItem_GetSelected(owner->Handle()) != 0; }
void TListItem::SetSelectedImpl(TObject* owner, const bool& value)  { internal::TListItem_SetSelected(owner->Handle(), value ? 1 : 0); }
bool TListItem::GetFocusedImpl(TObject* owner)                     { return internal::TListItem_GetFocused(owner->Handle()) != 0; }
void TListItem::SetFocusedImpl(TObject* owner, const bool& value)   { internal::TListItem_SetFocused(owner->Handle(), value ? 1 : 0); }
void* TListItem::GetDataImpl(TObject* owner)                       { return internal::TListItem_GetData(owner->Handle()); }
void TListItem::SetDataImpl(TObject* owner, void* const& value)     { internal::TListItem_SetData(owner->Handle(), value); }
int  TListItem::GetIndexImpl(TObject* owner)                       { return internal::TListItem_GetIndex(owner->Handle()); }

TCustomListView* TListItem::GetListViewImpl(TObject* owner)
{
    return static_cast<TCustomListView*>(TCustomListView::FromHandle(internal::TListItem_GetListView(owner->Handle())));
}

TListItems::TListItems(ObjectHandle handle)
    : TPersistent(handle)
    , Count(this, &TListItems::GetCountImpl)
    , Item(this, &TListItems::GetItemImpl)
{}

TListItem* TListItems::Add()                     { return TListItem::Wrap(internal::TListItems_Add(handle_)); }
TListItem* TListItems::Insert(int Index)         { return TListItem::Wrap(internal::TListItems_Insert(handle_, Index)); }
void TListItems::Delete(int Index)               { internal::TListItems_Delete(handle_, Index); }
void TListItems::Clear()                         { internal::TListItems_Clear(handle_); }
TListItem* TListItems::GetItemImpl(TObject* owner, int Index) { return TListItem::Wrap(internal::TListItems_GetItem(owner->Handle(), Index)); }
int  TListItems::IndexOf(TListItem* Item) const  { return internal::TListItems_IndexOf(handle_, ItemHandle(Item)); }

TListItem* TListItems::FindCaption(int StartIndex, const std::string& Value, bool Partial, bool Inclusive, bool Wrap) const
{
    return TListItem::Wrap(internal::TListItems_FindCaption(handle_, StartIndex, Value.c_str(),
                                                        Partial ? 1 : 0, Inclusive ? 1 : 0, Wrap ? 1 : 0));
}

void TListItems::Exchange(int Index1, int Index2) { internal::TListItems_Exchange(handle_, Index1, Index2); }
void TListItems::Move(int FromIndex, int ToIndex) { internal::TListItems_Move(handle_, FromIndex, ToIndex); }
void TListItems::BeginUpdate()                    { internal::TListItems_BeginUpdate(handle_); }
void TListItems::EndUpdate()                      { internal::TListItems_EndUpdate(handle_); }
int  TListItems::GetCountImpl(TObject* owner)     { return internal::TListItems_GetCount(owner->Handle()); }

TListColumn::TListColumn(ObjectHandle handle)
    : TPersistent(handle)
    , Caption(this, &TListColumn::GetCaptionImpl, &TListColumn::SetCaptionImpl)
    , Width(this, &TListColumn::GetWidthImpl, &TListColumn::SetWidthImpl)
    , Alignment(this, &TListColumn::GetAlignmentImpl, &TListColumn::SetAlignmentImpl)
    , AutoSize(this, &TListColumn::GetAutoSizeImpl, &TListColumn::SetAutoSizeImpl)
    , Visible(this, &TListColumn::GetVisibleImpl, &TListColumn::SetVisibleImpl)
    , Index(this, &TListColumn::GetIndexImpl, &TListColumn::SetIndexImpl)
    , ImageIndex(this, &TListColumn::GetImageIndexImpl, &TListColumn::SetImageIndexImpl)
{}

std::string TListColumn::GetCaptionImpl(TObject* owner) { return std::string(internal::TListColumn_GetCaption(owner->Handle())); }
void TListColumn::SetCaptionImpl(TObject* owner, const std::string& value) { internal::TListColumn_SetCaption(owner->Handle(), value.c_str()); }
int  TListColumn::GetWidthImpl(TObject* owner)                    { return internal::TListColumn_GetWidth(owner->Handle()); }
void TListColumn::SetWidthImpl(TObject* owner, const int& value)   { internal::TListColumn_SetWidth(owner->Handle(), value); }
TAlignment TListColumn::GetAlignmentImpl(TObject* owner) { return static_cast<TAlignment>(internal::TListColumn_GetAlignment(owner->Handle())); }
void TListColumn::SetAlignmentImpl(TObject* owner, const TAlignment& value) { internal::TListColumn_SetAlignment(owner->Handle(), value); }
bool TListColumn::GetAutoSizeImpl(TObject* owner)                 { return internal::TListColumn_GetAutoSize(owner->Handle()) != 0; }
void TListColumn::SetAutoSizeImpl(TObject* owner, const bool& value) { internal::TListColumn_SetAutoSize(owner->Handle(), value ? 1 : 0); }
bool TListColumn::GetVisibleImpl(TObject* owner)                  { return internal::TListColumn_GetVisible(owner->Handle()) != 0; }
void TListColumn::SetVisibleImpl(TObject* owner, const bool& value) { internal::TListColumn_SetVisible(owner->Handle(), value ? 1 : 0); }
int  TListColumn::GetIndexImpl(TObject* owner)                    { return internal::TListColumn_GetIndex(owner->Handle()); }
void TListColumn::SetIndexImpl(TObject* owner, const int& value)   { internal::TListColumn_SetIndex(owner->Handle(), value); }

TListColumns::TListColumns(ObjectHandle handle)
    : TPersistent(handle)
    , Count(this, &TListColumns::GetCountImpl)
    , Items(this, &TListColumns::GetItemsImpl)
{}

TListColumn* TListColumns::Add()                    { return TListColumn::Wrap(internal::TListColumns_Add(handle_)); }
TListColumn* TListColumns::GetItemsImpl(TObject* owner, int Index) { return TListColumn::Wrap(internal::TListColumns_GetItem(owner->Handle(), Index)); }
void TListColumns::Delete(int Index)                { internal::TListColumns_Delete(handle_, Index); }
void TListColumns::Clear()                          { internal::TListColumns_Clear(handle_); }
int  TListColumns::GetCountImpl(TObject* owner)     { return internal::TListColumns_GetCount(owner->Handle()); }

TCustomListView::TCustomListView(ObjectHandle handle)
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
    , items_(internal::TCustomListView_GetItems(handle))
{}

void TCustomListView::Clear()          { internal::TCustomListView_Clear(handle_); }
void TCustomListView::BeginUpdate()    { internal::TCustomListView_BeginUpdate(handle_); }
void TCustomListView::EndUpdate()      { internal::TCustomListView_EndUpdate(handle_); }
void TCustomListView::ClearSelection() { internal::TCustomListView_ClearSelection(handle_); }
void TCustomListView::SelectAll()      { internal::TCustomListView_SelectAll(handle_); }

TListItem* TCustomListView::GetItemAt(int X, int Y) const
{
    return TListItem::Wrap(internal::TCustomListView_GetItemAt(handle_, X, Y));
}

TListItems* TCustomListView::GetItemsImpl(TObject* owner)   { return &static_cast<TCustomListView*>(owner)->items_; }
TListItem*  TCustomListView::GetSelectedImpl(TObject* owner) { return TListItem::Wrap(internal::TCustomListView_GetSelected(owner->Handle())); }
void TCustomListView::SetSelectedImpl(TObject* owner, TListItem* const& value) { internal::TCustomListView_SetSelected(owner->Handle(), ItemHandle(value)); }
int  TCustomListView::GetItemIndexImpl(TObject* owner)                   { return internal::TCustomListView_GetItemIndex(owner->Handle()); }
void TCustomListView::SetItemIndexImpl(TObject* owner, const int& value)  { internal::TCustomListView_SetItemIndex(owner->Handle(), value); }
int  TCustomListView::GetSelCountImpl(TObject* owner)                    { return internal::TCustomListView_GetSelCount(owner->Handle()); }
bool TCustomListView::GetCheckboxesImpl(TObject* owner)                  { return internal::TCustomListView_GetCheckboxes(owner->Handle()) != 0; }
void TCustomListView::SetCheckboxesImpl(TObject* owner, const bool& value) { internal::TCustomListView_SetCheckboxes(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetGridLinesImpl(TObject* owner)                   { return internal::TCustomListView_GetGridLines(owner->Handle()) != 0; }
void TCustomListView::SetGridLinesImpl(TObject* owner, const bool& value) { internal::TCustomListView_SetGridLines(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetMultiSelectImpl(TObject* owner)                 { return internal::TCustomListView_GetMultiSelect(owner->Handle()) != 0; }
void TCustomListView::SetMultiSelectImpl(TObject* owner, const bool& value) { internal::TCustomListView_SetMultiSelect(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetReadOnlyImpl(TObject* owner)                    { return internal::TCustomListView_GetReadOnly(owner->Handle()) != 0; }
void TCustomListView::SetReadOnlyImpl(TObject* owner, const bool& value)  { internal::TCustomListView_SetReadOnly(owner->Handle(), value ? 1 : 0); }
bool TCustomListView::GetRowSelectImpl(TObject* owner)                   { return internal::TCustomListView_GetRowSelect(owner->Handle()) != 0; }
void TCustomListView::SetRowSelectImpl(TObject* owner, const bool& value) { internal::TCustomListView_SetRowSelect(owner->Handle(), value ? 1 : 0); }

TListView::TListView(TComponent* AOwner)
    : TCustomListView(internal::TListView_Create(HandleOf(AOwner)))
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
    , columns_(internal::TListView_GetColumns(handle_))
{}

TListColumns* TListView::GetColumnsImpl(TObject* owner) { return &static_cast<TListView*>(owner)->columns_; }
TViewStyle TListView::GetViewStyleImpl(TObject* owner) { return static_cast<TViewStyle>(internal::TListView_GetViewStyle(owner->Handle())); }
void TListView::SetViewStyleImpl(TObject* owner, const TViewStyle& value) { internal::TListView_SetViewStyle(owner->Handle(), value); }
bool TListView::GetHideSelectionImpl(TObject* owner)                    { return internal::TListView_GetHideSelection(owner->Handle()) != 0; }
void TListView::SetHideSelectionImpl(TObject* owner, const bool& value)  { internal::TListView_SetHideSelection(owner->Handle(), value ? 1 : 0); }
TSortType TListView::GetSortTypeImpl(TObject* owner) { return static_cast<TSortType>(internal::TListView_GetSortType(owner->Handle())); }
void TListView::SetSortTypeImpl(TObject* owner, const TSortType& value) { internal::TListView_SetSortType(owner->Handle(), value); }
int  TListView::GetSortColumnImpl(TObject* owner)                       { return internal::TListView_GetSortColumn(owner->Handle()); }
void TListView::SetSortColumnImpl(TObject* owner, const int& value)      { internal::TListView_SetSortColumn(owner->Handle(), value); }
TSortDirection TListView::GetSortDirectionImpl(TObject* owner) { return static_cast<TSortDirection>(internal::TListView_GetSortDirection(owner->Handle())); }
void TListView::SetSortDirectionImpl(TObject* owner, const TSortDirection& value) { internal::TListView_SetSortDirection(owner->Handle(), value); }

// リストビューの破棄では、リストビュー自身のラッパーが delete された後に項目が破棄される(LCL の順序)。
// そのときの OnDeletion は FromHandle が nullptr を返すため、ハンドラは呼ばれない。
void BETH_CALL TListView::SelectItemTrampoline(ObjectHandle sender, ObjectHandle item, internal::int_t selected, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onSelectItem_)
            return;
        TLVSelectItemEvent handler = self->onSelectItem_;
        handler(self, TListItem::Wrap(item), selected != 0);
    });
}

void BETH_CALL TListView::ChangeTrampoline(ObjectHandle sender, ObjectHandle item, internal::int_t change, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onChange_)
            return;
        TLVChangeEvent handler = self->onChange_;
        handler(self, TListItem::Wrap(item), static_cast<TItemChange>(change));
    });
}

void BETH_CALL TListView::DeletionTrampoline(ObjectHandle sender, ObjectHandle item, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onDeletion_)
            return;
        TLVDeletedEvent handler = self->onDeletion_;
        handler(self, TListItem::Wrap(item));
    });
}

void BETH_CALL TListView::ItemCheckedTrampoline(ObjectHandle sender, ObjectHandle item, void*)
{
    GuardCallback([&] {
        TListView* self = static_cast<TListView*>(FromHandle(sender));
        if (!self || !self->onItemChecked_)
            return;
        TLVCheckedItemEvent handler = self->onItemChecked_;
        handler(self, TListItem::Wrap(item));
    });
}

void BETH_CALL TListView::ColumnClickTrampoline(ObjectHandle sender, ObjectHandle column, void*)
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
                   &internal::TListView_SetOnSelectItem, &TListView::SelectItemTrampoline);
}

void TListView::SetOnChangeImpl(TObject* owner, const TLVChangeEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onChange_, self->onChangeHooked_, value,
                   &internal::TListView_SetOnChange, &TListView::ChangeTrampoline);
}

void TListView::SetOnDeletionImpl(TObject* owner, const TLVDeletedEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onDeletion_, self->onDeletionHooked_, value,
                   &internal::TListView_SetOnDeletion, &TListView::DeletionTrampoline);
}

void TListView::SetOnItemCheckedImpl(TObject* owner, const TLVCheckedItemEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onItemChecked_, self->onItemCheckedHooked_, value,
                   &internal::TListView_SetOnItemChecked, &TListView::ItemCheckedTrampoline);
}

void TListView::SetOnColumnClickImpl(TObject* owner, const TLVColumnClickEvent& value)
{
    TListView* self = static_cast<TListView*>(owner);
    SetSimpleEvent(self->handle_, self->onColumnClick_, self->onColumnClickHooked_, value,
                   &internal::TListView_SetOnColumnClick, &TListView::ColumnClickTrampoline);
}

TCustomSplitter::TCustomSplitter(ObjectHandle handle)
    : TCustomControl(handle)
    , AutoSnap(this, &TCustomSplitter::GetAutoSnapImpl, &TCustomSplitter::SetAutoSnapImpl)
    , Beveled(this, &TCustomSplitter::GetBeveledImpl, &TCustomSplitter::SetBeveledImpl)
    , MinSize(this, &TCustomSplitter::GetMinSizeImpl, &TCustomSplitter::SetMinSizeImpl)
    , ResizeAnchor(this, &TCustomSplitter::GetResizeAnchorImpl, &TCustomSplitter::SetResizeAnchorImpl)
    , ResizeStyle(this, &TCustomSplitter::GetResizeStyleImpl, &TCustomSplitter::SetResizeStyleImpl)
    , OnMoved(this, &TCustomSplitter::GetOnMovedImpl, &TCustomSplitter::SetOnMovedImpl)
{}

int  TCustomSplitter::GetSplitterPosition() const  { return internal::TCustomSplitter_GetSplitterPosition(handle_); }
void TCustomSplitter::SetSplitterPosition(int pos) { internal::TCustomSplitter_SetSplitterPosition(handle_, pos); }

bool TCustomSplitter::GetAutoSnapImpl(TObject* owner)                   { return internal::TCustomSplitter_GetAutoSnap(owner->Handle()) != 0; }
void TCustomSplitter::SetAutoSnapImpl(TObject* owner, const bool& value) { internal::TCustomSplitter_SetAutoSnap(owner->Handle(), value ? 1 : 0); }
bool TCustomSplitter::GetBeveledImpl(TObject* owner)                    { return internal::TCustomSplitter_GetBeveled(owner->Handle()) != 0; }
void TCustomSplitter::SetBeveledImpl(TObject* owner, const bool& value)  { internal::TCustomSplitter_SetBeveled(owner->Handle(), value ? 1 : 0); }
int  TCustomSplitter::GetMinSizeImpl(TObject* owner)                    { return internal::TCustomSplitter_GetMinSize(owner->Handle()); }
void TCustomSplitter::SetMinSizeImpl(TObject* owner, const int& value)   { internal::TCustomSplitter_SetMinSize(owner->Handle(), value); }

TAnchorKind TCustomSplitter::GetResizeAnchorImpl(TObject* owner) { return static_cast<TAnchorKind>(internal::TCustomSplitter_GetResizeAnchor(owner->Handle())); }
void TCustomSplitter::SetResizeAnchorImpl(TObject* owner, const TAnchorKind& value) { internal::TCustomSplitter_SetResizeAnchor(owner->Handle(), value); }
TResizeStyle TCustomSplitter::GetResizeStyleImpl(TObject* owner) { return static_cast<TResizeStyle>(internal::TCustomSplitter_GetResizeStyle(owner->Handle())); }
void TCustomSplitter::SetResizeStyleImpl(TObject* owner, const TResizeStyle& value) { internal::TCustomSplitter_SetResizeStyle(owner->Handle(), value); }

TNotifyEvent TCustomSplitter::GetOnMovedImpl(TObject* owner) { return static_cast<TCustomSplitter*>(owner)->onMoved_; }

void TCustomSplitter::SetOnMovedImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomSplitter* self = static_cast<TCustomSplitter*>(owner);
    SetSimpleEvent(self->handle_, self->onMoved_, self->onMovedHooked_, value,
                   &internal::TCustomSplitter_SetOnMoved, &TCustomSplitter::MovedTrampoline);
}

void BETH_CALL TCustomSplitter::MovedTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomSplitter* self = static_cast<TCustomSplitter*>(FromHandle(sender)))
            CallNotify(self->onMoved_, self);
    });
}

TSplitter::TSplitter(TComponent* AOwner)
    : TCustomSplitter(internal::TSplitter_Create(HandleOf(AOwner)))
{}

TCustomMemo::TCustomMemo(ObjectHandle handle)
    : TCustomEdit(handle)
    , WordWrap(this, &TCustomMemo::GetWordWrapImpl, &TCustomMemo::SetWordWrapImpl)
    , WantReturns(this, &TCustomMemo::GetWantReturnsImpl, &TCustomMemo::SetWantReturnsImpl)
    , WantTabs(this, &TCustomMemo::GetWantTabsImpl, &TCustomMemo::SetWantTabsImpl)
    , ScrollBars(this, &TCustomMemo::GetScrollBarsImpl, &TCustomMemo::SetScrollBarsImpl)
    , Lines(this, &TCustomMemo::GetLinesImpl)
    , lines_(this, &internal::TCustomMemo_GetLines)
{}

// ---- docs/adr/0042 ----

bool TCustomMemo::GetWordWrapImpl(TObject* owner)
{
    return internal::TCustomMemo_GetWordWrap(owner->Handle()) != 0;
}

void TCustomMemo::SetWordWrapImpl(TObject* owner, const bool& value)
{
    internal::TCustomMemo_SetWordWrap(owner->Handle(), value ? 1 : 0);
}

bool TCustomMemo::GetWantReturnsImpl(TObject* owner)
{
    return internal::TCustomMemo_GetWantReturns(owner->Handle()) != 0;
}

void TCustomMemo::SetWantReturnsImpl(TObject* owner, const bool& value)
{
    internal::TCustomMemo_SetWantReturns(owner->Handle(), value ? 1 : 0);
}

bool TCustomMemo::GetWantTabsImpl(TObject* owner)
{
    return internal::TCustomMemo_GetWantTabs(owner->Handle()) != 0;
}

void TCustomMemo::SetWantTabsImpl(TObject* owner, const bool& value)
{
    internal::TCustomMemo_SetWantTabs(owner->Handle(), value ? 1 : 0);
}

void TCustomMemo::Append(const std::string& S) { internal::TCustomMemo_Append(handle_, S.c_str()); }

TStrings* TCustomMemo::GetLinesImpl(TObject* owner) { return &static_cast<TCustomMemo*>(owner)->lines_; }

int  TCustomMemo::GetScrollBarsImpl(TObject* owner)                   { return internal::TCustomMemo_GetScrollBars(owner->Handle()); }
void TCustomMemo::SetScrollBarsImpl(TObject* owner, const int& value) { internal::TCustomMemo_SetScrollBars(owner->Handle(), value); }

TMemo::TMemo(TComponent* AOwner)
    : TCustomMemo(internal::TMemo_Create(HandleOf(AOwner)))
{}

/* ---------------- ComboBox / ListBox ---------------- */

TCustomComboBox::TCustomComboBox(ObjectHandle handle)
    : TWinControl(handle)
    , Style(this, &TCustomComboBox::GetStyleImpl, &TCustomComboBox::SetStyleImpl)
    , DropDownCount(this, &TCustomComboBox::GetDropDownCountImpl, &TCustomComboBox::SetDropDownCountImpl)
    , Sorted(this, &TCustomComboBox::GetSortedImpl, &TCustomComboBox::SetSortedImpl)
    , ReadOnly(this, &TCustomComboBox::GetReadOnlyImpl, &TCustomComboBox::SetReadOnlyImpl)
    , DroppedDown(this, &TCustomComboBox::GetDroppedDownImpl, &TCustomComboBox::SetDroppedDownImpl)
    , AutoComplete(this, &TCustomComboBox::GetAutoCompleteImpl, &TCustomComboBox::SetAutoCompleteImpl)
    , OnSelect(this, &TCustomComboBox::GetOnSelectImpl, &TCustomComboBox::SetOnSelectImpl)
    , OnDropDown(this, &TCustomComboBox::GetOnDropDownImpl, &TCustomComboBox::SetOnDropDownImpl)
    , OnCloseUp(this, &TCustomComboBox::GetOnCloseUpImpl, &TCustomComboBox::SetOnCloseUpImpl)
    , ItemIndex(this, &TCustomComboBox::GetItemIndexImpl, &TCustomComboBox::SetItemIndexImpl)
    , Items(this, &TCustomComboBox::GetItemsImpl)
    , items_(this, &internal::TCustomComboBox_GetItems)
{}

// ---- docs/adr/0043 ----

TComboBoxStyle TCustomComboBox::GetStyleImpl(TObject* owner)
{
    return static_cast<TComboBoxStyle>(internal::TCustomComboBox_GetStyle(owner->Handle()));
}

void TCustomComboBox::SetStyleImpl(TObject* owner, const TComboBoxStyle& value)
{
    internal::TCustomComboBox_SetStyle(owner->Handle(), value);
}

int TCustomComboBox::GetDropDownCountImpl(TObject* owner)
{
    return internal::TCustomComboBox_GetDropDownCount(owner->Handle());
}

void TCustomComboBox::SetDropDownCountImpl(TObject* owner, const int& value)
{
    internal::TCustomComboBox_SetDropDownCount(owner->Handle(), value);
}

bool TCustomComboBox::GetSortedImpl(TObject* owner)
{
    return internal::TCustomComboBox_GetSorted(owner->Handle()) != 0;
}

void TCustomComboBox::SetSortedImpl(TObject* owner, const bool& value)
{
    internal::TCustomComboBox_SetSorted(owner->Handle(), value ? 1 : 0);
}

bool TCustomComboBox::GetReadOnlyImpl(TObject* owner)
{
    return internal::TCustomComboBox_GetReadOnly(owner->Handle()) != 0;
}

void TCustomComboBox::SetReadOnlyImpl(TObject* owner, const bool& value)
{
    internal::TCustomComboBox_SetReadOnly(owner->Handle(), value ? 1 : 0);
}

bool TCustomComboBox::GetDroppedDownImpl(TObject* owner)
{
    return internal::TCustomComboBox_GetDroppedDown(owner->Handle()) != 0;
}

void TCustomComboBox::SetDroppedDownImpl(TObject* owner, const bool& value)
{
    internal::TCustomComboBox_SetDroppedDown(owner->Handle(), value ? 1 : 0);
}

bool TCustomComboBox::GetAutoCompleteImpl(TObject* owner)
{
    return internal::TCustomComboBox_GetAutoComplete(owner->Handle()) != 0;
}

void TCustomComboBox::SetAutoCompleteImpl(TObject* owner, const bool& value)
{
    internal::TCustomComboBox_SetAutoComplete(owner->Handle(), value ? 1 : 0);
}

TNotifyEvent TCustomComboBox::GetOnSelectImpl(TObject* owner)
{
    return static_cast<TCustomComboBox*>(owner)->onSelect_;
}
void TCustomComboBox::SetOnSelectImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomComboBox* self = static_cast<TCustomComboBox*>(owner);
    self->onSelect_ = value;
    if (value && !self->onSelectHooked_)
    {
        internal::TCustomComboBox_SetOnSelect(self->handle_, &TCustomComboBox::SelectTrampoline, nullptr);
        self->onSelectHooked_ = true;
    }
}

void BETH_CALL TCustomComboBox::SelectTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCustomComboBox* self = static_cast<TCustomComboBox*>(FromHandle(sender));
        if (!self || !self->onSelect_)
            return;
        TNotifyEvent handler = self->onSelect_;
        handler(self);
    });
}

TNotifyEvent TCustomComboBox::GetOnDropDownImpl(TObject* owner)
{
    return static_cast<TCustomComboBox*>(owner)->onDropDown_;
}
void TCustomComboBox::SetOnDropDownImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomComboBox* self = static_cast<TCustomComboBox*>(owner);
    self->onDropDown_ = value;
    if (value && !self->onDropDownHooked_)
    {
        internal::TCustomComboBox_SetOnDropDown(self->handle_, &TCustomComboBox::DropDownTrampoline, nullptr);
        self->onDropDownHooked_ = true;
    }
}

void BETH_CALL TCustomComboBox::DropDownTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCustomComboBox* self = static_cast<TCustomComboBox*>(FromHandle(sender));
        if (!self || !self->onDropDown_)
            return;
        TNotifyEvent handler = self->onDropDown_;
        handler(self);
    });
}

TNotifyEvent TCustomComboBox::GetOnCloseUpImpl(TObject* owner)
{
    return static_cast<TCustomComboBox*>(owner)->onCloseUp_;
}
void TCustomComboBox::SetOnCloseUpImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomComboBox* self = static_cast<TCustomComboBox*>(owner);
    self->onCloseUp_ = value;
    if (value && !self->onCloseUpHooked_)
    {
        internal::TCustomComboBox_SetOnCloseUp(self->handle_, &TCustomComboBox::CloseUpTrampoline, nullptr);
        self->onCloseUpHooked_ = true;
    }
}

void BETH_CALL TCustomComboBox::CloseUpTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCustomComboBox* self = static_cast<TCustomComboBox*>(FromHandle(sender));
        if (!self || !self->onCloseUp_)
            return;
        TNotifyEvent handler = self->onCloseUp_;
        handler(self);
    });
}

TStrings* TCustomComboBox::GetItemsImpl(TObject* owner) { return &static_cast<TCustomComboBox*>(owner)->items_; }

int  TCustomComboBox::GetItemIndexImpl(TObject* owner)                   { return internal::TCustomComboBox_GetItemIndex(owner->Handle()); }
void TCustomComboBox::SetItemIndexImpl(TObject* owner, const int& value) { internal::TCustomComboBox_SetItemIndex(owner->Handle(), value); }

TComboBox::TComboBox(TComponent* AOwner)
    : TCustomComboBox(internal::TComboBox_Create(HandleOf(AOwner)))
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
        internal::TComboBox_SetOnChange(self->handle_, &TComboBox::ChangeTrampoline, nullptr);
        self->onChangeHooked_ = true;
    }
}

void BETH_CALL TComboBox::ChangeTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TComboBox* self = static_cast<TComboBox*>(FromHandle(sender));
        if (!self || !self->onChange_)
            return;
        TNotifyEvent handler = self->onChange_;
        handler(self);
    });
}

TCustomListBox::TCustomListBox(ObjectHandle handle)
    : TWinControl(handle)
    , MultiSelect(this, &TCustomListBox::GetMultiSelectImpl, &TCustomListBox::SetMultiSelectImpl)
    , ExtendedSelect(this, &TCustomListBox::GetExtendedSelectImpl, &TCustomListBox::SetExtendedSelectImpl)
    , Sorted(this, &TCustomListBox::GetSortedImpl, &TCustomListBox::SetSortedImpl)
    , TopIndex(this, &TCustomListBox::GetTopIndexImpl, &TCustomListBox::SetTopIndexImpl)
    , SelCount(this, &TCustomListBox::GetSelCountImpl)
    , Selected(this, &TCustomListBox::GetSelectedImpl, &TCustomListBox::SetSelectedImpl)
    , OnSelectionChange(this, &TCustomListBox::GetOnSelectionChangeImpl, &TCustomListBox::SetOnSelectionChangeImpl)
    , ItemIndex(this, &TCustomListBox::GetItemIndexImpl, &TCustomListBox::SetItemIndexImpl)
    , Items(this, &TCustomListBox::GetItemsImpl)
    , items_(this, &internal::TCustomListBox_GetItems)
{}

// ---- docs/adr/0043 ----

bool TCustomListBox::GetMultiSelectImpl(TObject* owner)
{
    return internal::TCustomListBox_GetMultiSelect(owner->Handle()) != 0;
}

void TCustomListBox::SetMultiSelectImpl(TObject* owner, const bool& value)
{
    internal::TCustomListBox_SetMultiSelect(owner->Handle(), value ? 1 : 0);
}

bool TCustomListBox::GetExtendedSelectImpl(TObject* owner)
{
    return internal::TCustomListBox_GetExtendedSelect(owner->Handle()) != 0;
}

void TCustomListBox::SetExtendedSelectImpl(TObject* owner, const bool& value)
{
    internal::TCustomListBox_SetExtendedSelect(owner->Handle(), value ? 1 : 0);
}

bool TCustomListBox::GetSortedImpl(TObject* owner)
{
    return internal::TCustomListBox_GetSorted(owner->Handle()) != 0;
}

void TCustomListBox::SetSortedImpl(TObject* owner, const bool& value)
{
    internal::TCustomListBox_SetSorted(owner->Handle(), value ? 1 : 0);
}

int TCustomListBox::GetTopIndexImpl(TObject* owner)
{
    return internal::TCustomListBox_GetTopIndex(owner->Handle());
}

void TCustomListBox::SetTopIndexImpl(TObject* owner, const int& value)
{
    internal::TCustomListBox_SetTopIndex(owner->Handle(), value);
}

int TCustomListBox::GetSelCountImpl(TObject* owner)
{
    return internal::TCustomListBox_GetSelCount(owner->Handle());
}

bool TCustomListBox::GetSelectedImpl(TObject* owner, int index)
{
    return internal::TCustomListBox_GetSelected(owner->Handle(), index) != 0;
}

void TCustomListBox::SetSelectedImpl(TObject* owner, int index, const bool& value)
{
    internal::TCustomListBox_SetSelected(owner->Handle(), index, value ? 1 : 0);
}

void TCustomListBox::ClearSelection() { internal::TCustomListBox_ClearSelection(handle_); }

void TCustomListBox::SelectAll() { internal::TCustomListBox_SelectAll(handle_); }

int TCustomListBox::ItemAtPos(const TPoint& Pos, bool Existing) const
{
    return internal::TCustomListBox_ItemAtPos(handle_, Pos.X, Pos.Y, Existing ? 1 : 0);
}

TSelectionChangeEvent TCustomListBox::GetOnSelectionChangeImpl(TObject* owner)
{
    return static_cast<TCustomListBox*>(owner)->onSelectionChange_;
}
void TCustomListBox::SetOnSelectionChangeImpl(TObject* owner, const TSelectionChangeEvent& value)
{
    TCustomListBox* self = static_cast<TCustomListBox*>(owner);
    self->onSelectionChange_ = value;
    if (value && !self->onSelectionChangeHooked_)
    {
        internal::TCustomListBox_SetOnSelectionChange(self->handle_, &TCustomListBox::SelectionChangeTrampoline, nullptr);
        self->onSelectionChangeHooked_ = true;
    }
}

void BETH_CALL TCustomListBox::SelectionChangeTrampoline(ObjectHandle sender, internal::bool_t user, void*)
{
    GuardCallback([&] {
        TCustomListBox* self = static_cast<TCustomListBox*>(FromHandle(sender));
        if (!self || !self->onSelectionChange_)
            return;
        TSelectionChangeEvent handler = self->onSelectionChange_;
        handler(self, user != 0);
    });
}

TStrings* TCustomListBox::GetItemsImpl(TObject* owner) { return &static_cast<TCustomListBox*>(owner)->items_; }

int  TCustomListBox::GetItemIndexImpl(TObject* owner)                   { return internal::TCustomListBox_GetItemIndex(owner->Handle()); }
void TCustomListBox::SetItemIndexImpl(TObject* owner, const int& value) { internal::TCustomListBox_SetItemIndex(owner->Handle(), value); }

TListBox::TListBox(TComponent* AOwner)
    : TCustomListBox(internal::TListBox_Create(HandleOf(AOwner)))
{}

TCustomCheckListBox::TCustomCheckListBox(ObjectHandle handle)
    : TCustomListBox(handle)
    , OnClickCheck(this, &TCustomCheckListBox::GetOnClickCheckImpl, &TCustomCheckListBox::SetOnClickCheckImpl)
    , Checked(this, &TCustomCheckListBox::GetCheckedImpl, &TCustomCheckListBox::SetCheckedImpl)
{}

bool TCustomCheckListBox::GetCheckedImpl(TObject* owner, int index)                    { return internal::TCustomCheckListBox_GetChecked(owner->Handle(), index) != 0; }
void TCustomCheckListBox::SetCheckedImpl(TObject* owner, int index, const bool& value) { internal::TCustomCheckListBox_SetChecked(owner->Handle(), index, value ? 1 : 0); }

TNotifyEvent TCustomCheckListBox::GetOnClickCheckImpl(TObject* owner) { return static_cast<TCustomCheckListBox*>(owner)->onClickCheck_; }

void TCustomCheckListBox::SetOnClickCheckImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomCheckListBox* self = static_cast<TCustomCheckListBox*>(owner);
    SetSimpleEvent(self->handle_, self->onClickCheck_, self->onClickCheckHooked_, value,
                   &internal::TCustomCheckListBox_SetOnClickCheck, &TCustomCheckListBox::ClickCheckTrampoline);
}

void BETH_CALL TCustomCheckListBox::ClickCheckTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TCustomCheckListBox* self = static_cast<TCustomCheckListBox*>(FromHandle(sender)))
            CallNotify(self->onClickCheck_, self);
    });
}

TCheckListBox::TCheckListBox(TComponent* AOwner)
    : TCustomCheckListBox(internal::TCheckListBox_Create(HandleOf(AOwner)))
{}

TCustomStaticText::TCustomStaticText(ObjectHandle handle)
    : TWinControl(handle)
    , BorderStyle(this, &TCustomStaticText::GetBorderStyleImpl, &TCustomStaticText::SetBorderStyleImpl)
{}

TStaticBorderStyle TCustomStaticText::GetBorderStyleImpl(TObject* owner)
{
    return static_cast<TStaticBorderStyle>(internal::TCustomStaticText_GetBorderStyle(owner->Handle()));
}

void TCustomStaticText::SetBorderStyleImpl(TObject* owner, const TStaticBorderStyle& value)
{
    internal::TCustomStaticText_SetBorderStyle(owner->Handle(), value);
}

TStaticText::TStaticText(TComponent* AOwner)
    : TCustomStaticText(internal::TStaticText_Create(HandleOf(AOwner)))
{}

TStatusPanel::TStatusPanel(ObjectHandle handle)
    : TPersistent(handle)
    , Text(this, &TStatusPanel::GetTextImpl, &TStatusPanel::SetTextImpl)
    , Width(this, &TStatusPanel::GetWidthImpl, &TStatusPanel::SetWidthImpl)
    , Alignment(this, &TStatusPanel::GetAlignmentImpl, &TStatusPanel::SetAlignmentImpl)
    , Bevel(this, &TStatusPanel::GetBevelImpl, &TStatusPanel::SetBevelImpl)
    , Style(this, &TStatusPanel::GetStyleImpl, &TStatusPanel::SetStyleImpl)
    , Index(this, &TStatusPanel::GetIndexImpl, &TStatusPanel::SetIndexImpl)
{}

std::string TStatusPanel::GetTextImpl(TObject* owner) { return std::string(internal::TStatusPanel_GetText(owner->Handle())); }
void TStatusPanel::SetTextImpl(TObject* owner, const std::string& value) { internal::TStatusPanel_SetText(owner->Handle(), value.c_str()); }
int  TStatusPanel::GetWidthImpl(TObject* owner)                  { return internal::TStatusPanel_GetWidth(owner->Handle()); }
void TStatusPanel::SetWidthImpl(TObject* owner, const int& value) { internal::TStatusPanel_SetWidth(owner->Handle(), value); }
TAlignment TStatusPanel::GetAlignmentImpl(TObject* owner) { return static_cast<TAlignment>(internal::TStatusPanel_GetAlignment(owner->Handle())); }
void TStatusPanel::SetAlignmentImpl(TObject* owner, const TAlignment& value) { internal::TStatusPanel_SetAlignment(owner->Handle(), value); }
TStatusPanelBevel TStatusPanel::GetBevelImpl(TObject* owner) { return static_cast<TStatusPanelBevel>(internal::TStatusPanel_GetBevel(owner->Handle())); }
void TStatusPanel::SetBevelImpl(TObject* owner, const TStatusPanelBevel& value) { internal::TStatusPanel_SetBevel(owner->Handle(), value); }
TStatusPanelStyle TStatusPanel::GetStyleImpl(TObject* owner) { return static_cast<TStatusPanelStyle>(internal::TStatusPanel_GetStyle(owner->Handle())); }
void TStatusPanel::SetStyleImpl(TObject* owner, const TStatusPanelStyle& value) { internal::TStatusPanel_SetStyle(owner->Handle(), value); }
int  TStatusPanel::GetIndexImpl(TObject* owner)                  { return internal::TStatusPanel_GetIndex(owner->Handle()); }
void TStatusPanel::SetIndexImpl(TObject* owner, const int& value) { internal::TStatusPanel_SetIndex(owner->Handle(), value); }

TStatusPanels::TStatusPanels(ObjectHandle handle)
    : TPersistent(handle)
    , Count(this, &TStatusPanels::GetCountImpl)
    , Items(this, &TStatusPanels::GetItemsImpl)
{}

TStatusPanel* TStatusPanels::Add()             { return TStatusPanel::Wrap(internal::TStatusPanels_Add(handle_)); }
TStatusPanel* TStatusPanels::Insert(int Index) { return TStatusPanel::Wrap(internal::TStatusPanels_Insert(handle_, Index)); }
void TStatusPanels::Delete(int Index)          { internal::TStatusPanels_Delete(handle_, Index); }
void TStatusPanels::Clear()                    { internal::TStatusPanels_Clear(handle_); }
void TStatusPanels::BeginUpdate()              { internal::TStatusPanels_BeginUpdate(handle_); }
void TStatusPanels::EndUpdate()                { internal::TStatusPanels_EndUpdate(handle_); }
TStatusPanel* TStatusPanels::GetItemsImpl(TObject* owner, int Index) { return TStatusPanel::Wrap(internal::TStatusPanels_GetItem(owner->Handle(), Index)); }
int  TStatusPanels::GetCountImpl(TObject* owner) { return internal::TStatusPanels_GetCount(owner->Handle()); }

TStatusBar::TStatusBar(TComponent* AOwner)
    : TWinControl(internal::TStatusBar_Create(HandleOf(AOwner)))
    , SimpleText(this, &TStatusBar::GetSimpleTextImpl, &TStatusBar::SetSimpleTextImpl)
    , SimplePanel(this, &TStatusBar::GetSimplePanelImpl, &TStatusBar::SetSimplePanelImpl)
    , Panels(this, &TStatusBar::GetPanelsImpl)
    , SizeGrip(this, &TStatusBar::GetSizeGripImpl, &TStatusBar::SetSizeGripImpl)
    , AutoHint(this, &TStatusBar::GetAutoHintImpl, &TStatusBar::SetAutoHintImpl)
    , Canvas(internal::TStatusBar_GetCanvas(handle_))
    , OnDrawPanel(this, &TStatusBar::GetOnDrawPanelImpl, &TStatusBar::SetOnDrawPanelImpl)
    , OnHint(this, &TStatusBar::GetOnHintImpl, &TStatusBar::SetOnHintImpl)
    , panels_(internal::TStatusBar_GetPanels(handle_))
{}

int  TStatusBar::GetPanelIndexAt(int X, int Y) const { return internal::TStatusBar_GetPanelIndexAt(handle_, X, Y); }
void TStatusBar::BeginUpdate()                        { internal::TStatusBar_BeginUpdate(handle_); }
void TStatusBar::EndUpdate()                          { internal::TStatusBar_EndUpdate(handle_); }

TStatusPanels* TStatusBar::GetPanelsImpl(TObject* owner) { return &static_cast<TStatusBar*>(owner)->panels_; }
bool TStatusBar::GetSizeGripImpl(TObject* owner) { return internal::TStatusBar_GetSizeGrip(owner->Handle()) != 0; }
void TStatusBar::SetSizeGripImpl(TObject* owner, const bool& value) { internal::TStatusBar_SetSizeGrip(owner->Handle(), value ? 1 : 0); }
bool TStatusBar::GetAutoHintImpl(TObject* owner) { return internal::TStatusBar_GetAutoHint(owner->Handle()) != 0; }
void TStatusBar::SetAutoHintImpl(TObject* owner, const bool& value) { internal::TStatusBar_SetAutoHint(owner->Handle(), value ? 1 : 0); }

TDrawPanelEvent TStatusBar::GetOnDrawPanelImpl(TObject* owner) { return static_cast<TStatusBar*>(owner)->onDrawPanel_; }
void TStatusBar::SetOnDrawPanelImpl(TObject* owner, const TDrawPanelEvent& value)
{
    TStatusBar* self = static_cast<TStatusBar*>(owner);
    SetSimpleEvent(self->handle_, self->onDrawPanel_, self->onDrawPanelHooked_, value,
                   &internal::TStatusBar_SetOnDrawPanel, &TStatusBar::DrawPanelTrampoline);
}

TNotifyEvent TStatusBar::GetOnHintImpl(TObject* owner) { return static_cast<TStatusBar*>(owner)->onHint_; }
void TStatusBar::SetOnHintImpl(TObject* owner, const TNotifyEvent& value)
{
    TStatusBar* self = static_cast<TStatusBar*>(owner);
    SetSimpleEvent(self->handle_, self->onHint_, self->onHintHooked_, value,
                   &internal::TStatusBar_SetOnHint, &TStatusBar::HintTrampoline);
}

void BETH_CALL TStatusBar::DrawPanelTrampoline(ObjectHandle sender, ObjectHandle panel, internal::int_t left, internal::int_t top,
                                               internal::int_t right, internal::int_t bottom, void*)
{
    GuardCallback([&] {
        TStatusBar* self = static_cast<TStatusBar*>(FromHandle(sender));
        if (!self || !self->onDrawPanel_)
            return;
        TDrawPanelEvent handler = self->onDrawPanel_;
        handler(self, TStatusPanel::Wrap(panel), TRect{left, top, right, bottom});
    });
}

void BETH_CALL TStatusBar::HintTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TStatusBar* self = static_cast<TStatusBar*>(FromHandle(sender));
        if (!self || !self->onHint_)
            return;
        TNotifyEvent handler = self->onHint_;
        handler(self);
    });
}

std::string TStatusBar::GetSimpleTextImpl(TObject* owner)
{
    return std::string(internal::TStatusBar_GetSimpleText(owner->Handle()));
}

void TStatusBar::SetSimpleTextImpl(TObject* owner, const std::string& value)
{
    internal::TStatusBar_SetSimpleText(owner->Handle(), value.c_str());
}

bool TStatusBar::GetSimplePanelImpl(TObject* owner) { return internal::TStatusBar_GetSimplePanel(owner->Handle()) != 0; }
void TStatusBar::SetSimplePanelImpl(TObject* owner, const bool& value) { internal::TStatusBar_SetSimplePanel(owner->Handle(), value ? 1 : 0); }

/* ---------------- Canvas ---------------- */

TPen::TPen(ObjectHandle handle)
    : TPersistent(handle)
    , Color(this, &TPen::GetColorImpl, &TPen::SetColorImpl)
    , Width(this, &TPen::GetWidthImpl, &TPen::SetWidthImpl)
    , Style(this, &TPen::GetStyleImpl, &TPen::SetStyleImpl)
    , Mode(this, &TPen::GetModeImpl, &TPen::SetModeImpl)
{}

TPenStyle TPen::GetStyleImpl(TObject* owner) { return static_cast<TPenStyle>(internal::TPen_GetStyle(owner->Handle())); }
void TPen::SetStyleImpl(TObject* owner, const TPenStyle& value) { internal::TPen_SetStyle(owner->Handle(), value); }
TPenMode TPen::GetModeImpl(TObject* owner) { return static_cast<TPenMode>(internal::TPen_GetMode(owner->Handle())); }
void TPen::SetModeImpl(TObject* owner, const TPenMode& value) { internal::TPen_SetMode(owner->Handle(), value); }

TColor TPen::GetColorImpl(TObject* owner)                      { return static_cast<TColor>(internal::TPen_GetColor(owner->Handle())); }
void   TPen::SetColorImpl(TObject* owner, const TColor& value) { internal::TPen_SetColor(owner->Handle(), value); }
int    TPen::GetWidthImpl(TObject* owner)                      { return internal::TPen_GetWidth(owner->Handle()); }
void   TPen::SetWidthImpl(TObject* owner, const int& value)    { internal::TPen_SetWidth(owner->Handle(), value); }

TBrush::TBrush(ObjectHandle handle)
    : TPersistent(handle)
    , Color(this, &TBrush::GetColorImpl, &TBrush::SetColorImpl)
    , Style(this, &TBrush::GetStyleImpl, &TBrush::SetStyleImpl)
{}

TBrushStyle TBrush::GetStyleImpl(TObject* owner) { return static_cast<TBrushStyle>(internal::TBrush_GetStyle(owner->Handle())); }
void TBrush::SetStyleImpl(TObject* owner, const TBrushStyle& value) { internal::TBrush_SetStyle(owner->Handle(), value); }

TColor TBrush::GetColorImpl(TObject* owner)                      { return static_cast<TColor>(internal::TBrush_GetColor(owner->Handle())); }
void   TBrush::SetColorImpl(TObject* owner, const TColor& value) { internal::TBrush_SetColor(owner->Handle(), value); }

/* ---------------- TSizeConstraints / TControlBorderSpacing ---------------- */

TSizeConstraints::TSizeConstraints(ObjectHandle handle)
    : TPersistent(handle)
    , MinWidth(this, &TSizeConstraints::GetMinWidthImpl, &TSizeConstraints::SetMinWidthImpl)
    , MinHeight(this, &TSizeConstraints::GetMinHeightImpl, &TSizeConstraints::SetMinHeightImpl)
    , MaxWidth(this, &TSizeConstraints::GetMaxWidthImpl, &TSizeConstraints::SetMaxWidthImpl)
    , MaxHeight(this, &TSizeConstraints::GetMaxHeightImpl, &TSizeConstraints::SetMaxHeightImpl)
{}

int  TSizeConstraints::GetMinWidthImpl(TObject* owner)                    { return internal::TSizeConstraints_GetMinWidth(owner->Handle()); }
void TSizeConstraints::SetMinWidthImpl(TObject* owner, const int& value)  { internal::TSizeConstraints_SetMinWidth(owner->Handle(), value); }
int  TSizeConstraints::GetMinHeightImpl(TObject* owner)                   { return internal::TSizeConstraints_GetMinHeight(owner->Handle()); }
void TSizeConstraints::SetMinHeightImpl(TObject* owner, const int& value) { internal::TSizeConstraints_SetMinHeight(owner->Handle(), value); }
int  TSizeConstraints::GetMaxWidthImpl(TObject* owner)                    { return internal::TSizeConstraints_GetMaxWidth(owner->Handle()); }
void TSizeConstraints::SetMaxWidthImpl(TObject* owner, const int& value)  { internal::TSizeConstraints_SetMaxWidth(owner->Handle(), value); }
int  TSizeConstraints::GetMaxHeightImpl(TObject* owner)                   { return internal::TSizeConstraints_GetMaxHeight(owner->Handle()); }
void TSizeConstraints::SetMaxHeightImpl(TObject* owner, const int& value) { internal::TSizeConstraints_SetMaxHeight(owner->Handle(), value); }

TControlBorderSpacing::TControlBorderSpacing(ObjectHandle handle)
    : TPersistent(handle)
    , Left(this, &TControlBorderSpacing::GetLeftImpl, &TControlBorderSpacing::SetLeftImpl)
    , Top(this, &TControlBorderSpacing::GetTopImpl, &TControlBorderSpacing::SetTopImpl)
    , Right(this, &TControlBorderSpacing::GetRightImpl, &TControlBorderSpacing::SetRightImpl)
    , Bottom(this, &TControlBorderSpacing::GetBottomImpl, &TControlBorderSpacing::SetBottomImpl)
    , Around(this, &TControlBorderSpacing::GetAroundImpl, &TControlBorderSpacing::SetAroundImpl)
    , InnerBorder(this, &TControlBorderSpacing::GetInnerBorderImpl, &TControlBorderSpacing::SetInnerBorderImpl)
{}

int  TControlBorderSpacing::GetLeftImpl(TObject* owner)                          { return internal::TControlBorderSpacing_GetLeft(owner->Handle()); }
void TControlBorderSpacing::SetLeftImpl(TObject* owner, const int& value)        { internal::TControlBorderSpacing_SetLeft(owner->Handle(), value); }
int  TControlBorderSpacing::GetTopImpl(TObject* owner)                           { return internal::TControlBorderSpacing_GetTop(owner->Handle()); }
void TControlBorderSpacing::SetTopImpl(TObject* owner, const int& value)         { internal::TControlBorderSpacing_SetTop(owner->Handle(), value); }
int  TControlBorderSpacing::GetRightImpl(TObject* owner)                         { return internal::TControlBorderSpacing_GetRight(owner->Handle()); }
void TControlBorderSpacing::SetRightImpl(TObject* owner, const int& value)       { internal::TControlBorderSpacing_SetRight(owner->Handle(), value); }
int  TControlBorderSpacing::GetBottomImpl(TObject* owner)                        { return internal::TControlBorderSpacing_GetBottom(owner->Handle()); }
void TControlBorderSpacing::SetBottomImpl(TObject* owner, const int& value)      { internal::TControlBorderSpacing_SetBottom(owner->Handle(), value); }
int  TControlBorderSpacing::GetAroundImpl(TObject* owner)                        { return internal::TControlBorderSpacing_GetAround(owner->Handle()); }
void TControlBorderSpacing::SetAroundImpl(TObject* owner, const int& value)      { internal::TControlBorderSpacing_SetAround(owner->Handle(), value); }
int  TControlBorderSpacing::GetInnerBorderImpl(TObject* owner)                   { return internal::TControlBorderSpacing_GetInnerBorder(owner->Handle()); }
void TControlBorderSpacing::SetInnerBorderImpl(TObject* owner, const int& value) { internal::TControlBorderSpacing_SetInnerBorder(owner->Handle(), value); }

TFont::TFont(ObjectHandle handle)
    : TPersistent(handle)
    , Name(this, &TFont::GetNameImpl, &TFont::SetNameImpl)
    , Size(this, &TFont::GetSizeImpl, &TFont::SetSizeImpl)
    , Color(this, &TFont::GetColorImpl, &TFont::SetColorImpl)
    , Style(this, &TFont::GetStyleImpl, &TFont::SetStyleImpl)
    , Height(this, &TFont::GetHeightImpl, &TFont::SetHeightImpl)
    , Orientation(this, &TFont::GetOrientationImpl, &TFont::SetOrientationImpl)
    , Quality(this, &TFont::GetQualityImpl, &TFont::SetQualityImpl)
{}

int  TFont::GetHeightImpl(TObject* owner) { return internal::TFont_GetHeight(owner->Handle()); }
void TFont::SetHeightImpl(TObject* owner, const int& value) { internal::TFont_SetHeight(owner->Handle(), value); }
int  TFont::GetOrientationImpl(TObject* owner) { return internal::TFont_GetOrientation(owner->Handle()); }
void TFont::SetOrientationImpl(TObject* owner, const int& value) { internal::TFont_SetOrientation(owner->Handle(), value); }
TFontQuality TFont::GetQualityImpl(TObject* owner) { return static_cast<TFontQuality>(internal::TFont_GetQuality(owner->Handle())); }
void TFont::SetQualityImpl(TObject* owner, const TFontQuality& value) { internal::TFont_SetQuality(owner->Handle(), value); }

std::string TFont::GetNameImpl(TObject* owner)
{
    return std::string(internal::TFont_GetName(owner->Handle()));
}

void TFont::SetNameImpl(TObject* owner, const std::string& value)
{
    internal::TFont_SetName(owner->Handle(), value.c_str());
}

int    TFont::GetSizeImpl(TObject* owner)                      { return internal::TFont_GetSize(owner->Handle()); }
void   TFont::SetSizeImpl(TObject* owner, const int& value)    { internal::TFont_SetSize(owner->Handle(), value); }
TColor TFont::GetColorImpl(TObject* owner)                     { return static_cast<TColor>(internal::TFont_GetColor(owner->Handle())); }
void   TFont::SetColorImpl(TObject* owner, const TColor& value){ internal::TFont_SetColor(owner->Handle(), value); }
TFontStyles TFont::GetStyleImpl(TObject* owner)                         { return internal::TFont_GetStyle(owner->Handle()); }
void        TFont::SetStyleImpl(TObject* owner, const TFontStyles& value) { internal::TFont_SetStyle(owner->Handle(), value); }

void TFont::Assign(const TFont* Source)
{
    internal::TFont_Assign(handle_, Source ? Source->Handle() : nullptr);
}

TCanvas::TCanvas(ObjectHandle handle)
    : TPersistent(handle)
    , Pen(internal::TCanvas_GetPen(handle))
    , Brush(internal::TCanvas_GetBrush(handle))
    , Font(internal::TCanvas_GetFont(handle))
    , Pixels(this, &TCanvas::GetPixelsImpl, &TCanvas::SetPixelsImpl)
{}

void TCanvas::MoveTo(int x, int y)                        { internal::TCanvas_MoveTo(handle_, x, y); }
void TCanvas::LineTo(int x, int y)                        { internal::TCanvas_LineTo(handle_, x, y); }
void TCanvas::Rectangle(int x1, int y1, int x2, int y2)   { internal::TCanvas_Rectangle(handle_, x1, y1, x2, y2); }
void TCanvas::Ellipse(int x1, int y1, int x2, int y2)     { internal::TCanvas_Ellipse(handle_, x1, y1, x2, y2); }
void TCanvas::TextOut(int x, int y, const std::string& t) { internal::TCanvas_TextOut(handle_, x, y, t.c_str()); }
void TCanvas::FillRect(const TRect& Rect)                 { internal::TCanvas_FillRect(handle_, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom); }

void TCanvas::Draw(int X, int Y, const TGraphic* Graphic)
{
    if (Graphic)
        internal::TCanvas_Draw(handle_, X, Y, Graphic->Current());
}

void TCanvas::StretchDraw(const TRect& Rect, const TGraphic* Graphic)
{
    if (Graphic)
        internal::TCanvas_StretchDraw(handle_, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom, Graphic->Current());
}

// ---- docs/adr/0045 ----

namespace
{

// 点の並びを DLL に渡す形((X, Y) を並べた整数の配列)にする。
std::vector<internal::int_t> FlattenPoints(const TPoint* Points, int Count)
{
    std::vector<internal::int_t> flat;
    if (!Points || Count <= 0)
        return flat;
    flat.reserve(static_cast<size_t>(Count) * 2);
    for (int i = 0; i < Count; ++i)
    {
        flat.push_back(Points[i].X);
        flat.push_back(Points[i].Y);
    }
    return flat;
}

} // namespace

int TCanvas::TextWidth(const std::string& Text) const { return internal::TCanvas_TextWidth(handle_, Text.c_str()); }
int TCanvas::TextHeight(const std::string& Text) const { return internal::TCanvas_TextHeight(handle_, Text.c_str()); }

void TCanvas::TextRect(const TRect& Rect, int X, int Y, const std::string& Text)
{
    internal::TCanvas_TextRect(handle_, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom, X, Y, Text.c_str());
}

void TCanvas::Polygon(const std::vector<TPoint>& Points)
{
    Polygon(Points.data(), static_cast<int>(Points.size()));
}

void TCanvas::Polygon(const TPoint* Points, int Count)
{
    std::vector<internal::int_t> flat = FlattenPoints(Points, Count);
    if (!flat.empty())
        internal::TCanvas_Polygon(handle_, flat.data(), Count);
}

void TCanvas::Polyline(const std::vector<TPoint>& Points)
{
    Polyline(Points.data(), static_cast<int>(Points.size()));
}

void TCanvas::Polyline(const TPoint* Points, int Count)
{
    std::vector<internal::int_t> flat = FlattenPoints(Points, Count);
    if (!flat.empty())
        internal::TCanvas_Polyline(handle_, flat.data(), Count);
}

void TCanvas::RoundRect(int X1, int Y1, int X2, int Y2, int RX, int RY) { internal::TCanvas_RoundRect(handle_, X1, Y1, X2, Y2, RX, RY); }
void TCanvas::Arc(int X1, int Y1, int X2, int Y2, int X3, int Y3, int X4, int Y4) { internal::TCanvas_Arc(handle_, X1, Y1, X2, Y2, X3, Y3, X4, Y4); }
void TCanvas::Pie(int X1, int Y1, int X2, int Y2, int X3, int Y3, int X4, int Y4) { internal::TCanvas_Pie(handle_, X1, Y1, X2, Y2, X3, Y3, X4, Y4); }
void TCanvas::Chord(int X1, int Y1, int X2, int Y2, int X3, int Y3, int X4, int Y4) { internal::TCanvas_Chord(handle_, X1, Y1, X2, Y2, X3, Y3, X4, Y4); }

void TCanvas::FrameRect(const TRect& Rect)
{
    internal::TCanvas_FrameRect(handle_, Rect.Left, Rect.Top, Rect.Right, Rect.Bottom);
}

void TCanvas::CopyRect(const TRect& Dest, const TCanvas* Canvas, const TRect& Source)
{
    if (Canvas)
        internal::TCanvas_CopyRect(handle_, Dest.Left, Dest.Top, Dest.Right, Dest.Bottom, Canvas->Handle(), Source.Left,
                                   Source.Top, Source.Right, Source.Bottom);
}

TColor TCanvas::GetPixelsImpl(TObject* owner, int X, int Y) { return static_cast<TColor>(internal::TCanvas_GetPixels(owner->Handle(), X, Y)); }
void   TCanvas::SetPixelsImpl(TObject* owner, int X, int Y, const TColor& value) { internal::TCanvas_SetPixels(owner->Handle(), X, Y, value); }

/* ---------------- Graphics ---------------- */

TCanvas* CanvasHolder::Get(ObjectHandle canvas)
{
    if (!canvas)
        return nullptr;
    // 同じアドレスに別の Canvas が作られることもあるため、Pen・Brush・Font のハンドルも確かめる。
    if (!canvas_ || canvas_->Handle() != canvas
        || canvas_->Pen.Handle() != internal::TCanvas_GetPen(canvas)
        || canvas_->Brush.Handle() != internal::TCanvas_GetBrush(canvas)
        || canvas_->Font.Handle() != internal::TCanvas_GetFont(canvas))
        canvas_.reset(new TCanvas(canvas));
    return canvas_.get();
}

TGraphic::TGraphic(ObjectHandle handle)
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
        DestroyNoThrow(&internal::TGraphic_Destroy, handle_);
}

void TGraphic::LoadFromFile(const std::string& FileName)     { internal::TGraphic_LoadFromFile(Current(), FileName.c_str()); }
void TGraphic::SaveToFile(const std::string& FileName) const { internal::TGraphic_SaveToFile(Current(), FileName.c_str()); }
void TGraphic::Assign(const TGraphic* Source)                { internal::TGraphic_Assign(Current(), Source ? Source->Current() : nullptr); }
void TGraphic::Assign(const TClipboard* Source)
{
    if (Source)
        internal::TGraphic_Assign(Current(), Source->Handle());
}
void TGraphic::Clear()                                       { internal::TGraphic_Clear(Current()); }

int  TGraphic::GetWidthImpl(TObject* owner)                   { return internal::TGraphic_GetWidth(static_cast<TGraphic*>(owner)->Current()); }
void TGraphic::SetWidthImpl(TObject* owner, const int& value) { internal::TGraphic_SetWidth(static_cast<TGraphic*>(owner)->Current(), value); }
int  TGraphic::GetHeightImpl(TObject* owner)                  { return internal::TGraphic_GetHeight(static_cast<TGraphic*>(owner)->Current()); }
void TGraphic::SetHeightImpl(TObject* owner, const int& value) { internal::TGraphic_SetHeight(static_cast<TGraphic*>(owner)->Current(), value); }
bool TGraphic::GetEmptyImpl(TObject* owner)                   { return internal::TGraphic_GetEmpty(static_cast<TGraphic*>(owner)->Current()) != 0; }
bool TGraphic::GetTransparentImpl(TObject* owner)             { return internal::TGraphic_GetTransparent(static_cast<TGraphic*>(owner)->Current()) != 0; }
void TGraphic::SetTransparentImpl(TObject* owner, const bool& value)
{
    internal::TGraphic_SetTransparent(static_cast<TGraphic*>(owner)->Current(), value ? 1 : 0);
}

TRasterImage::TRasterImage(ObjectHandle handle)
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
    return self->canvas_.Get(internal::TRasterImage_GetCanvas(self->Current()));
}

TPixelFormat TRasterImage::GetPixelFormatImpl(TObject* owner)
{
    return static_cast<TPixelFormat>(internal::TRasterImage_GetPixelFormat(static_cast<TRasterImage*>(owner)->Current()));
}
void TRasterImage::SetPixelFormatImpl(TObject* owner, const TPixelFormat& value)
{
    internal::TRasterImage_SetPixelFormat(static_cast<TRasterImage*>(owner)->Current(), value);
}
TColor TRasterImage::GetTransparentColorImpl(TObject* owner)
{
    return static_cast<TColor>(internal::TRasterImage_GetTransparentColor(static_cast<TRasterImage*>(owner)->Current()));
}
void TRasterImage::SetTransparentColorImpl(TObject* owner, const TColor& value)
{
    internal::TRasterImage_SetTransparentColor(static_cast<TRasterImage*>(owner)->Current(), value);
}
TTransparentMode TRasterImage::GetTransparentModeImpl(TObject* owner)
{
    return static_cast<TTransparentMode>(internal::TRasterImage_GetTransparentMode(static_cast<TRasterImage*>(owner)->Current()));
}
void TRasterImage::SetTransparentModeImpl(TObject* owner, const TTransparentMode& value)
{
    internal::TRasterImage_SetTransparentMode(static_cast<TRasterImage*>(owner)->Current(), value);
}

void TCustomBitmap::SetSize(int AWidth, int AHeight) { internal::TCustomBitmap_SetSize(Current(), AWidth, AHeight); }

TBitmap::TBitmap() : TCustomBitmap(internal::TBitmap_Create()) {}

TPortableNetworkGraphic::TPortableNetworkGraphic() : TCustomBitmap(internal::TPortableNetworkGraphic_Create()) {}

TJPEGImage::TJPEGImage()
    : TCustomBitmap(internal::TJPEGImage_Create())
    , CompressionQuality(this, &TJPEGImage::GetCompressionQualityImpl, &TJPEGImage::SetCompressionQualityImpl)
{}

TJPEGImage::TJPEGImage(TObject* owner, Accessor accessor)
    : TCustomBitmap(owner, accessor)
    , CompressionQuality(this, &TJPEGImage::GetCompressionQualityImpl, &TJPEGImage::SetCompressionQualityImpl)
{}

int TJPEGImage::GetCompressionQualityImpl(TObject* owner)
{
    return internal::TJPEGImage_GetCompressionQuality(static_cast<TJPEGImage*>(owner)->Current());
}
void TJPEGImage::SetCompressionQualityImpl(TObject* owner, const int& value)
{
    internal::TJPEGImage_SetCompressionQuality(static_cast<TJPEGImage*>(owner)->Current(), value);
}

TIcon::TIcon() : TRasterImage(internal::TIcon_Create()) {}

TPicture::TPicture() : TPicture(internal::TPicture_Create(), true) {}

TPicture::TPicture(ObjectHandle handle, bool owns)
    : TPersistent(handle)
    , Graphic(this, &TPicture::GetGraphicImpl, &TPicture::SetGraphicImpl)
    , Bitmap(this, &TPicture::GetBitmapImpl, &TPicture::SetBitmapImpl)
    , PNG(this, &TPicture::GetPNGImpl, &TPicture::SetPNGImpl)
    , Jpeg(this, &TPicture::GetJpegImpl, &TPicture::SetJpegImpl)
    , Icon(this, &TPicture::GetIconImpl, &TPicture::SetIconImpl)
    , Width(this, &TPicture::GetWidthImpl)
    , Height(this, &TPicture::GetHeightImpl)
    , owns_(owns)
    , graphic_(this, &internal::TPicture_GetGraphic)
    , bitmap_(this, &internal::TPicture_GetBitmap)
    , png_(this, &internal::TPicture_GetPNG)
    , jpeg_(this, &internal::TPicture_GetJpeg)
    , icon_(this, &internal::TPicture_GetIcon)
{}

TPicture::~TPicture()
{
    if (owns_)
        DestroyNoThrow(&internal::TPicture_Destroy, handle_);
}

void TPicture::LoadFromFile(const std::string& FileName)     { internal::TPicture_LoadFromFile(handle_, FileName.c_str()); }
void TPicture::SaveToFile(const std::string& FileName) const { internal::TPicture_SaveToFile(handle_, FileName.c_str()); }
void TPicture::Assign(const TPicture* Source)                { internal::TPicture_Assign(handle_, Source ? Source->Handle() : nullptr); }
void TPicture::Assign(const TClipboard* Source)
{
    if (Source)
        internal::TPicture_Assign(handle_, Source->Handle());
}
void TPicture::Clear()                                       { internal::TPicture_Clear(handle_); }

// 空の TPicture の Graphic は nullptr(VCL と同じ)。それ以外は、クラスを問わないビューを返す。
TGraphic* TPicture::GetGraphicImpl(TObject* owner)
{
    TPicture* self = static_cast<TPicture*>(owner);
    return internal::TPicture_GetGraphic(self->handle_) ? &self->graphic_ : nullptr;
}
void TPicture::SetGraphicImpl(TObject* owner, TGraphic* const& value)
{
    internal::TPicture_SetGraphic(owner->Handle(), value ? value->Current() : nullptr);
}
// Bitmap・PNG・Jpeg は、ビューを返すだけで中身には触れない(変換はビューを操作したときに LCL が行う)。
TBitmap* TPicture::GetBitmapImpl(TObject* owner) { return &static_cast<TPicture*>(owner)->bitmap_; }
void TPicture::SetBitmapImpl(TObject* owner, TBitmap* const& value)
{
    internal::TPicture_SetGraphic(owner->Handle(), value ? value->Current() : nullptr);
}
TPortableNetworkGraphic* TPicture::GetPNGImpl(TObject* owner) { return &static_cast<TPicture*>(owner)->png_; }
void TPicture::SetPNGImpl(TObject* owner, TPortableNetworkGraphic* const& value)
{
    internal::TPicture_SetGraphic(owner->Handle(), value ? value->Current() : nullptr);
}
TJPEGImage* TPicture::GetJpegImpl(TObject* owner) { return &static_cast<TPicture*>(owner)->jpeg_; }
void TPicture::SetJpegImpl(TObject* owner, TJPEGImage* const& value)
{
    internal::TPicture_SetGraphic(owner->Handle(), value ? value->Current() : nullptr);
}
TIcon* TPicture::GetIconImpl(TObject* owner) { return &static_cast<TPicture*>(owner)->icon_; }
void TPicture::SetIconImpl(TObject* owner, TIcon* const& value)
{
    internal::TPicture_SetIcon(owner->Handle(), value ? value->Current() : nullptr);
}
int TPicture::GetWidthImpl(TObject* owner)  { return internal::TPicture_GetWidth(owner->Handle()); }
int TPicture::GetHeightImpl(TObject* owner) { return internal::TPicture_GetHeight(owner->Handle()); }

TPaintBox::TPaintBox(TComponent* AOwner)
    : TGraphicControl(internal::TPaintBox_Create(HandleOf(AOwner)))
    , Canvas(internal::TPaintBox_GetCanvas(handle_))
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
        internal::TPaintBox_SetOnPaint(self->handle_, &TPaintBox::PaintTrampoline, nullptr);
        self->onPaintHooked_ = true;
    }
}

void BETH_CALL TPaintBox::PaintTrampoline(ObjectHandle sender, void*)
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

TCustomImage::TCustomImage(ObjectHandle handle)
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
    , picture_(internal::TCustomImage_GetPicture(handle), false)
{}

TPicture* TCustomImage::GetPictureImpl(TObject* owner) { return &static_cast<TCustomImage*>(owner)->picture_; }
void TCustomImage::SetPictureImpl(TObject* owner, TPicture* const& value)
{
    internal::TCustomImage_SetPicture(owner->Handle(), value ? value->Handle() : nullptr);
}
TCanvas* TCustomImage::GetCanvasImpl(TObject* owner)
{
    TCustomImage* self = static_cast<TCustomImage*>(owner);
    return self->canvas_.Get(internal::TCustomImage_GetCanvas(self->handle_));
}
bool TCustomImage::GetHasGraphicImpl(TObject* owner) { return internal::TCustomImage_GetHasGraphic(owner->Handle()) != 0; }
bool TCustomImage::GetCenterImpl(TObject* owner)     { return internal::TCustomImage_GetCenter(owner->Handle()) != 0; }
void TCustomImage::SetCenterImpl(TObject* owner, const bool& value) { internal::TCustomImage_SetCenter(owner->Handle(), value ? 1 : 0); }
bool TCustomImage::GetStretchImpl(TObject* owner)    { return internal::TCustomImage_GetStretch(owner->Handle()) != 0; }
void TCustomImage::SetStretchImpl(TObject* owner, const bool& value) { internal::TCustomImage_SetStretch(owner->Handle(), value ? 1 : 0); }
bool TCustomImage::GetStretchOutEnabledImpl(TObject* owner) { return internal::TCustomImage_GetStretchOutEnabled(owner->Handle()) != 0; }
void TCustomImage::SetStretchOutEnabledImpl(TObject* owner, const bool& value)
{
    internal::TCustomImage_SetStretchOutEnabled(owner->Handle(), value ? 1 : 0);
}
bool TCustomImage::GetStretchInEnabledImpl(TObject* owner) { return internal::TCustomImage_GetStretchInEnabled(owner->Handle()) != 0; }
void TCustomImage::SetStretchInEnabledImpl(TObject* owner, const bool& value)
{
    internal::TCustomImage_SetStretchInEnabled(owner->Handle(), value ? 1 : 0);
}
bool TCustomImage::GetProportionalImpl(TObject* owner) { return internal::TCustomImage_GetProportional(owner->Handle()) != 0; }
void TCustomImage::SetProportionalImpl(TObject* owner, const bool& value) { internal::TCustomImage_SetProportional(owner->Handle(), value ? 1 : 0); }
bool TCustomImage::GetTransparentImpl(TObject* owner)  { return internal::TCustomImage_GetTransparent(owner->Handle()) != 0; }
void TCustomImage::SetTransparentImpl(TObject* owner, const bool& value) { internal::TCustomImage_SetTransparent(owner->Handle(), value ? 1 : 0); }

void BETH_CALL TCustomImage::PictureChangedTrampoline(ObjectHandle sender, void*)
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
                   &internal::TCustomImage_SetOnPictureChanged, &TCustomImage::PictureChangedTrampoline);
}

TImage::TImage(TComponent* AOwner)
    : TCustomImage(internal::TImage_Create(HandleOf(AOwner)))
{}

/* ---------------- Timer ---------------- */

TCustomTimer::TCustomTimer(ObjectHandle handle)
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
        internal::TCustomTimer_SetOnTimer(self->handle_, &TCustomTimer::TimerTrampoline, nullptr);
        self->onTimerHooked_ = true;
    }
}

void BETH_CALL TCustomTimer::TimerTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCustomTimer* self = static_cast<TCustomTimer*>(FromHandle(sender));
        if (!self || !self->onTimer_)
            return;
        TNotifyEvent handler = self->onTimer_;
        handler(self);
    });
}

int  TCustomTimer::GetIntervalImpl(TObject* owner)                   { return internal::TCustomTimer_GetInterval(owner->Handle()); }
void TCustomTimer::SetIntervalImpl(TObject* owner, const int& value) { internal::TCustomTimer_SetInterval(owner->Handle(), value); }
bool TCustomTimer::GetEnabledImpl(TObject* owner)                    { return internal::TCustomTimer_GetEnabled(owner->Handle()) != 0; }
void TCustomTimer::SetEnabledImpl(TObject* owner, const bool& value) { internal::TCustomTimer_SetEnabled(owner->Handle(), value ? 1 : 0); }

TTimer::TTimer(TComponent* AOwner)
    : TCustomTimer(internal::TTimer_Create(HandleOf(AOwner)))
{}

/* ---------------- Action(docs/adr/0046) ---------------- */

TBasicAction::TBasicAction(ObjectHandle handle)
    : TComponent(handle)
    , ActionComponent(this, &TBasicAction::GetActionComponentImpl)
    , OnExecute(this, &TBasicAction::GetOnExecuteImpl, &TBasicAction::SetOnExecuteImpl)
    , OnUpdate(this, &TBasicAction::GetOnUpdateImpl, &TBasicAction::SetOnUpdateImpl)
{}

bool TBasicAction::Execute() { return internal::TBasicAction_Execute(handle_) != 0; }
bool TBasicAction::Update() { return internal::TBasicAction_Update(handle_) != 0; }

TComponent* TBasicAction::GetActionComponentImpl(TObject* owner)
{
    return static_cast<TComponent*>(FromHandle(internal::TBasicAction_GetActionComponent(owner->Handle())));
}

TNotifyEvent TBasicAction::GetOnExecuteImpl(TObject* owner) { return static_cast<TBasicAction*>(owner)->onExecute_; }
void TBasicAction::SetOnExecuteImpl(TObject* owner, const TNotifyEvent& value)
{
    TBasicAction* self = static_cast<TBasicAction*>(owner);
    SetSimpleEvent(self->handle_, self->onExecute_, self->onExecuteHooked_, value,
                   &internal::TBasicAction_SetOnExecute, &TBasicAction::ExecuteTrampoline);
}

TNotifyEvent TBasicAction::GetOnUpdateImpl(TObject* owner) { return static_cast<TBasicAction*>(owner)->onUpdate_; }
void TBasicAction::SetOnUpdateImpl(TObject* owner, const TNotifyEvent& value)
{
    TBasicAction* self = static_cast<TBasicAction*>(owner);
    SetSimpleEvent(self->handle_, self->onUpdate_, self->onUpdateHooked_, value,
                   &internal::TBasicAction_SetOnUpdate, &TBasicAction::UpdateTrampoline);
}

void BETH_CALL TBasicAction::ExecuteTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TBasicAction* self = static_cast<TBasicAction*>(FromHandle(sender));
        if (!self || !self->onExecute_)
            return;
        TNotifyEvent handler = self->onExecute_;
        handler(self);
    });
}

void BETH_CALL TBasicAction::UpdateTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TBasicAction* self = static_cast<TBasicAction*>(FromHandle(sender));
        if (!self || !self->onUpdate_)
            return;
        TNotifyEvent handler = self->onUpdate_;
        handler(self);
    });
}

TContainedAction::TContainedAction(ObjectHandle handle)
    : TBasicAction(handle)
    , ActionList(this, &TContainedAction::GetActionListImpl, &TContainedAction::SetActionListImpl)
    , Category(this, &TContainedAction::GetCategoryImpl, &TContainedAction::SetCategoryImpl)
    , Index(this, &TContainedAction::GetIndexImpl, &TContainedAction::SetIndexImpl)
{}

TCustomActionList* TContainedAction::GetActionListImpl(TObject* owner)
{
    return static_cast<TCustomActionList*>(FromHandle(internal::TContainedAction_GetActionList(owner->Handle())));
}
void TContainedAction::SetActionListImpl(TObject* owner, TCustomActionList* const& value)
{
    internal::TContainedAction_SetActionList(owner->Handle(), HandleOf(value));
}
std::string TContainedAction::GetCategoryImpl(TObject* owner) { return internal::TContainedAction_GetCategory(owner->Handle()); }
void TContainedAction::SetCategoryImpl(TObject* owner, const std::string& value) { internal::TContainedAction_SetCategory(owner->Handle(), value.c_str()); }
int  TContainedAction::GetIndexImpl(TObject* owner) { return internal::TContainedAction_GetIndex(owner->Handle()); }
void TContainedAction::SetIndexImpl(TObject* owner, const int& value) { internal::TContainedAction_SetIndex(owner->Handle(), value); }

TCustomAction::TCustomAction(ObjectHandle handle)
    : TContainedAction(handle)
    , Caption(this, &TCustomAction::GetCaptionImpl, &TCustomAction::SetCaptionImpl)
    , Hint(this, &TCustomAction::GetHintImpl, &TCustomAction::SetHintImpl)
    , Checked(this, &TCustomAction::GetCheckedImpl, &TCustomAction::SetCheckedImpl)
    , AutoCheck(this, &TCustomAction::GetAutoCheckImpl, &TCustomAction::SetAutoCheckImpl)
    , GroupIndex(this, &TCustomAction::GetGroupIndexImpl, &TCustomAction::SetGroupIndexImpl)
    , Enabled(this, &TCustomAction::GetEnabledImpl, &TCustomAction::SetEnabledImpl)
    , Visible(this, &TCustomAction::GetVisibleImpl, &TCustomAction::SetVisibleImpl)
    , ImageIndex(this, &TCustomAction::GetImageIndexImpl, &TCustomAction::SetImageIndexImpl)
    , ShortCut(this, &TCustomAction::GetShortCutImpl, &TCustomAction::SetShortCutImpl)
    , DisableIfNoHandler(this, &TCustomAction::GetDisableIfNoHandlerImpl, &TCustomAction::SetDisableIfNoHandlerImpl)
{}

std::string TCustomAction::GetCaptionImpl(TObject* owner) { return internal::TCustomAction_GetCaption(owner->Handle()); }
void TCustomAction::SetCaptionImpl(TObject* owner, const std::string& value) { internal::TCustomAction_SetCaption(owner->Handle(), value.c_str()); }
std::string TCustomAction::GetHintImpl(TObject* owner) { return internal::TCustomAction_GetHint(owner->Handle()); }
void TCustomAction::SetHintImpl(TObject* owner, const std::string& value) { internal::TCustomAction_SetHint(owner->Handle(), value.c_str()); }
bool TCustomAction::GetCheckedImpl(TObject* owner) { return internal::TCustomAction_GetChecked(owner->Handle()) != 0; }
void TCustomAction::SetCheckedImpl(TObject* owner, const bool& value) { internal::TCustomAction_SetChecked(owner->Handle(), value ? 1 : 0); }
bool TCustomAction::GetAutoCheckImpl(TObject* owner) { return internal::TCustomAction_GetAutoCheck(owner->Handle()) != 0; }
void TCustomAction::SetAutoCheckImpl(TObject* owner, const bool& value) { internal::TCustomAction_SetAutoCheck(owner->Handle(), value ? 1 : 0); }
int  TCustomAction::GetGroupIndexImpl(TObject* owner) { return internal::TCustomAction_GetGroupIndex(owner->Handle()); }
void TCustomAction::SetGroupIndexImpl(TObject* owner, const int& value) { internal::TCustomAction_SetGroupIndex(owner->Handle(), value); }
bool TCustomAction::GetEnabledImpl(TObject* owner) { return internal::TCustomAction_GetEnabled(owner->Handle()) != 0; }
void TCustomAction::SetEnabledImpl(TObject* owner, const bool& value) { internal::TCustomAction_SetEnabled(owner->Handle(), value ? 1 : 0); }
bool TCustomAction::GetVisibleImpl(TObject* owner) { return internal::TCustomAction_GetVisible(owner->Handle()) != 0; }
void TCustomAction::SetVisibleImpl(TObject* owner, const bool& value) { internal::TCustomAction_SetVisible(owner->Handle(), value ? 1 : 0); }
int  TCustomAction::GetImageIndexImpl(TObject* owner) { return internal::TCustomAction_GetImageIndex(owner->Handle()); }
void TCustomAction::SetImageIndexImpl(TObject* owner, const int& value) { internal::TCustomAction_SetImageIndex(owner->Handle(), value); }
TShortCut TCustomAction::GetShortCutImpl(TObject* owner) { return static_cast<TShortCut>(internal::TCustomAction_GetShortCut(owner->Handle())); }
void TCustomAction::SetShortCutImpl(TObject* owner, const TShortCut& value) { internal::TCustomAction_SetShortCut(owner->Handle(), value); }
bool TCustomAction::GetDisableIfNoHandlerImpl(TObject* owner) { return internal::TCustomAction_GetDisableIfNoHandler(owner->Handle()) != 0; }
void TCustomAction::SetDisableIfNoHandlerImpl(TObject* owner, const bool& value) { internal::TCustomAction_SetDisableIfNoHandler(owner->Handle(), value ? 1 : 0); }

TAction::TAction(TComponent* AOwner)
    : TCustomAction(internal::TAction_Create(HandleOf(AOwner)))
{}

TCustomActionList::TCustomActionList(ObjectHandle handle)
    : TComponent(handle)
    , Actions(this, &TCustomActionList::GetActionsImpl)
    , ActionCount(this, &TCustomActionList::GetActionCountImpl)
    , Images(this, &TCustomActionList::GetImagesImpl, &TCustomActionList::SetImagesImpl)
    , State(this, &TCustomActionList::GetStateImpl, &TCustomActionList::SetStateImpl)
    , OnExecute(this, &TCustomActionList::GetOnExecuteImpl, &TCustomActionList::SetOnExecuteImpl)
    , OnUpdate(this, &TCustomActionList::GetOnUpdateImpl, &TCustomActionList::SetOnUpdateImpl)
{}

TContainedAction* TCustomActionList::GetActionsImpl(TObject* owner, int Index)
{
    return static_cast<TContainedAction*>(FromHandle(internal::TCustomActionList_GetActions(owner->Handle(), Index)));
}
int TCustomActionList::GetActionCountImpl(TObject* owner) { return internal::TCustomActionList_GetActionCount(owner->Handle()); }
TCustomImageList* TCustomActionList::GetImagesImpl(TObject* owner)
{
    return static_cast<TCustomImageList*>(FromHandle(internal::TCustomActionList_GetImages(owner->Handle())));
}
void TCustomActionList::SetImagesImpl(TObject* owner, TCustomImageList* const& value)
{
    internal::TCustomActionList_SetImages(owner->Handle(), HandleOf(value));
}
TActionListState TCustomActionList::GetStateImpl(TObject* owner) { return static_cast<TActionListState>(internal::TCustomActionList_GetState(owner->Handle())); }
void TCustomActionList::SetStateImpl(TObject* owner, const TActionListState& value) { internal::TCustomActionList_SetState(owner->Handle(), value); }

TActionEvent TCustomActionList::GetOnExecuteImpl(TObject* owner) { return static_cast<TCustomActionList*>(owner)->onExecute_; }
void TCustomActionList::SetOnExecuteImpl(TObject* owner, const TActionEvent& value)
{
    TCustomActionList* self = static_cast<TCustomActionList*>(owner);
    SetSimpleEvent(self->handle_, self->onExecute_, self->onExecuteHooked_, value,
                   &internal::TCustomActionList_SetOnExecute, &TCustomActionList::ExecuteTrampoline);
}

TActionEvent TCustomActionList::GetOnUpdateImpl(TObject* owner) { return static_cast<TCustomActionList*>(owner)->onUpdate_; }
void TCustomActionList::SetOnUpdateImpl(TObject* owner, const TActionEvent& value)
{
    TCustomActionList* self = static_cast<TCustomActionList*>(owner);
    SetSimpleEvent(self->handle_, self->onUpdate_, self->onUpdateHooked_, value,
                   &internal::TCustomActionList_SetOnUpdate, &TCustomActionList::UpdateTrampoline);
}

namespace
{

// ActionList の OnExecute・OnUpdate の共通の呼び出し(ハンドラはコピーしてから呼ぶ)。
void CallActionEvent(TCustomActionList* self, TActionEvent handler, TBasicAction* action, internal::bool_t* handled)
{
    if (!handler)
        return;
    bool value = *handled != 0;
    handler(self, action, value);
    *handled = value ? 1 : 0;
}

} // namespace

void BETH_CALL TCustomActionList::ExecuteTrampoline(ObjectHandle sender, ObjectHandle action, internal::bool_t* handled, void*)
{
    GuardCallback([&] {
        if (TCustomActionList* self = static_cast<TCustomActionList*>(FromHandle(sender)))
            CallActionEvent(self, self->onExecute_, static_cast<TBasicAction*>(FromHandle(action)), handled);
    });
}

void BETH_CALL TCustomActionList::UpdateTrampoline(ObjectHandle sender, ObjectHandle action, internal::bool_t* handled, void*)
{
    GuardCallback([&] {
        if (TCustomActionList* self = static_cast<TCustomActionList*>(FromHandle(sender)))
            CallActionEvent(self, self->onUpdate_, static_cast<TBasicAction*>(FromHandle(action)), handled);
    });
}

TActionList::TActionList(TComponent* AOwner)
    : TCustomActionList(internal::TActionList_Create(HandleOf(AOwner)))
{}

/* ---------------- ImageList ---------------- */

TCustomImageList::TCustomImageList(ObjectHandle handle)
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
    return internal::TCustomImageList_Add(handle_, Image ? Image->Current() : nullptr, Mask ? Mask->Current() : nullptr);
}
int TCustomImageList::AddSliced(const TCustomBitmap* Image, int AHorizontalCount, int AVerticalCount)
{
    return internal::TCustomImageList_AddSliced(handle_, Image ? Image->Current() : nullptr, AHorizontalCount, AVerticalCount);
}
int TCustomImageList::AddMasked(const TBitmap* Image, TColor MaskColor)
{
    return internal::TCustomImageList_AddMasked(handle_, Image ? Image->Current() : nullptr, MaskColor);
}
void TCustomImageList::Insert(int Index, const TCustomBitmap* Image, const TCustomBitmap* Mask)
{
    internal::TCustomImageList_Insert(handle_, Index, Image ? Image->Current() : nullptr, Mask ? Mask->Current() : nullptr);
}
void TCustomImageList::Replace(int Index, const TCustomBitmap* Image, const TCustomBitmap* Mask)
{
    internal::TCustomImageList_Replace(handle_, Index, Image ? Image->Current() : nullptr, Mask ? Mask->Current() : nullptr);
}
void TCustomImageList::Delete(int Index)                  { internal::TCustomImageList_Delete(handle_, Index); }
void TCustomImageList::Clear()                            { internal::TCustomImageList_Clear(handle_); }
void TCustomImageList::Move(int CurIndex, int NewIndex)   { internal::TCustomImageList_Move(handle_, CurIndex, NewIndex); }
void TCustomImageList::GetBitmap(int Index, TCustomBitmap* Image) const
{
    if (Image)
        internal::TCustomImageList_GetBitmap(handle_, Index, Image->Current());
}
void TCustomImageList::Draw(TCanvas* Canvas, int X, int Y, int Index, bool Enabled) const
{
    if (Canvas)
        internal::TCustomImageList_Draw(handle_, Canvas->Handle(), X, Y, Index, Enabled ? 1 : 0);
}
void TCustomImageList::BeginUpdate() { internal::TCustomImageList_BeginUpdate(handle_); }
void TCustomImageList::EndUpdate()   { internal::TCustomImageList_EndUpdate(handle_); }

int    TCustomImageList::GetWidthImpl(TObject* owner)                   { return internal::TCustomImageList_GetWidth(owner->Handle()); }
void   TCustomImageList::SetWidthImpl(TObject* owner, const int& value) { internal::TCustomImageList_SetWidth(owner->Handle(), value); }
int    TCustomImageList::GetHeightImpl(TObject* owner)                  { return internal::TCustomImageList_GetHeight(owner->Handle()); }
void   TCustomImageList::SetHeightImpl(TObject* owner, const int& value) { internal::TCustomImageList_SetHeight(owner->Handle(), value); }
int    TCustomImageList::GetCountImpl(TObject* owner)                   { return internal::TCustomImageList_GetCount(owner->Handle()); }
bool   TCustomImageList::GetMaskedImpl(TObject* owner)                  { return internal::TCustomImageList_GetMasked(owner->Handle()) != 0; }
void   TCustomImageList::SetMaskedImpl(TObject* owner, const bool& value) { internal::TCustomImageList_SetMasked(owner->Handle(), value ? 1 : 0); }
TColor TCustomImageList::GetBkColorImpl(TObject* owner)                 { return static_cast<TColor>(internal::TCustomImageList_GetBkColor(owner->Handle())); }
void   TCustomImageList::SetBkColorImpl(TObject* owner, const TColor& value) { internal::TCustomImageList_SetBkColor(owner->Handle(), value); }
TDrawingStyle TCustomImageList::GetDrawingStyleImpl(TObject* owner)
{
    return static_cast<TDrawingStyle>(internal::TCustomImageList_GetDrawingStyle(owner->Handle()));
}
void TCustomImageList::SetDrawingStyleImpl(TObject* owner, const TDrawingStyle& value)
{
    internal::TCustomImageList_SetDrawingStyle(owner->Handle(), value);
}

void BETH_CALL TCustomImageList::ChangeTrampoline(ObjectHandle sender, void*)
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
                   &internal::TCustomImageList_SetOnChange, &TCustomImageList::ChangeTrampoline);
}

TImageList::TImageList(TComponent* AOwner)
    : TCustomImageList(internal::TImageList_Create(HandleOf(AOwner)))
{}

/* ---------------- メニュー ---------------- */

TShortCut ShortCut(unsigned short Key, TShiftState Shift)
{
    return static_cast<TShortCut>(internal::ShortCut_Make(Key, static_cast<internal::int_t>(Shift)));
}

TShortCut TextToShortCut(const std::string& Text)
{
    return static_cast<TShortCut>(internal::ShortCut_FromText(Text.c_str()));
}

std::string ShortCutToText(TShortCut ShortCut)
{
    return std::string(internal::ShortCut_ToText(ShortCut));
}

TMenuItem::TMenuItem(TComponent* AOwner)
    : TMenuItem(internal::TMenuItem_Create(HandleOf(AOwner)))
{}

TMenuItem::TMenuItem(ObjectHandle handle)
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
    , Action(this, &TMenuItem::GetActionImpl, &TMenuItem::SetActionImpl)
    , bitmap_(this, &internal::TMenuItem_GetBitmap)
{}

TBasicAction* TMenuItem::GetActionImpl(TObject* owner)
{
    return static_cast<TBasicAction*>(FromHandle(internal::TMenuItem_GetAction(owner->Handle())));
}

void TMenuItem::SetActionImpl(TObject* owner, TBasicAction* const& value)
{
    internal::TMenuItem_SetAction(owner->Handle(), HandleOf(value));
}

TMenuItem* TMenuItem::GetItemsImpl(TObject* owner, int Index) { return WrapExisting<TMenuItem>(internal::TMenuItem_GetItem(owner->Handle(), Index)); }
void TMenuItem::Add(TMenuItem* Item)              { internal::TMenuItem_Add(handle_, HandleOf(Item)); }
void TMenuItem::Insert(int Index, TMenuItem* Item) { internal::TMenuItem_Insert(handle_, Index, HandleOf(Item)); }
void TMenuItem::Delete(int Index)                 { internal::TMenuItem_Delete(handle_, Index); }
void TMenuItem::Remove(TMenuItem* Item)           { internal::TMenuItem_Remove(handle_, HandleOf(Item)); }
void TMenuItem::Clear()                           { internal::TMenuItem_Clear(handle_); }
int  TMenuItem::IndexOf(TMenuItem* Item) const    { return internal::TMenuItem_IndexOf(handle_, HandleOf(Item)); }
void TMenuItem::AddSeparator()                    { internal::TMenuItem_AddSeparator(handle_); }
bool TMenuItem::IsLine() const                    { return internal::TMenuItem_IsLine(handle_) != 0; }
void TMenuItem::Click()                           { internal::TMenuItem_Click(handle_); }

std::string TMenuItem::GetCaptionImpl(TObject* owner) { return std::string(internal::TMenuItem_GetCaption(owner->Handle())); }
void TMenuItem::SetCaptionImpl(TObject* owner, const std::string& value) { internal::TMenuItem_SetCaption(owner->Handle(), value.c_str()); }
bool TMenuItem::GetCheckedImpl(TObject* owner)                     { return internal::TMenuItem_GetChecked(owner->Handle()) != 0; }
void TMenuItem::SetCheckedImpl(TObject* owner, const bool& value)   { internal::TMenuItem_SetChecked(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetEnabledImpl(TObject* owner)                     { return internal::TMenuItem_GetEnabled(owner->Handle()) != 0; }
void TMenuItem::SetEnabledImpl(TObject* owner, const bool& value)   { internal::TMenuItem_SetEnabled(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetVisibleImpl(TObject* owner)                     { return internal::TMenuItem_GetVisible(owner->Handle()) != 0; }
void TMenuItem::SetVisibleImpl(TObject* owner, const bool& value)   { internal::TMenuItem_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetAutoCheckImpl(TObject* owner)                   { return internal::TMenuItem_GetAutoCheck(owner->Handle()) != 0; }
void TMenuItem::SetAutoCheckImpl(TObject* owner, const bool& value) { internal::TMenuItem_SetAutoCheck(owner->Handle(), value ? 1 : 0); }
bool TMenuItem::GetRadioItemImpl(TObject* owner)                   { return internal::TMenuItem_GetRadioItem(owner->Handle()) != 0; }
void TMenuItem::SetRadioItemImpl(TObject* owner, const bool& value) { internal::TMenuItem_SetRadioItem(owner->Handle(), value ? 1 : 0); }
int  TMenuItem::GetGroupIndexImpl(TObject* owner)                  { return internal::TMenuItem_GetGroupIndex(owner->Handle()); }
void TMenuItem::SetGroupIndexImpl(TObject* owner, const int& value) { internal::TMenuItem_SetGroupIndex(owner->Handle(), value); }
bool TMenuItem::GetDefaultImpl(TObject* owner)                     { return internal::TMenuItem_GetDefault(owner->Handle()) != 0; }
void TMenuItem::SetDefaultImpl(TObject* owner, const bool& value)   { internal::TMenuItem_SetDefault(owner->Handle(), value ? 1 : 0); }
TShortCut TMenuItem::GetShortCutImpl(TObject* owner) { return static_cast<TShortCut>(internal::TMenuItem_GetShortCut(owner->Handle())); }
void TMenuItem::SetShortCutImpl(TObject* owner, const TShortCut& value) { internal::TMenuItem_SetShortCut(owner->Handle(), value); }
std::string TMenuItem::GetHintImpl(TObject* owner) { return std::string(internal::TMenuItem_GetHint(owner->Handle())); }
void TMenuItem::SetHintImpl(TObject* owner, const std::string& value) { internal::TMenuItem_SetHint(owner->Handle(), value.c_str()); }
int  TMenuItem::GetCountImpl(TObject* owner) { return internal::TMenuItem_GetCount(owner->Handle()); }
TMenuItem* TMenuItem::GetParentImpl(TObject* owner) { return WrapExisting<TMenuItem>(internal::TMenuItem_GetParent(owner->Handle())); }

TNotifyEvent TMenuItem::GetOnClickImpl(TObject* owner) { return static_cast<TMenuItem*>(owner)->onClick_; }

void TMenuItem::SetOnClickImpl(TObject* owner, const TNotifyEvent& value)
{
    TMenuItem* self = static_cast<TMenuItem*>(owner);
    SetSimpleEvent(self->handle_, self->onClick_, self->onClickHooked_, value,
                   &internal::TMenuItem_SetOnClick, &TMenuItem::ClickTrampoline);
}

void BETH_CALL TMenuItem::ClickTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TMenuItem* self = static_cast<TMenuItem*>(FromHandle(sender)))
            CallNotify(self->onClick_, self);
    });
}

TMenu::TMenu(ObjectHandle handle)
    : TComponent(handle)
    , Items(this, &TMenu::GetItemsImpl)
    , Images(this, &TMenu::GetImagesImpl, &TMenu::SetImagesImpl)
{}

TMenuItem* TMenu::GetItemsImpl(TObject* owner) { return WrapExisting<TMenuItem>(internal::TMenu_GetItems(owner->Handle())); }

TMainMenu::TMainMenu(TComponent* AOwner)
    : TMenu(internal::TMainMenu_Create(HandleOf(AOwner)))
{}

TPopupMenu::TPopupMenu(TComponent* AOwner)
    : TMenu(internal::TPopupMenu_Create(HandleOf(AOwner)))
    , AutoPopup(this, &TPopupMenu::GetAutoPopupImpl, &TPopupMenu::SetAutoPopupImpl)
    , PopupComponent(this, &TPopupMenu::GetPopupComponentImpl, &TPopupMenu::SetPopupComponentImpl)
    , OnPopup(this, &TPopupMenu::GetOnPopupImpl, &TPopupMenu::SetOnPopupImpl)
    , OnClose(this, &TPopupMenu::GetOnCloseImpl, &TPopupMenu::SetOnCloseImpl)
{}

void TPopupMenu::Popup(int X, int Y) { internal::TPopupMenu_Popup(handle_, X, Y); }

bool TPopupMenu::GetAutoPopupImpl(TObject* owner)                   { return internal::TPopupMenu_GetAutoPopup(owner->Handle()) != 0; }
void TPopupMenu::SetAutoPopupImpl(TObject* owner, const bool& value) { internal::TPopupMenu_SetAutoPopup(owner->Handle(), value ? 1 : 0); }
TComponent* TPopupMenu::GetPopupComponentImpl(TObject* owner) { return FromHandle(internal::TPopupMenu_GetPopupComponent(owner->Handle())); }
void TPopupMenu::SetPopupComponentImpl(TObject* owner, TComponent* const& value) { internal::TPopupMenu_SetPopupComponent(owner->Handle(), HandleOf(value)); }

TNotifyEvent TPopupMenu::GetOnPopupImpl(TObject* owner) { return static_cast<TPopupMenu*>(owner)->onPopup_; }
TNotifyEvent TPopupMenu::GetOnCloseImpl(TObject* owner) { return static_cast<TPopupMenu*>(owner)->onClose_; }

void TPopupMenu::SetOnPopupImpl(TObject* owner, const TNotifyEvent& value)
{
    TPopupMenu* self = static_cast<TPopupMenu*>(owner);
    SetSimpleEvent(self->handle_, self->onPopup_, self->onPopupHooked_, value,
                   &internal::TPopupMenu_SetOnPopup, &TPopupMenu::PopupTrampoline);
}

void TPopupMenu::SetOnCloseImpl(TObject* owner, const TNotifyEvent& value)
{
    TPopupMenu* self = static_cast<TPopupMenu*>(owner);
    SetSimpleEvent(self->handle_, self->onClose_, self->onCloseHooked_, value,
                   &internal::TPopupMenu_SetOnClose, &TPopupMenu::CloseTrampoline);
}

void BETH_CALL TPopupMenu::PopupTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TPopupMenu* self = static_cast<TPopupMenu*>(FromHandle(sender)))
            CallNotify(self->onPopup_, self);
    });
}

void BETH_CALL TPopupMenu::CloseTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        if (TPopupMenu* self = static_cast<TPopupMenu*>(FromHandle(sender)))
            CallNotify(self->onClose_, self);
    });
}

/* ---------------- Grid ---------------- */

void TCustomGrid::BeginUpdate() { internal::TCustomGrid_BeginUpdate(handle_); }
void TCustomGrid::EndUpdate()   { internal::TCustomGrid_EndUpdate(handle_); }
void TCustomGrid::Clear()       { internal::TCustomGrid_Clear(handle_); }

TRect TCustomGrid::CellRect(int ACol, int ARow) const
{
    TRect r{};
    internal::TCustomGrid_CellRect(handle_, ACol, ARow, &r.Left, &r.Top, &r.Right, &r.Bottom);
    return r;
}

void TCustomGrid::MouseToCell(int X, int Y, int& ACol, int& ARow) const
{
    internal::TCustomGrid_MouseToCell(handle_, X, Y, &ACol, &ARow);
}

TCustomDrawGrid::TCustomDrawGrid(ObjectHandle handle)
    : TCustomGrid(handle)
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

int  TCustomDrawGrid::GetColWidthsImpl(TObject* owner, int ACol)                     { return internal::TCustomDrawGrid_GetColWidths(owner->Handle(), ACol); }
void TCustomDrawGrid::SetColWidthsImpl(TObject* owner, int ACol, const int& value)   { internal::TCustomDrawGrid_SetColWidths(owner->Handle(), ACol, value); }
int  TCustomDrawGrid::GetRowHeightsImpl(TObject* owner, int ARow)                    { return internal::TCustomDrawGrid_GetRowHeights(owner->Handle(), ARow); }
void TCustomDrawGrid::SetRowHeightsImpl(TObject* owner, int ARow, const int& value)  { internal::TCustomDrawGrid_SetRowHeights(owner->Handle(), ARow, value); }

void TCustomDrawGrid::InsertColRow(bool IsColumn, int Index) { internal::TCustomDrawGrid_InsertColRow(handle_, IsColumn ? 1 : 0, Index); }
void TCustomDrawGrid::DeleteColRow(bool IsColumn, int Index) { internal::TCustomDrawGrid_DeleteColRow(handle_, IsColumn ? 1 : 0, Index); }
void TCustomDrawGrid::SortColRow(bool IsColumn, int Index)   { internal::TCustomDrawGrid_SortColRow(handle_, IsColumn ? 1 : 0, Index); }

void TCustomDrawGrid::MoveColRow(bool IsColumn, int FromIndex, int ToIndex)
{
    internal::TCustomDrawGrid_MoveColRow(handle_, IsColumn ? 1 : 0, FromIndex, ToIndex);
}

int  TCustomDrawGrid::GetColCountImpl(TObject* owner)                        { return internal::TCustomDrawGrid_GetColCount(owner->Handle()); }
void TCustomDrawGrid::SetColCountImpl(TObject* owner, const int& value)       { internal::TCustomDrawGrid_SetColCount(owner->Handle(), value); }
int  TCustomDrawGrid::GetRowCountImpl(TObject* owner)                        { return internal::TCustomDrawGrid_GetRowCount(owner->Handle()); }
void TCustomDrawGrid::SetRowCountImpl(TObject* owner, const int& value)       { internal::TCustomDrawGrid_SetRowCount(owner->Handle(), value); }
int  TCustomDrawGrid::GetFixedColsImpl(TObject* owner)                       { return internal::TCustomDrawGrid_GetFixedCols(owner->Handle()); }
void TCustomDrawGrid::SetFixedColsImpl(TObject* owner, const int& value)      { internal::TCustomDrawGrid_SetFixedCols(owner->Handle(), value); }
int  TCustomDrawGrid::GetFixedRowsImpl(TObject* owner)                       { return internal::TCustomDrawGrid_GetFixedRows(owner->Handle()); }
void TCustomDrawGrid::SetFixedRowsImpl(TObject* owner, const int& value)      { internal::TCustomDrawGrid_SetFixedRows(owner->Handle(), value); }
int  TCustomDrawGrid::GetColImpl(TObject* owner)                             { return internal::TCustomDrawGrid_GetCol(owner->Handle()); }
void TCustomDrawGrid::SetColImpl(TObject* owner, const int& value)            { internal::TCustomDrawGrid_SetCol(owner->Handle(), value); }
int  TCustomDrawGrid::GetRowImpl(TObject* owner)                             { return internal::TCustomDrawGrid_GetRow(owner->Handle()); }
void TCustomDrawGrid::SetRowImpl(TObject* owner, const int& value)            { internal::TCustomDrawGrid_SetRow(owner->Handle(), value); }
int  TCustomDrawGrid::GetDefaultColWidthImpl(TObject* owner)                 { return internal::TCustomDrawGrid_GetDefaultColWidth(owner->Handle()); }
void TCustomDrawGrid::SetDefaultColWidthImpl(TObject* owner, const int& value) { internal::TCustomDrawGrid_SetDefaultColWidth(owner->Handle(), value); }
int  TCustomDrawGrid::GetDefaultRowHeightImpl(TObject* owner)                { return internal::TCustomDrawGrid_GetDefaultRowHeight(owner->Handle()); }
void TCustomDrawGrid::SetDefaultRowHeightImpl(TObject* owner, const int& value) { internal::TCustomDrawGrid_SetDefaultRowHeight(owner->Handle(), value); }
TGridOptions TCustomDrawGrid::GetOptionsImpl(TObject* owner)                  { return internal::TCustomDrawGrid_GetOptions(owner->Handle()); }
void TCustomDrawGrid::SetOptionsImpl(TObject* owner, const TGridOptions& value) { internal::TCustomDrawGrid_SetOptions(owner->Handle(), value); }
int  TCustomDrawGrid::GetLeftColImpl(TObject* owner)                         { return internal::TCustomDrawGrid_GetLeftCol(owner->Handle()); }
void TCustomDrawGrid::SetLeftColImpl(TObject* owner, const int& value)        { internal::TCustomDrawGrid_SetLeftCol(owner->Handle(), value); }
int  TCustomDrawGrid::GetTopRowImpl(TObject* owner)                          { return internal::TCustomDrawGrid_GetTopRow(owner->Handle()); }
void TCustomDrawGrid::SetTopRowImpl(TObject* owner, const int& value)         { internal::TCustomDrawGrid_SetTopRow(owner->Handle(), value); }
bool TCustomDrawGrid::GetDefaultDrawingImpl(TObject* owner)                  { return internal::TCustomDrawGrid_GetDefaultDrawing(owner->Handle()) != 0; }
void TCustomDrawGrid::SetDefaultDrawingImpl(TObject* owner, const bool& value) { internal::TCustomDrawGrid_SetDefaultDrawing(owner->Handle(), value ? 1 : 0); }
TColor TCustomDrawGrid::GetFixedColorImpl(TObject* owner)                    { return internal::TCustomDrawGrid_GetFixedColor(owner->Handle()); }
void TCustomDrawGrid::SetFixedColorImpl(TObject* owner, const TColor& value)  { internal::TCustomDrawGrid_SetFixedColor(owner->Handle(), value); }
bool TCustomDrawGrid::GetEditorModeImpl(TObject* owner)                      { return internal::TCustomDrawGrid_GetEditorMode(owner->Handle()) != 0; }
void TCustomDrawGrid::SetEditorModeImpl(TObject* owner, const bool& value)    { internal::TCustomDrawGrid_SetEditorMode(owner->Handle(), value ? 1 : 0); }

TGridRect TCustomDrawGrid::GetSelectionImpl(TObject* owner)
{
    TGridRect r{};
    internal::TCustomDrawGrid_GetSelection(owner->Handle(), &r.Left, &r.Top, &r.Right, &r.Bottom);
    return r;
}

void TCustomDrawGrid::SetSelectionImpl(TObject* owner, const TGridRect& value)
{
    internal::TCustomDrawGrid_SetSelection(owner->Handle(), value.Left, value.Top, value.Right, value.Bottom);
}

void BETH_CALL TCustomDrawGrid::DrawCellTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row,
                                                     internal::int_t left, internal::int_t top, internal::int_t right, internal::int_t bottom,
                                                     internal::uint_t state, void*)
{
    GuardCallback([&] {
        TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
        if (!self || !self->onDrawCell_)
            return;
        TOnDrawCell handler = self->onDrawCell_;
        handler(self, col, row, TRect{left, top, right, bottom}, state);
    });
}

void BETH_CALL TCustomDrawGrid::SelectCellTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::bool_t* canSelect, void*)
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

void BETH_CALL TCustomDrawGrid::SelectionTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, void*)
{
    GuardCallback([&] {
        TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(FromHandle(sender));
        if (!self || !self->onSelection_)
            return;
        TOnSelectEvent handler = self->onSelection_;
        handler(self, col, row);
    });
}

void BETH_CALL TCustomDrawGrid::HeaderClickTrampoline(ObjectHandle sender, internal::int_t isColumn, internal::int_t index, void*)
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
                   &internal::TCustomDrawGrid_SetOnDrawCell, &TCustomDrawGrid::DrawCellTrampoline);
}

void TCustomDrawGrid::SetOnSelectCellImpl(TObject* owner, const TOnSelectCellEvent& value)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(owner);
    SetSimpleEvent(self->handle_, self->onSelectCell_, self->onSelectCellHooked_, value,
                   &internal::TCustomDrawGrid_SetOnSelectCell, &TCustomDrawGrid::SelectCellTrampoline);
}

void TCustomDrawGrid::SetOnSelectionImpl(TObject* owner, const TOnSelectEvent& value)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(owner);
    SetSimpleEvent(self->handle_, self->onSelection_, self->onSelectionHooked_, value,
                   &internal::TCustomDrawGrid_SetOnSelection, &TCustomDrawGrid::SelectionTrampoline);
}

void TCustomDrawGrid::SetOnHeaderClickImpl(TObject* owner, const THdrEvent& value)
{
    TCustomDrawGrid* self = static_cast<TCustomDrawGrid*>(owner);
    SetSimpleEvent(self->handle_, self->onHeaderClick_, self->onHeaderClickHooked_, value,
                   &internal::TCustomDrawGrid_SetOnHeaderClick, &TCustomDrawGrid::HeaderClickTrampoline);
}

TDrawGrid::TDrawGrid(TComponent* AOwner)
    : TCustomDrawGrid(internal::TDrawGrid_Create(HandleOf(AOwner)))
{}

TCustomStringGrid::TCustomStringGrid(ObjectHandle handle)
    : TCustomDrawGrid(handle)
    , Cells(this, &TCustomStringGrid::GetCellsImpl, &TCustomStringGrid::SetCellsImpl)
{}

std::string TCustomStringGrid::GetCellsImpl(TObject* owner, int ACol, int ARow)
{
    return std::string(internal::TCustomStringGrid_GetCells(owner->Handle(), ACol, ARow));
}

void TCustomStringGrid::SetCellsImpl(TObject* owner, int ACol, int ARow, const std::string& value)
{
    internal::TCustomStringGrid_SetCells(owner->Handle(), ACol, ARow, value.c_str());
}

void TCustomStringGrid::Clean()                   { internal::TCustomStringGrid_Clean(handle_); }
void TCustomStringGrid::AutoSizeColumns()         { internal::TCustomStringGrid_AutoSizeColumns(handle_); }
void TCustomStringGrid::AutoSizeColumn(int ACol)  { internal::TCustomStringGrid_AutoSizeColumn(handle_, ACol); }

TStringGrid::TStringGrid(TComponent* AOwner)
    : TCustomStringGrid(internal::TStringGrid_Create(HandleOf(AOwner)))
{}

/* ---------------- HeaderControl ---------------- */

THeaderSection::THeaderSection(ObjectHandle handle)
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

std::string THeaderSection::GetTextImpl(TObject* owner) { return std::string(internal::THeaderSection_GetText(owner->Handle())); }
void THeaderSection::SetTextImpl(TObject* owner, const std::string& value) { internal::THeaderSection_SetText(owner->Handle(), value.c_str()); }
int  THeaderSection::GetWidthImpl(TObject* owner)                     { return internal::THeaderSection_GetWidth(owner->Handle()); }
void THeaderSection::SetWidthImpl(TObject* owner, const int& value)    { internal::THeaderSection_SetWidth(owner->Handle(), value); }
int  THeaderSection::GetMinWidthImpl(TObject* owner)                  { return internal::THeaderSection_GetMinWidth(owner->Handle()); }
void THeaderSection::SetMinWidthImpl(TObject* owner, const int& value) { internal::THeaderSection_SetMinWidth(owner->Handle(), value); }
int  THeaderSection::GetMaxWidthImpl(TObject* owner)                  { return internal::THeaderSection_GetMaxWidth(owner->Handle()); }
void THeaderSection::SetMaxWidthImpl(TObject* owner, const int& value) { internal::THeaderSection_SetMaxWidth(owner->Handle(), value); }
TAlignment THeaderSection::GetAlignmentImpl(TObject* owner) { return static_cast<TAlignment>(internal::THeaderSection_GetAlignment(owner->Handle())); }
void THeaderSection::SetAlignmentImpl(TObject* owner, const TAlignment& value) { internal::THeaderSection_SetAlignment(owner->Handle(), value); }
bool THeaderSection::GetVisibleImpl(TObject* owner)                   { return internal::THeaderSection_GetVisible(owner->Handle()) != 0; }
void THeaderSection::SetVisibleImpl(TObject* owner, const bool& value) { internal::THeaderSection_SetVisible(owner->Handle(), value ? 1 : 0); }
int  THeaderSection::GetIndexImpl(TObject* owner)                     { return internal::THeaderSection_GetIndex(owner->Handle()); }
void THeaderSection::SetIndexImpl(TObject* owner, const int& value)    { internal::THeaderSection_SetIndex(owner->Handle(), value); }
int  THeaderSection::GetLeftImpl(TObject* owner)                      { return internal::THeaderSection_GetLeft(owner->Handle()); }
int  THeaderSection::GetRightImpl(TObject* owner)                     { return internal::THeaderSection_GetRight(owner->Handle()); }
int  THeaderSection::GetOriginalIndexImpl(TObject* owner)             { return internal::THeaderSection_GetOriginalIndex(owner->Handle()); }

THeaderSections::THeaderSections(ObjectHandle handle)
    : TPersistent(handle)
    , Count(this, &THeaderSections::GetCountImpl)
    , Items(this, &THeaderSections::GetItemsImpl)
{}

THeaderSection* THeaderSections::Add()             { return THeaderSection::Wrap(internal::THeaderSections_Add(handle_)); }
THeaderSection* THeaderSections::Insert(int Index) { return THeaderSection::Wrap(internal::THeaderSections_Insert(handle_, Index)); }
void THeaderSections::Delete(int Index)            { internal::THeaderSections_Delete(handle_, Index); }
void THeaderSections::Clear()                      { internal::THeaderSections_Clear(handle_); }
void THeaderSections::BeginUpdate()                { internal::THeaderSections_BeginUpdate(handle_); }
void THeaderSections::EndUpdate()                  { internal::THeaderSections_EndUpdate(handle_); }
THeaderSection* THeaderSections::GetItemsImpl(TObject* owner, int Index) { return THeaderSection::Wrap(internal::THeaderSections_GetItem(owner->Handle(), Index)); }
int  THeaderSections::GetCountImpl(TObject* owner) { return internal::THeaderSections_GetCount(owner->Handle()); }

TCustomHeaderControl::TCustomHeaderControl(ObjectHandle handle)
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
    , sections_(internal::TCustomHeaderControl_GetSections(handle_))
{}

int TCustomHeaderControl::GetSectionAt(const TPoint& P) const
{
    return internal::TCustomHeaderControl_GetSectionAt(handle_, P.X, P.Y);
}

THeaderSections* TCustomHeaderControl::GetSectionsImpl(TObject* owner) { return &static_cast<TCustomHeaderControl*>(owner)->sections_; }
bool TCustomHeaderControl::GetDragReorderImpl(TObject* owner) { return internal::TCustomHeaderControl_GetDragReorder(owner->Handle()) != 0; }
void TCustomHeaderControl::SetDragReorderImpl(TObject* owner, const bool& value) { internal::TCustomHeaderControl_SetDragReorder(owner->Handle(), value ? 1 : 0); }
THeaderSection* TCustomHeaderControl::GetSectionFromOriginalIndexImpl(TObject* owner, int OriginalIndex)
{
    return THeaderSection::Wrap(internal::TCustomHeaderControl_GetSectionFromOriginalIndex(owner->Handle(), OriginalIndex));
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

void BETH_CALL TCustomHeaderControl::SectionClickTrampoline(ObjectHandle sender, ObjectHandle section, void*)
{
    GuardCallback([&] {
        if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
            CallSectionNotify(self, self->onSectionClick_, THeaderSection::Wrap(section));
    });
}

void BETH_CALL TCustomHeaderControl::SectionResizeTrampoline(ObjectHandle sender, ObjectHandle section, void*)
{
    GuardCallback([&] {
        if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
            CallSectionNotify(self, self->onSectionResize_, THeaderSection::Wrap(section));
    });
}

void BETH_CALL TCustomHeaderControl::SectionSeparatorDblClickTrampoline(ObjectHandle sender, ObjectHandle section, void*)
{
    GuardCallback([&] {
        if (TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender)))
            CallSectionNotify(self, self->onSectionSeparatorDblClick_, THeaderSection::Wrap(section));
    });
}

void BETH_CALL TCustomHeaderControl::SectionTrackTrampoline(ObjectHandle sender, ObjectHandle section,
                                                              internal::int_t width, internal::int_t state, void*)
{
    GuardCallback([&] {
        TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(FromHandle(sender));
        if (!self || !self->onSectionTrack_)
            return;
        TCustomSectionTrackEvent handler = self->onSectionTrack_;
        handler(self, THeaderSection::Wrap(section), width, static_cast<TSectionTrackState>(state));
    });
}

void BETH_CALL TCustomHeaderControl::SectionDragTrampoline(ObjectHandle sender, ObjectHandle fromSection,
                                                             ObjectHandle toSection, internal::bool_t* allow, void*)
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

void BETH_CALL TCustomHeaderControl::SectionEndDragTrampoline(ObjectHandle sender, void*)
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
                   &internal::TCustomHeaderControl_SetOnSectionClick, &TCustomHeaderControl::SectionClickTrampoline);
}

void TCustomHeaderControl::SetOnSectionResizeImpl(TObject* owner, const TCustomSectionNotifyEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionResize_, self->onSectionResizeHooked_, value,
                   &internal::TCustomHeaderControl_SetOnSectionResize, &TCustomHeaderControl::SectionResizeTrampoline);
}

void TCustomHeaderControl::SetOnSectionSeparatorDblClickImpl(TObject* owner, const TCustomSectionNotifyEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionSeparatorDblClick_, self->onSectionSeparatorDblClickHooked_, value,
                   &internal::TCustomHeaderControl_SetOnSectionSeparatorDblClick, &TCustomHeaderControl::SectionSeparatorDblClickTrampoline);
}

void TCustomHeaderControl::SetOnSectionTrackImpl(TObject* owner, const TCustomSectionTrackEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionTrack_, self->onSectionTrackHooked_, value,
                   &internal::TCustomHeaderControl_SetOnSectionTrack, &TCustomHeaderControl::SectionTrackTrampoline);
}

void TCustomHeaderControl::SetOnSectionDragImpl(TObject* owner, const TSectionDragEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionDrag_, self->onSectionDragHooked_, value,
                   &internal::TCustomHeaderControl_SetOnSectionDrag, &TCustomHeaderControl::SectionDragTrampoline);
}

void TCustomHeaderControl::SetOnSectionEndDragImpl(TObject* owner, const TNotifyEvent& value)
{
    TCustomHeaderControl* self = static_cast<TCustomHeaderControl*>(owner);
    SetSimpleEvent(self->handle_, self->onSectionEndDrag_, self->onSectionEndDragHooked_, value,
                   &internal::TCustomHeaderControl_SetOnSectionEndDrag, &TCustomHeaderControl::SectionEndDragTrampoline);
}

THeaderControl::THeaderControl(TComponent* AOwner)
    : TCustomHeaderControl(internal::THeaderControl_Create(HandleOf(AOwner)))
{}

/* ---------------- ToolBar ---------------- */

TToolWindow::TToolWindow(ObjectHandle handle)
    : TCustomControl(handle)
    , EdgeBorders(this, &TToolWindow::GetEdgeBordersImpl, &TToolWindow::SetEdgeBordersImpl)
    , EdgeInner(this, &TToolWindow::GetEdgeInnerImpl, &TToolWindow::SetEdgeInnerImpl)
    , EdgeOuter(this, &TToolWindow::GetEdgeOuterImpl, &TToolWindow::SetEdgeOuterImpl)
{}

void TToolWindow::BeginUpdate() { internal::TToolWindow_BeginUpdate(handle_); }
void TToolWindow::EndUpdate()   { internal::TToolWindow_EndUpdate(handle_); }
TEdgeBorders TToolWindow::GetEdgeBordersImpl(TObject* owner) { return internal::TToolWindow_GetEdgeBorders(owner->Handle()); }
void TToolWindow::SetEdgeBordersImpl(TObject* owner, const TEdgeBorders& value) { internal::TToolWindow_SetEdgeBorders(owner->Handle(), value); }
TEdgeStyle TToolWindow::GetEdgeInnerImpl(TObject* owner) { return static_cast<TEdgeStyle>(internal::TToolWindow_GetEdgeInner(owner->Handle())); }
void TToolWindow::SetEdgeInnerImpl(TObject* owner, const TEdgeStyle& value) { internal::TToolWindow_SetEdgeInner(owner->Handle(), value); }
TEdgeStyle TToolWindow::GetEdgeOuterImpl(TObject* owner) { return static_cast<TEdgeStyle>(internal::TToolWindow_GetEdgeOuter(owner->Handle())); }
void TToolWindow::SetEdgeOuterImpl(TObject* owner, const TEdgeStyle& value) { internal::TToolWindow_SetEdgeOuter(owner->Handle(), value); }

TToolBar::TToolBar(TComponent* AOwner)
    : TToolWindow(internal::TToolBar_Create(HandleOf(AOwner)))
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
    internal::TToolBar_SetButtonSize(handle_, NewButtonWidth, NewButtonHeight);
}

int  TToolBar::GetButtonCountImpl(TObject* owner) { return internal::TToolBar_GetButtonCount(owner->Handle()); }
// ボタンは利用者が生成したコンポーネントなので、ラッパーは必ずある。
TToolButton* TToolBar::GetButtonsImpl(TObject* owner, int Index)
{
    return static_cast<TToolButton*>(FromHandle(internal::TToolBar_GetButton(owner->Handle(), Index)));
}
int  TToolBar::GetRowCountImpl(TObject* owner)                         { return internal::TToolBar_GetRowCount(owner->Handle()); }
int  TToolBar::GetButtonHeightImpl(TObject* owner)                     { return internal::TToolBar_GetButtonHeight(owner->Handle()); }
void TToolBar::SetButtonHeightImpl(TObject* owner, const int& value)    { internal::TToolBar_SetButtonHeight(owner->Handle(), value); }
int  TToolBar::GetButtonWidthImpl(TObject* owner)                      { return internal::TToolBar_GetButtonWidth(owner->Handle()); }
void TToolBar::SetButtonWidthImpl(TObject* owner, const int& value)     { internal::TToolBar_SetButtonWidth(owner->Handle(), value); }
int  TToolBar::GetDropDownWidthImpl(TObject* owner)                    { return internal::TToolBar_GetDropDownWidth(owner->Handle()); }
void TToolBar::SetDropDownWidthImpl(TObject* owner, const int& value)   { internal::TToolBar_SetDropDownWidth(owner->Handle(), value); }
int  TToolBar::GetIndentImpl(TObject* owner)                           { return internal::TToolBar_GetIndent(owner->Handle()); }
void TToolBar::SetIndentImpl(TObject* owner, const int& value)          { internal::TToolBar_SetIndent(owner->Handle(), value); }
bool TToolBar::GetFlatImpl(TObject* owner)                             { return internal::TToolBar_GetFlat(owner->Handle()) != 0; }
void TToolBar::SetFlatImpl(TObject* owner, const bool& value)           { internal::TToolBar_SetFlat(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetListImpl(TObject* owner)                             { return internal::TToolBar_GetList(owner->Handle()) != 0; }
void TToolBar::SetListImpl(TObject* owner, const bool& value)           { internal::TToolBar_SetList(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetShowCaptionsImpl(TObject* owner)                     { return internal::TToolBar_GetShowCaptions(owner->Handle()) != 0; }
void TToolBar::SetShowCaptionsImpl(TObject* owner, const bool& value)   { internal::TToolBar_SetShowCaptions(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetTransparentImpl(TObject* owner)                      { return internal::TToolBar_GetTransparent(owner->Handle()) != 0; }
void TToolBar::SetTransparentImpl(TObject* owner, const bool& value)    { internal::TToolBar_SetTransparent(owner->Handle(), value ? 1 : 0); }
bool TToolBar::GetWrapableImpl(TObject* owner)                         { return internal::TToolBar_GetWrapable(owner->Handle()) != 0; }
void TToolBar::SetWrapableImpl(TObject* owner, const bool& value)       { internal::TToolBar_SetWrapable(owner->Handle(), value ? 1 : 0); }

TToolButton::TToolButton(TComponent* AOwner)
    : TGraphicControl(internal::TToolButton_Create(HandleOf(AOwner)))
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

void TToolButton::Click()      { internal::TToolButton_Click(handle_); }
void TToolButton::ArrowClick() { internal::TToolButton_ArrowClick(handle_); }
bool TToolButton::PointInArrow(int X, int Y) const { return internal::TToolButton_PointInArrow(handle_, X, Y) != 0; }

bool TToolButton::GetAllowAllUpImpl(TObject* owner)                      { return internal::TToolButton_GetAllowAllUp(owner->Handle()) != 0; }
void TToolButton::SetAllowAllUpImpl(TObject* owner, const bool& value)    { internal::TToolButton_SetAllowAllUp(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetDownImpl(TObject* owner)                            { return internal::TToolButton_GetDown(owner->Handle()) != 0; }
void TToolButton::SetDownImpl(TObject* owner, const bool& value)          { internal::TToolButton_SetDown(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetGroupedImpl(TObject* owner)                         { return internal::TToolButton_GetGrouped(owner->Handle()) != 0; }
void TToolButton::SetGroupedImpl(TObject* owner, const bool& value)       { internal::TToolButton_SetGrouped(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetIndeterminateImpl(TObject* owner)                   { return internal::TToolButton_GetIndeterminate(owner->Handle()) != 0; }
void TToolButton::SetIndeterminateImpl(TObject* owner, const bool& value) { internal::TToolButton_SetIndeterminate(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetMarkedImpl(TObject* owner)                          { return internal::TToolButton_GetMarked(owner->Handle()) != 0; }
void TToolButton::SetMarkedImpl(TObject* owner, const bool& value)        { internal::TToolButton_SetMarked(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetShowCaptionImpl(TObject* owner)                     { return internal::TToolButton_GetShowCaption(owner->Handle()) != 0; }
void TToolButton::SetShowCaptionImpl(TObject* owner, const bool& value)   { internal::TToolButton_SetShowCaption(owner->Handle(), value ? 1 : 0); }
bool TToolButton::GetWrapImpl(TObject* owner)                            { return internal::TToolButton_GetWrap(owner->Handle()) != 0; }
void TToolButton::SetWrapImpl(TObject* owner, const bool& value)          { internal::TToolButton_SetWrap(owner->Handle(), value ? 1 : 0); }
TToolButtonStyle TToolButton::GetStyleImpl(TObject* owner) { return static_cast<TToolButtonStyle>(internal::TToolButton_GetStyle(owner->Handle())); }
void TToolButton::SetStyleImpl(TObject* owner, const TToolButtonStyle& value) { internal::TToolButton_SetStyle(owner->Handle(), value); }
TPopupMenu* TToolButton::GetDropdownMenuImpl(TObject* owner)
{
    return static_cast<TPopupMenu*>(FromHandle(internal::TToolButton_GetDropdownMenu(owner->Handle())));
}
void TToolButton::SetDropdownMenuImpl(TObject* owner, TPopupMenu* const& value) { internal::TToolButton_SetDropdownMenu(owner->Handle(), HandleOf(value)); }
// メニュー項目は LCL が内部で生成したもの(メニューのルート項目等)もありうるため WrapExisting で引く。
TMenuItem* TToolButton::GetMenuItemImpl(TObject* owner) { return WrapExisting<TMenuItem>(internal::TToolButton_GetMenuItem(owner->Handle())); }
void TToolButton::SetMenuItemImpl(TObject* owner, TMenuItem* const& value) { internal::TToolButton_SetMenuItem(owner->Handle(), HandleOf(value)); }
int  TToolButton::GetIndexImpl(TObject* owner) { return internal::TToolButton_GetIndex(owner->Handle()); }

void BETH_CALL TToolButton::ArrowClickTrampoline(ObjectHandle sender, void*)
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
                   &internal::TToolButton_SetOnArrowClick, &TToolButton::ArrowClickTrampoline);
}

/* ---------------- CoolBar ---------------- */

TCoolBand::TCoolBand(ObjectHandle handle)
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
    , bitmap_(this, &internal::TCoolBand_GetBitmap)
{}

void TCoolBand::AutosizeWidth() { internal::TCoolBand_AutosizeWidth(handle_); }

std::string TCoolBand::GetTextImpl(TObject* owner) { return std::string(internal::TCoolBand_GetText(owner->Handle())); }
void TCoolBand::SetTextImpl(TObject* owner, const std::string& value) { internal::TCoolBand_SetText(owner->Handle(), value.c_str()); }
int  TCoolBand::GetWidthImpl(TObject* owner)                            { return internal::TCoolBand_GetWidth(owner->Handle()); }
void TCoolBand::SetWidthImpl(TObject* owner, const int& value)           { internal::TCoolBand_SetWidth(owner->Handle(), value); }
int  TCoolBand::GetMinWidthImpl(TObject* owner)                         { return internal::TCoolBand_GetMinWidth(owner->Handle()); }
void TCoolBand::SetMinWidthImpl(TObject* owner, const int& value)        { internal::TCoolBand_SetMinWidth(owner->Handle(), value); }
int  TCoolBand::GetMinHeightImpl(TObject* owner)                        { return internal::TCoolBand_GetMinHeight(owner->Handle()); }
void TCoolBand::SetMinHeightImpl(TObject* owner, const int& value)       { internal::TCoolBand_SetMinHeight(owner->Handle(), value); }
bool TCoolBand::GetBreakImpl(TObject* owner)                            { return internal::TCoolBand_GetBreak(owner->Handle()) != 0; }
void TCoolBand::SetBreakImpl(TObject* owner, const bool& value)          { internal::TCoolBand_SetBreak(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetVisibleImpl(TObject* owner)                          { return internal::TCoolBand_GetVisible(owner->Handle()) != 0; }
void TCoolBand::SetVisibleImpl(TObject* owner, const bool& value)        { internal::TCoolBand_SetVisible(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetFixedSizeImpl(TObject* owner)                        { return internal::TCoolBand_GetFixedSize(owner->Handle()) != 0; }
void TCoolBand::SetFixedSizeImpl(TObject* owner, const bool& value)      { internal::TCoolBand_SetFixedSize(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetFixedBackgroundImpl(TObject* owner)                  { return internal::TCoolBand_GetFixedBackground(owner->Handle()) != 0; }
void TCoolBand::SetFixedBackgroundImpl(TObject* owner, const bool& value) { internal::TCoolBand_SetFixedBackground(owner->Handle(), value ? 1 : 0); }
bool TCoolBand::GetHorizontalOnlyImpl(TObject* owner)                   { return internal::TCoolBand_GetHorizontalOnly(owner->Handle()) != 0; }
void TCoolBand::SetHorizontalOnlyImpl(TObject* owner, const bool& value) { internal::TCoolBand_SetHorizontalOnly(owner->Handle(), value ? 1 : 0); }
TColor TCoolBand::GetColorImpl(TObject* owner)                          { return static_cast<TColor>(internal::TCoolBand_GetColor(owner->Handle())); }
void TCoolBand::SetColorImpl(TObject* owner, const TColor& value)        { internal::TCoolBand_SetColor(owner->Handle(), static_cast<internal::int_t>(value)); }
bool TCoolBand::GetParentColorImpl(TObject* owner)                      { return internal::TCoolBand_GetParentColor(owner->Handle()) != 0; }
void TCoolBand::SetParentColorImpl(TObject* owner, const bool& value)    { internal::TCoolBand_SetParentColor(owner->Handle(), value ? 1 : 0); }
int  TCoolBand::GetIndexImpl(TObject* owner)                            { return internal::TCoolBand_GetIndex(owner->Handle()); }
void TCoolBand::SetIndexImpl(TObject* owner, const int& value)           { internal::TCoolBand_SetIndex(owner->Handle(), value); }
// バンドに置くコントロールは利用者が生成したコンポーネントなので、ラッパーは必ずある。
TControl* TCoolBand::GetControlImpl(TObject* owner)
{
    return static_cast<TControl*>(TControl::FromHandle(internal::TCoolBand_GetControl(owner->Handle())));
}
void TCoolBand::SetControlImpl(TObject* owner, TControl* const& value)   { internal::TCoolBand_SetControl(owner->Handle(), value ? value->Handle() : nullptr); }
int  TCoolBand::GetLeftImpl(TObject* owner)                             { return internal::TCoolBand_GetLeft(owner->Handle()); }
int  TCoolBand::GetTopImpl(TObject* owner)                              { return internal::TCoolBand_GetTop(owner->Handle()); }
int  TCoolBand::GetRightImpl(TObject* owner)                            { return internal::TCoolBand_GetRight(owner->Handle()); }
int  TCoolBand::GetHeightImpl(TObject* owner)                           { return internal::TCoolBand_GetHeight(owner->Handle()); }

TCoolBands::TCoolBands(ObjectHandle handle)
    : TPersistent(handle)
    , Count(this, &TCoolBands::GetCountImpl)
    , Items(this, &TCoolBands::GetItemsImpl)
{}

TCoolBand* TCoolBands::Add()          { return TCoolBand::Wrap(internal::TCoolBands_Add(handle_)); }
void TCoolBands::Delete(int Index)    { internal::TCoolBands_Delete(handle_, Index); }
void TCoolBands::Clear()              { internal::TCoolBands_Clear(handle_); }
void TCoolBands::BeginUpdate()        { internal::TCoolBands_BeginUpdate(handle_); }
void TCoolBands::EndUpdate()          { internal::TCoolBands_EndUpdate(handle_); }
TCoolBand* TCoolBands::FindBand(TControl* AControl) const { return TCoolBand::Wrap(internal::TCoolBands_FindBand(handle_, AControl ? AControl->Handle() : nullptr)); }
int  TCoolBands::FindBandIndex(TControl* AControl) const  { return internal::TCoolBands_FindBandIndex(handle_, AControl ? AControl->Handle() : nullptr); }
TCoolBand* TCoolBands::GetItemsImpl(TObject* owner, int Index) { return TCoolBand::Wrap(internal::TCoolBands_GetItem(owner->Handle(), Index)); }
int  TCoolBands::GetCountImpl(TObject* owner) { return internal::TCoolBands_GetCount(owner->Handle()); }

TCustomCoolBar::TCustomCoolBar(ObjectHandle handle)
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
    , bands_(internal::TCustomCoolBar_GetBands(handle_))
    , bitmap_(this, &internal::TCustomCoolBar_GetBitmap)
{}

void TCustomCoolBar::AutosizeBands() { internal::TCustomCoolBar_AutosizeBands(handle_); }

void TCustomCoolBar::MouseToBandPos(int X, int Y, int& ABand, bool& AGrabber) const
{
    internal::int_t band = -1;
    internal::bool_t grabber = 0;
    internal::TCustomCoolBar_MouseToBandPos(handle_, X, Y, &band, &grabber);
    ABand = band;
    AGrabber = grabber != 0;
}

TCoolBands* TCustomCoolBar::GetBandsImpl(TObject* owner) { return &static_cast<TCustomCoolBar*>(owner)->bands_; }
bool TCustomCoolBar::GetFixedSizeImpl(TObject* owner)                        { return internal::TCustomCoolBar_GetFixedSize(owner->Handle()) != 0; }
void TCustomCoolBar::SetFixedSizeImpl(TObject* owner, const bool& value)      { internal::TCustomCoolBar_SetFixedSize(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetFixedOrderImpl(TObject* owner)                       { return internal::TCustomCoolBar_GetFixedOrder(owner->Handle()) != 0; }
void TCustomCoolBar::SetFixedOrderImpl(TObject* owner, const bool& value)     { internal::TCustomCoolBar_SetFixedOrder(owner->Handle(), value ? 1 : 0); }
TGrabStyle TCustomCoolBar::GetGrabStyleImpl(TObject* owner) { return static_cast<TGrabStyle>(internal::TCustomCoolBar_GetGrabStyle(owner->Handle())); }
void TCustomCoolBar::SetGrabStyleImpl(TObject* owner, const TGrabStyle& value) { internal::TCustomCoolBar_SetGrabStyle(owner->Handle(), value); }
int  TCustomCoolBar::GetGrabWidthImpl(TObject* owner)                        { return internal::TCustomCoolBar_GetGrabWidth(owner->Handle()); }
void TCustomCoolBar::SetGrabWidthImpl(TObject* owner, const int& value)       { internal::TCustomCoolBar_SetGrabWidth(owner->Handle(), value); }
int  TCustomCoolBar::GetHorizontalSpacingImpl(TObject* owner)                { return internal::TCustomCoolBar_GetHorizontalSpacing(owner->Handle()); }
void TCustomCoolBar::SetHorizontalSpacingImpl(TObject* owner, const int& value) { internal::TCustomCoolBar_SetHorizontalSpacing(owner->Handle(), value); }
int  TCustomCoolBar::GetVerticalSpacingImpl(TObject* owner)                  { return internal::TCustomCoolBar_GetVerticalSpacing(owner->Handle()); }
void TCustomCoolBar::SetVerticalSpacingImpl(TObject* owner, const int& value) { internal::TCustomCoolBar_SetVerticalSpacing(owner->Handle(), value); }
bool TCustomCoolBar::GetShowTextImpl(TObject* owner)                         { return internal::TCustomCoolBar_GetShowText(owner->Handle()) != 0; }
void TCustomCoolBar::SetShowTextImpl(TObject* owner, const bool& value)       { internal::TCustomCoolBar_SetShowText(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetThemedImpl(TObject* owner)                           { return internal::TCustomCoolBar_GetThemed(owner->Handle()) != 0; }
void TCustomCoolBar::SetThemedImpl(TObject* owner, const bool& value)         { internal::TCustomCoolBar_SetThemed(owner->Handle(), value ? 1 : 0); }
bool TCustomCoolBar::GetVerticalImpl(TObject* owner)                         { return internal::TCustomCoolBar_GetVertical(owner->Handle()) != 0; }
void TCustomCoolBar::SetVerticalImpl(TObject* owner, const bool& value)       { internal::TCustomCoolBar_SetVertical(owner->Handle(), value ? 1 : 0); }

void BETH_CALL TCustomCoolBar::ChangeTrampoline(ObjectHandle sender, void*)
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
                   &internal::TCustomCoolBar_SetOnChange, &TCustomCoolBar::ChangeTrampoline);
}

TCoolBar::TCoolBar(TComponent* AOwner)
    : TCustomCoolBar(internal::TCoolBar_Create(HandleOf(AOwner)))
{}


/* ---------------- Images・ImageIndex・Bitmap(docs/adr/0030) ---------------- */

TCustomImageList* TCustomImage::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TCustomImage_GetImages(owner->Handle()))); }
void TCustomImage::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TCustomImage_SetImages(owner->Handle(), HandleOf(value)); }
int  TCustomImage::GetImageIndexImpl(TObject* owner) { return internal::TCustomImage_GetImageIndex(owner->Handle()); }
void TCustomImage::SetImageIndexImpl(TObject* owner, const int& value) { internal::TCustomImage_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomBitBtn::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TCustomBitBtn_GetImages(owner->Handle()))); }
void TCustomBitBtn::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TCustomBitBtn_SetImages(owner->Handle(), HandleOf(value)); }
int  TCustomBitBtn::GetImageIndexImpl(TObject* owner) { return internal::TCustomBitBtn_GetImageIndex(owner->Handle()); }
void TCustomBitBtn::SetImageIndexImpl(TObject* owner, const int& value) { internal::TCustomBitBtn_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomSpeedButton::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TCustomSpeedButton_GetImages(owner->Handle()))); }
void TCustomSpeedButton::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TCustomSpeedButton_SetImages(owner->Handle(), HandleOf(value)); }
int  TCustomSpeedButton::GetImageIndexImpl(TObject* owner) { return internal::TCustomSpeedButton_GetImageIndex(owner->Handle()); }
void TCustomSpeedButton::SetImageIndexImpl(TObject* owner, const int& value) { internal::TCustomSpeedButton_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomTabControl::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TCustomTabControl_GetImages(owner->Handle()))); }
void TCustomTabControl::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TCustomTabControl_SetImages(owner->Handle(), HandleOf(value)); }

int  TCustomPage::GetImageIndexImpl(TObject* owner) { return internal::TCustomPage_GetImageIndex(owner->Handle()); }
void TCustomPage::SetImageIndexImpl(TObject* owner, const int& value) { internal::TCustomPage_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomTreeView::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TCustomTreeView_GetImages(owner->Handle()))); }
void TCustomTreeView::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TCustomTreeView_SetImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TCustomTreeView::GetStateImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TCustomTreeView_GetStateImages(owner->Handle()))); }
void TCustomTreeView::SetStateImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TCustomTreeView_SetStateImages(owner->Handle(), HandleOf(value)); }

int  TTreeNode::GetImageIndexImpl(TObject* owner) { return internal::TTreeNode_GetImageIndex(owner->Handle()); }
void TTreeNode::SetImageIndexImpl(TObject* owner, const int& value) { internal::TTreeNode_SetImageIndex(owner->Handle(), value); }
int  TTreeNode::GetSelectedIndexImpl(TObject* owner) { return internal::TTreeNode_GetSelectedIndex(owner->Handle()); }
void TTreeNode::SetSelectedIndexImpl(TObject* owner, const int& value) { internal::TTreeNode_SetSelectedIndex(owner->Handle(), value); }
int  TTreeNode::GetStateIndexImpl(TObject* owner) { return internal::TTreeNode_GetStateIndex(owner->Handle()); }
void TTreeNode::SetStateIndexImpl(TObject* owner, const int& value) { internal::TTreeNode_SetStateIndex(owner->Handle(), value); }
int  TTreeNode::GetOverlayIndexImpl(TObject* owner) { return internal::TTreeNode_GetOverlayIndex(owner->Handle()); }
void TTreeNode::SetOverlayIndexImpl(TObject* owner, const int& value) { internal::TTreeNode_SetOverlayIndex(owner->Handle(), value); }

TCustomImageList* TListView::GetLargeImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TListView_GetLargeImages(owner->Handle()))); }
void TListView::SetLargeImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TListView_SetLargeImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TListView::GetSmallImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TListView_GetSmallImages(owner->Handle()))); }
void TListView::SetSmallImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TListView_SetSmallImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TListView::GetStateImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TListView_GetStateImages(owner->Handle()))); }
void TListView::SetStateImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TListView_SetStateImages(owner->Handle(), HandleOf(value)); }

int  TListItem::GetImageIndexImpl(TObject* owner) { return internal::TListItem_GetImageIndex(owner->Handle()); }
void TListItem::SetImageIndexImpl(TObject* owner, const int& value) { internal::TListItem_SetImageIndex(owner->Handle(), value); }
int  TListItem::GetStateIndexImpl(TObject* owner) { return internal::TListItem_GetStateIndex(owner->Handle()); }
void TListItem::SetStateIndexImpl(TObject* owner, const int& value) { internal::TListItem_SetStateIndex(owner->Handle(), value); }

int  TListColumn::GetImageIndexImpl(TObject* owner) { return internal::TListColumn_GetImageIndex(owner->Handle()); }
void TListColumn::SetImageIndexImpl(TObject* owner, const int& value) { internal::TListColumn_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TToolBar::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TToolBar_GetImages(owner->Handle()))); }
void TToolBar::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TToolBar_SetImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TToolBar::GetHotImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TToolBar_GetHotImages(owner->Handle()))); }
void TToolBar::SetHotImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TToolBar_SetHotImages(owner->Handle(), HandleOf(value)); }
TCustomImageList* TToolBar::GetDisabledImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TToolBar_GetDisabledImages(owner->Handle()))); }
void TToolBar::SetDisabledImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TToolBar_SetDisabledImages(owner->Handle(), HandleOf(value)); }

int  TToolButton::GetImageIndexImpl(TObject* owner) { return internal::TToolButton_GetImageIndex(owner->Handle()); }
void TToolButton::SetImageIndexImpl(TObject* owner, const int& value) { internal::TToolButton_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomHeaderControl::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TCustomHeaderControl_GetImages(owner->Handle()))); }
void TCustomHeaderControl::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TCustomHeaderControl_SetImages(owner->Handle(), HandleOf(value)); }

int  THeaderSection::GetImageIndexImpl(TObject* owner) { return internal::THeaderSection_GetImageIndex(owner->Handle()); }
void THeaderSection::SetImageIndexImpl(TObject* owner, const int& value) { internal::THeaderSection_SetImageIndex(owner->Handle(), value); }

TCustomImageList* TCustomCoolBar::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TCustomCoolBar_GetImages(owner->Handle()))); }
void TCustomCoolBar::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TCustomCoolBar_SetImages(owner->Handle(), HandleOf(value)); }
TBitmap* TCustomCoolBar::GetBitmapImpl(TObject* owner) { return &static_cast<TCustomCoolBar*>(owner)->bitmap_; }
void TCustomCoolBar::SetBitmapImpl(TObject* owner, TBitmap* const& value)
{
    internal::TCustomCoolBar_SetBitmap(owner->Handle(), value ? value->Current() : nullptr);
}

int  TCoolBand::GetImageIndexImpl(TObject* owner) { return internal::TCoolBand_GetImageIndex(owner->Handle()); }
void TCoolBand::SetImageIndexImpl(TObject* owner, const int& value) { internal::TCoolBand_SetImageIndex(owner->Handle(), value); }
TBitmap* TCoolBand::GetBitmapImpl(TObject* owner) { return &static_cast<TCoolBand*>(owner)->bitmap_; }
void TCoolBand::SetBitmapImpl(TObject* owner, TBitmap* const& value)
{
    internal::TCoolBand_SetBitmap(owner->Handle(), value ? value->Current() : nullptr);
}

TCustomImageList* TMenu::GetImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TMenu_GetImages(owner->Handle()))); }
void TMenu::SetImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TMenu_SetImages(owner->Handle(), HandleOf(value)); }

int  TMenuItem::GetImageIndexImpl(TObject* owner) { return internal::TMenuItem_GetImageIndex(owner->Handle()); }
void TMenuItem::SetImageIndexImpl(TObject* owner, const int& value) { internal::TMenuItem_SetImageIndex(owner->Handle(), value); }
TCustomImageList* TMenuItem::GetSubMenuImagesImpl(TObject* owner) { return static_cast<TCustomImageList*>(FromHandle(internal::TMenuItem_GetSubMenuImages(owner->Handle()))); }
void TMenuItem::SetSubMenuImagesImpl(TObject* owner, TCustomImageList* const& value) { internal::TMenuItem_SetSubMenuImages(owner->Handle(), HandleOf(value)); }
TBitmap* TMenuItem::GetBitmapImpl(TObject* owner) { return &static_cast<TMenuItem*>(owner)->bitmap_; }
void TMenuItem::SetBitmapImpl(TObject* owner, TBitmap* const& value)
{
    internal::TMenuItem_SetBitmap(owner->Handle(), value ? value->Current() : nullptr);
}


/* ---------------- Dialogs ---------------- */

TCommonDialog::TCommonDialog(ObjectHandle handle)
    : TComponent(handle)
    , Title(this, &TCommonDialog::GetTitleImpl, &TCommonDialog::SetTitleImpl)
    , OnShow(this, &TCommonDialog::GetOnShowImpl, &TCommonDialog::SetOnShowImpl)
    , OnClose(this, &TCommonDialog::GetOnCloseImpl, &TCommonDialog::SetOnCloseImpl)
    , OnCanClose(this, &TCommonDialog::GetOnCanCloseImpl, &TCommonDialog::SetOnCanCloseImpl)
{}

bool TCommonDialog::Execute() { return internal::TCommonDialog_Execute(handle_) != 0; }

std::string TCommonDialog::GetTitleImpl(TObject* owner)                            { return std::string(internal::TCommonDialog_GetTitle(owner->Handle())); }
void        TCommonDialog::SetTitleImpl(TObject* owner, const std::string& value) { internal::TCommonDialog_SetTitle(owner->Handle(), value.c_str()); }

TNotifyEvent     TCommonDialog::GetOnShowImpl(TObject* owner)     { return static_cast<TCommonDialog*>(owner)->onShow_; }
TNotifyEvent     TCommonDialog::GetOnCloseImpl(TObject* owner)    { return static_cast<TCommonDialog*>(owner)->onClose_; }
TCloseQueryEvent TCommonDialog::GetOnCanCloseImpl(TObject* owner) { return static_cast<TCommonDialog*>(owner)->onCanClose_; }

void TCommonDialog::SetOnShowImpl(TObject* owner, const TNotifyEvent& value)
{
    TCommonDialog* self = static_cast<TCommonDialog*>(owner);
    SetSimpleEvent(self->handle_, self->onShow_, self->onShowHooked_, value,
                   &internal::TCommonDialog_SetOnShow, &TCommonDialog::ShowTrampoline);
}

void TCommonDialog::SetOnCloseImpl(TObject* owner, const TNotifyEvent& value)
{
    TCommonDialog* self = static_cast<TCommonDialog*>(owner);
    SetSimpleEvent(self->handle_, self->onClose_, self->onCloseHooked_, value,
                   &internal::TCommonDialog_SetOnClose, &TCommonDialog::CloseTrampoline);
}

void TCommonDialog::SetOnCanCloseImpl(TObject* owner, const TCloseQueryEvent& value)
{
    TCommonDialog* self = static_cast<TCommonDialog*>(owner);
    SetSimpleEvent(self->handle_, self->onCanClose_, self->onCanCloseHooked_, value,
                   &internal::TCommonDialog_SetOnCanClose, &TCommonDialog::CanCloseTrampoline);
}

void BETH_CALL TCommonDialog::ShowTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCommonDialog* self = static_cast<TCommonDialog*>(FromHandle(sender));
        if (self)
            CallNotify(TNotifyEvent(self->onShow_), self);
    });
}

void BETH_CALL TCommonDialog::CloseTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TCommonDialog* self = static_cast<TCommonDialog*>(FromHandle(sender));
        if (self)
            CallNotify(TNotifyEvent(self->onClose_), self);
    });
}

void BETH_CALL TCommonDialog::CanCloseTrampoline(ObjectHandle sender, internal::bool_t* canClose, void*)
{
    GuardCallback([&] {
        TCommonDialog* self = static_cast<TCommonDialog*>(FromHandle(sender));
        if (!self || !self->onCanClose_)
            return;
        TCloseQueryEvent handler = self->onCanClose_;
        bool value = *canClose != 0;
        handler(self, value);
        *canClose = value ? 1 : 0;
    });
}

TFileDialog::TFileDialog(ObjectHandle handle)
    : TCommonDialog(handle)
    , FileName(this, &TFileDialog::GetFileNameImpl, &TFileDialog::SetFileNameImpl)
    , Filter(this, &TFileDialog::GetFilterImpl, &TFileDialog::SetFilterImpl)
    , FilterIndex(this, &TFileDialog::GetFilterIndexImpl, &TFileDialog::SetFilterIndexImpl)
    , InitialDir(this, &TFileDialog::GetInitialDirImpl, &TFileDialog::SetInitialDirImpl)
    , DefaultExt(this, &TFileDialog::GetDefaultExtImpl, &TFileDialog::SetDefaultExtImpl)
    , Files(this, &TFileDialog::GetFilesImpl)
    , files_(this, &internal::TFileDialog_GetFiles)
{}

std::string TFileDialog::GetFileNameImpl(TObject* owner)                              { return std::string(internal::TFileDialog_GetFileName(owner->Handle())); }
void        TFileDialog::SetFileNameImpl(TObject* owner, const std::string& value)   { internal::TFileDialog_SetFileName(owner->Handle(), value.c_str()); }
std::string TFileDialog::GetFilterImpl(TObject* owner)                                { return std::string(internal::TFileDialog_GetFilter(owner->Handle())); }
void        TFileDialog::SetFilterImpl(TObject* owner, const std::string& value)     { internal::TFileDialog_SetFilter(owner->Handle(), value.c_str()); }
int         TFileDialog::GetFilterIndexImpl(TObject* owner)                           { return internal::TFileDialog_GetFilterIndex(owner->Handle()); }
void        TFileDialog::SetFilterIndexImpl(TObject* owner, const int& value)        { internal::TFileDialog_SetFilterIndex(owner->Handle(), value); }
std::string TFileDialog::GetInitialDirImpl(TObject* owner)                            { return std::string(internal::TFileDialog_GetInitialDir(owner->Handle())); }
void        TFileDialog::SetInitialDirImpl(TObject* owner, const std::string& value) { internal::TFileDialog_SetInitialDir(owner->Handle(), value.c_str()); }
std::string TFileDialog::GetDefaultExtImpl(TObject* owner)                            { return std::string(internal::TFileDialog_GetDefaultExt(owner->Handle())); }
void        TFileDialog::SetDefaultExtImpl(TObject* owner, const std::string& value) { internal::TFileDialog_SetDefaultExt(owner->Handle(), value.c_str()); }
TStrings*   TFileDialog::GetFilesImpl(TObject* owner)                                 { return &static_cast<TFileDialog*>(owner)->files_; }

TOpenDialog::TOpenDialog(TComponent* AOwner)
    : TOpenDialog(internal::TOpenDialog_Create(HandleOf(AOwner)), DerivedTag())
{}

TOpenDialog::TOpenDialog(ObjectHandle handle, DerivedTag)
    : TFileDialog(handle)
    , Options(this, &TOpenDialog::GetOptionsImpl, &TOpenDialog::SetOptionsImpl)
{}

TOpenOptions TOpenDialog::GetOptionsImpl(TObject* owner)                         { return internal::TOpenDialog_GetOptions(owner->Handle()); }
void         TOpenDialog::SetOptionsImpl(TObject* owner, const TOpenOptions& value) { internal::TOpenDialog_SetOptions(owner->Handle(), value); }

TSaveDialog::TSaveDialog(TComponent* AOwner)
    : TOpenDialog(internal::TSaveDialog_Create(HandleOf(AOwner)), DerivedTag())
{}

TSelectDirectoryDialog::TSelectDirectoryDialog(TComponent* AOwner)
    : TOpenDialog(internal::TSelectDirectoryDialog_Create(HandleOf(AOwner)), DerivedTag())
{}

TColorDialog::TColorDialog(TComponent* AOwner)
    : TCommonDialog(internal::TColorDialog_Create(HandleOf(AOwner)))
    , Color(this, &TColorDialog::GetColorImpl, &TColorDialog::SetColorImpl)
    , CustomColors(this, &TColorDialog::GetCustomColorsImpl)
    , Options(this, &TColorDialog::GetOptionsImpl, &TColorDialog::SetOptionsImpl)
    , customColors_(this, &internal::TColorDialog_GetCustomColors)
{}

TColor              TColorDialog::GetColorImpl(TObject* owner)                                { return static_cast<TColor>(internal::TColorDialog_GetColor(owner->Handle())); }
void                TColorDialog::SetColorImpl(TObject* owner, const TColor& value)           { internal::TColorDialog_SetColor(owner->Handle(), value); }
TStrings*           TColorDialog::GetCustomColorsImpl(TObject* owner)                         { return &static_cast<TColorDialog*>(owner)->customColors_; }
TColorDialogOptions TColorDialog::GetOptionsImpl(TObject* owner)                              { return internal::TColorDialog_GetOptions(owner->Handle()); }
void                TColorDialog::SetOptionsImpl(TObject* owner, const TColorDialogOptions& value) { internal::TColorDialog_SetOptions(owner->Handle(), value); }

TFontDialog::TFontDialog(TComponent* AOwner)
    : TCommonDialog(internal::TFontDialog_Create(HandleOf(AOwner)))
    , Font(this, &TFontDialog::GetFontImpl, &TFontDialog::SetFontImpl)
    , MinFontSize(this, &TFontDialog::GetMinFontSizeImpl, &TFontDialog::SetMinFontSizeImpl)
    , MaxFontSize(this, &TFontDialog::GetMaxFontSizeImpl, &TFontDialog::SetMaxFontSizeImpl)
    , Options(this, &TFontDialog::GetOptionsImpl, &TFontDialog::SetOptionsImpl)
    , font_(internal::TFontDialog_GetFont(handle_))
{}

TFont*             TFontDialog::GetFontImpl(TObject* owner)                                  { return &static_cast<TFontDialog*>(owner)->font_; }
void               TFontDialog::SetFontImpl(TObject* owner, TFont* const& value)             { internal::TFontDialog_SetFont(owner->Handle(), value ? value->Handle() : nullptr); }
int                TFontDialog::GetMinFontSizeImpl(TObject* owner)                           { return internal::TFontDialog_GetMinFontSize(owner->Handle()); }
void               TFontDialog::SetMinFontSizeImpl(TObject* owner, const int& value)         { internal::TFontDialog_SetMinFontSize(owner->Handle(), value); }
int                TFontDialog::GetMaxFontSizeImpl(TObject* owner)                           { return internal::TFontDialog_GetMaxFontSize(owner->Handle()); }
void               TFontDialog::SetMaxFontSizeImpl(TObject* owner, const int& value)         { internal::TFontDialog_SetMaxFontSize(owner->Handle(), value); }
TFontDialogOptions TFontDialog::GetOptionsImpl(TObject* owner)                               { return internal::TFontDialog_GetOptions(owner->Handle()); }
void               TFontDialog::SetOptionsImpl(TObject* owner, const TFontDialogOptions& value) { internal::TFontDialog_SetOptions(owner->Handle(), value); }

TFindDialog::TFindDialog(TComponent* AOwner)
    : TFindDialog(internal::TFindDialog_Create(HandleOf(AOwner)), DerivedTag())
{}

TFindDialog::TFindDialog(ObjectHandle handle, DerivedTag)
    : TCommonDialog(handle)
    , FindText(this, &TFindDialog::GetFindTextImpl, &TFindDialog::SetFindTextImpl)
    , Options(this, &TFindDialog::GetOptionsImpl, &TFindDialog::SetOptionsImpl)
    , Left(this, &TFindDialog::GetLeftImpl, &TFindDialog::SetLeftImpl)
    , Top(this, &TFindDialog::GetTopImpl, &TFindDialog::SetTopImpl)
    , OnFind(this, &TFindDialog::GetOnFindImpl, &TFindDialog::SetOnFindImpl)
    , ReplaceText(this, &TFindDialog::GetReplaceTextImpl, &TFindDialog::SetReplaceTextImpl)
    , OnReplace(this, &TFindDialog::GetOnReplaceImpl, &TFindDialog::SetOnReplaceImpl)
{}

void TFindDialog::CloseDialog() { internal::TFindDialog_CloseDialog(handle_); }

std::string  TFindDialog::GetFindTextImpl(TObject* owner)                               { return std::string(internal::TFindDialog_GetFindText(owner->Handle())); }
void         TFindDialog::SetFindTextImpl(TObject* owner, const std::string& value)    { internal::TFindDialog_SetFindText(owner->Handle(), value.c_str()); }
std::string  TFindDialog::GetReplaceTextImpl(TObject* owner)                            { return std::string(internal::TFindDialog_GetReplaceText(owner->Handle())); }
void         TFindDialog::SetReplaceTextImpl(TObject* owner, const std::string& value) { internal::TFindDialog_SetReplaceText(owner->Handle(), value.c_str()); }
TFindOptions TFindDialog::GetOptionsImpl(TObject* owner)                                { return internal::TFindDialog_GetOptions(owner->Handle()); }
void         TFindDialog::SetOptionsImpl(TObject* owner, const TFindOptions& value)    { internal::TFindDialog_SetOptions(owner->Handle(), value); }
int          TFindDialog::GetLeftImpl(TObject* owner)                                   { return internal::TFindDialog_GetLeft(owner->Handle()); }
void         TFindDialog::SetLeftImpl(TObject* owner, const int& value)                { internal::TFindDialog_SetLeft(owner->Handle(), value); }
int          TFindDialog::GetTopImpl(TObject* owner)                                    { return internal::TFindDialog_GetTop(owner->Handle()); }
void         TFindDialog::SetTopImpl(TObject* owner, const int& value)                 { internal::TFindDialog_SetTop(owner->Handle(), value); }

TNotifyEvent TFindDialog::GetOnFindImpl(TObject* owner)    { return static_cast<TFindDialog*>(owner)->onFind_; }
TNotifyEvent TFindDialog::GetOnReplaceImpl(TObject* owner) { return static_cast<TFindDialog*>(owner)->onReplace_; }

void TFindDialog::SetOnFindImpl(TObject* owner, const TNotifyEvent& value)
{
    TFindDialog* self = static_cast<TFindDialog*>(owner);
    SetSimpleEvent(self->handle_, self->onFind_, self->onFindHooked_, value,
                   &internal::TFindDialog_SetOnFind, &TFindDialog::FindTrampoline);
}

void TFindDialog::SetOnReplaceImpl(TObject* owner, const TNotifyEvent& value)
{
    TFindDialog* self = static_cast<TFindDialog*>(owner);
    SetSimpleEvent(self->handle_, self->onReplace_, self->onReplaceHooked_, value,
                   &internal::TFindDialog_SetOnReplace, &TFindDialog::ReplaceTrampoline);
}

void BETH_CALL TFindDialog::FindTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TFindDialog* self = static_cast<TFindDialog*>(FromHandle(sender));
        if (self)
            CallNotify(TNotifyEvent(self->onFind_), self);
    });
}

void BETH_CALL TFindDialog::ReplaceTrampoline(ObjectHandle sender, void*)
{
    GuardCallback([&] {
        TFindDialog* self = static_cast<TFindDialog*>(FromHandle(sender));
        if (self)
            CallNotify(TNotifyEvent(self->onReplace_), self);
    });
}

TReplaceDialog::TReplaceDialog(TComponent* AOwner)
    : TFindDialog(internal::TReplaceDialog_Create(HandleOf(AOwner)), DerivedTag())
{}

} // namespace beth
