#ifndef BETH_INTERNAL_FUNCS_H
#define BETH_INTERNAL_FUNCS_H

// DLL(beth.dll / libbeth.so)の公開関数の一覧(戻り値型, 名前, 引数リスト, 呼び出し時の引数)。docs/adr/0032。
// 型は beth::internal の型(internal/api.h)で、この一覧は必ず namespace beth::internal の中で展開する。
// ここに 1 行追加すると、宣言(internal/api.h)と、関数ポインタ・読み込み・呼び出しの後にエラーを確かめて
// Exception を送出する中継関数(internal/api.cpp)がまとめて生成される。
// 名前は DLL の公開名(Pascal 側の exports)と同じ。DLL の関数の決まり(呼び出し規約・文字列・コールバック・例外)は
// docs/dll-abi.md を参照。
#define BETH_FUNCS(X) \
    X(void,          SetCallbackError,                      (str_t c, str_t m),                                            (c, m)) \
    X(void,          FreeNotify_SetCallback,                (callback_t cb, void* d),                                      (cb, d)) \
    X(void,          TComponent_Destroy,                    (obj_t o),                                                     (o)) \
    X(void,          TComponent_DestroyComponents,          (obj_t o),                                                     (o)) \
    X(obj_t,         TControl_GetParent,                    (obj_t o),                                                     (o)) \
    X(void,          TControl_SetParent,                    (obj_t o, obj_t p),                                            (o, p)) \
    X(int_t,         TControl_GetLeft,                      (obj_t o),                                                     (o)) \
    X(void,          TControl_SetLeft,                      (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControl_GetTop,                       (obj_t o),                                                     (o)) \
    X(void,          TControl_SetTop,                       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControl_GetWidth,                     (obj_t o),                                                     (o)) \
    X(void,          TControl_SetWidth,                     (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControl_GetHeight,                    (obj_t o),                                                     (o)) \
    X(void,          TControl_SetHeight,                    (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TControl_GetVisible,                   (obj_t o),                                                     (o)) \
    X(void,          TControl_SetVisible,                   (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TControl_GetEnabled,                   (obj_t o),                                                     (o)) \
    X(void,          TControl_SetEnabled,                   (obj_t o, bool_t v),                                           (o, v)) \
    X(str_t,         TControl_GetCaption,                   (obj_t o),                                                     (o)) \
    X(void,          TControl_SetCaption,                   (obj_t o, str_t v),                                            (o, v)) \
    X(int_t,         TControl_GetAlign,                     (obj_t o),                                                     (o)) \
    X(void,          TControl_SetAlign,                     (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TControl_GetAutoSize,                  (obj_t o),                                                     (o)) \
    X(void,          TControl_SetAutoSize,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(str_t,         TControl_GetText,                      (obj_t o),                                                     (o)) \
    X(void,          TControl_SetText,                      (obj_t o, str_t v),                                            (o, v)) \
    X(void,          TControl_Show,                         (obj_t o),                                                     (o)) \
    X(void,          TControl_Hide,                         (obj_t o),                                                     (o)) \
    X(void,          TControl_SetOnClick,                   (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TControl_SetOnDblClick,                (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TControl_SetOnResize,                  (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TControl_SetOnMouseDown,               (obj_t o, mouse_callback_t cb, void* d),                       (o, cb, d)) \
    X(void,          TControl_SetOnMouseUp,                 (obj_t o, mouse_callback_t cb, void* d),                       (o, cb, d)) \
    X(void,          TControl_SetOnMouseMove,               (obj_t o, mouse_move_callback_t cb, void* d),                  (o, cb, d)) \
    X(void,          TControl_SetOnMouseEnter,              (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TControl_SetOnMouseLeave,              (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TControl_SetOnMouseWheel,              (obj_t o, mouse_wheel_callback_t cb, void* d),                 (o, cb, d)) \
    X(void,          TWinControl_SetOnKeyDown,              (obj_t o, key_callback_t cb, void* d),                         (o, cb, d)) \
    X(void,          TWinControl_SetOnKeyUp,                (obj_t o, key_callback_t cb, void* d),                         (o, cb, d)) \
    X(void,          TWinControl_SetOnKeyPress,             (obj_t o, key_press_callback_t cb, void* d),                   (o, cb, d)) \
    X(obj_t,         TForm_Create,                          (obj_t owner),                                                 (owner)) \
    X(void,          TCustomForm_Show,                      (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_Hide,                      (obj_t o),                                                     (o)) \
    X(int_t,         TCustomForm_ShowModal,                 (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_Close,                     (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_Release,                   (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetOnClose,                (obj_t o, close_callback_t cb, void* d),                       (o, cb, d)) \
    X(void,          TCustomForm_SetOnCloseQuery,           (obj_t o, close_query_callback_t cb, void* d),                 (o, cb, d)) \
    X(void,          TCustomForm_SetOnShow,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCustomForm_SetOnHide,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCustomForm_SetOnActivate,             (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCustomForm_SetOnDeactivate,           (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCustomForm_SetOnDestroy,              (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         GetApplication,                        (void),                                                        ()) \
    X(obj_t,         TApplication_CreateForm,               (obj_t o),                                                     (o)) \
    X(obj_t,         TApplication_GetMainForm,              (obj_t o),                                                     (o)) \
    X(void,          TApplication_Run,                      (obj_t o),                                                     (o)) \
    X(void,          TApplication_ProcessMessages,          (obj_t o),                                                     (o)) \
    X(void,          TApplication_Terminate,                (obj_t o),                                                     (o)) \
    X(bool_t,        TApplication_GetTerminated,            (obj_t o),                                                     (o)) \
    X(str_t,         TApplication_GetTitle,                 (obj_t o),                                                     (o)) \
    X(void,          TApplication_SetTitle,                 (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TApplication_GetShowMainForm,          (obj_t o),                                                     (o)) \
    X(void,          TApplication_SetShowMainForm,          (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TPanel_Create,                         (obj_t owner),                                                 (owner)) \
    X(obj_t,         TGroupBox_Create,                      (obj_t owner),                                                 (owner)) \
    X(obj_t,         TLabel_Create,                         (obj_t owner),                                                 (owner)) \
    X(bool_t,        TButtonControl_GetChecked,             (obj_t o),                                                     (o)) \
    X(void,          TButtonControl_SetChecked,             (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TButton_Create,                        (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCheckBox_Create,                      (obj_t owner),                                                 (owner)) \
    X(obj_t,         TRadioButton_Create,                   (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomEdit_GetMaxLength,              (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetMaxLength,              (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomEdit_GetReadOnly,               (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetReadOnly,               (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomEdit_SetOnChange,               (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TEdit_Create,                          (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomMemo_GetLines,                  (obj_t o),                                                     (o)) \
    X(int_t,         TCustomMemo_GetScrollBars,             (obj_t o),                                                     (o)) \
    X(void,          TCustomMemo_SetScrollBars,             (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TMemo_Create,                          (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomComboBox_GetItemIndex,          (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetItemIndex,          (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomComboBox_GetItems,              (obj_t o),                                                     (o)) \
    X(obj_t,         TComboBox_Create,                      (obj_t owner),                                                 (owner)) \
    X(void,          TComboBox_SetOnChange,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(int_t,         TCustomListBox_GetItemIndex,           (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SetItemIndex,           (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomListBox_GetItems,               (obj_t o),                                                     (o)) \
    X(obj_t,         TListBox_Create,                       (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomTimer_GetInterval,              (obj_t o),                                                     (o)) \
    X(void,          TCustomTimer_SetInterval,              (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomTimer_GetEnabled,               (obj_t o),                                                     (o)) \
    X(void,          TCustomTimer_SetEnabled,               (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomTimer_SetOnTimer,               (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TTimer_Create,                         (obj_t owner),                                                 (owner)) \
    X(obj_t,         TPaintBox_Create,                      (obj_t owner),                                                 (owner)) \
    X(obj_t,         TPaintBox_GetCanvas,                   (obj_t o),                                                     (o)) \
    X(void,          TPaintBox_SetOnPaint,                  (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCanvas_MoveTo,                        (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(void,          TCanvas_LineTo,                        (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(void,          TCanvas_Rectangle,                     (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2),             (o, x1, y1, x2, y2)) \
    X(void,          TCanvas_Ellipse,                       (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2),             (o, x1, y1, x2, y2)) \
    X(void,          TCanvas_TextOut,                       (obj_t o, int_t x, int_t y, str_t s),                          (o, x, y, s)) \
    X(obj_t,         TCanvas_GetPen,                        (obj_t o),                                                     (o)) \
    X(obj_t,         TCanvas_GetBrush,                      (obj_t o),                                                     (o)) \
    X(obj_t,         TCanvas_GetFont,                       (obj_t o),                                                     (o)) \
    X(void,          TCanvas_Draw,                          (obj_t o, int_t x, int_t y, obj_t graphic),                    (o, x, y, graphic)) \
    X(void,          TCanvas_StretchDraw,                   (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2, obj_t graphic), (o, x1, y1, x2, y2, graphic)) \
    X(void,          TCanvas_FillRect,                      (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2),             (o, x1, y1, x2, y2)) \
    X(int_t,         TCanvas_GetPixels,                     (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(void,          TCanvas_SetPixels,                     (obj_t o, int_t x, int_t y, int_t v),                          (o, x, y, v)) \
    X(int_t,         TPen_GetColor,                         (obj_t o),                                                     (o)) \
    X(void,          TPen_SetColor,                         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TPen_GetWidth,                         (obj_t o),                                                     (o)) \
    X(void,          TPen_SetWidth,                         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TBrush_GetColor,                       (obj_t o),                                                     (o)) \
    X(void,          TBrush_SetColor,                       (obj_t o, int_t v),                                            (o, v)) \
    X(str_t,         TFont_GetName,                         (obj_t o),                                                     (o)) \
    X(void,          TFont_SetName,                         (obj_t o, str_t v),                                            (o, v)) \
    X(int_t,         TFont_GetSize,                         (obj_t o),                                                     (o)) \
    X(void,          TFont_SetSize,                         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TFont_GetColor,                        (obj_t o),                                                     (o)) \
    X(void,          TFont_SetColor,                        (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TScrollBox_Create,                     (obj_t owner),                                                 (owner)) \
    X(obj_t,         TToggleBox_Create,                     (obj_t owner),                                                 (owner)) \
    X(obj_t,         TBevel_Create,                         (obj_t owner),                                                 (owner)) \
    X(int_t,         TBevel_GetShape,                       (obj_t o),                                                     (o)) \
    X(void,          TBevel_SetShape,                       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TBevel_GetStyle,                       (obj_t o),                                                     (o)) \
    X(void,          TBevel_SetStyle,                       (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TShape_Create,                         (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomShape_GetShape,                 (obj_t o),                                                     (o)) \
    X(void,          TCustomShape_SetShape,                 (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomShape_GetPen,                   (obj_t o),                                                     (o)) \
    X(obj_t,         TCustomShape_GetBrush,                 (obj_t o),                                                     (o)) \
    X(obj_t,         TStaticText_Create,                    (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomStaticText_GetBorderStyle,      (obj_t o),                                                     (o)) \
    X(void,          TCustomStaticText_SetBorderStyle,      (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TStatusBar_Create,                     (obj_t owner),                                                 (owner)) \
    X(str_t,         TStatusBar_GetSimpleText,              (obj_t o),                                                     (o)) \
    X(void,          TStatusBar_SetSimpleText,              (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TStatusBar_GetSimplePanel,             (obj_t o),                                                     (o)) \
    X(void,          TStatusBar_SetSimplePanel,             (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TScrollBar_Create,                     (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomScrollBar_GetKind,              (obj_t o),                                                     (o)) \
    X(void,          TCustomScrollBar_SetKind,              (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomScrollBar_GetMin,               (obj_t o),                                                     (o)) \
    X(void,          TCustomScrollBar_SetMin,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomScrollBar_GetMax,               (obj_t o),                                                     (o)) \
    X(void,          TCustomScrollBar_SetMax,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomScrollBar_GetPosition,          (obj_t o),                                                     (o)) \
    X(void,          TCustomScrollBar_SetPosition,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomScrollBar_GetPageSize,          (obj_t o),                                                     (o)) \
    X(void,          TCustomScrollBar_SetPageSize,          (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TCustomScrollBar_SetOnChange,          (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TTrackBar_Create,                      (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomTrackBar_GetMin,                (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetMin,                (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetMax,                (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetMax,                (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetPosition,           (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetPosition,           (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TCustomTrackBar_SetOnChange,           (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TProgressBar_Create,                   (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomProgressBar_GetMin,             (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_SetMin,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomProgressBar_GetMax,             (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_SetMax,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomProgressBar_GetPosition,        (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_SetPosition,        (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TUpDown_Create,                        (obj_t owner),                                                 (owner)) \
    X(int_t,         TUpDown_GetMin,                        (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetMin,                        (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TUpDown_GetMax,                        (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetMax,                        (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TUpDown_GetPosition,                   (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetPosition,                   (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TUpDown_GetIncrement,                  (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetIncrement,                  (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TUpDown_GetAssociate,                  (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetAssociate,                  (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TRadioGroup_Create,                    (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomRadioGroup_GetItems,            (obj_t o),                                                     (o)) \
    X(int_t,         TCustomRadioGroup_GetItemIndex,        (obj_t o),                                                     (o)) \
    X(void,          TCustomRadioGroup_SetItemIndex,        (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TCustomRadioGroup_SetOnClick,          (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TCheckGroup_Create,                    (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomCheckGroup_GetItems,            (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomCheckGroup_GetChecked,          (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCustomCheckGroup_SetChecked,          (obj_t o, int_t i, bool_t v),                                  (o, i, v)) \
    X(obj_t,         TCheckListBox_Create,                  (obj_t owner),                                                 (owner)) \
    X(bool_t,        TCustomCheckListBox_GetChecked,        (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCustomCheckListBox_SetChecked,        (obj_t o, int_t i, bool_t v),                                  (o, i, v)) \
    X(void,          TCustomCheckListBox_SetOnClickCheck,   (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TSpeedButton_Create,                   (obj_t owner),                                                 (owner)) \
    X(bool_t,        TCustomSpeedButton_GetDown,            (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetDown,            (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomSpeedButton_GetGroupIndex,      (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetGroupIndex,      (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomSpeedButton_GetFlat,            (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetFlat,            (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomSpeedButton_GetAllowAllUp,      (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetAllowAllUp,      (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TBitBtn_Create,                        (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomBitBtn_GetKind,                 (obj_t o),                                                     (o)) \
    X(void,          TCustomBitBtn_SetKind,                 (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TFloatSpinEdit_Create,                 (obj_t owner),                                                 (owner)) \
    X(real_t,        TCustomFloatSpinEdit_GetValue,         (obj_t o),                                                     (o)) \
    X(void,          TCustomFloatSpinEdit_SetValue,         (obj_t o, real_t v),                                           (o, v)) \
    X(real_t,        TCustomFloatSpinEdit_GetMinValue,      (obj_t o),                                                     (o)) \
    X(void,          TCustomFloatSpinEdit_SetMinValue,      (obj_t o, real_t v),                                           (o, v)) \
    X(real_t,        TCustomFloatSpinEdit_GetMaxValue,      (obj_t o),                                                     (o)) \
    X(void,          TCustomFloatSpinEdit_SetMaxValue,      (obj_t o, real_t v),                                           (o, v)) \
    X(real_t,        TCustomFloatSpinEdit_GetIncrement,     (obj_t o),                                                     (o)) \
    X(void,          TCustomFloatSpinEdit_SetIncrement,     (obj_t o, real_t v),                                           (o, v)) \
    X(int_t,         TCustomFloatSpinEdit_GetDecimalPlaces, (obj_t o),                                                     (o)) \
    X(void,          TCustomFloatSpinEdit_SetDecimalPlaces, (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TSpinEdit_Create,                      (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomSpinEdit_GetValue,              (obj_t o),                                                     (o)) \
    X(void,          TCustomSpinEdit_SetValue,              (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSpinEdit_GetMinValue,           (obj_t o),                                                     (o)) \
    X(void,          TCustomSpinEdit_SetMinValue,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSpinEdit_GetMaxValue,           (obj_t o),                                                     (o)) \
    X(void,          TCustomSpinEdit_SetMaxValue,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSpinEdit_GetIncrement,          (obj_t o),                                                     (o)) \
    X(void,          TCustomSpinEdit_SetIncrement,          (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TMaskEdit_Create,                      (obj_t owner),                                                 (owner)) \
    X(str_t,         TMaskEdit_GetEditMask,                 (obj_t o),                                                     (o)) \
    X(void,          TMaskEdit_SetEditMask,                 (obj_t o, str_t v),                                            (o, v)) \
    X(obj_t,         TLabeledEdit_Create,                   (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomLabeledEdit_GetEditLabel,       (obj_t o),                                                     (o)) \
    X(int_t,         TCustomLabeledEdit_GetLabelPosition,   (obj_t o),                                                     (o)) \
    X(void,          TCustomLabeledEdit_SetLabelPosition,   (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomLabeledEdit_GetLabelSpacing,    (obj_t o),                                                     (o)) \
    X(void,          TCustomLabeledEdit_SetLabelSpacing,    (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TTabControl_Create,                    (obj_t owner),                                                 (owner)) \
    X(obj_t,         TTabControl_GetTabs,                   (obj_t o),                                                     (o)) \
    X(int_t,         TTabControl_GetTabIndex,               (obj_t o),                                                     (o)) \
    X(void,          TTabControl_SetTabIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TTabControl_SetOnChange,               (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TSplitter_Create,                      (obj_t owner),                                                 (owner)) \
    X(bool_t,        TCustomSplitter_GetAutoSnap,           (obj_t o),                                                     (o)) \
    X(void,          TCustomSplitter_SetAutoSnap,           (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomSplitter_GetBeveled,            (obj_t o),                                                     (o)) \
    X(void,          TCustomSplitter_SetBeveled,            (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomSplitter_GetMinSize,            (obj_t o),                                                     (o)) \
    X(void,          TCustomSplitter_SetMinSize,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSplitter_GetResizeAnchor,       (obj_t o),                                                     (o)) \
    X(void,          TCustomSplitter_SetResizeAnchor,       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSplitter_GetResizeStyle,        (obj_t o),                                                     (o)) \
    X(void,          TCustomSplitter_SetResizeStyle,        (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSplitter_GetSplitterPosition,   (obj_t o),                                                     (o)) \
    X(void,          TCustomSplitter_SetSplitterPosition,   (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TCustomSplitter_SetOnMoved,            (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(int_t,         ShortCut_Make,                         (int_t k, int_t s),                                            (k, s)) \
    X(int_t,         ShortCut_FromText,                     (str_t t),                                                     (t)) \
    X(str_t,         ShortCut_ToText,                       (int_t v),                                                     (v)) \
    X(obj_t,         TMenuItem_Create,                      (obj_t owner),                                                 (owner)) \
    X(str_t,         TMenuItem_GetCaption,                  (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetCaption,                  (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TMenuItem_GetChecked,                  (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetChecked,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TMenuItem_GetEnabled,                  (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetEnabled,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TMenuItem_GetVisible,                  (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetVisible,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TMenuItem_GetAutoCheck,                (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetAutoCheck,                (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TMenuItem_GetRadioItem,                (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetRadioItem,                (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TMenuItem_GetGroupIndex,               (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetGroupIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TMenuItem_GetDefault,                  (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetDefault,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TMenuItem_GetShortCut,                 (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetShortCut,                 (obj_t o, int_t v),                                            (o, v)) \
    X(str_t,         TMenuItem_GetHint,                     (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetHint,                     (obj_t o, str_t v),                                            (o, v)) \
    X(void,          TMenuItem_SetOnClick,                  (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(int_t,         TMenuItem_GetCount,                    (obj_t o),                                                     (o)) \
    X(obj_t,         TMenuItem_GetItem,                     (obj_t o, int_t i),                                            (o, i)) \
    X(obj_t,         TMenuItem_GetParent,                   (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_Add,                         (obj_t o, obj_t item),                                         (o, item)) \
    X(void,          TMenuItem_Insert,                      (obj_t o, int_t i, obj_t item),                                (o, i, item)) \
    X(void,          TMenuItem_Delete,                      (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TMenuItem_Remove,                      (obj_t o, obj_t item),                                         (o, item)) \
    X(void,          TMenuItem_Clear,                       (obj_t o),                                                     (o)) \
    X(int_t,         TMenuItem_IndexOf,                     (obj_t o, obj_t item),                                         (o, item)) \
    X(void,          TMenuItem_AddSeparator,                (obj_t o),                                                     (o)) \
    X(bool_t,        TMenuItem_IsLine,                      (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_Click,                       (obj_t o),                                                     (o)) \
    X(obj_t,         TMenu_GetItems,                        (obj_t o),                                                     (o)) \
    X(obj_t,         TMainMenu_Create,                      (obj_t owner),                                                 (owner)) \
    X(obj_t,         TPopupMenu_Create,                     (obj_t owner),                                                 (owner)) \
    X(void,          TPopupMenu_Popup,                      (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(bool_t,        TPopupMenu_GetAutoPopup,               (obj_t o),                                                     (o)) \
    X(void,          TPopupMenu_SetAutoPopup,               (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TPopupMenu_GetPopupComponent,          (obj_t o),                                                     (o)) \
    X(void,          TPopupMenu_SetPopupComponent,          (obj_t o, obj_t v),                                            (o, v)) \
    X(void,          TPopupMenu_SetOnPopup,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TPopupMenu_SetOnClose,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TCustomForm_GetMenu,                   (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetMenu,                   (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TControl_GetPopupMenu,                 (obj_t o),                                                     (o)) \
    X(void,          TControl_SetPopupMenu,                 (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TPageControl_Create,                   (obj_t owner),                                                 (owner)) \
    X(obj_t,         TPageControl_GetActivePage,            (obj_t o),                                                     (o)) \
    X(void,          TPageControl_SetActivePage,            (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TPageControl_GetActivePageIndex,       (obj_t o),                                                     (o)) \
    X(void,          TPageControl_SetActivePageIndex,       (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TPageControl_GetPage,                  (obj_t o, int_t i),                                            (o, i)) \
    X(int_t,         TCustomTabControl_GetPageCount,        (obj_t o),                                                     (o)) \
    X(obj_t,         TPageControl_AddTabSheet,              (obj_t o),                                                     (o)) \
    X(void,          TPageControl_Clear,                    (obj_t o),                                                     (o)) \
    X(void,          TPageControl_SelectNextPage,           (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TPageControl_GetTabIndex,              (obj_t o),                                                     (o)) \
    X(void,          TPageControl_SetTabIndex,              (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TPageControl_SetOnChange,              (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCustomTabControl_SetOnChanging,       (obj_t o, close_query_callback_t cb, void* d),                 (o, cb, d)) \
    X(bool_t,        TCustomTabControl_GetMultiLine,        (obj_t o),                                                     (o)) \
    X(void,          TCustomTabControl_SetMultiLine,        (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomTabControl_GetShowTabs,         (obj_t o),                                                     (o)) \
    X(void,          TCustomTabControl_SetShowTabs,         (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomTabControl_GetTabPosition,      (obj_t o),                                                     (o)) \
    X(void,          TCustomTabControl_SetTabPosition,      (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TTabSheet_Create,                      (obj_t owner),                                                 (owner)) \
    X(obj_t,         TTabSheet_GetPageControl,              (obj_t o),                                                     (o)) \
    X(void,          TTabSheet_SetPageControl,              (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TTabSheet_GetTabIndex,                 (obj_t o),                                                     (o)) \
    X(int_t,         TCustomPage_GetPageIndex,              (obj_t o),                                                     (o)) \
    X(void,          TCustomPage_SetPageIndex,              (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomPage_GetTabVisible,             (obj_t o),                                                     (o)) \
    X(void,          TCustomPage_SetTabVisible,             (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomPage_SetOnShow,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCustomPage_SetOnHide,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          ItemFree_SetCallback,                  (callback_t cb, void* d),                                      (cb, d)) \
    X(obj_t,         TTreeView_Create,                      (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomTreeView_GetItems,              (obj_t o),                                                     (o)) \
    X(obj_t,         TCustomTreeView_GetSelected,           (obj_t o),                                                     (o)) \
    X(void,          TCustomTreeView_SetSelected,           (obj_t o, obj_t n),                                            (o, n)) \
    X(void,          TCustomTreeView_FullExpand,            (obj_t o),                                                     (o)) \
    X(void,          TCustomTreeView_FullCollapse,          (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomTreeView_AlphaSort,             (obj_t o),                                                     (o)) \
    X(obj_t,         TCustomTreeView_GetNodeAt,             (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(bool_t,        TTreeView_GetReadOnly,                 (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetReadOnly,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeView_GetShowLines,                (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetShowLines,                (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeView_GetShowRoot,                 (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetShowRoot,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeView_GetShowButtons,              (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetShowButtons,              (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeView_GetAutoExpand,               (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetAutoExpand,               (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeView_GetHideSelection,            (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetHideSelection,            (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeView_GetRowSelect,                (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetRowSelect,                (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TTreeView_SetOnChange,                 (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TTreeView_SetOnExpanded,               (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TTreeView_SetOnCollapsed,              (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TTreeView_SetOnDeletion,               (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TTreeView_SetOnChanging,               (obj_t o, item_allow_callback_t cb, void* d),                  (o, cb, d)) \
    X(void,          TTreeView_SetOnExpanding,              (obj_t o, item_allow_callback_t cb, void* d),                  (o, cb, d)) \
    X(void,          TTreeView_SetOnCollapsing,             (obj_t o, item_allow_callback_t cb, void* d),                  (o, cb, d)) \
    X(obj_t,         TTreeNodes_Add,                        (obj_t o, obj_t n, str_t t),                                   (o, n, t)) \
    X(obj_t,         TTreeNodes_AddFirst,                   (obj_t o, obj_t n, str_t t),                                   (o, n, t)) \
    X(obj_t,         TTreeNodes_AddChild,                   (obj_t o, obj_t n, str_t t),                                   (o, n, t)) \
    X(obj_t,         TTreeNodes_AddChildFirst,              (obj_t o, obj_t n, str_t t),                                   (o, n, t)) \
    X(obj_t,         TTreeNodes_Insert,                     (obj_t o, obj_t n, str_t t),                                   (o, n, t)) \
    X(void,          TTreeNodes_Clear,                      (obj_t o),                                                     (o)) \
    X(void,          TTreeNodes_Delete,                     (obj_t o, obj_t n),                                            (o, n)) \
    X(int_t,         TTreeNodes_GetCount,                   (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNodes_GetItem,                    (obj_t o, int_t i),                                            (o, i)) \
    X(obj_t,         TTreeNodes_GetFirstNode,               (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNodes_FindNodeWithText,           (obj_t o, str_t t),                                            (o, t)) \
    X(void,          TTreeNodes_BeginUpdate,                (obj_t o),                                                     (o)) \
    X(void,          TTreeNodes_EndUpdate,                  (obj_t o),                                                     (o)) \
    X(str_t,         TTreeNode_GetText,                     (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetText,                     (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TTreeNode_GetExpanded,                 (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetExpanded,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeNode_GetSelected,                 (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetSelected,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeNode_GetHasChildren,              (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetHasChildren,              (obj_t o, bool_t v),                                           (o, v)) \
    X(void*,         TTreeNode_GetData,                     (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetData,                     (obj_t o, void* v),                                            (o, v)) \
    X(int_t,         TTreeNode_GetCount,                    (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetItem,                     (obj_t o, int_t i),                                            (o, i)) \
    X(int_t,         TTreeNode_GetIndex,                    (obj_t o),                                                     (o)) \
    X(int_t,         TTreeNode_GetLevel,                    (obj_t o),                                                     (o)) \
    X(int_t,         TTreeNode_GetAbsoluteIndex,            (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetParent,                   (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetTreeView,                 (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetFirstChild,               (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetLastChild,                (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetNextSibling,              (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetPrevSibling,              (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetNext,                     (obj_t o),                                                     (o)) \
    X(obj_t,         TTreeNode_GetPrev,                     (obj_t o),                                                     (o)) \
    X(int_t,         TTreeNode_IndexOf,                     (obj_t o, obj_t n),                                            (o, n)) \
    X(void,          TTreeNode_Expand,                      (obj_t o, bool_t r),                                           (o, r)) \
    X(void,          TTreeNode_Collapse,                    (obj_t o, bool_t r),                                           (o, r)) \
    X(void,          TTreeNode_Delete,                      (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_DeleteChildren,              (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_MakeVisible,                 (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_MoveTo,                      (obj_t o, obj_t d, int_t m),                                   (o, d, m)) \
    X(obj_t,         TListView_Create,                      (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomListView_GetItems,              (obj_t o),                                                     (o)) \
    X(obj_t,         TCustomListView_GetSelected,           (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SetSelected,           (obj_t o, obj_t i),                                            (o, i)) \
    X(int_t,         TCustomListView_GetItemIndex,          (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SetItemIndex,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomListView_GetSelCount,           (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomListView_GetCheckboxes,         (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SetCheckboxes,         (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomListView_GetGridLines,          (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SetGridLines,          (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomListView_GetMultiSelect,        (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SetMultiSelect,        (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomListView_GetReadOnly,           (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SetReadOnly,           (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomListView_GetRowSelect,          (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SetRowSelect,          (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomListView_Clear,                 (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_BeginUpdate,           (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_EndUpdate,             (obj_t o),                                                     (o)) \
    X(obj_t,         TCustomListView_GetItemAt,             (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(void,          TCustomListView_ClearSelection,        (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SelectAll,             (obj_t o),                                                     (o)) \
    X(obj_t,         TListView_GetColumns,                  (obj_t o),                                                     (o)) \
    X(int_t,         TListView_GetViewStyle,                (obj_t o),                                                     (o)) \
    X(void,          TListView_SetViewStyle,                (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TListView_GetHideSelection,            (obj_t o),                                                     (o)) \
    X(void,          TListView_SetHideSelection,            (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TListView_GetSortType,                 (obj_t o),                                                     (o)) \
    X(void,          TListView_SetSortType,                 (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TListView_GetSortColumn,               (obj_t o),                                                     (o)) \
    X(void,          TListView_SetSortColumn,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TListView_GetSortDirection,            (obj_t o),                                                     (o)) \
    X(void,          TListView_SetSortDirection,            (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TListView_SetOnSelectItem,             (obj_t o, item_int_callback_t cb, void* d),                    (o, cb, d)) \
    X(void,          TListView_SetOnChange,                 (obj_t o, item_int_callback_t cb, void* d),                    (o, cb, d)) \
    X(void,          TListView_SetOnDeletion,               (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TListView_SetOnItemChecked,            (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TListView_SetOnColumnClick,            (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(obj_t,         TListItems_Add,                        (obj_t o),                                                     (o)) \
    X(obj_t,         TListItems_Insert,                     (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TListItems_Delete,                     (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TListItems_Clear,                      (obj_t o),                                                     (o)) \
    X(int_t,         TListItems_GetCount,                   (obj_t o),                                                     (o)) \
    X(obj_t,         TListItems_GetItem,                    (obj_t o, int_t i),                                            (o, i)) \
    X(int_t,         TListItems_IndexOf,                    (obj_t o, obj_t i),                                            (o, i)) \
    X(obj_t,         TListItems_FindCaption,                (obj_t o, int_t s, str_t v, bool_t p, bool_t inc, bool_t w),   (o, s, v, p, inc, w)) \
    X(void,          TListItems_Exchange,                   (obj_t o, int_t a, int_t b),                                   (o, a, b)) \
    X(void,          TListItems_Move,                       (obj_t o, int_t a, int_t b),                                   (o, a, b)) \
    X(void,          TListItems_BeginUpdate,                (obj_t o),                                                     (o)) \
    X(void,          TListItems_EndUpdate,                  (obj_t o),                                                     (o)) \
    X(str_t,         TListItem_GetCaption,                  (obj_t o),                                                     (o)) \
    X(void,          TListItem_SetCaption,                  (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TListItem_GetChecked,                  (obj_t o),                                                     (o)) \
    X(void,          TListItem_SetChecked,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TListItem_GetSelected,                 (obj_t o),                                                     (o)) \
    X(void,          TListItem_SetSelected,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TListItem_GetFocused,                  (obj_t o),                                                     (o)) \
    X(void,          TListItem_SetFocused,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(void*,         TListItem_GetData,                     (obj_t o),                                                     (o)) \
    X(void,          TListItem_SetData,                     (obj_t o, void* v),                                            (o, v)) \
    X(int_t,         TListItem_GetIndex,                    (obj_t o),                                                     (o)) \
    X(obj_t,         TListItem_GetListView,                 (obj_t o),                                                     (o)) \
    X(obj_t,         TListItem_GetSubItems,                 (obj_t o),                                                     (o)) \
    X(void,          TListItem_Delete,                      (obj_t o),                                                     (o)) \
    X(void,          TListItem_MakeVisible,                 (obj_t o, bool_t p),                                           (o, p)) \
    X(obj_t,         TListColumns_Add,                      (obj_t o),                                                     (o)) \
    X(int_t,         TListColumns_GetCount,                 (obj_t o),                                                     (o)) \
    X(obj_t,         TListColumns_GetItem,                  (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TListColumns_Delete,                   (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TListColumns_Clear,                    (obj_t o),                                                     (o)) \
    X(str_t,         TListColumn_GetCaption,                (obj_t o),                                                     (o)) \
    X(void,          TListColumn_SetCaption,                (obj_t o, str_t v),                                            (o, v)) \
    X(int_t,         TListColumn_GetWidth,                  (obj_t o),                                                     (o)) \
    X(void,          TListColumn_SetWidth,                  (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TListColumn_GetAlignment,              (obj_t o),                                                     (o)) \
    X(void,          TListColumn_SetAlignment,              (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TListColumn_GetAutoSize,               (obj_t o),                                                     (o)) \
    X(void,          TListColumn_SetAutoSize,               (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TListColumn_GetVisible,                (obj_t o),                                                     (o)) \
    X(void,          TListColumn_SetVisible,                (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TListColumn_GetIndex,                  (obj_t o),                                                     (o)) \
    X(void,          TListColumn_SetIndex,                  (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TDrawGrid_Create,                      (obj_t owner),                                                 (owner)) \
    X(obj_t,         TStringGrid_Create,                    (obj_t owner),                                                 (owner)) \
    X(void,          TCustomGrid_BeginUpdate,               (obj_t o),                                                     (o)) \
    X(void,          TCustomGrid_EndUpdate,                 (obj_t o),                                                     (o)) \
    X(void,          TCustomGrid_Clear,                     (obj_t o),                                                     (o)) \
    X(void,          TCustomGrid_CellRect,                  (obj_t o, int_t c, int_t r, int_t* l, int_t* t, int_t* rt, int_t* b), (o, c, r, l, t, rt, b)) \
    X(void,          TCustomGrid_MouseToCell,               (obj_t o, int_t x, int_t y, int_t* c, int_t* r),               (o, x, y, c, r)) \
    X(int_t,         TCustomDrawGrid_GetColCount,           (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetColCount,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetRowCount,           (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetRowCount,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetFixedCols,          (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetFixedCols,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetFixedRows,          (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetFixedRows,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetCol,                (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetCol,                (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetRow,                (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetRow,                (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetDefaultColWidth,    (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetDefaultColWidth,    (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetDefaultRowHeight,   (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetDefaultRowHeight,   (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetColWidths,          (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCustomDrawGrid_SetColWidths,          (obj_t o, int_t i, int_t v),                                   (o, i, v)) \
    X(int_t,         TCustomDrawGrid_GetRowHeights,         (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCustomDrawGrid_SetRowHeights,         (obj_t o, int_t i, int_t v),                                   (o, i, v)) \
    X(uint_t,        TCustomDrawGrid_GetOptions,            (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetOptions,            (obj_t o, uint_t v),                                           (o, v)) \
    X(void,          TCustomDrawGrid_GetSelection,          (obj_t o, int_t* l, int_t* t, int_t* r, int_t* b),             (o, l, t, r, b)) \
    X(void,          TCustomDrawGrid_SetSelection,          (obj_t o, int_t l, int_t t, int_t r, int_t b),                 (o, l, t, r, b)) \
    X(int_t,         TCustomDrawGrid_GetLeftCol,            (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetLeftCol,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetTopRow,             (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetTopRow,             (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomDrawGrid_GetDefaultDrawing,     (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetDefaultDrawing,     (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomDrawGrid_GetFixedColor,         (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetFixedColor,         (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomDrawGrid_GetEditorMode,         (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetEditorMode,         (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomDrawGrid_InsertColRow,          (obj_t o, bool_t c, int_t i),                                  (o, c, i)) \
    X(void,          TCustomDrawGrid_DeleteColRow,          (obj_t o, bool_t c, int_t i),                                  (o, c, i)) \
    X(void,          TCustomDrawGrid_MoveColRow,            (obj_t o, bool_t c, int_t f, int_t t),                         (o, c, f, t)) \
    X(void,          TCustomDrawGrid_SortColRow,            (obj_t o, bool_t c, int_t i),                                  (o, c, i)) \
    X(void,          TCustomDrawGrid_SetOnDrawCell,         (obj_t o, draw_cell_callback_t cb, void* d),                   (o, cb, d)) \
    X(void,          TCustomDrawGrid_SetOnSelectCell,       (obj_t o, cell_allow_callback_t cb, void* d),                  (o, cb, d)) \
    X(void,          TCustomDrawGrid_SetOnSelection,        (obj_t o, cell_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TCustomDrawGrid_SetOnHeaderClick,      (obj_t o, header_callback_t cb, void* d),                      (o, cb, d)) \
    X(str_t,         TCustomStringGrid_GetCells,            (obj_t o, int_t c, int_t r),                                   (o, c, r)) \
    X(void,          TCustomStringGrid_SetCells,            (obj_t o, int_t c, int_t r, str_t v),                          (o, c, r, v)) \
    X(void,          TCustomStringGrid_Clean,               (obj_t o),                                                     (o)) \
    X(void,          TCustomStringGrid_AutoSizeColumns,     (obj_t o),                                                     (o)) \
    X(void,          TCustomStringGrid_AutoSizeColumn,      (obj_t o, int_t c),                                            (o, c)) \
    X(obj_t,         THeaderControl_Create,                 (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomHeaderControl_GetSections,      (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomHeaderControl_GetDragReorder,   (obj_t o),                                                     (o)) \
    X(void,          TCustomHeaderControl_SetDragReorder,   (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomHeaderControl_GetSectionAt,     (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(obj_t,         TCustomHeaderControl_GetSectionFromOriginalIndex, (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCustomHeaderControl_SetOnSectionClick, (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionResize, (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionSeparatorDblClick, (obj_t o, item_callback_t cb, void* d),                        (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionTrack, (obj_t o, section_track_callback_t cb, void* d),               (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionDrag, (obj_t o, section_drag_callback_t cb, void* d),                (o, cb, d)) \
    X(void,          TCustomHeaderControl_SetOnSectionEndDrag, (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         THeaderSections_Add,                   (obj_t o),                                                     (o)) \
    X(obj_t,         THeaderSections_Insert,                (obj_t o, int_t i),                                            (o, i)) \
    X(void,          THeaderSections_Delete,                (obj_t o, int_t i),                                            (o, i)) \
    X(void,          THeaderSections_Clear,                 (obj_t o),                                                     (o)) \
    X(int_t,         THeaderSections_GetCount,              (obj_t o),                                                     (o)) \
    X(obj_t,         THeaderSections_GetItem,               (obj_t o, int_t i),                                            (o, i)) \
    X(void,          THeaderSections_BeginUpdate,           (obj_t o),                                                     (o)) \
    X(void,          THeaderSections_EndUpdate,             (obj_t o),                                                     (o)) \
    X(str_t,         THeaderSection_GetText,                (obj_t o),                                                     (o)) \
    X(void,          THeaderSection_SetText,                (obj_t o, str_t v),                                            (o, v)) \
    X(int_t,         THeaderSection_GetWidth,               (obj_t o),                                                     (o)) \
    X(void,          THeaderSection_SetWidth,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         THeaderSection_GetMinWidth,            (obj_t o),                                                     (o)) \
    X(void,          THeaderSection_SetMinWidth,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         THeaderSection_GetMaxWidth,            (obj_t o),                                                     (o)) \
    X(void,          THeaderSection_SetMaxWidth,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         THeaderSection_GetAlignment,           (obj_t o),                                                     (o)) \
    X(void,          THeaderSection_SetAlignment,           (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        THeaderSection_GetVisible,             (obj_t o),                                                     (o)) \
    X(void,          THeaderSection_SetVisible,             (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         THeaderSection_GetIndex,               (obj_t o),                                                     (o)) \
    X(void,          THeaderSection_SetIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         THeaderSection_GetLeft,                (obj_t o),                                                     (o)) \
    X(int_t,         THeaderSection_GetRight,               (obj_t o),                                                     (o)) \
    X(int_t,         THeaderSection_GetOriginalIndex,       (obj_t o),                                                     (o)) \
    X(uint_t,        TToolWindow_GetEdgeBorders,            (obj_t o),                                                     (o)) \
    X(void,          TToolWindow_SetEdgeBorders,            (obj_t o, uint_t v),                                           (o, v)) \
    X(int_t,         TToolWindow_GetEdgeInner,              (obj_t o),                                                     (o)) \
    X(void,          TToolWindow_SetEdgeInner,              (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TToolWindow_GetEdgeOuter,              (obj_t o),                                                     (o)) \
    X(void,          TToolWindow_SetEdgeOuter,              (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TToolWindow_BeginUpdate,               (obj_t o),                                                     (o)) \
    X(void,          TToolWindow_EndUpdate,                 (obj_t o),                                                     (o)) \
    X(obj_t,         TToolBar_Create,                       (obj_t owner),                                                 (owner)) \
    X(int_t,         TToolBar_GetButtonCount,               (obj_t o),                                                     (o)) \
    X(obj_t,         TToolBar_GetButton,                    (obj_t o, int_t i),                                            (o, i)) \
    X(int_t,         TToolBar_GetRowCount,                  (obj_t o),                                                     (o)) \
    X(int_t,         TToolBar_GetButtonHeight,              (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetButtonHeight,              (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TToolBar_GetButtonWidth,               (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetButtonWidth,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TToolBar_GetDropDownWidth,             (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetDropDownWidth,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TToolBar_GetIndent,                    (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetIndent,                    (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TToolBar_GetFlat,                      (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetFlat,                      (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolBar_GetList,                      (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetList,                      (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolBar_GetShowCaptions,              (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetShowCaptions,              (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolBar_GetTransparent,               (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetTransparent,               (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolBar_GetWrapable,                  (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetWrapable,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TToolBar_SetButtonSize,                (obj_t o, int_t w, int_t h),                                   (o, w, h)) \
    X(obj_t,         TToolButton_Create,                    (obj_t owner),                                                 (owner)) \
    X(bool_t,        TToolButton_GetAllowAllUp,             (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetAllowAllUp,             (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolButton_GetDown,                   (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetDown,                   (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolButton_GetGrouped,                (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetGrouped,                (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolButton_GetIndeterminate,          (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetIndeterminate,          (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolButton_GetMarked,                 (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetMarked,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolButton_GetShowCaption,            (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetShowCaption,            (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TToolButton_GetWrap,                   (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetWrap,                   (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TToolButton_GetStyle,                  (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetStyle,                  (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TToolButton_GetDropdownMenu,           (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetDropdownMenu,           (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TToolButton_GetMenuItem,               (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetMenuItem,               (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TToolButton_GetIndex,                  (obj_t o),                                                     (o)) \
    X(void,          TToolButton_Click,                     (obj_t o),                                                     (o)) \
    X(void,          TToolButton_ArrowClick,                (obj_t o),                                                     (o)) \
    X(bool_t,        TToolButton_PointInArrow,              (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(void,          TToolButton_SetOnArrowClick,           (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TCoolBar_Create,                       (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomCoolBar_GetBands,               (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_AutosizeBands,          (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_MouseToBandPos,         (obj_t o, int_t x, int_t y, int_t* b, bool_t* g),              (o, x, y, b, g)) \
    X(bool_t,        TCustomCoolBar_GetFixedSize,           (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetFixedSize,           (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomCoolBar_GetFixedOrder,          (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetFixedOrder,          (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomCoolBar_GetGrabStyle,           (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetGrabStyle,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomCoolBar_GetGrabWidth,           (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetGrabWidth,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomCoolBar_GetHorizontalSpacing,   (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetHorizontalSpacing,   (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomCoolBar_GetVerticalSpacing,     (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetVerticalSpacing,     (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomCoolBar_GetShowText,            (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetShowText,            (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomCoolBar_GetThemed,              (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetThemed,              (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomCoolBar_GetVertical,            (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetVertical,            (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomCoolBar_SetOnChange,            (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TCoolBands_Add,                        (obj_t o),                                                     (o)) \
    X(int_t,         TCoolBands_GetCount,                   (obj_t o),                                                     (o)) \
    X(obj_t,         TCoolBands_GetItem,                    (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCoolBands_Delete,                     (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCoolBands_Clear,                      (obj_t o),                                                     (o)) \
    X(void,          TCoolBands_BeginUpdate,                (obj_t o),                                                     (o)) \
    X(void,          TCoolBands_EndUpdate,                  (obj_t o),                                                     (o)) \
    X(obj_t,         TCoolBands_FindBand,                   (obj_t o, obj_t c),                                            (o, c)) \
    X(int_t,         TCoolBands_FindBandIndex,              (obj_t o, obj_t c),                                            (o, c)) \
    X(str_t,         TCoolBand_GetText,                     (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetText,                     (obj_t o, str_t v),                                            (o, v)) \
    X(int_t,         TCoolBand_GetWidth,                    (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetWidth,                    (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCoolBand_GetMinWidth,                 (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetMinWidth,                 (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCoolBand_GetMinHeight,                (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetMinHeight,                (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCoolBand_GetBreak,                    (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetBreak,                    (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCoolBand_GetVisible,                  (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetVisible,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCoolBand_GetFixedSize,                (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetFixedSize,                (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCoolBand_GetFixedBackground,          (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetFixedBackground,          (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCoolBand_GetHorizontalOnly,           (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetHorizontalOnly,           (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCoolBand_GetColor,                    (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetColor,                    (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCoolBand_GetParentColor,              (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetParentColor,              (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCoolBand_GetIndex,                    (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetIndex,                    (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCoolBand_GetControl,                  (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetControl,                  (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCoolBand_GetLeft,                     (obj_t o),                                                     (o)) \
    X(int_t,         TCoolBand_GetTop,                      (obj_t o),                                                     (o)) \
    X(int_t,         TCoolBand_GetRight,                    (obj_t o),                                                     (o)) \
    X(int_t,         TCoolBand_GetHeight,                   (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_AutosizeWidth,               (obj_t o),                                                     (o)) \
    X(int_t,         TStrings_GetCount,                     (obj_t o),                                                     (o)) \
    X(str_t,         TStrings_GetStrings,                   (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TStrings_SetStrings,                   (obj_t o, int_t i, str_t v),                                   (o, i, v)) \
    X(void*,         TStrings_GetObjects,                   (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TStrings_SetObjects,                   (obj_t o, int_t i, void* v),                                   (o, i, v)) \
    X(int_t,         TStrings_Add,                          (obj_t o, str_t s),                                            (o, s)) \
    X(int_t,         TStrings_AddObject,                    (obj_t o, str_t s, void* a),                                   (o, s, a)) \
    X(void,          TStrings_Insert,                       (obj_t o, int_t i, str_t s),                                   (o, i, s)) \
    X(void,          TStrings_Delete,                       (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TStrings_Clear,                        (obj_t o),                                                     (o)) \
    X(int_t,         TStrings_IndexOf,                      (obj_t o, str_t s),                                            (o, s)) \
    X(void,          TStrings_Exchange,                     (obj_t o, int_t a, int_t b),                                   (o, a, b)) \
    X(void,          TStrings_Move,                         (obj_t o, int_t a, int_t b),                                   (o, a, b)) \
    X(void,          TStrings_BeginUpdate,                  (obj_t o),                                                     (o)) \
    X(void,          TStrings_EndUpdate,                    (obj_t o),                                                     (o)) \
    X(str_t,         TStrings_GetText,                      (obj_t o),                                                     (o)) \
    X(void,          TStrings_SetText,                      (obj_t o, str_t v),                                            (o, v)) \
    X(str_t,         TStrings_GetCommaText,                 (obj_t o),                                                     (o)) \
    X(void,          TStrings_SetCommaText,                 (obj_t o, str_t v),                                            (o, v)) \
    X(void,          TStrings_Assign,                       (obj_t o, obj_t s),                                            (o, s)) \
    X(void,          TStrings_AddStrings,                   (obj_t o, obj_t s),                                            (o, s)) \
    X(str_t,         TStrings_GetNames,                     (obj_t o, int_t i),                                            (o, i)) \
    X(str_t,         TStrings_GetValues,                    (obj_t o, str_t n),                                            (o, n)) \
    X(void,          TStrings_SetValues,                    (obj_t o, str_t n, str_t v),                                   (o, n, v)) \
    X(str_t,         TStrings_GetValueFromIndex,            (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TStrings_SetValueFromIndex,            (obj_t o, int_t i, str_t v),                                   (o, i, v)) \
    X(int_t,         TStrings_IndexOfName,                  (obj_t o, str_t n),                                            (o, n)) \
    X(char,          TStrings_GetDelimiter,                 (obj_t o),                                                     (o)) \
    X(void,          TStrings_SetDelimiter,                 (obj_t o, char v),                                             (o, v)) \
    X(bool_t,        TStrings_GetStrictDelimiter,           (obj_t o),                                                     (o)) \
    X(void,          TStrings_SetStrictDelimiter,           (obj_t o, bool_t v),                                           (o, v)) \
    X(str_t,         TStrings_GetDelimitedText,             (obj_t o),                                                     (o)) \
    X(void,          TStrings_SetDelimitedText,             (obj_t o, str_t v),                                            (o, v)) \
    X(void,          TStrings_LoadFromFile,                 (obj_t o, str_t f),                                            (o, f)) \
    X(void,          TStrings_SaveToFile,                   (obj_t o, str_t f),                                            (o, f)) \
    X(obj_t,         TStringList_Create,                    (void),                                                        ()) \
    X(void,          TStringList_Destroy,                   (obj_t o),                                                     (o)) \
    X(void,          TStringList_Sort,                      (obj_t o),                                                     (o)) \
    X(bool_t,        TStringList_Find,                      (obj_t o, str_t s, int_t* i),                                  (o, s, i)) \
    X(bool_t,        TStringList_GetSorted,                 (obj_t o),                                                     (o)) \
    X(void,          TStringList_SetSorted,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TStringList_GetDuplicates,             (obj_t o),                                                     (o)) \
    X(void,          TStringList_SetDuplicates,             (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TStringList_GetCaseSensitive,          (obj_t o),                                                     (o)) \
    X(void,          TStringList_SetCaseSensitive,          (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TGraphic_Destroy,                      (obj_t o),                                                     (o)) \
    X(int_t,         TGraphic_GetWidth,                     (obj_t o),                                                     (o)) \
    X(void,          TGraphic_SetWidth,                     (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TGraphic_GetHeight,                    (obj_t o),                                                     (o)) \
    X(void,          TGraphic_SetHeight,                    (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TGraphic_GetEmpty,                     (obj_t o),                                                     (o)) \
    X(bool_t,        TGraphic_GetTransparent,               (obj_t o),                                                     (o)) \
    X(void,          TGraphic_SetTransparent,               (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TGraphic_LoadFromFile,                 (obj_t o, str_t filename),                                     (o, filename)) \
    X(void,          TGraphic_SaveToFile,                   (obj_t o, str_t filename),                                     (o, filename)) \
    X(void,          TGraphic_Assign,                       (obj_t o, obj_t source),                                       (o, source)) \
    X(void,          TGraphic_Clear,                        (obj_t o),                                                     (o)) \
    X(obj_t,         TRasterImage_GetCanvas,                (obj_t o),                                                     (o)) \
    X(int_t,         TRasterImage_GetPixelFormat,           (obj_t o),                                                     (o)) \
    X(void,          TRasterImage_SetPixelFormat,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TRasterImage_GetTransparentColor,      (obj_t o),                                                     (o)) \
    X(void,          TRasterImage_SetTransparentColor,      (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TRasterImage_GetTransparentMode,       (obj_t o),                                                     (o)) \
    X(void,          TRasterImage_SetTransparentMode,       (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TCustomBitmap_SetSize,                 (obj_t o, int_t awidth, int_t aheight),                        (o, awidth, aheight)) \
    X(obj_t,         TBitmap_Create,                        (void),                                                        ()) \
    X(obj_t,         TPortableNetworkGraphic_Create,        (void),                                                        ()) \
    X(obj_t,         TJPEGImage_Create,                     (void),                                                        ()) \
    X(int_t,         TJPEGImage_GetCompressionQuality,      (obj_t o),                                                     (o)) \
    X(void,          TJPEGImage_SetCompressionQuality,      (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TPicture_Create,                       (void),                                                        ()) \
    X(void,          TPicture_Destroy,                      (obj_t o),                                                     (o)) \
    X(obj_t,         TPicture_GetGraphic,                   (obj_t o),                                                     (o)) \
    X(void,          TPicture_SetGraphic,                   (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TPicture_GetBitmap,                    (obj_t o),                                                     (o)) \
    X(obj_t,         TPicture_GetPNG,                       (obj_t o),                                                     (o)) \
    X(obj_t,         TPicture_GetJpeg,                      (obj_t o),                                                     (o)) \
    X(int_t,         TPicture_GetWidth,                     (obj_t o),                                                     (o)) \
    X(int_t,         TPicture_GetHeight,                    (obj_t o),                                                     (o)) \
    X(void,          TPicture_LoadFromFile,                 (obj_t o, str_t filename),                                     (o, filename)) \
    X(void,          TPicture_SaveToFile,                   (obj_t o, str_t filename),                                     (o, filename)) \
    X(void,          TPicture_Assign,                       (obj_t o, obj_t source),                                       (o, source)) \
    X(void,          TPicture_Clear,                        (obj_t o),                                                     (o)) \
    X(obj_t,         TImage_Create,                         (obj_t owner),                                                 (owner)) \
    X(obj_t,         TCustomImage_GetPicture,               (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetPicture,               (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TCustomImage_GetCanvas,                (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomImage_GetHasGraphic,            (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomImage_GetCenter,                (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetCenter,                (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomImage_GetStretch,               (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetStretch,               (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomImage_GetStretchOutEnabled,     (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetStretchOutEnabled,     (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomImage_GetStretchInEnabled,      (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetStretchInEnabled,      (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomImage_GetProportional,          (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetProportional,          (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomImage_GetTransparent,           (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetTransparent,           (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomImage_SetOnPictureChanged,      (obj_t o, callback_t cb, void* data),                          (o, cb, data)) \
    X(obj_t,         TCustomBitBtn_GetGlyph,                (obj_t o),                                                     (o)) \
    X(void,          TCustomBitBtn_SetGlyph,                (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomBitBtn_GetNumGlyphs,            (obj_t o),                                                     (o)) \
    X(void,          TCustomBitBtn_SetNumGlyphs,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomBitBtn_GetLayout,               (obj_t o),                                                     (o)) \
    X(void,          TCustomBitBtn_SetLayout,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomBitBtn_GetMargin,               (obj_t o),                                                     (o)) \
    X(void,          TCustomBitBtn_SetMargin,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomBitBtn_GetSpacing,              (obj_t o),                                                     (o)) \
    X(void,          TCustomBitBtn_SetSpacing,              (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomSpeedButton_GetGlyph,           (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetGlyph,           (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomSpeedButton_GetNumGlyphs,       (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetNumGlyphs,       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSpeedButton_GetLayout,          (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetLayout,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSpeedButton_GetMargin,          (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetMargin,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomSpeedButton_GetSpacing,         (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetSpacing,         (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TImageList_Create,                     (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomImageList_GetWidth,             (obj_t o),                                                     (o)) \
    X(void,          TCustomImageList_SetWidth,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomImageList_GetHeight,            (obj_t o),                                                     (o)) \
    X(void,          TCustomImageList_SetHeight,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomImageList_GetCount,             (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomImageList_GetMasked,            (obj_t o),                                                     (o)) \
    X(void,          TCustomImageList_SetMasked,            (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomImageList_GetBkColor,           (obj_t o),                                                     (o)) \
    X(void,          TCustomImageList_SetBkColor,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomImageList_GetDrawingStyle,      (obj_t o),                                                     (o)) \
    X(void,          TCustomImageList_SetDrawingStyle,      (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomImageList_Add,                  (obj_t o, obj_t image, obj_t mask),                            (o, image, mask)) \
    X(int_t,         TCustomImageList_AddSliced,            (obj_t o, obj_t image, int_t h, int_t v),                      (o, image, h, v)) \
    X(int_t,         TCustomImageList_AddMasked,            (obj_t o, obj_t image, int_t maskcolor),                       (o, image, maskcolor)) \
    X(void,          TCustomImageList_Insert,               (obj_t o, int_t index, obj_t image, obj_t mask),               (o, index, image, mask)) \
    X(void,          TCustomImageList_Replace,              (obj_t o, int_t index, obj_t image, obj_t mask),               (o, index, image, mask)) \
    X(void,          TCustomImageList_Delete,               (obj_t o, int_t index),                                        (o, index)) \
    X(void,          TCustomImageList_Clear,                (obj_t o),                                                     (o)) \
    X(void,          TCustomImageList_Move,                 (obj_t o, int_t curindex, int_t newindex),                     (o, curindex, newindex)) \
    X(void,          TCustomImageList_GetBitmap,            (obj_t o, int_t index, obj_t image),                           (o, index, image)) \
    X(void,          TCustomImageList_Draw,                 (obj_t o, obj_t canvas, int_t x, int_t y, int_t index, bool_t enabled), (o, canvas, x, y, index, enabled)) \
    X(void,          TCustomImageList_BeginUpdate,          (obj_t o),                                                     (o)) \
    X(void,          TCustomImageList_EndUpdate,            (obj_t o),                                                     (o)) \
    X(void,          TCustomImageList_SetOnChange,          (obj_t o, callback_t cb, void* data),                          (o, cb, data)) \
    X(obj_t,         TCustomImage_GetImages,                (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetImages,                (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomImage_GetImageIndex,            (obj_t o),                                                     (o)) \
    X(void,          TCustomImage_SetImageIndex,            (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomBitBtn_GetImages,               (obj_t o),                                                     (o)) \
    X(void,          TCustomBitBtn_SetImages,               (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomBitBtn_GetImageIndex,           (obj_t o),                                                     (o)) \
    X(void,          TCustomBitBtn_SetImageIndex,           (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomSpeedButton_GetImages,          (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetImages,          (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomSpeedButton_GetImageIndex,      (obj_t o),                                                     (o)) \
    X(void,          TCustomSpeedButton_SetImageIndex,      (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomTabControl_GetImages,           (obj_t o),                                                     (o)) \
    X(void,          TCustomTabControl_SetImages,           (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomPage_GetImageIndex,             (obj_t o),                                                     (o)) \
    X(void,          TCustomPage_SetImageIndex,             (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomTreeView_GetImages,             (obj_t o),                                                     (o)) \
    X(void,          TCustomTreeView_SetImages,             (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TCustomTreeView_GetStateImages,        (obj_t o),                                                     (o)) \
    X(void,          TCustomTreeView_SetStateImages,        (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TTreeNode_GetImageIndex,               (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetImageIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TTreeNode_GetSelectedIndex,            (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetSelectedIndex,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TTreeNode_GetStateIndex,               (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetStateIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TTreeNode_GetOverlayIndex,             (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_SetOverlayIndex,             (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TListView_GetLargeImages,              (obj_t o),                                                     (o)) \
    X(void,          TListView_SetLargeImages,              (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TListView_GetSmallImages,              (obj_t o),                                                     (o)) \
    X(void,          TListView_SetSmallImages,              (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TListView_GetStateImages,              (obj_t o),                                                     (o)) \
    X(void,          TListView_SetStateImages,              (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TListItem_GetImageIndex,               (obj_t o),                                                     (o)) \
    X(void,          TListItem_SetImageIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TListItem_GetStateIndex,               (obj_t o),                                                     (o)) \
    X(void,          TListItem_SetStateIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TListColumn_GetImageIndex,             (obj_t o),                                                     (o)) \
    X(void,          TListColumn_SetImageIndex,             (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TToolBar_GetImages,                    (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetImages,                    (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TToolBar_GetHotImages,                 (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetHotImages,                 (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TToolBar_GetDisabledImages,            (obj_t o),                                                     (o)) \
    X(void,          TToolBar_SetDisabledImages,            (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TToolButton_GetImageIndex,             (obj_t o),                                                     (o)) \
    X(void,          TToolButton_SetImageIndex,             (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomHeaderControl_GetImages,        (obj_t o),                                                     (o)) \
    X(void,          TCustomHeaderControl_SetImages,        (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         THeaderSection_GetImageIndex,          (obj_t o),                                                     (o)) \
    X(void,          THeaderSection_SetImageIndex,          (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomCoolBar_GetImages,              (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetImages,              (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TCustomCoolBar_GetBitmap,              (obj_t o),                                                     (o)) \
    X(void,          TCustomCoolBar_SetBitmap,              (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCoolBand_GetImageIndex,               (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetImageIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCoolBand_GetBitmap,                   (obj_t o),                                                     (o)) \
    X(void,          TCoolBand_SetBitmap,                   (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TMenu_GetImages,                       (obj_t o),                                                     (o)) \
    X(void,          TMenu_SetImages,                       (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TMenuItem_GetImageIndex,               (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetImageIndex,               (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TMenuItem_GetSubMenuImages,            (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetSubMenuImages,            (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TMenuItem_GetBitmap,                   (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetBitmap,                   (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TControl_GetColor,                     (obj_t o),                                                     (o)) \
    X(void,          TControl_SetColor,                     (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TControl_GetFont,                      (obj_t o),                                                     (o)) \
    X(void,          TControl_SetFont,                      (obj_t o, obj_t v),                                            (o, v)) \
    X(uint_t,        TFont_GetStyle,                        (obj_t o),                                                     (o)) \
    X(void,          TFont_SetStyle,                        (obj_t o, uint_t v),                                           (o, v)) \
    X(void,          TFont_Assign,                          (obj_t o, obj_t s),                                            (o, s)) \
    X(bool_t,        TCommonDialog_Execute,                 (obj_t o),                                                     (o)) \
    X(str_t,         TCommonDialog_GetTitle,                (obj_t o),                                                     (o)) \
    X(void,          TCommonDialog_SetTitle,                (obj_t o, str_t v),                                            (o, v)) \
    X(void,          TCommonDialog_SetOnShow,               (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCommonDialog_SetOnClose,              (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCommonDialog_SetOnCanClose,           (obj_t o, close_query_callback_t cb, void* d),                 (o, cb, d)) \
    X(str_t,         TFileDialog_GetFileName,               (obj_t o),                                                     (o)) \
    X(void,          TFileDialog_SetFileName,               (obj_t o, str_t v),                                            (o, v)) \
    X(str_t,         TFileDialog_GetFilter,                 (obj_t o),                                                     (o)) \
    X(void,          TFileDialog_SetFilter,                 (obj_t o, str_t v),                                            (o, v)) \
    X(int_t,         TFileDialog_GetFilterIndex,            (obj_t o),                                                     (o)) \
    X(void,          TFileDialog_SetFilterIndex,            (obj_t o, int_t v),                                            (o, v)) \
    X(str_t,         TFileDialog_GetInitialDir,             (obj_t o),                                                     (o)) \
    X(void,          TFileDialog_SetInitialDir,             (obj_t o, str_t v),                                            (o, v)) \
    X(str_t,         TFileDialog_GetDefaultExt,             (obj_t o),                                                     (o)) \
    X(void,          TFileDialog_SetDefaultExt,             (obj_t o, str_t v),                                            (o, v)) \
    X(obj_t,         TFileDialog_GetFiles,                  (obj_t o),                                                     (o)) \
    X(obj_t,         TOpenDialog_Create,                    (obj_t owner),                                                 (owner)) \
    X(uint_t,        TOpenDialog_GetOptions,                (obj_t o),                                                     (o)) \
    X(void,          TOpenDialog_SetOptions,                (obj_t o, uint_t v),                                           (o, v)) \
    X(obj_t,         TSaveDialog_Create,                    (obj_t owner),                                                 (owner)) \
    X(obj_t,         TSelectDirectoryDialog_Create,         (obj_t owner),                                                 (owner)) \
    X(obj_t,         TColorDialog_Create,                   (obj_t owner),                                                 (owner)) \
    X(int_t,         TColorDialog_GetColor,                 (obj_t o),                                                     (o)) \
    X(void,          TColorDialog_SetColor,                 (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TColorDialog_GetCustomColors,          (obj_t o),                                                     (o)) \
    X(uint_t,        TColorDialog_GetOptions,               (obj_t o),                                                     (o)) \
    X(void,          TColorDialog_SetOptions,               (obj_t o, uint_t v),                                           (o, v)) \
    X(obj_t,         TFontDialog_Create,                    (obj_t owner),                                                 (owner)) \
    X(obj_t,         TFontDialog_GetFont,                   (obj_t o),                                                     (o)) \
    X(void,          TFontDialog_SetFont,                   (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TFontDialog_GetMinFontSize,            (obj_t o),                                                     (o)) \
    X(void,          TFontDialog_SetMinFontSize,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TFontDialog_GetMaxFontSize,            (obj_t o),                                                     (o)) \
    X(void,          TFontDialog_SetMaxFontSize,            (obj_t o, int_t v),                                            (o, v)) \
    X(uint_t,        TFontDialog_GetOptions,                (obj_t o),                                                     (o)) \
    X(void,          TFontDialog_SetOptions,                (obj_t o, uint_t v),                                           (o, v)) \
    X(obj_t,         TFindDialog_Create,                    (obj_t owner),                                                 (owner)) \
    X(obj_t,         TReplaceDialog_Create,                 (obj_t owner),                                                 (owner)) \
    X(str_t,         TFindDialog_GetFindText,               (obj_t o),                                                     (o)) \
    X(void,          TFindDialog_SetFindText,               (obj_t o, str_t v),                                            (o, v)) \
    X(str_t,         TFindDialog_GetReplaceText,            (obj_t o),                                                     (o)) \
    X(void,          TFindDialog_SetReplaceText,            (obj_t o, str_t v),                                            (o, v)) \
    X(uint_t,        TFindDialog_GetOptions,                (obj_t o),                                                     (o)) \
    X(void,          TFindDialog_SetOptions,                (obj_t o, uint_t v),                                           (o, v)) \
    X(int_t,         TFindDialog_GetLeft,                   (obj_t o),                                                     (o)) \
    X(void,          TFindDialog_SetLeft,                   (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TFindDialog_GetTop,                    (obj_t o),                                                     (o)) \
    X(void,          TFindDialog_SetTop,                    (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TFindDialog_CloseDialog,               (obj_t o),                                                     (o)) \
    X(void,          TFindDialog_SetOnFind,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TFindDialog_SetOnReplace,              (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(iptr_t,        TComponent_GetTag,                     (obj_t o),                                                     (o)) \
    X(void,          TComponent_SetTag,                     (obj_t o, iptr_t v),                                           (o, v)) \
    X(uint_t,        TControl_GetAnchors,                   (obj_t o),                                                     (o)) \
    X(void,          TControl_SetAnchors,                   (obj_t o, uint_t v),                                           (o, v)) \
    X(obj_t,         TControl_GetBorderSpacing,             (obj_t o),                                                     (o)) \
    X(void,          TControl_SetBorderSpacing,             (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TControl_GetConstraints,               (obj_t o),                                                     (o)) \
    X(void,          TControl_SetConstraints,               (obj_t o, obj_t v),                                            (o, v)) \
    X(str_t,         TControl_GetHint,                      (obj_t o),                                                     (o)) \
    X(void,          TControl_SetHint,                      (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TControl_GetShowHint,                  (obj_t o),                                                     (o)) \
    X(void,          TControl_SetShowHint,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TControl_GetCursor,                    (obj_t o),                                                     (o)) \
    X(void,          TControl_SetCursor,                    (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TControl_GetParentColor,               (obj_t o),                                                     (o)) \
    X(void,          TControl_SetParentColor,               (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TControl_GetParentFont,                (obj_t o),                                                     (o)) \
    X(void,          TControl_SetParentFont,                (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TControl_GetParentShowHint,            (obj_t o),                                                     (o)) \
    X(void,          TControl_SetParentShowHint,            (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TWinControl_GetTabOrder,               (obj_t o),                                                     (o)) \
    X(void,          TWinControl_SetTabOrder,               (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TWinControl_GetTabStop,                (obj_t o),                                                     (o)) \
    X(void,          TWinControl_SetTabStop,                (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TSizeConstraints_GetMinWidth,          (obj_t o),                                                     (o)) \
    X(void,          TSizeConstraints_SetMinWidth,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TSizeConstraints_GetMinHeight,         (obj_t o),                                                     (o)) \
    X(void,          TSizeConstraints_SetMinHeight,         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TSizeConstraints_GetMaxWidth,          (obj_t o),                                                     (o)) \
    X(void,          TSizeConstraints_SetMaxWidth,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TSizeConstraints_GetMaxHeight,         (obj_t o),                                                     (o)) \
    X(void,          TSizeConstraints_SetMaxHeight,         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlBorderSpacing_GetLeft,         (obj_t o),                                                     (o)) \
    X(void,          TControlBorderSpacing_SetLeft,         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlBorderSpacing_GetTop,          (obj_t o),                                                     (o)) \
    X(void,          TControlBorderSpacing_SetTop,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlBorderSpacing_GetRight,        (obj_t o),                                                     (o)) \
    X(void,          TControlBorderSpacing_SetRight,        (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlBorderSpacing_GetBottom,       (obj_t o),                                                     (o)) \
    X(void,          TControlBorderSpacing_SetBottom,       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlBorderSpacing_GetAround,       (obj_t o),                                                     (o)) \
    X(void,          TControlBorderSpacing_SetAround,       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlBorderSpacing_GetInnerBorder,  (obj_t o),                                                     (o)) \
    X(void,          TControlBorderSpacing_SetInnerBorder,  (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomForm_GetModalResult,            (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetModalResult,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomForm_GetBorderStyle,            (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetBorderStyle,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomForm_GetPosition,               (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetPosition,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomForm_GetWindowState,            (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetWindowState,            (obj_t o, int_t v),                                            (o, v)) \
    X(uint_t,        TCustomForm_GetBorderIcons,            (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetBorderIcons,            (obj_t o, uint_t v),                                           (o, v)) \
    X(int_t,         TCustomForm_GetFormStyle,              (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetFormStyle,              (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomForm_GetKeyPreview,             (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetKeyPreview,             (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TCustomForm_GetActiveControl,          (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetActiveControl,          (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomButton_GetModalResult,          (obj_t o),                                                     (o)) \
    X(void,          TCustomButton_SetModalResult,          (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomButton_GetDefault,              (obj_t o),                                                     (o)) \
    X(void,          TCustomButton_SetDefault,              (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomButton_GetCancel,               (obj_t o),                                                     (o)) \
    X(void,          TCustomButton_SetCancel,               (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          Dialogs_ShowMessage,                   (str_t m),                                                     (m)) \
    X(int_t,         Dialogs_MessageDlg,                    (str_t c, str_t m, int_t t, uint_t b, int_t h),                (c, m, t, b, h)) \
    X(str_t,         Dialogs_InputBox,                      (str_t c, str_t p, str_t d),                                   (c, p, d)) \
    X(str_t,         Dialogs_PasswordBox,                   (str_t c, str_t p),                                            (c, p)) \
    X(str_t,         Dialogs_InputQuery,                    (str_t c, str_t p, str_t v, bool_t* ok),                       (c, p, v, ok)) \
    X(int_t,         TApplication_MessageBox,               (obj_t o, str_t t, str_t c, int_t f),                          (o, t, c, f)) \
    X(int_t,         TControl_GetClientWidth,               (obj_t o),                                                     (o)) \
    X(void,          TControl_SetClientWidth,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControl_GetClientHeight,              (obj_t o),                                                     (o)) \
    X(void,          TControl_SetClientHeight,              (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TControl_Invalidate,                   (obj_t o),                                                     (o)) \
    X(void,          TControl_Repaint,                      (obj_t o),                                                     (o)) \
    X(void,          TControl_Refresh,                      (obj_t o),                                                     (o)) \
    X(void,          TControl_Update,                       (obj_t o),                                                     (o)) \
    X(void,          TControl_BringToFront,                 (obj_t o),                                                     (o)) \
    X(void,          TControl_SendToBack,                   (obj_t o),                                                     (o)) \
    X(void,          TControl_SetBounds,                    (obj_t o, int_t l, int_t t, int_t w, int_t h),                 (o, l, t, w, h)) \
    X(void,          TWinControl_SetFocus,                  (obj_t o),                                                     (o)) \
    X(bool_t,        TWinControl_CanFocus,                  (obj_t o),                                                     (o)) \
    X(bool_t,        TWinControl_Focused,                   (obj_t o),                                                     (o)) \
    X(int_t,         TCustomEdit_GetSelStart,               (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetSelStart,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomEdit_GetSelLength,              (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetSelLength,              (obj_t o, int_t v),                                            (o, v)) \
    X(str_t,         TCustomEdit_GetSelText,                (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetSelText,                (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TCustomEdit_GetModified,               (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetModified,               (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomEdit_GetCanUndo,                (obj_t o),                                                     (o)) \
    X(char,          TCustomEdit_GetPasswordChar,           (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetPasswordChar,           (obj_t o, char v),                                             (o, v)) \
    X(int_t,         TCustomEdit_GetEchoMode,               (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetEchoMode,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomEdit_GetCharCase,               (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetCharCase,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomEdit_GetAlignment,              (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetAlignment,              (obj_t o, int_t v),                                            (o, v)) \
    X(str_t,         TCustomEdit_GetTextHint,               (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetTextHint,               (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TCustomEdit_GetNumbersOnly,            (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetNumbersOnly,            (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomEdit_GetAutoSelect,             (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetAutoSelect,             (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomEdit_GetHideSelection,          (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_SetHideSelection,          (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomEdit_GetCaretPos,               (obj_t o, int_t* x, int_t* y),                                 (o, x, y)) \
    X(void,          TCustomEdit_SetCaretPos,               (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(void,          TCustomEdit_SelectAll,                 (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_ClearSelection,            (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_Clear,                     (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_CopyToClipboard,           (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_CutToClipboard,            (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_PasteFromClipboard,        (obj_t o),                                                     (o)) \
    X(void,          TCustomEdit_Undo,                      (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomMemo_GetWordWrap,               (obj_t o),                                                     (o)) \
    X(void,          TCustomMemo_SetWordWrap,               (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomMemo_GetWantReturns,            (obj_t o),                                                     (o)) \
    X(void,          TCustomMemo_SetWantReturns,            (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomMemo_GetWantTabs,               (obj_t o),                                                     (o)) \
    X(void,          TCustomMemo_SetWantTabs,               (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomMemo_Append,                    (obj_t o, str_t s),                                            (o, s)) \
    X(int_t,         TCustomLabel_GetAlignment,             (obj_t o),                                                     (o)) \
    X(void,          TCustomLabel_SetAlignment,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomLabel_GetLayout,                (obj_t o),                                                     (o)) \
    X(void,          TCustomLabel_SetLayout,                (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomLabel_GetWordWrap,              (obj_t o),                                                     (o)) \
    X(void,          TCustomLabel_SetWordWrap,              (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomLabel_GetTransparent,           (obj_t o),                                                     (o)) \
    X(void,          TCustomLabel_SetTransparent,           (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TCustomLabel_GetFocusControl,          (obj_t o),                                                     (o)) \
    X(void,          TCustomLabel_SetFocusControl,          (obj_t o, obj_t v),                                            (o, v)) \
    X(bool_t,        TCustomLabel_GetShowAccelChar,         (obj_t o),                                                     (o)) \
    X(void,          TCustomLabel_SetShowAccelChar,         (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TWinControl_SetOnEnter,                (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TWinControl_SetOnExit,                 (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(bool_t,        TCustomListBox_GetMultiSelect,         (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SetMultiSelect,         (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomListBox_GetExtendedSelect,      (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SetExtendedSelect,      (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomListBox_GetSorted,              (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SetSorted,              (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomListBox_GetTopIndex,            (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SetTopIndex,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomListBox_GetSelCount,            (obj_t o),                                                     (o)) \
    X(bool_t,        TCustomListBox_GetSelected,            (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCustomListBox_SetSelected,            (obj_t o, int_t i, bool_t v),                                  (o, i, v)) \
    X(void,          TCustomListBox_ClearSelection,         (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SelectAll,              (obj_t o),                                                     (o)) \
    X(int_t,         TCustomListBox_ItemAtPos,              (obj_t o, int_t x, int_t y, bool_t e),                         (o, x, y, e)) \
    X(void,          TCustomListBox_SetOnSelectionChange,   (obj_t o, bool_callback_t cb, void* d),                        (o, cb, d)) \
    X(int_t,         TCustomComboBox_GetStyle,              (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetStyle,              (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomComboBox_GetDropDownCount,      (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetDropDownCount,      (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomComboBox_GetSorted,             (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetSorted,             (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomComboBox_GetReadOnly,           (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetReadOnly,           (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomComboBox_GetDroppedDown,        (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetDroppedDown,        (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomComboBox_GetAutoComplete,       (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetAutoComplete,       (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomComboBox_SetOnSelect,           (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCustomComboBox_SetOnDropDown,         (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TCustomComboBox_SetOnCloseUp,          (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(int_t,         TCustomCheckBox_GetState,              (obj_t o),                                                     (o)) \
    X(void,          TCustomCheckBox_SetState,              (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomCheckBox_GetAllowGrayed,        (obj_t o),                                                     (o)) \
    X(void,          TCustomCheckBox_SetAllowGrayed,        (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomCheckBox_SetOnChange,           (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(str_t,         TApplication_GetExeName,               (obj_t o),                                                     (o)) \
    X(str_t,         TApplication_GetHint,                  (obj_t o),                                                     (o)) \
    X(void,          TApplication_SetHint,                  (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TApplication_GetShowHint,              (obj_t o),                                                     (o)) \
    X(void,          TApplication_SetShowHint,              (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TApplication_GetHintPause,             (obj_t o),                                                     (o)) \
    X(void,          TApplication_SetHintPause,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TApplication_GetHintHidePause,         (obj_t o),                                                     (o)) \
    X(void,          TApplication_SetHintHidePause,         (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TApplication_Minimize,                 (obj_t o),                                                     (o)) \
    X(void,          TApplication_Restore,                  (obj_t o),                                                     (o)) \
    X(void,          TApplication_BringToFront,             (obj_t o),                                                     (o)) \
    X(void,          TApplication_SetOnIdle,                (obj_t o, close_query_callback_t cb, void* d),                 (o, cb, d)) \
    X(void,          TApplication_SetOnException,           (obj_t o, exception_callback_t cb, void* d),                   (o, cb, d)) \
    X(obj_t,         TStatusBar_GetPanels,                  (obj_t o),                                                     (o)) \
    X(bool_t,        TStatusBar_GetSizeGrip,                (obj_t o),                                                     (o)) \
    X(void,          TStatusBar_SetSizeGrip,                (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TStatusBar_GetAutoHint,                (obj_t o),                                                     (o)) \
    X(void,          TStatusBar_SetAutoHint,                (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TStatusBar_GetCanvas,                  (obj_t o),                                                     (o)) \
    X(int_t,         TStatusBar_GetPanelIndexAt,            (obj_t o, int_t x, int_t y),                                   (o, x, y)) \
    X(void,          TStatusBar_BeginUpdate,                (obj_t o),                                                     (o)) \
    X(void,          TStatusBar_EndUpdate,                  (obj_t o),                                                     (o)) \
    X(void,          TStatusBar_SetOnDrawPanel,             (obj_t o, item_rect_callback_t cb, void* d),                   (o, cb, d)) \
    X(void,          TStatusBar_SetOnHint,                  (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TStatusPanels_Add,                     (obj_t o),                                                     (o)) \
    X(obj_t,         TStatusPanels_Insert,                  (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TStatusPanels_Delete,                  (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TStatusPanels_Clear,                   (obj_t o),                                                     (o)) \
    X(int_t,         TStatusPanels_GetCount,                (obj_t o),                                                     (o)) \
    X(obj_t,         TStatusPanels_GetItem,                 (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TStatusPanels_BeginUpdate,             (obj_t o),                                                     (o)) \
    X(void,          TStatusPanels_EndUpdate,               (obj_t o),                                                     (o)) \
    X(str_t,         TStatusPanel_GetText,                  (obj_t o),                                                     (o)) \
    X(void,          TStatusPanel_SetText,                  (obj_t o, str_t v),                                            (o, v)) \
    X(int_t,         TStatusPanel_GetWidth,                 (obj_t o),                                                     (o)) \
    X(void,          TStatusPanel_SetWidth,                 (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TStatusPanel_GetAlignment,             (obj_t o),                                                     (o)) \
    X(void,          TStatusPanel_SetAlignment,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TStatusPanel_GetBevel,                 (obj_t o),                                                     (o)) \
    X(void,          TStatusPanel_SetBevel,                 (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TStatusPanel_GetStyle,                 (obj_t o),                                                     (o)) \
    X(void,          TStatusPanel_SetStyle,                 (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TStatusPanel_GetIndex,                 (obj_t o),                                                     (o)) \
    X(void,          TStatusPanel_SetIndex,                 (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomControl_GetCanvas,              (obj_t o),                                                     (o)) \
    X(void,          TCustomControl_SetOnPaint,             (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(int_t,         TCanvas_TextWidth,                     (obj_t o, str_t s),                                            (o, s)) \
    X(int_t,         TCanvas_TextHeight,                    (obj_t o, str_t s),                                            (o, s)) \
    X(void,          TCanvas_TextRect,                      (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2, int_t x, int_t y, str_t s), (o, x1, y1, x2, y2, x, y, s)) \
    X(void,          TCanvas_Polygon,                       (obj_t o, int_t* points, int_t count),                         (o, points, count)) \
    X(void,          TCanvas_Polyline,                      (obj_t o, int_t* points, int_t count),                         (o, points, count)) \
    X(void,          TCanvas_RoundRect,                     (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2, int_t rx, int_t ry), (o, x1, y1, x2, y2, rx, ry)) \
    X(void,          TCanvas_Arc,                           (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2, int_t x3, int_t y3, int_t x4, int_t y4), (o, x1, y1, x2, y2, x3, y3, x4, y4)) \
    X(void,          TCanvas_Pie,                           (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2, int_t x3, int_t y3, int_t x4, int_t y4), (o, x1, y1, x2, y2, x3, y3, x4, y4)) \
    X(void,          TCanvas_Chord,                         (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2, int_t x3, int_t y3, int_t x4, int_t y4), (o, x1, y1, x2, y2, x3, y3, x4, y4)) \
    X(void,          TCanvas_FrameRect,                     (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2),             (o, x1, y1, x2, y2)) \
    X(void,          TCanvas_CopyRect,                      (obj_t o, int_t x1, int_t y1, int_t x2, int_t y2, obj_t source, int_t sx1, int_t sy1, int_t sx2, int_t sy2), (o, x1, y1, x2, y2, source, sx1, sy1, sx2, sy2)) \
    X(int_t,         TPen_GetStyle,                         (obj_t o),                                                     (o)) \
    X(void,          TPen_SetStyle,                         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TPen_GetMode,                          (obj_t o),                                                     (o)) \
    X(void,          TPen_SetMode,                          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TBrush_GetStyle,                       (obj_t o),                                                     (o)) \
    X(void,          TBrush_SetStyle,                       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TFont_GetHeight,                       (obj_t o),                                                     (o)) \
    X(void,          TFont_SetHeight,                       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TFont_GetOrientation,                  (obj_t o),                                                     (o)) \
    X(void,          TFont_SetOrientation,                  (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TFont_GetQuality,                      (obj_t o),                                                     (o)) \
    X(void,          TFont_SetQuality,                      (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TBasicAction_Execute,                  (obj_t o),                                                     (o)) \
    X(bool_t,        TBasicAction_Update,                   (obj_t o),                                                     (o)) \
    X(obj_t,         TBasicAction_GetActionComponent,       (obj_t o),                                                     (o)) \
    X(void,          TBasicAction_SetOnExecute,             (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(void,          TBasicAction_SetOnUpdate,              (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(obj_t,         TContainedAction_GetActionList,        (obj_t o),                                                     (o)) \
    X(void,          TContainedAction_SetActionList,        (obj_t o, obj_t v),                                            (o, v)) \
    X(str_t,         TContainedAction_GetCategory,          (obj_t o),                                                     (o)) \
    X(void,          TContainedAction_SetCategory,          (obj_t o, str_t v),                                            (o, v)) \
    X(int_t,         TContainedAction_GetIndex,             (obj_t o),                                                     (o)) \
    X(void,          TContainedAction_SetIndex,             (obj_t o, int_t v),                                            (o, v)) \
    X(str_t,         TCustomAction_GetCaption,              (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetCaption,              (obj_t o, str_t v),                                            (o, v)) \
    X(str_t,         TCustomAction_GetHint,                 (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetHint,                 (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TCustomAction_GetChecked,              (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetChecked,              (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomAction_GetAutoCheck,            (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetAutoCheck,            (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomAction_GetEnabled,              (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetEnabled,              (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomAction_GetVisible,              (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetVisible,              (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomAction_GetDisableIfNoHandler,   (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetDisableIfNoHandler,   (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomAction_GetGroupIndex,           (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetGroupIndex,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomAction_GetImageIndex,           (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetImageIndex,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomAction_GetShortCut,             (obj_t o),                                                     (o)) \
    X(void,          TCustomAction_SetShortCut,             (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TAction_Create,                        (obj_t owner),                                                 (owner)) \
    X(int_t,         TCustomActionList_GetActionCount,      (obj_t o),                                                     (o)) \
    X(obj_t,         TCustomActionList_GetActions,          (obj_t o, int_t i),                                            (o, i)) \
    X(obj_t,         TCustomActionList_GetImages,           (obj_t o),                                                     (o)) \
    X(void,          TCustomActionList_SetImages,           (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomActionList_GetState,            (obj_t o),                                                     (o)) \
    X(void,          TCustomActionList_SetState,            (obj_t o, int_t v),                                            (o, v)) \
    X(void,          TCustomActionList_SetOnExecute,        (obj_t o, item_allow_callback_t cb, void* d),                  (o, cb, d)) \
    X(void,          TCustomActionList_SetOnUpdate,         (obj_t o, item_allow_callback_t cb, void* d),                  (o, cb, d)) \
    X(obj_t,         TActionList_Create,                    (obj_t owner),                                                 (owner)) \
    X(obj_t,         TControl_GetAction,                    (obj_t o),                                                     (o)) \
    X(void,          TControl_SetAction,                    (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TMenuItem_GetAction,                   (obj_t o),                                                     (o)) \
    X(void,          TMenuItem_SetAction,                   (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         GetScreen,                             (void),                                                        ()) \
    X(int_t,         TScreen_GetCursor,                     (obj_t o),                                                     (o)) \
    X(void,          TScreen_SetCursor,                     (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TScreen_GetWidth,                      (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetHeight,                     (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetDesktopLeft,                (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetDesktopTop,                 (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetDesktopWidth,               (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetDesktopHeight,              (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetWorkAreaLeft,               (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetWorkAreaTop,                (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetWorkAreaWidth,              (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetWorkAreaHeight,             (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetPixelsPerInch,              (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetMonitorCount,               (obj_t o),                                                     (o)) \
    X(int_t,         TScreen_GetFormCount,                  (obj_t o),                                                     (o)) \
    X(void,          TScreen_GetWorkAreaRect,               (obj_t o, int_t* l, int_t* t, int_t* r, int_t* b),             (o, l, t, r, b)) \
    X(obj_t,         TScreen_GetForms,                      (obj_t o, int_t i),                                            (o, i)) \
    X(obj_t,         TScreen_GetActiveForm,                 (obj_t o),                                                     (o)) \
    X(obj_t,         TScreen_GetActiveControl,              (obj_t o),                                                     (o)) \
    X(obj_t,         TScreen_GetFonts,                      (obj_t o),                                                     (o)) \
    X(void,          TScreen_SetOnActiveFormChange,         (obj_t o, callback_t cb, void* data),                          (o, cb, data)) \
    X(void,          TScreen_SetOnActiveControlChange,      (obj_t o, callback_t cb, void* data),                          (o, cb, data)) \
    X(obj_t,         GetClipboard,                          (void),                                                        ()) \
    X(uint_t,        Clipboard_CF_Text,                     (void),                                                        ()) \
    X(uint_t,        Clipboard_CF_Bitmap,                   (void),                                                        ()) \
    X(uint_t,        Clipboard_CF_Picture,                  (void),                                                        ()) \
    X(str_t,         TClipboard_GetAsText,                  (obj_t o),                                                     (o)) \
    X(void,          TClipboard_SetAsText,                  (obj_t o, str_t v),                                            (o, v)) \
    X(bool_t,        TClipboard_HasFormat,                  (obj_t o, uint_t format),                                      (o, format)) \
    X(bool_t,        TClipboard_HasPictureFormat,           (obj_t o),                                                     (o)) \
    X(void,          TClipboard_Clear,                      (obj_t o),                                                     (o)) \
    X(void,          TClipboard_Open,                       (obj_t o),                                                     (o)) \
    X(void,          TClipboard_Close,                      (obj_t o),                                                     (o)) \
    X(int_t,         TClipboard_GetFormatCount,             (obj_t o),                                                     (o)) \
    X(uint_t,        TClipboard_GetFormats,                 (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TClipboard_Assign,                     (obj_t o, obj_t source),                                       (o, source)) \
    X(obj_t,         TIcon_Create,                          (void),                                                        ()) \
    X(obj_t,         TCustomForm_GetIcon,                   (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetIcon,                   (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TApplication_GetIcon,                  (obj_t o),                                                     (o)) \
    X(void,          TApplication_SetIcon,                  (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TPicture_GetIcon,                      (obj_t o),                                                     (o)) \
    X(void,          TPicture_SetIcon,                      (obj_t o, obj_t v),                                            (o, v)) \
    X(bool_t,        TCustomForm_GetAllowDropFiles,         (obj_t o),                                                     (o)) \
    X(void,          TCustomForm_SetAllowDropFiles,         (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomForm_SetOnDropFiles,            (obj_t o, drop_files_callback_t cb, void* data),               (o, cb, data)) \
    X(int_t,         TWinControl_GetBorderStyle,            (obj_t o),                                                     (o)) \
    X(void,          TWinControl_SetBorderStyle,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TWinControl_GetBorderWidth,            (obj_t o),                                                     (o)) \
    X(void,          TWinControl_SetBorderWidth,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomPanel_GetAlignment,             (obj_t o),                                                     (o)) \
    X(void,          TCustomPanel_SetAlignment,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomPanel_GetVerticalAlignment,     (obj_t o),                                                     (o)) \
    X(void,          TCustomPanel_SetVerticalAlignment,     (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomPanel_GetWordWrap,              (obj_t o),                                                     (o)) \
    X(void,          TCustomPanel_SetWordWrap,              (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomPanel_GetBevelColor,            (obj_t o),                                                     (o)) \
    X(void,          TCustomPanel_SetBevelColor,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomPanel_GetBevelInner,            (obj_t o),                                                     (o)) \
    X(void,          TCustomPanel_SetBevelInner,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomPanel_GetBevelOuter,            (obj_t o),                                                     (o)) \
    X(void,          TCustomPanel_SetBevelOuter,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomPanel_GetBevelWidth,            (obj_t o),                                                     (o)) \
    X(void,          TCustomPanel_SetBevelWidth,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomDrawGrid_GetScrollBars,             (obj_t o),                                                     (o)) \
    X(void,          TCustomDrawGrid_SetScrollBars,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTreeView_GetScrollBars,         (obj_t o),                                                     (o)) \
    X(void,          TCustomTreeView_SetScrollBars,         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomListView_GetScrollBars,         (obj_t o),                                                     (o)) \
    X(void,          TCustomListView_SetScrollBars,         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlScrollBar_GetKind,             (obj_t o),                                                     (o)) \
    X(int_t,         TControlScrollBar_GetSize,             (obj_t o),                                                     (o)) \
    X(bool_t,        TControlScrollBar_IsScrollBarVisible,  (obj_t o),                                                     (o)) \
    X(int_t,         TControlScrollBar_GetIncrement,        (obj_t o),                                                     (o)) \
    X(void,          TControlScrollBar_SetIncrement,        (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlScrollBar_GetPage,             (obj_t o),                                                     (o)) \
    X(void,          TControlScrollBar_SetPage,             (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlScrollBar_GetPosition,         (obj_t o),                                                     (o)) \
    X(void,          TControlScrollBar_SetPosition,         (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TControlScrollBar_GetRange,            (obj_t o),                                                     (o)) \
    X(void,          TControlScrollBar_SetRange,            (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TControlScrollBar_GetSmooth,           (obj_t o),                                                     (o)) \
    X(void,          TControlScrollBar_SetSmooth,           (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TControlScrollBar_GetTracking,         (obj_t o),                                                     (o)) \
    X(void,          TControlScrollBar_SetTracking,         (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TControlScrollBar_GetVisible,          (obj_t o),                                                     (o)) \
    X(void,          TControlScrollBar_SetVisible,          (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TScrollingWinControl_GetAutoScroll,    (obj_t o),                                                     (o)) \
    X(void,          TScrollingWinControl_SetAutoScroll,    (obj_t o, bool_t v),                                           (o, v)) \
    X(obj_t,         TScrollingWinControl_GetHorzScrollBar, (obj_t o),                                                     (o)) \
    X(void,          TScrollingWinControl_SetHorzScrollBar, (obj_t o, obj_t v),                                            (o, v)) \
    X(obj_t,         TScrollingWinControl_GetVertScrollBar, (obj_t o),                                                     (o)) \
    X(void,          TScrollingWinControl_SetVertScrollBar, (obj_t o, obj_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetOrientation,        (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetOrientation,        (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetFrequency,          (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetFrequency,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetTickMarks,          (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetTickMarks,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetTickStyle,          (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetTickStyle,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetLineSize,           (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetLineSize,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetPageSize,           (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetPageSize,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetSelStart,           (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetSelStart,           (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomTrackBar_GetSelEnd,             (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetSelEnd,             (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomTrackBar_GetShowSelRange,       (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetShowSelRange,       (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TCustomTrackBar_GetReversed,           (obj_t o),                                                     (o)) \
    X(void,          TCustomTrackBar_SetReversed,           (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomProgressBar_GetOrientation,     (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_SetOrientation,     (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomProgressBar_GetSmooth,          (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_SetSmooth,          (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomProgressBar_GetStep,            (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_SetStep,            (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomProgressBar_GetStyle,           (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_SetStyle,           (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomProgressBar_GetBarShowText,     (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_SetBarShowText,     (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomScrollBar_GetLargeChange,       (obj_t o),                                                     (o)) \
    X(void,          TCustomScrollBar_SetLargeChange,       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomScrollBar_GetSmallChange,       (obj_t o),                                                     (o)) \
    X(void,          TCustomScrollBar_SetSmallChange,       (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TUpDown_GetOrientation,                (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetOrientation,                (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TUpDown_GetAlignButton,                (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetAlignButton,                (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TUpDown_GetWrap,                       (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetWrap,                       (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TUpDown_GetArrowKeys,                  (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetArrowKeys,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TUpDown_GetThousands,                  (obj_t o),                                                     (o)) \
    X(void,          TUpDown_SetThousands,                  (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomRadioGroup_GetColumns,          (obj_t o),                                                     (o)) \
    X(void,          TCustomRadioGroup_SetColumns,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomRadioGroup_GetColumnLayout,     (obj_t o),                                                     (o)) \
    X(void,          TCustomRadioGroup_SetColumnLayout,     (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomRadioGroup_GetAutoFill,         (obj_t o),                                                     (o)) \
    X(void,          TCustomRadioGroup_SetAutoFill,         (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TCustomCheckGroup_GetColumns,          (obj_t o),                                                     (o)) \
    X(void,          TCustomCheckGroup_SetColumns,          (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomCheckGroup_GetColumnLayout,     (obj_t o),                                                     (o)) \
    X(void,          TCustomCheckGroup_SetColumnLayout,     (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TCustomCheckGroup_GetAutoFill,         (obj_t o),                                                     (o)) \
    X(void,          TCustomCheckGroup_SetAutoFill,         (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TCustomProgressBar_StepIt,             (obj_t o),                                                     (o)) \
    X(void,          TCustomProgressBar_StepBy,             (obj_t o, int_t delta),                                        (o, delta)) \
    X(void,          TCustomScrollBar_SetOnScroll,          (obj_t o, scroll_callback_t cb, void* d),                      (o, cb, d)) \
    X(void,          TCustomRadioGroup_SetOnSelectionChanged, (obj_t o, callback_t cb, void* d),                             (o, cb, d)) \
    X(bool_t,        TCustomCheckGroup_GetCheckEnabled,     (obj_t o, int_t i),                                            (o, i)) \
    X(void,          TCustomCheckGroup_SetCheckEnabled,     (obj_t o, int_t i, bool_t v),                                  (o, i, v)) \
    X(void,          TCustomCheckGroup_SetOnItemClick,      (obj_t o, int_callback_t cb, void* d),                         (o, cb, d)) \
    X(int_t,         TCustomListBox_GetStyle,               (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SetStyle,               (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TCustomListBox_GetItemHeight,          (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SetItemHeight,          (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomListBox_GetCanvas,              (obj_t o),                                                     (o)) \
    X(void,          TCustomListBox_SetOnDrawItem,          (obj_t o, draw_item_callback_t cb, void* d),                   (o, cb, d)) \
    X(void,          TCustomListBox_SetOnMeasureItem,       (obj_t o, measure_item_callback_t cb, void* d),                (o, cb, d)) \
    X(int_t,         TCustomComboBox_GetItemHeight,         (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetItemHeight,         (obj_t o, int_t v),                                            (o, v)) \
    X(obj_t,         TCustomComboBox_GetCanvas,             (obj_t o),                                                     (o)) \
    X(void,          TCustomComboBox_SetOnDrawItem,         (obj_t o, draw_item_callback_t cb, void* d),                   (o, cb, d)) \
    X(void,          TCustomComboBox_SetOnMeasureItem,      (obj_t o, measure_item_callback_t cb, void* d),                (o, cb, d)) \
    X(bool_t,        TMenu_GetOwnerDraw,                    (obj_t o),                                                     (o)) \
    X(void,          TMenu_SetOwnerDraw,                    (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TMenuItem_SetOnDrawItem,               (obj_t o, menu_draw_callback_t cb, void* d),                   (o, cb, d)) \
    X(void,          TMenuItem_SetOnMeasureItem,            (obj_t o, menu_measure_callback_t cb, void* d),                (o, cb, d)) \
    X(uint_t,        TCustomTreeView_GetOptions,            (obj_t o),                                                     (o)) \
    X(void,          TCustomTreeView_SetOptions,            (obj_t o, uint_t v),                                           (o, v)) \
    X(uint_t,        TCustomTreeView_GetMultiSelectStyle,   (obj_t o),                                                     (o)) \
    X(void,          TCustomTreeView_SetMultiSelectStyle,   (obj_t o, uint_t v),                                           (o, v)) \
    X(int_t,         TCustomTreeView_GetSelectionCount,     (obj_t o),                                                     (o)) \
    X(obj_t,         TCustomTreeView_GetSelections,         (obj_t o, int_t i),                                            (o, i)) \
    X(bool_t,        TCustomTreeView_IsEditing,             (obj_t o),                                                     (o)) \
    X(bool_t,        TTreeView_GetMultiSelect,              (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetMultiSelect,              (obj_t o, bool_t v),                                           (o, v)) \
    X(int_t,         TTreeView_GetSortType,                 (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetSortType,                 (obj_t o, int_t v),                                            (o, v)) \
    X(int_t,         TTreeView_GetIndent,                   (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetIndent,                   (obj_t o, int_t v),                                            (o, v)) \
    X(bool_t,        TTreeView_GetHotTrack,                 (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetHotTrack,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeView_GetRightClickSelect,         (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetRightClickSelect,         (obj_t o, bool_t v),                                           (o, v)) \
    X(bool_t,        TTreeView_GetToolTips,                 (obj_t o),                                                     (o)) \
    X(void,          TTreeView_SetToolTips,                 (obj_t o, bool_t v),                                           (o, v)) \
    X(void,          TTreeView_SetOnEditing,                (obj_t o, item_allow_callback_t cb, void* d),                  (o, cb, d)) \
    X(void,          TTreeView_SetOnCompare,                (obj_t o, tv_compare_callback_t cb, void* d),                  (o, cb, d)) \
    X(void,          TTreeView_SetOnEdited,                 (obj_t o, tv_edited_callback_t cb, void* d),                   (o, cb, d)) \
    X(void,          TTreeView_SetOnCustomDrawItem,         (obj_t o, tv_custom_draw_callback_t cb, void* d),              (o, cb, d)) \
    X(void,          TTreeNode_DisplayRect,                 (obj_t o, bool_t textOnly, int_t* l, int_t* t, int_t* r, int_t* b), (o, textOnly, l, t, r, b)) \
    X(bool_t,        TTreeNode_EditText,                    (obj_t o),                                                     (o)) \
    X(void,          TTreeNode_EndEdit,                     (obj_t o, bool_t cancel),                                      (o, cancel))

#endif
