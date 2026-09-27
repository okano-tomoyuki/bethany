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
/* ビット集合のうち、32 ビットすべてを使うもの(グリッドの Options)。 */
typedef unsigned int no_vcl_uint_t;
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

/* docs/component-coverage.md の Tier 2、2 バッチ目(TTreeView)。docs/adr/0019-... を参照。
 *
 * ノード(TTreeNode)とノードの一覧(TTreeNodes)は TComponent ではない。
 * - TTreeNodes(no_vcl_TCustomTreeView_GetItems)はツリービューが所有する非所有のハンドルで、ツリービューと寿命が一致する。
 * - ノードは no_vcl_TTreeNodes_Add 等で追加し、no_vcl_TTreeNode_Delete 等で削除する(破棄は LCL が行う)。
 *   ノードが削除されると(ツリービューの破棄に伴う削除も含め)、OnDeletion などの削除の処理がすべて終わった後に
 *   項目の破棄通知が呼ばれる。 */

/*
 * 項目の破棄通知: TComponent ではない項目(ツリービューのノード・リストビューの項目と列・ヘッダーコントロールのセクション・
 * クールバーのバンド)が破棄されると、登録したコールバックが破棄される項目を引数に呼ばれる(no_vcl_FreeNotify_SetCallback の項目版)。
 * - 対象は、一度でもこの API から返された(関数の戻り値・イベントの引数として渡された)項目。返されたことの無い項目には
 *   C 側のラッパーが無いため通知しない。
 * - 通知は項目の破棄の最後(LCL の削除の処理とイベントがすべて終わった後)に、どの経路で破棄されても(Delete・Clear・
 *   親のコントロールの破棄・LCL の内部の処理)必ず届く(docs/adr/0026)。
 * コールバックは 1 つだけ登録でき、C++ ラッパー(no_vcl.hpp)を使う場合はラッパーが登録するため上書きしないこと。
 */
void          NO_VCL_CALL no_vcl_ItemFree_SetCallback(no_vcl_callback_t Cb, void* Data);

/* 項目を 1 つ受け取るイベント用(ツリービューの OnChange/OnExpanded/OnCollapsed/OnDeletion、
 * リストビューの OnDeletion/OnItemChecked/OnColumnClick)。item はイベントの対象(ツリービューの OnChange では NULL もありうる)。 */
typedef void (NO_VCL_CALL *no_vcl_item_callback_t)(no_vcl_obj_t sender, no_vcl_obj_t item, void* data);
/* ツリービューの OnChanging/OnExpanding/OnCollapsing 用。*allow は 0 以外が入った状態で呼ばれ、0 を書き込むと取りやめる。 */
typedef void (NO_VCL_CALL *no_vcl_item_allow_callback_t)(no_vcl_obj_t sender, no_vcl_obj_t item, no_vcl_bool_t* allow, void* data);
/* 項目と整数を 1 つずつ受け取るイベント用(リストビューの OnSelectItem では選択されたか、OnChange では変更の種類 no_vcl_ct*)。 */
typedef void (NO_VCL_CALL *no_vcl_item_int_callback_t)(no_vcl_obj_t sender, no_vcl_obj_t item, no_vcl_int_t value, void* data);

no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeView_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomTreeView_GetItems(no_vcl_obj_t Obj);
/* 選択されているノード(無ければ NULL)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomTreeView_GetSelected(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTreeView_SetSelected(no_vcl_obj_t Obj, no_vcl_obj_t Node);
void          NO_VCL_CALL no_vcl_TCustomTreeView_FullExpand(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomTreeView_FullCollapse(no_vcl_obj_t Obj);
/* ノードを文字列の順に並べ替える。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomTreeView_AlphaSort(no_vcl_obj_t Obj);
/* X, Y はツリービューのクライアント座標。そこにノードが無ければ NULL。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomTreeView_GetNodeAt(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y);
/* 以下は LCL では TCustomTreeView の protected で、TTreeView が published にしている。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeView_GetReadOnly(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeView_SetReadOnly(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeView_GetShowLines(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeView_SetShowLines(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeView_GetShowRoot(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeView_SetShowRoot(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeView_GetShowButtons(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeView_SetShowButtons(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeView_GetAutoExpand(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeView_SetAutoExpand(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeView_GetHideSelection(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeView_SetHideSelection(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeView_GetRowSelect(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeView_SetRowSelect(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 選択が変わった後。 */
void          NO_VCL_CALL no_vcl_TTreeView_SetOnChange(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TTreeView_SetOnExpanded(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TTreeView_SetOnCollapsed(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
/* ノードが削除される直前(ノードはまだ有効)。 */
void          NO_VCL_CALL no_vcl_TTreeView_SetOnDeletion(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
/* 選択が変わる前。node は新しく選択されるノード。 */
void          NO_VCL_CALL no_vcl_TTreeView_SetOnChanging(no_vcl_obj_t Obj, no_vcl_item_allow_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TTreeView_SetOnExpanding(no_vcl_obj_t Obj, no_vcl_item_allow_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TTreeView_SetOnCollapsing(no_vcl_obj_t Obj, no_vcl_item_allow_callback_t Cb, void* Data);

/* TTreeNodes。Sibling/Parent に NULL を渡すと最上位のノードになる(LCL と同じ)。追加したノードを返す。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNodes_Add(no_vcl_obj_t Obj, no_vcl_obj_t Sibling, no_vcl_str_t Text);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNodes_AddFirst(no_vcl_obj_t Obj, no_vcl_obj_t Sibling, no_vcl_str_t Text);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNodes_AddChild(no_vcl_obj_t Obj, no_vcl_obj_t Parent, no_vcl_str_t Text);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNodes_AddChildFirst(no_vcl_obj_t Obj, no_vcl_obj_t Parent, no_vcl_str_t Text);
/* NextNode の前に挿入する。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNodes_Insert(no_vcl_obj_t Obj, no_vcl_obj_t NextNode, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TTreeNodes_Clear(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeNodes_Delete(no_vcl_obj_t Obj, no_vcl_obj_t Node);
/* すべてのノード(子孫を含む)の数。GetItem の Index は、上から順に数えた位置(AbsoluteIndex)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TTreeNodes_GetCount(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNodes_GetItem(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNodes_GetFirstNode(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNodes_FindNodeWithText(no_vcl_obj_t Obj, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TTreeNodes_BeginUpdate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeNodes_EndUpdate(no_vcl_obj_t Obj);

/* TTreeNode。ノードを返す関数は、該当するノードが無ければ NULL を返す。 */
enum { no_vcl_naAdd = 0, no_vcl_naAddFirst, no_vcl_naAddChild, no_vcl_naAddChildFirst, no_vcl_naInsert, no_vcl_naInsertBehind };
no_vcl_str_t  NO_VCL_CALL no_vcl_TTreeNode_GetText(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeNode_SetText(no_vcl_obj_t Obj, no_vcl_str_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeNode_GetExpanded(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeNode_SetExpanded(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeNode_GetSelected(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeNode_SetSelected(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TTreeNode_GetHasChildren(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeNode_SetHasChildren(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 利用者データ(LCL は解釈しない)。 */
void*         NO_VCL_CALL no_vcl_TTreeNode_GetData(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeNode_SetData(no_vcl_obj_t Obj, void* Value);
/* 直下の子の数と、Index 番目の子(LCL の Items[Index])。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TTreeNode_GetCount(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetItem(no_vcl_obj_t Obj, no_vcl_int_t Index);
/* 兄弟の中での位置・深さ(最上位が 0)・上から順に数えた位置。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TTreeNode_GetIndex(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TTreeNode_GetLevel(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TTreeNode_GetAbsoluteIndex(no_vcl_obj_t Obj);
/* 最上位のノードなら NULL。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetParent(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetTreeView(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetFirstChild(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetLastChild(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetNextSibling(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetPrevSibling(no_vcl_obj_t Obj);
/* 上から順(子孫を含む)の次/前のノード。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetNext(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TTreeNode_GetPrev(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TTreeNode_IndexOf(no_vcl_obj_t Obj, no_vcl_obj_t Node);
void          NO_VCL_CALL no_vcl_TTreeNode_Expand(no_vcl_obj_t Obj, no_vcl_bool_t Recurse);
void          NO_VCL_CALL no_vcl_TTreeNode_Collapse(no_vcl_obj_t Obj, no_vcl_bool_t Recurse);
/* このノード(と子孫)を削除する。削除されたノードごとに OnDeletion とノードの破棄通知が呼ばれる。 */
void          NO_VCL_CALL no_vcl_TTreeNode_Delete(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TTreeNode_DeleteChildren(no_vcl_obj_t Obj);
/* 祖先を展開し、ノードが見えるようにスクロールする。 */
void          NO_VCL_CALL no_vcl_TTreeNode_MakeVisible(no_vcl_obj_t Obj);
/* Mode は no_vcl_na*。 */
void          NO_VCL_CALL no_vcl_TTreeNode_MoveTo(no_vcl_obj_t Obj, no_vcl_obj_t Destination, no_vcl_int_t Mode);

/* docs/component-coverage.md の Tier 2、3 バッチ目(TListView)。docs/adr/0020-... を参照。
 *
 * 項目(TListItem)・項目の一覧(TListItems)・列(TListColumn)・列の一覧(TListColumns)は TComponent ではない。
 * - TListItems(no_vcl_TCustomListView_GetItems)・TListColumns(no_vcl_TListView_GetColumns)はリストビューが所有する
 *   非所有のハンドルで、リストビューと寿命が一致する。
 * - 項目・列が削除されると(リストビューの破棄に伴う削除も含め)、OnDeletion などの削除の処理の後に項目の破棄通知が呼ばれる。
 *   リストビューの破棄では、リストビュー自身の破棄通知(no_vcl_FreeNotify_SetCallback)の後に項目が破棄される(LCL の順序)。
 *   そのときの OnDeletion の sender は、破棄通知を受け取った後のリストビューになる点に注意。 */
enum { no_vcl_vsIcon = 0, no_vcl_vsSmallIcon, no_vcl_vsList, no_vcl_vsReport };
enum { no_vcl_stNone = 0, no_vcl_stData, no_vcl_stText, no_vcl_stBoth };
enum { no_vcl_sdAscending = 0, no_vcl_sdDescending };
enum { no_vcl_taLeftJustify = 0, no_vcl_taRightJustify, no_vcl_taCenter };
/* OnChange の変更の種類(TItemChange)。 */
enum { no_vcl_ctText = 0, no_vcl_ctImage, no_vcl_ctState };

no_vcl_obj_t  NO_VCL_CALL no_vcl_TListView_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomListView_GetItems(no_vcl_obj_t Obj);
/* 選択されている項目(MultiSelect なら最初の 1 つ。無ければ NULL)と、その位置(無ければ -1)。
 * 表示前(ウィンドウハンドルが無いとき)に設定しても選択される(LCL 単体では選択されないため DLL 側で補っている)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomListView_GetSelected(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_SetSelected(no_vcl_obj_t Obj, no_vcl_obj_t Item);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomListView_GetItemIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_SetItemIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomListView_GetSelCount(no_vcl_obj_t Obj);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomListView_GetCheckboxes(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_SetCheckboxes(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomListView_GetGridLines(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_SetGridLines(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomListView_GetMultiSelect(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_SetMultiSelect(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomListView_GetReadOnly(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_SetReadOnly(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomListView_GetRowSelect(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_SetRowSelect(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* すべての項目を削除する(列は残る)。 */
void          NO_VCL_CALL no_vcl_TCustomListView_Clear(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_BeginUpdate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_EndUpdate(no_vcl_obj_t Obj);
/* X, Y はクライアント座標。そこに項目が無ければ NULL。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomListView_GetItemAt(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y);
void          NO_VCL_CALL no_vcl_TCustomListView_ClearSelection(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomListView_SelectAll(no_vcl_obj_t Obj);
/* 以下は LCL では TCustomListView の protected で、TListView が published にしている。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListView_GetColumns(no_vcl_obj_t Obj);
/* 列見出しと SubItems が表示されるのは no_vcl_vsReport のとき。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TListView_GetViewStyle(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListView_SetViewStyle(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TListView_GetHideSelection(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListView_SetHideSelection(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 並べ替え。SortType が no_vcl_stText のとき、SortColumn の列(0 が Caption の列)の文字列の順に並ぶ。
 * SortColumn が既定の -1 のままでは並べ替えない(LCL の仕様。先に SortColumn を設定する)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TListView_GetSortType(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListView_SetSortType(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TListView_GetSortColumn(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListView_SetSortColumn(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TListView_GetSortDirection(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListView_SetSortDirection(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* 項目の選択状態が変わったとき(value は選択されたなら 0 以外)。 */
void          NO_VCL_CALL no_vcl_TListView_SetOnSelectItem(no_vcl_obj_t Obj, no_vcl_item_int_callback_t Cb, void* Data);
/* 項目が変わったとき(value は no_vcl_ct*)。 */
void          NO_VCL_CALL no_vcl_TListView_SetOnChange(no_vcl_obj_t Obj, no_vcl_item_int_callback_t Cb, void* Data);
/* 項目が削除される直前(項目はまだ有効)。 */
void          NO_VCL_CALL no_vcl_TListView_SetOnDeletion(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
/* チェックボックス(Checkboxes)が切り替わったとき。 */
void          NO_VCL_CALL no_vcl_TListView_SetOnItemChecked(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
/* 列見出しがクリックされたとき(item は列)。 */
void          NO_VCL_CALL no_vcl_TListView_SetOnColumnClick(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);

/* TListItems。Add は末尾に、Insert は Index の位置に空の項目を追加して返す(Caption 等はその後で設定する)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListItems_Add(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListItems_Insert(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TListItems_Delete(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TListItems_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TListItems_GetCount(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListItems_GetItem(no_vcl_obj_t Obj, no_vcl_int_t Index);
no_vcl_int_t  NO_VCL_CALL no_vcl_TListItems_IndexOf(no_vcl_obj_t Obj, no_vcl_obj_t Item);
/* StartIndex の次(Inclusive なら StartIndex から)から Caption を探す。Partial なら前方一致、Wrap なら末尾から先頭へ続けて探す。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListItems_FindCaption(no_vcl_obj_t Obj, no_vcl_int_t StartIndex, no_vcl_str_t Value,
                                                        no_vcl_bool_t Partial, no_vcl_bool_t Inclusive, no_vcl_bool_t Wrap);
void          NO_VCL_CALL no_vcl_TListItems_Exchange(no_vcl_obj_t Obj, no_vcl_int_t Index1, no_vcl_int_t Index2);
void          NO_VCL_CALL no_vcl_TListItems_Move(no_vcl_obj_t Obj, no_vcl_int_t FromIndex, no_vcl_int_t ToIndex);
void          NO_VCL_CALL no_vcl_TListItems_BeginUpdate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListItems_EndUpdate(no_vcl_obj_t Obj);

/* TListItem。Caption は 1 列目、SubItems は 2 列目以降の文字列(ViewStyle が no_vcl_vsReport のときに表示される)。 */
no_vcl_str_t  NO_VCL_CALL no_vcl_TListItem_GetCaption(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListItem_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TListItem_GetChecked(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListItem_SetChecked(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TListItem_GetSelected(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListItem_SetSelected(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TListItem_GetFocused(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListItem_SetFocused(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 利用者データ(LCL は解釈しない)。 */
void*         NO_VCL_CALL no_vcl_TListItem_GetData(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListItem_SetData(no_vcl_obj_t Obj, void* Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TListItem_GetIndex(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListItem_GetListView(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListItem_SubItems_Add(no_vcl_obj_t Obj, no_vcl_str_t Text);
void          NO_VCL_CALL no_vcl_TListItem_SubItems_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TListItem_SubItems_Count(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TListItem_SubItems_GetText(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TListItem_SubItems_SetText(no_vcl_obj_t Obj, no_vcl_int_t Index, no_vcl_str_t Text);
/* この項目を削除する。OnDeletion と項目の破棄通知が呼ばれる。 */
void          NO_VCL_CALL no_vcl_TListItem_Delete(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListItem_MakeVisible(no_vcl_obj_t Obj, no_vcl_bool_t PartialOK);

/* TListColumns / TListColumn。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListColumns_Add(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TListColumns_GetCount(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TListColumns_GetItem(no_vcl_obj_t Obj, no_vcl_int_t Index);
/* 列を削除する(項目の破棄通知が呼ばれる)。 */
void          NO_VCL_CALL no_vcl_TListColumns_Delete(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TListColumns_Clear(no_vcl_obj_t Obj);
no_vcl_str_t  NO_VCL_CALL no_vcl_TListColumn_GetCaption(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListColumn_SetCaption(no_vcl_obj_t Obj, no_vcl_str_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TListColumn_GetWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListColumn_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* no_vcl_ta*。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TListColumn_GetAlignment(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListColumn_SetAlignment(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TListColumn_GetAutoSize(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListColumn_SetAutoSize(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TListColumn_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListColumn_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 列の並び順。書き換えると列が移動する。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TListColumn_GetIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TListColumn_SetIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);

/* docs/component-coverage.md の Tier 2、4 バッチ目(TDrawGrid・TStringGrid)。docs/adr/0021-... を参照。
 *
 * セルは(列, 行)の位置で指定する(0 始まり。固定行・固定列を含む)。主なメンバは LCL の TCustomGrid の protected を
 * TCustomDrawGrid が public にしているため no_vcl_TCustomDrawGrid_* で、TDrawGrid・TStringGrid の両方に使える。 */

/* Options(TGridOptions)のビット。複数のビットを OR して渡す/受け取る(ビットの位置は LCL の TGridOption の序数)。 */
#define no_vcl_goFixedVertLine               0x00000001u
#define no_vcl_goFixedHorzLine               0x00000002u
#define no_vcl_goVertLine                    0x00000004u
#define no_vcl_goHorzLine                    0x00000008u
#define no_vcl_goRangeSelect                 0x00000010u
#define no_vcl_goDrawFocusSelected           0x00000020u
#define no_vcl_goRowSizing                   0x00000040u
#define no_vcl_goColSizing                   0x00000080u
#define no_vcl_goRowMoving                   0x00000100u
#define no_vcl_goColMoving                   0x00000200u
#define no_vcl_goEditing                     0x00000400u
#define no_vcl_goAutoAddRows                 0x00000800u
#define no_vcl_goTabs                        0x00001000u
#define no_vcl_goRowSelect                   0x00002000u
#define no_vcl_goAlwaysShowEditor            0x00004000u
#define no_vcl_goThumbTracking               0x00008000u
#define no_vcl_goColSpanning                 0x00010000u
#define no_vcl_goRelaxedRowSelect            0x00020000u
#define no_vcl_goDblClickAutoSize            0x00040000u
#define no_vcl_goSmoothScroll                0x00080000u
#define no_vcl_goFixedRowNumbering           0x00100000u
#define no_vcl_goScrollKeepVisible           0x00200000u
#define no_vcl_goHeaderHotTracking           0x00400000u
#define no_vcl_goHeaderPushedLook            0x00800000u
#define no_vcl_goSelectionActive             0x01000000u
#define no_vcl_goFixedColSizing              0x02000000u
#define no_vcl_goDontScrollPartCell          0x04000000u
#define no_vcl_goCellHints                   0x08000000u
#define no_vcl_goTruncCellHints              0x10000000u
#define no_vcl_goCellEllipsis                0x20000000u
#define no_vcl_goAutoAddRowsSkipContentCheck 0x40000000u
#define no_vcl_goRowHighlight                0x80000000u

/* OnDrawCell の state(TGridDrawState)のビット。 */
enum
{
    no_vcl_gdSelected     = 0x01,
    no_vcl_gdFocused      = 0x02,
    no_vcl_gdFixed        = 0x04,
    no_vcl_gdHot          = 0x08,
    no_vcl_gdPushed       = 0x10,
    no_vcl_gdRowHighlight = 0x20
};

/* OnSelection 用(col, row は選択されたセル)。 */
typedef void (NO_VCL_CALL *no_vcl_cell_callback_t)(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row, void* data);
/* OnHeaderClick 用。isColumn は列見出し(固定行)なら 0 以外、行見出し(固定列)なら 0。index はその列・行。 */
typedef void (NO_VCL_CALL *no_vcl_header_callback_t)(no_vcl_obj_t sender, no_vcl_int_t isColumn, no_vcl_int_t index, void* data);
/* OnSelectCell 用。*canSelect は 0 以外が入った状態で呼ばれ、0 を書き込むとそのセルを選択させない。 */
typedef void (NO_VCL_CALL *no_vcl_cell_allow_callback_t)(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row, no_vcl_bool_t* canSelect, void* data);
/* OnDrawCell 用。left..bottom はセルのクライアント座標での矩形、state は no_vcl_gd* のビット集合。
 * 描画は no_vcl_TCustomDrawGrid_GetCanvas の Canvas に行う。 */
typedef void (NO_VCL_CALL *no_vcl_draw_cell_callback_t)(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row,
                                                         no_vcl_int_t left, no_vcl_int_t top, no_vcl_int_t right, no_vcl_int_t bottom,
                                                         no_vcl_uint_t state, void* data);

no_vcl_obj_t  NO_VCL_CALL no_vcl_TDrawGrid_Create(no_vcl_obj_t Owner);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TStringGrid_Create(no_vcl_obj_t Owner);

/* TCustomGrid の public。 */
void          NO_VCL_CALL no_vcl_TCustomGrid_BeginUpdate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomGrid_EndUpdate(no_vcl_obj_t Obj);
/* すべての行・列を削除する(ColCount・RowCount が 0 になる)。セルの文字列だけを消すのは no_vcl_TCustomStringGrid_Clean。 */
void          NO_VCL_CALL no_vcl_TCustomGrid_Clear(no_vcl_obj_t Obj);
/* セルのクライアント座標での矩形。 */
void          NO_VCL_CALL no_vcl_TCustomGrid_CellRect(no_vcl_obj_t Obj, no_vcl_int_t Col, no_vcl_int_t Row,
                                                      no_vcl_int_t* Left, no_vcl_int_t* Top, no_vcl_int_t* Right, no_vcl_int_t* Bottom);
/* クライアント座標 X, Y にあるセル。セルの外なら -1。 */
void          NO_VCL_CALL no_vcl_TCustomGrid_MouseToCell(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y, no_vcl_int_t* Col, no_vcl_int_t* Row);

/* TCustomDrawGrid の public。Canvas は no_vcl_TCanvas_* で描画できる非所有のハンドル(グリッドと寿命が一致する)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetCanvas(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetColCount(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetColCount(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetRowCount(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetRowCount(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* 固定列・固定行(見出し)の数。既定は 1。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetFixedCols(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetFixedCols(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetFixedRows(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetFixedRows(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* 現在のセル(フォーカスのあるセル)の列・行。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetCol(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetCol(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetRow(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetRow(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetDefaultColWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetDefaultColWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetDefaultRowHeight(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetDefaultRowHeight(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* LCL の ColWidths[Col] / RowHeights[Row]。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetColWidths(no_vcl_obj_t Obj, no_vcl_int_t Col);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetColWidths(no_vcl_obj_t Obj, no_vcl_int_t Col, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetRowHeights(no_vcl_obj_t Obj, no_vcl_int_t Row);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetRowHeights(no_vcl_obj_t Obj, no_vcl_int_t Row, no_vcl_int_t Value);
/* no_vcl_go* のビット集合。 */
no_vcl_uint_t NO_VCL_CALL no_vcl_TCustomDrawGrid_GetOptions(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetOptions(no_vcl_obj_t Obj, no_vcl_uint_t Value);
/* 選択範囲(Left/Right が列、Top/Bottom が行。単一のセルなら Left = Right、Top = Bottom)。 */
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_GetSelection(no_vcl_obj_t Obj, no_vcl_int_t* Left, no_vcl_int_t* Top, no_vcl_int_t* Right, no_vcl_int_t* Bottom);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetSelection(no_vcl_obj_t Obj, no_vcl_int_t Left, no_vcl_int_t Top, no_vcl_int_t Right, no_vcl_int_t Bottom);
/* スクロール位置(表示されている最初の列・行)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetLeftCol(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetLeftCol(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetTopRow(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetTopRow(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* 0 にすると、OnDrawCell の前にセルの既定の描画(背景・文字列)を行わない。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomDrawGrid_GetDefaultDrawing(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetDefaultDrawing(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomDrawGrid_GetFixedColor(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetFixedColor(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* セルの編集中か(no_vcl_goEditing のとき)。0 以外を設定すると現在のセルの編集を始める。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomDrawGrid_GetEditorMode(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetEditorMode(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* IsColumn が 0 以外なら列、0 なら行を対象にする。 */
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_InsertColRow(no_vcl_obj_t Obj, no_vcl_bool_t IsColumn, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_DeleteColRow(no_vcl_obj_t Obj, no_vcl_bool_t IsColumn, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_MoveColRow(no_vcl_obj_t Obj, no_vcl_bool_t IsColumn, no_vcl_int_t FromIndex, no_vcl_int_t ToIndex);
/* IsColumn が 0 以外なら、列 Index の値で行を並べ替える(固定行は除く)。0 なら行 Index の値で列を並べ替える。 */
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SortColRow(no_vcl_obj_t Obj, no_vcl_bool_t IsColumn, no_vcl_int_t Index);
/* セルを描画するとき(DefaultDrawing が 0 以外なら既定の描画の後)。 */
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetOnDrawCell(no_vcl_obj_t Obj, no_vcl_draw_cell_callback_t Cb, void* Data);
/* セルが選択される前。 */
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetOnSelectCell(no_vcl_obj_t Obj, no_vcl_cell_allow_callback_t Cb, void* Data);
/* セルが選択された後。 */
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetOnSelection(no_vcl_obj_t Obj, no_vcl_cell_callback_t Cb, void* Data);
/* 見出し(固定行・固定列)がクリックされたとき。 */
void          NO_VCL_CALL no_vcl_TCustomDrawGrid_SetOnHeaderClick(no_vcl_obj_t Obj, no_vcl_header_callback_t Cb, void* Data);

/* TCustomStringGrid の public。Cells[Col, Row] はセルの文字列。 */
no_vcl_str_t  NO_VCL_CALL no_vcl_TCustomStringGrid_GetCells(no_vcl_obj_t Obj, no_vcl_int_t Col, no_vcl_int_t Row);
void          NO_VCL_CALL no_vcl_TCustomStringGrid_SetCells(no_vcl_obj_t Obj, no_vcl_int_t Col, no_vcl_int_t Row, no_vcl_str_t Value);
/* すべてのセルの文字列を消す(行・列の数は変わらない)。 */
void          NO_VCL_CALL no_vcl_TCustomStringGrid_Clean(no_vcl_obj_t Obj);
/* 列の幅を文字列に合わせる。 */
void          NO_VCL_CALL no_vcl_TCustomStringGrid_AutoSizeColumns(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomStringGrid_AutoSizeColumn(no_vcl_obj_t Obj, no_vcl_int_t Col);

/* ---------------- THeaderControl(Tier 2、5 バッチ目。docs/adr/0024) ----------------
 * セクション(THeaderSection)は TComponent ではない項目で、ハンドルはヘッダーコントロールが所有する。
 * 破棄の通知(no_vcl_ItemFree_SetCallback)は、セクションが破棄されるとき(THeaderSections_Delete・Clear、
 * ヘッダーコントロールの破棄)に呼ばれる。 */

/* TSectionTrackState(OnSectionTrack の State)。 */
enum { no_vcl_tsTrackBegin = 0, no_vcl_tsTrackMove, no_vcl_tsTrackEnd };

/* OnSectionTrack 用(ドラッグで幅を変えている間)。width は新しい幅、state は no_vcl_ts*。 */
typedef void (NO_VCL_CALL *no_vcl_section_track_callback_t)(no_vcl_obj_t sender, no_vcl_obj_t section,
                                                             no_vcl_int_t width, no_vcl_int_t state, void* data);
/* OnSectionDrag 用(DragReorder のときにセクションをドラッグで移動する)。*allow に 0 を書き込むと移動させない。 */
typedef void (NO_VCL_CALL *no_vcl_section_drag_callback_t)(no_vcl_obj_t sender, no_vcl_obj_t fromSection,
                                                            no_vcl_obj_t toSection, no_vcl_bool_t* allow, void* data);

no_vcl_obj_t  NO_VCL_CALL no_vcl_THeaderControl_Create(no_vcl_obj_t Owner);

/* TCustomHeaderControl の public/published。Sections は no_vcl_THeaderSections_* で操作する非所有のハンドル。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomHeaderControl_GetSections(no_vcl_obj_t Obj);
/* 0 以外にすると、セクションをドラッグで並べ替えられる。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomHeaderControl_GetDragReorder(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomHeaderControl_SetDragReorder(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* クライアント座標 X, Y にあるセクションの位置。無ければ -1。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomHeaderControl_GetSectionAt(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y);
/* 並べ替えても変わらない位置(OriginalIndex)でセクションを探す。無ければ NULL。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomHeaderControl_GetSectionFromOriginalIndex(no_vcl_obj_t Obj, no_vcl_int_t OriginalIndex);
/* sender はヘッダーコントロール、item はセクション。 */
void          NO_VCL_CALL no_vcl_TCustomHeaderControl_SetOnSectionClick(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomHeaderControl_SetOnSectionResize(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomHeaderControl_SetOnSectionSeparatorDblClick(no_vcl_obj_t Obj, no_vcl_item_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomHeaderControl_SetOnSectionTrack(no_vcl_obj_t Obj, no_vcl_section_track_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomHeaderControl_SetOnSectionDrag(no_vcl_obj_t Obj, no_vcl_section_drag_callback_t Cb, void* Data);
void          NO_VCL_CALL no_vcl_TCustomHeaderControl_SetOnSectionEndDrag(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* THeaderSections。Add・Insert は空のセクションを追加して返す(Text 等はその後で設定する)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_THeaderSections_Add(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_THeaderSections_Insert(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_THeaderSections_Delete(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_THeaderSections_Clear(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSections_GetCount(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_THeaderSections_GetItem(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_THeaderSections_BeginUpdate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_THeaderSections_EndUpdate(no_vcl_obj_t Obj);

/* THeaderSection。 */
no_vcl_str_t  NO_VCL_CALL no_vcl_THeaderSection_GetText(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_THeaderSection_SetText(no_vcl_obj_t Obj, no_vcl_str_t Value);
/* Visible が 0 なら 0 を返す。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSection_GetWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_THeaderSection_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSection_GetMinWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_THeaderSection_SetMinWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSection_GetMaxWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_THeaderSection_SetMaxWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* no_vcl_ta*。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSection_GetAlignment(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_THeaderSection_SetAlignment(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_THeaderSection_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_THeaderSection_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 並び順。書き換えるとセクションが移動する。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSection_GetIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_THeaderSection_SetIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* クライアント座標での左端・右端(前のセクションの幅の合計から求める)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSection_GetLeft(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSection_GetRight(no_vcl_obj_t Obj);
/* 並べ替えても変わらない位置。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_THeaderSection_GetOriginalIndex(no_vcl_obj_t Obj);

/* ---------------- TToolBar / TToolButton(Tier 2、6 バッチ目。docs/adr/0025) ----------------
 * TToolButton は他のコントロールと同じく Owner を持つコンポーネント。ツールバーに置くには、Parent をツールバーにする。 */

/* TEdgeBorders(no_vcl_TToolWindow_GetEdgeBorders 等)のビット。 */
#define no_vcl_ebLeft   0x01u
#define no_vcl_ebTop    0x02u
#define no_vcl_ebRight  0x04u
#define no_vcl_ebBottom 0x08u
/* TEdgeStyle。 */
enum { no_vcl_esNone = 0, no_vcl_esRaised, no_vcl_esLowered };
/* TToolButtonStyle。 */
enum { no_vcl_tbsButton = 0, no_vcl_tbsCheck, no_vcl_tbsDropDown, no_vcl_tbsSeparator, no_vcl_tbsDivider, no_vcl_tbsButtonDrop };

/* TToolWindow の public(縁の描画と、更新の一時停止)。 */
no_vcl_uint_t NO_VCL_CALL no_vcl_TToolWindow_GetEdgeBorders(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolWindow_SetEdgeBorders(no_vcl_obj_t Obj, no_vcl_uint_t Value);
/* no_vcl_es*。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolWindow_GetEdgeInner(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolWindow_SetEdgeInner(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolWindow_GetEdgeOuter(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolWindow_SetEdgeOuter(no_vcl_obj_t Obj, no_vcl_int_t Value);
void          NO_VCL_CALL no_vcl_TToolWindow_BeginUpdate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolWindow_EndUpdate(no_vcl_obj_t Obj);

/* TToolBar。既定の Align は alTop。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TToolBar_Create(no_vcl_obj_t Owner);
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolBar_GetButtonCount(no_vcl_obj_t Obj);
/* LCL の Buttons[Index](並び順)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TToolBar_GetButton(no_vcl_obj_t Obj, no_vcl_int_t Index);
/* LCL では行数ではなく、Wrapable が 0 のときに Wrap のボタンで折り返した回数(折り返しが無ければ 0)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolBar_GetRowCount(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolBar_GetButtonHeight(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetButtonHeight(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolBar_GetButtonWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetButtonWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* tbsDropDown のボタンの矢印部分の幅。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolBar_GetDropDownWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetDropDownWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* 最初のボタンの左の余白。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolBar_GetIndent(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetIndent(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolBar_GetFlat(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetFlat(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 0 以外なら、ボタンの文字をアイコンの右に置く。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolBar_GetList(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetList(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 0 以外なら、ボタンに Caption を表示する(既定は 0)。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolBar_GetShowCaptions(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetShowCaptions(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolBar_GetTransparent(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetTransparent(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 0 以外なら、幅に収まらないボタンを次の行へ折り返す(既定は 0 以外)。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolBar_GetWrapable(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolBar_SetWrapable(no_vcl_obj_t Obj, no_vcl_bool_t Value);
void          NO_VCL_CALL no_vcl_TToolBar_SetButtonSize(no_vcl_obj_t Obj, no_vcl_int_t NewButtonWidth, no_vcl_int_t NewButtonHeight);

/* TToolButton。Caption・OnClick は no_vcl_TControl_* を使う。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TToolButton_Create(no_vcl_obj_t Owner);
/* tbsCheck で Grouped のとき、すべてのボタンを上げた状態にできるか。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolButton_GetAllowAllUp(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetAllowAllUp(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 押された状態(tbsCheck はクリックで切り替わる)。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolButton_GetDown(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetDown(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 隣り合う Grouped の tbsCheck のボタンは、どれか 1 つだけが Down になる。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolButton_GetGrouped(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetGrouped(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolButton_GetIndeterminate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetIndeterminate(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolButton_GetMarked(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetMarked(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolButton_GetShowCaption(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetShowCaption(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 0 以外なら、このボタンの後で行を折り返す。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolButton_GetWrap(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetWrap(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* no_vcl_tbs*。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolButton_GetStyle(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetStyle(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* tbsDropDown・tbsButtonDrop の矢印で表示するポップアップメニュー。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TToolButton_GetDropdownMenu(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetDropdownMenu(no_vcl_obj_t Obj, no_vcl_obj_t Value);
/* 設定すると、そのメニュー項目の Caption・Enabled 等を写す。マウスで押すと、その項目の OnClick を呼んでから
 * 子の項目をポップアップメニューとして表示する(メニューを閉じるまで戻らない)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TToolButton_GetMenuItem(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_SetMenuItem(no_vcl_obj_t Obj, no_vcl_obj_t Value);
/* ツールバーの中での位置(ツールバーに置かれていなければ -1)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TToolButton_GetIndex(no_vcl_obj_t Obj);
/* OnClick を呼ぶ(tbsCheck の Down は変えない)。ArrowClick は OnArrowClick を呼ぶ(DropdownMenu は表示しない)。 */
void          NO_VCL_CALL no_vcl_TToolButton_Click(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TToolButton_ArrowClick(no_vcl_obj_t Obj);
/* ボタンのクライアント座標 X, Y が矢印の部分にあるか。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TToolButton_PointInArrow(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y);
/* tbsDropDown・tbsButtonDrop の矢印がクリックされたとき(DropdownMenu を表示する前)。 */
void          NO_VCL_CALL no_vcl_TToolButton_SetOnArrowClick(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* ---------------- TCoolBar(Tier 2、7 バッチ目。docs/adr/0026) ----------------
 * バンド(TCoolBand)は TComponent ではない項目で、ハンドルはクールバーが所有する。
 * ウィンドウを持つコントロールの Parent をクールバーにすると、LCL がそのコントロールのバンドを自動で追加し、
 * コントロールを外す(Parent を変える・破棄する)とそのバンドを削除する。
 * 破棄の通知(no_vcl_ItemFree_SetCallback)は、他の項目と同じく、LCL の内部で削除されたときにも呼ばれる。 */

/* TGrabStyle。 */
enum { no_vcl_gsSimple = 0, no_vcl_gsDouble, no_vcl_gsHorLines, no_vcl_gsVerLines, no_vcl_gsGripper, no_vcl_gsButton };

no_vcl_obj_t  NO_VCL_CALL no_vcl_TCoolBar_Create(no_vcl_obj_t Owner);

/* TCustomCoolBar の public。Bands は no_vcl_TCoolBands_* で操作する非所有のハンドル。
 * Align は no_vcl_TControl_SetAlign で設定する(alLeft/alRight にすると Vertical も 0 以外になる)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCustomCoolBar_GetBands(no_vcl_obj_t Obj);
/* すべてのバンドの幅を、置いているコントロールに合わせる。 */
void          NO_VCL_CALL no_vcl_TCustomCoolBar_AutosizeBands(no_vcl_obj_t Obj);
/* クライアント座標 X, Y にあるバンドの表示上の位置(*Band。無ければ負)と、つまみの上か(*Grabber)。 */
void          NO_VCL_CALL no_vcl_TCustomCoolBar_MouseToBandPos(no_vcl_obj_t Obj, no_vcl_int_t X, no_vcl_int_t Y,
                                                             no_vcl_int_t* Band, no_vcl_bool_t* Grabber);
/* 0 以外なら、ドラッグでバンドの幅を変えられない。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomCoolBar_GetFixedSize(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetFixedSize(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 0 以外なら、ドラッグでバンドを並べ替えられない。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomCoolBar_GetFixedOrder(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetFixedOrder(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* no_vcl_gs*(バンドの左端のつまみの描き方)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomCoolBar_GetGrabStyle(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetGrabStyle(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomCoolBar_GetGrabWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetGrabWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomCoolBar_GetHorizontalSpacing(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetHorizontalSpacing(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCustomCoolBar_GetVerticalSpacing(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetVerticalSpacing(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* 0 以外なら、バンドの Text を表示する(既定は 0 以外)。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomCoolBar_GetShowText(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetShowText(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomCoolBar_GetThemed(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetThemed(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCustomCoolBar_GetVertical(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetVertical(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* ドラッグでバンドを動かす・幅を変えて、マウスを離したとき。 */
void          NO_VCL_CALL no_vcl_TCustomCoolBar_SetOnChange(no_vcl_obj_t Obj, no_vcl_callback_t Cb, void* Data);

/* TCoolBands。Add は空のバンドを追加して返す(Text・Control 等はその後で設定する)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCoolBands_Add(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBands_GetCount(no_vcl_obj_t Obj);
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCoolBands_GetItem(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TCoolBands_Delete(no_vcl_obj_t Obj, no_vcl_int_t Index);
void          NO_VCL_CALL no_vcl_TCoolBands_Clear(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBands_BeginUpdate(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBands_EndUpdate(no_vcl_obj_t Obj);
/* Control がそのコントロールのバンド(無ければ NULL・-1)。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCoolBands_FindBand(no_vcl_obj_t Obj, no_vcl_obj_t Control);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBands_FindBandIndex(no_vcl_obj_t Obj, no_vcl_obj_t Control);

/* TCoolBand。 */
no_vcl_str_t  NO_VCL_CALL no_vcl_TCoolBand_GetText(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetText(no_vcl_obj_t Obj, no_vcl_str_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetMinWidth(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetMinWidth(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetMinHeight(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetMinHeight(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* 0 以外なら、このバンドから新しい行を始める(既定は 0 以外)。 */
no_vcl_bool_t NO_VCL_CALL no_vcl_TCoolBand_GetBreak(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetBreak(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCoolBand_GetVisible(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetVisible(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCoolBand_GetFixedSize(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetFixedSize(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCoolBand_GetFixedBackground(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetFixedBackground(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCoolBand_GetHorizontalOnly(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetHorizontalOnly(no_vcl_obj_t Obj, no_vcl_bool_t Value);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetColor(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetColor(no_vcl_obj_t Obj, no_vcl_int_t Value);
no_vcl_bool_t NO_VCL_CALL no_vcl_TCoolBand_GetParentColor(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetParentColor(no_vcl_obj_t Obj, no_vcl_bool_t Value);
/* 並び順。書き換えるとバンドが移動する。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetIndex(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetIndex(no_vcl_obj_t Obj, no_vcl_int_t Value);
/* バンドに置くコントロール。設定するとそのコントロールの Parent がクールバーになり、Align は alNone になる。 */
no_vcl_obj_t  NO_VCL_CALL no_vcl_TCoolBand_GetControl(no_vcl_obj_t Obj);
void          NO_VCL_CALL no_vcl_TCoolBand_SetControl(no_vcl_obj_t Obj, no_vcl_obj_t Value);
/* クールバーのクライアント座標での位置(配置の計算の後で決まる)。 */
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetLeft(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetTop(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetRight(no_vcl_obj_t Obj);
no_vcl_int_t  NO_VCL_CALL no_vcl_TCoolBand_GetHeight(no_vcl_obj_t Obj);
/* 幅を、置いているコントロールに合わせる。 */
void          NO_VCL_CALL no_vcl_TCoolBand_AutosizeWidth(no_vcl_obj_t Obj);

#ifdef __cplusplus
}
#endif

#endif
