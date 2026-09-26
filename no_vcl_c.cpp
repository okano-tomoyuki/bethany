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
    X(void,          TCustomCheckListBox_SetOnClickCheck,    (no_vcl_obj_t o, no_vcl_callback_t cb, void* d),      (o, cb, d))

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
