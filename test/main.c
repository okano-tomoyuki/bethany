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

static void NO_VCL_CALL OnScrollBarChange(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("ScrollBar changed! Position=%d\n", no_vcl_TCustomScrollBar_GetPosition(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnTrackBarChange(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("TrackBar changed! Position=%d\n", no_vcl_TCustomTrackBar_GetPosition(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnRadioGroupClick(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("RadioGroup clicked! ItemIndex=%d\n", no_vcl_TCustomRadioGroup_GetItemIndex(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnCheckListBoxClickCheck(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("CheckListBox check clicked! Checked[0]/[1]/[2]=%d/%d/%d\n",
           no_vcl_TCustomCheckListBox_GetChecked(sender, 0), no_vcl_TCustomCheckListBox_GetChecked(sender, 1),
           no_vcl_TCustomCheckListBox_GetChecked(sender, 2));
    fflush(stdout);
}

static void NO_VCL_CALL OnSpeedButtonClick(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("SpeedButton clicked! Down=%d\n", no_vcl_TCustomSpeedButton_GetDown(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnTabControlChange(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("TabControl changed! TabIndex=%d\n", no_vcl_TTabControl_GetTabIndex(sender));
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

/* Align で配置するコントロール(OnFormShow の data)。 */
typedef struct
{
    no_vcl_obj_t topPanel;
    no_vcl_obj_t leftPanel;
    no_vcl_obj_t splitter;
    no_vcl_obj_t clientPanel;
} AlignedControls;

static void PrintBounds(const char* name, no_vcl_obj_t control, const char* expected)
{
    printf("%s Bounds=(%d,%d,%d,%d) (expected %s)\n", name,
           no_vcl_TControl_GetLeft(control), no_vcl_TControl_GetTop(control),
           no_vcl_TControl_GetWidth(control), no_vcl_TControl_GetHeight(control), expected);
}

static void NO_VCL_CALL OnFormShow(no_vcl_obj_t sender, void* data)
{
    AlignedControls* aligned = (AlignedControls*)data;
    (void)sender;
    printf("Form shown\n");
    /* Align による配置は、LCL ではフォームが表示されるまで行われない(VCL と異なる)。OnShow の時点では済んでいる。
       layoutPanel(280x90)のクライアント領域は、枠(BevelOuter)の 1px 分だけ内側の (1,1)-(279,89)。
       expected は Win32 の値で、Linux/GTK2 ではクライアント領域が右と下に 4px 狭いため、Width/Height がその分小さくなる。 */
    PrintBounds("alTop panel", aligned->topPanel, "1,1,278,20");
    PrintBounds("alLeft panel", aligned->leftPanel, "1,21,80,68");
    PrintBounds("Splitter", aligned->splitter, "81,21,5,68");
    PrintBounds("alClient panel", aligned->clientPanel, "86,21,193,68");
    /* プログラムから Splitter を動かすと、alLeft のパネルの幅と alClient のパネルが追随する。 */
    no_vcl_TCustomSplitter_SetSplitterPosition(aligned->splitter, 121);
    printf("After SetSplitterPosition(121): SplitterPosition=%d, alLeft Width=%d, alClient Left=%d (expected 121/120/126)\n",
           no_vcl_TCustomSplitter_GetSplitterPosition(aligned->splitter),
           no_vcl_TControl_GetWidth(aligned->leftPanel), no_vcl_TControl_GetLeft(aligned->clientPanel));
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

static void NO_VCL_CALL OnMenuItemClick(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("Menu item clicked! Caption=%s\n", no_vcl_TMenuItem_GetCaption(sender));
    fflush(stdout);
}

/* data は Application。 */
static void NO_VCL_CALL OnExitItemClick(no_vcl_obj_t sender, void* data)
{
    (void)sender;
    no_vcl_TApplication_Terminate((no_vcl_obj_t)data);
}

/* data は PopupMenu を割り当てたパネル。 */
static void NO_VCL_CALL OnPopupMenuPopup(no_vcl_obj_t sender, void* data)
{
    printf("PopupMenu popup! PopupComponent is panel: %s\n",
           no_vcl_TPopupMenu_GetPopupComponent(sender) == data ? "yes" : "no");
    fflush(stdout);
}

static void NO_VCL_CALL OnPageControlChange(no_vcl_obj_t sender, void* data)
{
    (void)data;
    printf("PageControl changed! ActivePageIndex=%d\n", no_vcl_TPageControl_GetActivePageIndex(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnPageControlChanging(no_vcl_obj_t sender, no_vcl_bool_t* allowChange, void* data)
{
    (void)sender; (void)data;
    printf("PageControl changing! allowChange=%d\n", *allowChange != 0);
    fflush(stdout);
}

/* ノードの破棄通知。data は破棄されたノードの数。 */
static void NO_VCL_CALL OnTreeNodeFreed(no_vcl_obj_t node, void* data)
{
    (void)node;
    ++*(int*)data;
}

static void NO_VCL_CALL OnTreeViewChange(no_vcl_obj_t sender, no_vcl_obj_t node, void* data)
{
    (void)sender; (void)data;
    printf("TreeView changed! Selected=%s\n", node ? no_vcl_TTreeNode_GetText(node) : "(none)");
    fflush(stdout);
}

/* "Locked" という名前のノードは折りたためないようにする。 */
static void NO_VCL_CALL OnTreeViewCollapsing(no_vcl_obj_t sender, no_vcl_obj_t node, no_vcl_bool_t* allow, void* data)
{
    const char* text = no_vcl_TTreeNode_GetText(node);
    (void)sender; (void)data;
    if (text[0] == 'L')
        *allow = 0;
}

/* data は Splitter が幅を変える alLeft のパネル。 */
static void NO_VCL_CALL OnSplitterMoved(no_vcl_obj_t sender, void* data)
{
    printf("Splitter moved! SplitterPosition=%d, left pane Width=%d\n",
           no_vcl_TCustomSplitter_GetSplitterPosition(sender), no_vcl_TControl_GetWidth((no_vcl_obj_t)data));
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
    no_vcl_obj_t scrollBox;
    no_vcl_obj_t scrolledButton;
    no_vcl_obj_t toggleBox;
    no_vcl_obj_t bevel;
    no_vcl_obj_t shape;
    no_vcl_obj_t staticText;
    no_vcl_obj_t statusBar;
    no_vcl_obj_t scrollBar;
    no_vcl_obj_t trackBar;
    no_vcl_obj_t progressBar;
    no_vcl_obj_t upDownEdit;
    no_vcl_obj_t upDown;
    no_vcl_obj_t radioGroup;
    no_vcl_obj_t checkGroup;
    no_vcl_obj_t checkListBox;
    no_vcl_obj_t speedButton;
    no_vcl_obj_t bitBtn;
    no_vcl_obj_t floatSpinEdit;
    no_vcl_obj_t spinEdit;
    no_vcl_obj_t maskEdit;
    no_vcl_obj_t tabControl;
    no_vcl_obj_t layoutPanel;
    AlignedControls aligned;
    no_vcl_obj_t mainMenu;
    no_vcl_obj_t menuRoot;
    no_vcl_obj_t fileMenu;
    no_vcl_obj_t openItem;
    no_vcl_obj_t exitItem;
    no_vcl_obj_t separator;
    no_vcl_obj_t popupMenu;
    no_vcl_obj_t popupItem;
    no_vcl_obj_t pageControl;
    no_vcl_obj_t tabSheet1;
    no_vcl_obj_t tabSheet2;
    no_vcl_obj_t tempPageControl;
    no_vcl_obj_t treeView;
    no_vcl_obj_t treeItems;
    no_vcl_obj_t rootNode;
    no_vcl_obj_t childNode;
    no_vcl_obj_t root2Node;
    no_vcl_obj_t lockedNode;
    int nodesFreed = 0;

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
    /* aligned の中身は下で Align のコントロールを生成したときに埋める(表示されるのは Run() の後)。 */
    no_vcl_TCustomForm_SetOnShow(form, OnFormShow, &aligned);

    no_vcl_TControl_SetCaption(form, "Hello from FPC DLL");
    no_vcl_TControl_SetWidth(form, 640);
    no_vcl_TControl_SetHeight(form, 930);
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

    /* docs/component-coverage.md の Tier 1 で追加したコントロール(1 バッチ目)。 */
    scrollBox = Place(no_vcl_TScrollBox_Create(form), form, 20, 380);
    no_vcl_TControl_SetWidth(scrollBox, 180);
    no_vcl_TControl_SetHeight(scrollBox, 50);

    scrolledButton = Place(no_vcl_TButton_Create(form), scrollBox, 10, 10);
    no_vcl_TControl_SetCaption(scrolledButton, "Inside ScrollBox");
    no_vcl_TControl_SetWidth(scrolledButton, 140);
    printf("ScrolledButton parent is scrollBox: %s\n", no_vcl_TControl_GetParent(scrolledButton) == scrollBox ? "yes" : "no");

    toggleBox = Place(no_vcl_TToggleBox_Create(form), form, 220, 380);
    no_vcl_TControl_SetCaption(toggleBox, "Toggle me");
    no_vcl_TControl_SetOnClick(toggleBox, OnCheckBoxClick, NULL);

    bevel = Place(no_vcl_TBevel_Create(form), form, 340, 380);
    no_vcl_TControl_SetWidth(bevel, 100);
    no_vcl_TControl_SetHeight(bevel, 50);
    no_vcl_TBevel_SetShape(bevel, no_vcl_bsFrame);
    no_vcl_TBevel_SetStyle(bevel, no_vcl_bsRaised);

    shape = Place(no_vcl_TShape_Create(form), form, 460, 380);
    no_vcl_TControl_SetWidth(shape, 60);
    no_vcl_TControl_SetHeight(shape, 50);
    no_vcl_TCustomShape_SetShape(shape, no_vcl_stEllipse);
    no_vcl_TBrush_SetColor(no_vcl_TCustomShape_GetBrush(shape), CL_YELLOW);
    no_vcl_TPen_SetColor(no_vcl_TCustomShape_GetPen(shape), CL_BLUE);

    staticText = Place(no_vcl_TStaticText_Create(form), form, 20, 440);
    no_vcl_TControl_SetCaption(staticText, "Static text");
    no_vcl_TControl_SetWidth(staticText, 150);
    no_vcl_TCustomStaticText_SetBorderStyle(staticText, no_vcl_sbsSunken);

    /* Tier 1、3 バッチ目(範囲・数値系のコントロール)。 */
    scrollBar = Place(no_vcl_TScrollBar_Create(form), form, 20, 480);
    no_vcl_TControl_SetWidth(scrollBar, 150);
    no_vcl_TCustomScrollBar_SetMin(scrollBar, 0);
    no_vcl_TCustomScrollBar_SetMax(scrollBar, 100);
    no_vcl_TCustomScrollBar_SetPosition(scrollBar, 30);
    no_vcl_TCustomScrollBar_SetOnChange(scrollBar, OnScrollBarChange, NULL);

    trackBar = Place(no_vcl_TTrackBar_Create(form), form, 190, 480);
    no_vcl_TControl_SetWidth(trackBar, 150);
    no_vcl_TCustomTrackBar_SetMin(trackBar, 0);
    no_vcl_TCustomTrackBar_SetMax(trackBar, 10);
    no_vcl_TCustomTrackBar_SetPosition(trackBar, 5);
    no_vcl_TCustomTrackBar_SetOnChange(trackBar, OnTrackBarChange, NULL);

    progressBar = Place(no_vcl_TProgressBar_Create(form), form, 360, 480);
    no_vcl_TControl_SetWidth(progressBar, 150);
    no_vcl_TCustomProgressBar_SetMin(progressBar, 0);
    no_vcl_TCustomProgressBar_SetMax(progressBar, 100);
    no_vcl_TCustomProgressBar_SetPosition(progressBar, 42);
    printf("ScrollBar/TrackBar/ProgressBar Position: %d/%d/%d\n",
           no_vcl_TCustomScrollBar_GetPosition(scrollBar), no_vcl_TCustomTrackBar_GetPosition(trackBar),
           no_vcl_TCustomProgressBar_GetPosition(progressBar));

    upDownEdit = Place(no_vcl_TEdit_Create(form), form, 20, 510);
    no_vcl_TControl_SetText(upDownEdit, "3");
    no_vcl_TControl_SetWidth(upDownEdit, 60);

    upDown = Place(no_vcl_TUpDown_Create(form), form, 80, 510);
    no_vcl_TUpDown_SetMin(upDown, 0);
    no_vcl_TUpDown_SetMax(upDown, 10);
    no_vcl_TUpDown_SetPosition(upDown, 3);
    no_vcl_TUpDown_SetIncrement(upDown, 1);
    no_vcl_TUpDown_SetAssociate(upDown, upDownEdit);
    printf("UpDown Position: %d, Associate is upDownEdit: %s\n",
           no_vcl_TUpDown_GetPosition(upDown), no_vcl_TUpDown_GetAssociate(upDown) == upDownEdit ? "yes" : "no");

    /* Tier 1、4 バッチ目(Items を持つグループ・リスト系のコントロール)。 */
    radioGroup = Place(no_vcl_TRadioGroup_Create(form), form, 20, 550);
    no_vcl_TControl_SetCaption(radioGroup, "RadioGroup1");
    no_vcl_TControl_SetWidth(radioGroup, 180);
    no_vcl_TControl_SetHeight(radioGroup, 90);
    no_vcl_TCustomRadioGroup_Items_Add(radioGroup, "Option A");
    no_vcl_TCustomRadioGroup_Items_Add(radioGroup, "Option B");
    no_vcl_TCustomRadioGroup_Items_Add(radioGroup, "Option C");
    no_vcl_TCustomRadioGroup_SetItemIndex(radioGroup, 1);
    no_vcl_TCustomRadioGroup_SetOnClick(radioGroup, OnRadioGroupClick, NULL);

    checkGroup = Place(no_vcl_TCheckGroup_Create(form), form, 210, 550);
    no_vcl_TControl_SetCaption(checkGroup, "CheckGroup1");
    no_vcl_TControl_SetWidth(checkGroup, 180);
    no_vcl_TControl_SetHeight(checkGroup, 90);
    no_vcl_TCustomCheckGroup_Items_Add(checkGroup, "Feature X");
    no_vcl_TCustomCheckGroup_Items_Add(checkGroup, "Feature Y");
    no_vcl_TCustomCheckGroup_Items_Add(checkGroup, "Feature Z");
    no_vcl_TCustomCheckGroup_SetChecked(checkGroup, 0, 1);
    no_vcl_TCustomCheckGroup_SetChecked(checkGroup, 2, 1);

    checkListBox = Place(no_vcl_TCheckListBox_Create(form), form, 400, 550);
    no_vcl_TControl_SetWidth(checkListBox, 180);
    no_vcl_TControl_SetHeight(checkListBox, 90);
    no_vcl_TCustomListBox_Items_Add(checkListBox, "Item 1");
    no_vcl_TCustomListBox_Items_Add(checkListBox, "Item 2");
    no_vcl_TCustomListBox_Items_Add(checkListBox, "Item 3");
    no_vcl_TCustomCheckListBox_SetChecked(checkListBox, 1, 1);
    no_vcl_TCustomCheckListBox_SetOnClickCheck(checkListBox, OnCheckListBoxClickCheck, NULL);
    printf("RadioGroup ItemIndex=%d, CheckGroup Checked[0]/[1]/[2]=%d/%d/%d, CheckListBox Checked[1]=%d\n",
           no_vcl_TCustomRadioGroup_GetItemIndex(radioGroup),
           no_vcl_TCustomCheckGroup_GetChecked(checkGroup, 0), no_vcl_TCustomCheckGroup_GetChecked(checkGroup, 1),
           no_vcl_TCustomCheckGroup_GetChecked(checkGroup, 2), no_vcl_TCustomCheckListBox_GetChecked(checkListBox, 1));

    /* Tier 1、5 バッチ目(ボタンの派生)。 */
    speedButton = Place(no_vcl_TSpeedButton_Create(form), form, 20, 650);
    no_vcl_TControl_SetCaption(speedButton, "Speed");
    no_vcl_TControl_SetWidth(speedButton, 80);
    no_vcl_TCustomSpeedButton_SetGroupIndex(speedButton, 1);
    no_vcl_TControl_SetOnClick(speedButton, OnSpeedButtonClick, NULL);

    bitBtn = Place(no_vcl_TBitBtn_Create(form), form, 120, 650);
    no_vcl_TCustomBitBtn_SetKind(bitBtn, no_vcl_bkOK);
    printf("BitBtn Kind=%d (expected bkOK=1), Caption=%s\n",
           no_vcl_TCustomBitBtn_GetKind(bitBtn), no_vcl_TControl_GetCaption(bitBtn));

    /* Tier 1、6 バッチ目(数値・書式付き Edit)。 */
    floatSpinEdit = Place(no_vcl_TFloatSpinEdit_Create(form), form, 20, 690);
    no_vcl_TControl_SetWidth(floatSpinEdit, 100);
    no_vcl_TCustomFloatSpinEdit_SetMinValue(floatSpinEdit, 0.0);
    no_vcl_TCustomFloatSpinEdit_SetMaxValue(floatSpinEdit, 10.0);
    no_vcl_TCustomFloatSpinEdit_SetIncrement(floatSpinEdit, 0.5);
    no_vcl_TCustomFloatSpinEdit_SetDecimalPlaces(floatSpinEdit, 1);
    no_vcl_TCustomFloatSpinEdit_SetValue(floatSpinEdit, 2.5);

    spinEdit = Place(no_vcl_TSpinEdit_Create(form), form, 130, 690);
    no_vcl_TControl_SetWidth(spinEdit, 100);
    no_vcl_TCustomSpinEdit_SetMinValue(spinEdit, 0);
    no_vcl_TCustomSpinEdit_SetMaxValue(spinEdit, 100);
    no_vcl_TCustomSpinEdit_SetIncrement(spinEdit, 5);
    no_vcl_TCustomSpinEdit_SetValue(spinEdit, 42);

    maskEdit = Place(no_vcl_TMaskEdit_Create(form), form, 240, 690);
    no_vcl_TControl_SetWidth(maskEdit, 100);
    no_vcl_TMaskEdit_SetEditMask(maskEdit, "000-0000;1;_");
    printf("FloatSpinEdit Value=%.1f, SpinEdit Value=%d, MaskEdit EditMask=%s\n",
           no_vcl_TCustomFloatSpinEdit_GetValue(floatSpinEdit), no_vcl_TCustomSpinEdit_GetValue(spinEdit),
           no_vcl_TMaskEdit_GetEditMask(maskEdit));

    /* Tier 1、7 バッチ目(最後のバッチ)。ページ付きの TPageControl/TTabSheet は今回見送る。 */
    tabControl = Place(no_vcl_TTabControl_Create(form), form, 20, 730);
    no_vcl_TControl_SetWidth(tabControl, 300);
    no_vcl_TControl_SetHeight(tabControl, 90);
    no_vcl_TTabControl_Tabs_Add(tabControl, "Tab A");
    no_vcl_TTabControl_Tabs_Add(tabControl, "Tab B");
    no_vcl_TTabControl_Tabs_Add(tabControl, "Tab C");
    no_vcl_TTabControl_SetTabIndex(tabControl, 0);
    no_vcl_TTabControl_SetOnChange(tabControl, OnTabControlChange, NULL);
    printf("TabControl TabsCount=%d, TabIndex=%d\n",
           no_vcl_TTabControl_Tabs_Count(tabControl), no_vcl_TTabControl_GetTabIndex(tabControl));

    /* TStatusBar も Run() の前に生成してよい(ADR 0015 の問題は DLL 側で回避済み)。 */
    statusBar = no_vcl_TStatusBar_Create(form);
    no_vcl_TControl_SetParent(statusBar, form);
    no_vcl_TStatusBar_SetSimpleText(statusBar, "Ready");
    printf("StatusBar SimpleText: %s\n", no_vcl_TStatusBar_GetSimpleText(statusBar));
    /* TStatusBar の Align の既定値は alBottom(Left/Top を指定しなくてもフォームの下端に付く)。 */
    printf("StatusBar Align=%d (expected alBottom=%d)\n", no_vcl_TControl_GetAlign(statusBar), no_vcl_alBottom);

    /* TControl.Align と TSplitter。layoutPanel の中を、上端の alTop、左の alLeft + Splitter、残りの alClient で分ける。
       alLeft 同士は Left の小さい順に並ぶため、Splitter(既定の Align が alLeft)が alLeft のパネルの右に来るよう、
       Parent より先に Left をパネルの幅より大きくしておく。 */
    layoutPanel = Place(no_vcl_TPanel_Create(form), form, 340, 730);
    no_vcl_TControl_SetWidth(layoutPanel, 280);
    no_vcl_TControl_SetHeight(layoutPanel, 90);
    no_vcl_TControl_SetCaption(layoutPanel, "");
    aligned.topPanel = no_vcl_TPanel_Create(form);
    no_vcl_TControl_SetParent(aligned.topPanel, layoutPanel);
    no_vcl_TControl_SetAlign(aligned.topPanel, no_vcl_alTop);
    no_vcl_TControl_SetHeight(aligned.topPanel, 20);
    no_vcl_TControl_SetCaption(aligned.topPanel, "alTop");
    aligned.leftPanel = no_vcl_TPanel_Create(form);
    no_vcl_TControl_SetParent(aligned.leftPanel, layoutPanel);
    no_vcl_TControl_SetAlign(aligned.leftPanel, no_vcl_alLeft);
    no_vcl_TControl_SetWidth(aligned.leftPanel, 80);
    no_vcl_TControl_SetCaption(aligned.leftPanel, "alLeft");
    aligned.splitter = no_vcl_TSplitter_Create(form);
    no_vcl_TControl_SetLeft(aligned.splitter, 100);
    no_vcl_TControl_SetParent(aligned.splitter, layoutPanel);
    no_vcl_TCustomSplitter_SetMinSize(aligned.splitter, 40);
    no_vcl_TCustomSplitter_SetBeveled(aligned.splitter, 1);
    no_vcl_TCustomSplitter_SetOnMoved(aligned.splitter, OnSplitterMoved, aligned.leftPanel);
    aligned.clientPanel = no_vcl_TPanel_Create(form);
    no_vcl_TControl_SetParent(aligned.clientPanel, layoutPanel);
    no_vcl_TControl_SetAlign(aligned.clientPanel, no_vcl_alClient);
    no_vcl_TControl_SetCaption(aligned.clientPanel, "alClient");
    /* 配置後の位置・大きさは OnFormShow で確認する(表示されるまで Align による配置は行われない)。 */
    printf("alTop/alLeft/alClient panel Align=%d/%d/%d (expected %d/%d/%d)\n",
           no_vcl_TControl_GetAlign(aligned.topPanel), no_vcl_TControl_GetAlign(aligned.leftPanel),
           no_vcl_TControl_GetAlign(aligned.clientPanel), no_vcl_alTop, no_vcl_alLeft, no_vcl_alClient);
    printf("Splitter Align=%d (expected alLeft=%d) MinSize=%d Beveled=%d AutoSnap=%d "
           "ResizeAnchor=%d (expected akLeft=%d) ResizeStyle=%d (expected rsUpdate=%d)\n",
           no_vcl_TControl_GetAlign(aligned.splitter), no_vcl_alLeft,
           no_vcl_TCustomSplitter_GetMinSize(aligned.splitter), no_vcl_TCustomSplitter_GetBeveled(aligned.splitter) != 0,
           no_vcl_TCustomSplitter_GetAutoSnap(aligned.splitter) != 0,
           no_vcl_TCustomSplitter_GetResizeAnchor(aligned.splitter), no_vcl_akLeft,
           no_vcl_TCustomSplitter_GetResizeStyle(aligned.splitter), no_vcl_rsUpdate);

    /* Tier 5(メニュー)。項目の Owner はフォームにし、親子関係は no_vcl_TMenuItem_Add で組む。
       menuRoot(メニューのルート項目)は LCL が内部で生成したもので、取得した時点で破棄通知の対象になる。 */
    mainMenu = no_vcl_TMainMenu_Create(form);
    menuRoot = no_vcl_TMenu_GetItems(mainMenu);
    fileMenu = no_vcl_TMenuItem_Create(form);
    no_vcl_TMenuItem_SetCaption(fileMenu, "&File");
    no_vcl_TMenuItem_Add(menuRoot, fileMenu);
    openItem = no_vcl_TMenuItem_Create(form);
    no_vcl_TMenuItem_SetCaption(openItem, "&Open");
    no_vcl_TMenuItem_SetShortCut(openItem, no_vcl_ShortCut_FromText("Ctrl+O"));
    no_vcl_TMenuItem_SetOnClick(openItem, OnMenuItemClick, NULL);
    no_vcl_TMenuItem_Add(fileMenu, openItem);
    no_vcl_TMenuItem_AddSeparator(fileMenu);
    exitItem = no_vcl_TMenuItem_Create(form);
    no_vcl_TMenuItem_SetCaption(exitItem, "E&xit");
    no_vcl_TMenuItem_SetOnClick(exitItem, OnExitItemClick, app);
    no_vcl_TMenuItem_Add(fileMenu, exitItem);
    no_vcl_TCustomForm_SetMenu(form, mainMenu);

    separator = no_vcl_TMenuItem_GetItem(fileMenu, 1);
    printf("Form Menu is mainMenu: %s, root Count=%d (expected 1), fileMenu Count=%d (expected 3)\n",
           no_vcl_TCustomForm_GetMenu(form) == mainMenu ? "yes" : "no",
           no_vcl_TMenuItem_GetCount(menuRoot), no_vcl_TMenuItem_GetCount(fileMenu));
    printf("fileMenu Parent is root: %s, separator IsLine=%d, separator Parent is fileMenu: %s\n",
           no_vcl_TMenuItem_GetParent(fileMenu) == menuRoot ? "yes" : "no",
           no_vcl_TMenuItem_IsLine(separator) != 0,
           no_vcl_TMenuItem_GetParent(separator) == fileMenu ? "yes" : "no");
    printf("openItem ShortCut=0x%04x (expected Ctrl+O = 0x%04x) Text=%s\n",
           (unsigned)no_vcl_TMenuItem_GetShortCut(openItem),
           (unsigned)no_vcl_ShortCut_Make('O', no_vcl_ssCtrl),
           no_vcl_ShortCut_ToText(no_vcl_TMenuItem_GetShortCut(openItem)));
    no_vcl_TMenuItem_Click(openItem);

    /* panel を右クリックすると開くメニュー。 */
    popupMenu = no_vcl_TPopupMenu_Create(form);
    popupItem = no_vcl_TMenuItem_Create(form);
    no_vcl_TMenuItem_SetCaption(popupItem, "Popup item");
    no_vcl_TMenuItem_SetOnClick(popupItem, OnMenuItemClick, NULL);
    no_vcl_TMenuItem_Add(no_vcl_TMenu_GetItems(popupMenu), popupItem);
    no_vcl_TPopupMenu_SetOnPopup(popupMenu, OnPopupMenuPopup, panel);
    no_vcl_TControl_SetPopupMenu(panel, popupMenu);
    printf("panel PopupMenu is popupMenu: %s, AutoPopup=%d\n",
           no_vcl_TControl_GetPopupMenu(panel) == popupMenu ? "yes" : "no",
           no_vcl_TPopupMenu_GetAutoPopup(popupMenu) != 0);

    /* Tier 2、1 バッチ目(TPageControl + TTabSheet)。tabSheet1 は VCL と同じく生成して PageControl を設定し、
       tabSheet2 は AddTabSheet で追加する(LCL が生成し、Owner は pageControl。返す時点で破棄通知の対象になる)。 */
    pageControl = Place(no_vcl_TPageControl_Create(form), form, 400, 200);
    no_vcl_TControl_SetWidth(pageControl, 220);
    no_vcl_TControl_SetHeight(pageControl, 160);
    tabSheet1 = no_vcl_TTabSheet_Create(form);
    no_vcl_TTabSheet_SetPageControl(tabSheet1, pageControl);
    no_vcl_TControl_SetCaption(tabSheet1, "Page 1");
    no_vcl_TControl_SetCaption(Place(no_vcl_TLabel_Create(form), tabSheet1, 10, 10), "On page 1");
    tabSheet2 = no_vcl_TPageControl_AddTabSheet(pageControl);
    no_vcl_TControl_SetCaption(tabSheet2, "Page 2");
    no_vcl_TControl_SetCaption(Place(no_vcl_TButton_Create(form), tabSheet2, 10, 10), "On page 2");
    no_vcl_TPageControl_SetActivePage(pageControl, tabSheet1);
    no_vcl_TPageControl_SetOnChange(pageControl, OnPageControlChange, NULL);
    no_vcl_TCustomTabControl_SetOnChanging(pageControl, OnPageControlChanging, NULL);
    printf("PageControl PageCount=%d (expected 2), ActivePage is tabSheet1: %s, GetPage(1) is tabSheet2: %s, "
           "tabSheet2 PageControl is pageControl: %s\n",
           no_vcl_TCustomTabControl_GetPageCount(pageControl),
           no_vcl_TPageControl_GetActivePage(pageControl) == tabSheet1 ? "yes" : "no",
           no_vcl_TPageControl_GetPage(pageControl, 1) == tabSheet2 ? "yes" : "no",
           no_vcl_TTabSheet_GetPageControl(tabSheet2) == pageControl ? "yes" : "no");

    /* Clear はすべてのページを外して、遅延破棄する(Application.ReleaseComponent)。
       Clear の時点ではまだ破棄されず、ここでは Owner(tempPageControl)の破棄と一緒に破棄される。 */
    tempPageControl = no_vcl_TPageControl_Create(form);
    no_vcl_TPageControl_AddTabSheet(tempPageControl);
    no_vcl_TPageControl_AddTabSheet(tempPageControl);
    {
        int freedBefore = freedCount;
        no_vcl_TPageControl_Clear(tempPageControl);
        printf("temp PageCount after Clear=%d (expected 0), freed right after Clear=%d (expected 0: deferred)\n",
               no_vcl_TCustomTabControl_GetPageCount(tempPageControl), freedCount - freedBefore);
        no_vcl_TComponent_Destroy(tempPageControl);
        printf("freed after destroying temp=%d (expected 3: 2 pages + temp)\n", freedCount - freedBefore);
    }

    /* Tier 2、2 バッチ目(TTreeView)。tabSheet1 の上に置く。ノードは TComponent ではないため、
       破棄は no_vcl_FreeNotify_SetCallback ではなく no_vcl_TreeNodeFree_SetCallback で通知される。 */
    no_vcl_TreeNodeFree_SetCallback(OnTreeNodeFreed, &nodesFreed);
    treeView = Place(no_vcl_TTreeView_Create(form), tabSheet1, 10, 30);
    no_vcl_TControl_SetWidth(treeView, 190);
    no_vcl_TControl_SetHeight(treeView, 95);
    treeItems = no_vcl_TCustomTreeView_GetItems(treeView);
    rootNode = no_vcl_TTreeNodes_Add(treeItems, NULL, "Root");
    childNode = no_vcl_TTreeNodes_AddChild(treeItems, rootNode, "Child");
    no_vcl_TTreeNodes_AddChild(treeItems, childNode, "Grandchild");
    root2Node = no_vcl_TTreeNodes_Add(treeItems, rootNode, "Root 2");
    no_vcl_TTreeNode_SetExpanded(rootNode, 1);
    no_vcl_TTreeView_SetOnChange(treeView, OnTreeViewChange, NULL);
    no_vcl_TTreeView_SetOnCollapsing(treeView, OnTreeViewCollapsing, NULL);
    printf("TreeView Count=%d (expected 4), root children=%d (expected 1), child Parent is root: %s, "
           "Grandchild Level=%d (expected 2), root next sibling is Root 2: %s, GetItem(3) is Root 2: %s\n",
           no_vcl_TTreeNodes_GetCount(treeItems), no_vcl_TTreeNode_GetCount(rootNode),
           no_vcl_TTreeNode_GetParent(childNode) == rootNode ? "yes" : "no",
           no_vcl_TTreeNode_GetLevel(no_vcl_TTreeNodes_FindNodeWithText(treeItems, "Grandchild")),
           no_vcl_TTreeNode_GetNextSibling(rootNode) == root2Node ? "yes" : "no",
           no_vcl_TTreeNodes_GetItem(treeItems, 3) == root2Node ? "yes" : "no");
    printf("node TreeView is treeView: %s\n", no_vcl_TTreeNode_GetTreeView(childNode) == treeView ? "yes" : "no");

    lockedNode = no_vcl_TTreeNodes_Add(treeItems, NULL, "Locked");
    no_vcl_TTreeNodes_AddChild(treeItems, lockedNode, "Inside");
    no_vcl_TTreeNode_Expand(lockedNode, 0);
    no_vcl_TTreeNode_Collapse(lockedNode, 0);
    printf("Locked Expanded after Collapse=%d (expected 1: OnCollapsing refused)\n",
           no_vcl_TTreeNode_GetExpanded(lockedNode) != 0);
    no_vcl_TTreeNode_Delete(lockedNode);
    printf("After Delete: Count=%d (expected 4), nodes freed=%d (expected 2)\n",
           no_vcl_TTreeNodes_GetCount(treeItems), nodesFreed);

    printf("Running (click the button, then close the window twice: the first close is blocked)...\n");
    fflush(stdout);
    /* MainForm を表示してメッセージループに入り、MainForm が閉じられると戻る。 */
    no_vcl_TApplication_Run(app);
    printf("Run returned. Terminated=%d\n", no_vcl_TApplication_GetTerminated(app) != 0);

    /* Application が所有するフォーム(と、フォームが所有するコントロール)をまとめて破棄する。
       呼ばなくても DLL の切り離し時に LCL が破棄するが、そのときは破棄通知が呼ばれない。 */
    no_vcl_TComponent_DestroyComponents(app);
    printf("Clicks: %d, Freed components: %d (expected 60: form + 53 owned + 6 created inside LCL: 2 menu roots, a separator and 3 AddTabSheet pages)\n", clickCount, freedCount);
    /* ツリービューの破棄に伴って、残りのノード(4 つ)も破棄通知が届く。 */
    printf("Tree nodes freed: %d (expected 6: 2 deleted + 4 with the tree view)\n", nodesFreed);

    printf("OK\n");
    return 0;
}
