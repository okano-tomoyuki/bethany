#include <thread>
#include <mutex>
#include <cassert>
#include <stdexcept>

#include <windows.h>
#include "no_vcl_impl.h"

namespace
{

/* TForm */
typedef no_vcl_obj_t (NO_VCL_CALL *TForm_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TForm_Destroy)(no_vcl_obj_t Obj);

typedef no_vcl_str_t (NO_VCL_CALL *TForm_GetCaption)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TForm_SetCaption)(no_vcl_obj_t Obj, no_vcl_str_t Caption);

typedef no_vcl_int_t (NO_VCL_CALL *TForm_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TForm_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TForm_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TForm_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef void         (NO_VCL_CALL *TForm_Show)(no_vcl_obj_t Obj);
typedef no_vcl_int_t (NO_VCL_CALL *TForm_ShowModal)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TForm_Hide)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TForm_Close)(no_vcl_obj_t Obj);

/* Application */
typedef void         (NO_VCL_CALL *Application_Run)(void);
typedef void         (NO_VCL_CALL *Application_ProcessMessages)(void);

/* TButton */
typedef no_vcl_obj_t (NO_VCL_CALL *TButton_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TButton_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TButton_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_str_t (NO_VCL_CALL *TButton_GetCaption)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TButton_SetCaption)(no_vcl_obj_t Obj, no_vcl_str_t Caption);

typedef no_vcl_int_t (NO_VCL_CALL *TButton_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TButton_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TButton_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TButton_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TButton_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TButton_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TButton_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TButton_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef void         (NO_VCL_CALL *TButton_SetOnClick)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* thread_local 関数ポインタ */
thread_local TForm_Create       TForm_Create_           = nullptr;
thread_local TForm_Destroy      TForm_Destroy_          = nullptr;
thread_local TForm_GetCaption   TForm_GetCaption_       = nullptr;
thread_local TForm_SetCaption   TForm_SetCaption_       = nullptr;
thread_local TForm_GetWidth     TForm_GetWidth_         = nullptr;
thread_local TForm_SetWidth     TForm_SetWidth_         = nullptr;
thread_local TForm_GetHeight    TForm_GetHeight_        = nullptr;
thread_local TForm_SetHeight    TForm_SetHeight_        = nullptr;

thread_local TForm_Show         TForm_Show_             = nullptr;
thread_local TForm_ShowModal    TForm_ShowModal_        = nullptr;
thread_local TForm_Hide         TForm_Hide_             = nullptr;
thread_local TForm_Close        TForm_Close_            = nullptr;

thread_local Application_Run                Application_Run_                = nullptr;
thread_local Application_ProcessMessages    Application_ProcessMessages_    = nullptr;

thread_local TButton_Create     TButton_Create_         = nullptr;
thread_local TButton_Destroy    TButton_Destroy_        = nullptr;
thread_local TButton_SetParent  TButton_SetParent_      = nullptr;
thread_local TButton_GetCaption TButton_GetCaption_     = nullptr;
thread_local TButton_SetCaption TButton_SetCaption_     = nullptr;
thread_local TButton_GetLeft    TButton_GetLeft_        = nullptr;
thread_local TButton_SetLeft    TButton_SetLeft_        = nullptr;
thread_local TButton_GetTop     TButton_GetTop_         = nullptr;
thread_local TButton_SetTop     TButton_SetTop_         = nullptr;
thread_local TButton_GetWidth   TButton_GetWidth_       = nullptr;
thread_local TButton_SetWidth   TButton_SetWidth_       = nullptr;
thread_local TButton_GetHeight  TButton_GetHeight_      = nullptr;
thread_local TButton_SetHeight  TButton_SetHeight_      = nullptr;
thread_local TButton_SetOnClick TButton_SetOnClick_     = nullptr;

/* 関数ポインタマッピング */
template<typename Func>
void no_vcl_map(Func& f, HMODULE m, const char* n)
{
    f = reinterpret_cast<Func>(::GetProcAddress(m, n));
}

#define NO_VCL_MAP(m, f) \
    do { no_vcl_map(f##_, m, #f); } while (0)

void no_vcl_init(void)
{
    static HMODULE m = nullptr;
    static std::once_flag once;

    if (!m) 
    {
        std::call_once(once, [&](){
            m = ::LoadLibraryA("no_vcl.dll");
        });
    }

    NO_VCL_MAP(m, TForm_Create);
    NO_VCL_MAP(m, TForm_Destroy);
    NO_VCL_MAP(m, TForm_GetCaption);
    NO_VCL_MAP(m, TForm_SetCaption);
    NO_VCL_MAP(m, TForm_GetWidth);
    NO_VCL_MAP(m, TForm_SetWidth);
    NO_VCL_MAP(m, TForm_GetHeight);
    NO_VCL_MAP(m, TForm_SetHeight);

    NO_VCL_MAP(m, TForm_Show);
    NO_VCL_MAP(m, TForm_ShowModal);
    NO_VCL_MAP(m, TForm_Hide);
    NO_VCL_MAP(m, TForm_Close);

    NO_VCL_MAP(m, Application_Run);
    NO_VCL_MAP(m, Application_ProcessMessages);

    NO_VCL_MAP(m, TButton_Create);
    NO_VCL_MAP(m, TButton_Destroy);
    NO_VCL_MAP(m, TButton_SetParent);
    NO_VCL_MAP(m, TButton_GetCaption);
    NO_VCL_MAP(m, TButton_SetCaption);
    NO_VCL_MAP(m, TButton_GetLeft);
    NO_VCL_MAP(m, TButton_SetLeft);
    NO_VCL_MAP(m, TButton_GetTop);
    NO_VCL_MAP(m, TButton_SetTop);
    NO_VCL_MAP(m, TButton_GetWidth);
    NO_VCL_MAP(m, TButton_SetWidth);
    NO_VCL_MAP(m, TButton_GetHeight);
    NO_VCL_MAP(m, TButton_SetHeight);
    NO_VCL_MAP(m, TButton_SetOnClick);
}

#define NO_VCL_INIT_CHECK(f) \
    do { \
        if (!f) \
        { \
            no_vcl_init(); \
        } \
        assert(f != nullptr); \
    } while (0)

} // namespace

extern "C" {

no_vcl_obj_t NO_VCL_CALL no_vcl_TForm_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TForm_Create_);
    return TForm_Create_(owner);
}

void NO_VCL_CALL no_vcl_TForm_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_Destroy_);
    TForm_Destroy_(obj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TForm_GetCaption(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_GetCaption_);
    return TForm_GetCaption_(obj);
}

void NO_VCL_CALL no_vcl_TForm_SetCaption(no_vcl_obj_t obj, no_vcl_str_t cap)
{
    NO_VCL_INIT_CHECK(TForm_SetCaption_);
    TForm_SetCaption_(obj, cap);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TForm_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_GetWidth_);
    return TForm_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TForm_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TForm_SetWidth_);
    TForm_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TForm_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_GetHeight_);
    return TForm_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TForm_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TForm_SetHeight_);
    TForm_SetHeight_(obj, height);
}

void NO_VCL_CALL no_vcl_TForm_Show(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_Show_);
    TForm_Show_(obj);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TForm_ShowModal(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_ShowModal_);
    return TForm_ShowModal_(obj);
}

void NO_VCL_CALL no_vcl_TForm_Hide(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_Hide_);
    TForm_Hide_(obj);
}

void NO_VCL_CALL no_vcl_TForm_Close(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_Close_);
    TForm_Close_(obj);
}

void NO_VCL_CALL no_vcl_Application_Run(void)
{
    NO_VCL_INIT_CHECK(Application_Run_);
    Application_Run_();
}

void NO_VCL_CALL no_vcl_Application_ProcessMessages(void)
{
    NO_VCL_INIT_CHECK(Application_ProcessMessages_);
    Application_ProcessMessages_();
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TButton_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TButton_Create_);
    return TButton_Create_(owner);
}

void NO_VCL_CALL no_vcl_TButton_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TButton_Destroy_);
    TButton_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TButton_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TButton_SetParent_);
    TButton_SetParent_(obj, parentObj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TButton_GetCaption(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TButton_GetCaption_);
    return TButton_GetCaption_(obj);
}

void NO_VCL_CALL no_vcl_TButton_SetCaption(no_vcl_obj_t obj, no_vcl_str_t cap)
{
    NO_VCL_INIT_CHECK(TButton_SetCaption_);
    TButton_SetCaption_(obj, cap);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TButton_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TButton_GetLeft_);
    return TButton_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TButton_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TButton_SetLeft_);
    TButton_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TButton_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TButton_GetTop_);
    return TButton_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TButton_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TButton_SetTop_);
    TButton_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TButton_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TButton_GetWidth_);
    return TButton_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TButton_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TButton_SetWidth_);
    TButton_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TButton_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TButton_GetHeight_);
    return TButton_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TButton_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TButton_SetHeight_);
    TButton_SetHeight_(obj, height);
}

void NO_VCL_CALL no_vcl_TButton_SetOnClick(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TButton_SetOnClick_);
    TButton_SetOnClick_(obj, cb);
}

} // extern "C"
