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

/* TButton */
typedef no_vcl_obj_t (NO_VCL_CALL *TButton_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TButton_Destroy)(no_vcl_obj_t Obj);

typedef no_vcl_str_t (NO_VCL_CALL *TButton_GetCaption)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TButton_SetCaption)(no_vcl_obj_t Obj, no_vcl_str_t Caption);

/* thread_local 関数ポインタ */
thread_local TForm_Create       TForm_Create_           = nullptr;
thread_local TForm_Destroy      TForm_Destroy_          = nullptr;
thread_local TForm_GetCaption   TForm_GetCaption_       = nullptr;
thread_local TForm_SetCaption   TForm_SetCaption_       = nullptr;
thread_local TForm_GetWidth     TForm_GetWidth_         = nullptr;
thread_local TForm_SetWidth     TForm_SetWidth_         = nullptr;
thread_local TForm_GetHeight    TForm_GetHeight_        = nullptr;
thread_local TForm_SetHeight    TForm_SetHeight_        = nullptr;

thread_local TButton_Create     TButton_Create_         = nullptr;
thread_local TButton_Destroy    TButton_Destroy_        = nullptr;
thread_local TButton_GetCaption TButton_GetCaption_     = nullptr;
thread_local TButton_SetCaption TButton_SetCaption_     = nullptr;

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

    NO_VCL_MAP(m, TButton_Create);
    NO_VCL_MAP(m, TButton_Destroy);
    NO_VCL_MAP(m, TButton_GetCaption);
    NO_VCL_MAP(m, TButton_SetCaption);
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

} // extern "C"
