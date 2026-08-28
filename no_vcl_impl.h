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

#ifdef __cplusplus
}
#endif

#endif
