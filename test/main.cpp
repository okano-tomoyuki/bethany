#include <cstdio>
#include <string>

#include "no_vcl.hpp"

using namespace no_vcl;

namespace
{

int g_destroyedLabels = 0;

// ラッパーが実際に delete されたかを数えるためのラベル。
// コンポーネントの派生クラスは、ヒープ生成を強制するためデストラクタを protected にする。
class TTracedLabel : public TLabel
{
public:
    explicit TTracedLabel(TComponent* AOwner) : TLabel(AOwner) {}

protected:
    ~TTracedLabel() override
    {
        ++g_destroyedLabels;
        std::printf("~TTracedLabel: destroyed labels=%d\n", g_destroyedLabels);
        std::fflush(stdout);
    }
};

// C++Builder のフォームユニットと同じく、フォームはグローバルなポインタ変数で持つ。
class TMainForm;
TMainForm* Form1 = nullptr;

// 閉じ方の確認用のサブフォーム。
//   - Release me ボタン: 自身のボタンのハンドラの中から Release() する(Free() と違い安全)。
//   - ウィンドウを閉じる: OnClose で Action = caFree にし、LCL に Release させる。
class TSubForm : public TForm
{
public:
    TButton* ReleaseButton;

    explicit TSubForm(TComponent* AOwner) : TForm(AOwner)
    {
        Caption = "Sub form";
        Width = 260;
        Height = 120;

        ReleaseButton = new TButton(this);
        ReleaseButton->Parent = this;
        ReleaseButton->Caption = "Release me";
        ReleaseButton->Left = 20;
        ReleaseButton->Top = 20;
        ReleaseButton->Width = 120;
        ReleaseButton->OnClick = [this](TObject*) {
            std::printf("TSubForm: Release() from its own button\n");
            std::fflush(stdout);
            Release();
        };

        // new で直接生成したフォームの OnCreate は、最初に表示される直前に呼ばれる。
        OnCreate = [this](TObject* Sender) {
            std::printf("TSubForm OnCreate: Sender is this: %s\n", Sender == this ? "yes" : "no");
            std::fflush(stdout);
        };
        OnClose = [](TObject*, TCloseAction& Action) {
            std::printf("TSubForm OnClose: default Action=%d, set caFree\n", (int)Action);
            std::fflush(stdout);
            Action = caFree;
        };
        OnActivate = [](TObject*) { std::printf("TSubForm OnActivate\n"); std::fflush(stdout); };
        OnDeactivate = [](TObject*) { std::printf("TSubForm OnDeactivate\n"); std::fflush(stdout); };
        OnHide = [](TObject*) { std::printf("TSubForm OnHide\n"); std::fflush(stdout); };
        OnDestroy = [this](TObject*) {
            std::printf("TSubForm OnDestroy: ReleaseButton->Caption=%s\n", std::string(ReleaseButton->Caption).c_str());
            std::fflush(stdout);
        };
    }

protected:
    ~TSubForm() override
    {
        std::printf("~TSubForm\n");
        std::fflush(stdout);
    }
};

// デザイナーが生成することを想定した形のフォーム。
// コントロールは生ポインタメンバとして持ち、コンストラクタ本体で new する(破棄は Owner に任せる)。
// イベントハンドラはメンバ関数にし、[this] のラムダで割り当てる。
class TMainForm : public TForm
{
public:
    TButton*      Button1;
    TLabel*       Label1;
    TEdit*        Edit1;
    TCheckBox*    CheckBox1;
    TRadioButton* RadioButton1;
    TRadioButton* RadioButton2;
    TPanel*       Panel1;
    TButton*      PanelButton;
    TGroupBox*    GroupBox1;
    TComboBox*    ComboBox1;
    TListBox*     ListBox1;
    TMemo*        Memo1;
    TTracedLabel* TickLabel;
    TTimer*       Timer1;
    TPaintBox*    PaintBox1;
    TScrollBox*   ScrollBox1;
    TButton*      ScrolledButton;
    TToggleBox*   ToggleBox1;
    TBevel*       Bevel1;
    TShape*       Shape1;
    TStaticText*  StaticText1;
    TStatusBar*   StatusBar1 = nullptr;
    TScrollBar*   ScrollBar1;
    TTrackBar*    TrackBar1;
    TProgressBar* ProgressBar1;
    TEdit*        UpDownEdit;
    TUpDown*      UpDown1;
    TRadioGroup*  RadioGroup1;
    TCheckGroup*  CheckGroup1;
    TCheckListBox* CheckListBox1;
    TSpeedButton* SpeedButton1;
    TBitBtn*      BitBtn1;
    TFloatSpinEdit* FloatSpinEdit1;
    TSpinEdit*      SpinEdit1;
    TMaskEdit*      MaskEdit1;
    TTabControl*    TabControl1;
    TPanel*         LayoutPanel;
    TPanel*         AlignTopPanel;
    TPanel*         AlignLeftPanel;
    TSplitter*      Splitter1;
    TPanel*         AlignClientPanel;
    TMainMenu*      MainMenu1;
    TMenuItem*      FileMenu;
    TMenuItem*      FileNewItem;
    TMenuItem*      FileExitItem;
    TMenuItem*      ViewMenu;
    TMenuItem*      ViewStatusBarItem;
    TMenuItem*      ViewSmallItem;
    TMenuItem*      ViewLargeItem;
    TPopupMenu*     PopupMenu1;
    TMenuItem*      PopupHelloItem;
    TPageControl*   PageControl1;
    TTabSheet*      TabSheet1;
    TTabSheet*      TabSheet2;
    TTabSheet*      TabSheet3;
    TTreeView*      TreeView1;
    TTreeNode*      RootNode;
    TTreeNode*      Child1Node;
    TTreeNode*      Child2Node;
    TTreeNode*      GrandchildNode;
    TTreeNode*      Root2Node;
    int             deletedNodes_ = 0;
    TListView*      ListView1;
    TListItem*      AlphaItem;
    TListItem*      BetaItem;
    TListItem*      GammaItem;
    int             deletedListItems_ = 0;
    TTabSheet*      GridSheet;
    TStringGrid*    StringGrid1;
    TDrawGrid*      DrawGrid1;
    int             drawnCells_ = 0;
    TTabSheet*      HeaderSheet;
    THeaderControl* HeaderControl1;
    TTabSheet*      ToolsSheet;
    TToolBar*       ToolBar1;
    TToolButton*    NewToolButton;
    TToolButton*    SepToolButton;
    TToolButton*    BoldToolButton;
    TToolButton*    LeftToolButton;
    TToolButton*    RightToolButton;
    TToolButton*    DropToolButton;

    // C++Builder と同じく Owner を受け取り、TForm に渡す(Application->CreateForm が Application を渡す)。
    explicit TMainForm(TComponent* AOwner) : TForm(AOwner)
    {
        Caption = "no_vcl C++ wrapper";
        Width = 640;
        Height = 930;

        Button1 = new TButton(this);
        Button1->Parent = this;
        Button1->Caption = "Click me";
        Button1->Left = 20;
        Button1->Top = 20;
        Button1->Width = 100;
        Button1->Height = 30;
        Button1->OnClick = [this](TObject* Sender) { Button1Click(Sender); };
        Button1->OnDblClick = [this](TObject* Sender) { Button1DblClick(Sender); };
        Button1->OnMouseDown = [this](TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y) {
            Button1MouseDown(Sender, Button, Shift, X, Y);
        };
        Button1->OnMouseUp = [this](TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y) {
            Button1MouseUp(Sender, Button, Shift, X, Y);
        };
        Button1->OnMouseEnter = [this](TObject* Sender) { Button1MouseEnter(Sender); };
        Button1->OnMouseLeave = [this](TObject* Sender) { Button1MouseLeave(Sender); };

        Label1 = new TLabel(this);
        Label1->Parent = this;
        Label1->Caption = "Label text";
        Label1->Left = 20;
        Label1->Top = 60;

        Edit1 = new TEdit(this);
        Edit1->Parent = this;
        Edit1->Text = "Edit me";
        Edit1->Left = 20;
        Edit1->Top = 90;
        Edit1->Width = 150;
        Edit1->OnChange = [this](TObject* Sender) { TextChange(Sender); };
        Edit1->OnKeyDown = [this](TObject* Sender, int& Key, TShiftState Shift) { Edit1KeyDown(Sender, Key, Shift); };
        Edit1->OnKeyPress = [this](TObject* Sender, char& Key) { Edit1KeyPress(Sender, Key); };

        CheckBox1 = new TCheckBox(this);
        CheckBox1->Parent = this;
        CheckBox1->Caption = "Check me";
        CheckBox1->Left = 20;
        CheckBox1->Top = 130;
        CheckBox1->OnClick = [this](TObject* Sender) { CheckBox1Click(Sender); };

        // 2つのラジオボタンで同じハンドラを共有し、Sender でどちらが押されたかを区別する。
        RadioButton1 = new TRadioButton(this);
        RadioButton1->Parent = this;
        RadioButton1->Caption = "Option A";
        RadioButton1->Left = 20;
        RadioButton1->Top = 160;
        RadioButton1->Checked = true;
        RadioButton1->OnClick = [this](TObject* Sender) { RadioButtonClick(Sender); };

        RadioButton2 = new TRadioButton(this);
        RadioButton2->Parent = this;
        RadioButton2->Caption = "Option B";
        RadioButton2->Left = 20;
        RadioButton2->Top = 190;
        RadioButton2->OnClick = RadioButton1->OnClick;

        Panel1 = new TPanel(this);
        Panel1->Parent = this;
        Panel1->Left = 220;
        Panel1->Top = 20;
        Panel1->Width = 180;
        Panel1->Height = 60;

        // Panel の中にボタンを置く(Parent が TWinControl* なので Panel も親にできる)。
        PanelButton = new TButton(this);
        PanelButton->Parent = Panel1;
        PanelButton->Caption = "In panel";
        PanelButton->Left = 10;
        PanelButton->Top = 15;
        PanelButton->OnClick = [this](TObject* Sender) { PanelButtonClick(Sender); };

        GroupBox1 = new TGroupBox(this);
        GroupBox1->Parent = this;
        GroupBox1->Caption = "Group";
        GroupBox1->Left = 220;
        GroupBox1->Top = 90;
        GroupBox1->Width = 180;
        GroupBox1->Height = 60;

        ComboBox1 = new TComboBox(this);
        ComboBox1->Parent = this;
        ComboBox1->ItemsAdd("Combo A");
        ComboBox1->ItemsAdd("Combo B");
        ComboBox1->ItemsAdd("Combo C");
        ComboBox1->ItemIndex = 0;
        ComboBox1->Left = 220;
        ComboBox1->Top = 160;
        ComboBox1->Width = 150;
        ComboBox1->OnChange = [this](TObject* Sender) { TextChange(Sender); };

        ListBox1 = new TListBox(this);
        ListBox1->Parent = this;
        ListBox1->ItemsAdd("List 1");
        ListBox1->ItemsAdd("List 2");
        ListBox1->ItemsAdd("List 3");
        ListBox1->Left = 220;
        ListBox1->Top = 190;
        ListBox1->Width = 150;
        ListBox1->Height = 80;
        ListBox1->OnClick = [this](TObject* Sender) { ListBox1Click(Sender); };

        // TMemo は TCustomEdit の派生なので、Edit1 と同じ TextChange を共有できる。
        Memo1 = new TMemo(this);
        Memo1->Parent = this;
        Memo1->LinesAdd("Memo line 1");
        Memo1->LinesAdd("Memo line 2");
        Memo1->Left = 220;
        Memo1->Top = 280;
        Memo1->Width = 150;
        Memo1->Height = 80;
        Memo1->OnChange = Edit1->OnChange;

        TickLabel = new TTracedLabel(this);
        TickLabel->Parent = this;
        TickLabel->Caption = "Tick: 0";
        TickLabel->Left = 20;
        TickLabel->Top = 230;

        Timer1 = new TTimer(this);
        Timer1->Interval = 500;
        Timer1->OnTimer = [this](TObject* Sender) { Timer1Timer(Sender); };
        Timer1->Enabled = true;

        PaintBox1 = new TPaintBox(this);
        PaintBox1->Parent = this;
        PaintBox1->Left = 400;
        PaintBox1->Top = 20;
        PaintBox1->Width = 220;
        PaintBox1->Height = 130;
        PaintBox1->OnPaint = [this](TObject* Sender) { PaintBox1Paint(Sender); };

        OpenSubButton = new TButton(this);
        OpenSubButton->Parent = this;
        OpenSubButton->Caption = "Open sub";
        OpenSubButton->Left = 400;
        OpenSubButton->Top = 170;
        OpenSubButton->OnClick = [this](TObject* Sender) { OpenSubButtonClick(Sender); };

        // docs/component-coverage.md の Tier 1 で追加したコントロール(1 バッチ目)。
        ScrollBox1 = new TScrollBox(this);
        ScrollBox1->Parent = this;
        ScrollBox1->Left = 20;
        ScrollBox1->Top = 380;
        ScrollBox1->Width = 180;
        ScrollBox1->Height = 50;

        // TScrollBox もウィンドウを持つコントロールなので、Parent として子を配置できる。
        ScrolledButton = new TButton(this);
        ScrolledButton->Parent = ScrollBox1;
        ScrolledButton->Caption = "Inside ScrollBox";
        ScrolledButton->Left = 10;
        ScrolledButton->Top = 10;
        ScrolledButton->Width = 140;

        ToggleBox1 = new TToggleBox(this);
        ToggleBox1->Parent = this;
        ToggleBox1->Caption = "Toggle me";
        ToggleBox1->Left = 220;
        ToggleBox1->Top = 380;
        ToggleBox1->OnClick = [this](TObject* Sender) { ToggleBox1Click(Sender); };

        Bevel1 = new TBevel(this);
        Bevel1->Parent = this;
        Bevel1->Left = 340;
        Bevel1->Top = 380;
        Bevel1->Width = 100;
        Bevel1->Height = 50;
        Bevel1->Shape = bsFrame;
        Bevel1->Style = bsRaised;

        Shape1 = new TShape(this);
        Shape1->Parent = this;
        Shape1->Left = 460;
        Shape1->Top = 380;
        Shape1->Width = 60;
        Shape1->Height = 50;
        Shape1->Shape = stEllipse;
        Shape1->Brush.Color = clYellow;
        Shape1->Pen.Color = clBlue;

        StaticText1 = new TStaticText(this);
        StaticText1->Parent = this;
        StaticText1->Caption = "Static text";
        StaticText1->Left = 20;
        StaticText1->Top = 440;
        StaticText1->Width = 150;
        StaticText1->BorderStyle = sbsSunken;

        // Tier 1、3 バッチ目(範囲・数値系のコントロール)。
        ScrollBar1 = new TScrollBar(this);
        ScrollBar1->Parent = this;
        ScrollBar1->Left = 20;
        ScrollBar1->Top = 480;
        ScrollBar1->Width = 150;
        ScrollBar1->Min = 0;
        ScrollBar1->Max = 100;
        ScrollBar1->Position = 30;
        ScrollBar1->OnChange = [this](TObject* Sender) { ScrollBar1Change(Sender); };

        TrackBar1 = new TTrackBar(this);
        TrackBar1->Parent = this;
        TrackBar1->Left = 190;
        TrackBar1->Top = 480;
        TrackBar1->Width = 150;
        TrackBar1->Min = 0;
        TrackBar1->Max = 10;
        TrackBar1->Position = 5;
        TrackBar1->OnChange = [this](TObject* Sender) { TrackBar1Change(Sender); };

        ProgressBar1 = new TProgressBar(this);
        ProgressBar1->Parent = this;
        ProgressBar1->Left = 360;
        ProgressBar1->Top = 480;
        ProgressBar1->Width = 150;
        ProgressBar1->Min = 0;
        ProgressBar1->Max = 100;
        ProgressBar1->Position = 42;

        UpDownEdit = new TEdit(this);
        UpDownEdit->Parent = this;
        UpDownEdit->Text = "3";
        UpDownEdit->Left = 20;
        UpDownEdit->Top = 510;
        UpDownEdit->Width = 60;

        UpDown1 = new TUpDown(this);
        UpDown1->Parent = this;
        UpDown1->Left = 80;
        UpDown1->Top = 510;
        UpDown1->Min = 0;
        UpDown1->Max = 10;
        UpDown1->Position = 3;
        UpDown1->Increment = 1;
        UpDown1->Associate = UpDownEdit;

        // Tier 1、4 バッチ目(Items を持つグループ・リスト系のコントロール)。
        RadioGroup1 = new TRadioGroup(this);
        RadioGroup1->Parent = this;
        RadioGroup1->Caption = "RadioGroup1";
        RadioGroup1->Left = 20;
        RadioGroup1->Top = 550;
        RadioGroup1->Width = 180;
        RadioGroup1->Height = 90;
        RadioGroup1->ItemsAdd("Option A");
        RadioGroup1->ItemsAdd("Option B");
        RadioGroup1->ItemsAdd("Option C");
        RadioGroup1->ItemIndex = 1;
        RadioGroup1->OnClick = [this](TObject* Sender) { RadioGroup1Click(Sender); };

        CheckGroup1 = new TCheckGroup(this);
        CheckGroup1->Parent = this;
        CheckGroup1->Caption = "CheckGroup1";
        CheckGroup1->Left = 210;
        CheckGroup1->Top = 550;
        CheckGroup1->Width = 180;
        CheckGroup1->Height = 90;
        CheckGroup1->ItemsAdd("Feature X");
        CheckGroup1->ItemsAdd("Feature Y");
        CheckGroup1->ItemsAdd("Feature Z");
        CheckGroup1->Checked[0] = true;
        CheckGroup1->Checked[2] = true;

        CheckListBox1 = new TCheckListBox(this);
        CheckListBox1->Parent = this;
        CheckListBox1->Left = 400;
        CheckListBox1->Top = 550;
        CheckListBox1->Width = 180;
        CheckListBox1->Height = 90;
        CheckListBox1->ItemsAdd("Item 1");
        CheckListBox1->ItemsAdd("Item 2");
        CheckListBox1->ItemsAdd("Item 3");
        CheckListBox1->Checked[1] = true;
        CheckListBox1->OnClickCheck = [this](TObject* Sender) { CheckListBox1ClickCheck(Sender); };

        // Tier 1、5 バッチ目(ボタンの派生)。
        SpeedButton1 = new TSpeedButton(this);
        SpeedButton1->Parent = this;
        SpeedButton1->Caption = "Speed";
        SpeedButton1->Left = 20;
        SpeedButton1->Top = 650;
        SpeedButton1->Width = 80;
        SpeedButton1->GroupIndex = 1;
        SpeedButton1->OnClick = [this](TObject* Sender) { SpeedButton1Click(Sender); };

        BitBtn1 = new TBitBtn(this);
        BitBtn1->Parent = this;
        BitBtn1->Left = 120;
        BitBtn1->Top = 650;
        BitBtn1->Kind = bkOK;

        // Tier 1、6 バッチ目(数値・書式付き Edit)。
        FloatSpinEdit1 = new TFloatSpinEdit(this);
        FloatSpinEdit1->Parent = this;
        FloatSpinEdit1->Left = 20;
        FloatSpinEdit1->Top = 690;
        FloatSpinEdit1->Width = 100;
        FloatSpinEdit1->MinValue = 0.0;
        FloatSpinEdit1->MaxValue = 10.0;
        FloatSpinEdit1->Increment = 0.5;
        FloatSpinEdit1->DecimalPlaces = 1;
        FloatSpinEdit1->Value = 2.5;

        SpinEdit1 = new TSpinEdit(this);
        SpinEdit1->Parent = this;
        SpinEdit1->Left = 130;
        SpinEdit1->Top = 690;
        SpinEdit1->Width = 100;
        SpinEdit1->MinValue = 0;
        SpinEdit1->MaxValue = 100;
        SpinEdit1->Increment = 5;
        SpinEdit1->Value = 42;

        MaskEdit1 = new TMaskEdit(this);
        MaskEdit1->Parent = this;
        MaskEdit1->Left = 240;
        MaskEdit1->Top = 690;
        MaskEdit1->Width = 100;
        MaskEdit1->EditMask = "000-0000;1;_";

        // Tier 1、7 バッチ目(最後のバッチ)。ページ付きの TPageControl/TTabSheet は今回見送る。
        TabControl1 = new TTabControl(this);
        TabControl1->Parent = this;
        TabControl1->Left = 20;
        TabControl1->Top = 730;
        TabControl1->Width = 300;
        TabControl1->Height = 90;
        TabControl1->TabsAdd("Tab A");
        TabControl1->TabsAdd("Tab B");
        TabControl1->TabsAdd("Tab C");
        TabControl1->TabIndex = 0;
        TabControl1->OnChange = [this](TObject* Sender) { TabControl1Change(Sender); };

        // TStatusBar も他のコントロールと同じくコンストラクタの中で生成してよい
        // (ADR 0015 の問題は DLL 側で回避済み)。
        StatusBar1 = new TStatusBar(this);
        StatusBar1->Parent = this;
        StatusBar1->SimpleText = "Ready";

        // TControl.Align と TSplitter。LayoutPanel の中を、上端の alTop、左の alLeft + Splitter、残りの alClient で分ける。
        // alLeft 同士は Left の小さい順に並ぶため、Splitter(既定の Align が alLeft)が alLeft のパネルの右に来るよう、
        // Parent より先に Left をパネルの幅より大きくしておく。
        LayoutPanel = new TPanel(this);
        LayoutPanel->Parent = this;
        LayoutPanel->Left = 340;
        LayoutPanel->Top = 730;
        LayoutPanel->Width = 280;
        LayoutPanel->Height = 90;
        LayoutPanel->Caption = "";

        AlignTopPanel = new TPanel(this);
        AlignTopPanel->Parent = LayoutPanel;
        AlignTopPanel->Align = alTop;
        AlignTopPanel->Height = 20;
        AlignTopPanel->Caption = "alTop";

        AlignLeftPanel = new TPanel(this);
        AlignLeftPanel->Parent = LayoutPanel;
        AlignLeftPanel->Align = alLeft;
        AlignLeftPanel->Width = 80;
        AlignLeftPanel->Caption = "alLeft";

        Splitter1 = new TSplitter(this);
        Splitter1->Left = 100;
        Splitter1->Parent = LayoutPanel;
        Splitter1->MinSize = 40;
        Splitter1->Beveled = true;
        Splitter1->OnMoved = [this](TObject* Sender) { Splitter1Moved(Sender); };

        AlignClientPanel = new TPanel(this);
        AlignClientPanel->Parent = LayoutPanel;
        AlignClientPanel->Align = alClient;
        AlignClientPanel->Caption = "alClient";

        // Tier 5(メニュー)。項目の Owner はフォームにし、親子関係は Add で組む。
        // MainMenu1->Items はメニューのルート項目で、LCL が内部で生成したもの(初回アクセス時にラッパーができる)。
        MainMenu1 = new TMainMenu(this);

        FileMenu = new TMenuItem(this);
        FileMenu->Caption = "&File";
        MainMenu1->Items->Add(FileMenu);

        FileNewItem = new TMenuItem(this);
        FileNewItem->Caption = "&New";
        FileNewItem->ShortCut = ShortCut('N', ssCtrl);
        FileNewItem->OnClick = [this](TObject* Sender) { FileNewItemClick(Sender); };
        FileMenu->Add(FileNewItem);

        FileMenu->AddSeparator();

        FileExitItem = new TMenuItem(this);
        FileExitItem->Caption = "E&xit";
        FileExitItem->OnClick = [](TObject*) { Application->Terminate(); };
        FileMenu->Add(FileExitItem);

        ViewMenu = new TMenuItem(this);
        ViewMenu->Caption = "&View";
        MainMenu1->Items->Add(ViewMenu);

        ViewStatusBarItem = new TMenuItem(this);
        ViewStatusBarItem->Caption = "&Status bar";
        ViewStatusBarItem->AutoCheck = true;
        ViewStatusBarItem->Checked = true;
        ViewStatusBarItem->OnClick = [this](TObject*) { StatusBar1->Visible = (bool)ViewStatusBarItem->Checked; };
        ViewMenu->Add(ViewStatusBarItem);

        ViewMenu->AddSeparator();

        // 同じ GroupIndex の RadioItem は、どれか 1 つだけが Checked になる。
        ViewSmallItem = new TMenuItem(this);
        ViewSmallItem->Caption = "S&mall";
        ViewSmallItem->RadioItem = true;
        ViewSmallItem->GroupIndex = 1;
        ViewSmallItem->AutoCheck = true;
        ViewSmallItem->Checked = true;
        ViewMenu->Add(ViewSmallItem);

        ViewLargeItem = new TMenuItem(this);
        ViewLargeItem->Caption = "&Large";
        ViewLargeItem->RadioItem = true;
        ViewLargeItem->GroupIndex = 1;
        ViewLargeItem->AutoCheck = true;
        ViewMenu->Add(ViewLargeItem);

        Menu = MainMenu1;

        // Panel1 を右クリックすると開くメニュー。
        PopupMenu1 = new TPopupMenu(this);
        PopupHelloItem = new TMenuItem(this);
        PopupHelloItem->Caption = "Say hello";
        PopupHelloItem->OnClick = [](TObject*) { std::printf("PopupHelloItem clicked\n"); std::fflush(stdout); };
        PopupMenu1->Items->Add(PopupHelloItem);
        PopupMenu1->OnPopup = [this](TObject* Sender) { PopupMenu1Popup(Sender); };
        Panel1->PopupMenu = PopupMenu1;

        // Tier 2、1 バッチ目(TPageControl + TTabSheet)。
        PageControl1 = new TPageControl(this);
        PageControl1->Parent = this;
        PageControl1->Left = 400;
        PageControl1->Top = 200;
        PageControl1->Width = 220;
        PageControl1->Height = 160;

        // VCL と同じく、TTabSheet を生成して PageControl を設定する。
        TabSheet1 = new TTabSheet(this);
        TabSheet1->PageControl = PageControl1;
        TabSheet1->Caption = "Page 1";
        TLabel* pageLabel = new TLabel(this);
        pageLabel->Parent = TabSheet1;
        pageLabel->Left = 10;
        pageLabel->Top = 10;
        pageLabel->Caption = "On page 1";

        // AddTabSheet のページは LCL が生成する(ラッパーは初回の取得時に作られる)。
        TabSheet2 = PageControl1->AddTabSheet();
        TabSheet2->Caption = "Page 2";
        TButton* pageButton = new TButton(this);
        pageButton->Parent = TabSheet2;
        pageButton->Left = 10;
        pageButton->Top = 10;
        pageButton->Caption = "On page 2";

        TabSheet3 = new TTabSheet(this);
        TabSheet3->PageControl = PageControl1;
        TabSheet3->Caption = "Hidden";
        TabSheet3->TabVisible = false;

        PageControl1->ActivePage = TabSheet1;
        PageControl1->OnChanging = [](TObject*, bool& AllowChange) {
            std::printf("PageControl1Changing (AllowChange=%d)\n", AllowChange);
            std::fflush(stdout);
        };
        PageControl1->OnChange = [this](TObject* Sender) { PageControl1Change(Sender); };
        // 表示前に設定した ListView1 の選択が、ウィンドウハンドルができた後も保たれていることの確認を兼ねる。
        TabSheet2->OnShow = [this](TObject*) {
            TListItem* selected = ListView1->Selected;
            std::printf("TabSheet2Show: ListView1->Selected=%s (expected Beta)\n",
                        selected ? std::string(selected->Caption).c_str() : "(none)");
            std::fflush(stdout);
        };

        // Tier 2、2 バッチ目(TTreeView)。TabSheet1 の上に置く。
        TreeView1 = new TTreeView(this);
        TreeView1->Parent = TabSheet1;
        TreeView1->Left = 10;
        TreeView1->Top = 30;
        TreeView1->Width = 190;
        TreeView1->Height = 95;
        RootNode = TreeView1->Items->Add(nullptr, "Root");
        Child1Node = TreeView1->Items->AddChild(RootNode, "Child 1");
        Child2Node = TreeView1->Items->AddChild(RootNode, "Child 2");
        GrandchildNode = TreeView1->Items->AddChild(Child1Node, "Grandchild");
        Root2Node = TreeView1->Items->Add(RootNode, "Root 2");
        RootNode->Expanded = true;
        TreeView1->OnChange = [](TObject*, TTreeNode* Node) {
            std::printf("TreeView1Change: %s\n", Node ? std::string(Node->Text).c_str() : "(none)");
            std::fflush(stdout);
        };
        TreeView1->OnChanging = [](TObject*, TTreeNode* Node, bool& AllowChange) {
            std::printf("TreeView1Changing: to %s (AllowChange=%d)\n", std::string(Node->Text).c_str(), AllowChange);
            std::fflush(stdout);
        };
        TreeView1->OnExpanded = [](TObject*, TTreeNode* Node) {
            std::printf("TreeView1Expanded: %s\n", std::string(Node->Text).c_str());
            std::fflush(stdout);
        };
        // 名前が "Locked" のノードは折りたためないようにする。
        TreeView1->OnCollapsing = [](TObject*, TTreeNode* Node, bool& AllowCollapse) {
            if (std::string(Node->Text) == "Locked")
                AllowCollapse = false;
        };
        TreeView1->OnDeletion = [this](TObject*, TTreeNode*) { ++deletedNodes_; };

        // Tier 2、3 バッチ目(TListView)。TabSheet2 の上に、レポート表示(列見出し付き)で置く。
        ListView1 = new TListView(this);
        ListView1->Parent = TabSheet2;
        ListView1->Left = 10;
        ListView1->Top = 40;
        ListView1->Width = 190;
        ListView1->Height = 85;
        ListView1->ViewStyle = vsReport;
        ListView1->RowSelect = true;
        ListView1->Checkboxes = true;
        TListColumn* nameColumn = ListView1->Columns->Add();
        nameColumn->Caption = "Name";
        nameColumn->Width = 90;
        TListColumn* sizeColumn = ListView1->Columns->Add();
        sizeColumn->Caption = "Size";
        sizeColumn->Width = 60;
        sizeColumn->Alignment = taRightJustify;
        AlphaItem = ListView1->Items->Add();
        AlphaItem->Caption = "Alpha";
        AlphaItem->SubItemsAdd("10");
        BetaItem = ListView1->Items->Add();
        BetaItem->Caption = "Beta";
        BetaItem->SubItemsAdd("20");
        GammaItem = ListView1->Items->Add();
        GammaItem->Caption = "Gamma";
        GammaItem->SubItemsAdd("30");
        ListView1->OnSelectItem = [](TObject*, TListItem* Item, bool Selected) {
            std::printf("ListView1SelectItem: %s Selected=%d\n", std::string(Item->Caption).c_str(), Selected);
            std::fflush(stdout);
        };
        ListView1->OnItemChecked = [](TObject*, TListItem* Item) {
            std::printf("ListView1ItemChecked: %s Checked=%d\n", std::string(Item->Caption).c_str(), (bool)Item->Checked);
            std::fflush(stdout);
        };
        // 列見出しのクリックで、その列の文字列の順に並べ替える。
        ListView1->OnColumnClick = [this](TObject*, TListColumn* Column) {
            std::printf("ListView1ColumnClick: %s\n", std::string(Column->Caption).c_str());
            std::fflush(stdout);
            ListView1->SortType = stText;
            ListView1->SortColumn = (int)Column->Index;
        };
        ListView1->OnDeletion = [this](TObject*, TListItem*) { ++deletedListItems_; };

        // Tier 2、4 バッチ目(TStringGrid・TDrawGrid)。PageControl1 の "Grids" ページに上下に並べる。
        GridSheet = new TTabSheet(this);
        GridSheet->PageControl = PageControl1;
        GridSheet->Caption = "Grids";

        StringGrid1 = new TStringGrid(this);
        StringGrid1->Parent = GridSheet;
        StringGrid1->Left = 5;
        StringGrid1->Top = 5;
        StringGrid1->Width = 200;
        StringGrid1->Height = 62;
        StringGrid1->ColCount = 3;
        StringGrid1->RowCount = 4;
        StringGrid1->FixedCols = 0;
        StringGrid1->DefaultRowHeight = 18;
        StringGrid1->Options = (TGridOptions)StringGrid1->Options | goEditing;
        StringGrid1->Cells[0][0] = "Name";
        StringGrid1->Cells[1][0] = "Qty";
        StringGrid1->Cells[2][0] = "Locked";
        const char* names[] = { "Cherry", "Apple", "Banana" };
        const char* qtys[]  = { "3", "1", "2" };
        for (int r = 1; r <= 3; ++r)
        {
            StringGrid1->Cells[0][r] = names[r - 1];
            StringGrid1->Cells[1][r] = qtys[r - 1];
            StringGrid1->Cells[2][r] = "-";
        }
        // 3 列目("Locked")のセルは選択させない。
        StringGrid1->OnSelectCell = [](TObject*, int ACol, int ARow, bool& CanSelect) {
            if (ACol == 2)
            {
                CanSelect = false;
                std::printf("StringGrid1SelectCell: refused (%d,%d)\n", ACol, ARow);
                std::fflush(stdout);
            }
        };
        StringGrid1->OnSelection = [this](TObject*, int ACol, int ARow) {
            std::printf("StringGrid1Selection: (%d,%d) = %s\n", ACol, ARow, std::string(StringGrid1->Cells[ACol][ARow]).c_str());
            std::fflush(stdout);
        };
        // 列見出しのクリックで、その列の値で行を並べ替える。
        StringGrid1->OnHeaderClick = [this](TObject*, bool IsColumn, int Index) {
            std::printf("StringGrid1HeaderClick: IsColumn=%d Index=%d\n", IsColumn, Index);
            std::fflush(stdout);
            if (IsColumn)
                StringGrid1->SortColRow(true, Index);
        };

        // DrawGrid1 はセルの内容を OnDrawCell で描く(市松模様と、固定セル以外に列・行の番号)。
        DrawGrid1 = new TDrawGrid(this);
        DrawGrid1->Parent = GridSheet;
        DrawGrid1->Left = 5;
        DrawGrid1->Top = 70;
        DrawGrid1->Width = 200;
        DrawGrid1->Height = 58;
        DrawGrid1->ColCount = 4;
        DrawGrid1->RowCount = 3;
        DrawGrid1->DefaultColWidth = 45;
        DrawGrid1->DefaultRowHeight = 18;
        DrawGrid1->OnDrawCell = [this](TObject*, int ACol, int ARow, TRect ARect, TGridDrawState AState) {
            ++drawnCells_;
            if (AState & gdFixed)
                return;  // 見出しは既定の描画のまま
            TCanvas& canvas = DrawGrid1->Canvas;
            canvas.Brush.Color = ((ACol + ARow) % 2) ? clYellow : clWhite;
            canvas.Pen.Color = clBlack;
            canvas.Rectangle(ARect.Left, ARect.Top, ARect.Right, ARect.Bottom);
            canvas.TextOut(ARect.Left + 3, ARect.Top + 2, std::to_string(ACol) + "," + std::to_string(ARow));
        };
        GridSheet->OnShow = [this](TObject*) {
            TRect r = DrawGrid1->CellRect(1, 1);
            int col = -1, row = -1;
            StringGrid1->MouseToCell(60, 25, col, row);
            std::printf("GridSheetShow: DrawGrid1->CellRect(1,1)=(%d,%d,%d,%d), StringGrid1->MouseToCell(60,25)=(%d,%d)\n",
                        r.Left, r.Top, r.Right, r.Bottom, col, row);
            std::fflush(stdout);
        };

        // Tier 2、5 バッチ目(THeaderControl)。"Header" ページの上端に置く。
        HeaderSheet = new TTabSheet(this);
        HeaderSheet->PageControl = PageControl1;
        HeaderSheet->Caption = "Header";
        HeaderControl1 = new THeaderControl(this);
        HeaderControl1->Parent = HeaderSheet;
        HeaderControl1->Align = alTop;
        HeaderControl1->DragReorder = true;
        const char* headerTexts[] = { "Name", "Size", "Date" };
        const int   headerWidths[] = { 80, 50, 60 };
        for (int i = 0; i < 3; ++i)
        {
            THeaderSection* section = HeaderControl1->Sections->Add();
            section->Text = headerTexts[i];
            section->Width = headerWidths[i];
            section->MinWidth = 20;
        }
        HeaderControl1->Sections->Items[1]->Alignment = taRightJustify;
        HeaderControl1->OnSectionClick = [](TCustomHeaderControl*, THeaderSection* Section) {
            std::printf("HeaderControl1SectionClick: %s (Index=%d)\n", std::string(Section->Text).c_str(), (int)Section->Index);
            std::fflush(stdout);
        };
        HeaderControl1->OnSectionResize = [](TCustomHeaderControl*, THeaderSection* Section) {
            std::printf("HeaderControl1SectionResize: %s Width=%d\n", std::string(Section->Text).c_str(), (int)Section->Width);
            std::fflush(stdout);
        };
        HeaderControl1->OnSectionTrack = [](TCustomHeaderControl*, THeaderSection* Section, int Width, TSectionTrackState State) {
            if (State != tsTrackMove)  // 移動中は何度も呼ばれるので、開始と終了だけ表示する
            {
                std::printf("HeaderControl1SectionTrack: %s Width=%d State=%d\n", std::string(Section->Text).c_str(), Width, (int)State);
                std::fflush(stdout);
            }
        };
        // "Name" は他のセクションと入れ替えさせない。
        HeaderControl1->OnSectionDrag = [](TObject*, THeaderSection* FromSection, THeaderSection* ToSection, bool& AllowDrag) {
            AllowDrag = std::string(FromSection->Text) != "Name" && std::string(ToSection->Text) != "Name";
            std::printf("HeaderControl1SectionDrag: %s -> %s AllowDrag=%d\n",
                        std::string(FromSection->Text).c_str(), std::string(ToSection->Text).c_str(), AllowDrag);
            std::fflush(stdout);
        };
        HeaderControl1->OnSectionEndDrag = [](TObject*) {
            std::printf("HeaderControl1SectionEndDrag\n");
            std::fflush(stdout);
        };
        // Tier 2、6 バッチ目(TToolBar・TToolButton)。"Tools" ページの上端に置く(TToolBar の既定の Align は alTop)。
        ToolsSheet = new TTabSheet(this);
        ToolsSheet->PageControl = PageControl1;
        ToolsSheet->Caption = "Tools";
        ToolBar1 = new TToolBar(this);
        ToolBar1->Parent = ToolsSheet;
        ToolBar1->ShowCaptions = true;
        ToolBar1->SetButtonSize(30, 22);
        // ボタンは Parent をツールバーにすると、その順で末尾に追加される。
        auto addButton = [this](const char* caption, TToolButtonStyle style) {
            TToolButton* button = new TToolButton(this);
            button->Caption = caption;
            button->Style = style;
            button->Parent = ToolBar1;
            return button;
        };
        NewToolButton   = addButton("New", tbsButton);
        SepToolButton   = addButton("", tbsDivider);
        BoldToolButton  = addButton("B", tbsCheck);
        LeftToolButton  = addButton("L", tbsCheck);
        RightToolButton = addButton("R", tbsCheck);
        DropToolButton  = addButton("Drop", tbsDropDown);
        // 隣り合う Grouped の tbsCheck は、どれか 1 つだけが Down になる。
        LeftToolButton->Grouped = true;
        RightToolButton->Grouped = true;
        LeftToolButton->Down = true;
        NewToolButton->OnClick = [](TObject* Sender) {
            std::printf("NewToolButtonClick: %s\n", std::string(static_cast<TToolButton*>(Sender)->Caption).c_str());
            std::fflush(stdout);
        };
        auto printDown = [this](TObject* Sender) {
            std::printf("ToolButtonClick: %s Down=%d, B/L/R Down=%d/%d/%d\n",
                        std::string(static_cast<TToolButton*>(Sender)->Caption).c_str(),
                        (bool)static_cast<TToolButton*>(Sender)->Down,
                        (bool)BoldToolButton->Down, (bool)LeftToolButton->Down, (bool)RightToolButton->Down);
            std::fflush(stdout);
        };
        BoldToolButton->OnClick = printDown;
        LeftToolButton->OnClick = printDown;
        RightToolButton->OnClick = printDown;
        DropToolButton->OnClick = [](TObject*) {
            std::printf("DropToolButtonClick\n");
            std::fflush(stdout);
        };
        // DropdownMenu は設定しない(矢印で OnArrowClick だけが呼ばれる)。
        DropToolButton->OnArrowClick = [](TObject*) {
            std::printf("DropToolButtonArrowClick\n");
            std::fflush(stdout);
        };
        ToolsSheet->OnShow = [this](TObject*) {
            std::printf("ToolsSheetShow: ToolBar1 Height=%d RowCount=%d, buttons (Left,Top,Width):",
                        (int)ToolBar1->Height, (int)ToolBar1->RowCount);
            for (int i = 0; i < ToolBar1->ButtonCount; ++i)
            {
                TToolButton* b = ToolBar1->Buttons[i];
                std::printf(" %d,%d,%d", (int)b->Left, (int)b->Top, (int)b->Width);
            }
            std::printf("\n");
            std::fflush(stdout);
        };

        HeaderSheet->OnShow = [this](TObject*) {
            std::printf("HeaderSheetShow: HeaderControl1 Width/Height=%d/%d, GetSectionAt(100, 5)=%d (expected 1)\n",
                        (int)HeaderControl1->Width, (int)HeaderControl1->Height, HeaderControl1->GetSectionAt(TPoint{100, 5}));
            std::fflush(stdout);
        };

        OnCreate = [this](TObject* Sender) { FormCreate(Sender); };
        OnShow = [this](TObject* Sender) { FormShow(Sender); };
        OnResize = [this](TObject* Sender) { FormResize(Sender); };
        OnCloseQuery = [this](TObject* Sender, bool& CanClose) { FormCloseQuery(Sender, CanClose); };
        OnClose = [this](TObject* Sender, TCloseAction& Action) { FormClose(Sender, Action); };
        OnDestroy = [this](TObject* Sender) { FormDestroy(Sender); };
    }

    TButton* OpenSubButton;

protected:
    // Application が所有するフォームは、main から戻った後にまとめて破棄される。
    ~TMainForm() override
    {
        std::printf("~TMainForm\n");
        std::fflush(stdout);
    }

private:
    int clicks_ = 0;
    int ticks_ = 0;
    int closeAttempts_ = 0;

    void FormCreate(TObject* Sender)
    {
        std::printf("FormCreate: Sender is Form: %s, Form1 assigned: %s\n",
                    Sender == this ? "yes" : "no", Form1 == this ? "yes" : "no");
        std::fflush(stdout);
    }

    void FormShow(TObject*)
    {
        std::printf("FormShow\n");
        // Align による配置は、LCL ではフォームが表示されるまで行われない(VCL と異なる)。OnShow の時点では済んでいる。
        // LayoutPanel(280x90)のクライアント領域は、枠(BevelOuter)の 1px 分だけ内側の (1,1)-(279,89)。
        // expected は Win32 の値で、Linux/GTK2 ではクライアント領域が右と下に 4px 狭いため、Width/Height がその分小さくなる。
        auto printBounds = [](const char* name, TControl* c, const char* expected) {
            std::printf("%s Bounds=(%d,%d,%d,%d) (expected %s)\n", name,
                        (int)c->Left, (int)c->Top, (int)c->Width, (int)c->Height, expected);
        };
        printBounds("AlignTopPanel", AlignTopPanel, "1,1,278,20");
        printBounds("AlignLeftPanel", AlignLeftPanel, "1,21,80,68");
        printBounds("Splitter1", Splitter1, "81,21,5,68");
        printBounds("AlignClientPanel", AlignClientPanel, "86,21,193,68");
        // 表示後は、座標からノードを引ける(1 行目は Root)。
        TTreeNode* atTop = TreeView1->GetNodeAt(30, 5);
        std::printf("TreeView1->GetNodeAt(30, 5) is RootNode: %s\n", atTop == RootNode ? "yes" : "no");
        // プログラムから Splitter を動かすと、alLeft のパネルの幅と alClient のパネルが追随する。
        Splitter1->SetSplitterPosition(121);
        std::printf("After SetSplitterPosition(121): SplitterPosition=%d AlignLeftPanel->Width=%d AlignClientPanel->Left=%d\n",
                    Splitter1->GetSplitterPosition(), (int)AlignLeftPanel->Width, (int)AlignClientPanel->Left);
        std::fflush(stdout);
    }

    // 閉じる操作の 1 回目は OnCloseQuery で、2 回目は OnClose で取りやめ、
    // 3 回目は既定の動作(MainForm なので caFree = アプリケーションの終了)のままにする。
    void FormCloseQuery(TObject*, bool& CanClose)
    {
        ++closeAttempts_;
        std::printf("FormCloseQuery: attempt=%d, default CanClose=%d\n", closeAttempts_, (int)CanClose);
        if (closeAttempts_ == 1)
        {
            CanClose = false;
            std::printf("FormCloseQuery: blocked (CanClose = false)\n");
        }
        std::fflush(stdout);
    }

    void FormClose(TObject*, TCloseAction& Action)
    {
        std::printf("FormClose: attempt=%d, default Action=%d\n", closeAttempts_, (int)Action);
        if (closeAttempts_ == 2)
        {
            Action = caNone;
            std::printf("FormClose: blocked (Action = caNone)\n");
        }
        std::fflush(stdout);
    }

    // main から戻った後の終了処理で呼ばれる。この時点では子コントロールもまだ有効。
    void FormDestroy(TObject*)
    {
        std::printf("FormDestroy: Button1->Caption=%s\n", std::string(Button1->Caption).c_str());
        std::fflush(stdout);
    }

    void OpenSubButtonClick(TObject*);

    void Button1Click(TObject* Sender)
    {
        TButton* button = static_cast<TButton*>(Sender);
        ++clicks_;
        button->Caption = "Clicked " + std::to_string(clicks_);
        std::printf("Button1Click: Sender is Button1: %s, count=%d\n", Sender == Button1 ? "yes" : "no", clicks_);
        std::fflush(stdout);
    }

    void Button1DblClick(TObject*)
    {
        std::printf("Button1DblClick\n");
        std::fflush(stdout);
    }

    void Button1MouseDown(TObject*, TMouseButton Button, TShiftState Shift, int X, int Y)
    {
        std::printf("Button1MouseDown: button=%d shift=0x%x pos=(%d,%d)\n", (int)Button, Shift, X, Y);
        std::fflush(stdout);
    }

    void Button1MouseUp(TObject*, TMouseButton Button, TShiftState Shift, int X, int Y)
    {
        std::printf("Button1MouseUp: button=%d shift=0x%x pos=(%d,%d)\n", (int)Button, Shift, X, Y);
        std::fflush(stdout);
    }

    void Button1MouseEnter(TObject*)
    {
        std::printf("Button1MouseEnter\n");
        std::fflush(stdout);
    }

    void Button1MouseLeave(TObject*)
    {
        std::printf("Button1MouseLeave\n");
        std::fflush(stdout);
    }

    void Edit1KeyDown(TObject*, int& Key, TShiftState Shift)
    {
        std::printf("Edit1KeyDown: key=%d shift=0x%x\n", Key, Shift);
        std::fflush(stdout);
    }

    void Edit1KeyPress(TObject*, char& Key)
    {
        std::printf("Edit1KeyPress: key=%d ('%c')\n", (int)(unsigned char)Key, Key >= 32 ? Key : '?');
        std::fflush(stdout);
    }

    void FormResize(TObject*)
    {
        std::printf("FormResize: %dx%d\n", (int)Width, (int)Height);
        std::fflush(stdout);
    }

    void ToggleBox1Click(TObject* Sender)
    {
        std::printf("ToggleBox1Click: Checked=%d\n", (bool)static_cast<TToggleBox*>(Sender)->Checked);
        if (StatusBar1)
            StatusBar1->SimpleText = std::string("Toggle: ") + (ToggleBox1->Checked ? "on" : "off");
        std::fflush(stdout);
    }

    void ScrollBar1Change(TObject* Sender)
    {
        std::printf("ScrollBar1Change: Position=%d\n", (int)static_cast<TScrollBar*>(Sender)->Position);
        std::fflush(stdout);
    }

    void TrackBar1Change(TObject* Sender)
    {
        std::printf("TrackBar1Change: Position=%d\n", (int)static_cast<TTrackBar*>(Sender)->Position);
        std::fflush(stdout);
    }

    void RadioGroup1Click(TObject* Sender)
    {
        std::printf("RadioGroup1Click: ItemIndex=%d\n", (int)static_cast<TRadioGroup*>(Sender)->ItemIndex);
        std::fflush(stdout);
    }

    void CheckListBox1ClickCheck(TObject* Sender)
    {
        TCheckListBox* box = static_cast<TCheckListBox*>(Sender);
        std::printf("CheckListBox1ClickCheck: Checked[0]=%d Checked[1]=%d Checked[2]=%d\n",
                    (bool)box->Checked[0], (bool)box->Checked[1], (bool)box->Checked[2]);
        std::fflush(stdout);
    }

    void PageControl1Change(TObject* Sender)
    {
        TPageControl* pc = static_cast<TPageControl*>(Sender);
        std::printf("PageControl1Change: ActivePageIndex=%d Caption=%s\n",
                    (int)pc->ActivePageIndex, std::string(pc->ActivePage->Caption).c_str());
        std::fflush(stdout);
    }

    void FileNewItemClick(TObject* Sender)
    {
        std::printf("FileNewItemClick: Sender is FileNewItem: %s\n", Sender == FileNewItem ? "yes" : "no");
        std::fflush(stdout);
    }

    void PopupMenu1Popup(TObject* Sender)
    {
        TComponent* from = static_cast<TPopupMenu*>(Sender)->PopupComponent;
        std::printf("PopupMenu1Popup: PopupComponent is Panel1: %s\n", from == Panel1 ? "yes" : "no");
        std::fflush(stdout);
    }

    void Splitter1Moved(TObject* Sender)
    {
        std::printf("Splitter1Moved: SplitterPosition=%d, AlignLeftPanel->Width=%d\n",
                    static_cast<TSplitter*>(Sender)->GetSplitterPosition(), (int)AlignLeftPanel->Width);
        std::fflush(stdout);
    }

    void TabControl1Change(TObject* Sender)
    {
        std::printf("TabControl1Change: TabIndex=%d\n", (int)static_cast<TTabControl*>(Sender)->TabIndex);
        std::fflush(stdout);
    }

    void SpeedButton1Click(TObject* Sender)
    {
        std::printf("SpeedButton1Click: Down=%d\n", (bool)static_cast<TSpeedButton*>(Sender)->Down);
        std::fflush(stdout);
    }

    void CheckBox1Click(TObject* Sender)
    {
        std::printf("CheckBox1Click: Checked=%d\n", (bool)static_cast<TCheckBox*>(Sender)->Checked);
        std::fflush(stdout);
    }

    void RadioButtonClick(TObject* Sender)
    {
        TRadioButton* radio = static_cast<TRadioButton*>(Sender);
        std::printf("RadioButtonClick: %s Checked=%d\n", std::string(radio->Caption).c_str(), (bool)radio->Checked);
        std::fflush(stdout);
    }

    void PanelButtonClick(TObject* Sender)
    {
        TWinControl* parent = static_cast<TControl*>(Sender)->Parent;
        std::printf("PanelButtonClick: Parent is Panel1: %s\n", parent == Panel1 ? "yes" : "no");
        std::fflush(stdout);
    }

    // Edit1 / Memo1 / ComboBox1 で共有する。dynamic_cast で Sender の種類を判定する。
    void TextChange(TObject* Sender)
    {
        const char* kind = dynamic_cast<TMemo*>(Sender)      ? "Memo"
                         : dynamic_cast<TCustomEdit*>(Sender) ? "Edit"
                         : dynamic_cast<TComboBox*>(Sender)   ? "ComboBox"
                         : "?";
        std::string text = dynamic_cast<TComboBox*>(Sender) ? std::string(ComboBox1->Text)
                         : std::string(static_cast<TCustomEdit*>(Sender)->Text);
        std::printf("TextChange: %s Text=%s\n", kind, text.c_str());
        std::fflush(stdout);
    }

    void ListBox1Click(TObject* Sender)
    {
        std::printf("ListBox1Click: ItemIndex=%d\n", (int)static_cast<TListBox*>(Sender)->ItemIndex);
        std::fflush(stdout);
    }

    void Timer1Timer(TObject*)
    {
        ++ticks_;
        TickLabel->Caption = "Tick: " + std::to_string(ticks_);
        std::printf("Timer tick! count=%d\n", ticks_);
        std::fflush(stdout);
    }

    void PaintBox1Paint(TObject* Sender)
    {
        TCanvas& canvas = static_cast<TPaintBox*>(Sender)->Canvas;
        canvas.Pen.Color = clRed;
        canvas.Pen.Width = 2;
        canvas.Brush.Color = clYellow;
        canvas.Rectangle(10, 10, 110, 70);

        canvas.Pen.Color = clBlue;
        canvas.Brush.Color = clWhite;
        canvas.Ellipse(120, 10, 200, 70);

        canvas.Pen.Color = clBlack;
        canvas.MoveTo(10, 90);
        canvas.LineTo(200, 90);

        canvas.Font.Color = clGreen;
        canvas.Font.Size = 14;
        canvas.TextOut(10, 100, "Canvas drawing test");
    }
};

void TMainForm::OpenSubButtonClick(TObject*)
{
    // Owner を this にしているが、caFree / Release で先に破棄されても Owner 側から外れるだけで問題ない。
    TSubForm* sub = new TSubForm(this);
    sub->Show();
}

} // namespace

int main()
{
    Application->Initialize();
    Application->Title = "no_vcl test";
    Application->CreateForm(&Form1);

    std::printf("Title: %s\n", std::string(Application->Title).c_str());
    std::printf("Application->MainForm is Form1: %s\n", Application->MainForm == Form1 ? "yes" : "no");
    std::printf("MainForm caption: %s\n", std::string(Application->MainForm->Caption).c_str());
    std::printf("Button1->Parent->Caption: %s\n", std::string(Form1->Button1->Parent->Caption).c_str());
    std::printf("ScrolledButton->Parent is ScrollBox1: %s\n", Form1->ScrolledButton->Parent == Form1->ScrollBox1 ? "yes" : "no");
    std::printf("Bevel1 Shape/Style: %d/%d (expected bsFrame=1/bsRaised=1)\n", (int)Form1->Bevel1->Shape, (int)Form1->Bevel1->Style);
    std::printf("Shape1 Shape/Brush.Color/Pen.Color: %d/%06x/%06x\n",
                (int)Form1->Shape1->Shape, (unsigned)(int)Form1->Shape1->Brush.Color, (unsigned)(int)Form1->Shape1->Pen.Color);
    std::printf("StaticText1 BorderStyle: %d (expected sbsSunken=2)\n", (int)Form1->StaticText1->BorderStyle);
    std::printf("ScrollBar1 Position: %d (expected 30)\n", (int)Form1->ScrollBar1->Position);
    std::printf("TrackBar1 Position: %d (expected 5)\n", (int)Form1->TrackBar1->Position);
    std::printf("ProgressBar1 Position: %d (expected 42)\n", (int)Form1->ProgressBar1->Position);
    std::printf("UpDown1 Position: %d, Associate is UpDownEdit: %s\n",
                (int)Form1->UpDown1->Position, Form1->UpDown1->Associate == Form1->UpDownEdit ? "yes" : "no");
    std::printf("RadioGroup1 ItemsCount/ItemIndex: %d/%d (expected 3/1)\n",
                Form1->RadioGroup1->ItemsCount(), (int)Form1->RadioGroup1->ItemIndex);
    std::printf("CheckGroup1 Checked[0]/[1]/[2]: %d/%d/%d (expected 1/0/1)\n",
                (bool)Form1->CheckGroup1->Checked[0], (bool)Form1->CheckGroup1->Checked[1], (bool)Form1->CheckGroup1->Checked[2]);
    std::printf("CheckListBox1 ItemsCount/Checked[1]: %d/%d (expected 3/1)\n",
                Form1->CheckListBox1->ItemsCount(), (bool)Form1->CheckListBox1->Checked[1]);
    std::printf("BitBtn1 Kind: %d (expected bkOK=1), Caption: %s\n",
                (int)Form1->BitBtn1->Kind, std::string(Form1->BitBtn1->Caption).c_str());
    std::printf("FloatSpinEdit1 Value: %.1f (expected 2.5)\n", (double)Form1->FloatSpinEdit1->Value);
    // SpinEdit1->Value は int 版(TCustomSpinEdit)が基底の double 版を隠していることの確認。
    std::printf("SpinEdit1 Value: %d (expected 42, int hides the inherited double)\n", (int)Form1->SpinEdit1->Value);
    std::printf("MaskEdit1 EditMask: %s\n", std::string(Form1->MaskEdit1->EditMask).c_str());
    std::printf("TabControl1 TabsCount/TabIndex: %d/%d (expected 3/0)\n",
                Form1->TabControl1->TabsCount(), (int)Form1->TabControl1->TabIndex);
    std::printf("StatusBar1 SimpleText: %s (expected Ready)\n", std::string(Form1->StatusBar1->SimpleText).c_str());
    // TStatusBar の Align の既定値は alBottom(Left/Top を指定しなくてもフォームの下端に付く)。
    std::printf("StatusBar1 Align: %d (expected alBottom=%d)\n", (int)Form1->StatusBar1->Align, (int)alBottom);
    // 配置後の位置・大きさは FormShow で確認する(表示されるまで Align による配置は行われない)。
    std::printf("AlignTopPanel/AlignLeftPanel/AlignClientPanel Align: %d/%d/%d (expected alTop=%d/alLeft=%d/alClient=%d)\n",
                (int)Form1->AlignTopPanel->Align, (int)Form1->AlignLeftPanel->Align, (int)Form1->AlignClientPanel->Align,
                (int)alTop, (int)alLeft, (int)alClient);
    {
        TSplitter* sp = Form1->Splitter1;
        std::printf("Splitter1 Align=%d (expected alLeft=%d) MinSize=%d Beveled=%d AutoSnap=%d "
                    "ResizeAnchor=%d (expected akLeft=%d) ResizeStyle=%d (expected rsUpdate=%d)\n",
                    (int)sp->Align, (int)alLeft, (int)sp->MinSize, (bool)sp->Beveled, (bool)sp->AutoSnap,
                    (int)sp->ResizeAnchor, (int)akLeft, (int)sp->ResizeStyle, (int)rsUpdate);
    }

    // Tier 5(メニュー)。
    {
        TMainForm* f = Form1;
        TMenuItem* root = f->MainMenu1->Items;
        std::printf("Form1->Menu is MainMenu1: %s, Panel1->PopupMenu is PopupMenu1: %s\n",
                    f->Menu == f->MainMenu1 ? "yes" : "no", f->Panel1->PopupMenu == f->PopupMenu1 ? "yes" : "no");
        // ルート項目のラッパーは初回アクセス時に作られ、以降は同じものが返る。
        std::printf("MainMenu1->Items is the same wrapper each time: %s, Count=%d (expected 2)\n",
                    root == (TMenuItem*)f->MainMenu1->Items ? "yes" : "no", (int)root->Count);
        std::printf("FileMenu->Parent is MainMenu1->Items: %s, Items->Items[1] is ViewMenu: %s\n",
                    f->FileMenu->Parent == root ? "yes" : "no", root->Items[1] == f->ViewMenu ? "yes" : "no");
        // AddSeparator の区切り線も LCL が内部で生成した項目で、Items[i] で初めてラッパーができる。
        TMenuItem* sep = f->FileMenu->Items[1];
        std::printf("FileMenu Count=%d (expected 3), Items[1] IsLine=%d Caption=%s Parent is FileMenu: %s\n",
                    (int)f->FileMenu->Count, sep->IsLine(), std::string(sep->Caption).c_str(),
                    sep->Parent == f->FileMenu ? "yes" : "no");
        std::printf("FileNewItem ShortCut=%s (0x%04x, TextToShortCut(\"Ctrl+N\") matches: %s)\n",
                    ShortCutToText(f->FileNewItem->ShortCut).c_str(), (unsigned)(TShortCut)f->FileNewItem->ShortCut,
                    TextToShortCut("Ctrl+N") == (TShortCut)f->FileNewItem->ShortCut ? "yes" : "no");

        // Click() は利用者が選んだときと同じく、AutoCheck の反映と OnClick を行う。
        f->FileNewItem->Click();
        f->ViewLargeItem->Click();
        std::printf("After clicking Large: Small/Large Checked=%d/%d (expected 0/1)\n",
                    (bool)f->ViewSmallItem->Checked, (bool)f->ViewLargeItem->Checked);
        f->ViewSmallItem->Click();
        std::printf("After clicking Small: Small/Large Checked=%d/%d (expected 1/0)\n",
                    (bool)f->ViewSmallItem->Checked, (bool)f->ViewLargeItem->Checked);

        // Insert/Delete。Delete は外すだけで破棄しない。
        TMenuItem* temp = new TMenuItem(f);
        temp->Caption = "Temp";
        f->FileMenu->Insert(0, temp);
        std::printf("After Insert(0): IndexOf(temp)=%d (expected 0), Count=%d (expected 4)\n",
                    f->FileMenu->IndexOf(temp), (int)f->FileMenu->Count);
        f->FileMenu->Delete(0);
        std::printf("After Delete(0): Count=%d (expected 3), temp->Parent is null: %s\n",
                    (int)f->FileMenu->Count, temp->Parent == nullptr ? "yes" : "no");
        temp->Free();
    }

    // Tier 2、1 バッチ目(TPageControl + TTabSheet)。
    {
        TMainForm* f = Form1;
        TPageControl* pc = f->PageControl1;
        std::printf("PageControl1 PageCount=%d (expected 3), ActivePage is TabSheet1: %s, ActivePageIndex=%d (expected 0)\n",
                    (int)pc->PageCount, pc->ActivePage == f->TabSheet1 ? "yes" : "no", (int)pc->ActivePageIndex);
        std::printf("Pages[1] is TabSheet2 (AddTabSheet): %s, TabSheet2->PageControl is PageControl1: %s\n",
                    pc->Pages[1] == f->TabSheet2 ? "yes" : "no", f->TabSheet2->PageControl == pc ? "yes" : "no");
        std::printf("TabSheet3 TabVisible=%d TabIndex=%d (expected 0/-1), PageIndex=%d (expected 2)\n",
                    (bool)f->TabSheet3->TabVisible, (int)f->TabSheet3->TabIndex, (int)f->TabSheet3->PageIndex);
        // PageIndex を書き換えるとページの並びが変わる。
        f->TabSheet2->PageIndex = 0;
        std::printf("After TabSheet2->PageIndex = 0: Pages[0] is TabSheet2: %s, TabSheet1 PageIndex=%d (expected 1)\n",
                    pc->Pages[0] == f->TabSheet2 ? "yes" : "no", (int)f->TabSheet1->PageIndex);
        f->TabSheet2->PageIndex = 1;
        std::printf("TabPosition=%d (expected tpTop=%d), ShowTabs=%d, MultiLine=%d\n",
                    (int)pc->TabPosition, (int)tpTop, (bool)pc->ShowTabs, (bool)pc->MultiLine);

        // Clear はすべてのページを外して遅延破棄する。ここでは直後の temp->Free() で Owner と一緒に破棄される。
        TPageControl* temp = new TPageControl(f);
        temp->AddTabSheet()->Caption = "A";
        temp->AddTabSheet()->Caption = "B";
        std::printf("temp PageCount before/after Clear: %d/", (int)temp->PageCount);
        temp->Clear();
        std::printf("%d (expected 2/0)\n", (int)temp->PageCount);
        temp->Free();
    }

    // Tier 2、2 バッチ目(TTreeView)。
    {
        TMainForm* f = Form1;
        TTreeView* tv = f->TreeView1;
        TTreeNodes* items = tv->Items;
        std::printf("TreeView1 Items->Count=%d (expected 5), RootNode->Count=%d (expected 2), GrandchildNode->Level=%d (expected 2)\n",
                    (int)items->Count, (int)f->RootNode->Count, (int)f->GrandchildNode->Level);
        // 同じノードには常に同じラッパーが返る。
        std::printf("Child1Node->Parent is RootNode: %s, Items->Item[2] is GrandchildNode: %s, "
                    "FindNodeWithText(\"Child 2\") is Child2Node: %s, RootNode->GetNextSibling() is Root2Node: %s\n",
                    f->Child1Node->Parent == f->RootNode ? "yes" : "no",
                    items->Item[2] == f->GrandchildNode ? "yes" : "no",
                    items->FindNodeWithText("Child 2") == f->Child2Node ? "yes" : "no",
                    f->RootNode->GetNextSibling() == f->Root2Node ? "yes" : "no");
        std::printf("RootNode->Parent is null: %s, TreeView is TreeView1: %s\n",
                    f->RootNode->Parent == nullptr ? "yes" : "no", f->RootNode->TreeView == tv ? "yes" : "no");
        // TTreeNode::Items[i] は直下の子。読み取り専用の添字は値そのものを返すので auto で受けられる。
        auto firstChild = f->RootNode->Items[0];
        std::printf("RootNode->Items[1] is Child2Node: %s, RootNode->Items[0]->Items[0] is GrandchildNode: %s\n",
                    f->RootNode->Items[1] == f->Child2Node ? "yes" : "no",
                    firstChild->Items[0] == f->GrandchildNode ? "yes" : "no");

        static int userData = 42;
        f->Child2Node->Data = &userData;
        std::printf("Child2Node->Data: %d (expected 42)\n", *static_cast<int*>((void*)f->Child2Node->Data));

        tv->Selected = f->Child2Node;
        std::printf("Selected is Child2Node: %s, Child2Node->Selected=%d\n",
                    tv->Selected == f->Child2Node ? "yes" : "no", (bool)f->Child2Node->Selected);

        // MoveTo: Child2 を Root 2 の子に移す。
        f->Child2Node->MoveTo(f->Root2Node, naAddChild);
        std::printf("After MoveTo: Child2Node->Parent is Root2Node: %s, RootNode->Count=%d (expected 1)\n",
                    f->Child2Node->Parent == f->Root2Node ? "yes" : "no", (int)f->RootNode->Count);
        f->Root2Node->Expand(false);
        std::printf("Root2Node->Expanded=%d (expected 1)\n", (bool)f->Root2Node->Expanded);

        // OnCollapsing で取りやめると、折りたたまれない。
        TTreeNode* locked = items->Add(nullptr, "Locked");
        items->AddChild(locked, "Inside");
        locked->Expanded = true;
        locked->Collapse(false);
        std::printf("Locked->Expanded after Collapse=%d (expected 1: OnCollapsing refused)\n", (bool)locked->Expanded);

        // Delete: 子孫も含めて削除され、ノードごとに OnDeletion が呼ばれ、ラッパーも delete される。
        int before = f->deletedNodes_;
        locked->Delete();
        std::printf("After Delete: deleted=%d (expected 2), Items->Count=%d (expected 5)\n",
                    f->deletedNodes_ - before, (int)items->Count);
    }

    // Tier 2、3 バッチ目(TListView)。
    {
        TMainForm* f = Form1;
        TListView* lv = f->ListView1;
        TListItems* items = lv->Items;
        TListColumns* columns = lv->Columns;
        std::printf("ListView1 Columns->Count=%d (expected 2), Items[1]->Caption=%s Alignment=%d (expected taRightJustify=%d), "
                    "same wrapper: %s\n",
                    (int)columns->Count, std::string(columns->Items[1]->Caption).c_str(),
                    (int)columns->Items[1]->Alignment, (int)taRightJustify,
                    columns->Items[1] == columns->Items[1] ? "yes" : "no");
        std::printf("Items->Count=%d (expected 3), Item[1] is BetaItem: %s, SubItems[0]=%s (expected 20), "
                    "ListView is ListView1: %s\n",
                    (int)items->Count, items->Item[1] == f->BetaItem ? "yes" : "no",
                    f->BetaItem->SubItemsGetText(0).c_str(), f->BetaItem->ListView == lv ? "yes" : "no");
        std::printf("FindCaption(\"Gam\", partial) is GammaItem: %s\n",
                    items->FindCaption(0, "Gam", true, true, false) == f->GammaItem ? "yes" : "no");

        lv->Selected = f->BetaItem;
        std::printf("Selected is BetaItem: %s, ItemIndex=%d (expected 1), SelCount=%d (expected 1)\n",
                    lv->Selected == f->BetaItem ? "yes" : "no", (int)lv->ItemIndex, (int)lv->SelCount);
        f->GammaItem->Checked = true;
        std::printf("GammaItem Checked=%d (expected 1)\n", (bool)f->GammaItem->Checked);
        f->GammaItem->SubItemsSetText(0, "33");
        std::printf("GammaItem SubItems[0]=%s (expected 33)\n", f->GammaItem->SubItemsGetText(0).c_str());

        // Exchange で入れ替え、SortType = stText で Caption の順に並べ直す。
        items->Exchange(0, 2);
        std::printf("After Exchange(0, 2): Item[0] is GammaItem: %s\n", items->Item[0] == f->GammaItem ? "yes" : "no");
        lv->SortColumn = 0;  // 既定の -1 のままでは並べ替えない
        lv->SortType = stText;
        std::printf("After SortType = stText: Item[0] is AlphaItem: %s, Item[2] is GammaItem: %s\n",
                    items->Item[0] == f->AlphaItem ? "yes" : "no", items->Item[2] == f->GammaItem ? "yes" : "no");
        lv->SortType = stNone;

        // Delete: OnDeletion の後に項目のラッパーも delete される。列の Delete も同様。
        int before = f->deletedListItems_;
        TListItem* temp = items->Add();
        temp->Caption = "Temp";
        temp->Delete();
        std::printf("After Delete: deleted=%d (expected 1), Items->Count=%d (expected 3)\n",
                    f->deletedListItems_ - before, (int)items->Count);
        columns->Add()->Caption = "Temp";
        columns->Delete(2);
        std::printf("After column Delete: Columns->Count=%d (expected 2)\n", (int)columns->Count);
    }

    // Tier 2、4 バッチ目(TStringGrid・TDrawGrid)。
    {
        TStringGrid* sg = Form1->StringGrid1;
        std::printf("StringGrid1 ColCount/RowCount=%d/%d (expected 3/4), FixedCols/FixedRows=%d/%d (expected 0/1), "
                    "Cells[0][1]=%s, goEditing in Options: %s\n",
                    (int)sg->ColCount, (int)sg->RowCount, (int)sg->FixedCols, (int)sg->FixedRows,
                    std::string(sg->Cells[0][1]).c_str(), ((TGridOptions)sg->Options & goEditing) ? "yes" : "no");

        // ColWidths / RowHeights: 添字で読み書きする。要素同士の代入は値のコピー。
        sg->ColWidths[0] = 80;
        sg->ColWidths[1] = sg->ColWidths[0];
        sg->RowHeights[2] = 25;
        std::printf("ColWidths[0]/[1]=%d/%d (expected 80/80), RowHeights[2]=%d (expected 25)\n",
                    (int)sg->ColWidths[0], (int)sg->ColWidths[1], (int)sg->RowHeights[2]);
        sg->ColWidths[1] = 64;
        sg->RowHeights[2] = 18;

        // Cells: 要素同士の代入、Property<std::string> との間の代入、std::string への変換。
        sg->Cells[2][1] = sg->Cells[1][1];
        std::string savedCaption = Form1->Caption;
        Form1->Caption = sg->Cells[0][1];
        sg->Cells[2][2] = Form1->Caption;
        Form1->Caption = savedCaption;
        std::string cell = sg->Cells[2][1];
        std::printf("Cells[2][1]=%s (expected 3), Cells[2][2]=%s (expected Cherry), same as Cells[1][1]: %s\n",
                    cell.c_str(), std::string(sg->Cells[2][2]).c_str(), cell == std::string(sg->Cells[1][1]) ? "yes" : "no");
        sg->Cells[2][1] = "-";
        sg->Cells[2][2] = "-";

        // 列 0 の値で行を並べ替える(固定行は除く)。
        sg->SortColRow(true, 0);
        std::printf("After SortColRow(true, 0): %s/%s/%s (expected Apple/Banana/Cherry), Qty of Apple=%s (expected 1)\n",
                    std::string(sg->Cells[0][1]).c_str(), std::string(sg->Cells[0][2]).c_str(),
                    std::string(sg->Cells[0][3]).c_str(), std::string(sg->Cells[1][1]).c_str());

        // 行の挿入・削除・移動。
        sg->InsertColRow(false, 1);
        std::printf("After InsertColRow(false, 1): RowCount=%d (expected 5), Cells[0][1]='%s' (expected ''), Cells[0][2]=%s (expected Apple)\n",
                    (int)sg->RowCount, std::string(sg->Cells[0][1]).c_str(), std::string(sg->Cells[0][2]).c_str());
        sg->DeleteColRow(false, 1);
        sg->MoveColRow(false, 3, 1);
        std::printf("After DeleteColRow and MoveColRow(false, 3, 1): RowCount=%d (expected 4), Cells[0][1]=%s (expected Cherry)\n",
                    (int)sg->RowCount, std::string(sg->Cells[0][1]).c_str());
        sg->MoveColRow(false, 1, 3);

        // 選択範囲。
        sg->Col = 1;
        sg->Row = 2;
        TGridRect sel = sg->Selection;
        std::printf("Col/Row=%d/%d, Selection=(%d,%d,%d,%d) (expected 1,2,1,2)\n",
                    (int)sg->Col, (int)sg->Row, sel.Left, sel.Top, sel.Right, sel.Bottom);

        TDrawGrid* dg = Form1->DrawGrid1;
        std::printf("DrawGrid1 ColCount/RowCount=%d/%d (expected 4/3), DefaultColWidth=%d (expected 45)\n",
                    (int)dg->ColCount, (int)dg->RowCount, (int)dg->DefaultColWidth);

        // Clean は文字列だけを消し、Clear は行・列を削除する。
        TStringGrid* temp = new TStringGrid(Form1);
        temp->Cells[1][1] = "x";
        temp->Clean();
        std::printf("temp after Clean: Cells[1][1]='%s' (expected ''), ColCount=%d (expected 5)\n",
                    std::string(temp->Cells[1][1]).c_str(), (int)temp->ColCount);
        temp->Clear();
        std::printf("temp after Clear: ColCount/RowCount=%d/%d (expected 0/0)\n", (int)temp->ColCount, (int)temp->RowCount);
        temp->Free();
    }

    // Tier 2、5 バッチ目(THeaderControl)。
    {
        THeaderControl* hc = Form1->HeaderControl1;
        THeaderSections* sections = hc->Sections;
        THeaderSection* size = sections->Items[1];
        std::printf("HeaderControl1 Sections->Count=%d (expected 3), Items[1]->Text=%s Width=%d (expected 50), "
                    "Left/Right=%d/%d (expected 80/130), Alignment=%d (expected taRightJustify=%d), same wrapper: %s\n",
                    (int)sections->Count, std::string(size->Text).c_str(), (int)size->Width,
                    (int)size->Left, (int)size->Right, (int)size->Alignment, (int)taRightJustify,
                    sections->Items[1] == size ? "yes" : "no");

        // Index で移動しても OriginalIndex は変わらない。
        THeaderSection* date = sections->Items[2];
        date->Index = 0;
        std::printf("After Date->Index = 0: Items[0] is Date: %s, Date OriginalIndex=%d (expected 2), "
                    "SectionFromOriginalIndex[2] is Date: %s, Name Left=%d (expected 60)\n",
                    sections->Items[0] == date ? "yes" : "no", (int)date->OriginalIndex,
                    hc->SectionFromOriginalIndex[2] == date ? "yes" : "no", (int)sections->Items[1]->Left);
        date->Index = 2;

        // Visible が false のセクションは幅 0 として扱われる。
        size->Visible = false;
        std::printf("Size->Visible = false: Width=%d (expected 0), Date Left=%d (expected 80)\n", (int)size->Width, (int)date->Left);
        size->Visible = true;

        // Insert・Delete。Delete したセクションのラッパーは delete される(C のテストで破棄通知の数を確認する)。
        THeaderSection* inserted = sections->Insert(1);
        inserted->Text = "Temp";
        std::printf("After Insert(1): Count=%d (expected 4), Items[1]->Text=%s, Items[2] is Size: %s\n",
                    (int)sections->Count, std::string(sections->Items[1]->Text).c_str(), sections->Items[2] == size ? "yes" : "no");
        sections->Delete(1);
        std::printf("After Delete(1): Count=%d (expected 3), Items[1] is Size: %s, DragReorder=%d (expected 1)\n",
                    (int)sections->Count, sections->Items[1] == size ? "yes" : "no", (bool)hc->DragReorder);
    }

    // Tier 2、6 バッチ目(TToolBar・TToolButton)。
    {
        TMainForm* f = Form1;
        TToolBar* tb = f->ToolBar1;
        std::printf("ToolBar1 ButtonCount=%d (expected 6), Buttons[2] is BoldToolButton: %s, DropToolButton->Index=%d (expected 5), "
                    "Align=%d (expected alTop=%d), EdgeBorders=0x%x (expected ebTop=0x%x), ButtonWidth/Height=%d/%d (expected 30/22)\n",
                    (int)tb->ButtonCount, tb->Buttons[2] == f->BoldToolButton ? "yes" : "no", (int)f->DropToolButton->Index,
                    (int)(TAlign)tb->Align, (int)alTop, (unsigned)(TEdgeBorders)tb->EdgeBorders, (unsigned)ebTop,
                    (int)tb->ButtonWidth, (int)tb->ButtonHeight);

        // Click は OnClick を呼ぶだけで、tbsCheck の Down は変えない(Down の切り替えはマウスを離したときに LCL が行う)。
        f->BoldToolButton->Click();
        std::printf("After BoldToolButton->Click(): B Down=%d (expected 0)\n", (bool)f->BoldToolButton->Down);
        // Grouped の tbsCheck は、Down を設定すると他方が上がる。
        f->RightToolButton->Down = true;
        std::printf("After RightToolButton->Down = true: L/R Down=%d/%d (expected 0/1), Style of SepToolButton=%d (expected tbsDivider=%d)\n",
                    (bool)f->LeftToolButton->Down, (bool)f->RightToolButton->Down, (int)(TToolButtonStyle)f->SepToolButton->Style, (int)tbsDivider);
        f->LeftToolButton->Down = true;

        // MenuItem を設定すると、その項目の Caption 等を写す。
        TToolButton* temp = new TToolButton(f);
        temp->Parent = tb;
        temp->MenuItem = f->FileNewItem;
        std::printf("temp MenuItem is FileNewItem: %s, Caption=%s (expected &New), ButtonCount=%d (expected 7)\n",
                    temp->MenuItem == f->FileNewItem ? "yes" : "no", std::string(temp->Caption).c_str(), (int)tb->ButtonCount);
        temp->Free();
        std::printf("After temp->Free(): ButtonCount=%d (expected 6)\n", (int)tb->ButtonCount);
    }

    // 2 つ目以降に生成したフォームは MainForm にならない。
    TForm* subForm = new TForm(Application);
    std::printf("MainForm after creating another form is still Form1: %s\n", Application->MainForm == Form1 ? "yes" : "no");
    subForm->Free();

    // ハンドラの解除(nullptr の代入)。解除したボタンを押しても何も起きない。
    TButton* disabledHandler = new TButton(Form1);
    disabledHandler->Parent = Form1;
    disabledHandler->Caption = "No handler";
    disabledHandler->Left = 20;
    disabledHandler->Top = 280;
    disabledHandler->OnClick = [](TObject*) { std::printf("must not be called\n"); };
    disabledHandler->OnClick = nullptr;

    // Application->Terminate() でメッセージループを抜ける(ウィンドウを閉じても抜ける)。
    TButton* quitButton = new TButton(Form1);
    quitButton->Parent = Form1;
    quitButton->Caption = "Quit";
    quitButton->Left = 20;
    quitButton->Top = 320;
    quitButton->OnClick = [](TObject*) { Application->Terminate(); };

    // Free() で個別に破棄すると、ラッパーも破棄される。
    TTracedLabel* tempLabel = new TTracedLabel(Form1);
    tempLabel->Parent = Form1;
    tempLabel->Free();
    std::printf("Destroyed labels after Free(): %d (expected 1)\n", g_destroyedLabels);

    std::printf("Running (click the buttons, then close the window three times: the first two closes are blocked, or press Quit)...\n");
    std::fflush(stdout);
    Application->Run();

    std::printf("Run returned. Terminated=%d\n", (bool)Application->Terminated);
    std::printf("OK (Form1 and its components are destroyed after main returns)\n");
    std::fflush(stdout);
    return 0;
}
