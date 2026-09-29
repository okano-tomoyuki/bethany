#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string>

#include <bethany/beth.hpp>

using namespace beth;

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
    TLabeledEdit*   LabeledEdit1;
    TTabControl*    TabControl1;
    TPanel*         LayoutPanel;
    TPanel*         AlignTopPanel;
    TPanel*         AlignLeftPanel;
    TSplitter*      Splitter1;
    TPanel*         AlignClientPanel;
    TButton*        AnchoredButton;
    TButton*        SpacedButton;
    int             anchoredWidthBefore_ = 0;
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
    // ウィンドウを作る前の ListBox1->Items の中身のハンドル(表示後に LCL が差し替えることの確認用)。
    ObjectHandle    listBoxItemsBeforeShow_ = nullptr;
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
    TTabSheet*      CoolSheet;
    TCoolBar*       CoolBar1;
    TEdit*          CoolEdit;
    TComboBox*      CoolCombo;
    TTabSheet*      ImageSheet;
    TImage*         Image1;
    TImage*         Image2;
    TBitBtn*        BitBtn2;
    int             pictureChanges_ = 0;
    TImageList*     ImageList1;
    int             imageListChanges_ = 0;
    // Tier 4(ダイアログ。docs/adr/0033)。"Dialogs" メニューから Execute する。
    TMenuItem*      DialogsMenu;
    TOpenDialog*    OpenDialog1;
    TSaveDialog*    SaveDialog1;
    TSelectDirectoryDialog* SelectDirectoryDialog1;
    TColorDialog*   ColorDialog1;
    TFontDialog*    FontDialog1;
    TFindDialog*    FindDialog1;
    TReplaceDialog* ReplaceDialog1;

    // C++Builder と同じく Owner を受け取り、TForm に渡す(Application->CreateForm が Application を渡す)。
    explicit TMainForm(TComponent* AOwner) : TForm(AOwner)
    {
        Caption = "Bethany C++ wrapper";
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

        // パネルへの描画(TCustomControl::Canvas・OnPaint。docs/adr/0045)。ボタンの右の空きに図形を描く
        Panel1->OnPaint = [this](TObject*) {
            TCanvas& c = Panel1->Canvas;
            c.Pen.Color = clBlack;
            c.Brush.Color = clYellow;
            c.Polygon({{100, 50}, {125, 8}, {150, 50}});
            c.Pen.Style = psDot;
            c.Brush.Style = bsClear;
            c.RoundRect(95, 4, 176, 56, 12, 12);
            c.Pen.Style = psSolid;
            c.Brush.Style = bsSolid;
            c.Font.Orientation = 900;
            c.Font.Color = clBlue;
            c.TextOut(158, 50, "Paint");
            c.Font.Orientation = 0;
        };

        GroupBox1 = new TGroupBox(this);
        GroupBox1->Parent = this;
        GroupBox1->Caption = "Group";
        GroupBox1->Left = 220;
        GroupBox1->Top = 90;
        GroupBox1->Width = 180;
        GroupBox1->Height = 60;

        ComboBox1 = new TComboBox(this);
        ComboBox1->Parent = this;
        ComboBox1->Items->Add("Combo A");
        ComboBox1->Items->Add("Combo B");
        ComboBox1->Items->Add("Combo C");
        ComboBox1->ItemIndex = 0;
        ComboBox1->Left = 220;
        ComboBox1->Top = 160;
        ComboBox1->Width = 150;
        ComboBox1->OnChange = [this](TObject* Sender) { TextChange(Sender); };

        ListBox1 = new TListBox(this);
        ListBox1->Parent = this;
        ListBox1->Items->Add("List 1");
        ListBox1->Items->Add("List 2");
        ListBox1->Items->Add("List 3");
        listBoxItemsBeforeShow_ = ListBox1->Items->Current();
        ListBox1->Left = 220;
        ListBox1->Top = 190;
        ListBox1->Width = 150;
        ListBox1->Height = 80;
        ListBox1->OnClick = [this](TObject* Sender) { ListBox1Click(Sender); };

        // TMemo は TCustomEdit の派生なので、Edit1 と同じ TextChange を共有できる。
        Memo1 = new TMemo(this);
        Memo1->Parent = this;
        Memo1->Lines->Add("Memo line 1");
        Memo1->Lines->Add("Memo line 2");
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
        RadioGroup1->Items->Add("Option A");
        RadioGroup1->Items->Add("Option B");
        RadioGroup1->Items->Add("Option C");
        RadioGroup1->ItemIndex = 1;
        RadioGroup1->OnClick = [this](TObject* Sender) { RadioGroup1Click(Sender); };

        CheckGroup1 = new TCheckGroup(this);
        CheckGroup1->Parent = this;
        CheckGroup1->Caption = "CheckGroup1";
        CheckGroup1->Left = 210;
        CheckGroup1->Top = 550;
        CheckGroup1->Width = 180;
        CheckGroup1->Height = 90;
        CheckGroup1->Items->Add("Feature X");
        CheckGroup1->Items->Add("Feature Y");
        CheckGroup1->Items->Add("Feature Z");
        CheckGroup1->Checked[0] = true;
        CheckGroup1->Checked[2] = true;

        CheckListBox1 = new TCheckListBox(this);
        CheckListBox1->Parent = this;
        CheckListBox1->Left = 400;
        CheckListBox1->Top = 550;
        CheckListBox1->Width = 180;
        CheckListBox1->Height = 90;
        CheckListBox1->Items->Add("Item 1");
        CheckListBox1->Items->Add("Item 2");
        CheckListBox1->Items->Add("Item 3");
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

        // TLabeledEdit。EditLabel は LCL が内部で生成したラベル。Parent を設定すると、ラベルも同じ親に置かれる。
        LabeledEdit1 = new TLabeledEdit(this);
        LabeledEdit1->Parent = this;
        LabeledEdit1->Left = 420;
        LabeledEdit1->Top = 690;
        LabeledEdit1->Width = 100;
        LabeledEdit1->LabelPosition = lpLeft;
        LabeledEdit1->LabelSpacing = 6;
        LabeledEdit1->EditLabel->Caption = "Zip:";

        // Tier 1、7 バッチ目(最後のバッチ)。ページ付きの TPageControl/TTabSheet は今回見送る。
        TabControl1 = new TTabControl(this);
        TabControl1->Parent = this;
        TabControl1->Left = 20;
        TabControl1->Top = 730;
        TabControl1->Width = 300;
        TabControl1->Height = 90;
        TabControl1->Tabs->Add("Tab A");
        TabControl1->Tabs->Add("Tab B");
        TabControl1->Tabs->Add("Tab C");
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

        // Anchors・BorderSpacing(docs/adr/0034)。Align と同じく、LCL ではフォームが表示されるまで配置されない。
        // AnchoredButton は左右の辺に付けるので、AlignClientPanel の幅が変わると同じだけ幅が変わる。
        AnchoredButton = new TButton(this);
        AnchoredButton->Parent = AlignClientPanel;
        AnchoredButton->Left = 5;
        AnchoredButton->Top = 5;
        AnchoredButton->Width = 60;
        AnchoredButton->Height = 22;
        AnchoredButton->Caption = "Anchored";
        AnchoredButton->Anchors = TAnchors() << akLeft << akTop << akRight;
        // SpacedButton は下端に寄せ、周りに 4px の余白を空ける。
        SpacedButton = new TButton(this);
        SpacedButton->Parent = AlignClientPanel;
        SpacedButton->Height = 22;
        SpacedButton->Caption = "Spaced";
        SpacedButton->Align = alBottom;
        SpacedButton->BorderSpacing->Around = 4;

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

        // Tier 4(ダイアログ)。VCL と同じく、フォームを Owner にして生成し、メニューから Execute する。
        OpenDialog1 = new TOpenDialog(this);
        OpenDialog1->Title = "Open a text file";
        OpenDialog1->Filter = "Text files (*.txt;*.md)|*.txt;*.md|All files (*.*)|*.*";
        OpenDialog1->Options = OpenDialog1->Options | ofFileMustExist;
        OpenDialog1->OnShow = [this](TObject* Sender) {
            std::printf("OpenDialog1 OnShow: Sender is OpenDialog1: %s\n", Sender == OpenDialog1 ? "yes" : "no");
            std::fflush(stdout);
        };
        OpenDialog1->OnClose = [](TObject*) { std::printf("OpenDialog1 OnClose\n"); std::fflush(stdout); };
        OpenDialog1->OnCanClose = [this](TObject*, bool& CanClose) {
            std::printf("OpenDialog1 OnCanClose: FileName=%s\n", std::string(OpenDialog1->FileName).c_str());
            std::fflush(stdout);
            CanClose = true;
        };
        SaveDialog1 = new TSaveDialog(this);
        SaveDialog1->Filter = OpenDialog1->Filter;
        SaveDialog1->DefaultExt = "txt";
        SaveDialog1->Options = SaveDialog1->Options | ofOverwritePrompt;
        SelectDirectoryDialog1 = new TSelectDirectoryDialog(this);
        ColorDialog1 = new TColorDialog(this);
        FontDialog1 = new TFontDialog(this);
        FindDialog1 = new TFindDialog(this);
        FindDialog1->OnFind = [this](TObject* Sender) { FindDialog1Find(Sender); };
        ReplaceDialog1 = new TReplaceDialog(this);
        ReplaceDialog1->OnFind = [this](TObject* Sender) { FindDialog1Find(Sender); };
        ReplaceDialog1->OnReplace = [this](TObject* Sender) { ReplaceDialog1Replace(Sender); };

        DialogsMenu = new TMenuItem(this);
        DialogsMenu->Caption = "&Dialogs";
        MainMenu1->Items->Add(DialogsMenu);
        AddDialogItem("&Open... (into Memo1)", [this] { OpenItemClick(); });
        AddDialogItem("&Save... (Memo1)", [this] { SaveItemClick(); });
        AddDialogItem("Select &directory...", [this] {
            if (SelectDirectoryDialog1->Execute())
                std::printf("SelectDirectoryDialog1: %s\n", std::string(SelectDirectoryDialog1->FileName).c_str());
            else
                std::printf("SelectDirectoryDialog1: cancelled\n");
            std::fflush(stdout);
        });
        DialogsMenu->AddSeparator();
        AddDialogItem("&Color... (Panel1)", [this] {
            ColorDialog1->Color = Panel1->Color;
            if (ColorDialog1->Execute())
            {
                Panel1->Color = ColorDialog1->Color;
                std::printf("ColorDialog1: Color=%06X\n", (unsigned)(TColor)ColorDialog1->Color);
            }
            else
                std::printf("ColorDialog1: cancelled\n");
            std::fflush(stdout);
        });
        AddDialogItem("&Font... (Memo1)", [this] {
            FontDialog1->Font = Memo1->Font;
            if (FontDialog1->Execute())
            {
                Memo1->Font = FontDialog1->Font;
                std::printf("FontDialog1: Name=%s Size=%d Style=0x%x Color=%06X\n",
                            std::string(Memo1->Font->Name).c_str(), (int)Memo1->Font->Size,
                            (unsigned)(TFontStyles)Memo1->Font->Style, (unsigned)(TColor)Memo1->Font->Color);
            }
            else
                std::printf("FontDialog1: cancelled\n");
            std::fflush(stdout);
        });
        DialogsMenu->AddSeparator();
        // TFindDialog・TReplaceDialog はモードレス(Execute はすぐ戻り、ボタンが押されるたびに OnFind・OnReplace が呼ばれる)。
        AddDialogItem("F&ind in Memo1...", [this] { FindDialog1->Execute(); });
        AddDialogItem("&Replace in Memo1...", [this] { ReplaceDialog1->Execute(); });

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
        AlphaItem->SubItems->Add("10");
        BetaItem = ListView1->Items->Add();
        BetaItem->Caption = "Beta";
        BetaItem->SubItems->Add("20");
        GammaItem = ListView1->Items->Add();
        GammaItem->Caption = "Gamma";
        GammaItem->SubItems->Add("30");
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

        // Tier 2、7 バッチ目(TCoolBar)。"Cool" ページの上端に置く(TCoolBar の既定の Align は alTop)。
        // ウィンドウを持つコントロールの Parent をクールバーにすると、LCL がバンドを自動で追加する。
        CoolSheet = new TTabSheet(this);
        CoolSheet->PageControl = PageControl1;
        CoolSheet->Caption = "Cool";
        CoolBar1 = new TCoolBar(this);
        CoolBar1->Parent = CoolSheet;
        CoolEdit = new TEdit(this);
        CoolEdit->Width = 80;
        CoolEdit->Parent = CoolBar1;
        CoolCombo = new TComboBox(this);
        CoolCombo->Width = 80;
        CoolCombo->Parent = CoolBar1;
        CoolBar1->Bands->Items[0]->Text = "Edit";
        CoolBar1->Bands->Items[1]->Text = "Combo";
        CoolBar1->OnChange = [this](TObject*) {
            TCoolBands* bands = CoolBar1->Bands;
            std::printf("CoolBar1Change: Bands[0]=%s Width=%d Break=%d, Bands[1]=%s Width=%d Break=%d\n",
                        std::string(bands->Items[0]->Text).c_str(), (int)bands->Items[0]->Width, (bool)bands->Items[0]->Break,
                        std::string(bands->Items[1]->Text).c_str(), (int)bands->Items[1]->Width, (bool)bands->Items[1]->Break);
            std::fflush(stdout);
        };
        CoolSheet->OnShow = [this](TObject*) {
            TCoolBands* bands = CoolBar1->Bands;
            std::printf("CoolSheetShow: CoolBar1 Height=%d, bands (Left,Top,Right,Height):", (int)CoolBar1->Height);
            for (int i = 0; i < bands->Count; ++i)
            {
                TCoolBand* b = bands->Items[i];
                std::printf(" %d,%d,%d,%d", (int)b->Left, (int)b->Top, (int)b->Right, (int)b->Height);
            }
            std::printf("\n");
            std::fflush(stdout);
        };

        HeaderSheet->OnShow = [this](TObject*) {
            std::printf("HeaderSheetShow: HeaderControl1 Width/Height=%d/%d, GetSectionAt(100, 5)=%d (expected 1)\n",
                        (int)HeaderControl1->Width, (int)HeaderControl1->Height, HeaderControl1->GetSectionAt(TPoint{100, 5}));
            std::fflush(stdout);
        };

        // Tier 3、1 バッチ目(TImage。docs/adr/0029)。
        ImageSheet = new TTabSheet(this);
        ImageSheet->PageControl = PageControl1;
        ImageSheet->Caption = "Image";

        Image1 = new TImage(this);
        Image1->Parent = ImageSheet;
        Image1->Left = 10;
        Image1->Top = 10;
        Image1->AutoSize = true;
        Image1->OnPictureChanged = [this](TObject*) { ++pictureChanges_; };
        // Picture->Bitmap は、Picture の中身をビットマップとして扱うビュー(空なら LCL が空のビットマップを作る)。
        Image1->Picture->Bitmap->SetSize(60, 40);
        Image1->Picture->Bitmap->Canvas->Brush.Color = clYellow;
        Image1->Picture->Bitmap->Canvas->FillRect(TRect{0, 0, 60, 40});
        Image1->Picture->Bitmap->Canvas->Pen.Color = clBlue;
        Image1->Picture->Bitmap->Canvas->Ellipse(0, 0, 60, 40);
        std::printf("Image1 Picture %dx%d (expected 60x40), OnPictureChanged called: %s, HasGraphic=%d (expected 1)\n",
                    (int)Image1->Picture->Width, (int)Image1->Picture->Height, pictureChanges_ > 0 ? "yes" : "no",
                    (bool)Image1->HasGraphic);

        // Picture が空の TImage の Canvas は、コントロールの大きさのビットマップを作ってから返る(描いた内容は Picture に残る)。
        Image2 = new TImage(this);
        Image2->Parent = ImageSheet;
        Image2->Left = 100;
        Image2->Top = 10;
        Image2->Width = 50;
        Image2->Height = 30;
        Image2->Stretch = true;
        std::printf("Image2 before Canvas: Graphic is null: %s, HasGraphic=%d (expected 0)\n",
                    (TGraphic*)Image2->Picture->Graphic == nullptr ? "yes" : "no", (bool)Image2->HasGraphic);
        Image2->Canvas->Brush.Color = clRed;
        Image2->Canvas->FillRect(TRect{0, 0, 50, 30});
        std::printf("Image2 after Canvas: Picture %dx%d (expected 50x30), Pixels[5][5]=%06X (expected 0000FF), Stretch=%d\n",
                    (int)Image2->Picture->Width, (int)Image2->Picture->Height,
                    (unsigned)(TColor)Image2->Picture->Bitmap->Canvas->Pixels[5][5], (bool)Image2->Stretch);

        // Glyph。VCL と同じく、一時的な TBitmap に描いて代入する(代入は内容のコピー)。
        BitBtn2 = new TBitBtn(this);
        BitBtn2->Parent = ImageSheet;
        BitBtn2->Left = 10;
        BitBtn2->Top = 60;
        BitBtn2->Width = 100;
        BitBtn2->Kind = bkOK;
        {
            TBitmap* glyph = new TBitmap;
            glyph->SetSize(16, 16);
            glyph->Canvas->Brush.Color = clGreen;
            glyph->Canvas->FillRect(TRect{0, 0, 16, 16});
            // LCL では、Glyph を設定すると Kind が bkCustom に戻る(Caption はそのまま)。
            BitBtn2->Glyph = glyph;
            BitBtn2->Layout = blGlyphRight;
            BitBtn2->Spacing = 8;
            // 幅が高さの 2 倍の画像は、状態別の画像が 2 つ並んだものとして NumGlyphs が 2 になる。
            glyph->SetSize(32, 16);
            SpeedButton1->Glyph = glyph;
            delete glyph;
        }
        std::printf("BitBtn2 Glyph: %dx%d (expected 16x16), NumGlyphs=%d (expected 1), Pixels[8][8]=%06X (expected 008000), "
                    "Layout=%d (expected 1), Spacing=%d (expected 8), Margin=%d (expected -1), Kind=%d (expected bkCustom=0), Caption=%s\n",
                    (int)BitBtn2->Glyph->Width, (int)BitBtn2->Glyph->Height, (int)BitBtn2->NumGlyphs,
                    (unsigned)(TColor)BitBtn2->Glyph->Canvas->Pixels[8][8], (int)(TButtonLayout)BitBtn2->Layout,
                    (int)BitBtn2->Spacing, (int)BitBtn2->Margin, (int)(TBitBtnKind)BitBtn2->Kind,
                    std::string(BitBtn2->Caption).c_str());
        std::printf("SpeedButton1 Glyph Width=%d (expected 32), NumGlyphs=%d (expected 2)\n",
                    (int)SpeedButton1->Glyph->Width, (int)SpeedButton1->NumGlyphs);
        BitBtn2->Glyph = nullptr;
        std::printf("BitBtn2 Glyph = nullptr: Glyph->Empty=%d (expected 1)\n", (bool)BitBtn2->Glyph->Empty);

        // Tier 3、2 バッチ目(TImageList と各コントロールの Images・ImageIndex。docs/adr/0030)。
        ImageList1 = new TImageList(this);
        ImageList1->OnChange = [this](TObject*) { ++imageListChanges_; };
        {
            // 横に 2 つ並んだ画像を、AddSliced で 2 つの画像として加える(LCL の Add は分けずに 16x16 に縮める)。
            TBitmap* strip = new TBitmap;
            strip->SetSize(32, 16);
            strip->Canvas->Brush.Color = clRed;
            strip->Canvas->FillRect(TRect{0, 0, 16, 16});
            strip->Canvas->Brush.Color = clBlue;
            strip->Canvas->FillRect(TRect{16, 0, 32, 16});
            int first = ImageList1->AddSliced(strip, 2, 1);
            TBitmap* green = new TBitmap;
            green->SetSize(16, 16);
            green->Canvas->Brush.Color = clGreen;
            green->Canvas->FillRect(TRect{0, 0, 16, 16});
            int masked = ImageList1->AddMasked(green, clWhite);
            std::printf("ImageList1 AddSliced(32x16, 2, 1): first=%d (expected 0), Count=%d (expected 3), AddMasked=%d (expected 2), "
                        "Width/Height=%d/%d (expected 16/16), OnChange called by Add: %s (expected no)\n",
                        first, (int)ImageList1->Count, masked, (int)ImageList1->Width, (int)ImageList1->Height,
                        imageListChanges_ > 0 ? "yes" : "no");
            // Add は画像を 16x16 に縮めて 1 つとして加える。OnChange は Delete 等で呼ばれる。
            int scaled = ImageList1->Add(strip, nullptr);
            TBitmap* check = new TBitmap;
            ImageList1->GetBitmap(scaled, check);
            std::printf("ImageList1 Add(32x16): index=%d (expected 3), Count=%d (expected 4), image %dx%d (expected 16x16)\n",
                        scaled, (int)ImageList1->Count, (int)check->Width, (int)check->Height);
            delete check;
            ImageList1->Delete(scaled);
            std::printf("ImageList1 Delete: Count=%d (expected 3), OnChange called: %s (expected yes)\n",
                        (int)ImageList1->Count, imageListChanges_ > 0 ? "yes" : "no");
            // GetBitmap で画像を取り出し、Draw で Canvas に描く。
            TBitmap* out = new TBitmap;
            ImageList1->GetBitmap(1, out);
            std::printf("ImageList1 GetBitmap(1): %dx%d (expected 16x16), Pixels[8][8]=%06X (expected FF0000)\n",
                        (int)out->Width, (int)out->Height, (unsigned)(TColor)out->Canvas->Pixels[8][8]);
            out->SetSize(20, 20);
            out->Canvas->Brush.Color = clWhite;
            out->Canvas->FillRect(TRect{0, 0, 20, 20});
            ImageList1->Draw(out->Canvas, 2, 2, 2);
            std::printf("ImageList1 Draw(2) at (2,2): Pixels[10][10]=%06X (expected 008000), Pixels[0][0]=%06X (expected FFFFFF)\n",
                        (unsigned)(TColor)out->Canvas->Pixels[10][10], (unsigned)(TColor)out->Canvas->Pixels[0][0]);
            ImageList1->Move(2, 0);
            ImageList1->GetBitmap(0, out);
            std::printf("ImageList1 Move(2, 0): image 0 Pixels[8][8]=%06X (expected 008000)\n", (unsigned)(TColor)out->Canvas->Pixels[8][8]);
            ImageList1->Move(0, 2);
            delete out;
            delete green;
            delete strip;
        }

        // 各コントロールの Images と、項目の ImageIndex。
        TreeView1->Images = ImageList1;
        RootNode->ImageIndex = 0;
        RootNode->SelectedIndex = 1;
        ListView1->SmallImages = ImageList1;
        ListView1->Items->Item[0]->ImageIndex = 1;
        ToolBar1->Images = ImageList1;
        NewToolButton->ImageIndex = 0;
        BoldToolButton->ImageIndex = 1;
        PageControl1->Images = ImageList1;
        TabSheet1->ImageIndex = 2;
        HeaderControl1->Images = ImageList1;
        HeaderControl1->Sections->Items[0]->ImageIndex = 0;
        CoolBar1->Images = ImageList1;
        CoolBar1->Bands->Items[0]->ImageIndex = 1;
        MainMenu1->Images = ImageList1;
        FileNewItem->ImageIndex = 0;
        BitBtn2->Images = ImageList1;
        BitBtn2->ImageIndex = 2;
        std::printf("Images set: TreeView=%s ListView.Small=%s ToolBar=%s PageControl=%s Header=%s CoolBar=%s MainMenu=%s BitBtn2=%s\n",
                    (TCustomImageList*)TreeView1->Images == ImageList1 ? "yes" : "no",
                    (TCustomImageList*)ListView1->SmallImages == ImageList1 ? "yes" : "no",
                    (TCustomImageList*)ToolBar1->Images == ImageList1 ? "yes" : "no",
                    (TCustomImageList*)PageControl1->Images == ImageList1 ? "yes" : "no",
                    (TCustomImageList*)HeaderControl1->Images == ImageList1 ? "yes" : "no",
                    (TCustomImageList*)CoolBar1->Images == ImageList1 ? "yes" : "no",
                    (TCustomImageList*)MainMenu1->Images == ImageList1 ? "yes" : "no",
                    (TCustomImageList*)BitBtn2->Images == ImageList1 ? "yes" : "no");
        std::printf("ImageIndex: RootNode=%d/%d (expected 0/1), ListItem=%d (expected 1), NewToolButton=%d (expected 0), "
                    "TabSheet1=%d (expected 2), Section=%d (expected 0), Band=%d (expected 1), FileNewItem=%d (expected 0), "
                    "BitBtn2=%d (expected 2), Child1Node=%d (expected -1)\n",
                    (int)RootNode->ImageIndex, (int)RootNode->SelectedIndex, (int)ListView1->Items->Item[0]->ImageIndex,
                    (int)NewToolButton->ImageIndex, (int)TabSheet1->ImageIndex,
                    (int)HeaderControl1->Sections->Items[0]->ImageIndex, (int)CoolBar1->Bands->Items[0]->ImageIndex,
                    (int)FileNewItem->ImageIndex, (int)BitBtn2->ImageIndex, (int)Child1Node->ImageIndex);

        // 画像リストを破棄すると、LCL がコントロールの Images を外す(C++ でも nullptr になる)。
        {
            TImageList* temp = new TImageList(this);
            Image2->Images = temp;
            ToolBar1->HotImages = temp;
            std::printf("Before temp->Free(): Image2->Images is temp: %s, ToolBar1->HotImages is temp: %s\n",
                        (TCustomImageList*)Image2->Images == temp ? "yes" : "no",
                        (TCustomImageList*)ToolBar1->HotImages == temp ? "yes" : "no");
            temp->Free();
            std::printf("After temp->Free(): Image2->Images is null: %s, ToolBar1->HotImages is null: %s\n",
                        (TCustomImageList*)Image2->Images == nullptr ? "yes" : "no",
                        (TCustomImageList*)ToolBar1->HotImages == nullptr ? "yes" : "no");
        }

        // メニュー項目・クールバーの Bitmap(所有者が持つ TBitmap のビュー。代入は内容のコピー)。
        {
            TBitmap* icon = new TBitmap;
            icon->SetSize(12, 12);
            FileExitItem->Bitmap = icon;
            CoolBar1->Bitmap = icon;
            delete icon;
            std::printf("FileExitItem->Bitmap %dx%d (expected 12x12), CoolBar1->Bitmap Width=%d (expected 12)\n",
                        (int)FileExitItem->Bitmap->Width, (int)FileExitItem->Bitmap->Height, (int)CoolBar1->Bitmap->Width);
            CoolBar1->Bitmap = nullptr;
            std::printf("CoolBar1->Bitmap = nullptr: Empty=%d (expected 1)\n", (bool)CoolBar1->Bitmap->Empty);
        }

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
        // ウィンドウを作ると、LCL は ListBox の Items の中身を OS のリストの TStrings に差し替える(内容は引き継がれる)。
        // TStrings のビューは操作のたびに所有者から中身を取り直すので、そのまま使える。
        std::printf("ListBox1 Items replaced after the window was created: %s, Count=%d (expected 3), Strings[2]=%s (expected List 3)\n",
                    ListBox1->Items->Current() != listBoxItemsBeforeShow_ ? "yes" : "no", (int)ListBox1->Items->Count,
                    std::string(ListBox1->Items->Strings[2]).c_str());
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
        // LabeledEdit1 のラベル(lpLeft)は、エディットの左に LabelSpacing(6)だけ離れて置かれる。
        TBoundLabel* zipLabel = LabeledEdit1->EditLabel;
        std::printf("LabeledEdit1 EditLabel right + 6 == Edit Left: %s (%d + %d + 6 vs %d)\n",
                    zipLabel->Left + zipLabel->Width + 6 == LabeledEdit1->Left ? "yes" : "no",
                    (int)zipLabel->Left, (int)zipLabel->Width, (int)LabeledEdit1->Left);
        // 表示後は、座標からノードを引ける(1 行目は Root)。
        TTreeNode* atTop = TreeView1->GetNodeAt(30, 5);
        std::printf("TreeView1->GetNodeAt(30, 5) is RootNode: %s\n", atTop == RootNode ? "yes" : "no");
        // BorderSpacing->Around(4)の分だけ、AlignClientPanel のクライアント領域(枠の 1px の内側)から離れる。
        printBounds("SpacedButton", SpacedButton, "5,41,183,22");
        int anchoredBefore = AnchoredButton->Width;
        int clientBefore = AlignClientPanel->Width;
        // プログラムから Splitter を動かすと、alLeft のパネルの幅と alClient のパネルが追随する。
        Splitter1->SetSplitterPosition(121);
        std::printf("After SetSplitterPosition(121): SplitterPosition=%d AlignLeftPanel->Width=%d AlignClientPanel->Left=%d\n",
                    Splitter1->GetSplitterPosition(), (int)AlignLeftPanel->Width, (int)AlignClientPanel->Left);
        // 左右の辺に付けた AnchoredButton の幅は、AlignClientPanel の幅と同じだけ変わる。
        std::printf("AnchoredButton Width change=%d, AlignClientPanel Width change=%d (expected equal)\n",
                    AnchoredButton->Width - anchoredBefore, AlignClientPanel->Width - clientBefore);
        // AutoSize の TImage は画像の大きさになる(表示されていないページにあっても)。
        std::printf("Image1 AutoSize Width/Height=%d/%d (expected 60/40)\n", (int)Image1->Width, (int)Image1->Height);
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

    void AddDialogItem(const char* caption, std::function<void()> action)
    {
        TMenuItem* item = new TMenuItem(this);
        item->Caption = caption;
        item->OnClick = [action](TObject*) { action(); };
        DialogsMenu->Add(item);
    }

    void OpenItemClick()
    {
        // VCL と同じく、Execute が true を返したら FileName を使う。
        if (OpenDialog1->Execute())
        {
            Memo1->Lines->LoadFromFile(OpenDialog1->FileName);
            std::printf("OpenDialog1: FileName=%s, Files->Count=%d, FilterIndex=%d, Memo1 lines=%d\n",
                        std::string(OpenDialog1->FileName).c_str(), (int)OpenDialog1->Files->Count,
                        (int)OpenDialog1->FilterIndex, (int)Memo1->Lines->Count);
        }
        else
            std::printf("OpenDialog1: cancelled\n");
        std::fflush(stdout);
    }

    void SaveItemClick()
    {
        if (SaveDialog1->Execute())
        {
            Memo1->Lines->SaveToFile(SaveDialog1->FileName);
            std::printf("SaveDialog1: saved to %s\n", std::string(SaveDialog1->FileName).c_str());
        }
        else
            std::printf("SaveDialog1: cancelled\n");
        std::fflush(stdout);
    }

    // FindDialog1・ReplaceDialog1 の「次を検索」。Memo1 の文字列を FindText で探す(frMatchCase で大文字と小文字を区別する)。
    void FindDialog1Find(TObject* Sender)
    {
        TFindDialog* dialog = static_cast<TFindDialog*>(Sender);
        std::string text = Memo1->Text;
        std::string what = dialog->FindText;
        if (!((TFindOptions)dialog->Options & frMatchCase))
        {
            for (char& c : text) c = (char)std::tolower((unsigned char)c);
            for (char& c : what) c = (char)std::tolower((unsigned char)c);
        }
        std::string::size_type pos = what.empty() ? std::string::npos : text.find(what);
        std::printf("%s OnFind: FindText=%s, Options=0x%x, found at %d\n",
                    Sender == FindDialog1 ? "FindDialog1" : "ReplaceDialog1", std::string(dialog->FindText).c_str(),
                    (unsigned)(TFindOptions)dialog->Options, pos == std::string::npos ? -1 : (int)pos);
        std::fflush(stdout);
    }

    // ReplaceDialog1 の「置換」「すべて置換」。どちらが押されたかは Options の frReplace・frReplaceAll で分かる。
    void ReplaceDialog1Replace(TObject*)
    {
        std::string text = Memo1->Text;
        std::string what = ReplaceDialog1->FindText;
        std::string with = ReplaceDialog1->ReplaceText;
        bool all = ((TFindOptions)ReplaceDialog1->Options & frReplaceAll) != 0;
        int count = 0;
        std::string::size_type pos = 0;
        while (!what.empty() && (pos = text.find(what, pos)) != std::string::npos)
        {
            text.replace(pos, what.size(), with);
            pos += with.size();
            ++count;
            if (!all)
                break;
        }
        Memo1->Text = text;
        std::printf("ReplaceDialog1 OnReplace: %s, replaced %d\n", all ? "all" : "one", count);
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
    Application->Title = "Bethany test";
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
    std::printf("RadioGroup1 Items->Count/ItemIndex: %d/%d (expected 3/1)\n",
                (int)Form1->RadioGroup1->Items->Count, (int)Form1->RadioGroup1->ItemIndex);
    std::printf("CheckGroup1 Checked[0]/[1]/[2]: %d/%d/%d (expected 1/0/1)\n",
                (bool)Form1->CheckGroup1->Checked[0], (bool)Form1->CheckGroup1->Checked[1], (bool)Form1->CheckGroup1->Checked[2]);
    std::printf("CheckListBox1 Items->Count/Checked[1]: %d/%d (expected 3/1)\n",
                (int)Form1->CheckListBox1->Items->Count, (bool)Form1->CheckListBox1->Checked[1]);
    std::printf("BitBtn1 Kind: %d (expected bkOK=1), Caption: %s\n",
                (int)Form1->BitBtn1->Kind, std::string(Form1->BitBtn1->Caption).c_str());
    std::printf("FloatSpinEdit1 Value: %.1f (expected 2.5)\n", (double)Form1->FloatSpinEdit1->Value);
    // SpinEdit1->Value は int 版(TCustomSpinEdit)が基底の double 版を隠していることの確認。
    std::printf("SpinEdit1 Value: %d (expected 42, int hides the inherited double)\n", (int)Form1->SpinEdit1->Value);
    std::printf("MaskEdit1 EditMask: %s\n", std::string(Form1->MaskEdit1->EditMask).c_str());
    {
        // EditLabel は初めて取得したときにラッパーが作られ、以降は同じラッパーが返る。
        TLabeledEdit* le = Form1->LabeledEdit1;
        TBoundLabel* editLabel = le->EditLabel;
        std::printf("LabeledEdit1 EditLabel Caption=%s (expected Zip:), same wrapper: %s, Parent is Form1: %s, "
                    "LabelPosition=%d (expected lpLeft=%d), LabelSpacing=%d (expected 6)\n",
                    std::string(editLabel->Caption).c_str(), editLabel == (TBoundLabel*)le->EditLabel ? "yes" : "no",
                    editLabel->Parent == Form1 ? "yes" : "no", (int)(TLabelPosition)le->LabelPosition, (int)lpLeft,
                    (int)le->LabelSpacing);
        // 生成直後の既定値(lpAbove・3)と、エディットと一緒にラベルも破棄されること(ラッパーも delete される)。
        TLabeledEdit* temp = new TLabeledEdit(Form1);
        std::printf("temp LabeledEdit LabelPosition=%d (expected lpAbove=%d), LabelSpacing=%d (expected 3), EditLabel Parent is null: %s\n",
                    (int)(TLabelPosition)temp->LabelPosition, (int)lpAbove, (int)temp->LabelSpacing,
                    (TWinControl*)temp->EditLabel->Parent == nullptr ? "yes" : "no");
        temp->Free();
    }
    std::printf("TabControl1 Tabs->Count/TabIndex: %d/%d (expected 3/0)\n",
                (int)Form1->TabControl1->Tabs->Count, (int)Form1->TabControl1->TabIndex);
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
        std::printf("MainMenu1->Items is the same wrapper each time: %s, Count=%d (expected 3: File, View, Dialogs)\n",
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
                    std::string(f->BetaItem->SubItems->Strings[0]).c_str(), f->BetaItem->ListView == lv ? "yes" : "no");
        std::printf("FindCaption(\"Gam\", partial) is GammaItem: %s\n",
                    items->FindCaption(0, "Gam", true, true, false) == f->GammaItem ? "yes" : "no");

        lv->Selected = f->BetaItem;
        std::printf("Selected is BetaItem: %s, ItemIndex=%d (expected 1), SelCount=%d (expected 1)\n",
                    lv->Selected == f->BetaItem ? "yes" : "no", (int)lv->ItemIndex, (int)lv->SelCount);
        f->GammaItem->Checked = true;
        std::printf("GammaItem Checked=%d (expected 1)\n", (bool)f->GammaItem->Checked);
        f->GammaItem->SubItems->Strings[0] = "33";
        std::printf("GammaItem SubItems[0]=%s (expected 33)\n", std::string(f->GammaItem->SubItems->Strings[0]).c_str());

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

    // Tier 2、7 バッチ目(TCoolBar)。
    {
        TMainForm* f = Form1;
        TCoolBar* cb = f->CoolBar1;
        TCoolBands* bands = cb->Bands;
        TCoolBand* editBand = bands->Items[0];
        std::printf("CoolBar1 Bands->Count=%d (expected 2), Items[0]->Control is CoolEdit: %s, FindBand(CoolCombo) is Items[1]: %s, "
                    "FindBandIndex(CoolCombo)=%d (expected 1), Items[0]->Text=%s, same wrapper: %s, Align=%d (expected alTop=%d)\n",
                    (int)bands->Count, editBand->Control == f->CoolEdit ? "yes" : "no",
                    bands->FindBand(f->CoolCombo) == bands->Items[1] ? "yes" : "no", bands->FindBandIndex(f->CoolCombo),
                    std::string(editBand->Text).c_str(), bands->Items[0] == editBand ? "yes" : "no",
                    (int)(TAlign)cb->Align, (int)alTop);

        // コントロールを置かないバンドを Add で追加し、Index で移動する。
        TCoolBand* empty = bands->Add();
        empty->Text = "Empty";
        empty->Index = 0;
        std::printf("After Add and Index = 0: Count=%d (expected 3), Items[0] is the new band: %s, Items[1] is editBand: %s, Control is null: %s\n",
                    (int)bands->Count, bands->Items[0] == empty ? "yes" : "no", bands->Items[1] == editBand ? "yes" : "no",
                    empty->Control == nullptr ? "yes" : "no");
        bands->Delete(0);
        std::printf("After Delete(0): Count=%d (expected 2), Items[0] is editBand: %s\n",
                    (int)bands->Count, bands->Items[0] == editBand ? "yes" : "no");

        // コントロールを破棄すると、そのバンドも LCL が削除する(バンドのラッパーも delete される)。
        TEdit* tempEdit = new TEdit(f);
        tempEdit->Parent = cb;
        TCoolBand* tempBand = bands->FindBand(tempEdit);
        std::printf("tempEdit band: Count=%d (expected 3), FindBand is Items[2]: %s\n",
                    (int)bands->Count, tempBand == bands->Items[2] ? "yes" : "no");
        tempEdit->Free();
        std::printf("After tempEdit->Free(): Count=%d (expected 2)\n", (int)bands->Count);

        // Align を alLeft にすると Vertical も true になる(TCustomCoolBar が Align の Setter を差し替えている)。
        TCoolBar* temp = new TCoolBar(f);
        temp->Align = alLeft;
        std::printf("temp CoolBar Align = alLeft: Vertical=%d (expected 1), GrabStyle=%d (expected gsDouble=%d), ShowText=%d (expected 1)\n",
                    (bool)temp->Vertical, (int)(TGrabStyle)temp->GrabStyle, (int)gsDouble, (bool)temp->ShowText);
        temp->Free();
    }

    // TStrings(Items・Lines・Tabs・SubItems)。VCL と同じく Items->Add・Items->Strings[i] 等で操作する。
    {
        TMainForm* f = Form1;
        TListBox* box = new TListBox(f);
        TStrings* items = box->Items;
        items->Add("Banana");
        int appleIndex = items->Add("Apple");
        items->Insert(0, "Cherry");
        std::printf("TStrings Add returned %d (expected 1), Count=%d (expected 3), Strings[0]=%s (expected Cherry), "
                    "IndexOf(\"Apple\")=%d (expected 2), IndexOf(\"none\")=%d (expected -1)\n",
                    appleIndex, (int)items->Count, std::string(items->Strings[0]).c_str(), items->IndexOf("Apple"), items->IndexOf("none"));

        items->Exchange(0, 2);
        items->Move(0, 1);
        items->Strings[2] = "Cherry!";
        std::printf("After Exchange(0, 2), Move(0, 1), Strings[2] = \"Cherry!\": CommaText=%s (expected Banana,Apple,Cherry!)\n",
                    std::string(items->CommaText).c_str());

        // Objects は利用者データ(ポインタ)。
        int tag = 42;
        items->Objects[1] = &tag;
        items->AddObject("Date", &tag);
        std::printf("Objects[1] is &tag: %s, Objects[3] is &tag: %s, Objects[0] is null: %s\n",
                    items->Objects[1] == &tag ? "yes" : "no", (void*)items->Objects[3] == &tag ? "yes" : "no",
                    (void*)items->Objects[0] == nullptr ? "yes" : "no");

        // Text は改行でつないだ文字列。CommaText は空白・カンマを含む要素を二重引用符で囲む。
        items->Text = "one\ntwo\nthree";
        std::printf("After Text = one/two/three: Count=%d (expected 3), Strings[1]=%s (expected two)\n",
                    (int)items->Count, std::string(items->Strings[1]).c_str());
        items->CommaText = "a,\"b c\",d";
        std::printf("After CommaText = a,\"b c\",d: Count=%d (expected 3), Strings[1]=%s (expected b c), CommaText=%s\n",
                    (int)items->Count, std::string(items->Strings[1]).c_str(), std::string(items->CommaText).c_str());

        // Assign・AddStrings は別のコントロールの TStrings から写す(ListBox1->Items->Assign(Memo1->Lines) 等)。
        items->Assign(f->Memo1->Lines);
        std::printf("After Assign(Memo1->Lines): Count=%d (expected 2), Strings[0]=%s (expected Memo line 1)\n",
                    (int)items->Count, std::string(items->Strings[0]).c_str());
        items->AddStrings(f->ComboBox1->Items);
        items->Delete(0);
        std::printf("After AddStrings(ComboBox1->Items) and Delete(0): Count=%d (expected 4), Strings[1]=%s (expected Combo A)\n",
                    (int)items->Count, std::string(items->Strings[1]).c_str());
        items->BeginUpdate();
        items->Clear();
        items->EndUpdate();
        std::printf("After Clear: Count=%d (expected 0)\n", (int)items->Count);
        box->Free();

        // 他の所有者の TStrings も同じ形で使える。
        std::printf("Memo1 Lines->Count=%d (expected 2), TabControl1 Tabs->Strings[1]=%s (expected Tab B), "
                    "BetaItem SubItems->Strings[0]=%s (expected 20), RadioGroup1 Items->Strings[2]=%s (expected Option C)\n",
                    (int)f->Memo1->Lines->Count, std::string(f->TabControl1->Tabs->Strings[1]).c_str(),
                    std::string(f->BetaItem->SubItems->Strings[0]).c_str(), std::string(f->RadioGroup1->Items->Strings[2]).c_str());
    }

    // TStringList(利用者が生成する文字列の一覧。docs/adr/0028)。VCL と同じく new して delete する。
    {
        TStringList* list = new TStringList();
        list->Add("cherry");
        list->Add("Banana");
        list->Add("apple");
        list->Sort();
        std::printf("TStringList Sort: CommaText=%s (expected apple,Banana,cherry: case-insensitive by default)\n",
                    std::string(list->CommaText).c_str());
        // CaseSensitive は IndexOf・Find・並べ替えの比較に効く(並べ替えは地域の設定に従うため、大文字が先になるとは限らない)。
        int insensitive = list->IndexOf("APPLE");
        list->CaseSensitive = true;
        std::printf("IndexOf(\"APPLE\"): %d (expected 0) / CaseSensitive: %d (expected -1)\n", insensitive, list->IndexOf("APPLE"));
        list->CaseSensitive = false;

        // Sorted にすると、Add はソート順の位置に入り、既定の dupIgnore では重複を加えない。
        list->Sorted = true;
        int blueberry = list->Add("blueberry");
        int countBefore = (int)list->Count;
        list->Add("APPLE");
        int index = -1;
        bool found = list->Find("CHERRY", index);
        int missing = -1;
        bool foundMissing = list->Find("avocado", missing);
        std::printf("Sorted Add(\"blueberry\")=%d (expected 2), Add(\"APPLE\") ignored: %s, Duplicates=%d (expected dupIgnore=%d), "
                    "Find(\"CHERRY\")=%d/%d (expected 1/3), Find(\"avocado\")=%d/%d (expected 0/1)\n",
                    blueberry, (int)list->Count == countBefore ? "yes" : "no", (int)(TDuplicates)list->Duplicates, (int)dupIgnore,
                    found, index, foundMissing, missing);

        // TStrings* を受け取るものにそのまま渡せる。
        TListBox* box = new TListBox(Form1);
        box->Items->Assign(list);
        std::printf("ListBox Items->Assign(list): Count=%d (expected 4), Strings[3]=%s (expected cherry)\n",
                    (int)box->Items->Count, std::string(box->Items->Strings[3]).c_str());
        box->Free();
        delete list;

        // 名前=値 の行(Values・Names・ValueFromIndex)。スタックに置いてもよい。
        TStringList config;
        config.Values["host"] = "localhost";
        config.Values["port"] = "8080";
        std::printf("Values: Count=%d (expected 2), Names[1]=%s (expected port), ValueFromIndex[1]=%s (expected 8080), "
                    "Values[\"host\"]=%s (expected localhost), Values[\"none\"] is empty: %s, IndexOfName(\"port\")=%d (expected 1)\n",
                    (int)config.Count, config.Names[1].c_str(), std::string(config.ValueFromIndex[1]).c_str(),
                    std::string(config.Values["host"]).c_str(), std::string(config.Values["none"]).empty() ? "yes" : "no",
                    config.IndexOfName("port"));
        // LCL(FPC)では、Values[Name] に空文字列を代入しても行は削除されず、値が空になる(VCL は削除する)。
        // 行を削除するには ValueFromIndex[i] に空文字列を代入する(または Delete)。
        config.Values["host"] = "";
        std::printf("After Values[\"host\"] = \"\": Count=%d (expected 2), Strings[0]=%s (expected host=)\n",
                    (int)config.Count, std::string(config.Strings[0]).c_str());
        config.ValueFromIndex[0] = "";
        std::printf("After ValueFromIndex[0] = \"\": Count=%d (expected 1), Strings[0]=%s (expected port=8080)\n",
                    (int)config.Count, std::string(config.Strings[0]).c_str());

        // 任意の区切り文字(StrictDelimiter なら空白は区切りにならない)。
        TStringList fields;
        fields.Delimiter = ';';
        fields.StrictDelimiter = true;
        fields.DelimitedText = "a b;c;;d";
        std::printf("DelimitedText a b;c;;d: Count=%d (expected 4), Strings[0]=%s (expected a b), Strings[2] is empty: %s, Delimiter=%c\n",
                    (int)fields.Count, std::string(fields.Strings[0]).c_str(),
                    std::string(fields.Strings[2]).empty() ? "yes" : "no", (char)fields.Delimiter);

        // ファイルへの保存と読み込み。
        const char* path = "beth_stringlist_test.txt";
        fields.SaveToFile(path);
        TStringList loaded;
        loaded.LoadFromFile(path);
        std::remove(path);
        std::printf("SaveToFile/LoadFromFile: Count=%d (expected 4), Strings[3]=%s (expected d)\n",
                    (int)loaded.Count, std::string(loaded.Strings[3]).c_str());
    }

    // 例外(docs/adr/0031)。LCL が送出した例外は beth::Exception として送出され、VCL と同じく catch (Exception& E) で受けられる。
    {
        TStringList list;
        list.Add("only");
        try
        {
            std::string s = list.Strings[5];
            std::printf("must not be reached: %s\n", s.c_str());
        }
        catch (Exception& E)
        {
            std::printf("Strings[5] on 1 item threw: ClassName=%s (expected EStringListError), Message=%s\n",
                        E.ClassName().c_str(), E.Message.c_str());
        }
        std::printf("After the exception: Count=%d (expected 1)\n", (int)list.Count);

        try
        {
            TPicture picture;
            picture.LoadFromFile("beth_no_such_file.png");
        }
        catch (Exception& E)
        {
            std::printf("LoadFromFile(no such file) threw: ClassName=%s (expected EFOpenError)\n", E.ClassName().c_str());
        }

        // ハンドラから送出した例外は、ハンドラを呼んだ DLL の関数(ここでは Click)から送出し直される。
        TMenuItem* failing = new TMenuItem(Application);
        failing->OnClick = [](TObject*) { throw Exception("boom"); };
        try
        {
            failing->Click();
            std::printf("must not be reached\n");
        }
        catch (Exception& E)
        {
            std::printf("Click with a throwing handler: ClassName=%s (expected Exception), Message=%s (expected boom)\n",
                        E.ClassName().c_str(), E.Message.c_str());
        }
        // クラス名を指定した例外・標準の例外・ハンドラの中で LCL が送出した例外。
        failing->OnClick = [](TObject*) { throw Exception("EMyError", "custom"); };
        try { failing->Click(); } catch (Exception& E) { std::printf("Custom class: %s/%s (expected EMyError/custom)\n", E.ClassName().c_str(), E.Message.c_str()); }
        failing->OnClick = [](TObject*) { throw std::runtime_error("std error"); };
        try { failing->Click(); } catch (Exception& E) { std::printf("std::runtime_error: %s/%s (expected std::exception/std error)\n", E.ClassName().c_str(), E.Message.c_str()); }
        failing->OnClick = [](TObject*) {
            TStringList inner;
            inner.Delete(3);
        };
        try { failing->Click(); } catch (Exception& E) { std::printf("LCL exception inside the handler: %s (expected EStringListError)\n", E.ClassName().c_str()); }
        // 例外を送出しないハンドラに戻すと、Click は成功する。
        int clicks = 0;
        failing->OnClick = [&clicks](TObject*) { ++clicks; };
        failing->Click();
        std::printf("Click after the failures: clicks=%d (expected 1)\n", clicks);
        failing->Free();
    }

    // Tier 3、1 バッチ目(グラフィックス。docs/adr/0029)。
    {
        // 利用者が生成するグラフィックは、VCL と同じく new して delete する。
        TBitmap* bmp = new TBitmap;
        std::printf("New TBitmap: Empty=%d (expected 1)\n", (bool)bmp->Empty);
        bmp->SetSize(32, 16);
        bmp->Canvas->Brush.Color = clRed;
        bmp->Canvas->FillRect(TRect{0, 0, 32, 16});
        bmp->Canvas->Pixels[1][2] = clBlue;
        TCanvas* canvas1 = bmp->Canvas;
        TCanvas* canvas2 = bmp->Canvas;
        std::printf("TBitmap %dx%d (expected 32x16), Empty=%d (expected 0), Pixels[0][0]=%06X (expected 0000FF), "
                    "Pixels[1][2]=%06X (expected FF0000), same Canvas wrapper: %s\n",
                    (int)bmp->Width, (int)bmp->Height, (bool)bmp->Empty,
                    (unsigned)(TColor)bmp->Canvas->Pixels[0][0], (unsigned)(TColor)bmp->Canvas->Pixels[1][2],
                    canvas1 == canvas2 ? "yes" : "no");

        // 別の形式への変換(Assign)と保存。TPicture::LoadFromFile は拡張子から形式を選ぶ。
        const char* pngPath = "beth_graphic_test.png";
        const char* jpgPath = "beth_graphic_test.jpg";
        TPortableNetworkGraphic png;
        png.Assign(bmp);
        png.SaveToFile(pngPath);
        TJPEGImage jpg;
        jpg.Assign(bmp);
        jpg.CompressionQuality = 90;
        jpg.SaveToFile(jpgPath);
        std::printf("PNG %dx%d (expected 32x16), JPEG CompressionQuality=%d (expected 90)\n",
                    (int)png.Width, (int)png.Height, (int)jpg.CompressionQuality);

        TPicture* pic = new TPicture;
        std::printf("New TPicture: Graphic is null: %s\n", (TGraphic*)pic->Graphic == nullptr ? "yes" : "no");
        pic->LoadFromFile(pngPath);
        std::printf("TPicture LoadFromFile(png): %dx%d (expected 32x16), Graphic is null: %s, PNG->Width=%d (expected 32)\n",
                    (int)pic->Width, (int)pic->Height, (TGraphic*)pic->Graphic == nullptr ? "yes" : "no", (int)pic->PNG->Width);
        // Bitmap を操作すると、LCL が中身(PNG)をビットマップに変換する。画素は引き継がれる。
        std::printf("After converting to Bitmap: Pixels[1][2]=%06X (expected FF0000)\n",
                    (unsigned)(TColor)pic->Bitmap->Canvas->Pixels[1][2]);
        pic->LoadFromFile(jpgPath);
        std::printf("TPicture LoadFromFile(jpg): %dx%d (expected 32x16)\n", (int)pic->Width, (int)pic->Height);
        // Graphic への代入は内容のコピー(bmp はそのまま利用者の持ち物)。
        pic->Graphic = bmp;
        bmp->SetSize(8, 8);
        std::printf("After Graphic = bmp and resizing bmp: Picture %dx%d (expected 32x16)\n", (int)pic->Width, (int)pic->Height);
        pic->Clear();
        std::printf("After Clear: Graphic is null: %s\n", (TGraphic*)pic->Graphic == nullptr ? "yes" : "no");
        delete pic;
        delete bmp;
        std::remove(pngPath);
        std::remove(jpgPath);
    }

    // デザイナーで設定する共通のプロパティ(docs/adr/0034)。
    {
        TButton* temp = new TButton(Form1);
        temp->Parent = Form1->Panel1;
        std::printf("temp button defaults: Anchors=0x%x (expected akLeft|akTop=0x3), TabStop=%d (expected 1), ShowHint=%d (expected 0), "
                    "ParentShowHint=%d (expected 1), ParentFont=%d (expected 1), Cursor=%d (expected crDefault=0), Tag=%d (expected 0)\n",
                    ((TAnchors)temp->Anchors).ToInt(), (bool)temp->TabStop, (bool)temp->ShowHint, (bool)temp->ParentShowHint,
                    (bool)temp->ParentFont, (int)temp->Cursor, (int)temp->Tag);
        // Panel1 の中の最後の TabOrder を 0 にすると、それまで 0 だったもの(PanelButton)は 1 になる。
        temp->TabOrder = 0;
        std::printf("temp->TabOrder=0: temp=%d (expected 0), PanelButton=%d (expected 1)\n",
                    (int)temp->TabOrder, (int)Form1->PanelButton->TabOrder);
        temp->Hint = "ヒント";
        temp->ShowHint = true;
        temp->Font->Size = 14;
        temp->Cursor = crHandPoint;
        temp->Tag = (std::intptr_t)0x12345678;
        std::printf("After setting: Hint=%s, ParentShowHint=%d (expected 0: ShowHint was set), ParentFont=%d (expected 0: Font was set), "
                    "Cursor=%d (expected crHandPoint=-21), Tag=0x%x\n",
                    std::string(temp->Hint).c_str(), (bool)temp->ParentShowHint, (bool)temp->ParentFont, (int)temp->Cursor,
                    (unsigned)(std::intptr_t)temp->Tag);
        // Constraints は設定した時点で効く(表示を待たない)。代入は内容のコピー。
        temp->Constraints->MaxWidth = 60;
        temp->Width = 200;
        TButton* other = new TButton(Form1);
        other->Constraints = temp->Constraints;
        other->BorderSpacing->Around = 7;
        temp->BorderSpacing = other->BorderSpacing;
        std::printf("Constraints MaxWidth=60 then Width=200: Width=%d (expected 60); other Constraints MaxWidth=%d (expected 60), "
                    "temp BorderSpacing Around=%d (expected 7)\n",
                    (int)temp->Width, (int)other->Constraints->MaxWidth, (int)temp->BorderSpacing->Around);
        // ParentColor は Color を設定すると false になる(今と同じ色の代入では変わらない)。
        TPanel* tempPanel = new TPanel(Form1);
        std::printf("tempPanel ParentColor before/after setting Color: %d/", (bool)tempPanel->ParentColor);
        tempPanel->Color = clYellow;
        std::printf("%d (expected 1/0)\n", (bool)tempPanel->ParentColor);
        tempPanel->Free();
        other->Free();
        temp->Free();

        // Set<E>(TAnchors)の演算。
        TAnchors a = TAnchors() << akLeft << akTop;
        TAnchors b = TAnchors() << akTop << akBottom;
        std::printf("TAnchors: a+b=0x%x (expected 0xb), a-b=0x%x (expected 0x2), a*b=0x%x (expected 0x1), a==(akTop,akLeft): %d, "
                    "Contains(akRight): %d\n",
                    (a + b).ToInt(), (a - b).ToInt(), (a * b).ToInt(), a == (TAnchors() << akTop << akLeft), a.Contains(akRight));
        a >> akTop;
        std::printf("After a >> akTop: 0x%x (expected 0x2), AnchoredButton->Anchors->Contains(akRight): %d (expected 1)\n",
                    a.ToInt(), Form1->AnchoredButton->Anchors->Contains(akRight));
    }

    // Tier 4(ダイアログ。docs/adr/0033)。Execute は "Dialogs" メニューから試す。ここでは既定値とプロパティを確かめる。
    {
        TOpenDialog* open = Form1->OpenDialog1;
        std::printf("OpenDialog1 Options has ofEnableSizing|ofViewDetail|ofFileMustExist: %s, FilterIndex=%d (expected 1), Title=%s\n",
                    (TOpenOptions)open->Options == (ofEnableSizing | ofViewDetail | ofFileMustExist) ? "yes" : "no",
                    (int)open->FilterIndex, std::string(open->Title).c_str());
        std::printf("SaveDialog1 DefaultExt=%s (expected .txt: LCL adds the dot), Options has ofOverwritePrompt: %s, Files->Count=%d (expected 0)\n",
                    std::string(Form1->SaveDialog1->DefaultExt).c_str(),
                    ((TOpenOptions)Form1->SaveDialog1->Options & ofOverwritePrompt) ? "yes" : "no", (int)Form1->SaveDialog1->Files->Count);

        TColorDialog* color = Form1->ColorDialog1;
        std::printf("ColorDialog1 Options=%u (expected cdFullOpen=%u), CustomColors->Count=%d (expected 20), Values[\"ColorB\"]=%s (expected 000080)\n",
                    (unsigned)(TColorDialogOptions)color->Options, (unsigned)cdFullOpen, (int)color->CustomColors->Count,
                    std::string(color->CustomColors->Values["ColorB"]).c_str());
        color->Color = clBlue;
        std::printf("ColorDialog1 Color=%06X (expected FF0000)\n", (unsigned)(TColor)color->Color);

        // TControl::Color・Font(ダイアログの結果を適用する先)。
        std::printf("Panel1 Color is clDefault: %s\n", (TColor)Form1->Panel1->Color == clDefault ? "yes" : "no");
        TFontDialog* font = Form1->FontDialog1;
        font->Font->Name = "Arial";
        font->Font->Size = 13;
        font->Font->Style = fsBold | fsItalic;
        TFont* before = font->Font;
        Form1->Label1->Font = font->Font;
        std::printf("FontDialog1 Options=%u (expected fdEffects=%u), same Font wrapper: %s; Label1 Font after assignment: %s %d Style=0x%x (expected Arial 13 0x3)\n",
                    (unsigned)(TFontDialogOptions)font->Options, (unsigned)fdEffects, before == font->Font ? "yes" : "no",
                    std::string(Form1->Label1->Font->Name).c_str(), (int)Form1->Label1->Font->Size,
                    (unsigned)(TFontStyles)Form1->Label1->Font->Style);
        // 代入(Assign)は内容のコピーなので、後から元を変えても写した先は変わらない。
        font->Font->Size = 20;
        Form1->Label1->Font->Style = fsUnderline;
        std::printf("After changing the source: Label1 Font Size=%d (expected 13), Style=0x%x (expected 0x4), FontDialog1 Font Style=0x%x (expected 0x3)\n",
                    (int)Form1->Label1->Font->Size, (unsigned)(TFontStyles)Form1->Label1->Font->Style,
                    (unsigned)(TFontStyles)font->Font->Style);
        font->Font->Assign(Form1->Label1->Font);
        std::printf("FontDialog1 Font->Assign(Label1->Font): Size=%d (expected 13)\n", (int)font->Font->Size);

        // TFindDialog・TReplaceDialog はモードレス。Execute はすぐ true を返し、CloseDialog で閉じる。
        TReplaceDialog* replace = Form1->ReplaceDialog1;
        replace->FindText = "apple";
        replace->ReplaceText = "orange";
        std::printf("FindDialog1 Options=0x%x (expected frDown=0x1), ReplaceDialog1 Options has frReplace|frReplaceAll: %s, "
                    "FindText=%s, ReplaceText=%s\n",
                    (unsigned)(TFindOptions)Form1->FindDialog1->Options,
                    ((TFindOptions)replace->Options & (frReplace | frReplaceAll)) == (frReplace | frReplaceAll) ? "yes" : "no",
                    std::string(replace->FindText).c_str(), std::string(replace->ReplaceText).c_str());
        bool shown = Form1->FindDialog1->Execute();
        std::printf("FindDialog1 Execute (modeless) returned: %d (expected 1)\n", shown);
        Form1->FindDialog1->CloseDialog();
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

    // メッセージのダイアログと、ボタンの ModalResult で閉じるモーダルのフォーム(docs/adr/0041)。
    TButton* dialogsButton = new TButton(Form1);
    dialogsButton->Parent = Form1;
    dialogsButton->Caption = "Dialogs...";
    dialogsButton->Left = 110;
    dialogsButton->Top = 320;
    dialogsButton->OnClick = [](TObject*) {
        const TModalResult answer = MessageDlg("Save the changes?", mtConfirmation, mbYesNoCancel);
        std::printf("MessageDlg = %d (mrYes=%d, mrNo=%d, mrCancel=%d)\n", (int)answer, (int)mrYes, (int)mrNo, (int)mrCancel);
        std::string name = "Bethany";
        const bool ok = InputQuery("InputQuery", "Name:", name);
        std::printf("InputQuery = %s, Value = %s\n", ok ? "true" : "false", name.c_str());

        // Enter で OK(Default)、Esc で Cancel(Cancel)。押したボタンの ModalResult が ShowModal() の戻り値になる。
        TForm* dialog = new TForm(Form1);
        dialog->Caption = "ModalResult";
        dialog->BorderStyle = bsDialog;
        dialog->Position = poMainFormCenter;
        dialog->Width = 240;
        dialog->Height = 90;
        TButton* okButton = new TButton(dialog);
        okButton->Parent = dialog;
        okButton->Caption = "OK";
        okButton->Left = 40;
        okButton->Top = 30;
        okButton->Width = 75;
        okButton->Height = 25;
        okButton->ModalResult = mrOk;
        okButton->Default = true;
        TButton* cancelButton = new TButton(dialog);
        cancelButton->Parent = dialog;
        cancelButton->Caption = "Cancel";
        cancelButton->Left = 125;
        cancelButton->Top = 30;
        cancelButton->Width = 75;
        cancelButton->Height = 25;
        cancelButton->ModalResult = mrCancel;
        cancelButton->Cancel = true;
        std::printf("ShowModal = %d (mrOk=%d, mrCancel=%d)\n", (int)dialog->ShowModal(), (int)mrOk, (int)mrCancel);
        std::fflush(stdout);
        dialog->Release();
    };

    // Free() で個別に破棄すると、ラッパーも破棄される。
    TTracedLabel* tempLabel = new TTracedLabel(Form1);
    tempLabel->Parent = Form1;
    tempLabel->Free();
    std::printf("Destroyed labels after Free(): %d (expected 1)\n", g_destroyedLabels);

    // テキストの編集(docs/adr/0042)。表示の前でも、選択の置き換えは DLL がハンドルを作ってから行う。
    {
        TEdit* selEdit = new TEdit(Form1);
        selEdit->Parent = Form1;
        selEdit->Left = 200;
        selEdit->Top = 320;
        selEdit->Text = "Hello World";
        selEdit->SelStart = 6;
        selEdit->SelLength = 5;
        std::printf("SelText=%s (expected World)", std::string(selEdit->SelText).c_str());
        selEdit->SelText = "Bethany";
        std::printf(", Text after SelText=%s (expected Hello Bethany)\n", std::string(selEdit->Text).c_str());
        selEdit->TextHint = "hint";
        selEdit->CharCase = ecUpperCase;
        std::printf("TextHint=%s, CharCase=%d (expected ecUpperCase=%d), CanFocus before Show=%s\n",
                    std::string(selEdit->TextHint).c_str(), (int)(TEditCharCase)selEdit->CharCase, (int)ecUpperCase,
                    selEdit->CanFocus() ? "true" : "false");
        selEdit->OnEnter = [](TObject*) { std::printf("selEdit OnEnter\n"); };
        selEdit->OnExit = [](TObject*) { std::printf("selEdit OnExit\n"); };
    }

    // リストの複数選択・チェックの 3 状態・Application(docs/adr/0043)。
    {
        TListBox* multiList = new TListBox(Form1);
        multiList->Parent = Form1;
        multiList->Left = 340;
        multiList->Top = 320;
        multiList->Height = 60;
        multiList->Items->Add("one");
        multiList->Items->Add("two");
        multiList->Items->Add("three");
        multiList->MultiSelect = true;
        multiList->Selected[0] = true;
        multiList->Selected[2] = true;
        multiList->OnSelectionChange = [multiList](TObject*, bool User) {
            std::printf("multiList OnSelectionChange User=%s SelCount=%d\n", User ? "true" : "false", (int)multiList->SelCount);
        };
        std::printf("multiList SelCount=%d (expected 2), Selected[1]=%s (expected false)\n", (int)multiList->SelCount,
                    multiList->Selected[1] ? "true" : "false");
        TCheckBox* grayCheck = new TCheckBox(Form1);
        grayCheck->Parent = Form1;
        grayCheck->Caption = "3 states";
        grayCheck->Left = 340;
        grayCheck->Top = 390;
        grayCheck->AllowGrayed = true;
        grayCheck->State = cbGrayed;
        grayCheck->OnChange = [grayCheck](TObject*) { std::printf("grayCheck State=%d\n", (int)(TCheckBoxState)grayCheck->State); };
        // 下のパネルのステータスバーの AutoHint・OnHint の確認用(docs/adr/0044)。
        // LCL は ShowHint が true のコントロール(か親)にだけ Application->Hint を設定する(VCL は ShowHint によらない)
        multiList->Hint = "multi-select list";
        multiList->ShowHint = true;
        grayCheck->Hint = "three-state check box";
        grayCheck->ShowHint = true;
        std::printf("grayCheck State=%d (expected cbGrayed=%d), ExeName ends with test_cpp.exe: %s\n",
                    (int)(TCheckBoxState)grayCheck->State, (int)cbGrayed,
                    std::string(Application->ExeName).find("test_cpp.exe") != std::string::npos ? "yes" : "no");
    }

    // Action(docs/adr/0046)。1 つの Action を、ボタンと View メニューの項目の両方に割り当てる
    {
        TActionList* actions = new TActionList(Form1);
        TAction* demo = new TAction(Form1);
        demo->Caption = "&Action demo";
        demo->ShortCut = TextToShortCut("Ctrl+K");
        demo->ActionList = actions;
        demo->OnExecute = [demo](TObject* Sender) {
            demo->Checked = !demo->Checked;
            std::printf("demo OnExecute: Sender is the action: %s, Checked=%s\n", Sender == demo ? "yes" : "no",
                        demo->Checked ? "true" : "false");
        };
        TButton* demoButton = new TButton(Form1);
        demoButton->Parent = Form1;
        demoButton->SetBounds(440, 386, 110, 25);
        demoButton->Action = demo;
        TMenuItem* demoItem = new TMenuItem(Form1);
        Form1->ViewMenu->Add(demoItem);
        demoItem->Action = demo;
        std::printf("demoButton Caption=%s, demoItem ShortCut=%s (expected &Action demo, Ctrl+K)\n",
                    std::string(demoButton->Caption).c_str(), ShortCutToText(demoItem->ShortCut).c_str());
    }

    // Screen・Clipboard・アイコン・ファイルのドロップ(docs/adr/0047)
    {
        TRect wa = Screen->WorkAreaRect;
        std::printf("Screen %dx%d, work area (%d,%d)-(%d,%d), %d dpi, %d fonts, %d forms\n", int(Screen->Width),
                    int(Screen->Height), wa.Left, wa.Top, wa.Right, wa.Bottom, int(Screen->PixelsPerInch),
                    int(Screen->Fonts->Count), int(Screen->FormCount));
        // 青い丸のアイコンを描いて、アプリケーションのアイコンにする(タイトルバーとタスクバーに出る)
        TBitmap* bmp = new TBitmap;
        bmp->SetSize(32, 32);
        bmp->Canvas->Brush.Color = clWhite;
        bmp->Canvas->FillRect(TRect{0, 0, 32, 32});
        bmp->Canvas->Brush.Color = clBlue;
        bmp->Canvas->Ellipse(2, 2, 30, 30);
        TIcon* icon = new TIcon;
        icon->Assign(bmp);
        Application->Icon = icon;
        delete icon;
        delete bmp;
        // エクスプローラーからファイルをドロップすると、名前を表示し、最初の名前をクリップボードに置く
        Form1->AllowDropFiles = true;
        Form1->OnDropFiles = [](TObject*, const std::vector<std::string>& FileNames) {
            for (const std::string& name : FileNames)
                std::printf("dropped: %s\n", name.c_str());
            Clipboard()->AsText = FileNames.front();
            std::printf("clipboard: %s (HasFormat(CF_Text())=%s)\n", std::string(Clipboard()->AsText).c_str(),
                        Clipboard()->HasFormat(CF_Text()) ? "true" : "false");
        };
    }

    // ステータスバーのパネル(docs/adr/0044)。StatusBar1 の上に、パネルを持つ 2 つ目のステータスバーを置く。
    {
        TStatusBar* panelBar = new TStatusBar(Form1);
        panelBar->Parent = Form1;
        panelBar->SimplePanel = false;
        panelBar->AutoHint = true;
        TStatusPanel* hintPanel = panelBar->Panels->Add();
        hintPanel->Width = 220;
        hintPanel->Text = "Hover the multi-select list";
        TStatusPanel* drawPanel = panelBar->Panels->Add();
        drawPanel->Width = 90;
        drawPanel->Style = psOwnerDraw;
        TStatusPanel* lastPanel = panelBar->Panels->Insert(1);
        lastPanel->Width = 120;
        lastPanel->Text = "center";
        lastPanel->Alignment = taCenter;
        lastPanel->Bevel = pbRaised;
        lastPanel->Index = 2;  // 末尾へ移す(drawPanel が 1 番目になる)
        TStatusPanel* tailPanel = panelBar->Panels->Add();
        tailPanel->Text = "Click a panel";
        panelBar->OnDrawPanel = [](TStatusBar* StatusBar, TStatusPanel* Panel, const TRect& Rect) {
            StatusBar->Canvas.Brush.Color = clBlue;
            StatusBar->Canvas.FillRect(Rect);
            StatusBar->Canvas.Font.Color = clWhite;
            StatusBar->Canvas.TextOut(Rect.Left + 4, Rect.Top + 1, "owner " + std::to_string((int)Panel->Index));
        };
        // AutoHint: ヒントのあるコントロールにマウスを載せると、OnHint(無ければ最初のパネル)に届く
        panelBar->OnHint = [panelBar](TObject*) {
            panelBar->Panels->Items[0]->Text = "Hint: " + std::string(Application->Hint);
        };
        panelBar->OnMouseDown = [panelBar](TObject*, TMouseButton, TShiftState, int X, int Y) {
            std::printf("panelBar GetPanelIndexAt(%d, %d)=%d\n", X, Y, panelBar->GetPanelIndexAt(X, Y));
        };
        std::printf("panelBar Panels Count=%d (expected 4), Items[1] Style=%d (expected psOwnerDraw=%d), "
                    "Items[2] Text=%s (expected center), SizeGrip=%s\n",
                    (int)panelBar->Panels->Count, (int)(TStatusPanelStyle)panelBar->Panels->Items[1]->Style, (int)psOwnerDraw,
                    std::string(panelBar->Panels->Items[2]->Text).c_str(), panelBar->SizeGrip ? "true" : "false");
    }

    std::printf("Running (click the buttons, then close the window three times: the first two closes are blocked, or press Quit)...\n");
    std::fflush(stdout);
    Application->Run();

    std::printf("Run returned. Terminated=%d\n", (bool)Application->Terminated);
    std::printf("OK (Form1 and its components are destroyed after main returns)\n");
    std::fflush(stdout);
    return 0;
}
