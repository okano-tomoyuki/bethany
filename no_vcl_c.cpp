#include <cassert>
#include <mutex>

#include <windows.h>
#include "no_vcl_c.h"

// no_vcl.dll からエクスポートされる関数の一覧(戻り値型, 名前, 引数リスト, 呼び出し時の引数)。
// ここに1行追加すると、関数ポインタ型・thread_local 変数・GetProcAddress によるマッピング・
// extern "C" のラッパー(no_vcl_<名前>)がまとめて生成される。
// 宣言は no_vcl_c.h にも手で書く(型が食い違えばコンパイルエラーになる)。
#define NO_VCL_FUNCS(X) \
    X(void,          FreeNotify_SetCallback,        (no_vcl_callback_t cb, void* d),                          (cb, d)) \
    X(void,          TComponent_Destroy,            (no_vcl_obj_t o),                                         (o)) \
    X(void,          TComponent_DestroyComponents,  (no_vcl_obj_t o),                                         (o)) \
    \
    X(no_vcl_obj_t,  TControl_GetParent,            (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetParent,            (no_vcl_obj_t o, no_vcl_obj_t p),                         (o, p)) \
    X(no_vcl_int_t,  TControl_GetLeft,              (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetLeft,              (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_int_t,  TControl_GetTop,               (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetTop,               (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_int_t,  TControl_GetWidth,             (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetWidth,             (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_int_t,  TControl_GetHeight,            (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetHeight,            (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_bool_t, TControl_GetVisible,           (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetVisible,           (no_vcl_obj_t o, no_vcl_bool_t v),                        (o, v)) \
    X(no_vcl_bool_t, TControl_GetEnabled,           (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetEnabled,           (no_vcl_obj_t o, no_vcl_bool_t v),                        (o, v)) \
    X(no_vcl_str_t,  TControl_GetCaption,           (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetCaption,           (no_vcl_obj_t o, no_vcl_str_t v),                         (o, v)) \
    X(no_vcl_str_t,  TControl_GetText,              (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetText,              (no_vcl_obj_t o, no_vcl_str_t v),                         (o, v)) \
    X(void,          TControl_Show,                 (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_Hide,                 (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetOnClick,           (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TForm_Create,                  (no_vcl_obj_t owner),                                     (owner)) \
    X(void,          TCustomForm_Show,              (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomForm_Hide,              (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_int_t,  TCustomForm_ShowModal,         (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomForm_Close,             (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomForm_Release,           (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomForm_SetOnClose,        (no_vcl_obj_t o, no_vcl_close_callback_t cb, void* d),    (o, cb, d)) \
    X(void,          TCustomForm_SetOnShow,         (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    \
    X(no_vcl_obj_t,  GetApplication,                (void),                                                   ()) \
    X(no_vcl_obj_t,  TApplication_CreateForm,       (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_obj_t,  TApplication_GetMainForm,      (no_vcl_obj_t o),                                         (o)) \
    X(void,          TApplication_Run,              (no_vcl_obj_t o),                                         (o)) \
    X(void,          TApplication_ProcessMessages,  (no_vcl_obj_t o),                                         (o)) \
    X(void,          TApplication_Terminate,        (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_bool_t, TApplication_GetTerminated,    (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_str_t,  TApplication_GetTitle,         (no_vcl_obj_t o),                                         (o)) \
    X(void,          TApplication_SetTitle,         (no_vcl_obj_t o, no_vcl_str_t v),                         (o, v)) \
    X(no_vcl_bool_t, TApplication_GetShowMainForm,  (no_vcl_obj_t o),                                         (o)) \
    X(void,          TApplication_SetShowMainForm,  (no_vcl_obj_t o, no_vcl_bool_t v),                        (o, v)) \
    \
    X(no_vcl_obj_t,  TPanel_Create,                 (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_obj_t,  TGroupBox_Create,              (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_obj_t,  TLabel_Create,                 (no_vcl_obj_t owner),                                     (owner)) \
    \
    X(no_vcl_bool_t, TButtonControl_GetChecked,     (no_vcl_obj_t o),                                         (o)) \
    X(void,          TButtonControl_SetChecked,     (no_vcl_obj_t o, no_vcl_bool_t v),                        (o, v)) \
    X(no_vcl_obj_t,  TButton_Create,                (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_obj_t,  TCheckBox_Create,              (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_obj_t,  TRadioButton_Create,           (no_vcl_obj_t owner),                                     (owner)) \
    \
    X(no_vcl_int_t,  TCustomEdit_GetMaxLength,      (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomEdit_SetMaxLength,      (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_bool_t, TCustomEdit_GetReadOnly,       (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomEdit_SetReadOnly,       (no_vcl_obj_t o, no_vcl_bool_t v),                        (o, v)) \
    X(void,          TCustomEdit_SetOnChange,       (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(no_vcl_obj_t,  TEdit_Create,                  (no_vcl_obj_t owner),                                     (owner)) \
    \
    X(void,          TCustomMemo_Lines_Add,         (no_vcl_obj_t o, no_vcl_str_t s),                         (o, s)) \
    X(void,          TCustomMemo_Lines_Clear,       (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_int_t,  TCustomMemo_Lines_Count,       (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_str_t,  TCustomMemo_Lines_GetText,     (no_vcl_obj_t o, no_vcl_int_t i),                         (o, i)) \
    X(no_vcl_int_t,  TCustomMemo_GetScrollBars,     (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomMemo_SetScrollBars,     (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_obj_t,  TMemo_Create,                  (no_vcl_obj_t owner),                                     (owner)) \
    \
    X(no_vcl_int_t,  TCustomComboBox_GetItemIndex,  (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomComboBox_SetItemIndex,  (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(void,          TCustomComboBox_Items_Add,     (no_vcl_obj_t o, no_vcl_str_t s),                         (o, s)) \
    X(void,          TCustomComboBox_Items_Clear,   (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_int_t,  TCustomComboBox_Items_Count,   (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_str_t,  TCustomComboBox_Items_GetText, (no_vcl_obj_t o, no_vcl_int_t i),                         (o, i)) \
    X(no_vcl_obj_t,  TComboBox_Create,              (no_vcl_obj_t owner),                                     (owner)) \
    X(void,          TComboBox_SetOnChange,         (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    \
    X(no_vcl_int_t,  TCustomListBox_GetItemIndex,   (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomListBox_SetItemIndex,   (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(void,          TCustomListBox_Items_Add,      (no_vcl_obj_t o, no_vcl_str_t s),                         (o, s)) \
    X(void,          TCustomListBox_Items_Clear,    (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_int_t,  TCustomListBox_Items_Count,    (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_str_t,  TCustomListBox_Items_GetText,  (no_vcl_obj_t o, no_vcl_int_t i),                         (o, i)) \
    X(no_vcl_obj_t,  TListBox_Create,               (no_vcl_obj_t owner),                                     (owner)) \
    \
    X(no_vcl_int_t,  TCustomTimer_GetInterval,      (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomTimer_SetInterval,      (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_bool_t, TCustomTimer_GetEnabled,       (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomTimer_SetEnabled,       (no_vcl_obj_t o, no_vcl_bool_t v),                        (o, v)) \
    X(void,          TCustomTimer_SetOnTimer,       (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(no_vcl_obj_t,  TTimer_Create,                 (no_vcl_obj_t owner),                                     (owner)) \
    \
    X(no_vcl_obj_t,  TPaintBox_Create,              (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_obj_t,  TPaintBox_GetCanvas,           (no_vcl_obj_t o),                                         (o)) \
    X(void,          TPaintBox_SetOnPaint,          (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    \
    X(void,          TCanvas_MoveTo,                (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y),         (o, x, y)) \
    X(void,          TCanvas_LineTo,                (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y),         (o, x, y)) \
    X(void,          TCanvas_Rectangle,             (no_vcl_obj_t o, no_vcl_int_t x1, no_vcl_int_t y1, no_vcl_int_t x2, no_vcl_int_t y2), (o, x1, y1, x2, y2)) \
    X(void,          TCanvas_Ellipse,               (no_vcl_obj_t o, no_vcl_int_t x1, no_vcl_int_t y1, no_vcl_int_t x2, no_vcl_int_t y2), (o, x1, y1, x2, y2)) \
    X(void,          TCanvas_TextOut,               (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y, no_vcl_str_t s), (o, x, y, s)) \
    X(no_vcl_obj_t,  TCanvas_GetPen,                (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_obj_t,  TCanvas_GetBrush,              (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_obj_t,  TCanvas_GetFont,               (no_vcl_obj_t o),                                         (o)) \
    \
    X(no_vcl_int_t,  TPen_GetColor,                 (no_vcl_obj_t o),                                         (o)) \
    X(void,          TPen_SetColor,                 (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_int_t,  TPen_GetWidth,                 (no_vcl_obj_t o),                                         (o)) \
    X(void,          TPen_SetWidth,                 (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_int_t,  TBrush_GetColor,               (no_vcl_obj_t o),                                         (o)) \
    X(void,          TBrush_SetColor,               (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_str_t,  TFont_GetName,                 (no_vcl_obj_t o),                                         (o)) \
    X(void,          TFont_SetName,                 (no_vcl_obj_t o, no_vcl_str_t v),                         (o, v)) \
    X(no_vcl_int_t,  TFont_GetSize,                 (no_vcl_obj_t o),                                         (o)) \
    X(void,          TFont_SetSize,                 (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_int_t,  TFont_GetColor,                (no_vcl_obj_t o),                                         (o)) \
    X(void,          TFont_SetColor,                (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v))

namespace
{

/* 関数ポインタ型と thread_local 変数 */
#define NO_VCL_DECLARE(ret, name, params, args) \
    typedef ret (NO_VCL_CALL *name##_fn) params; \
    thread_local name##_fn name##_ = nullptr;
NO_VCL_FUNCS(NO_VCL_DECLARE)
#undef NO_VCL_DECLARE

/* 関数ポインタマッピング */
template<typename Func>
void no_vcl_map(Func& f, HMODULE m, const char* n)
{
    void* p = reinterpret_cast<void*>(::GetProcAddress(m, n));
    f = reinterpret_cast<Func>(p);
}

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

#define NO_VCL_MAP(ret, name, params, args) no_vcl_map(name##_, m, #name);
    NO_VCL_FUNCS(NO_VCL_MAP)
#undef NO_VCL_MAP
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

#define NO_VCL_DEFINE(ret, name, params, args) \
    ret NO_VCL_CALL no_vcl_##name params \
    { \
        NO_VCL_INIT_CHECK(name##_); \
        return name##_ args; \
    }
NO_VCL_FUNCS(NO_VCL_DEFINE)
#undef NO_VCL_DEFINE

} // extern "C"
