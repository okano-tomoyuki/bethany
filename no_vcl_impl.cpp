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

/* TPanel */
typedef no_vcl_obj_t (NO_VCL_CALL *TPanel_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TPanel_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TPanel_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_str_t (NO_VCL_CALL *TPanel_GetCaption)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TPanel_SetCaption)(no_vcl_obj_t Obj, no_vcl_str_t Caption);

typedef no_vcl_int_t (NO_VCL_CALL *TPanel_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TPanel_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TPanel_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TPanel_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TPanel_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TPanel_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TPanel_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TPanel_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TPanel_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TPanel_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TPanel_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TPanel_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* TGroupBox */
typedef no_vcl_obj_t (NO_VCL_CALL *TGroupBox_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TGroupBox_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TGroupBox_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_str_t (NO_VCL_CALL *TGroupBox_GetCaption)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TGroupBox_SetCaption)(no_vcl_obj_t Obj, no_vcl_str_t Caption);

typedef no_vcl_int_t (NO_VCL_CALL *TGroupBox_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TGroupBox_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TGroupBox_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TGroupBox_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TGroupBox_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TGroupBox_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TGroupBox_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TGroupBox_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TGroupBox_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TGroupBox_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TGroupBox_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TGroupBox_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* TComboBox */
typedef no_vcl_obj_t (NO_VCL_CALL *TComboBox_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TComboBox_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TComboBox_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_str_t (NO_VCL_CALL *TComboBox_GetText)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TComboBox_SetText)(no_vcl_obj_t Obj, no_vcl_str_t Text);

typedef no_vcl_int_t (NO_VCL_CALL *TComboBox_GetItemIndex)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TComboBox_SetItemIndex)(no_vcl_obj_t Obj, no_vcl_int_t Value);

typedef void         (NO_VCL_CALL *TComboBox_Items_Add)(no_vcl_obj_t Obj, no_vcl_str_t Text);
typedef void         (NO_VCL_CALL *TComboBox_Items_Clear)(no_vcl_obj_t Obj);
typedef no_vcl_int_t (NO_VCL_CALL *TComboBox_Items_Count)(no_vcl_obj_t Obj);
typedef no_vcl_str_t (NO_VCL_CALL *TComboBox_Items_GetText)(no_vcl_obj_t Obj, no_vcl_int_t Index);

typedef no_vcl_int_t (NO_VCL_CALL *TComboBox_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TComboBox_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TComboBox_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TComboBox_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TComboBox_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TComboBox_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TComboBox_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TComboBox_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TComboBox_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TComboBox_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TComboBox_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TComboBox_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef void         (NO_VCL_CALL *TComboBox_SetOnChange)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TListBox */
typedef no_vcl_obj_t (NO_VCL_CALL *TListBox_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TListBox_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TListBox_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_int_t (NO_VCL_CALL *TListBox_GetItemIndex)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TListBox_SetItemIndex)(no_vcl_obj_t Obj, no_vcl_int_t Value);

typedef void         (NO_VCL_CALL *TListBox_Items_Add)(no_vcl_obj_t Obj, no_vcl_str_t Text);
typedef void         (NO_VCL_CALL *TListBox_Items_Clear)(no_vcl_obj_t Obj);
typedef no_vcl_int_t (NO_VCL_CALL *TListBox_Items_Count)(no_vcl_obj_t Obj);
typedef no_vcl_str_t (NO_VCL_CALL *TListBox_Items_GetText)(no_vcl_obj_t Obj, no_vcl_int_t Index);

typedef no_vcl_int_t (NO_VCL_CALL *TListBox_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TListBox_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TListBox_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TListBox_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TListBox_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TListBox_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TListBox_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TListBox_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TListBox_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TListBox_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TListBox_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TListBox_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef void         (NO_VCL_CALL *TListBox_SetOnClick)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TMemo */
typedef no_vcl_obj_t (NO_VCL_CALL *TMemo_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TMemo_Destroy)(no_vcl_obj_t Obj);

typedef void         (NO_VCL_CALL *TMemo_SetParent)(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

typedef no_vcl_bool_t (NO_VCL_CALL *TMemo_GetReadOnly)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TMemo_SetReadOnly)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_int_t (NO_VCL_CALL *TMemo_GetScrollBars)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TMemo_SetScrollBars)(no_vcl_obj_t Obj, no_vcl_int_t Value);

typedef void         (NO_VCL_CALL *TMemo_Lines_Add)(no_vcl_obj_t Obj, no_vcl_str_t Text);
typedef void         (NO_VCL_CALL *TMemo_Lines_Clear)(no_vcl_obj_t Obj);
typedef no_vcl_int_t (NO_VCL_CALL *TMemo_Lines_Count)(no_vcl_obj_t Obj);
typedef no_vcl_str_t (NO_VCL_CALL *TMemo_Lines_GetText)(no_vcl_obj_t Obj, no_vcl_int_t Index);

typedef no_vcl_int_t (NO_VCL_CALL *TMemo_GetLeft)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TMemo_SetLeft)(no_vcl_obj_t Obj, no_vcl_int_t Left);

typedef no_vcl_int_t (NO_VCL_CALL *TMemo_GetTop)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TMemo_SetTop)(no_vcl_obj_t Obj, no_vcl_int_t Top);

typedef no_vcl_int_t (NO_VCL_CALL *TMemo_GetWidth)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TMemo_SetWidth)(no_vcl_obj_t Obj, no_vcl_int_t Width);

typedef no_vcl_int_t (NO_VCL_CALL *TMemo_GetHeight)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TMemo_SetHeight)(no_vcl_obj_t Obj, no_vcl_int_t Height);

typedef no_vcl_bool_t (NO_VCL_CALL *TMemo_GetVisible)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TMemo_SetVisible)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TMemo_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TMemo_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef void         (NO_VCL_CALL *TMemo_SetOnChange)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TTimer */
typedef no_vcl_obj_t (NO_VCL_CALL *TTimer_Create)(no_vcl_obj_t Owner);
typedef void         (NO_VCL_CALL *TTimer_Destroy)(no_vcl_obj_t Obj);

typedef no_vcl_int_t (NO_VCL_CALL *TTimer_GetInterval)(no_vcl_obj_t Obj);
typedef void         (NO_VCL_CALL *TTimer_SetInterval)(no_vcl_obj_t Obj, no_vcl_int_t Value);

typedef no_vcl_bool_t (NO_VCL_CALL *TTimer_GetEnabled)(no_vcl_obj_t Obj);
typedef void          (NO_VCL_CALL *TTimer_SetEnabled)(no_vcl_obj_t Obj, no_vcl_bool_t Value);

typedef void         (NO_VCL_CALL *TTimer_SetOnTimer)(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

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

thread_local TPanel_Create      TPanel_Create_          = nullptr;
thread_local TPanel_Destroy     TPanel_Destroy_         = nullptr;
thread_local TPanel_SetParent   TPanel_SetParent_       = nullptr;
thread_local TPanel_GetCaption  TPanel_GetCaption_      = nullptr;
thread_local TPanel_SetCaption  TPanel_SetCaption_      = nullptr;
thread_local TPanel_GetLeft     TPanel_GetLeft_         = nullptr;
thread_local TPanel_SetLeft     TPanel_SetLeft_         = nullptr;
thread_local TPanel_GetTop      TPanel_GetTop_          = nullptr;
thread_local TPanel_SetTop      TPanel_SetTop_          = nullptr;
thread_local TPanel_GetWidth    TPanel_GetWidth_        = nullptr;
thread_local TPanel_SetWidth    TPanel_SetWidth_        = nullptr;
thread_local TPanel_GetHeight   TPanel_GetHeight_       = nullptr;
thread_local TPanel_SetHeight   TPanel_SetHeight_       = nullptr;
thread_local TPanel_GetVisible  TPanel_GetVisible_      = nullptr;
thread_local TPanel_SetVisible  TPanel_SetVisible_      = nullptr;
thread_local TPanel_GetEnabled  TPanel_GetEnabled_      = nullptr;
thread_local TPanel_SetEnabled  TPanel_SetEnabled_      = nullptr;

thread_local TGroupBox_Create     TGroupBox_Create_         = nullptr;
thread_local TGroupBox_Destroy    TGroupBox_Destroy_        = nullptr;
thread_local TGroupBox_SetParent  TGroupBox_SetParent_      = nullptr;
thread_local TGroupBox_GetCaption TGroupBox_GetCaption_     = nullptr;
thread_local TGroupBox_SetCaption TGroupBox_SetCaption_     = nullptr;
thread_local TGroupBox_GetLeft    TGroupBox_GetLeft_        = nullptr;
thread_local TGroupBox_SetLeft    TGroupBox_SetLeft_        = nullptr;
thread_local TGroupBox_GetTop     TGroupBox_GetTop_         = nullptr;
thread_local TGroupBox_SetTop     TGroupBox_SetTop_         = nullptr;
thread_local TGroupBox_GetWidth   TGroupBox_GetWidth_       = nullptr;
thread_local TGroupBox_SetWidth   TGroupBox_SetWidth_       = nullptr;
thread_local TGroupBox_GetHeight  TGroupBox_GetHeight_      = nullptr;
thread_local TGroupBox_SetHeight  TGroupBox_SetHeight_      = nullptr;
thread_local TGroupBox_GetVisible TGroupBox_GetVisible_     = nullptr;
thread_local TGroupBox_SetVisible TGroupBox_SetVisible_     = nullptr;
thread_local TGroupBox_GetEnabled TGroupBox_GetEnabled_     = nullptr;
thread_local TGroupBox_SetEnabled TGroupBox_SetEnabled_     = nullptr;

thread_local TComboBox_Create          TComboBox_Create_          = nullptr;
thread_local TComboBox_Destroy         TComboBox_Destroy_         = nullptr;
thread_local TComboBox_SetParent       TComboBox_SetParent_       = nullptr;
thread_local TComboBox_GetText         TComboBox_GetText_         = nullptr;
thread_local TComboBox_SetText         TComboBox_SetText_         = nullptr;
thread_local TComboBox_GetItemIndex    TComboBox_GetItemIndex_    = nullptr;
thread_local TComboBox_SetItemIndex    TComboBox_SetItemIndex_    = nullptr;
thread_local TComboBox_Items_Add       TComboBox_Items_Add_       = nullptr;
thread_local TComboBox_Items_Clear     TComboBox_Items_Clear_     = nullptr;
thread_local TComboBox_Items_Count     TComboBox_Items_Count_     = nullptr;
thread_local TComboBox_Items_GetText   TComboBox_Items_GetText_   = nullptr;
thread_local TComboBox_GetLeft         TComboBox_GetLeft_         = nullptr;
thread_local TComboBox_SetLeft         TComboBox_SetLeft_         = nullptr;
thread_local TComboBox_GetTop          TComboBox_GetTop_          = nullptr;
thread_local TComboBox_SetTop          TComboBox_SetTop_          = nullptr;
thread_local TComboBox_GetWidth        TComboBox_GetWidth_        = nullptr;
thread_local TComboBox_SetWidth        TComboBox_SetWidth_        = nullptr;
thread_local TComboBox_GetHeight       TComboBox_GetHeight_       = nullptr;
thread_local TComboBox_SetHeight       TComboBox_SetHeight_       = nullptr;
thread_local TComboBox_GetVisible      TComboBox_GetVisible_      = nullptr;
thread_local TComboBox_SetVisible      TComboBox_SetVisible_      = nullptr;
thread_local TComboBox_GetEnabled      TComboBox_GetEnabled_      = nullptr;
thread_local TComboBox_SetEnabled      TComboBox_SetEnabled_      = nullptr;
thread_local TComboBox_SetOnChange     TComboBox_SetOnChange_     = nullptr;

thread_local TListBox_Create        TListBox_Create_        = nullptr;
thread_local TListBox_Destroy       TListBox_Destroy_       = nullptr;
thread_local TListBox_SetParent     TListBox_SetParent_     = nullptr;
thread_local TListBox_GetItemIndex  TListBox_GetItemIndex_  = nullptr;
thread_local TListBox_SetItemIndex  TListBox_SetItemIndex_  = nullptr;
thread_local TListBox_Items_Add     TListBox_Items_Add_     = nullptr;
thread_local TListBox_Items_Clear   TListBox_Items_Clear_   = nullptr;
thread_local TListBox_Items_Count   TListBox_Items_Count_   = nullptr;
thread_local TListBox_Items_GetText TListBox_Items_GetText_ = nullptr;
thread_local TListBox_GetLeft       TListBox_GetLeft_       = nullptr;
thread_local TListBox_SetLeft       TListBox_SetLeft_       = nullptr;
thread_local TListBox_GetTop        TListBox_GetTop_        = nullptr;
thread_local TListBox_SetTop        TListBox_SetTop_        = nullptr;
thread_local TListBox_GetWidth      TListBox_GetWidth_      = nullptr;
thread_local TListBox_SetWidth      TListBox_SetWidth_      = nullptr;
thread_local TListBox_GetHeight     TListBox_GetHeight_     = nullptr;
thread_local TListBox_SetHeight     TListBox_SetHeight_     = nullptr;
thread_local TListBox_GetVisible    TListBox_GetVisible_    = nullptr;
thread_local TListBox_SetVisible    TListBox_SetVisible_    = nullptr;
thread_local TListBox_GetEnabled    TListBox_GetEnabled_    = nullptr;
thread_local TListBox_SetEnabled    TListBox_SetEnabled_    = nullptr;
thread_local TListBox_SetOnClick    TListBox_SetOnClick_    = nullptr;

thread_local TMemo_Create        TMemo_Create_        = nullptr;
thread_local TMemo_Destroy       TMemo_Destroy_       = nullptr;
thread_local TMemo_SetParent     TMemo_SetParent_     = nullptr;
thread_local TMemo_GetReadOnly   TMemo_GetReadOnly_   = nullptr;
thread_local TMemo_SetReadOnly   TMemo_SetReadOnly_   = nullptr;
thread_local TMemo_GetScrollBars TMemo_GetScrollBars_ = nullptr;
thread_local TMemo_SetScrollBars TMemo_SetScrollBars_ = nullptr;
thread_local TMemo_Lines_Add     TMemo_Lines_Add_     = nullptr;
thread_local TMemo_Lines_Clear   TMemo_Lines_Clear_   = nullptr;
thread_local TMemo_Lines_Count   TMemo_Lines_Count_   = nullptr;
thread_local TMemo_Lines_GetText TMemo_Lines_GetText_ = nullptr;
thread_local TMemo_GetLeft       TMemo_GetLeft_       = nullptr;
thread_local TMemo_SetLeft       TMemo_SetLeft_       = nullptr;
thread_local TMemo_GetTop        TMemo_GetTop_        = nullptr;
thread_local TMemo_SetTop        TMemo_SetTop_        = nullptr;
thread_local TMemo_GetWidth      TMemo_GetWidth_      = nullptr;
thread_local TMemo_SetWidth      TMemo_SetWidth_      = nullptr;
thread_local TMemo_GetHeight     TMemo_GetHeight_     = nullptr;
thread_local TMemo_SetHeight     TMemo_SetHeight_     = nullptr;
thread_local TMemo_GetVisible    TMemo_GetVisible_    = nullptr;
thread_local TMemo_SetVisible    TMemo_SetVisible_    = nullptr;
thread_local TMemo_GetEnabled    TMemo_GetEnabled_    = nullptr;
thread_local TMemo_SetEnabled    TMemo_SetEnabled_    = nullptr;
thread_local TMemo_SetOnChange   TMemo_SetOnChange_   = nullptr;

thread_local TTimer_Create      TTimer_Create_      = nullptr;
thread_local TTimer_Destroy     TTimer_Destroy_     = nullptr;
thread_local TTimer_GetInterval TTimer_GetInterval_ = nullptr;
thread_local TTimer_SetInterval TTimer_SetInterval_ = nullptr;
thread_local TTimer_GetEnabled  TTimer_GetEnabled_  = nullptr;
thread_local TTimer_SetEnabled  TTimer_SetEnabled_  = nullptr;
thread_local TTimer_SetOnTimer  TTimer_SetOnTimer_  = nullptr;

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

    NO_VCL_MAP(m, TPanel_Create);
    NO_VCL_MAP(m, TPanel_Destroy);
    NO_VCL_MAP(m, TPanel_SetParent);
    NO_VCL_MAP(m, TPanel_GetCaption);
    NO_VCL_MAP(m, TPanel_SetCaption);
    NO_VCL_MAP(m, TPanel_GetLeft);
    NO_VCL_MAP(m, TPanel_SetLeft);
    NO_VCL_MAP(m, TPanel_GetTop);
    NO_VCL_MAP(m, TPanel_SetTop);
    NO_VCL_MAP(m, TPanel_GetWidth);
    NO_VCL_MAP(m, TPanel_SetWidth);
    NO_VCL_MAP(m, TPanel_GetHeight);
    NO_VCL_MAP(m, TPanel_SetHeight);
    NO_VCL_MAP(m, TPanel_GetVisible);
    NO_VCL_MAP(m, TPanel_SetVisible);
    NO_VCL_MAP(m, TPanel_GetEnabled);
    NO_VCL_MAP(m, TPanel_SetEnabled);

    NO_VCL_MAP(m, TGroupBox_Create);
    NO_VCL_MAP(m, TGroupBox_Destroy);
    NO_VCL_MAP(m, TGroupBox_SetParent);
    NO_VCL_MAP(m, TGroupBox_GetCaption);
    NO_VCL_MAP(m, TGroupBox_SetCaption);
    NO_VCL_MAP(m, TGroupBox_GetLeft);
    NO_VCL_MAP(m, TGroupBox_SetLeft);
    NO_VCL_MAP(m, TGroupBox_GetTop);
    NO_VCL_MAP(m, TGroupBox_SetTop);
    NO_VCL_MAP(m, TGroupBox_GetWidth);
    NO_VCL_MAP(m, TGroupBox_SetWidth);
    NO_VCL_MAP(m, TGroupBox_GetHeight);
    NO_VCL_MAP(m, TGroupBox_SetHeight);
    NO_VCL_MAP(m, TGroupBox_GetVisible);
    NO_VCL_MAP(m, TGroupBox_SetVisible);
    NO_VCL_MAP(m, TGroupBox_GetEnabled);
    NO_VCL_MAP(m, TGroupBox_SetEnabled);

    NO_VCL_MAP(m, TComboBox_Create);
    NO_VCL_MAP(m, TComboBox_Destroy);
    NO_VCL_MAP(m, TComboBox_SetParent);
    NO_VCL_MAP(m, TComboBox_GetText);
    NO_VCL_MAP(m, TComboBox_SetText);
    NO_VCL_MAP(m, TComboBox_GetItemIndex);
    NO_VCL_MAP(m, TComboBox_SetItemIndex);
    NO_VCL_MAP(m, TComboBox_Items_Add);
    NO_VCL_MAP(m, TComboBox_Items_Clear);
    NO_VCL_MAP(m, TComboBox_Items_Count);
    NO_VCL_MAP(m, TComboBox_Items_GetText);
    NO_VCL_MAP(m, TComboBox_GetLeft);
    NO_VCL_MAP(m, TComboBox_SetLeft);
    NO_VCL_MAP(m, TComboBox_GetTop);
    NO_VCL_MAP(m, TComboBox_SetTop);
    NO_VCL_MAP(m, TComboBox_GetWidth);
    NO_VCL_MAP(m, TComboBox_SetWidth);
    NO_VCL_MAP(m, TComboBox_GetHeight);
    NO_VCL_MAP(m, TComboBox_SetHeight);
    NO_VCL_MAP(m, TComboBox_GetVisible);
    NO_VCL_MAP(m, TComboBox_SetVisible);
    NO_VCL_MAP(m, TComboBox_GetEnabled);
    NO_VCL_MAP(m, TComboBox_SetEnabled);
    NO_VCL_MAP(m, TComboBox_SetOnChange);

    NO_VCL_MAP(m, TListBox_Create);
    NO_VCL_MAP(m, TListBox_Destroy);
    NO_VCL_MAP(m, TListBox_SetParent);
    NO_VCL_MAP(m, TListBox_GetItemIndex);
    NO_VCL_MAP(m, TListBox_SetItemIndex);
    NO_VCL_MAP(m, TListBox_Items_Add);
    NO_VCL_MAP(m, TListBox_Items_Clear);
    NO_VCL_MAP(m, TListBox_Items_Count);
    NO_VCL_MAP(m, TListBox_Items_GetText);
    NO_VCL_MAP(m, TListBox_GetLeft);
    NO_VCL_MAP(m, TListBox_SetLeft);
    NO_VCL_MAP(m, TListBox_GetTop);
    NO_VCL_MAP(m, TListBox_SetTop);
    NO_VCL_MAP(m, TListBox_GetWidth);
    NO_VCL_MAP(m, TListBox_SetWidth);
    NO_VCL_MAP(m, TListBox_GetHeight);
    NO_VCL_MAP(m, TListBox_SetHeight);
    NO_VCL_MAP(m, TListBox_GetVisible);
    NO_VCL_MAP(m, TListBox_SetVisible);
    NO_VCL_MAP(m, TListBox_GetEnabled);
    NO_VCL_MAP(m, TListBox_SetEnabled);
    NO_VCL_MAP(m, TListBox_SetOnClick);

    NO_VCL_MAP(m, TMemo_Create);
    NO_VCL_MAP(m, TMemo_Destroy);
    NO_VCL_MAP(m, TMemo_SetParent);
    NO_VCL_MAP(m, TMemo_GetReadOnly);
    NO_VCL_MAP(m, TMemo_SetReadOnly);
    NO_VCL_MAP(m, TMemo_GetScrollBars);
    NO_VCL_MAP(m, TMemo_SetScrollBars);
    NO_VCL_MAP(m, TMemo_Lines_Add);
    NO_VCL_MAP(m, TMemo_Lines_Clear);
    NO_VCL_MAP(m, TMemo_Lines_Count);
    NO_VCL_MAP(m, TMemo_Lines_GetText);
    NO_VCL_MAP(m, TMemo_GetLeft);
    NO_VCL_MAP(m, TMemo_SetLeft);
    NO_VCL_MAP(m, TMemo_GetTop);
    NO_VCL_MAP(m, TMemo_SetTop);
    NO_VCL_MAP(m, TMemo_GetWidth);
    NO_VCL_MAP(m, TMemo_SetWidth);
    NO_VCL_MAP(m, TMemo_GetHeight);
    NO_VCL_MAP(m, TMemo_SetHeight);
    NO_VCL_MAP(m, TMemo_GetVisible);
    NO_VCL_MAP(m, TMemo_SetVisible);
    NO_VCL_MAP(m, TMemo_GetEnabled);
    NO_VCL_MAP(m, TMemo_SetEnabled);
    NO_VCL_MAP(m, TMemo_SetOnChange);

    NO_VCL_MAP(m, TTimer_Create);
    NO_VCL_MAP(m, TTimer_Destroy);
    NO_VCL_MAP(m, TTimer_GetInterval);
    NO_VCL_MAP(m, TTimer_SetInterval);
    NO_VCL_MAP(m, TTimer_GetEnabled);
    NO_VCL_MAP(m, TTimer_SetEnabled);
    NO_VCL_MAP(m, TTimer_SetOnTimer);
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

no_vcl_obj_t NO_VCL_CALL no_vcl_TPanel_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TPanel_Create_);
    return TPanel_Create_(owner);
}

void NO_VCL_CALL no_vcl_TPanel_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TPanel_Destroy_);
    TPanel_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TPanel_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TPanel_SetParent_);
    TPanel_SetParent_(obj, parentObj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TPanel_GetCaption(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TPanel_GetCaption_);
    return TPanel_GetCaption_(obj);
}

void NO_VCL_CALL no_vcl_TPanel_SetCaption(no_vcl_obj_t obj, no_vcl_str_t cap)
{
    NO_VCL_INIT_CHECK(TPanel_SetCaption_);
    TPanel_SetCaption_(obj, cap);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TPanel_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TPanel_GetLeft_);
    return TPanel_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TPanel_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TPanel_SetLeft_);
    TPanel_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TPanel_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TPanel_GetTop_);
    return TPanel_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TPanel_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TPanel_SetTop_);
    TPanel_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TPanel_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TPanel_GetWidth_);
    return TPanel_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TPanel_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TPanel_SetWidth_);
    TPanel_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TPanel_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TPanel_GetHeight_);
    return TPanel_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TPanel_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TPanel_SetHeight_);
    TPanel_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TPanel_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TPanel_GetVisible_);
    return TPanel_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TPanel_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TPanel_SetVisible_);
    TPanel_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TPanel_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TPanel_GetEnabled_);
    return TPanel_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TPanel_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TPanel_SetEnabled_);
    TPanel_SetEnabled_(obj, value);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TGroupBox_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TGroupBox_Create_);
    return TGroupBox_Create_(owner);
}

void NO_VCL_CALL no_vcl_TGroupBox_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TGroupBox_Destroy_);
    TGroupBox_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TGroupBox_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TGroupBox_SetParent_);
    TGroupBox_SetParent_(obj, parentObj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TGroupBox_GetCaption(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TGroupBox_GetCaption_);
    return TGroupBox_GetCaption_(obj);
}

void NO_VCL_CALL no_vcl_TGroupBox_SetCaption(no_vcl_obj_t obj, no_vcl_str_t cap)
{
    NO_VCL_INIT_CHECK(TGroupBox_SetCaption_);
    TGroupBox_SetCaption_(obj, cap);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TGroupBox_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TGroupBox_GetLeft_);
    return TGroupBox_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TGroupBox_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TGroupBox_SetLeft_);
    TGroupBox_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TGroupBox_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TGroupBox_GetTop_);
    return TGroupBox_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TGroupBox_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TGroupBox_SetTop_);
    TGroupBox_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TGroupBox_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TGroupBox_GetWidth_);
    return TGroupBox_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TGroupBox_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TGroupBox_SetWidth_);
    TGroupBox_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TGroupBox_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TGroupBox_GetHeight_);
    return TGroupBox_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TGroupBox_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TGroupBox_SetHeight_);
    TGroupBox_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TGroupBox_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TGroupBox_GetVisible_);
    return TGroupBox_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TGroupBox_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TGroupBox_SetVisible_);
    TGroupBox_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TGroupBox_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TGroupBox_GetEnabled_);
    return TGroupBox_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TGroupBox_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TGroupBox_SetEnabled_);
    TGroupBox_SetEnabled_(obj, value);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TComboBox_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TComboBox_Create_);
    return TComboBox_Create_(owner);
}

void NO_VCL_CALL no_vcl_TComboBox_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_Destroy_);
    TComboBox_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TComboBox_SetParent_);
    TComboBox_SetParent_(obj, parentObj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TComboBox_GetText(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_GetText_);
    return TComboBox_GetText_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetText(no_vcl_obj_t obj, no_vcl_str_t text)
{
    NO_VCL_INIT_CHECK(TComboBox_SetText_);
    TComboBox_SetText_(obj, text);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetItemIndex(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_GetItemIndex_);
    return TComboBox_GetItemIndex_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetItemIndex(no_vcl_obj_t obj, no_vcl_int_t value)
{
    NO_VCL_INIT_CHECK(TComboBox_SetItemIndex_);
    TComboBox_SetItemIndex_(obj, value);
}

void NO_VCL_CALL no_vcl_TComboBox_Items_Add(no_vcl_obj_t obj, no_vcl_str_t text)
{
    NO_VCL_INIT_CHECK(TComboBox_Items_Add_);
    TComboBox_Items_Add_(obj, text);
}

void NO_VCL_CALL no_vcl_TComboBox_Items_Clear(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_Items_Clear_);
    TComboBox_Items_Clear_(obj);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_Items_Count(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_Items_Count_);
    return TComboBox_Items_Count_(obj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TComboBox_Items_GetText(no_vcl_obj_t obj, no_vcl_int_t index)
{
    NO_VCL_INIT_CHECK(TComboBox_Items_GetText_);
    return TComboBox_Items_GetText_(obj, index);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_GetLeft_);
    return TComboBox_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TComboBox_SetLeft_);
    TComboBox_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_GetTop_);
    return TComboBox_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TComboBox_SetTop_);
    TComboBox_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_GetWidth_);
    return TComboBox_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TComboBox_SetWidth_);
    TComboBox_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_GetHeight_);
    return TComboBox_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TComboBox_SetHeight_);
    TComboBox_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TComboBox_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_GetVisible_);
    return TComboBox_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TComboBox_SetVisible_);
    TComboBox_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TComboBox_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TComboBox_GetEnabled_);
    return TComboBox_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TComboBox_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TComboBox_SetEnabled_);
    TComboBox_SetEnabled_(obj, value);
}

void NO_VCL_CALL no_vcl_TComboBox_SetOnChange(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TComboBox_SetOnChange_);
    TComboBox_SetOnChange_(obj, cb);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TListBox_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TListBox_Create_);
    return TListBox_Create_(owner);
}

void NO_VCL_CALL no_vcl_TListBox_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_Destroy_);
    TListBox_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TListBox_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TListBox_SetParent_);
    TListBox_SetParent_(obj, parentObj);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetItemIndex(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_GetItemIndex_);
    return TListBox_GetItemIndex_(obj);
}

void NO_VCL_CALL no_vcl_TListBox_SetItemIndex(no_vcl_obj_t obj, no_vcl_int_t value)
{
    NO_VCL_INIT_CHECK(TListBox_SetItemIndex_);
    TListBox_SetItemIndex_(obj, value);
}

void NO_VCL_CALL no_vcl_TListBox_Items_Add(no_vcl_obj_t obj, no_vcl_str_t text)
{
    NO_VCL_INIT_CHECK(TListBox_Items_Add_);
    TListBox_Items_Add_(obj, text);
}

void NO_VCL_CALL no_vcl_TListBox_Items_Clear(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_Items_Clear_);
    TListBox_Items_Clear_(obj);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_Items_Count(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_Items_Count_);
    return TListBox_Items_Count_(obj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TListBox_Items_GetText(no_vcl_obj_t obj, no_vcl_int_t index)
{
    NO_VCL_INIT_CHECK(TListBox_Items_GetText_);
    return TListBox_Items_GetText_(obj, index);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_GetLeft_);
    return TListBox_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TListBox_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TListBox_SetLeft_);
    TListBox_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_GetTop_);
    return TListBox_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TListBox_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TListBox_SetTop_);
    TListBox_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_GetWidth_);
    return TListBox_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TListBox_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TListBox_SetWidth_);
    TListBox_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_GetHeight_);
    return TListBox_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TListBox_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TListBox_SetHeight_);
    TListBox_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TListBox_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_GetVisible_);
    return TListBox_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TListBox_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TListBox_SetVisible_);
    TListBox_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TListBox_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TListBox_GetEnabled_);
    return TListBox_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TListBox_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TListBox_SetEnabled_);
    TListBox_SetEnabled_(obj, value);
}

void NO_VCL_CALL no_vcl_TListBox_SetOnClick(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TListBox_SetOnClick_);
    TListBox_SetOnClick_(obj, cb);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TMemo_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TMemo_Create_);
    return TMemo_Create_(owner);
}

void NO_VCL_CALL no_vcl_TMemo_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_Destroy_);
    TMemo_Destroy_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetParent(no_vcl_obj_t obj, no_vcl_obj_t parentObj)
{
    NO_VCL_INIT_CHECK(TMemo_SetParent_);
    TMemo_SetParent_(obj, parentObj);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TMemo_GetReadOnly(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_GetReadOnly_);
    return TMemo_GetReadOnly_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetReadOnly(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TMemo_SetReadOnly_);
    TMemo_SetReadOnly_(obj, value);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetScrollBars(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_GetScrollBars_);
    return TMemo_GetScrollBars_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetScrollBars(no_vcl_obj_t obj, no_vcl_int_t value)
{
    NO_VCL_INIT_CHECK(TMemo_SetScrollBars_);
    TMemo_SetScrollBars_(obj, value);
}

void NO_VCL_CALL no_vcl_TMemo_Lines_Add(no_vcl_obj_t obj, no_vcl_str_t text)
{
    NO_VCL_INIT_CHECK(TMemo_Lines_Add_);
    TMemo_Lines_Add_(obj, text);
}

void NO_VCL_CALL no_vcl_TMemo_Lines_Clear(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_Lines_Clear_);
    TMemo_Lines_Clear_(obj);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_Lines_Count(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_Lines_Count_);
    return TMemo_Lines_Count_(obj);
}

no_vcl_str_t NO_VCL_CALL no_vcl_TMemo_Lines_GetText(no_vcl_obj_t obj, no_vcl_int_t index)
{
    NO_VCL_INIT_CHECK(TMemo_Lines_GetText_);
    return TMemo_Lines_GetText_(obj, index);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetLeft(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_GetLeft_);
    return TMemo_GetLeft_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetLeft(no_vcl_obj_t obj, no_vcl_int_t left)
{
    NO_VCL_INIT_CHECK(TMemo_SetLeft_);
    TMemo_SetLeft_(obj, left);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetTop(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_GetTop_);
    return TMemo_GetTop_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetTop(no_vcl_obj_t obj, no_vcl_int_t top)
{
    NO_VCL_INIT_CHECK(TMemo_SetTop_);
    TMemo_SetTop_(obj, top);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetWidth(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_GetWidth_);
    return TMemo_GetWidth_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetWidth(no_vcl_obj_t obj, no_vcl_int_t width)
{
    NO_VCL_INIT_CHECK(TMemo_SetWidth_);
    TMemo_SetWidth_(obj, width);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetHeight(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_GetHeight_);
    return TMemo_GetHeight_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetHeight(no_vcl_obj_t obj, no_vcl_int_t height)
{
    NO_VCL_INIT_CHECK(TMemo_SetHeight_);
    TMemo_SetHeight_(obj, height);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TMemo_GetVisible(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_GetVisible_);
    return TMemo_GetVisible_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetVisible(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TMemo_SetVisible_);
    TMemo_SetVisible_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TMemo_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TMemo_GetEnabled_);
    return TMemo_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TMemo_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TMemo_SetEnabled_);
    TMemo_SetEnabled_(obj, value);
}

void NO_VCL_CALL no_vcl_TMemo_SetOnChange(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TMemo_SetOnChange_);
    TMemo_SetOnChange_(obj, cb);
}

no_vcl_obj_t NO_VCL_CALL no_vcl_TTimer_Create(no_vcl_obj_t owner)
{
    NO_VCL_INIT_CHECK(TTimer_Create_);
    return TTimer_Create_(owner);
}

void NO_VCL_CALL no_vcl_TTimer_Destroy(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TTimer_Destroy_);
    TTimer_Destroy_(obj);
}

no_vcl_int_t NO_VCL_CALL no_vcl_TTimer_GetInterval(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TTimer_GetInterval_);
    return TTimer_GetInterval_(obj);
}

void NO_VCL_CALL no_vcl_TTimer_SetInterval(no_vcl_obj_t obj, no_vcl_int_t value)
{
    NO_VCL_INIT_CHECK(TTimer_SetInterval_);
    TTimer_SetInterval_(obj, value);
}

no_vcl_bool_t NO_VCL_CALL no_vcl_TTimer_GetEnabled(no_vcl_obj_t obj)
{
    NO_VCL_INIT_CHECK(TTimer_GetEnabled_);
    return TTimer_GetEnabled_(obj);
}

void NO_VCL_CALL no_vcl_TTimer_SetEnabled(no_vcl_obj_t obj, no_vcl_bool_t value)
{
    NO_VCL_INIT_CHECK(TTimer_SetEnabled_);
    TTimer_SetEnabled_(obj, value);
}

void NO_VCL_CALL no_vcl_TTimer_SetOnTimer(no_vcl_obj_t obj, no_vcl_callback_t cb)
{
    NO_VCL_INIT_CHECK(TTimer_SetOnTimer_);
    TTimer_SetOnTimer_(obj, cb);
}

} // extern "C"
