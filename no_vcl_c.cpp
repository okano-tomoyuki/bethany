#include <cassert>
#include <mutex>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
using no_vcl_module_t = HMODULE;
#else
#include <dlfcn.h>
using no_vcl_module_t = void*;
#endif

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
    X(no_vcl_int_t,  TControl_GetAlign,             (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetAlign,             (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_str_t,  TControl_GetText,              (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetText,              (no_vcl_obj_t o, no_vcl_str_t v),                         (o, v)) \
    X(void,          TControl_Show,                 (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_Hide,                 (no_vcl_obj_t o),                                         (o)) \
    X(void,          TControl_SetOnClick,           (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TControl_SetOnDblClick,        (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TControl_SetOnResize,          (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TControl_SetOnMouseDown,       (no_vcl_obj_t o, no_vcl_mouse_callback_t cb, void* d),    (o, cb, d)) \
    X(void,          TControl_SetOnMouseUp,         (no_vcl_obj_t o, no_vcl_mouse_callback_t cb, void* d),    (o, cb, d)) \
    X(void,          TControl_SetOnMouseMove,       (no_vcl_obj_t o, no_vcl_mouse_move_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TControl_SetOnMouseEnter,      (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TControl_SetOnMouseLeave,      (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TControl_SetOnMouseWheel,      (no_vcl_obj_t o, no_vcl_mouse_wheel_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TWinControl_SetOnKeyDown,      (no_vcl_obj_t o, no_vcl_key_callback_t cb, void* d),      (o, cb, d)) \
    X(void,          TWinControl_SetOnKeyUp,        (no_vcl_obj_t o, no_vcl_key_callback_t cb, void* d),      (o, cb, d)) \
    X(void,          TWinControl_SetOnKeyPress,     (no_vcl_obj_t o, no_vcl_key_press_callback_t cb, void* d), (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TForm_Create,                  (no_vcl_obj_t owner),                                     (owner)) \
    X(void,          TCustomForm_Show,              (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomForm_Hide,              (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_int_t,  TCustomForm_ShowModal,         (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomForm_Close,             (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomForm_Release,           (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomForm_SetOnClose,        (no_vcl_obj_t o, no_vcl_close_callback_t cb, void* d),    (o, cb, d)) \
    X(void,          TCustomForm_SetOnCloseQuery,   (no_vcl_obj_t o, no_vcl_close_query_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomForm_SetOnShow,         (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TCustomForm_SetOnHide,         (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TCustomForm_SetOnActivate,     (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TCustomForm_SetOnDeactivate,   (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
    X(void,          TCustomForm_SetOnDestroy,      (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),          (o, cb, d)) \
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
    X(void,          TFont_SetColor,                (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    \
    X(no_vcl_obj_t,  TScrollBox_Create,              (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_obj_t,  TToggleBox_Create,              (no_vcl_obj_t owner),                                     (owner)) \
    \
    X(no_vcl_obj_t,  TBevel_Create,                  (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_int_t,  TBevel_GetShape,                (no_vcl_obj_t o),                                         (o)) \
    X(void,          TBevel_SetShape,                (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_int_t,  TBevel_GetStyle,                (no_vcl_obj_t o),                                         (o)) \
    X(void,          TBevel_SetStyle,                (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    \
    X(no_vcl_obj_t,  TShape_Create,                  (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_int_t,  TCustomShape_GetShape,          (no_vcl_obj_t o),                                         (o)) \
    X(void,          TCustomShape_SetShape,          (no_vcl_obj_t o, no_vcl_int_t v),                         (o, v)) \
    X(no_vcl_obj_t,  TCustomShape_GetPen,            (no_vcl_obj_t o),                                         (o)) \
    X(no_vcl_obj_t,  TCustomShape_GetBrush,          (no_vcl_obj_t o),                                         (o)) \
    \
    X(no_vcl_obj_t,  TStaticText_Create,             (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_int_t,  TCustomStaticText_GetBorderStyle, (no_vcl_obj_t o),                                       (o)) \
    X(void,          TCustomStaticText_SetBorderStyle, (no_vcl_obj_t o, no_vcl_int_t v),                       (o, v)) \
    \
    X(no_vcl_obj_t,  TStatusBar_Create,              (no_vcl_obj_t owner),                                     (owner)) \
    X(no_vcl_str_t,  TStatusBar_GetSimpleText,       (no_vcl_obj_t o),                                         (o)) \
    X(void,          TStatusBar_SetSimpleText,       (no_vcl_obj_t o, no_vcl_str_t v),                         (o, v)) \
    X(no_vcl_bool_t, TStatusBar_GetSimplePanel,      (no_vcl_obj_t o),                                         (o)) \
    X(void,          TStatusBar_SetSimplePanel,      (no_vcl_obj_t o, no_vcl_bool_t v),                        (o, v)) \
    \
    X(no_vcl_obj_t,  TScrollBar_Create,               (no_vcl_obj_t owner),                                    (owner)) \
    X(no_vcl_int_t,  TCustomScrollBar_GetKind,        (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomScrollBar_SetKind,        (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TCustomScrollBar_GetMin,         (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomScrollBar_SetMin,         (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TCustomScrollBar_GetMax,         (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomScrollBar_SetMax,         (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TCustomScrollBar_GetPosition,    (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomScrollBar_SetPosition,    (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TCustomScrollBar_GetPageSize,    (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomScrollBar_SetPageSize,    (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(void,          TCustomScrollBar_SetOnChange,    (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),         (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TTrackBar_Create,                (no_vcl_obj_t owner),                                    (owner)) \
    X(no_vcl_int_t,  TCustomTrackBar_GetMin,          (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomTrackBar_SetMin,          (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TCustomTrackBar_GetMax,          (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomTrackBar_SetMax,          (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TCustomTrackBar_GetPosition,     (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomTrackBar_SetPosition,     (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(void,          TCustomTrackBar_SetOnChange,     (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),         (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TProgressBar_Create,             (no_vcl_obj_t owner),                                    (owner)) \
    X(no_vcl_int_t,  TCustomProgressBar_GetMin,       (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomProgressBar_SetMin,       (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TCustomProgressBar_GetMax,       (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomProgressBar_SetMax,       (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TCustomProgressBar_GetPosition,  (no_vcl_obj_t o),                                        (o)) \
    X(void,          TCustomProgressBar_SetPosition,  (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    \
    X(no_vcl_obj_t,  TUpDown_Create,                  (no_vcl_obj_t owner),                                    (owner)) \
    X(no_vcl_int_t,  TUpDown_GetMin,                  (no_vcl_obj_t o),                                        (o)) \
    X(void,          TUpDown_SetMin,                  (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TUpDown_GetMax,                  (no_vcl_obj_t o),                                        (o)) \
    X(void,          TUpDown_SetMax,                  (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TUpDown_GetPosition,             (no_vcl_obj_t o),                                        (o)) \
    X(void,          TUpDown_SetPosition,             (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_int_t,  TUpDown_GetIncrement,            (no_vcl_obj_t o),                                        (o)) \
    X(void,          TUpDown_SetIncrement,            (no_vcl_obj_t o, no_vcl_int_t v),                        (o, v)) \
    X(no_vcl_obj_t,  TUpDown_GetAssociate,            (no_vcl_obj_t o),                                        (o)) \
    X(void,          TUpDown_SetAssociate,            (no_vcl_obj_t o, no_vcl_obj_t v),                        (o, v)) \
    \
    X(no_vcl_obj_t,  TRadioGroup_Create,                     (no_vcl_obj_t owner),                                 (owner)) \
    X(void,          TCustomRadioGroup_Items_Add,            (no_vcl_obj_t o, no_vcl_str_t t),                     (o, t)) \
    X(void,          TCustomRadioGroup_Items_Clear,          (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TCustomRadioGroup_Items_Count,          (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_str_t,  TCustomRadioGroup_Items_GetText,        (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_int_t,  TCustomRadioGroup_GetItemIndex,         (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomRadioGroup_SetItemIndex,         (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(void,          TCustomRadioGroup_SetOnClick,           (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TCheckGroup_Create,                     (no_vcl_obj_t owner),                                 (owner)) \
    X(void,          TCustomCheckGroup_Items_Add,            (no_vcl_obj_t o, no_vcl_str_t t),                     (o, t)) \
    X(void,          TCustomCheckGroup_Items_Clear,          (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TCustomCheckGroup_Items_Count,          (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_str_t,  TCustomCheckGroup_Items_GetText,        (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_bool_t, TCustomCheckGroup_GetChecked,           (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TCustomCheckGroup_SetChecked,           (no_vcl_obj_t o, no_vcl_int_t i, no_vcl_bool_t v),    (o, i, v)) \
    \
    X(no_vcl_obj_t,  TCheckListBox_Create,                   (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_bool_t, TCustomCheckListBox_GetChecked,         (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TCustomCheckListBox_SetChecked,         (no_vcl_obj_t o, no_vcl_int_t i, no_vcl_bool_t v),    (o, i, v)) \
    X(void,          TCustomCheckListBox_SetOnClickCheck,    (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TSpeedButton_Create,                    (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_bool_t, TCustomSpeedButton_GetDown,              (no_vcl_obj_t o),                                    (o)) \
    X(void,          TCustomSpeedButton_SetDown,              (no_vcl_obj_t o, no_vcl_bool_t v),                   (o, v)) \
    X(no_vcl_int_t,  TCustomSpeedButton_GetGroupIndex,        (no_vcl_obj_t o),                                    (o)) \
    X(void,          TCustomSpeedButton_SetGroupIndex,        (no_vcl_obj_t o, no_vcl_int_t v),                    (o, v)) \
    X(no_vcl_bool_t, TCustomSpeedButton_GetFlat,               (no_vcl_obj_t o),                                    (o)) \
    X(void,          TCustomSpeedButton_SetFlat,               (no_vcl_obj_t o, no_vcl_bool_t v),                   (o, v)) \
    X(no_vcl_bool_t, TCustomSpeedButton_GetAllowAllUp,         (no_vcl_obj_t o),                                    (o)) \
    X(void,          TCustomSpeedButton_SetAllowAllUp,         (no_vcl_obj_t o, no_vcl_bool_t v),                   (o, v)) \
    \
    X(no_vcl_obj_t,  TBitBtn_Create,                          (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_int_t,  TCustomBitBtn_GetKind,                    (no_vcl_obj_t o),                                    (o)) \
    X(void,          TCustomBitBtn_SetKind,                    (no_vcl_obj_t o, no_vcl_int_t v),                    (o, v)) \
    \
    X(no_vcl_obj_t,   TFloatSpinEdit_Create,                          (no_vcl_obj_t owner),                          (owner)) \
    X(no_vcl_float_t, TCustomFloatSpinEdit_GetValue,                  (no_vcl_obj_t o),                              (o)) \
    X(void,           TCustomFloatSpinEdit_SetValue,                  (no_vcl_obj_t o, no_vcl_float_t v),            (o, v)) \
    X(no_vcl_float_t, TCustomFloatSpinEdit_GetMinValue,               (no_vcl_obj_t o),                              (o)) \
    X(void,           TCustomFloatSpinEdit_SetMinValue,               (no_vcl_obj_t o, no_vcl_float_t v),            (o, v)) \
    X(no_vcl_float_t, TCustomFloatSpinEdit_GetMaxValue,               (no_vcl_obj_t o),                              (o)) \
    X(void,           TCustomFloatSpinEdit_SetMaxValue,               (no_vcl_obj_t o, no_vcl_float_t v),            (o, v)) \
    X(no_vcl_float_t, TCustomFloatSpinEdit_GetIncrement,              (no_vcl_obj_t o),                              (o)) \
    X(void,           TCustomFloatSpinEdit_SetIncrement,              (no_vcl_obj_t o, no_vcl_float_t v),            (o, v)) \
    X(no_vcl_int_t,   TCustomFloatSpinEdit_GetDecimalPlaces,          (no_vcl_obj_t o),                              (o)) \
    X(void,           TCustomFloatSpinEdit_SetDecimalPlaces,          (no_vcl_obj_t o, no_vcl_int_t v),              (o, v)) \
    \
    X(no_vcl_obj_t,  TSpinEdit_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_int_t,  TCustomSpinEdit_GetValue,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSpinEdit_SetValue,                (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomSpinEdit_GetMinValue,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSpinEdit_SetMinValue,             (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomSpinEdit_GetMaxValue,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSpinEdit_SetMaxValue,             (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomSpinEdit_GetIncrement,            (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSpinEdit_SetIncrement,            (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    \
    X(no_vcl_obj_t,  TMaskEdit_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_str_t,  TMaskEdit_GetEditMask,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMaskEdit_SetEditMask,                   (no_vcl_obj_t o, no_vcl_str_t v),                     (o, v)) \
    \
    X(no_vcl_obj_t,  TTabControl_Create,                      (no_vcl_obj_t owner),                                 (owner)) \
    X(void,          TTabControl_Tabs_Add,                    (no_vcl_obj_t o, no_vcl_str_t t),                     (o, t)) \
    X(void,          TTabControl_Tabs_Clear,                  (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TTabControl_Tabs_Count,                  (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_str_t,  TTabControl_Tabs_GetText,                (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_int_t,  TTabControl_GetTabIndex,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTabControl_SetTabIndex,                 (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(void,          TTabControl_SetOnChange,                 (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TSplitter_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_bool_t, TCustomSplitter_GetAutoSnap,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSplitter_SetAutoSnap,             (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TCustomSplitter_GetBeveled,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSplitter_SetBeveled,              (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TCustomSplitter_GetMinSize,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSplitter_SetMinSize,              (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomSplitter_GetResizeAnchor,         (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSplitter_SetResizeAnchor,         (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomSplitter_GetResizeStyle,          (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSplitter_SetResizeStyle,          (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomSplitter_GetSplitterPosition,     (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomSplitter_SetSplitterPosition,     (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(void,          TCustomSplitter_SetOnMoved,              (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    \
    X(no_vcl_int_t,  ShortCut_Make,                           (no_vcl_int_t k, no_vcl_int_t s),                     (k, s)) \
    X(no_vcl_int_t,  ShortCut_FromText,                       (no_vcl_str_t t),                                     (t)) \
    X(no_vcl_str_t,  ShortCut_ToText,                         (no_vcl_int_t v),                                     (v)) \
    \
    X(no_vcl_obj_t,  TMenuItem_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_str_t,  TMenuItem_GetCaption,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetCaption,                    (no_vcl_obj_t o, no_vcl_str_t v),                     (o, v)) \
    X(no_vcl_bool_t, TMenuItem_GetChecked,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetChecked,                    (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TMenuItem_GetEnabled,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetEnabled,                    (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TMenuItem_GetVisible,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetVisible,                    (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TMenuItem_GetAutoCheck,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetAutoCheck,                  (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TMenuItem_GetRadioItem,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetRadioItem,                  (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TMenuItem_GetGroupIndex,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetGroupIndex,                 (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_bool_t, TMenuItem_GetDefault,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetDefault,                    (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TMenuItem_GetShortCut,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetShortCut,                   (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_str_t,  TMenuItem_GetHint,                       (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_SetHint,                       (no_vcl_obj_t o, no_vcl_str_t v),                     (o, v)) \
    X(void,          TMenuItem_SetOnClick,                    (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    X(no_vcl_int_t,  TMenuItem_GetCount,                      (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TMenuItem_GetItem,                       (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_obj_t,  TMenuItem_GetParent,                     (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_Add,                           (no_vcl_obj_t o, no_vcl_obj_t item),                  (o, item)) \
    X(void,          TMenuItem_Insert,                        (no_vcl_obj_t o, no_vcl_int_t i, no_vcl_obj_t item),  (o, i, item)) \
    X(void,          TMenuItem_Delete,                        (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TMenuItem_Remove,                        (no_vcl_obj_t o, no_vcl_obj_t item),                  (o, item)) \
    X(void,          TMenuItem_Clear,                         (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TMenuItem_IndexOf,                       (no_vcl_obj_t o, no_vcl_obj_t item),                  (o, item)) \
    X(void,          TMenuItem_AddSeparator,                  (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_bool_t, TMenuItem_IsLine,                        (no_vcl_obj_t o),                                     (o)) \
    X(void,          TMenuItem_Click,                         (no_vcl_obj_t o),                                     (o)) \
    \
    X(no_vcl_obj_t,  TMenu_GetItems,                          (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TMainMenu_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_obj_t,  TPopupMenu_Create,                       (no_vcl_obj_t owner),                                 (owner)) \
    X(void,          TPopupMenu_Popup,                        (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y),     (o, x, y)) \
    X(no_vcl_bool_t, TPopupMenu_GetAutoPopup,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TPopupMenu_SetAutoPopup,                 (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_obj_t,  TPopupMenu_GetPopupComponent,            (no_vcl_obj_t o),                                     (o)) \
    X(void,          TPopupMenu_SetPopupComponent,            (no_vcl_obj_t o, no_vcl_obj_t v),                     (o, v)) \
    X(void,          TPopupMenu_SetOnPopup,                   (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    X(void,          TPopupMenu_SetOnClose,                   (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TCustomForm_GetMenu,                     (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomForm_SetMenu,                     (no_vcl_obj_t o, no_vcl_obj_t v),                     (o, v)) \
    X(no_vcl_obj_t,  TControl_GetPopupMenu,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TControl_SetPopupMenu,                   (no_vcl_obj_t o, no_vcl_obj_t v),                     (o, v)) \
    \
    X(no_vcl_obj_t,  TPageControl_Create,                     (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_obj_t,  TPageControl_GetActivePage,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TPageControl_SetActivePage,              (no_vcl_obj_t o, no_vcl_obj_t v),                     (o, v)) \
    X(no_vcl_int_t,  TPageControl_GetActivePageIndex,         (no_vcl_obj_t o),                                     (o)) \
    X(void,          TPageControl_SetActivePageIndex,         (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_obj_t,  TPageControl_GetPage,                    (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_int_t,  TCustomTabControl_GetPageCount,          (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TPageControl_AddTabSheet,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TPageControl_Clear,                      (no_vcl_obj_t o),                                     (o)) \
    X(void,          TPageControl_SelectNextPage,             (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TPageControl_GetTabIndex,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TPageControl_SetTabIndex,                (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(void,          TPageControl_SetOnChange,                (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    X(void,          TCustomTabControl_SetOnChanging,         (no_vcl_obj_t o, no_vcl_close_query_callback_t cb, void* d), (o, cb, d)) \
    X(no_vcl_bool_t, TCustomTabControl_GetMultiLine,          (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomTabControl_SetMultiLine,          (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TCustomTabControl_GetShowTabs,           (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomTabControl_SetShowTabs,           (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TCustomTabControl_GetTabPosition,        (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomTabControl_SetTabPosition,        (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    \
    X(no_vcl_obj_t,  TTabSheet_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_obj_t,  TTabSheet_GetPageControl,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTabSheet_SetPageControl,                (no_vcl_obj_t o, no_vcl_obj_t v),                     (o, v)) \
    X(no_vcl_int_t,  TTabSheet_GetTabIndex,                   (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TCustomPage_GetPageIndex,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomPage_SetPageIndex,                (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_bool_t, TCustomPage_GetTabVisible,               (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomPage_SetTabVisible,               (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(void,          TCustomPage_SetOnShow,                   (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    X(void,          TCustomPage_SetOnHide,                   (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d)) \
    \
    X(void,          ItemFree_SetCallback,                (no_vcl_callback_t cb, void* d),                      (cb, d)) \
    X(no_vcl_obj_t,  TTreeView_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_obj_t,  TCustomTreeView_GetItems,                (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TCustomTreeView_GetSelected,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomTreeView_SetSelected,             (no_vcl_obj_t o, no_vcl_obj_t n),                     (o, n)) \
    X(void,          TCustomTreeView_FullExpand,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomTreeView_FullCollapse,            (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_bool_t, TCustomTreeView_AlphaSort,               (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TCustomTreeView_GetNodeAt,               (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y),     (o, x, y)) \
    X(no_vcl_bool_t, TTreeView_GetReadOnly,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeView_SetReadOnly,                   (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TTreeView_GetShowLines,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeView_SetShowLines,                  (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TTreeView_GetShowRoot,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeView_SetShowRoot,                   (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TTreeView_GetShowButtons,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeView_SetShowButtons,                (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TTreeView_GetAutoExpand,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeView_SetAutoExpand,                 (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TTreeView_GetHideSelection,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeView_SetHideSelection,              (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TTreeView_GetRowSelect,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeView_SetRowSelect,                  (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(void,          TTreeView_SetOnChange,                   (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TTreeView_SetOnExpanded,                 (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TTreeView_SetOnCollapsed,                (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TTreeView_SetOnDeletion,                 (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TTreeView_SetOnChanging,                 (no_vcl_obj_t o, no_vcl_item_allow_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TTreeView_SetOnExpanding,                (no_vcl_obj_t o, no_vcl_item_allow_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TTreeView_SetOnCollapsing,               (no_vcl_obj_t o, no_vcl_item_allow_callback_t cb, void* d), (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TTreeNodes_Add,                          (no_vcl_obj_t o, no_vcl_obj_t n, no_vcl_str_t t),     (o, n, t)) \
    X(no_vcl_obj_t,  TTreeNodes_AddFirst,                     (no_vcl_obj_t o, no_vcl_obj_t n, no_vcl_str_t t),     (o, n, t)) \
    X(no_vcl_obj_t,  TTreeNodes_AddChild,                     (no_vcl_obj_t o, no_vcl_obj_t n, no_vcl_str_t t),     (o, n, t)) \
    X(no_vcl_obj_t,  TTreeNodes_AddChildFirst,                (no_vcl_obj_t o, no_vcl_obj_t n, no_vcl_str_t t),     (o, n, t)) \
    X(no_vcl_obj_t,  TTreeNodes_Insert,                       (no_vcl_obj_t o, no_vcl_obj_t n, no_vcl_str_t t),     (o, n, t)) \
    X(void,          TTreeNodes_Clear,                        (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNodes_Delete,                       (no_vcl_obj_t o, no_vcl_obj_t n),                     (o, n)) \
    X(no_vcl_int_t,  TTreeNodes_GetCount,                     (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNodes_GetItem,                      (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_obj_t,  TTreeNodes_GetFirstNode,                 (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNodes_FindNodeWithText,             (no_vcl_obj_t o, no_vcl_str_t t),                     (o, t)) \
    X(void,          TTreeNodes_BeginUpdate,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNodes_EndUpdate,                    (no_vcl_obj_t o),                                     (o)) \
    \
    X(no_vcl_str_t,  TTreeNode_GetText,                       (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNode_SetText,                       (no_vcl_obj_t o, no_vcl_str_t v),                     (o, v)) \
    X(no_vcl_bool_t, TTreeNode_GetExpanded,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNode_SetExpanded,                   (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TTreeNode_GetSelected,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNode_SetSelected,                   (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TTreeNode_GetHasChildren,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNode_SetHasChildren,                (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(void*,         TTreeNode_GetData,                       (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNode_SetData,                       (no_vcl_obj_t o, void* v),                            (o, v)) \
    X(no_vcl_int_t,  TTreeNode_GetCount,                      (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetItem,                       (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_int_t,  TTreeNode_GetIndex,                      (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TTreeNode_GetLevel,                      (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TTreeNode_GetAbsoluteIndex,              (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetParent,                     (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetTreeView,                   (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetFirstChild,                 (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetLastChild,                  (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetNextSibling,                (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetPrevSibling,                (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetNext,                       (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TTreeNode_GetPrev,                       (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TTreeNode_IndexOf,                       (no_vcl_obj_t o, no_vcl_obj_t n),                     (o, n)) \
    X(void,          TTreeNode_Expand,                        (no_vcl_obj_t o, no_vcl_bool_t r),                    (o, r)) \
    X(void,          TTreeNode_Collapse,                      (no_vcl_obj_t o, no_vcl_bool_t r),                    (o, r)) \
    X(void,          TTreeNode_Delete,                        (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNode_DeleteChildren,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNode_MakeVisible,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TTreeNode_MoveTo,                        (no_vcl_obj_t o, no_vcl_obj_t d, no_vcl_int_t m),     (o, d, m)) \
    \
    X(no_vcl_obj_t,  TListView_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_obj_t,  TCustomListView_GetItems,                (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TCustomListView_GetSelected,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_SetSelected,             (no_vcl_obj_t o, no_vcl_obj_t i),                     (o, i)) \
    X(no_vcl_int_t,  TCustomListView_GetItemIndex,            (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_SetItemIndex,            (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomListView_GetSelCount,             (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_bool_t, TCustomListView_GetCheckboxes,           (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_SetCheckboxes,           (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TCustomListView_GetGridLines,            (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_SetGridLines,            (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TCustomListView_GetMultiSelect,          (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_SetMultiSelect,          (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TCustomListView_GetReadOnly,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_SetReadOnly,             (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TCustomListView_GetRowSelect,            (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_SetRowSelect,            (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(void,          TCustomListView_Clear,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_BeginUpdate,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_EndUpdate,               (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TCustomListView_GetItemAt,               (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y),     (o, x, y)) \
    X(void,          TCustomListView_ClearSelection,          (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomListView_SelectAll,               (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TListView_GetColumns,                    (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TListView_GetViewStyle,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListView_SetViewStyle,                  (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_bool_t, TListView_GetHideSelection,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListView_SetHideSelection,              (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TListView_GetSortType,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListView_SetSortType,                   (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TListView_GetSortColumn,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListView_SetSortColumn,                 (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TListView_GetSortDirection,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListView_SetSortDirection,              (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(void,          TListView_SetOnSelectItem,               (no_vcl_obj_t o, no_vcl_item_int_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TListView_SetOnChange,                   (no_vcl_obj_t o, no_vcl_item_int_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TListView_SetOnDeletion,                 (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TListView_SetOnItemChecked,              (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TListView_SetOnColumnClick,              (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    \
    X(no_vcl_obj_t,  TListItems_Add,                          (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TListItems_Insert,                       (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TListItems_Delete,                       (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TListItems_Clear,                        (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TListItems_GetCount,                     (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TListItems_GetItem,                      (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_int_t,  TListItems_IndexOf,                      (no_vcl_obj_t o, no_vcl_obj_t i),                     (o, i)) \
    X(no_vcl_obj_t,  TListItems_FindCaption,                  (no_vcl_obj_t o, no_vcl_int_t s, no_vcl_str_t v, no_vcl_bool_t p, no_vcl_bool_t inc, no_vcl_bool_t w), (o, s, v, p, inc, w)) \
    X(void,          TListItems_Exchange,                     (no_vcl_obj_t o, no_vcl_int_t a, no_vcl_int_t b),     (o, a, b)) \
    X(void,          TListItems_Move,                         (no_vcl_obj_t o, no_vcl_int_t a, no_vcl_int_t b),     (o, a, b)) \
    X(void,          TListItems_BeginUpdate,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListItems_EndUpdate,                    (no_vcl_obj_t o),                                     (o)) \
    \
    X(no_vcl_str_t,  TListItem_GetCaption,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListItem_SetCaption,                    (no_vcl_obj_t o, no_vcl_str_t v),                     (o, v)) \
    X(no_vcl_bool_t, TListItem_GetChecked,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListItem_SetChecked,                    (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TListItem_GetSelected,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListItem_SetSelected,                   (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TListItem_GetFocused,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListItem_SetFocused,                    (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(void*,         TListItem_GetData,                       (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListItem_SetData,                       (no_vcl_obj_t o, void* v),                            (o, v)) \
    X(no_vcl_int_t,  TListItem_GetIndex,                      (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TListItem_GetListView,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListItem_SubItems_Add,                  (no_vcl_obj_t o, no_vcl_str_t t),                     (o, t)) \
    X(void,          TListItem_SubItems_Clear,                (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TListItem_SubItems_Count,                (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_str_t,  TListItem_SubItems_GetText,              (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TListItem_SubItems_SetText,              (no_vcl_obj_t o, no_vcl_int_t i, no_vcl_str_t t),     (o, i, t)) \
    X(void,          TListItem_Delete,                        (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListItem_MakeVisible,                   (no_vcl_obj_t o, no_vcl_bool_t p),                    (o, p)) \
    \
    X(no_vcl_obj_t,  TListColumns_Add,                        (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TListColumns_GetCount,                   (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TListColumns_GetItem,                    (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TListColumns_Delete,                     (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TListColumns_Clear,                      (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_str_t,  TListColumn_GetCaption,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListColumn_SetCaption,                  (no_vcl_obj_t o, no_vcl_str_t v),                     (o, v)) \
    X(no_vcl_int_t,  TListColumn_GetWidth,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListColumn_SetWidth,                    (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TListColumn_GetAlignment,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListColumn_SetAlignment,                (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_bool_t, TListColumn_GetAutoSize,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListColumn_SetAutoSize,                 (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TListColumn_GetVisible,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListColumn_SetVisible,                  (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TListColumn_GetIndex,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TListColumn_SetIndex,                    (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    \
    X(no_vcl_obj_t,  TDrawGrid_Create,                        (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_obj_t,  TStringGrid_Create,                      (no_vcl_obj_t owner),                                 (owner)) \
    X(void,          TCustomGrid_BeginUpdate,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomGrid_EndUpdate,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomGrid_Clear,                       (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomGrid_CellRect,                    (no_vcl_obj_t o, no_vcl_int_t c, no_vcl_int_t r, no_vcl_int_t* l, no_vcl_int_t* t, no_vcl_int_t* rt, no_vcl_int_t* b), (o, c, r, l, t, rt, b)) \
    X(void,          TCustomGrid_MouseToCell,                 (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y, no_vcl_int_t* c, no_vcl_int_t* r), (o, x, y, c, r)) \
    X(no_vcl_obj_t,  TCustomDrawGrid_GetCanvas,               (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetColCount,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetColCount,             (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetRowCount,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetRowCount,             (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetFixedCols,            (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetFixedCols,            (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetFixedRows,            (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetFixedRows,            (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetCol,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetCol,                  (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetRow,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetRow,                  (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetDefaultColWidth,      (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetDefaultColWidth,      (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetDefaultRowHeight,     (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetDefaultRowHeight,     (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetColWidths,            (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TCustomDrawGrid_SetColWidths,            (no_vcl_obj_t o, no_vcl_int_t i, no_vcl_int_t v),     (o, i, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetRowHeights,           (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          TCustomDrawGrid_SetRowHeights,           (no_vcl_obj_t o, no_vcl_int_t i, no_vcl_int_t v),     (o, i, v)) \
    X(no_vcl_uint_t, TCustomDrawGrid_GetOptions,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetOptions,              (no_vcl_obj_t o, no_vcl_uint_t v),                    (o, v)) \
    X(void,          TCustomDrawGrid_GetSelection,            (no_vcl_obj_t o, no_vcl_int_t* l, no_vcl_int_t* t, no_vcl_int_t* r, no_vcl_int_t* b), (o, l, t, r, b)) \
    X(void,          TCustomDrawGrid_SetSelection,            (no_vcl_obj_t o, no_vcl_int_t l, no_vcl_int_t t, no_vcl_int_t r, no_vcl_int_t b), (o, l, t, r, b)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetLeftCol,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetLeftCol,              (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetTopRow,               (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetTopRow,               (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_bool_t, TCustomDrawGrid_GetDefaultDrawing,       (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetDefaultDrawing,       (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TCustomDrawGrid_GetFixedColor,           (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetFixedColor,           (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_bool_t, TCustomDrawGrid_GetEditorMode,           (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomDrawGrid_SetEditorMode,           (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(void,          TCustomDrawGrid_InsertColRow,            (no_vcl_obj_t o, no_vcl_bool_t c, no_vcl_int_t i),    (o, c, i)) \
    X(void,          TCustomDrawGrid_DeleteColRow,            (no_vcl_obj_t o, no_vcl_bool_t c, no_vcl_int_t i),    (o, c, i)) \
    X(void,          TCustomDrawGrid_MoveColRow,              (no_vcl_obj_t o, no_vcl_bool_t c, no_vcl_int_t f, no_vcl_int_t t), (o, c, f, t)) \
    X(void,          TCustomDrawGrid_SortColRow,              (no_vcl_obj_t o, no_vcl_bool_t c, no_vcl_int_t i),    (o, c, i)) \
    X(void,          TCustomDrawGrid_SetOnDrawCell,           (no_vcl_obj_t o, no_vcl_draw_cell_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomDrawGrid_SetOnSelectCell,         (no_vcl_obj_t o, no_vcl_cell_allow_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomDrawGrid_SetOnSelection,          (no_vcl_obj_t o, no_vcl_cell_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomDrawGrid_SetOnHeaderClick,        (no_vcl_obj_t o, no_vcl_header_callback_t cb, void* d), (o, cb, d)) \
    X(no_vcl_str_t,  TCustomStringGrid_GetCells,              (no_vcl_obj_t o, no_vcl_int_t c, no_vcl_int_t r),     (o, c, r)) \
    X(void,          TCustomStringGrid_SetCells,              (no_vcl_obj_t o, no_vcl_int_t c, no_vcl_int_t r, no_vcl_str_t v), (o, c, r, v)) \
    X(void,          TCustomStringGrid_Clean,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomStringGrid_AutoSizeColumns,       (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomStringGrid_AutoSizeColumn,        (no_vcl_obj_t o, no_vcl_int_t c),                     (o, c)) \
    X(no_vcl_obj_t,  THeaderControl_Create,                   (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_obj_t,  TCustomHeaderControl_GetSections,        (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_bool_t, TCustomHeaderControl_GetDragReorder,     (no_vcl_obj_t o),                                     (o)) \
    X(void,          TCustomHeaderControl_SetDragReorder,     (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TCustomHeaderControl_GetSectionAt,       (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y),     (o, x, y)) \
    X(no_vcl_obj_t,  TCustomHeaderControl_GetSectionFromOriginalIndex, (no_vcl_obj_t o, no_vcl_int_t i),            (o, i)) \
    X(void,          TCustomHeaderControl_SetOnSectionClick,  (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionResize, (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionSeparatorDblClick, (no_vcl_obj_t o, no_vcl_item_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionTrack,  (no_vcl_obj_t o, no_vcl_section_track_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionDrag,   (no_vcl_obj_t o, no_vcl_section_drag_callback_t cb, void* d), (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionEndDrag, (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),     (o, cb, d)) \
    X(no_vcl_obj_t,  THeaderSections_Add,                     (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  THeaderSections_Insert,                  (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          THeaderSections_Delete,                  (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          THeaderSections_Clear,                   (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  THeaderSections_GetCount,                (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  THeaderSections_GetItem,                 (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(void,          THeaderSections_BeginUpdate,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          THeaderSections_EndUpdate,               (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_str_t,  THeaderSection_GetText,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          THeaderSection_SetText,                  (no_vcl_obj_t o, no_vcl_str_t v),                     (o, v)) \
    X(no_vcl_int_t,  THeaderSection_GetWidth,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          THeaderSection_SetWidth,                 (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  THeaderSection_GetMinWidth,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          THeaderSection_SetMinWidth,              (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  THeaderSection_GetMaxWidth,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          THeaderSection_SetMaxWidth,              (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  THeaderSection_GetAlignment,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          THeaderSection_SetAlignment,             (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_bool_t, THeaderSection_GetVisible,               (no_vcl_obj_t o),                                     (o)) \
    X(void,          THeaderSection_SetVisible,               (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  THeaderSection_GetIndex,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          THeaderSection_SetIndex,                 (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  THeaderSection_GetLeft,                  (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  THeaderSection_GetRight,                 (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  THeaderSection_GetOriginalIndex,         (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_uint_t, TToolWindow_GetEdgeBorders,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolWindow_SetEdgeBorders,              (no_vcl_obj_t o, no_vcl_uint_t v),                    (o, v)) \
    X(no_vcl_int_t,  TToolWindow_GetEdgeInner,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolWindow_SetEdgeInner,                (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TToolWindow_GetEdgeOuter,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolWindow_SetEdgeOuter,                (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(void,          TToolWindow_BeginUpdate,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolWindow_EndUpdate,                   (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TToolBar_Create,                         (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_int_t,  TToolBar_GetButtonCount,                 (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_obj_t,  TToolBar_GetButton,                      (no_vcl_obj_t o, no_vcl_int_t i),                     (o, i)) \
    X(no_vcl_int_t,  TToolBar_GetRowCount,                    (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_int_t,  TToolBar_GetButtonHeight,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetButtonHeight,                (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TToolBar_GetButtonWidth,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetButtonWidth,                 (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TToolBar_GetDropDownWidth,               (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetDropDownWidth,               (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_int_t,  TToolBar_GetIndent,                      (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetIndent,                      (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_bool_t, TToolBar_GetFlat,                        (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetFlat,                        (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolBar_GetList,                        (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetList,                        (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolBar_GetShowCaptions,                (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetShowCaptions,                (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolBar_GetTransparent,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetTransparent,                 (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolBar_GetWrapable,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolBar_SetWrapable,                    (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(void,          TToolBar_SetButtonSize,                  (no_vcl_obj_t o, no_vcl_int_t w, no_vcl_int_t h),     (o, w, h)) \
    X(no_vcl_obj_t,  TToolButton_Create,                      (no_vcl_obj_t owner),                                 (owner)) \
    X(no_vcl_bool_t, TToolButton_GetAllowAllUp,               (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetAllowAllUp,               (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolButton_GetDown,                     (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetDown,                     (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolButton_GetGrouped,                  (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetGrouped,                  (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolButton_GetIndeterminate,            (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetIndeterminate,            (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolButton_GetMarked,                   (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetMarked,                   (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolButton_GetShowCaption,              (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetShowCaption,              (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_bool_t, TToolButton_GetWrap,                     (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetWrap,                     (no_vcl_obj_t o, no_vcl_bool_t v),                    (o, v)) \
    X(no_vcl_int_t,  TToolButton_GetStyle,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetStyle,                    (no_vcl_obj_t o, no_vcl_int_t v),                     (o, v)) \
    X(no_vcl_obj_t,  TToolButton_GetDropdownMenu,             (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetDropdownMenu,             (no_vcl_obj_t o, no_vcl_obj_t v),                     (o, v)) \
    X(no_vcl_obj_t,  TToolButton_GetMenuItem,                 (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_SetMenuItem,                 (no_vcl_obj_t o, no_vcl_obj_t v),                     (o, v)) \
    X(no_vcl_int_t,  TToolButton_GetIndex,                    (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_Click,                       (no_vcl_obj_t o),                                     (o)) \
    X(void,          TToolButton_ArrowClick,                  (no_vcl_obj_t o),                                     (o)) \
    X(no_vcl_bool_t, TToolButton_PointInArrow,                (no_vcl_obj_t o, no_vcl_int_t x, no_vcl_int_t y),     (o, x, y)) \
    X(void,          TToolButton_SetOnArrowClick,             (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d))

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
void no_vcl_map(Func& f, no_vcl_module_t m, const char* n)
{
#if defined(_WIN32) || defined(_WIN64)
    void* p = reinterpret_cast<void*>(::GetProcAddress(m, n));
#else
    void* p = ::dlsym(m, n);
#endif
    f = reinterpret_cast<Func>(p);
}

void no_vcl_init(void)
{
    static no_vcl_module_t m = nullptr;
    static std::once_flag once;

    if (!m)
    {
        std::call_once(once, [&](){
#if defined(_WIN32) || defined(_WIN64)
            m = ::LoadLibraryA("no_vcl.dll");
#else
            m = ::dlopen("libno_vcl.so", RTLD_LAZY);
#endif
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
