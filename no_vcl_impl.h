#ifndef NO_VCL_IMPL_H
#define NO_VCL_IMPL_H

#if defined(_WIN32) || defined(_WIN64)
    #define NO_VCL_CALL __stdcall
#else
    #define NO_VCL_CALL __cdecl
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void*       no_vcl_obj_t;
typedef const char* no_vcl_str_t;
typedef int         no_vcl_int_t;
typedef int         no_vcl_bool_t;
typedef void (NO_VCL_CALL *no_vcl_callback_t)(no_vcl_obj_t sender);


/* TForm */
no_vcl_obj_t NO_VCL_CALL no_vcl_TForm_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TForm_Destroy(no_vcl_obj_t Obj);

no_vcl_str_t NO_VCL_CALL no_vcl_TForm_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TForm_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

no_vcl_int_t NO_VCL_CALL no_vcl_TForm_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TForm_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TForm_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TForm_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TForm_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TForm_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TForm_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TForm_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TForm_Show(no_vcl_obj_t Obj);
no_vcl_int_t NO_VCL_CALL no_vcl_TForm_ShowModal(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TForm_Hide(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TForm_Close(no_vcl_obj_t Obj);

/* Application */
void         NO_VCL_CALL no_vcl_Application_Run(void);
void         NO_VCL_CALL no_vcl_Application_ProcessMessages(void);

/* TButton */
no_vcl_obj_t NO_VCL_CALL no_vcl_TButton_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TButton_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TButton_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_str_t NO_VCL_CALL no_vcl_TButton_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TButton_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

no_vcl_int_t NO_VCL_CALL no_vcl_TButton_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TButton_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TButton_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TButton_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TButton_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TButton_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TButton_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TButton_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TButton_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TButton_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TButton_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TButton_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TButton_SetOnClick(no_vcl_obj_t obj, no_vcl_callback_t cb);

/* TLabel */
no_vcl_obj_t NO_VCL_CALL no_vcl_TLabel_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TLabel_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TLabel_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_str_t NO_VCL_CALL no_vcl_TLabel_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TLabel_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

no_vcl_int_t NO_VCL_CALL no_vcl_TLabel_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TLabel_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TLabel_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TLabel_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TLabel_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TLabel_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TLabel_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TLabel_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TLabel_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TLabel_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TLabel_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TLabel_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* TEdit */
no_vcl_obj_t NO_VCL_CALL no_vcl_TEdit_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TEdit_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TEdit_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_str_t NO_VCL_CALL no_vcl_TEdit_GetText(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TEdit_SetText(no_vcl_obj_t Obj, no_vcl_str_t Text);

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetMaxLength(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TEdit_SetMaxLength(no_vcl_obj_t Obj, no_vcl_int_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TEdit_GetReadOnly(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TEdit_SetReadOnly(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TEdit_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TEdit_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TEdit_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TEdit_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TEdit_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TEdit_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TEdit_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TEdit_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TEdit_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TEdit_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TCheckBox */
no_vcl_obj_t NO_VCL_CALL no_vcl_TCheckBox_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TCheckBox_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TCheckBox_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_str_t NO_VCL_CALL no_vcl_TCheckBox_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TCheckBox_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

no_vcl_bool_t NO_VCL_CALL no_vcl_TCheckBox_GetChecked(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCheckBox_SetChecked(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_int_t NO_VCL_CALL no_vcl_TCheckBox_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TCheckBox_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TCheckBox_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TCheckBox_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TCheckBox_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TCheckBox_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TCheckBox_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TCheckBox_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TCheckBox_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCheckBox_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TCheckBox_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCheckBox_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TCheckBox_SetOnClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TRadioButton */
no_vcl_obj_t NO_VCL_CALL no_vcl_TRadioButton_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TRadioButton_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TRadioButton_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_str_t NO_VCL_CALL no_vcl_TRadioButton_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TRadioButton_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

no_vcl_bool_t NO_VCL_CALL no_vcl_TRadioButton_GetChecked(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TRadioButton_SetChecked(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_int_t NO_VCL_CALL no_vcl_TRadioButton_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TRadioButton_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TRadioButton_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TRadioButton_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TRadioButton_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TRadioButton_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TRadioButton_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TRadioButton_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TRadioButton_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TRadioButton_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TRadioButton_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TRadioButton_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TRadioButton_SetOnClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TPanel */
no_vcl_obj_t NO_VCL_CALL no_vcl_TPanel_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TPanel_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TPanel_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_str_t NO_VCL_CALL no_vcl_TPanel_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TPanel_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

no_vcl_int_t NO_VCL_CALL no_vcl_TPanel_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TPanel_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TPanel_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TPanel_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TPanel_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TPanel_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TPanel_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TPanel_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TPanel_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPanel_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TPanel_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPanel_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* TGroupBox */
no_vcl_obj_t NO_VCL_CALL no_vcl_TGroupBox_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TGroupBox_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TGroupBox_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_str_t NO_VCL_CALL no_vcl_TGroupBox_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TGroupBox_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

no_vcl_int_t NO_VCL_CALL no_vcl_TGroupBox_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TGroupBox_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TGroupBox_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TGroupBox_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TGroupBox_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TGroupBox_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TGroupBox_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TGroupBox_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TGroupBox_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TGroupBox_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TGroupBox_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TGroupBox_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* TComboBox */
no_vcl_obj_t NO_VCL_CALL no_vcl_TComboBox_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TComboBox_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TComboBox_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_str_t NO_VCL_CALL no_vcl_TComboBox_GetText(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TComboBox_SetText(no_vcl_obj_t Obj, no_vcl_str_t Text);

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetItemIndex(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TComboBox_SetItemIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);

void         NO_VCL_CALL no_vcl_TComboBox_Items_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void         NO_VCL_CALL no_vcl_TComboBox_Items_Clear(no_vcl_obj_t Obj);
no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_Items_Count(no_vcl_obj_t Obj);
no_vcl_str_t NO_VCL_CALL no_vcl_TComboBox_Items_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TComboBox_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TComboBox_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TComboBox_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TComboBox_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TComboBox_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TComboBox_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TComboBox_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TComboBox_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TComboBox_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TComboBox_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TListBox */
no_vcl_obj_t NO_VCL_CALL no_vcl_TListBox_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TListBox_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TListBox_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetItemIndex(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TListBox_SetItemIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);

void         NO_VCL_CALL no_vcl_TListBox_Items_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void         NO_VCL_CALL no_vcl_TListBox_Items_Clear(no_vcl_obj_t Obj);
no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_Items_Count(no_vcl_obj_t Obj);
no_vcl_str_t NO_VCL_CALL no_vcl_TListBox_Items_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TListBox_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TListBox_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TListBox_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TListBox_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TListBox_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TListBox_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListBox_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TListBox_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListBox_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TListBox_SetOnClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TMemo */
no_vcl_obj_t NO_VCL_CALL no_vcl_TMemo_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TMemo_Destroy(no_vcl_obj_t Obj);

void         NO_VCL_CALL no_vcl_TMemo_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_bool_t NO_VCL_CALL no_vcl_TMemo_GetReadOnly(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMemo_SetReadOnly(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetScrollBars(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TMemo_SetScrollBars(no_vcl_obj_t Obj, no_vcl_int_t Value);

void         NO_VCL_CALL no_vcl_TMemo_Lines_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void         NO_VCL_CALL no_vcl_TMemo_Lines_Clear(no_vcl_obj_t Obj);
no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_Lines_Count(no_vcl_obj_t Obj);
no_vcl_str_t NO_VCL_CALL no_vcl_TMemo_Lines_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetLeft(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TMemo_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Left);

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetTop(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TMemo_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Top);

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TMemo_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TMemo_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TMemo_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

no_vcl_bool_t NO_VCL_CALL no_vcl_TMemo_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMemo_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TMemo_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMemo_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TMemo_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

/* TTimer */
no_vcl_obj_t NO_VCL_CALL no_vcl_TTimer_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TTimer_Destroy(no_vcl_obj_t Obj);

no_vcl_int_t NO_VCL_CALL no_vcl_TTimer_GetInterval(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TTimer_SetInterval(no_vcl_obj_t Obj, no_vcl_int_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TTimer_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTimer_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

void         NO_VCL_CALL no_vcl_TTimer_SetOnTimer(no_vcl_obj_t Obj, no_vcl_callback_t Cb);

#ifdef __cplusplus
}
#endif

#endif
