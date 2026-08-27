#ifndef NO_VCL_IMPL_H
#define NO_VCL_IMPL_H

#ifdef _WIN32 || _WIN64
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

/* TForm */
no_vcl_obj_t NO_VCL_CALL no_vcl_TForm_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TForm_Destroy(no_vcl_obj_t Obj);

no_vcl_str_t NO_VCL_CALL no_vcl_TForm_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TForm_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

no_vcl_int_t NO_VCL_CALL no_vcl_TForm_GetWidth(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TForm_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Width);

no_vcl_int_t NO_VCL_CALL no_vcl_TForm_GetHeight(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TForm_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Height);

/* TButton */
no_vcl_obj_t NO_VCL_CALL no_vcl_TButton_Create(no_vcl_obj_t Owner);
void         NO_VCL_CALL no_vcl_TButton_Destroy(no_vcl_obj_t Obj);

no_vcl_str_t NO_VCL_CALL no_vcl_TButton_GetCaption(no_vcl_obj_t Obj);
void         NO_VCL_CALL no_vcl_TButton_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Caption);

#ifdef __cplusplus
}
#endif

#endif
