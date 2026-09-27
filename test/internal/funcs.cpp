// DLL の関数を内部層(no_vcl::internal)から直接呼ぶテスト(docs/adr/0032)。no_vcl.hpp のクラスは使わず、
// 定数と Exception だけを no_vcl.hpp から使う。C++ ラッパーのテストは test/main.cpp。
#include <stdio.h>

#include "no_vcl.hpp"

using namespace no_vcl;
using namespace no_vcl::internal;

/* コールバックの data には、登録時に渡したポインタがそのまま返ってくる。
   ここではカウンタやラベルのハンドルを渡し、グローバル変数を使わずに状態を持ち回る。 */

static void NO_VCL_CALL OnComponentFreed(obj_t obj, void* data)
{
    (void)obj;
    ++*(int*)data;
}

/* コールバックの中で処理を失敗させる(docs/adr/0031)。TMenuItem_Click の中で呼ばれ、その関数が失敗する。 */
static void NO_VCL_CALL OnFailingMenuClick(obj_t sender, void* data)
{
    (void)sender;
    (void)data;
    SetCallbackError("EMyError", "failed in the callback");
}

static void NO_VCL_CALL OnButtonClick(obj_t sender, void* data)
{
    int* clickCount = (int*)data;
    (void)sender;
    ++*clickCount;
    printf("Button clicked! (count=%d)\n", *clickCount);
    fflush(stdout);
}

static void NO_VCL_CALL OnButtonMouseDown(obj_t sender, int_t button, int_t shift, int_t x, int_t y, void* data)
{
    (void)sender; (void)data;
    printf("Button mouse down! button=%d shift=0x%x pos=(%d,%d)\n", (int)button, (unsigned)shift, (int)x, (int)y);
    fflush(stdout);
}

static void NO_VCL_CALL OnEditKeyDown(obj_t sender, int_t* key, int_t shift, void* data)
{
    (void)sender; (void)data;
    printf("Edit key down! key=%d shift=0x%x\n", (int)*key, (unsigned)shift);
    fflush(stdout);
}

static void NO_VCL_CALL OnScrollBarChange(obj_t sender, void* data)
{
    (void)data;
    printf("ScrollBar changed! Position=%d\n", TCustomScrollBar_GetPosition(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnTrackBarChange(obj_t sender, void* data)
{
    (void)data;
    printf("TrackBar changed! Position=%d\n", TCustomTrackBar_GetPosition(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnRadioGroupClick(obj_t sender, void* data)
{
    (void)data;
    printf("RadioGroup clicked! ItemIndex=%d\n", TCustomRadioGroup_GetItemIndex(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnCheckListBoxClickCheck(obj_t sender, void* data)
{
    (void)data;
    printf("CheckListBox check clicked! Checked[0]/[1]/[2]=%d/%d/%d\n",
           TCustomCheckListBox_GetChecked(sender, 0), TCustomCheckListBox_GetChecked(sender, 1),
           TCustomCheckListBox_GetChecked(sender, 2));
    fflush(stdout);
}

static void NO_VCL_CALL OnSpeedButtonClick(obj_t sender, void* data)
{
    (void)data;
    printf("SpeedButton clicked! Down=%d\n", TCustomSpeedButton_GetDown(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnTabControlChange(obj_t sender, void* data)
{
    (void)data;
    printf("TabControl changed! TabIndex=%d\n", TTabControl_GetTabIndex(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnCheckBoxClick(obj_t sender, void* data)
{
    (void)data;
    printf("CheckBox clicked! Checked=%d\n", TButtonControl_GetChecked(sender));
    fflush(stdout);
}

/* 2つのラジオボタンで共有し、sender でどちらが押されたかを区別する。 */
static void NO_VCL_CALL OnRadioButtonClick(obj_t sender, void* data)
{
    (void)data;
    printf("RadioButton clicked! %s Checked=%d\n",
           TControl_GetCaption(sender), TButtonControl_GetChecked(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnEditChange(obj_t sender, void* data)
{
    (void)data;
    printf("Edit changed! Text=%s\n", TControl_GetText(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnComboBoxChange(obj_t sender, void* data)
{
    (void)data;
    printf("ComboBox changed! ItemIndex=%d Text=%s\n",
           TCustomComboBox_GetItemIndex(sender), TControl_GetText(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnListBoxClick(obj_t sender, void* data)
{
    (void)data;
    printf("ListBox clicked! ItemIndex=%d\n", TCustomListBox_GetItemIndex(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnMemoChange(obj_t sender, void* data)
{
    (void)data;
    printf("Memo changed! LineCount=%d\n", TStrings_GetCount(TCustomMemo_GetLines(sender)));
    fflush(stdout);
}

/* *action には既定の動作が入っている。1 回目は閉じるのを取りやめ、2 回目は既定の動作のままにする。 */
static void NO_VCL_CALL OnFormClose(obj_t sender, int_t* action, void* data)
{
    int* attempts = (int*)data;
    (void)sender;
    ++*attempts;
    printf("Form closing: attempt=%d, default action=%d\n", *attempts, *action);
    if (*attempts == 1)
    {
        *action = caNone;
        printf("Form closing: blocked (caNone)\n");
    }
    fflush(stdout);
}

static void NO_VCL_CALL OnFormCloseQuery(obj_t sender, bool_t* canClose, void* data)
{
    (void)sender;
    (void)data;
    printf("Form close query: default canClose=%d\n", *canClose != 0);
    fflush(stdout);
}

/* 破棄の最初に呼ばれる。子コントロールはまだ有効なので、data で渡したボタンの Caption を読める。 */
static void NO_VCL_CALL OnFormDestroy(obj_t sender, void* data)
{
    (void)sender;
    printf("Form destroying: button caption=%s\n", TControl_GetCaption((obj_t)data));
    fflush(stdout);
}

/* Align で配置するコントロール(OnFormShow の data)。 */
typedef struct
{
    obj_t topPanel;
    obj_t leftPanel;
    obj_t splitter;
    obj_t clientPanel;
} AlignedControls;

static void PrintBounds(const char* name, obj_t control, const char* expected)
{
    printf("%s Bounds=(%d,%d,%d,%d) (expected %s)\n", name,
           TControl_GetLeft(control), TControl_GetTop(control),
           TControl_GetWidth(control), TControl_GetHeight(control), expected);
}

static void NO_VCL_CALL OnFormShow(obj_t sender, void* data)
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
    TCustomSplitter_SetSplitterPosition(aligned->splitter, 121);
    printf("After SetSplitterPosition(121): SplitterPosition=%d, alLeft Width=%d, alClient Left=%d (expected 121/120/126)\n",
           TCustomSplitter_GetSplitterPosition(aligned->splitter),
           TControl_GetWidth(aligned->leftPanel), TControl_GetLeft(aligned->clientPanel));
    fflush(stdout);
}

/* TColorはDelphi/LCLの$00BBGGRR順パック整数 */
#define CL_BLACK  0x000000
#define CL_WHITE  0xFFFFFF
#define CL_RED    0x0000FF
#define CL_GREEN  0x008000
#define CL_BLUE   0xFF0000
#define CL_YELLOW 0x00FFFF

static void NO_VCL_CALL OnPaintBoxPaint(obj_t sender, void* data)
{
    obj_t canvas = TPaintBox_GetCanvas(sender);
    obj_t pen = TCanvas_GetPen(canvas);
    obj_t brush = TCanvas_GetBrush(canvas);
    obj_t font = TCanvas_GetFont(canvas);
    (void)data;

    TPen_SetColor(pen, CL_RED);
    TPen_SetWidth(pen, 2);
    TBrush_SetColor(brush, CL_YELLOW);
    TCanvas_Rectangle(canvas, 10, 10, 110, 70);

    TPen_SetColor(pen, CL_BLUE);
    TBrush_SetColor(brush, CL_WHITE);
    TCanvas_Ellipse(canvas, 120, 10, 200, 70);

    TPen_SetColor(pen, CL_BLACK);
    TCanvas_MoveTo(canvas, 10, 90);
    TCanvas_LineTo(canvas, 200, 90);

    TFont_SetColor(font, CL_GREEN);
    TFont_SetSize(font, 14);
    TCanvas_TextOut(canvas, 10, 100, "Canvas drawing test");
}

/* data にはカウントを表示するラベルのハンドルを渡す。 */
static void NO_VCL_CALL OnTimerTick(obj_t sender, void* data)
{
    static int tickCount = 0;
    char buf[64];
    (void)sender;
    ++tickCount;
    snprintf(buf, sizeof(buf), "Tick: %d", tickCount);
    TControl_SetCaption((obj_t)data, buf);
    printf("Timer tick! count=%d\n", tickCount);
    fflush(stdout);
}

static void NO_VCL_CALL OnMenuItemClick(obj_t sender, void* data)
{
    (void)data;
    printf("Menu item clicked! Caption=%s\n", TMenuItem_GetCaption(sender));
    fflush(stdout);
}

/* data は Application。 */
static void NO_VCL_CALL OnExitItemClick(obj_t sender, void* data)
{
    (void)sender;
    TApplication_Terminate((obj_t)data);
}

/* data は PopupMenu を割り当てたパネル。 */
static void NO_VCL_CALL OnPopupMenuPopup(obj_t sender, void* data)
{
    printf("PopupMenu popup! PopupComponent is panel: %s\n",
           TPopupMenu_GetPopupComponent(sender) == data ? "yes" : "no");
    fflush(stdout);
}

static void NO_VCL_CALL OnPageControlChange(obj_t sender, void* data)
{
    (void)data;
    printf("PageControl changed! ActivePageIndex=%d\n", TPageControl_GetActivePageIndex(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnPageControlChanging(obj_t sender, bool_t* allowChange, void* data)
{
    (void)sender; (void)data;
    printf("PageControl changing! allowChange=%d\n", *allowChange != 0);
    fflush(stdout);
}

/* 項目(ツリービューのノード・リストビューの項目と列)の破棄通知。data は破棄された項目の数。 */
static void NO_VCL_CALL OnItemFreed(obj_t item, void* data)
{
    (void)item;
    ++*(int*)data;
}

/* data は削除された項目の数(OnDeletion の回数)。 */
static void NO_VCL_CALL OnListViewDeletion(obj_t sender, obj_t item, void* data)
{
    (void)sender; (void)item;
    ++*(int*)data;
}

static void NO_VCL_CALL OnGridSelection(obj_t sender, int_t col, int_t row, void* data)
{
    (void)data;
    printf("Grid selection: (%d,%d) = %s\n", (int)col, (int)row, TCustomStringGrid_GetCells(sender, col, row));
    fflush(stdout);
}

static void NO_VCL_CALL OnListViewSelectItem(obj_t sender, obj_t item, int_t selected, void* data)
{
    (void)sender; (void)data;
    printf("ListView item %s: %s\n", selected ? "selected" : "unselected", TListItem_GetCaption(item));
    fflush(stdout);
}

static void NO_VCL_CALL OnTreeViewChange(obj_t sender, obj_t node, void* data)
{
    (void)sender; (void)data;
    printf("TreeView changed! Selected=%s\n", node ? TTreeNode_GetText(node) : "(none)");
    fflush(stdout);
}

/* "Locked" という名前のノードは折りたためないようにする。 */
static void NO_VCL_CALL OnTreeViewCollapsing(obj_t sender, obj_t node, bool_t* allow, void* data)
{
    const char* text = TTreeNode_GetText(node);
    (void)sender; (void)data;
    if (text[0] == 'L')
        *allow = 0;
}

/* data は Splitter が幅を変える alLeft のパネル。 */
static void NO_VCL_CALL OnSplitterMoved(obj_t sender, void* data)
{
    printf("Splitter moved! SplitterPosition=%d, left pane Width=%d\n",
           TCustomSplitter_GetSplitterPosition(sender), TControl_GetWidth((obj_t)data));
    fflush(stdout);
}

/* コントロールを生成し、親と位置を設定する(Owner と Parent はどちらも form)。 */
static obj_t Place(obj_t control, obj_t parent, int left, int top)
{
    TControl_SetParent(control, parent);
    TControl_SetLeft(control, left);
    TControl_SetTop(control, top);
    return control;
}

int main(void)
{
    int freedCount = 0;
    int clickCount = 0;
    int closeAttempts = 0;
    obj_t app;
    obj_t form;
    obj_t button;
    obj_t label;
    obj_t edit;
    obj_t checkBox;
    obj_t radio1;
    obj_t radio2;
    obj_t panel;
    obj_t groupBox;
    obj_t comboBox;
    obj_t listBox;
    obj_t memo;
    obj_t noHandler;
    obj_t tickLabel;
    obj_t timer;
    obj_t paintBox;
    obj_t scrollBox;
    obj_t scrolledButton;
    obj_t toggleBox;
    obj_t bevel;
    obj_t shape;
    obj_t staticText;
    obj_t statusBar;
    obj_t scrollBar;
    obj_t trackBar;
    obj_t progressBar;
    obj_t upDownEdit;
    obj_t upDown;
    obj_t radioGroup;
    obj_t checkGroup;
    obj_t checkListBox;
    obj_t speedButton;
    obj_t bitBtn;
    obj_t floatSpinEdit;
    obj_t spinEdit;
    obj_t maskEdit;
    obj_t labeledEdit;
    obj_t tabControl;
    obj_t layoutPanel;
    AlignedControls aligned;
    obj_t mainMenu;
    obj_t menuRoot;
    obj_t fileMenu;
    obj_t openItem;
    obj_t exitItem;
    obj_t separator;
    obj_t popupMenu;
    obj_t popupItem;
    obj_t pageControl;
    obj_t tabSheet1;
    obj_t tabSheet2;
    obj_t tempPageControl;
    obj_t treeView;
    obj_t treeItems;
    obj_t rootNode;
    obj_t childNode;
    obj_t root2Node;
    obj_t lockedNode;
    int itemsFreed = 0;
    obj_t listView;
    obj_t listColumns;
    obj_t listItems;
    obj_t listItem;
    int listDeletions = 0;
    obj_t gridSheet;
    obj_t stringGrid;
    int_t selLeft, selTop, selRight, selBottom;
    obj_t headerSheet;
    obj_t headerControl;
    obj_t sections;
    obj_t toolsSheet;
    obj_t toolBar;
    obj_t leftButton;
    obj_t rightButton;
    obj_t coolBar;
    obj_t coolBands;

    FreeNotify_SetCallback(OnComponentFreed, &freedCount);

    app = GetApplication();
    TApplication_SetTitle(app, "no_vcl internal test");
    printf("Title: %s\n", TApplication_GetTitle(app));

    /* 最初に CreateForm で生成したフォームが MainForm になる(Owner は Application)。 */
    form = TApplication_CreateForm(app);
    if (!form)
    {
        printf("TApplication_CreateForm failed\n");
        return 1;
    }
    printf("MainForm is form: %s\n", TApplication_GetMainForm(app) == form ? "yes" : "no");
    TCustomForm_SetOnClose(form, OnFormClose, &closeAttempts);
    /* aligned の中身は下で Align のコントロールを生成したときに埋める(表示されるのは Run() の後)。 */
    TCustomForm_SetOnShow(form, OnFormShow, &aligned);

    TControl_SetCaption(form, "Hello from FPC DLL");
    TControl_SetWidth(form, 640);
    TControl_SetHeight(form, 930);
    printf("Caption: %s\n", TControl_GetCaption(form));

    button = Place(TButton_Create(form), form, 20, 20);
    TControl_SetCaption(button, "Click me");
    TControl_SetWidth(button, 100);
    TControl_SetHeight(button, 30);
    TControl_SetOnClick(button, OnButtonClick, &clickCount);
    TControl_SetOnMouseDown(button, OnButtonMouseDown, NULL);
    TCustomForm_SetOnCloseQuery(form, OnFormCloseQuery, NULL);
    TCustomForm_SetOnDestroy(form, OnFormDestroy, button);
    printf("Button caption: %s\n", TControl_GetCaption(button));
    printf("Button parent is form: %s\n", TControl_GetParent(button) == form ? "yes" : "no");

    label = Place(TLabel_Create(form), form, 20, 60);
    TControl_SetCaption(label, "Label text");

    edit = Place(TEdit_Create(form), form, 20, 90);
    TControl_SetText(edit, "Edit me");
    TControl_SetWidth(edit, 150);
    TCustomEdit_SetOnChange(edit, OnEditChange, NULL);
    TWinControl_SetOnKeyDown(edit, OnEditKeyDown, NULL);

    checkBox = Place(TCheckBox_Create(form), form, 20, 130);
    TControl_SetCaption(checkBox, "Check me");
    /* 登録し直すと最後に登録したものだけが呼ばれる(ブリッジは再利用され、蓄積しない)。 */
    TControl_SetOnClick(checkBox, OnButtonClick, &clickCount);
    TControl_SetOnClick(checkBox, OnCheckBoxClick, NULL);

    radio1 = Place(TRadioButton_Create(form), form, 20, 160);
    TControl_SetCaption(radio1, "Option A");
    TButtonControl_SetChecked(radio1, 1);
    TControl_SetOnClick(radio1, OnRadioButtonClick, NULL);

    radio2 = Place(TRadioButton_Create(form), form, 20, 190);
    TControl_SetCaption(radio2, "Option B");
    TControl_SetOnClick(radio2, OnRadioButtonClick, NULL);

    panel = Place(TPanel_Create(form), form, 220, 20);
    TControl_SetCaption(panel, "");
    TControl_SetWidth(panel, 180);
    TControl_SetHeight(panel, 60);

    groupBox = Place(TGroupBox_Create(form), form, 220, 90);
    TControl_SetCaption(groupBox, "Group");
    TControl_SetWidth(groupBox, 180);
    TControl_SetHeight(groupBox, 60);

    comboBox = Place(TComboBox_Create(form), form, 220, 160);
    TStrings_Add(TCustomComboBox_GetItems(comboBox), "Combo A");
    TStrings_Add(TCustomComboBox_GetItems(comboBox), "Combo B");
    TStrings_Add(TCustomComboBox_GetItems(comboBox), "Combo C");
    TCustomComboBox_SetItemIndex(comboBox, 0);
    TControl_SetWidth(comboBox, 150);
    TComboBox_SetOnChange(comboBox, OnComboBoxChange, NULL);

    listBox = Place(TListBox_Create(form), form, 220, 190);
    TStrings_Add(TCustomListBox_GetItems(listBox), "List 1");
    TStrings_Add(TCustomListBox_GetItems(listBox), "List 2");
    TStrings_Add(TCustomListBox_GetItems(listBox), "List 3");
    {
        /* TStrings のハンドルは保存せず、使うたびに取得する(LCL がウィンドウの生成時に中身を差し替えるため)。 */
        char text[64];
        snprintf(text, sizeof(text), "%s", TStrings_GetCommaText(TCustomListBox_GetItems(listBox)));
        printf("ListBox Items CommaText=%s (expected \"List 1\",\"List 2\",\"List 3\"), IndexOf(\"List 2\")=%d (expected 1)\n",
               text, TStrings_IndexOf(TCustomListBox_GetItems(listBox), "List 2"));
    }
    TControl_SetWidth(listBox, 150);
    TControl_SetHeight(listBox, 80);
    /* 利用者による選択の変更(マウス・キー操作とも)で呼ばれる。プログラムからの ItemIndex の変更では呼ばれない。 */
    TControl_SetOnClick(listBox, OnListBoxClick, NULL);
    TCustomListBox_SetItemIndex(listBox, 1);

    memo = Place(TMemo_Create(form), form, 220, 280);
    TStrings_Add(TCustomMemo_GetLines(memo), "Memo line 1");
    TStrings_Add(TCustomMemo_GetLines(memo), "Memo line 2");
    TControl_SetWidth(memo, 150);
    TControl_SetHeight(memo, 80);
    TCustomEdit_SetOnChange(memo, OnMemoChange, NULL);

    /* NULL を登録するとハンドラが解除され、押しても何も呼ばれない。 */
    noHandler = Place(TButton_Create(form), form, 20, 280);
    TControl_SetCaption(noHandler, "No handler");
    TControl_SetOnClick(noHandler, OnButtonClick, &clickCount);
    TControl_SetOnClick(noHandler, NULL, NULL);

    tickLabel = Place(TLabel_Create(form), form, 20, 230);
    TControl_SetCaption(tickLabel, "Tick: 0");

    timer = TTimer_Create(form);
    TCustomTimer_SetInterval(timer, 500);
    TCustomTimer_SetOnTimer(timer, OnTimerTick, tickLabel);
    TCustomTimer_SetEnabled(timer, 1);

    paintBox = Place(TPaintBox_Create(form), form, 400, 20);
    TControl_SetWidth(paintBox, 220);
    TControl_SetHeight(paintBox, 130);
    TPaintBox_SetOnPaint(paintBox, OnPaintBoxPaint, NULL);

    /* docs/component-coverage.md の Tier 1 で追加したコントロール(1 バッチ目)。 */
    scrollBox = Place(TScrollBox_Create(form), form, 20, 380);
    TControl_SetWidth(scrollBox, 180);
    TControl_SetHeight(scrollBox, 50);

    scrolledButton = Place(TButton_Create(form), scrollBox, 10, 10);
    TControl_SetCaption(scrolledButton, "Inside ScrollBox");
    TControl_SetWidth(scrolledButton, 140);
    printf("ScrolledButton parent is scrollBox: %s\n", TControl_GetParent(scrolledButton) == scrollBox ? "yes" : "no");

    toggleBox = Place(TToggleBox_Create(form), form, 220, 380);
    TControl_SetCaption(toggleBox, "Toggle me");
    TControl_SetOnClick(toggleBox, OnCheckBoxClick, NULL);

    bevel = Place(TBevel_Create(form), form, 340, 380);
    TControl_SetWidth(bevel, 100);
    TControl_SetHeight(bevel, 50);
    TBevel_SetShape(bevel, bsFrame);
    TBevel_SetStyle(bevel, bsRaised);

    shape = Place(TShape_Create(form), form, 460, 380);
    TControl_SetWidth(shape, 60);
    TControl_SetHeight(shape, 50);
    TCustomShape_SetShape(shape, stEllipse);
    TBrush_SetColor(TCustomShape_GetBrush(shape), CL_YELLOW);
    TPen_SetColor(TCustomShape_GetPen(shape), CL_BLUE);

    staticText = Place(TStaticText_Create(form), form, 20, 440);
    TControl_SetCaption(staticText, "Static text");
    TControl_SetWidth(staticText, 150);
    TCustomStaticText_SetBorderStyle(staticText, sbsSunken);

    /* Tier 1、3 バッチ目(範囲・数値系のコントロール)。 */
    scrollBar = Place(TScrollBar_Create(form), form, 20, 480);
    TControl_SetWidth(scrollBar, 150);
    TCustomScrollBar_SetMin(scrollBar, 0);
    TCustomScrollBar_SetMax(scrollBar, 100);
    TCustomScrollBar_SetPosition(scrollBar, 30);
    TCustomScrollBar_SetOnChange(scrollBar, OnScrollBarChange, NULL);

    trackBar = Place(TTrackBar_Create(form), form, 190, 480);
    TControl_SetWidth(trackBar, 150);
    TCustomTrackBar_SetMin(trackBar, 0);
    TCustomTrackBar_SetMax(trackBar, 10);
    TCustomTrackBar_SetPosition(trackBar, 5);
    TCustomTrackBar_SetOnChange(trackBar, OnTrackBarChange, NULL);

    progressBar = Place(TProgressBar_Create(form), form, 360, 480);
    TControl_SetWidth(progressBar, 150);
    TCustomProgressBar_SetMin(progressBar, 0);
    TCustomProgressBar_SetMax(progressBar, 100);
    TCustomProgressBar_SetPosition(progressBar, 42);
    printf("ScrollBar/TrackBar/ProgressBar Position: %d/%d/%d\n",
           TCustomScrollBar_GetPosition(scrollBar), TCustomTrackBar_GetPosition(trackBar),
           TCustomProgressBar_GetPosition(progressBar));

    upDownEdit = Place(TEdit_Create(form), form, 20, 510);
    TControl_SetText(upDownEdit, "3");
    TControl_SetWidth(upDownEdit, 60);

    upDown = Place(TUpDown_Create(form), form, 80, 510);
    TUpDown_SetMin(upDown, 0);
    TUpDown_SetMax(upDown, 10);
    TUpDown_SetPosition(upDown, 3);
    TUpDown_SetIncrement(upDown, 1);
    TUpDown_SetAssociate(upDown, upDownEdit);
    printf("UpDown Position: %d, Associate is upDownEdit: %s\n",
           TUpDown_GetPosition(upDown), TUpDown_GetAssociate(upDown) == upDownEdit ? "yes" : "no");

    /* Tier 1、4 バッチ目(Items を持つグループ・リスト系のコントロール)。 */
    radioGroup = Place(TRadioGroup_Create(form), form, 20, 550);
    TControl_SetCaption(radioGroup, "RadioGroup1");
    TControl_SetWidth(radioGroup, 180);
    TControl_SetHeight(radioGroup, 90);
    TStrings_Add(TCustomRadioGroup_GetItems(radioGroup), "Option A");
    TStrings_Add(TCustomRadioGroup_GetItems(radioGroup), "Option B");
    TStrings_Add(TCustomRadioGroup_GetItems(radioGroup), "Option C");
    TCustomRadioGroup_SetItemIndex(radioGroup, 1);
    TCustomRadioGroup_SetOnClick(radioGroup, OnRadioGroupClick, NULL);

    checkGroup = Place(TCheckGroup_Create(form), form, 210, 550);
    TControl_SetCaption(checkGroup, "CheckGroup1");
    TControl_SetWidth(checkGroup, 180);
    TControl_SetHeight(checkGroup, 90);
    TStrings_Add(TCustomCheckGroup_GetItems(checkGroup), "Feature X");
    TStrings_Add(TCustomCheckGroup_GetItems(checkGroup), "Feature Y");
    TStrings_Add(TCustomCheckGroup_GetItems(checkGroup), "Feature Z");
    TCustomCheckGroup_SetChecked(checkGroup, 0, 1);
    TCustomCheckGroup_SetChecked(checkGroup, 2, 1);

    checkListBox = Place(TCheckListBox_Create(form), form, 400, 550);
    TControl_SetWidth(checkListBox, 180);
    TControl_SetHeight(checkListBox, 90);
    TStrings_Add(TCustomListBox_GetItems(checkListBox), "Item 1");
    TStrings_Add(TCustomListBox_GetItems(checkListBox), "Item 2");
    TStrings_Add(TCustomListBox_GetItems(checkListBox), "Item 3");
    TCustomCheckListBox_SetChecked(checkListBox, 1, 1);
    TCustomCheckListBox_SetOnClickCheck(checkListBox, OnCheckListBoxClickCheck, NULL);
    printf("RadioGroup ItemIndex=%d, CheckGroup Checked[0]/[1]/[2]=%d/%d/%d, CheckListBox Checked[1]=%d\n",
           TCustomRadioGroup_GetItemIndex(radioGroup),
           TCustomCheckGroup_GetChecked(checkGroup, 0), TCustomCheckGroup_GetChecked(checkGroup, 1),
           TCustomCheckGroup_GetChecked(checkGroup, 2), TCustomCheckListBox_GetChecked(checkListBox, 1));

    /* Tier 1、5 バッチ目(ボタンの派生)。 */
    speedButton = Place(TSpeedButton_Create(form), form, 20, 650);
    TControl_SetCaption(speedButton, "Speed");
    TControl_SetWidth(speedButton, 80);
    TCustomSpeedButton_SetGroupIndex(speedButton, 1);
    TControl_SetOnClick(speedButton, OnSpeedButtonClick, NULL);

    bitBtn = Place(TBitBtn_Create(form), form, 120, 650);
    TCustomBitBtn_SetKind(bitBtn, bkOK);
    printf("BitBtn Kind=%d (expected bkOK=1), Caption=%s\n",
           TCustomBitBtn_GetKind(bitBtn), TControl_GetCaption(bitBtn));

    /* Tier 1、6 バッチ目(数値・書式付き Edit)。 */
    floatSpinEdit = Place(TFloatSpinEdit_Create(form), form, 20, 690);
    TControl_SetWidth(floatSpinEdit, 100);
    TCustomFloatSpinEdit_SetMinValue(floatSpinEdit, 0.0);
    TCustomFloatSpinEdit_SetMaxValue(floatSpinEdit, 10.0);
    TCustomFloatSpinEdit_SetIncrement(floatSpinEdit, 0.5);
    TCustomFloatSpinEdit_SetDecimalPlaces(floatSpinEdit, 1);
    TCustomFloatSpinEdit_SetValue(floatSpinEdit, 2.5);

    spinEdit = Place(TSpinEdit_Create(form), form, 130, 690);
    TControl_SetWidth(spinEdit, 100);
    TCustomSpinEdit_SetMinValue(spinEdit, 0);
    TCustomSpinEdit_SetMaxValue(spinEdit, 100);
    TCustomSpinEdit_SetIncrement(spinEdit, 5);
    TCustomSpinEdit_SetValue(spinEdit, 42);

    maskEdit = Place(TMaskEdit_Create(form), form, 240, 690);
    TControl_SetWidth(maskEdit, 100);
    TMaskEdit_SetEditMask(maskEdit, "000-0000;1;_");
    printf("FloatSpinEdit Value=%.1f, SpinEdit Value=%d, MaskEdit EditMask=%s\n",
           TCustomFloatSpinEdit_GetValue(floatSpinEdit), TCustomSpinEdit_GetValue(spinEdit),
           TMaskEdit_GetEditMask(maskEdit));

    /* TLabeledEdit。EditLabel は LCL が内部で生成したラベルで、取得すると破棄通知の対象になる(LabeledEdit と一緒に破棄される)。 */
    labeledEdit = Place(TLabeledEdit_Create(form), form, 420, 690);
    TControl_SetWidth(labeledEdit, 100);
    TCustomLabeledEdit_SetLabelPosition(labeledEdit, lpLeft);
    TControl_SetCaption(TCustomLabeledEdit_GetEditLabel(labeledEdit), "Zip:");
    printf("LabeledEdit EditLabel Caption=%s (expected Zip:), Parent is form: %s, LabelPosition=%d (expected lpLeft=%d), LabelSpacing=%d (expected 3)\n",
           TControl_GetCaption(TCustomLabeledEdit_GetEditLabel(labeledEdit)),
           TControl_GetParent(TCustomLabeledEdit_GetEditLabel(labeledEdit)) == form ? "yes" : "no",
           TCustomLabeledEdit_GetLabelPosition(labeledEdit), lpLeft,
           TCustomLabeledEdit_GetLabelSpacing(labeledEdit));

    /* TStringList。利用者が生成し、TStringList_Destroy で破棄する。操作は TStrings_*。 */
    {
        obj_t list = TStringList_Create();
        int_t index = -1;
        bool_t found;
        char text[64];
        TStrings_SetCommaText(list, "cherry,Banana,apple");
        TStringList_SetSorted(list, 1);
        found = TStringList_Find(list, "banana", &index);
        TStrings_SetValues(list, "key", "value");
        snprintf(text, sizeof(text), "%s", TStrings_GetCommaText(list));
        printf("TStringList Sorted CommaText=%s (expected apple,Banana,cherry,key=value), Find(banana)=%d/%d (expected 1/1), "
               "Values[key]=%s (expected value)\n",
               text, found != 0, index, TStrings_GetValues(list, "key"));
        TStringList_Destroy(list);
    }

    /* 例外(docs/adr/0031・0032)。DLL の中で LCL が例外を送出すると、内部層の関数が Exception を送出する。 */
    {
        obj_t list = TStringList_Create();
        TStrings_Add(list, "only");
        try
        {
            TStrings_GetStrings(list, 5);
            printf("must not be reached\n");
        }
        catch (Exception& E)
        {
            printf("internal GetStrings(5) on 1 item threw: ClassName=%s (expected EStringListError), Message=%s\n",
                   E.ClassName().c_str(), E.Message.c_str());
        }
        printf("internal after the exception: Count=%d (expected 1)\n", TStrings_GetCount(list));
        TStringList_Destroy(list);

        /* コールバックの中で SetCallbackError を呼ぶと、そのイベントを起こした関数(ここでは TMenuItem_Click)が失敗する。 */
        obj_t failingItem = TMenuItem_Create(form);
        TMenuItem_SetOnClick(failingItem, OnFailingMenuClick, NULL);
        try
        {
            TMenuItem_Click(failingItem);
            printf("must not be reached\n");
        }
        catch (Exception& E)
        {
            printf("internal TMenuItem_Click with a failing callback threw: ClassName=%s (expected EMyError), Message=%s\n",
                   E.ClassName().c_str(), E.Message.c_str());
        }
    }

    /* グラフィックス(docs/adr/0029)。生成したグラフィック・TPicture は TGraphic_Destroy・TPicture_Destroy で破棄する。
       TPicture の中身(TPicture_GetGraphic 等)のハンドルは保存せず、使うたびに取得する。 */
    {
        const char* path = "no_vcl_graphic_test_internal.png";
        obj_t bmp = TBitmap_Create();
        obj_t png = TPortableNetworkGraphic_Create();
        obj_t pic = TPicture_Create();
        obj_t canvas;
        obj_t image;
        TCustomBitmap_SetSize(bmp, 20, 10);
        canvas = TRasterImage_GetCanvas(bmp);
        TBrush_SetColor(TCanvas_GetBrush(canvas), 0x00FF00);
        TCanvas_FillRect(canvas, 0, 0, 20, 10);
        TCanvas_SetPixels(canvas, 3, 4, 0xFF0000);
        printf("internal TBitmap %dx%d (expected 20x10), Pixels[0][0]=%06X (expected 00FF00), Pixels[3][4]=%06X (expected FF0000)\n",
               TGraphic_GetWidth(bmp), TGraphic_GetHeight(bmp),
               (unsigned)TCanvas_GetPixels(canvas, 0, 0), (unsigned)TCanvas_GetPixels(canvas, 3, 4));
        TGraphic_Assign(png, bmp);
        TGraphic_SaveToFile(png, path);
        TPicture_LoadFromFile(pic, path);
        printf("internal TPicture LoadFromFile(png): %dx%d (expected 20x10), Bitmap Pixels[3][4]=%06X (expected FF0000)\n",
               TPicture_GetWidth(pic), TPicture_GetHeight(pic),
               (unsigned)TCanvas_GetPixels(TRasterImage_GetCanvas(TPicture_GetBitmap(pic)), 3, 4));

        /* TImage。Picture を設定すると内容が写される(pic はそのまま呼び出し側の持ち物)。 */
        image = Place(TImage_Create(form), form, 560, 20);
        TControl_SetAutoSize(image, 1);
        TCustomImage_SetPicture(image, pic);
        TPicture_Clear(pic);
        printf("internal TImage Picture %dx%d (expected 20x10), HasGraphic=%d (expected 1), AutoSize=%d, source Picture cleared: %s\n",
               TPicture_GetWidth(TCustomImage_GetPicture(image)),
               TPicture_GetHeight(TCustomImage_GetPicture(image)),
               TCustomImage_GetHasGraphic(image) != 0, TControl_GetAutoSize(image) != 0,
               TPicture_GetGraphic(pic) == NULL ? "yes" : "no");

        /* TImageList(docs/adr/0030)。TComponent なので Owner(フォーム)に破棄を任せる。
           20x10 の画像を AddSliced で 10x10 の 2 つに分けて加え、TImage の Images・ImageIndex に設定する。 */
        {
            obj_t imageList = TImageList_Create(form);
            int first;
            TCustomImageList_SetWidth(imageList, 10);
            TCustomImageList_SetHeight(imageList, 10);
            first = TCustomImageList_AddSliced(imageList, bmp, 2, 1);
            TCustomImage_SetImages(image, imageList);
            TCustomImage_SetImageIndex(image, 1);
            printf("internal TImageList AddSliced: first=%d (expected 0), Count=%d (expected 2), TImage Images is the list: %s, ImageIndex=%d (expected 1)\n",
                   first, TCustomImageList_GetCount(imageList),
                   TCustomImage_GetImages(image) == imageList ? "yes" : "no", TCustomImage_GetImageIndex(image));
        }

        TPicture_Destroy(pic);
        TGraphic_Destroy(png);
        TGraphic_Destroy(bmp);
        remove(path);
    }

    /* Tier 1、7 バッチ目(最後のバッチ)。ページ付きの TPageControl/TTabSheet は今回見送る。 */
    tabControl = Place(TTabControl_Create(form), form, 20, 730);
    TControl_SetWidth(tabControl, 300);
    TControl_SetHeight(tabControl, 90);
    TStrings_Add(TTabControl_GetTabs(tabControl), "Tab A");
    TStrings_Add(TTabControl_GetTabs(tabControl), "Tab B");
    TStrings_Add(TTabControl_GetTabs(tabControl), "Tab C");
    TTabControl_SetTabIndex(tabControl, 0);
    TTabControl_SetOnChange(tabControl, OnTabControlChange, NULL);
    printf("TabControl TabsCount=%d, TabIndex=%d\n",
           TStrings_GetCount(TTabControl_GetTabs(tabControl)), TTabControl_GetTabIndex(tabControl));

    /* TStatusBar も Run() の前に生成してよい(ADR 0015 の問題は DLL 側で回避済み)。 */
    statusBar = TStatusBar_Create(form);
    TControl_SetParent(statusBar, form);
    TStatusBar_SetSimpleText(statusBar, "Ready");
    printf("StatusBar SimpleText: %s\n", TStatusBar_GetSimpleText(statusBar));
    /* TStatusBar の Align の既定値は alBottom(Left/Top を指定しなくてもフォームの下端に付く)。 */
    printf("StatusBar Align=%d (expected alBottom=%d)\n", TControl_GetAlign(statusBar), alBottom);

    /* TControl.Align と TSplitter。layoutPanel の中を、上端の alTop、左の alLeft + Splitter、残りの alClient で分ける。
       alLeft 同士は Left の小さい順に並ぶため、Splitter(既定の Align が alLeft)が alLeft のパネルの右に来るよう、
       Parent より先に Left をパネルの幅より大きくしておく。 */
    layoutPanel = Place(TPanel_Create(form), form, 340, 730);
    TControl_SetWidth(layoutPanel, 280);
    TControl_SetHeight(layoutPanel, 90);
    TControl_SetCaption(layoutPanel, "");
    aligned.topPanel = TPanel_Create(form);
    TControl_SetParent(aligned.topPanel, layoutPanel);
    TControl_SetAlign(aligned.topPanel, alTop);
    TControl_SetHeight(aligned.topPanel, 20);
    TControl_SetCaption(aligned.topPanel, "alTop");
    aligned.leftPanel = TPanel_Create(form);
    TControl_SetParent(aligned.leftPanel, layoutPanel);
    TControl_SetAlign(aligned.leftPanel, alLeft);
    TControl_SetWidth(aligned.leftPanel, 80);
    TControl_SetCaption(aligned.leftPanel, "alLeft");
    aligned.splitter = TSplitter_Create(form);
    TControl_SetLeft(aligned.splitter, 100);
    TControl_SetParent(aligned.splitter, layoutPanel);
    TCustomSplitter_SetMinSize(aligned.splitter, 40);
    TCustomSplitter_SetBeveled(aligned.splitter, 1);
    TCustomSplitter_SetOnMoved(aligned.splitter, OnSplitterMoved, aligned.leftPanel);
    aligned.clientPanel = TPanel_Create(form);
    TControl_SetParent(aligned.clientPanel, layoutPanel);
    TControl_SetAlign(aligned.clientPanel, alClient);
    TControl_SetCaption(aligned.clientPanel, "alClient");
    /* 配置後の位置・大きさは OnFormShow で確認する(表示されるまで Align による配置は行われない)。 */
    printf("alTop/alLeft/alClient panel Align=%d/%d/%d (expected %d/%d/%d)\n",
           TControl_GetAlign(aligned.topPanel), TControl_GetAlign(aligned.leftPanel),
           TControl_GetAlign(aligned.clientPanel), alTop, alLeft, alClient);
    printf("Splitter Align=%d (expected alLeft=%d) MinSize=%d Beveled=%d AutoSnap=%d "
           "ResizeAnchor=%d (expected akLeft=%d) ResizeStyle=%d (expected rsUpdate=%d)\n",
           TControl_GetAlign(aligned.splitter), alLeft,
           TCustomSplitter_GetMinSize(aligned.splitter), TCustomSplitter_GetBeveled(aligned.splitter) != 0,
           TCustomSplitter_GetAutoSnap(aligned.splitter) != 0,
           TCustomSplitter_GetResizeAnchor(aligned.splitter), akLeft,
           TCustomSplitter_GetResizeStyle(aligned.splitter), rsUpdate);

    /* Tier 5(メニュー)。項目の Owner はフォームにし、親子関係は TMenuItem_Add で組む。
       menuRoot(メニューのルート項目)は LCL が内部で生成したもので、取得した時点で破棄通知の対象になる。 */
    mainMenu = TMainMenu_Create(form);
    menuRoot = TMenu_GetItems(mainMenu);
    fileMenu = TMenuItem_Create(form);
    TMenuItem_SetCaption(fileMenu, "&File");
    TMenuItem_Add(menuRoot, fileMenu);
    openItem = TMenuItem_Create(form);
    TMenuItem_SetCaption(openItem, "&Open");
    TMenuItem_SetShortCut(openItem, ShortCut_FromText("Ctrl+O"));
    TMenuItem_SetOnClick(openItem, OnMenuItemClick, NULL);
    TMenuItem_Add(fileMenu, openItem);
    TMenuItem_AddSeparator(fileMenu);
    exitItem = TMenuItem_Create(form);
    TMenuItem_SetCaption(exitItem, "E&xit");
    TMenuItem_SetOnClick(exitItem, OnExitItemClick, app);
    TMenuItem_Add(fileMenu, exitItem);
    TCustomForm_SetMenu(form, mainMenu);

    separator = TMenuItem_GetItem(fileMenu, 1);
    printf("Form Menu is mainMenu: %s, root Count=%d (expected 1), fileMenu Count=%d (expected 3)\n",
           TCustomForm_GetMenu(form) == mainMenu ? "yes" : "no",
           TMenuItem_GetCount(menuRoot), TMenuItem_GetCount(fileMenu));
    printf("fileMenu Parent is root: %s, separator IsLine=%d, separator Parent is fileMenu: %s\n",
           TMenuItem_GetParent(fileMenu) == menuRoot ? "yes" : "no",
           TMenuItem_IsLine(separator) != 0,
           TMenuItem_GetParent(separator) == fileMenu ? "yes" : "no");
    printf("openItem ShortCut=0x%04x (expected Ctrl+O = 0x%04x) Text=%s\n",
           (unsigned)TMenuItem_GetShortCut(openItem),
           (unsigned)ShortCut_Make('O', ssCtrl),
           ShortCut_ToText(TMenuItem_GetShortCut(openItem)));
    TMenuItem_Click(openItem);

    /* panel を右クリックすると開くメニュー。 */
    popupMenu = TPopupMenu_Create(form);
    popupItem = TMenuItem_Create(form);
    TMenuItem_SetCaption(popupItem, "Popup item");
    TMenuItem_SetOnClick(popupItem, OnMenuItemClick, NULL);
    TMenuItem_Add(TMenu_GetItems(popupMenu), popupItem);
    TPopupMenu_SetOnPopup(popupMenu, OnPopupMenuPopup, panel);
    TControl_SetPopupMenu(panel, popupMenu);
    printf("panel PopupMenu is popupMenu: %s, AutoPopup=%d\n",
           TControl_GetPopupMenu(panel) == popupMenu ? "yes" : "no",
           TPopupMenu_GetAutoPopup(popupMenu) != 0);

    /* Tier 2、1 バッチ目(TPageControl + TTabSheet)。tabSheet1 は VCL と同じく生成して PageControl を設定し、
       tabSheet2 は AddTabSheet で追加する(LCL が生成し、Owner は pageControl。返す時点で破棄通知の対象になる)。 */
    pageControl = Place(TPageControl_Create(form), form, 400, 200);
    TControl_SetWidth(pageControl, 220);
    TControl_SetHeight(pageControl, 160);
    tabSheet1 = TTabSheet_Create(form);
    TTabSheet_SetPageControl(tabSheet1, pageControl);
    TControl_SetCaption(tabSheet1, "Page 1");
    TControl_SetCaption(Place(TLabel_Create(form), tabSheet1, 10, 10), "On page 1");
    tabSheet2 = TPageControl_AddTabSheet(pageControl);
    TControl_SetCaption(tabSheet2, "Page 2");
    TControl_SetCaption(Place(TButton_Create(form), tabSheet2, 10, 10), "On page 2");
    TPageControl_SetActivePage(pageControl, tabSheet1);
    TPageControl_SetOnChange(pageControl, OnPageControlChange, NULL);
    TCustomTabControl_SetOnChanging(pageControl, OnPageControlChanging, NULL);
    printf("PageControl PageCount=%d (expected 2), ActivePage is tabSheet1: %s, GetPage(1) is tabSheet2: %s, "
           "tabSheet2 PageControl is pageControl: %s\n",
           TCustomTabControl_GetPageCount(pageControl),
           TPageControl_GetActivePage(pageControl) == tabSheet1 ? "yes" : "no",
           TPageControl_GetPage(pageControl, 1) == tabSheet2 ? "yes" : "no",
           TTabSheet_GetPageControl(tabSheet2) == pageControl ? "yes" : "no");

    /* Clear はすべてのページを外して、遅延破棄する(Application.ReleaseComponent)。
       Clear の時点ではまだ破棄されず、ここでは Owner(tempPageControl)の破棄と一緒に破棄される。 */
    tempPageControl = TPageControl_Create(form);
    TPageControl_AddTabSheet(tempPageControl);
    TPageControl_AddTabSheet(tempPageControl);
    {
        int freedBefore = freedCount;
        TPageControl_Clear(tempPageControl);
        printf("temp PageCount after Clear=%d (expected 0), freed right after Clear=%d (expected 0: deferred)\n",
               TCustomTabControl_GetPageCount(tempPageControl), freedCount - freedBefore);
        TComponent_Destroy(tempPageControl);
        printf("freed after destroying temp=%d (expected 3: 2 pages + temp)\n", freedCount - freedBefore);
    }

    /* Tier 2、2 バッチ目(TTreeView)。tabSheet1 の上に置く。ノードは TComponent ではないため、
       破棄は FreeNotify_SetCallback ではなく ItemFree_SetCallback(項目の破棄通知)で通知される。 */
    ItemFree_SetCallback(OnItemFreed, &itemsFreed);
    treeView = Place(TTreeView_Create(form), tabSheet1, 10, 30);
    TControl_SetWidth(treeView, 190);
    TControl_SetHeight(treeView, 95);
    treeItems = TCustomTreeView_GetItems(treeView);
    rootNode = TTreeNodes_Add(treeItems, NULL, "Root");
    childNode = TTreeNodes_AddChild(treeItems, rootNode, "Child");
    TTreeNodes_AddChild(treeItems, childNode, "Grandchild");
    root2Node = TTreeNodes_Add(treeItems, rootNode, "Root 2");
    TTreeNode_SetExpanded(rootNode, 1);
    TTreeView_SetOnChange(treeView, OnTreeViewChange, NULL);
    TTreeView_SetOnCollapsing(treeView, OnTreeViewCollapsing, NULL);
    printf("TreeView Count=%d (expected 4), root children=%d (expected 1), child Parent is root: %s, "
           "Grandchild Level=%d (expected 2), root next sibling is Root 2: %s, GetItem(3) is Root 2: %s\n",
           TTreeNodes_GetCount(treeItems), TTreeNode_GetCount(rootNode),
           TTreeNode_GetParent(childNode) == rootNode ? "yes" : "no",
           TTreeNode_GetLevel(TTreeNodes_FindNodeWithText(treeItems, "Grandchild")),
           TTreeNode_GetNextSibling(rootNode) == root2Node ? "yes" : "no",
           TTreeNodes_GetItem(treeItems, 3) == root2Node ? "yes" : "no");
    printf("node TreeView is treeView: %s\n", TTreeNode_GetTreeView(childNode) == treeView ? "yes" : "no");

    lockedNode = TTreeNodes_Add(treeItems, NULL, "Locked");
    TTreeNodes_AddChild(treeItems, lockedNode, "Inside");
    TTreeNode_Expand(lockedNode, 0);
    TTreeNode_Collapse(lockedNode, 0);
    printf("Locked Expanded after Collapse=%d (expected 1: OnCollapsing refused)\n",
           TTreeNode_GetExpanded(lockedNode) != 0);
    TTreeNode_Delete(lockedNode);
    printf("After Delete: Count=%d (expected 4), nodes freed=%d (expected 2)\n",
           TTreeNodes_GetCount(treeItems), itemsFreed);

    /* Tier 2、3 バッチ目(TListView)。tabSheet2 の上に、レポート表示で置く。項目・列も項目の破棄通知の対象。 */
    listView = Place(TListView_Create(form), tabSheet2, 10, 40);
    TControl_SetWidth(listView, 190);
    TControl_SetHeight(listView, 85);
    TListView_SetViewStyle(listView, vsReport);
    listColumns = TListView_GetColumns(listView);
    TListColumn_SetCaption(TListColumns_Add(listColumns), "Name");
    TListColumn_SetCaption(TListColumns_Add(listColumns), "Size");
    TListColumn_SetCaption(TListColumns_Add(listColumns), "Temp");
    listItems = TCustomListView_GetItems(listView);
    listItem = TListItems_Add(listItems);
    TListItem_SetCaption(listItem, "Beta");
    TStrings_Add(TListItem_GetSubItems(listItem), "20");
    listItem = TListItems_Add(listItems);
    TListItem_SetCaption(listItem, "Alpha");
    TStrings_Add(TListItem_GetSubItems(listItem), "10");
    TListItem_SetCaption(TListItems_Add(listItems), "Temp");
    TListView_SetOnDeletion(listView, OnListViewDeletion, &listDeletions);
    TListView_SetOnSelectItem(listView, OnListViewSelectItem, NULL);
    /* 表示前に設定した選択も有効(LCL 単体では効かないため DLL 側で補っている)。 */
    TCustomListView_SetItemIndex(listView, 1);
    printf("ListView Columns=%d Items=%d (expected 3/3), ItemIndex=%d (expected 1), Selected is Alpha: %s, Item(0) SubItems[0]=%s\n",
           TListColumns_GetCount(listColumns), TListItems_GetCount(listItems),
           TCustomListView_GetItemIndex(listView),
           TCustomListView_GetSelected(listView) == listItem ? "yes" : "no",
           TStrings_GetStrings(TListItem_GetSubItems(TListItems_GetItem(listItems, 0)), 0));
    /* SortColumn を先に設定する(既定の -1 のままでは並べ替えない)。 */
    TListView_SetSortColumn(listView, 0);
    TListView_SetSortType(listView, stText);
    printf("After sort: Item(0)=%s (expected Alpha)\n", TListItem_GetCaption(TListItems_GetItem(listItems, 0)));
    TListView_SetSortType(listView, stNone);
    {
        int freedBefore = itemsFreed;
        TListItem_Delete(TListItems_FindCaption(listItems, 0, "Temp", 0, 1, 0));
        TListColumns_Delete(listColumns, 2);
        printf("After deleting an item and a column: Items=%d Columns=%d (expected 2/2), OnDeletion=%d (expected 1), "
               "items freed=%d (expected 2)\n",
               TListItems_GetCount(listItems), TListColumns_GetCount(listColumns),
               listDeletions, itemsFreed - freedBefore);
    }

    /* Tier 2、4 バッチ目(TStringGrid)。pageControl の新しいページ "Grid" の上に置く。 */
    gridSheet = TTabSheet_Create(form);
    TTabSheet_SetPageControl(gridSheet, pageControl);
    TControl_SetCaption(gridSheet, "Grid");
    stringGrid = Place(TStringGrid_Create(form), gridSheet, 5, 5);
    TControl_SetWidth(stringGrid, 200);
    TControl_SetHeight(stringGrid, 120);
    TCustomDrawGrid_SetColCount(stringGrid, 2);
    TCustomDrawGrid_SetRowCount(stringGrid, 4);
    TCustomDrawGrid_SetFixedCols(stringGrid, 0);
    TCustomDrawGrid_SetOptions(stringGrid, TCustomDrawGrid_GetOptions(stringGrid) | goEditing);
    TCustomStringGrid_SetCells(stringGrid, 0, 0, "Name");
    TCustomStringGrid_SetCells(stringGrid, 1, 0, "Qty");
    TCustomStringGrid_SetCells(stringGrid, 0, 1, "Cherry");
    TCustomStringGrid_SetCells(stringGrid, 0, 2, "Apple");
    TCustomStringGrid_SetCells(stringGrid, 0, 3, "Banana");
    /* 列 0 の値で行を並べ替える(固定行は除く)。 */
    TCustomDrawGrid_SortColRow(stringGrid, 1, 0);
    TCustomDrawGrid_SetOnSelection(stringGrid, OnGridSelection, NULL);
    TCustomDrawGrid_SetSelection(stringGrid, 0, 2, 1, 3);
    TCustomDrawGrid_GetSelection(stringGrid, &selLeft, &selTop, &selRight, &selBottom);
    {
        /* 文字列を返す関数の戻り値は次の呼び出しで上書きされるため、並べて使うときはコピーする。 */
        char sorted[3][16];
        int r;
        for (r = 0; r < 3; ++r)
            snprintf(sorted[r], sizeof(sorted[r]), "%s", TCustomStringGrid_GetCells(stringGrid, 0, r + 1));
        printf("StringGrid ColCount/RowCount=%d/%d (expected 2/4), sorted: %s/%s/%s (expected Apple/Banana/Cherry), "
               "goEditing: %s, Selection=(%d,%d,%d,%d) (expected 0,2,1,3)\n",
               TCustomDrawGrid_GetColCount(stringGrid), TCustomDrawGrid_GetRowCount(stringGrid),
               sorted[0], sorted[1], sorted[2],
               (TCustomDrawGrid_GetOptions(stringGrid) & goEditing) ? "yes" : "no",
               (int)selLeft, (int)selTop, (int)selRight, (int)selBottom);
    }

    /* Tier 2、5 バッチ目(THeaderControl)。pageControl の新しいページ "Header" の上端に置く。 */
    headerSheet = TTabSheet_Create(form);
    TTabSheet_SetPageControl(headerSheet, pageControl);
    TControl_SetCaption(headerSheet, "Header");
    headerControl = THeaderControl_Create(form);
    TControl_SetParent(headerControl, headerSheet);
    TControl_SetAlign(headerControl, alTop);
    TCustomHeaderControl_SetDragReorder(headerControl, 1);
    sections = TCustomHeaderControl_GetSections(headerControl);
    {
        const char* texts[] = { "Temp", "Name", "Size" };
        int i;
        for (i = 0; i < 3; ++i)
        {
            obj_t section = THeaderSections_Add(sections);
            THeaderSection_SetText(section, texts[i]);
            THeaderSection_SetWidth(section, 60 + i * 10);
        }
    }
    {
        /* Delete したセクションは、その場で破棄通知が届く。 */
        int freedBefore = itemsFreed;
        obj_t size;
        THeaderSections_Delete(sections, 0);
        size = THeaderSections_GetItem(sections, 1);
        printf("HeaderControl Sections Count=%d (expected 2), freed by Delete: %d (expected 1), "
               "Size Left/Width=%d/%d (expected 70/80), OriginalIndex=%d (expected 1), DragReorder=%d (expected 1)\n",
               THeaderSections_GetCount(sections), itemsFreed - freedBefore,
               THeaderSection_GetLeft(size), THeaderSection_GetWidth(size),
               THeaderSection_GetOriginalIndex(size),
               TCustomHeaderControl_GetDragReorder(headerControl) != 0);
    }

    /* Tier 2、6 バッチ目(TToolBar・TToolButton)。pageControl の新しいページ "Tools" の上端に置く。
       ボタンは Parent をツールバーにすると末尾に追加される。 */
    toolsSheet = TTabSheet_Create(form);
    TTabSheet_SetPageControl(toolsSheet, pageControl);
    TControl_SetCaption(toolsSheet, "Tools");
    toolBar = TToolBar_Create(form);
    TControl_SetParent(toolBar, toolsSheet);
    TToolBar_SetShowCaptions(toolBar, 1);
    leftButton = TToolButton_Create(form);
    TControl_SetCaption(leftButton, "L");
    TToolButton_SetStyle(leftButton, tbsCheck);
    TToolButton_SetGrouped(leftButton, 1);
    TControl_SetParent(leftButton, toolBar);
    rightButton = TToolButton_Create(form);
    TControl_SetCaption(rightButton, "R");
    TToolButton_SetStyle(rightButton, tbsCheck);
    TToolButton_SetGrouped(rightButton, 1);
    TControl_SetParent(rightButton, toolBar);
    TToolButton_SetDown(leftButton, 1);
    TToolButton_SetDown(rightButton, 1);
    printf("ToolBar ButtonCount=%d (expected 2), GetButton(1) is rightButton: %s, rightButton Index=%d (expected 1), "
           "Grouped Down L/R=%d/%d (expected 0/1), Align=%d (expected alTop=%d), EdgeBorders=0x%x (expected ebTop=0x%x)\n",
           TToolBar_GetButtonCount(toolBar), TToolBar_GetButton(toolBar, 1) == rightButton ? "yes" : "no",
           TToolButton_GetIndex(rightButton),
           TToolButton_GetDown(leftButton) != 0, TToolButton_GetDown(rightButton) != 0,
           TControl_GetAlign(toolBar), alTop, TToolWindow_GetEdgeBorders(toolBar), ebTop);

    /* Tier 2、7 バッチ目(TCoolBar)。"Tools" ページのツールバーの下に置く。
       ウィンドウを持つコントロールの Parent をクールバーにすると LCL がバンドを追加し、コントロールを破棄すると LCL がバンドを削除する
       (この API を通らない削除でも、一度返したバンドには破棄通知が届く)。 */
    coolBar = TCoolBar_Create(form);
    TControl_SetParent(coolBar, toolsSheet);
    coolBands = TCustomCoolBar_GetBands(coolBar);
    {
        obj_t edit = TEdit_Create(form);
        obj_t autoBand;
        obj_t textBand;
        int freedBefore;
        TControl_SetParent(edit, coolBar);
        autoBand = TCoolBands_GetItem(coolBands, 0);
        TCoolBand_SetText(autoBand, "Edit");
        textBand = TCoolBands_Add(coolBands);
        TCoolBand_SetText(textBand, "Text");
        printf("CoolBar Bands Count=%d (expected 2), band 0 Control is edit: %s, FindBandIndex(edit)=%d (expected 0), Vertical=%d (expected 0)\n",
               TCoolBands_GetCount(coolBands), TCoolBand_GetControl(autoBand) == edit ? "yes" : "no",
               TCoolBands_FindBandIndex(coolBands, edit), TCustomCoolBar_GetVertical(coolBar) != 0);
        freedBefore = itemsFreed;
        TComponent_Destroy(edit);
        printf("After destroying edit: Bands Count=%d (expected 1), band 0 is textBand: %s, items freed: %d (expected 1)\n",
               TCoolBands_GetCount(coolBands), TCoolBands_GetItem(coolBands, 0) == textBand ? "yes" : "no",
               itemsFreed - freedBefore);
    }

    /* Tier 4(ダイアログ。docs/adr/0033)。TComponent なので Owner(フォーム)に破棄を任せる。
       モーダルのダイアログの Execute は閉じるまで戻らないため、ここでは呼ばない(C++ のテストの "Dialogs" メニューから試す)。 */
    {
        obj_t openDialog = TOpenDialog_Create(form);
        obj_t saveDialog = TSaveDialog_Create(form);
        obj_t dirDialog = TSelectDirectoryDialog_Create(form);
        obj_t colorDialog = TColorDialog_Create(form);
        obj_t fontDialog = TFontDialog_Create(form);
        obj_t findDialog = TFindDialog_Create(form);
        obj_t replaceDialog = TReplaceDialog_Create(form);
        obj_t dialogFont = TFontDialog_GetFont(fontDialog);

        TCommonDialog_SetTitle(openDialog, "Open");
        TFileDialog_SetFilter(openDialog, "Text|*.txt|All|*.*");
        TFileDialog_SetFilterIndex(openDialog, 2);
        TOpenDialog_SetOptions(openDialog, TOpenDialog_GetOptions(openDialog) | ofAllowMultiSelect);
        printf("internal TOpenDialog Title=%s Filter=%s FilterIndex=%d (expected 2), Options=0x%x (expected 0x%x), Files Count=%d (expected 0)\n",
               TCommonDialog_GetTitle(openDialog), TFileDialog_GetFilter(openDialog), TFileDialog_GetFilterIndex(openDialog),
               TOpenDialog_GetOptions(openDialog), ofEnableSizing | ofViewDetail | ofAllowMultiSelect,
               TStrings_GetCount(TFileDialog_GetFiles(openDialog)));
        TFileDialog_SetDefaultExt(saveDialog, "txt");
        TFileDialog_SetInitialDir(dirDialog, ".");
        printf("internal TSaveDialog DefaultExt=%s (expected .txt), TSelectDirectoryDialog InitialDir=%s\n",
               TFileDialog_GetDefaultExt(saveDialog), TFileDialog_GetInitialDir(dirDialog));

        TColorDialog_SetColor(colorDialog, 0x00FF00);
        printf("internal TColorDialog Color=%06X (expected 00FF00), Options=%u (expected cdFullOpen=1), CustomColors Count=%d (expected 20)\n",
               (unsigned)TColorDialog_GetColor(colorDialog), TColorDialog_GetOptions(colorDialog),
               TStrings_GetCount(TColorDialog_GetCustomColors(colorDialog)));

        /* TFont の Style・Assign と、TControl の Color・Font。フォントの代入は内容のコピー。 */
        TFont_SetName(dialogFont, "Arial");
        TFont_SetStyle(dialogFont, fsBold | fsStrikeOut);
        TFontDialog_SetOptions(fontDialog, TFontDialog_GetOptions(fontDialog) | fdLimitSize);
        TFontDialog_SetMaxFontSize(fontDialog, 30);
        TControl_SetFont(label, dialogFont);
        TFont_SetStyle(dialogFont, 0);
        printf("internal TFontDialog Options=0x%x (expected 0x%x), MaxFontSize=%d (expected 30); label Font %s Style=0x%x (expected Arial 0x9)\n",
               TFontDialog_GetOptions(fontDialog), fdEffects | fdLimitSize, TFontDialog_GetMaxFontSize(fontDialog),
               TFont_GetName(TControl_GetFont(label)), TFont_GetStyle(TControl_GetFont(label)));
        TFont_Assign(dialogFont, TControl_GetFont(label));
        TControl_SetColor(panel, clYellow);
        printf("internal TFont_Assign: Style=0x%x (expected 0x9); panel Color=%06X (expected 00FFFF), button Color is clDefault: %s\n",
               TFont_GetStyle(dialogFont), (unsigned)TControl_GetColor(panel), TControl_GetColor(button) == clDefault ? "yes" : "no");

        TFindDialog_SetFindText(findDialog, "needle");
        TFindDialog_SetOptions(findDialog, frDown | frMatchCase);
        TFindDialog_SetReplaceText(replaceDialog, "thread");
        printf("internal TFindDialog FindText=%s Options=0x%x (expected 0x21), TReplaceDialog ReplaceText=%s\n",
               TFindDialog_GetFindText(findDialog), TFindDialog_GetOptions(findDialog), TFindDialog_GetReplaceText(replaceDialog));
    }

    printf("Running (click the button, then close the window twice: the first close is blocked)...\n");
    fflush(stdout);
    /* MainForm を表示してメッセージループに入り、MainForm が閉じられると戻る。 */
    TApplication_Run(app);
    printf("Run returned. Terminated=%d\n", TApplication_GetTerminated(app) != 0);

    /* Application が所有するフォーム(と、フォームが所有するコントロール)をまとめて破棄する。
       呼ばなくても DLL の切り離し時に LCL が破棄するが、そのときは破棄通知が呼ばれない。 */
    TComponent_DestroyComponents(app);
    printf("Clicks: %d, Freed components: %d (expected 83: form + 75 owned + 7 created inside LCL: 2 menu roots, a separator, 3 AddTabSheet pages and an EditLabel)\n", clickCount, freedCount);
    /* ツリービュー・リストビュー・ヘッダーコントロールの破棄に伴って、残りのノード(4 つ)・リストビューの項目(2 つ)と列(2 つ)・
       セクション(2 つ)・バンド(1 つ)も破棄通知が届く。 */
    printf("Items freed: %d (expected 17: tree 2 deleted + 4 with the tree view, list 1 item + 1 column deleted "
           "+ 2 items + 2 columns with the list view, header 1 section deleted + 2 with the header control, cool bar 1 band deleted + 1 with the cool bar)\n", itemsFreed);

    printf("OK\n");
    return 0;
}
