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

typedef no_vcl_bool_t (NO_VCL_CALL *TForm_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TForm_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TForm_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TForm_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

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

typedef no_vcl_bool_t (NO_VCL_CALL *TButton_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TButton_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TButton_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TButton_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef void         (NO_VCL_CALL *TButton_SetOnClick)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TLabel */
typedef no_vcl_obj_t (NO_VCL_CALL *TLabel_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TLabel_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TLabel_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_str_t (NO_VCL_CALL *TLabel_GetCaption)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TLabel_SetCaption)(no_vcl_obj_t Obj, no_vcl_str_t Caption);

typedef no_vcl_int_t (NO_VCL_CALL *TLabel_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TLabel_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TLabel_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TLabel_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TLabel_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TLabel_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TLabel_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TLabel_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TLabel_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TLabel_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TLabel_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TLabel_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* TEdit */
typedef no_vcl_obj_t (NO_VCL_CALL *TEdit_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TEdit_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TEdit_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_str_t (NO_VCL_CALL *TEdit_GetText)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TEdit_SetText)(no_vcl_obj_t Obj, no_vcl_str_t Text);

typedef no_vcl_int_t (NO_VCL_CALL *TEdit_GetMaxLength)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TEdit_SetMaxLength)(no_vcl_obj_t Obj, no_vcl_int_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TEdit_GetReadOnly)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TEdit_SetReadOnly)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_int_t (NO_VCL_CALL *TEdit_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TEdit_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TEdit_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TEdit_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TEdit_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TEdit_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TEdit_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TEdit_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TEdit_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TEdit_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TEdit_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TEdit_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef void         (NO_VCL_CALL *TEdit_SetOnChange)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TCheckBox */
typedef no_vcl_obj_t (NO_VCL_CALL *TCheckBox_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TCheckBox_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TCheckBox_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_str_t (NO_VCL_CALL *TCheckBox_GetCaption)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TCheckBox_SetCaption)(no_vcl_obj_t Obj, no_vcl_str_t Caption);

typedef no_vcl_bool_t (NO_VCL_CALL *TCheckBox_GetChecked)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TCheckBox_SetChecked)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_int_t (NO_VCL_CALL *TCheckBox_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TCheckBox_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TCheckBox_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TCheckBox_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TCheckBox_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TCheckBox_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TCheckBox_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TCheckBox_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TCheckBox_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TCheckBox_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TCheckBox_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TCheckBox_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef void         (NO_VCL_CALL *TCheckBox_SetOnClick)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TRadioButton */
typedef no_vcl_obj_t (NO_VCL_CALL *TRadioButton_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TRadioButton_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TRadioButton_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_str_t (NO_VCL_CALL *TRadioButton_GetCaption)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TRadioButton_SetCaption)(no_vcl_obj_t Obj, no_vcl_str_t Caption);

typedef no_vcl_bool_t (NO_VCL_CALL *TRadioButton_GetChecked)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TRadioButton_SetChecked)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_int_t (NO_VCL_CALL *TRadioButton_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TRadioButton_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TRadioButton_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TRadioButton_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TRadioButton_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TRadioButton_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TRadioButton_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TRadioButton_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TRadioButton_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TRadioButton_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TRadioButton_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TRadioButton_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef void         (NO_VCL_CALL *TRadioButton_SetOnClick)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* thread_local 関数ポインタ */
thread_local TForm_Create       TForm_Create_           = nullptr;
thread_local TForm_Destroy      TForm_Destroy_          = nullptr;
thread_local TForm_GetCaption   TForm_GetCaption_       = nullptr;
thread_local TForm_SetCaption   TForm_SetCaption_       = nullptr;
thread_local TForm_GetWidth     TForm_GetWidth_         = nullptr;
thread_local TForm_SetWidth     TForm_SetWidth_         = nullptr;
thread_local TForm_GetHeight    TForm_GetHeight_        = nullptr;
thread_local TForm_SetHeight    TForm_SetHeight_        = nullptr;
thread_local TForm_GetVisible   TForm_GetVisible_       = nullptr;
thread_local TForm_SetVisible   TForm_SetVisible_       = nullptr;
thread_local TForm_GetEnabled   TForm_GetEnabled_       = nullptr;
thread_local TForm_SetEnabled   TForm_SetEnabled_       = nullptr;

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
thread_local TButton_GetVisible TButton_GetVisible_     = nullptr;
thread_local TButton_SetVisible TButton_SetVisible_     = nullptr;
thread_local TButton_GetEnabled TButton_GetEnabled_     = nullptr;
thread_local TButton_SetEnabled TButton_SetEnabled_     = nullptr;
thread_local TButton_SetOnClick TButton_SetOnClick_     = nullptr;

thread_local TLabel_Create      TLabel_Create_          = nullptr;
thread_local TLabel_Destroy     TLabel_Destroy_         = nullptr;
thread_local TLabel_SetParent   TLabel_SetParent_       = nullptr;
thread_local TLabel_GetCaption  TLabel_GetCaption_      = nullptr;
thread_local TLabel_SetCaption  TLabel_SetCaption_      = nullptr;
thread_local TLabel_GetLeft     TLabel_GetLeft_         = nullptr;
thread_local TLabel_SetLeft     TLabel_SetLeft_         = nullptr;
thread_local TLabel_GetTop      TLabel_GetTop_          = nullptr;
thread_local TLabel_SetTop      TLabel_SetTop_          = nullptr;
thread_local TLabel_GetWidth    TLabel_GetWidth_        = nullptr;
thread_local TLabel_SetWidth    TLabel_SetWidth_        = nullptr;
thread_local TLabel_GetHeight   TLabel_GetHeight_       = nullptr;
thread_local TLabel_SetHeight   TLabel_SetHeight_       = nullptr;
thread_local TLabel_GetVisible  TLabel_GetVisible_      = nullptr;
thread_local TLabel_SetVisible  TLabel_SetVisible_      = nullptr;
thread_local TLabel_GetEnabled  TLabel_GetEnabled_      = nullptr;
thread_local TLabel_SetEnabled  TLabel_SetEnabled_      = nullptr;

thread_local TEdit_Create       TEdit_Create_           = nullptr;
thread_local TEdit_Destroy      TEdit_Destroy_          = nullptr;
thread_local TEdit_SetParent    TEdit_SetParent_        = nullptr;
thread_local TEdit_GetText      TEdit_GetText_          = nullptr;
thread_local TEdit_SetText      TEdit_SetText_          = nullptr;
thread_local TEdit_GetMaxLength TEdit_GetMaxLength_     = nullptr;
thread_local TEdit_SetMaxLength TEdit_SetMaxLength_     = nullptr;
thread_local TEdit_GetReadOnly  TEdit_GetReadOnly_      = nullptr;
thread_local TEdit_SetReadOnly  TEdit_SetReadOnly_      = nullptr;
thread_local TEdit_GetLeft      TEdit_GetLeft_          = nullptr;
thread_local TEdit_SetLeft      TEdit_SetLeft_          = nullptr;
thread_local TEdit_GetTop       TEdit_GetTop_           = nullptr;
thread_local TEdit_SetTop       TEdit_SetTop_           = nullptr;
thread_local TEdit_GetWidth     TEdit_GetWidth_         = nullptr;
thread_local TEdit_SetWidth     TEdit_SetWidth_         = nullptr;
thread_local TEdit_GetHeight    TEdit_GetHeight_        = nullptr;
thread_local TEdit_SetHeight    TEdit_SetHeight_        = nullptr;
thread_local TEdit_GetVisible   TEdit_GetVisible_       = nullptr;
thread_local TEdit_SetVisible   TEdit_SetVisible_       = nullptr;
thread_local TEdit_GetEnabled   TEdit_GetEnabled_       = nullptr;
thread_local TEdit_SetEnabled   TEdit_SetEnabled_       = nullptr;
thread_local TEdit_SetOnChange  TEdit_SetOnChange_      = nullptr;

thread_local TCheckBox_Create     TCheckBox_Create_         = nullptr;
thread_local TCheckBox_Destroy    TCheckBox_Destroy_        = nullptr;
thread_local TCheckBox_SetParent  TCheckBox_SetParent_      = nullptr;
thread_local TCheckBox_GetCaption TCheckBox_GetCaption_     = nullptr;
thread_local TCheckBox_SetCaption TCheckBox_SetCaption_     = nullptr;
thread_local TCheckBox_GetChecked TCheckBox_GetChecked_     = nullptr;
thread_local TCheckBox_SetChecked TCheckBox_SetChecked_     = nullptr;
thread_local TCheckBox_GetLeft    TCheckBox_GetLeft_        = nullptr;
thread_local TCheckBox_SetLeft    TCheckBox_SetLeft_        = nullptr;
thread_local TCheckBox_GetTop     TCheckBox_GetTop_         = nullptr;
thread_local TCheckBox_SetTop     TCheckBox_SetTop_         = nullptr;
thread_local TCheckBox_GetWidth   TCheckBox_GetWidth_       = nullptr;
thread_local TCheckBox_SetWidth   TCheckBox_SetWidth_       = nullptr;
thread_local TCheckBox_GetHeight  TCheckBox_GetHeight_      = nullptr;
thread_local TCheckBox_SetHeight  TCheckBox_SetHeight_      = nullptr;
thread_local TCheckBox_GetVisible TCheckBox_GetVisible_     = nullptr;
thread_local TCheckBox_SetVisible TCheckBox_SetVisible_     = nullptr;
thread_local TCheckBox_GetEnabled TCheckBox_GetEnabled_     = nullptr;
thread_local TCheckBox_SetEnabled TCheckBox_SetEnabled_     = nullptr;
thread_local TCheckBox_SetOnClick TCheckBox_SetOnClick_     = nullptr;

thread_local TRadioButton_Create     TRadioButton_Create_         = nullptr;
thread_local TRadioButton_Destroy    TRadioButton_Destroy_        = nullptr;
thread_local TRadioButton_SetParent  TRadioButton_SetParent_      = nullptr;
thread_local TRadioButton_GetCaption TRadioButton_GetCaption_     = nullptr;
thread_local TRadioButton_SetCaption TRadioButton_SetCaption_     = nullptr;
thread_local TRadioButton_GetChecked TRadioButton_GetChecked_     = nullptr;
thread_local TRadioButton_SetChecked TRadioButton_SetChecked_     = nullptr;
thread_local TRadioButton_GetLeft    TRadioButton_GetLeft_        = nullptr;
thread_local TRadioButton_SetLeft    TRadioButton_SetLeft_        = nullptr;
thread_local TRadioButton_GetTop     TRadioButton_GetTop_         = nullptr;
thread_local TRadioButton_SetTop     TRadioButton_SetTop_         = nullptr;
thread_local TRadioButton_GetWidth   TRadioButton_GetWidth_       = nullptr;
thread_local TRadioButton_SetWidth   TRadioButton_SetWidth_       = nullptr;
thread_local TRadioButton_GetHeight  TRadioButton_GetHeight_      = nullptr;
thread_local TRadioButton_SetHeight  TRadioButton_SetHeight_      = nullptr;
thread_local TRadioButton_GetVisible TRadioButton_GetVisible_     = nullptr;
thread_local TRadioButton_SetVisible TRadioButton_SetVisible_     = nullptr;
thread_local TRadioButton_GetEnabled TRadioButton_GetEnabled_     = nullptr;
thread_local TRadioButton_SetEnabled TRadioButton_SetEnabled_     = nullptr;
thread_local TRadioButton_SetOnClick TRadioButton_SetOnClick_     = nullptr;

/* 関数ポインタマッピング */
template<typename Func>
void no_vcl_map(Func& f, HMODULE m, const char* n)
{
    void* p = reinterpret_cast<void*>(::GetProcAddress(m, n));
    f = reinterpret_cast<Func>(p);
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
    NO_VCL_MAP(m, TForm_GetVisible);
    NO_VCL_MAP(m, TForm_SetVisible);
    NO_VCL_MAP(m, TForm_GetEnabled);
    NO_VCL_MAP(m, TForm_SetEnabled);

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
    NO_VCL_MAP(m, TButton_GetVisible);
    NO_VCL_MAP(m, TButton_SetVisible);
    NO_VCL_MAP(m, TButton_GetEnabled);
    NO_VCL_MAP(m, TButton_SetEnabled);
    NO_VCL_MAP(m, TButton_SetOnClick);

    NO_VCL_MAP(m, TLabel_Create);
    NO_VCL_MAP(m, TLabel_Destroy);
    NO_VCL_MAP(m, TLabel_SetParent);
    NO_VCL_MAP(m, TLabel_GetCaption);
    NO_VCL_MAP(m, TLabel_SetCaption);
    NO_VCL_MAP(m, TLabel_GetLeft);
    NO_VCL_MAP(m, TLabel_SetLeft);
    NO_VCL_MAP(m, TLabel_GetTop);
    NO_VCL_MAP(m, TLabel_SetTop);
    NO_VCL_MAP(m, TLabel_GetWidth);
    NO_VCL_MAP(m, TLabel_SetWidth);
    NO_VCL_MAP(m, TLabel_GetHeight);
    NO_VCL_MAP(m, TLabel_SetHeight);
    NO_VCL_MAP(m, TLabel_GetVisible);
    NO_VCL_MAP(m, TLabel_SetVisible);
    NO_VCL_MAP(m, TLabel_GetEnabled);
    NO_VCL_MAP(m, TLabel_SetEnabled);

    NO_VCL_MAP(m, TEdit_Create);
    NO_VCL_MAP(m, TEdit_Destroy);
    NO_VCL_MAP(m, TEdit_SetParent);
    NO_VCL_MAP(m, TEdit_GetText);
    NO_VCL_MAP(m, TEdit_SetText);
    NO_VCL_MAP(m, TEdit_GetMaxLength);
    NO_VCL_MAP(m, TEdit_SetMaxLength);
    NO_VCL_MAP(m, TEdit_GetReadOnly);
    NO_VCL_MAP(m, TEdit_SetReadOnly);
    NO_VCL_MAP(m, TEdit_GetLeft);
    NO_VCL_MAP(m, TEdit_SetLeft);
    NO_VCL_MAP(m, TEdit_GetTop);
    NO_VCL_MAP(m, TEdit_SetTop);
    NO_VCL_MAP(m, TEdit_GetWidth);
    NO_VCL_MAP(m, TEdit_SetWidth);
    NO_VCL_MAP(m, TEdit_GetHeight);
    NO_VCL_MAP(m, TEdit_SetHeight);
    NO_VCL_MAP(m, TEdit_GetVisible);
    NO_VCL_MAP(m, TEdit_SetVisible);
    NO_VCL_MAP(m, TEdit_GetEnabled);
    NO_VCL_MAP(m, TEdit_SetEnabled);
    NO_VCL_MAP(m, TEdit_SetOnChange);

    NO_VCL_MAP(m, TCheckBox_Create);
    NO_VCL_MAP(m, TCheckBox_Destroy);
    NO_VCL_MAP(m, TCheckBox_SetParent);
    NO_VCL_MAP(m, TCheckBox_GetCaption);
    NO_VCL_MAP(m, TCheckBox_SetCaption);
    NO_VCL_MAP(m, TCheckBox_GetChecked);
    NO_VCL_MAP(m, TCheckBox_SetChecked);
    NO_VCL_MAP(m, TCheckBox_GetLeft);
    NO_VCL_MAP(m, TCheckBox_SetLeft);
    NO_VCL_MAP(m, TCheckBox_GetTop);
    NO_VCL_MAP(m, TCheckBox_SetTop);
    NO_VCL_MAP(m, TCheckBox_GetWidth);
    NO_VCL_MAP(m, TCheckBox_SetWidth);
    NO_VCL_MAP(m, TCheckBox_GetHeight);
    NO_VCL_MAP(m, TCheckBox_SetHeight);
    NO_VCL_MAP(m, TCheckBox_GetVisible);
    NO_VCL_MAP(m, TCheckBox_SetVisible);
    NO_VCL_MAP(m, TCheckBox_GetEnabled);
    NO_VCL_MAP(m, TCheckBox_SetEnabled);
    NO_VCL_MAP(m, TCheckBox_SetOnClick);

    NO_VCL_MAP(m, TRadioButton_Create);
    NO_VCL_MAP(m, TRadioButton_Destroy);
    NO_VCL_MAP(m, TRadioButton_SetParent);
    NO_VCL_MAP(m, TRadioButton_GetCaption);
    NO_VCL_MAP(m, TRadioButton_SetCaption);
    NO_VCL_MAP(m, TRadioButton_GetChecked);
    NO_VCL_MAP(m, TRadioButton_SetChecked);
    NO_VCL_MAP(m, TRadioButton_GetLeft);
    NO_VCL_MAP(m, TRadioButton_SetLeft);
    NO_VCL_MAP(m, TRadioButton_GetTop);
    NO_VCL_MAP(m, TRadioButton_SetTop);
    NO_VCL_MAP(m, TRadioButton_GetWidth);
    NO_VCL_MAP(m, TRadioButton_SetWidth);
    NO_VCL_MAP(m, TRadioButton_GetHeight);
    NO_VCL_MAP(m, TRadioButton_SetHeight);
    NO_VCL_MAP(m, TRadioButton_GetVisible);
    NO_VCL_MAP(m, TRadioButton_SetVisible);
    NO_VCL_MAP(m, TRadioButton_GetEnabled);
    NO_VCL_MAP(m, TRadioButton_SetEnabled);
    NO_VCL_MAP(m, TRadioButton_SetOnClick);
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

no_vcl_bool_t NO_VCL_CALL no_vcl_TForm_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_GetVisible_);
    return TForm_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TForm_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TForm_SetVisible_);
    TForm_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TForm_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TForm_GetEnabled_);
    return TForm_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TForm_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TForm_SetEnabled_);
    TForm_SetEnabled_(obj, value);
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

no_vcl_bool_t NO_VCL_CALL no_vcl_TButton_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TButton_GetVisible_);
    return TButton_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TButton_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TButton_SetVisible_);
    TButton_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TButton_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TButton_GetEnabled_);
    return TButton_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TButton_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TButton_SetEnabled_);
    TButton_SetEnabled_(obj, value);
}

void NO_VCL_CALL no_vcl_TButton_SetOnClick(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TButton_SetOnClick_);
    TButton_SetOnClick_(obj, cb);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TLabel_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TLabel_Create_);
    return TLabel_Create_(owner);
}

void NO_VCL_CALL no_vcl_TLabel_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TLabel_Destroy_);
    TLabel_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TLabel_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TLabel_SetParent_);
    TLabel_SetParent_(obj, parentObj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TLabel_GetCaption(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TLabel_GetCaption_);
    return TLabel_GetCaption_(obj);
}

void NO_VCL_CALL no_vcl_TLabel_SetCaption(no_vcl_obj_t obj, no_vcl_str_t cap)
{
    NO_VCL_INIT_CHECK(TLabel_SetCaption_);
    TLabel_SetCaption_(obj, cap);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TLabel_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TLabel_GetLeft_);
    return TLabel_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TLabel_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TLabel_SetLeft_);
    TLabel_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TLabel_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TLabel_GetTop_);
    return TLabel_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TLabel_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TLabel_SetTop_);
    TLabel_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TLabel_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TLabel_GetWidth_);
    return TLabel_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TLabel_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TLabel_SetWidth_);
    TLabel_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TLabel_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TLabel_GetHeight_);
    return TLabel_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TLabel_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TLabel_SetHeight_);
    TLabel_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TLabel_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TLabel_GetVisible_);
    return TLabel_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TLabel_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TLabel_SetVisible_);
    TLabel_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TLabel_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TLabel_GetEnabled_);
    return TLabel_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TLabel_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TLabel_SetEnabled_);
    TLabel_SetEnabled_(obj, value);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TEdit_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TEdit_Create_);
    return TEdit_Create_(owner);
}

void NO_VCL_CALL no_vcl_TEdit_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_Destroy_);
    TEdit_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TEdit_SetParent_);
    TEdit_SetParent_(obj, parentObj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TEdit_GetText(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetText_);
    return TEdit_GetText_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetText(no_vcl_obj_t obj, no_vcl_str_t text)
{
    NO_VCL_INIT_CHECK(TEdit_SetText_);
    TEdit_SetText_(obj, text);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetMaxLength(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetMaxLength_);
    return TEdit_GetMaxLength_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetMaxLength(no_vcl_obj_t obj, no_vcl_int_t value)
{
    NO_VCL_INIT_CHECK(TEdit_SetMaxLength_);
    TEdit_SetMaxLength_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TEdit_GetReadOnly(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetReadOnly_);
    return TEdit_GetReadOnly_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetReadOnly(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TEdit_SetReadOnly_);
    TEdit_SetReadOnly_(obj, value);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetLeft_);
    return TEdit_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TEdit_SetLeft_);
    TEdit_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetTop_);
    return TEdit_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TEdit_SetTop_);
    TEdit_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetWidth_);
    return TEdit_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TEdit_SetWidth_);
    TEdit_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetHeight_);
    return TEdit_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TEdit_SetHeight_);
    TEdit_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TEdit_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetVisible_);
    return TEdit_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TEdit_SetVisible_);
    TEdit_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TEdit_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TEdit_GetEnabled_);
    return TEdit_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TEdit_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TEdit_SetEnabled_);
    TEdit_SetEnabled_(obj, value);
}

void NO_VCL_CALL no_vcl_TEdit_SetOnChange(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TEdit_SetOnChange_);
    TEdit_SetOnChange_(obj, cb);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TCheckBox_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TCheckBox_Create_);
    return TCheckBox_Create_(owner);
}

void NO_VCL_CALL no_vcl_TCheckBox_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_Destroy_);
    TCheckBox_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetParent_);
    TCheckBox_SetParent_(obj, parentObj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TCheckBox_GetCaption(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_GetCaption_);
    return TCheckBox_GetCaption_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetCaption(no_vcl_obj_t obj, no_vcl_str_t cap)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetCaption_);
    TCheckBox_SetCaption_(obj, cap);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TCheckBox_GetChecked(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_GetChecked_);
    return TCheckBox_GetChecked_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetChecked(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetChecked_);
    TCheckBox_SetChecked_(obj, value);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TCheckBox_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_GetLeft_);
    return TCheckBox_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetLeft_);
    TCheckBox_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TCheckBox_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_GetTop_);
    return TCheckBox_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetTop_);
    TCheckBox_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TCheckBox_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_GetWidth_);
    return TCheckBox_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetWidth_);
    TCheckBox_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TCheckBox_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_GetHeight_);
    return TCheckBox_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetHeight_);
    TCheckBox_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TCheckBox_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_GetVisible_);
    return TCheckBox_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetVisible_);
    TCheckBox_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TCheckBox_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TCheckBox_GetEnabled_);
    return TCheckBox_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetEnabled_);
    TCheckBox_SetEnabled_(obj, value);
}

void NO_VCL_CALL no_vcl_TCheckBox_SetOnClick(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TCheckBox_SetOnClick_);
    TCheckBox_SetOnClick_(obj, cb);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TRadioButton_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TRadioButton_Create_);
    return TRadioButton_Create_(owner);
}

void NO_VCL_CALL no_vcl_TRadioButton_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_Destroy_);
    TRadioButton_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetParent_);
    TRadioButton_SetParent_(obj, parentObj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TRadioButton_GetCaption(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_GetCaption_);
    return TRadioButton_GetCaption_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetCaption(no_vcl_obj_t obj, no_vcl_str_t cap)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetCaption_);
    TRadioButton_SetCaption_(obj, cap);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TRadioButton_GetChecked(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_GetChecked_);
    return TRadioButton_GetChecked_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetChecked(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetChecked_);
    TRadioButton_SetChecked_(obj, value);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TRadioButton_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_GetLeft_);
    return TRadioButton_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetLeft_);
    TRadioButton_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TRadioButton_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_GetTop_);
    return TRadioButton_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetTop_);
    TRadioButton_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TRadioButton_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_GetWidth_);
    return TRadioButton_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetWidth_);
    TRadioButton_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TRadioButton_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_GetHeight_);
    return TRadioButton_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetHeight_);
    TRadioButton_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TRadioButton_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_GetVisible_);
    return TRadioButton_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetVisible_);
    TRadioButton_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TRadioButton_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TRadioButton_GetEnabled_);
    return TRadioButton_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetEnabled_);
    TRadioButton_SetEnabled_(obj, value);
}

void NO_VCL_CALL no_vcl_TRadioButton_SetOnClick(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TRadioButton_SetOnClick_);
    TRadioButton_SetOnClick_(obj, cb);
}

} // extern "C"
