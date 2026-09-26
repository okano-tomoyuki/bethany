#ifndef NO_VCL_C_H
#define NO_VCL_C_H

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
/* sender はイベントを発生させたオブジェクト、data はコールバック登録時に渡した利用者データ。 */
typedef void (NO_VCL_CALL *no_vcl_callback_t)(no_vcl_obj_t sender, void* data);

/*
 * 関数は「LCL でそのメンバが公開(public/published)されるクラス」の名前で 1 本ずつ用意する。
 * 例えば Left/Top 等は TControl で公開されているため、TButton でも TLabel でも
 * no_vcl_TControl_GetLeft を使う。どの関数がどのオブジェクトに使えるかは
 * docs/class-hierarchy.md の継承関係に従う(派生クラスのオブジェクトは基底クラスの関数に渡してよい)。
 *
 * *_Create の Owner は LCL の Owner(破棄の責任を持つコンポーネント)で、NULL も可。
 * 画面上の親(Parent)は no_vcl_TControl_SetParent で別途設定する。
 */

/*
 * 破棄通知: *_Create で生成したコンポーネントが破棄されると、原因(no_vcl_TComponent_Destroy・
 * Owner による連鎖破棄など)にかかわらず、登録したコールバックが破棄されるオブジェクトを引数に呼ばれる。
 * Owner が破棄されると所有されているコンポーネントも破棄されるため、保持しているハンドルが
 * 無効になったことを知る手段として使う。コールバックは 1 つだけ登録でき、C++ ラッパー(no_vcl.hpp)を
 * 使う場合はラッパーが登録するため上書きしないこと。
 */
void          NO_VCL_CALL no_vcl_FreeNotify_SetCallback(no_vcl_callback_t Cb, void* Data);

/* TComponent */
void          NO_VCL_CALL no_vcl_TComponent_Destroy(no_vcl_obj_t Obj);
/* 所有しているコンポーネントをすべて破棄する(Obj 自身は残る)。 */
void          NO_VCL_CALL no_vcl_TComponent_DestroyComponents(no_vcl_obj_t Obj);

/* TControl */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TControl_GetParent(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetParent(no_vcl_obj_t Obj, no_vcl_obj_t ParentObj);

no_vcl_int_t  NO_VCL_CALL no_vcl_TControl_GetLeft(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetLeft(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TControl_GetTop(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetTop(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TControl_GetWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TControl_GetHeight(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetHeight(no_vcl_obj_t Obj, no_vcl_int_t Value);

no_vcl_bool_t NO_VCL_CALL no_vcl_TControl_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TControl_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);

no_vcl_str_t  NO_VCL_CALL no_vcl_TControl_GetCaption(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Value);

/* Text は LCL では TControl の protected。公開しているのは TCustomEdit / TCustomComboBox の系統。 */
no_vcl_str_t  NO_VCL_CALL no_vcl_TControl_GetText(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetText(no_vcl_obj_t Obj, no_vcl_str_t Value);

void          NO_VCL_CALL no_vcl_TControl_Show(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_Hide(no_vcl_obj_t Obj);

void          NO_VCL_CALL no_vcl_TControl_SetOnClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* TCustomForm / TForm */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TForm_Create(no_vcl_obj_t Owner);
void          NO_VCL_CALL no_vcl_TCustomForm_Show(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomForm_Hide(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomForm_ShowModal(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomForm_Close(no_vcl_obj_t Obj);

/* TApplication
 * no_vcl_GetApplication は LCL のグローバルな Application(DLL の読み込み時に初期化済み)を返す。
 * no_vcl_TApplication_CreateForm は Application を Owner とする TForm を生成して返す。
 * 最初に生成したフォームが MainForm になり、no_vcl_TApplication_Run はそれを表示して
 * メッセージループに入る(MainForm が閉じられると戻る)。
 * Application が所有するフォームは DLL の切り離し時に LCL が破棄するが、その時点では破棄通知は
 * 呼ばれない。破棄を通知で受けたい場合は、終了前に no_vcl_TComponent_DestroyComponents(app) を呼ぶ。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_GetApplication(void);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TApplication_CreateForm(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TApplication_GetMainForm(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TApplication_Run(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TApplication_ProcessMessages(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TApplication_Terminate(no_vcl_obj_t Obj);
no_vcl_bool_t NO_VCL_CALL no_vcl_TApplication_GetTerminated(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TApplication_GetTitle(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TApplication_SetTitle(no_vcl_obj_t Obj, no_vcl_str_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TApplication_GetShowMainForm(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TApplication_SetShowMainForm(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* TPanel / TGroupBox / TLabel */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPanel_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TGroupBox_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TLabel_Create(no_vcl_obj_t Owner);

/* TButtonControl / TButton / TCheckBox / TRadioButton */
/* Checked は LCL では TButtonControl の protected。公開しているのは TCheckBox / TRadioButton。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TButtonControl_GetChecked(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TButtonControl_SetChecked(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TButton_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCheckBox_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TRadioButton_Create(no_vcl_obj_t Owner);

/* TCustomEdit / TEdit (TMemo も TCustomEdit の派生) */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomEdit_GetMaxLength(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomEdit_SetMaxLength(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomEdit_GetReadOnly(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomEdit_SetReadOnly(no_vcl_obj_t Obj, no_vcl_bool_t Value);
void          NO_VCL_CALL no_vcl_TCustomEdit_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TEdit_Create(no_vcl_obj_t Owner);

/* TCustomMemo / TMemo */
void          NO_VCL_CALL no_vcl_TCustomMemo_Lines_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TCustomMemo_Lines_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomMemo_Lines_Count(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TCustomMemo_Lines_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomMemo_GetScrollBars(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomMemo_SetScrollBars(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TMemo_Create(no_vcl_obj_t Owner);

/* TCustomComboBox / TComboBox */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomComboBox_GetItemIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomComboBox_SetItemIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
void          NO_VCL_CALL no_vcl_TCustomComboBox_Items_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TCustomComboBox_Items_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomComboBox_Items_Count(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TCustomComboBox_Items_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TComboBox_Create(no_vcl_obj_t Owner);
/* OnChange は TCustomComboBox では protected で、公開しているのは TComboBox だけ。 */
void          NO_VCL_CALL no_vcl_TComboBox_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* TCustomListBox / TListBox */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomListBox_GetItemIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListBox_SetItemIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
void          NO_VCL_CALL no_vcl_TCustomListBox_Items_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TCustomListBox_Items_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomListBox_Items_Count(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TCustomListBox_Items_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListBox_Create(no_vcl_obj_t Owner);

/* TCustomTimer / TTimer */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomTimer_GetInterval(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTimer_SetInterval(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomTimer_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTimer_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);
void          NO_VCL_CALL no_vcl_TCustomTimer_SetOnTimer(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTimer_Create(no_vcl_obj_t Owner);

/* TPaintBox */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPaintBox_Create(no_vcl_obj_t Owner);
/* Canvas はコントロールが内部で保持するオブジェクトの参照を返すだけで、
   独自の Create/Destroy は持たない(コントロール破棄時に一緒に破棄される) */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPaintBox_GetCanvas(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPaintBox_SetOnPaint(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* TCanvas (非所有: Create/Destroy なし) */
void          NO_VCL_CALL no_vcl_TCanvas_MoveTo(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y);
void          NO_VCL_CALL no_vcl_TCanvas_LineTo(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y);
void          NO_VCL_CALL no_vcl_TCanvas_Rectangle(no_vcl_obj_t Obj, no_vcl_int_t X1, no_vcl_int_t Y1, no_vcl_int_t X2, no_vcl_int_t Y2);
void          NO_VCL_CALL no_vcl_TCanvas_Ellipse(no_vcl_obj_t Obj, no_vcl_int_t X1, no_vcl_int_t Y1, no_vcl_int_t X2, no_vcl_int_t Y2);
void          NO_VCL_CALL no_vcl_TCanvas_TextOut(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y, no_vcl_str_t Text);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCanvas_GetPen(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCanvas_GetBrush(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCanvas_GetFont(no_vcl_obj_t Obj);

/* TPen / TBrush / TFont (非所有: Create/Destroy なし) */
no_vcl_int_t  NO_VCL_CALL no_vcl_TPen_GetColor(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPen_SetColor(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TPen_GetWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPen_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);

no_vcl_int_t  NO_VCL_CALL no_vcl_TBrush_GetColor(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TBrush_SetColor(no_vcl_obj_t Obj, no_vcl_int_t Value);

no_vcl_str_t  NO_VCL_CALL no_vcl_TFont_GetName(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TFont_SetName(no_vcl_obj_t Obj, no_vcl_str_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TFont_GetSize(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TFont_SetSize(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TFont_GetColor(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TFont_SetColor(no_vcl_obj_t Obj, no_vcl_int_t Value);

#ifdef __cplusplus
}
#endif

#endif
