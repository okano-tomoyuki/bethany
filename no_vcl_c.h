#ifndef NO_VCL_C_H
#define NO_VCL_C_H

#if defined(_WIN32) || defined(_WIN64)
    #define NO_VCL_CALL __stdcall
#else
    /* x86_64 Linux では呼び出し規約は1種類しかなく、__cdecl はキーワードとして存在しない。 */
    #define NO_VCL_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void*       no_vcl_obj_t;
/* 文字列は UTF-8。関数が返す文字列は DLL 内のスレッドごとのバッファを指し、同じスレッドで次に文字列を返す関数を
   呼ぶまで有効(解放は不要)。保持する場合や、2 つの戻り値を同時に使う場合(printf の引数に 2 つ並べる等)はコピーすること。 */
typedef const char* no_vcl_str_t;
typedef int         no_vcl_int_t;
typedef int         no_vcl_bool_t;
typedef double      no_vcl_float_t;
/* sender はイベントを発生させたオブジェクト、data はコールバック登録時に渡した利用者データ。
   イベントの登録関数(no_vcl_*_SetOnXxx)は同じイベントに何度呼んでもよく、最後に登録したものだけが呼ばれる。
   コールバックに NULL を渡すとハンドラを解除する(コールバックの実行中に解除してもよい)。 */
typedef void (NO_VCL_CALL *no_vcl_callback_t)(no_vcl_obj_t sender, void* data);

/* TCustomForm の OnClose 用。*action は Close の動作(no_vcl_ca*)で、既定値が入った状態で呼ばれる。
   コールバックの中で書き換えると動作が変わる(例: no_vcl_caNone で閉じるのを取りやめる)。 */
typedef void (NO_VCL_CALL *no_vcl_close_callback_t)(no_vcl_obj_t sender, no_vcl_int_t* action, void* data);

/* TCustomForm の OnCloseQuery 用。*canClose は 0 以外(閉じてよい)が入った状態で呼ばれ、
   0 を書き込むと閉じるのを取りやめる(OnClose より前に呼ばれる)。 */
typedef void (NO_VCL_CALL *no_vcl_close_query_callback_t)(no_vcl_obj_t sender, no_vcl_bool_t* canClose, void* data);

/* キー入力・マウス操作のイベント用。Shift は修飾キー・マウスボタンの状態を表すビット集合で、
   複数のビットを OR して渡す/受け取る(押されていれば該当ビットが立つ)。 */
enum
{
    no_vcl_ssShift  = 0x0001,
    no_vcl_ssAlt    = 0x0002,
    no_vcl_ssCtrl   = 0x0004,
    no_vcl_ssLeft   = 0x0008,  /* マウスの左ボタンが押されている */
    no_vcl_ssRight  = 0x0010,
    no_vcl_ssMiddle = 0x0020,
    no_vcl_ssDouble = 0x0040,  /* ダブルクリックの一部として発生した */
    no_vcl_ssMeta   = 0x0080,
    no_vcl_ssSuper  = 0x0100,
    no_vcl_ssHyper  = 0x0200,
    no_vcl_ssAltGr  = 0x0400,
    no_vcl_ssCaps   = 0x0800,
    no_vcl_ssNum    = 0x1000,
    no_vcl_ssScroll = 0x2000,
    no_vcl_ssTriple = 0x4000,
    no_vcl_ssQuad   = 0x8000,
    no_vcl_ssExtra1 = 0x10000,
    no_vcl_ssExtra2 = 0x20000
};

/* マウスボタン(OnMouseDown / OnMouseUp の button 引数)。 */
enum
{
    no_vcl_mbLeft   = 0,
    no_vcl_mbRight  = 1,
    no_vcl_mbMiddle = 2,
    no_vcl_mbExtra1 = 3,
    no_vcl_mbExtra2 = 4
};

/* OnKeyDown/OnKeyUp 用。*key はキーコード(Windows の仮想キーコード)で、書き換えると LCL に渡る値が変わる。
   0 にすると、そのキー入力を LCL に渡さない(既定の処理をさせない)。 */
typedef void (NO_VCL_CALL *no_vcl_key_callback_t)(no_vcl_obj_t sender, no_vcl_int_t* key, no_vcl_int_t shift, void* data);
/* OnKeyPress 用。*key は文字コードで、書き換え・0 にする効果は上記と同じ。 */
typedef void (NO_VCL_CALL *no_vcl_key_press_callback_t)(no_vcl_obj_t sender, no_vcl_int_t* key, void* data);
/* OnMouseDown/OnMouseUp 用。 */
typedef void (NO_VCL_CALL *no_vcl_mouse_callback_t)(no_vcl_obj_t sender, no_vcl_int_t button, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void* data);
/* OnMouseMove 用。 */
typedef void (NO_VCL_CALL *no_vcl_mouse_move_callback_t)(no_vcl_obj_t sender, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void* data);
/* OnMouseWheel 用。*handled に 0 以外を書き込むと、ホイール操作をこのハンドラで処理済みとして扱う(既定のスクロール等が起きなくなる)。 */
typedef void (NO_VCL_CALL *no_vcl_mouse_wheel_callback_t)(no_vcl_obj_t sender, no_vcl_int_t shift, no_vcl_int_t wheelDelta, no_vcl_int_t x, no_vcl_int_t y, no_vcl_bool_t* handled, void* data);

enum
{
    no_vcl_caNone     = 0,  /* 閉じない */
    no_vcl_caHide     = 1,  /* 隠す(MainForm 以外の既定値) */
    no_vcl_caFree     = 2,  /* 破棄する(MainForm の既定値。MainForm ならアプリケーションを終了する) */
    no_vcl_caMinimize = 3   /* 最小化する */
};

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

/* Align は TAlign の序数(no_vcl_al*)。親のクライアント領域の上下左右に寄せる/残りを埋める配置で、
 * 寄せた方向の Left/Top/Width/Height は LCL が決める(alTop なら Left/Top/Width が親に合わせられ、Height は保たれる)。
 * 既定値はクラスごとに異なる(多くは no_vcl_alNone、TStatusBar は no_vcl_alBottom、TSplitter は no_vcl_alLeft)。 */
enum { no_vcl_alNone = 0, no_vcl_alTop, no_vcl_alBottom, no_vcl_alLeft, no_vcl_alRight, no_vcl_alClient, no_vcl_alCustom };
no_vcl_int_t  NO_VCL_CALL no_vcl_TControl_GetAlign(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetAlign(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* Text は LCL では TControl の protected。公開しているのは TCustomEdit / TCustomComboBox の系統。 */
no_vcl_str_t  NO_VCL_CALL no_vcl_TControl_GetText(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetText(no_vcl_obj_t Obj, no_vcl_str_t Value);

void          NO_VCL_CALL no_vcl_TControl_Show(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_Hide(no_vcl_obj_t Obj);

void          NO_VCL_CALL no_vcl_TControl_SetOnClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TControl_SetOnDblClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TControl_SetOnResize(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TControl_SetOnMouseDown(no_vcl_obj_t Obj, no_vcl_mouse_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TControl_SetOnMouseUp(no_vcl_obj_t Obj, no_vcl_mouse_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TControl_SetOnMouseMove(no_vcl_obj_t Obj, no_vcl_mouse_move_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TControl_SetOnMouseEnter(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TControl_SetOnMouseLeave(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TControl_SetOnMouseWheel(no_vcl_obj_t Obj, no_vcl_mouse_wheel_callback_t Cb, void* Data);

/* TWinControl */
void          NO_VCL_CALL no_vcl_TWinControl_SetOnKeyDown(no_vcl_obj_t Obj, no_vcl_key_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TWinControl_SetOnKeyUp(no_vcl_obj_t Obj, no_vcl_key_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TWinControl_SetOnKeyPress(no_vcl_obj_t Obj, no_vcl_key_press_callback_t Cb, void* Data);

/* TCustomForm / TForm */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TForm_Create(no_vcl_obj_t Owner);
void          NO_VCL_CALL no_vcl_TCustomForm_Show(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomForm_Hide(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomForm_ShowModal(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomForm_Close(no_vcl_obj_t Obj);
/* 保留中のメッセージを処理し終えてから破棄する。フォーム自身やその子のイベントハンドラの中からでも安全に呼べる。 */
void          NO_VCL_CALL no_vcl_TCustomForm_Release(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomForm_SetOnClose(no_vcl_obj_t Obj, no_vcl_close_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomForm_SetOnCloseQuery(no_vcl_obj_t Obj, no_vcl_close_query_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomForm_SetOnShow(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomForm_SetOnHide(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomForm_SetOnActivate(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomForm_SetOnDeactivate(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
/* 破棄の最初に呼ばれる(子コントロールはまだ有効)。DLL の切り離し時の破棄では呼ばれない。 */
void          NO_VCL_CALL no_vcl_TCustomForm_SetOnDestroy(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

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

/* TCustomListBox / TListBox
 * 利用者による選択の変更(マウス・キー操作とも)は no_vcl_TControl_SetOnClick で受け取れる
 * (LCL の ClickOnSelChange が既定で有効なため。VCL と同じ)。プログラムからの ItemIndex の変更では呼ばれない。 */
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

/* docs/component-coverage.md の Tier 1 で挙げたコントロール。 */

/* TScrollBox: TScrollingWinControl の直接の派生で、追加の関数は無い。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TScrollBox_Create(no_vcl_obj_t Owner);

/* TToggleBox: TCustomCheckBox の直接の派生で、Checked は no_vcl_TButtonControl_* を共有する。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TToggleBox_Create(no_vcl_obj_t Owner);

/* TBevel */
enum { no_vcl_bsBox = 0, no_vcl_bsFrame, no_vcl_bsTopLine, no_vcl_bsBottomLine, no_vcl_bsLeftLine, no_vcl_bsRightLine, no_vcl_bsSpacer };
enum { no_vcl_bsLowered = 0, no_vcl_bsRaised };
no_vcl_obj_t  NO_VCL_CALL no_vcl_TBevel_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TBevel_GetShape(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TBevel_SetShape(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TBevel_GetStyle(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TBevel_SetStyle(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* TShape。Pen/Brush は no_vcl_TCanvas_GetPen 等と同じく、コントロールが所有する非所有のハンドルを返す。 */
enum { no_vcl_stRectangle = 0, no_vcl_stSquare, no_vcl_stRoundRect, no_vcl_stRoundSquare,
       no_vcl_stEllipse, no_vcl_stCircle, no_vcl_stSquaredDiamond, no_vcl_stDiamond,
       no_vcl_stTriangle, no_vcl_stTriangleLeft, no_vcl_stTriangleRight, no_vcl_stTriangleDown,
       no_vcl_stStar, no_vcl_stStarDown, no_vcl_stPolygon };
no_vcl_obj_t  NO_VCL_CALL no_vcl_TShape_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomShape_GetShape(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomShape_SetShape(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomShape_GetPen(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomShape_GetBrush(no_vcl_obj_t Obj);

/* TStaticText */
enum { no_vcl_sbsNone = 0, no_vcl_sbsSingle, no_vcl_sbsSunken };
no_vcl_obj_t  NO_VCL_CALL no_vcl_TStaticText_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomStaticText_GetBorderStyle(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomStaticText_SetBorderStyle(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* TStatusBar。Panels(複数区画)は今回未対応で、SimpleText/SimplePanel のみ。
 * 他のコントロールと同じく、Application の Run() より前(フォームの生成中)に生成・配置してよい
 * (LCL の Win32 実装が DLL で失敗する問題は no_vcl_TStatusBar_Create の中で回避済み。
 * docs/adr/0015-... を参照)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TStatusBar_Create(no_vcl_obj_t Owner);
no_vcl_str_t  NO_VCL_CALL no_vcl_TStatusBar_GetSimpleText(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TStatusBar_SetSimpleText(no_vcl_obj_t Obj, no_vcl_str_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TStatusBar_GetSimplePanel(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TStatusBar_SetSimplePanel(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* docs/component-coverage.md の Tier 1、2 バッチ目(範囲・数値系のコントロール)。
 * いずれも ComCtrls のネイティブコントロール(詳細は docs/adr/0015-... を参照)。 */

/* TScrollBar */
enum { no_vcl_sbHorizontal = 0, no_vcl_sbVertical };
no_vcl_obj_t  NO_VCL_CALL no_vcl_TScrollBar_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomScrollBar_GetKind(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomScrollBar_SetKind(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomScrollBar_GetMin(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomScrollBar_SetMin(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomScrollBar_GetMax(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomScrollBar_SetMax(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomScrollBar_GetPosition(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomScrollBar_SetPosition(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomScrollBar_GetPageSize(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomScrollBar_SetPageSize(no_vcl_obj_t Obj, no_vcl_int_t Value);
void          NO_VCL_CALL no_vcl_TCustomScrollBar_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* TTrackBar */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTrackBar_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomTrackBar_GetMin(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTrackBar_SetMin(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomTrackBar_GetMax(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTrackBar_SetMax(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomTrackBar_GetPosition(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTrackBar_SetPosition(no_vcl_obj_t Obj, no_vcl_int_t Value);
void          NO_VCL_CALL no_vcl_TCustomTrackBar_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* TProgressBar (表示専用。イベントは無い) */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TProgressBar_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomProgressBar_GetMin(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomProgressBar_SetMin(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomProgressBar_GetMax(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomProgressBar_SetMax(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomProgressBar_GetPosition(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomProgressBar_SetPosition(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* TUpDown。Min/Max/Position/Increment/Associate は TCustomUpDown では protected だが、
 * 唯一の具象クラス TUpDown が published にしているため、関数名は no_vcl_TUpDown_* にする。
 * Associate は対象の TWinControl(TEdit 等)。OnClick/OnChanging は未対応。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TUpDown_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TUpDown_GetMin(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TUpDown_SetMin(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TUpDown_GetMax(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TUpDown_SetMax(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TUpDown_GetPosition(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TUpDown_SetPosition(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TUpDown_GetIncrement(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TUpDown_SetIncrement(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TUpDown_GetAssociate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TUpDown_SetAssociate(no_vcl_obj_t Obj, no_vcl_obj_t Value);

/* docs/component-coverage.md の Tier 1、4 バッチ目(Items を持つグループ・リスト系のコントロール)。 */

/* TRadioGroup */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TRadioGroup_Create(no_vcl_obj_t Owner);
void          NO_VCL_CALL no_vcl_TCustomRadioGroup_Items_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TCustomRadioGroup_Items_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomRadioGroup_Items_Count(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TCustomRadioGroup_Items_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomRadioGroup_GetItemIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomRadioGroup_SetItemIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* OnClick は TCustomRadioGroup 自身のフィールドで、no_vcl_TControl_SetOnClick とは別物。 */
void          NO_VCL_CALL no_vcl_TCustomRadioGroup_SetOnClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* TCheckGroup */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCheckGroup_Create(no_vcl_obj_t Owner);
void          NO_VCL_CALL no_vcl_TCustomCheckGroup_Items_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TCustomCheckGroup_Items_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomCheckGroup_Items_Count(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TCustomCheckGroup_Items_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomCheckGroup_GetChecked(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TCustomCheckGroup_SetChecked(no_vcl_obj_t Obj, no_vcl_int_t Index, no_vcl_bool_t Value);

/* TCheckListBox。Items は no_vcl_TCustomListBox_Items_* を共有する(TCustomListBox の派生のため)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCheckListBox_Create(no_vcl_obj_t Owner);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomCheckListBox_GetChecked(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TCustomCheckListBox_SetChecked(no_vcl_obj_t Obj, no_vcl_int_t Index, no_vcl_bool_t Value);
void          NO_VCL_CALL no_vcl_TCustomCheckListBox_SetOnClickCheck(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* docs/component-coverage.md の Tier 1、5 バッチ目(ボタンの派生)。Glyph(ビットマップ)は未対応。 */

/* TSpeedButton */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TSpeedButton_Create(no_vcl_obj_t Owner);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomSpeedButton_GetDown(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSpeedButton_SetDown(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSpeedButton_GetGroupIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSpeedButton_SetGroupIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomSpeedButton_GetFlat(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSpeedButton_SetFlat(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomSpeedButton_GetAllowAllUp(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSpeedButton_SetAllowAllUp(no_vcl_obj_t Obj, no_vcl_bool_t Value);

/* TBitBtn。Kind は bkOK/bkCancel 等の定型ボタン(既定の Caption を LCL が設定する)。 */
enum { no_vcl_bkCustom = 0, no_vcl_bkOK, no_vcl_bkCancel, no_vcl_bkHelp, no_vcl_bkYes, no_vcl_bkNo,
       no_vcl_bkClose, no_vcl_bkAbort, no_vcl_bkRetry, no_vcl_bkIgnore, no_vcl_bkAll,
       no_vcl_bkNoToAll, no_vcl_bkYesToAll };
no_vcl_obj_t  NO_VCL_CALL no_vcl_TBitBtn_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomBitBtn_GetKind(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomBitBtn_SetKind(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* docs/component-coverage.md の Tier 1、6 バッチ目(数値・書式付き Edit)。
 * TCustomSpinEdit(整数)は TCustomFloatSpinEdit(実数)の派生で、Value/MinValue/MaxValue/Increment を
 * Integer で再宣言して Double 版を隠す。そのため関数名を宣言元のクラスごとに分けている
 * (no_vcl_TCustomFloatSpinEdit_* は double、no_vcl_TCustomSpinEdit_* は int)。 */

/* TFloatSpinEdit */
no_vcl_obj_t   NO_VCL_CALL no_vcl_TFloatSpinEdit_Create(no_vcl_obj_t Owner);
no_vcl_float_t NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_GetValue(no_vcl_obj_t Obj);
void           NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_SetValue(no_vcl_obj_t Obj, no_vcl_float_t Value);
no_vcl_float_t NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_GetMinValue(no_vcl_obj_t Obj);
void           NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_SetMinValue(no_vcl_obj_t Obj, no_vcl_float_t Value);
no_vcl_float_t NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_GetMaxValue(no_vcl_obj_t Obj);
void           NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_SetMaxValue(no_vcl_obj_t Obj, no_vcl_float_t Value);
no_vcl_float_t NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_GetIncrement(no_vcl_obj_t Obj);
void           NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_SetIncrement(no_vcl_obj_t Obj, no_vcl_float_t Value);
no_vcl_int_t   NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_GetDecimalPlaces(no_vcl_obj_t Obj);
void           NO_VCL_CALL no_vcl_TCustomFloatSpinEdit_SetDecimalPlaces(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* TSpinEdit */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TSpinEdit_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSpinEdit_GetValue(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSpinEdit_SetValue(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSpinEdit_GetMinValue(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSpinEdit_SetMinValue(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSpinEdit_GetMaxValue(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSpinEdit_SetMaxValue(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSpinEdit_GetIncrement(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSpinEdit_SetIncrement(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* TMaskEdit。EditMask は TCustomMaskEdit では protected だが、唯一の具象クラス TMaskEdit が
 * published にしているため、関数名は no_vcl_TMaskEdit_* にする。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TMaskEdit_Create(no_vcl_obj_t Owner);
no_vcl_str_t  NO_VCL_CALL no_vcl_TMaskEdit_GetEditMask(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMaskEdit_SetEditMask(no_vcl_obj_t Obj, no_vcl_str_t Value);

/* docs/component-coverage.md の Tier 1、7 バッチ目(最後のバッチ)。
 * Tabs/TabIndex/OnChange は TCustomTabControl では protected だが、唯一の具象クラス TTabControl が
 * 独自のフィールドで再宣言して published にしているため、関数名は no_vcl_TTabControl_* にする。
 * TPageControl/TTabSheet(所有ページの生成・破棄)は今回見送る。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTabControl_Create(no_vcl_obj_t Owner);
void          NO_VCL_CALL no_vcl_TTabControl_Tabs_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TTabControl_Tabs_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TTabControl_Tabs_Count(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TTabControl_Tabs_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_int_t  NO_VCL_CALL no_vcl_TTabControl_GetTabIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTabControl_SetTabIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
void          NO_VCL_CALL no_vcl_TTabControl_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* TSplitter。同じ Align を持つ直前のコントロール(alLeft なら、自分より左にある alLeft のコントロール)の
 * 幅・高さを、ドラッグで変える。Align が no_vcl_alLeft/alRight なら縦のバー、alTop/alBottom なら横のバーになる。
 * メンバはすべて TCustomSplitter の public。OnCanResize/OnCanOffset は今回未対応。 */
enum { no_vcl_akTop = 0, no_vcl_akLeft, no_vcl_akRight, no_vcl_akBottom };
enum { no_vcl_rsLine = 0, no_vcl_rsNone, no_vcl_rsPattern, no_vcl_rsUpdate };
no_vcl_obj_t  NO_VCL_CALL no_vcl_TSplitter_Create(no_vcl_obj_t Owner);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomSplitter_GetAutoSnap(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSplitter_SetAutoSnap(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomSplitter_GetBeveled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSplitter_SetBeveled(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSplitter_GetMinSize(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSplitter_SetMinSize(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSplitter_GetResizeAnchor(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSplitter_SetResizeAnchor(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSplitter_GetResizeStyle(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSplitter_SetResizeStyle(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* SplitterPosition は LCL ではプロパティではなく GetSplitterPosition/SetSplitterPosition メソッド。
 * 縦のバーなら Left、横のバーなら Top にあたる(親のクライアント座標)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomSplitter_GetSplitterPosition(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomSplitter_SetSplitterPosition(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* マウスでのドラッグが終わったときに呼ばれる(no_vcl_TCustomSplitter_SetSplitterPosition では呼ばれない)。 */
void          NO_VCL_CALL no_vcl_TCustomSplitter_SetOnMoved(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* docs/component-coverage.md の Tier 5(メニュー)。docs/adr/0017-... を参照。
 *
 * TMenuItem・TMainMenu・TPopupMenu は TControl ではない(Parent/Left 等は無い)。項目の親子関係は
 * no_vcl_TMenuItem_Add 等で組み、破棄は Owner(no_vcl_TMenuItem_Create に渡したもの)に任せる。
 * 親の項目が破棄されると、子の項目も(Owner が別でも)一緒に破棄される(LCL の仕様)。
 *
 * コンポーネントを返す関数のうち no_vcl_TMenu_GetItems・no_vcl_TMenuItem_GetItem・no_vcl_TMenuItem_GetParent は、
 * LCL が内部で生成した項目(メニューのルートの Items、AddSeparator で追加した区切り線)を返すことがある。
 * これらは返す時点で破棄通知の対象に登録されるため、*_Create で生成したものと同じく破棄通知が届く。 */

/* ショートカットキー(TShortCut)。VCL と同じく、仮想キーコードに修飾キーのビット(no_vcl_sc*)を OR した値。 */
enum { no_vcl_scShift = 0x2000, no_vcl_scCtrl = 0x4000, no_vcl_scAlt = 0x8000 };
/* key は仮想キーコード、shift は no_vcl_ss* のビット集合(Shift/Ctrl/Alt 以外のビットは無視される)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_ShortCut_Make(no_vcl_int_t Key, no_vcl_int_t Shift);
/* "Ctrl+S" のような文字列との変換。解釈できない文字列は 0 になる。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_ShortCut_FromText(no_vcl_str_t Text);
no_vcl_str_t  NO_VCL_CALL no_vcl_ShortCut_ToText(no_vcl_int_t Value);

/* TMenuItem。Caption に "-" を設定すると区切り線になる。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TMenuItem_Create(no_vcl_obj_t Owner);
no_vcl_str_t  NO_VCL_CALL no_vcl_TMenuItem_GetCaption(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TMenuItem_GetChecked(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetChecked(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TMenuItem_GetEnabled(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetEnabled(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TMenuItem_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 0 以外にすると、選ばれるたびに Checked が反転する(RadioItem なら同じ GroupIndex の他の項目が外れる)。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TMenuItem_GetAutoCheck(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetAutoCheck(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TMenuItem_GetRadioItem(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetRadioItem(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 0〜255。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TMenuItem_GetGroupIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetGroupIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TMenuItem_GetDefault(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetDefault(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TMenuItem_GetShortCut(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetShortCut(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_str_t  NO_VCL_CALL no_vcl_TMenuItem_GetHint(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_SetHint(no_vcl_obj_t Obj, no_vcl_str_t Value);
void          NO_VCL_CALL no_vcl_TMenuItem_SetOnClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
/* 子の項目(LCL の Items[Index] / Count)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TMenuItem_GetCount(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TMenuItem_GetItem(no_vcl_obj_t Obj, no_vcl_int_t Index);
/* 親の項目。メニューの直下の項目なら、そのメニューのルート(no_vcl_TMenu_GetItems)。どこにも追加されていなければ NULL。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TMenuItem_GetParent(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TMenuItem_Add(no_vcl_obj_t Obj, no_vcl_obj_t Item);
void          NO_VCL_CALL no_vcl_TMenuItem_Insert(no_vcl_obj_t Obj, no_vcl_int_t Index, no_vcl_obj_t Item);
/* Delete/Remove は子から外すだけで破棄しない(VCL と同じ)。Clear はすべての子を破棄する。 */
void          NO_VCL_CALL no_vcl_TMenuItem_Delete(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TMenuItem_Remove(no_vcl_obj_t Obj, no_vcl_obj_t Item);
void          NO_VCL_CALL no_vcl_TMenuItem_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TMenuItem_IndexOf(no_vcl_obj_t Obj, no_vcl_obj_t Item);
/* 区切り線を末尾に追加する(項目は LCL が内部で生成する。取得は no_vcl_TMenuItem_GetItem で)。 */
void          NO_VCL_CALL no_vcl_TMenuItem_AddSeparator(no_vcl_obj_t Obj);
no_vcl_bool_t NO_VCL_CALL no_vcl_TMenuItem_IsLine(no_vcl_obj_t Obj);
/* 利用者が項目を選んだときと同じ処理(AutoCheck の反映と OnClick)を行う。 */
void          NO_VCL_CALL no_vcl_TMenuItem_Click(no_vcl_obj_t Obj);

/* TMenu(TMainMenu・TPopupMenu の共通の基底)。Items はメニューのルートの項目で、メニュー自身が所有する。
 * メニューに表示する項目は、このルートに no_vcl_TMenuItem_Add で追加する。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TMenu_GetItems(no_vcl_obj_t Obj);

/* TMainMenu。フォームに表示するには no_vcl_TCustomForm_SetMenu で割り当てる。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TMainMenu_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomForm_GetMenu(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomForm_SetMenu(no_vcl_obj_t Obj, no_vcl_obj_t Menu);

/* TPopupMenu。no_vcl_TControl_SetPopupMenu でコントロールに割り当てると、右クリックで開く(AutoPopup が 0 以外のとき)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPopupMenu_Create(no_vcl_obj_t Owner);
/* X, Y はスクリーン座標。Win32 ではメニューが閉じるまで戻らない。 */
void          NO_VCL_CALL no_vcl_TPopupMenu_Popup(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y);
no_vcl_bool_t NO_VCL_CALL no_vcl_TPopupMenu_GetAutoPopup(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPopupMenu_SetAutoPopup(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 右クリックでメニューを開いたコントロール(OnPopup の中で、どこから開かれたかを知るのに使う)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPopupMenu_GetPopupComponent(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPopupMenu_SetPopupComponent(no_vcl_obj_t Obj, no_vcl_obj_t Value);
/* OnPopup は開く直前、OnClose は閉じた後に呼ばれる。 */
void          NO_VCL_CALL no_vcl_TPopupMenu_SetOnPopup(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TPopupMenu_SetOnClose(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TControl_GetPopupMenu(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TControl_SetPopupMenu(no_vcl_obj_t Obj, no_vcl_obj_t Menu);

/* docs/component-coverage.md の Tier 2、1 バッチ目(TPageControl + TTabSheet)。docs/adr/0018-... を参照。
 *
 * TPageControl は TTabControl と同じ TCustomTabControl の派生で、no_vcl_TCustomTabControl_* は両方に使える。
 * ページ(TTabSheet)は、VCL と同じく no_vcl_TTabSheet_Create で生成して no_vcl_TTabSheet_SetPageControl で追加するか、
 * no_vcl_TPageControl_AddTabSheet で追加する(こちらは LCL が内部で生成し、Owner はページコントロール)。
 * ページを返す関数(GetActivePage・GetPage・AddTabSheet)は、返す時点でそのページを破棄通知の対象に登録する。
 * ページの上のコントロールは、ページを Parent にして置く。 */
enum { no_vcl_tpTop = 0, no_vcl_tpBottom, no_vcl_tpLeft, no_vcl_tpRight };
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPageControl_Create(no_vcl_obj_t Owner);
/* ページが 1 つも無ければ NULL。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPageControl_GetActivePage(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPageControl_SetActivePage(no_vcl_obj_t Obj, no_vcl_obj_t Page);
no_vcl_int_t  NO_VCL_CALL no_vcl_TPageControl_GetActivePageIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPageControl_SetActivePageIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* LCL の Pages[Index]。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPageControl_GetPage(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomTabControl_GetPageCount(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TPageControl_AddTabSheet(no_vcl_obj_t Obj);
/* すべてのページを外して破棄する。破棄は LCL の Application.ReleaseComponent による遅延破棄で、
 * 次にメッセージを処理したとき(または Owner の破棄時)に行われ、そのときに破棄通知が届く。 */
void          NO_VCL_CALL no_vcl_TPageControl_Clear(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPageControl_SelectNextPage(no_vcl_obj_t Obj, no_vcl_bool_t GoForward);
/* TabIndex は表示されているタブの中での位置(TabVisible が 0 のページは数えない)。ページの位置は ActivePageIndex。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TPageControl_GetTabIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TPageControl_SetTabIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* ページが切り替わった後に呼ばれる。プログラムからの ActivePage・ActivePageIndex の変更では呼ばれないが、
 * no_vcl_TCustomPage_SetPageIndex でページを並べ替えたときは呼ばれる。 */
void          NO_VCL_CALL no_vcl_TPageControl_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
/* 利用者の操作でページが切り替わる前に呼ばれる。*allowChange に 0 を書き込むと切り替えを取りやめる
 * (コールバックの形は OnCloseQuery と同じ)。 */
void          NO_VCL_CALL no_vcl_TCustomTabControl_SetOnChanging(no_vcl_obj_t Obj, no_vcl_close_query_callback_t Cb, void* Data);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomTabControl_GetMultiLine(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTabControl_SetMultiLine(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomTabControl_GetShowTabs(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTabControl_SetShowTabs(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomTabControl_GetTabPosition(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTabControl_SetTabPosition(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* TTabSheet(TCustomPage の派生)。タブの文字列は no_vcl_TControl_SetCaption。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTabSheet_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTabSheet_GetPageControl(no_vcl_obj_t Obj);
/* ページコントロールの末尾に追加する(NULL で外す)。 */
void          NO_VCL_CALL no_vcl_TTabSheet_SetPageControl(no_vcl_obj_t Obj, no_vcl_obj_t PageControl);
/* 表示されているタブの中での位置(TabVisible が 0 なら -1)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TTabSheet_GetTabIndex(no_vcl_obj_t Obj);
/* ページの並び順。書き換えるとタブの位置が移動する。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomPage_GetPageIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomPage_SetPageIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomPage_GetTabVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomPage_SetTabVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* ページが表示された/隠されたときに呼ばれる。 */
void          NO_VCL_CALL no_vcl_TCustomPage_SetOnShow(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomPage_SetOnHide(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

#ifdef __cplusplus
}
#endif

#endif
