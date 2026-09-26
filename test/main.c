#include <stdio.h>

#include "no_vcl_c.h"

/* コールバックの data には、登録時に渡したポインタがそのまま返ってくる。
   ここではカウンタやラベルのハンドルを渡し、グローバル変数を使わずに状態を持ち回る。 */

static void NO_VCL_CALL OnComponentFreed(no_vcl_obj_t obj, void* data)
{
    (void)obj;
    ++*(int*)data;
}

static void NO_VCL_CALL OnButtonClick(no_vcl_obj_t sender, void* data)
{
    int* clickCount = (int*)data;
    (void)sender;
    ++*clickCount;
    printf("Button clicked! (count=%d)\n", *clickCount);
    fflush(stdout);
}

static void NO_VCL_CALL OnButtonMouseDown(no_vcl_obj_t sender, no_vcl_int_t button, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void* data)
{
    (void)sender; (void)data;
    printf("Button mouse down! button=%d shift=0x%x pos=(%d,%d)\n", (int)button, (unsigned)shift, (int)x, (int)y);
    fflush(stdout);
}

static void NO_VCL_CALL OnEditKeyDown(no_vcl_obj_t sender, no_vcl_int_t* key, no_vcl_int_t shift, void* data)
{
    (void)sender; (void)data;
    printf("Edit key down! key=%d shift=0x%x\n", (int)*key, (unsigned)shift);
    fflush(stdout);
}

static void NO_VCL_CALL OnCheckBoxClick(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("CheckBox clicked! Checked=%d\n", no_vcl_TButtonControl_GetChecked(sender));
    fflush(stdout);
}

/* 2つのラジオボタンで共有し、sender でどちらが押されたかを区別する。 */
static void NO_VCL_CALL OnRadioButtonClick(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("RadioButton clicked! %s Checked=%d\n",
           no_vcl_TControl_GetCaption(sender), no_vcl_TButtonControl_GetChecked(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnEditChange(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("Edit changed! Text=%s\n", no_vcl_TControl_GetText(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnComboBoxChange(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("ComboBox changed! ItemIndex=%d Text=%s\n",
           no_vcl_TCustomComboBox_GetItemIndex(sender), no_vcl_TControl_GetText(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnListBoxClick(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("ListBox clicked! ItemIndex=%d\n", no_vcl_TCustomListBox_GetItemIndex(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnMemoChange(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("Memo changed! LineCount=%d\n", no_vcl_TCustomMemo_Lines_Count(sender));
    fflush(stdout);
}

/* *action には既定の動作が入っている。1 回目は閉じるのを取りやめ、2 回目は既定の動作のままにする。 */
static void NO_VCL_CALL OnFormClose(no_vcl_obj_t sender, no_vcl_int_t* action, void* data)
{
    int* attempts = (int*)data;
    (void)sender;
    ++*attempts;
    printf("Form closing: attempt=%d, default action=%d\n", *attempts, *action);
    if (*attempts == 1)
    {
        *action = no_vcl_caNone;
        printf("Form closing: blocked (no_vcl_caNone)\n");
    }
    fflush(stdout);
}

static void NO_VCL_CALL OnFormCloseQuery(no_vcl_obj_t sender, no_vcl_bool_t* canClose, void* data)
{
    (void)sender;
    (void)data;
    printf("Form close query: default canClose=%d\n", *canClose != 0);
    fflush(stdout);
}

/* 破棄の最初に呼ばれる。子コントロールはまだ有効なので、data で渡したボタンの Caption を読める。 */
static void NO_VCL_CALL OnFormDestroy(no_vcl_obj_t sender, void* data)
{
    (void)sender;
    printf("Form destroying: button caption=%s\n", no_vcl_TControl_GetCaption((no_vcl_obj_t)data));
    fflush(stdout);
}

static void NO_VCL_CALL OnFormShow(no_vcl_obj_t sender, void* data)
{
    (void)sender;
    (void)data;
    printf("Form shown\n");
    fflush(stdout);
}

/* TColorはDelphi/LCLの$00BBGGRR順パック整数 */
#define CL_BLACK  0x000000
#define CL_WHITE  0xFFFFFF
#define CL_RED    0x0000FF
#define CL_GREEN  0x008000
#define CL_BLUE   0xFF0000
#define CL_YELLOW 0x00FFFF

static void NO_VCL_CALL OnPaintBoxPaint(no_vcl_obj_t sender, void* data)
{
    no_vcl_obj_t canvas = no_vcl_TPaintBox_GetCanvas(sender);
    no_vcl_obj_t pen = no_vcl_TCanvas_GetPen(canvas);
    no_vcl_obj_t brush = no_vcl_TCanvas_GetBrush(canvas);
    no_vcl_obj_t font = no_vcl_TCanvas_GetFont(canvas);
    (void)data;

    no_vcl_TPen_SetColor(pen, CL_RED);
    no_vcl_TPen_SetWidth(pen, 2);
    no_vcl_TBrush_SetColor(brush, CL_YELLOW);
    no_vcl_TCanvas_Rectangle(canvas, 10, 10, 110, 70);

    no_vcl_TPen_SetColor(pen, CL_BLUE);
    no_vcl_TBrush_SetColor(brush, CL_WHITE);
    no_vcl_TCanvas_Ellipse(canvas, 120, 10, 200, 70);

    no_vcl_TPen_SetColor(pen, CL_BLACK);
    no_vcl_TCanvas_MoveTo(canvas, 10, 90);
    no_vcl_TCanvas_LineTo(canvas, 200, 90);

    no_vcl_TFont_SetColor(font, CL_GREEN);
    no_vcl_TFont_SetSize(font, 14);
    no_vcl_TCanvas_TextOut(canvas, 10, 100, "Canvas drawing test");
}

/* data にはカウントを表示するラベルのハンドルを渡す。 */
static void NO_VCL_CALL OnTimerTick(no_vcl_obj_t sender, void* data)
{
    static int tickCount = 0;
    char buf[64];
    (void)sender;
    ++tickCount;
    snprintf(buf, sizeof(buf), "Tick: %d", tickCount);
    no_vcl_TControl_SetCaption((no_vcl_obj_t)data, buf);
    printf("Timer tick! count=%d\n", tickCount);
    fflush(stdout);
}

/* コントロールを生成し、親と位置を設定する(Owner と Parent はどちらも form)。 */
static no_vcl_obj_t Place(no_vcl_obj_t control, no_vcl_obj_t parent, int left, int top)
{
    no_vcl_TControl_SetParent(control, parent);
    no_vcl_TControl_SetLeft(control, left);
    no_vcl_TControl_SetTop(control, top);
    return control;
}

int main(void)
{
    int freedCount = 0;
    int clickCount = 0;
    int closeAttempts = 0;
    no_vcl_obj_t app;
    no_vcl_obj_t form;
    no_vcl_obj_t button;
    no_vcl_obj_t label;
    no_vcl_obj_t edit;
    no_vcl_obj_t checkBox;
    no_vcl_obj_t radio1;
    no_vcl_obj_t radio2;
    no_vcl_obj_t panel;
    no_vcl_obj_t groupBox;
    no_vcl_obj_t comboBox;
    no_vcl_obj_t listBox;
    no_vcl_obj_t memo;
    no_vcl_obj_t noHandler;
    no_vcl_obj_t tickLabel;
    no_vcl_obj_t timer;
    no_vcl_obj_t paintBox;

    no_vcl_FreeNotify_SetCallback(OnComponentFreed, &freedCount);

    app = no_vcl_GetApplication();
    no_vcl_TApplication_SetTitle(app, "no_vcl C test");
    printf("Title: %s\n", no_vcl_TApplication_GetTitle(app));

    /* 最初に CreateForm で生成したフォームが MainForm になる(Owner は Application)。 */
    form = no_vcl_TApplication_CreateForm(app);
    if (!form)
    {
        printf("TApplication_CreateForm failed\n");
        return 1;
    }
    printf("MainForm is form: %s\n", no_vcl_TApplication_GetMainForm(app) == form ? "yes" : "no");
    no_vcl_TCustomForm_SetOnClose(form, OnFormClose, &closeAttempts);
    no_vcl_TCustomForm_SetOnShow(form, OnFormShow, NULL);

    no_vcl_TControl_SetCaption(form, "Hello from FPC DLL");
    no_vcl_TControl_SetWidth(form, 640);
    no_vcl_TControl_SetHeight(form, 420);
    printf("Caption: %s\n", no_vcl_TControl_GetCaption(form));

    button = Place(no_vcl_TButton_Create(form), form, 20, 20);
    no_vcl_TControl_SetCaption(button, "Click me");
    no_vcl_TControl_SetWidth(button, 100);
    no_vcl_TControl_SetHeight(button, 30);
    no_vcl_TControl_SetOnClick(button, OnButtonClick, &clickCount);
    no_vcl_TControl_SetOnMouseDown(button, OnButtonMouseDown, NULL);
    no_vcl_TCustomForm_SetOnCloseQuery(form, OnFormCloseQuery, NULL);
    no_vcl_TCustomForm_SetOnDestroy(form, OnFormDestroy, button);
    printf("Button caption: %s\n", no_vcl_TControl_GetCaption(button));
    printf("Button parent is form: %s\n", no_vcl_TControl_GetParent(button) == form ? "yes" : "no");

    label = Place(no_vcl_TLabel_Create(form), form, 20, 60);
    no_vcl_TControl_SetCaption(label, "Label text");

    edit = Place(no_vcl_TEdit_Create(form), form, 20, 90);
    no_vcl_TControl_SetText(edit, "Edit me");
    no_vcl_TControl_SetWidth(edit, 150);
    no_vcl_TCustomEdit_SetOnChange(edit, OnEditChange, NULL);
    no_vcl_TWinControl_SetOnKeyDown(edit, OnEditKeyDown, NULL);

    checkBox = Place(no_vcl_TCheckBox_Create(form), form, 20, 130);
    no_vcl_TControl_SetCaption(checkBox, "Check me");
    /* 登録し直すと最後に登録したものだけが呼ばれる(ブリッジは再利用され、蓄積しない)。 */
    no_vcl_TControl_SetOnClick(checkBox, OnButtonClick, &clickCount);
    no_vcl_TControl_SetOnClick(checkBox, OnCheckBoxClick, NULL);

    radio1 = Place(no_vcl_TRadioButton_Create(form), form, 20, 160);
    no_vcl_TControl_SetCaption(radio1, "Option A");
    no_vcl_TButtonControl_SetChecked(radio1, 1);
    no_vcl_TControl_SetOnClick(radio1, OnRadioButtonClick, NULL);

    radio2 = Place(no_vcl_TRadioButton_Create(form), form, 20, 190);
    no_vcl_TControl_SetCaption(radio2, "Option B");
    no_vcl_TControl_SetOnClick(radio2, OnRadioButtonClick, NULL);

    panel = Place(no_vcl_TPanel_Create(form), form, 220, 20);
    no_vcl_TControl_SetCaption(panel, "");
    no_vcl_TControl_SetWidth(panel, 180);
    no_vcl_TControl_SetHeight(panel, 60);

    groupBox = Place(no_vcl_TGroupBox_Create(form), form, 220, 90);
    no_vcl_TControl_SetCaption(groupBox, "Group");
    no_vcl_TControl_SetWidth(groupBox, 180);
    no_vcl_TControl_SetHeight(groupBox, 60);

    comboBox = Place(no_vcl_TComboBox_Create(form), form, 220, 160);
    no_vcl_TCustomComboBox_Items_Add(comboBox, "Combo A");
    no_vcl_TCustomComboBox_Items_Add(comboBox, "Combo B");
    no_vcl_TCustomComboBox_Items_Add(comboBox, "Combo C");
    no_vcl_TCustomComboBox_SetItemIndex(comboBox, 0);
    no_vcl_TControl_SetWidth(comboBox, 150);
    no_vcl_TComboBox_SetOnChange(comboBox, OnComboBoxChange, NULL);

    listBox = Place(no_vcl_TListBox_Create(form), form, 220, 190);
    no_vcl_TCustomListBox_Items_Add(listBox, "List 1");
    no_vcl_TCustomListBox_Items_Add(listBox, "List 2");
    no_vcl_TCustomListBox_Items_Add(listBox, "List 3");
    no_vcl_TControl_SetWidth(listBox, 150);
    no_vcl_TControl_SetHeight(listBox, 80);
    /* 利用者による選択の変更(マウス・キー操作とも)で呼ばれる。プログラムからの ItemIndex の変更では呼ばれない。 */
    no_vcl_TControl_SetOnClick(listBox, OnListBoxClick, NULL);
    no_vcl_TCustomListBox_SetItemIndex(listBox, 1);

    memo = Place(no_vcl_TMemo_Create(form), form, 220, 280);
    no_vcl_TCustomMemo_Lines_Add(memo, "Memo line 1");
    no_vcl_TCustomMemo_Lines_Add(memo, "Memo line 2");
    no_vcl_TControl_SetWidth(memo, 150);
    no_vcl_TControl_SetHeight(memo, 80);
    no_vcl_TCustomEdit_SetOnChange(memo, OnMemoChange, NULL);

    /* NULL を登録するとハンドラが解除され、押しても何も呼ばれない。 */
    noHandler = Place(no_vcl_TButton_Create(form), form, 20, 280);
    no_vcl_TControl_SetCaption(noHandler, "No handler");
    no_vcl_TControl_SetOnClick(noHandler, OnButtonClick, &clickCount);
    no_vcl_TControl_SetOnClick(noHandler, NULL, NULL);

    tickLabel = Place(no_vcl_TLabel_Create(form), form, 20, 230);
    no_vcl_TControl_SetCaption(tickLabel, "Tick: 0");

    timer = no_vcl_TTimer_Create(form);
    no_vcl_TCustomTimer_SetInterval(timer, 500);
    no_vcl_TCustomTimer_SetOnTimer(timer, OnTimerTick, tickLabel);
    no_vcl_TCustomTimer_SetEnabled(timer, 1);

    paintBox = Place(no_vcl_TPaintBox_Create(form), form, 400, 20);
    no_vcl_TControl_SetWidth(paintBox, 220);
    no_vcl_TControl_SetHeight(paintBox, 130);
    no_vcl_TPaintBox_SetOnPaint(paintBox, OnPaintBoxPaint, NULL);

    printf("Running (click the button, then close the window twice: the first close is blocked)...\n");
    fflush(stdout);
    /* MainForm を表示してメッセージループに入り、MainForm が閉じられると戻る。 */
    no_vcl_TApplication_Run(app);
    printf("Run returned. Terminated=%d\n", no_vcl_TApplication_GetTerminated(app) != 0);

    /* Application が所有するフォーム(と、フォームが所有するコントロール)をまとめて破棄する。
       呼ばなくても DLL の切り離し時に LCL が破棄するが、そのときは破棄通知が呼ばれない。 */
    no_vcl_TComponent_DestroyComponents(app);
    printf("Clicks: %d, Freed components: %d (expected 16: form + 15 owned)\n", clickCount, freedCount);

    printf("OK\n");
    return 0;
}
