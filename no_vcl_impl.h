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

void         NO_VCL_CALL no_vcl_TButton_SetOnClick(no_vcl_obj_t obj, no_vcl_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif
