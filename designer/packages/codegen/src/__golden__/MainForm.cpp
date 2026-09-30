// <bethany-designer:begin id="beth_SourceBegin">
#include "MainForm.hpp"
// <bethany-designer:end id="beth_SourceBegin" hash="209f96a1">

// <bethany-designer:begin id="beth_NamespaceBegin">
using namespace beth;
// <bethany-designer:end id="beth_NamespaceBegin" hash="40a01aa8">

TMainForm* MainForm = nullptr;

TMainForm::TMainForm(TComponent* AOwner)
    : TForm(AOwner)
{
    beth_CreateComponents();
}

// <bethany-designer:begin id="beth_CreateComponents">
// Creates the components and sets their properties (generated).
void TMainForm::beth_CreateComponents()
{
    NameEdit = new TEdit(this);
    OkButton = new TButton(this);
    PageControl1 = new TPageControl(this);
    OptionSheet = new TTabSheet(this);
    WrapCheck = new TCheckBox(this);
    SizeSpin = new TSpinEdit(this);
    OptionStatus = new TStatusBar(this);
    RadioGroup1 = new TRadioGroup(this);
    ScrollBar1 = new TScrollBar(this);
    MemoSheet = new TTabSheet(this);
    Memo1 = new TMemo(this);
    ListSheet = new TTabSheet(this);
    List1 = new TListView(this);
    GridSheet = new TTabSheet(this);
    Grid1 = new TStringGrid(this);
    BottomPanel = new TPanel(this);
    StatusLabel = new TLabel(this);
    HintPanel = new TPanel(this);
    ColorList = new TListBox(this);
    Tree1 = new TTreeView(this);
    MainMenu1 = new TMainMenu(this);
    FileMenu = new TMenuItem(this);
    FileOpenItem = new TMenuItem(this);
    FileSaveItem = new TMenuItem(this);
    N1 = new TMenuItem(this);
    FileExitItem = new TMenuItem(this);
    PopupMenu1 = new TPopupMenu(this);
    ClearItem = new TMenuItem(this);
    OpenDialog1 = new TOpenDialog(this);
    Timer1 = new TTimer(this);
    ActionList1 = new TActionList(this);
    FileSaveAction = new TAction(this);

    Width = 400;
    Height = 300;
    Caption = "Sample";
    Menu = MainMenu1;
    AllowDropFiles = true;
    OnCreate = [this](TObject* Sender) { FormCreate(Sender); };
    OnCloseQuery = [this](TObject* Sender, bool& CanClose) { FormCloseQuery(Sender, CanClose); };
    OnDropFiles = [this](TObject* Sender, const std::vector<std::string>& FileNames) { FormDropFiles(Sender, FileNames); };

    NameEdit->Parent = this;
    NameEdit->Left = 16;
    NameEdit->Top = 16;
    NameEdit->Width = 280;
    NameEdit->Height = 23;
    NameEdit->Hint = "Your name";
    NameEdit->ShowHint = true;
    NameEdit->Anchors = TAnchors() << akTop << akLeft << akRight;
    NameEdit->OnChange = [this](TObject* Sender) { NameEditChange(Sender); };

    OkButton->Parent = this;
    OkButton->Left = 304;
    OkButton->Top = 15;
    OkButton->Width = 75;
    OkButton->Height = 25;
    OkButton->Caption = "&OK";
    OkButton->Font->Style = fsBold;
    OkButton->Anchors = TAnchors() << akTop << akRight;
    OkButton->OnClick = [this](TObject* Sender) { OkButtonClick(Sender); };

    PageControl1->Parent = this;
    PageControl1->Left = 16;
    PageControl1->Top = 48;
    PageControl1->Width = 363;
    PageControl1->Height = 120;
    PageControl1->Anchors = TAnchors() << akTop << akLeft << akRight;

    OptionSheet->PageControl = PageControl1;
    OptionSheet->Caption = "Options";

    WrapCheck->Parent = OptionSheet;
    WrapCheck->Left = 8;
    WrapCheck->Top = 8;
    WrapCheck->Width = 80;
    WrapCheck->Height = 19;
    WrapCheck->Caption = "Word wrap";
    WrapCheck->Checked = true;
    WrapCheck->OnClick = [this](TObject* Sender) { WrapCheckClick(Sender); };

    SizeSpin->Parent = OptionSheet;
    SizeSpin->Left = 8;
    SizeSpin->Top = 36;
    SizeSpin->Width = 80;
    SizeSpin->Height = 23;
    SizeSpin->MaxValue = 200;
    SizeSpin->Value = 150;

    OptionStatus->Parent = OptionSheet;
    OptionStatus->Left = 0;
    OptionStatus->Top = 68;
    OptionStatus->Width = 355;
    OptionStatus->Height = 24;
    OptionStatus->SimplePanel = false;
    {
        TStatusPanel* item = OptionStatus->Panels->Add();
        item->Text = "Ready";
        item->Width = 120;
    }
    {
        TStatusPanel* item = OptionStatus->Panels->Add();
        item->Width = 60;
        item->Style = psOwnerDraw;
    }
    {
        TStatusPanel* item = OptionStatus->Panels->Add();
        item->Text = "right";
        item->Alignment = taRightJustify;
        item->Bevel = pbNone;
    }

    RadioGroup1->Parent = OptionSheet;
    RadioGroup1->Left = 100;
    RadioGroup1->Top = 0;
    RadioGroup1->Width = 120;
    RadioGroup1->Height = 64;
    RadioGroup1->Caption = "Size";
    RadioGroup1->Items->Add("S");
    RadioGroup1->Items->Add("M");
    RadioGroup1->Items->Add("L");
    RadioGroup1->Items->Add("XL");
    RadioGroup1->Columns = 2;
    RadioGroup1->ColumnLayout = clVerticalThenHorizontal;
    RadioGroup1->OnSelectionChanged = [this](TObject* Sender) { RadioGroup1SelectionChanged(Sender); };

    ScrollBar1->Parent = OptionSheet;
    ScrollBar1->Left = 228;
    ScrollBar1->Top = 8;
    ScrollBar1->Width = 120;
    ScrollBar1->Height = 17;
    ScrollBar1->LargeChange = 10;
    ScrollBar1->SmallChange = 2;
    ScrollBar1->OnScroll = [this](TObject* Sender, TScrollCode ScrollCode, int& ScrollPos) { ScrollBar1Scroll(Sender, ScrollCode, ScrollPos); };

    MemoSheet->PageControl = PageControl1;
    MemoSheet->Caption = "Memo";

    Memo1->Parent = MemoSheet;
    Memo1->Left = 0;
    Memo1->Top = 0;
    Memo1->Width = 355;
    Memo1->Height = 92;
    Memo1->Align = alClient;
    Memo1->PopupMenu = PopupMenu1;
    Memo1->BorderStyle = bsNone;
    Memo1->ScrollBars = ssBoth;
    Memo1->Lines->Add("line 1");
    Memo1->Lines->Add("line \"2\"");

    ListSheet->PageControl = PageControl1;
    ListSheet->Caption = "List";

    List1->Parent = ListSheet;
    List1->Left = 0;
    List1->Top = 0;
    List1->Width = 355;
    List1->Height = 92;
    List1->Align = alClient;
    List1->OwnerData = true;
    List1->ViewStyle = vsReport;
    List1->ShowColumnHeaders = false;
    List1->AutoSort = false;
    List1->OnCompare = [this](TObject* Sender, TListItem* Item1, TListItem* Item2, int Data, int& Compare) { List1Compare(Sender, Item1, Item2, Data, Compare); };
    List1->OnData = [this](TObject* Sender, TListItem* Item) { List1Data(Sender, Item); };
    List1->OnEdited = [this](TObject* Sender, TListItem* Item, std::string& AValue) { List1Edited(Sender, Item, AValue); };

    GridSheet->PageControl = PageControl1;
    GridSheet->Caption = "Grid";

    Grid1->Parent = GridSheet;
    Grid1->Left = 0;
    Grid1->Top = 0;
    Grid1->Width = 355;
    Grid1->Height = 92;
    Grid1->Align = alClient;
    Grid1->RowCount = 4;
    Grid1->AlternateColor = clInfoBk;
    Grid1->GridLineColor = clGray;
    Grid1->TitleFont->Style = fsBold;
    Grid1->AutoFillColumns = true;
    Grid1->ColumnClickSorts = true;
    {
        TGridColumn* item = Grid1->Columns->Add();
        item->Title->Caption = "Name";
        item->Width = 120;
    }
    {
        TGridColumn* item = Grid1->Columns->Add();
        item->Title->Caption = "Color";
        item->ButtonStyle = cbsPickList;
        item->PickList->Add("Red");
        item->PickList->Add("Green");
    }
    {
        TGridColumn* item = Grid1->Columns->Add();
        item->Title->Caption = "Done";
        item->ButtonStyle = cbsCheckboxColumn;
        item->ValueChecked = "Y";
        item->ValueUnchecked = "N";
    }
    Grid1->OnValidateEntry = [this](TObject* Sender, int ACol, int ARow, const std::string& OldValue, std::string& NewValue) { Grid1ValidateEntry(Sender, ACol, ARow, OldValue, NewValue); };
    Grid1->OnPrepareCanvas = [this](TObject* Sender, int ACol, int ARow, TGridDrawState AState) { Grid1PrepareCanvas(Sender, ACol, ARow, AState); };
    Grid1->OnCompareCells = [this](TObject* Sender, int ACol, int ARow, int BCol, int BRow, int& Result) { Grid1CompareCells(Sender, ACol, ARow, BCol, BRow, Result); };
    Grid1->OnCheckboxToggled = [this](TObject* Sender, int ACol, int ARow, TCheckBoxState AState) { Grid1CheckboxToggled(Sender, ACol, ARow, AState); };

    BottomPanel->Parent = this;
    BottomPanel->Left = 0;
    BottomPanel->Top = 259;
    BottomPanel->Width = 400;
    BottomPanel->Height = 41;
    BottomPanel->Caption = "";
    BottomPanel->Align = alBottom;
    BottomPanel->Color = 0x00C0F0FF /* #FFF0C0 */;
    BottomPanel->BorderWidth = 1;
    BottomPanel->BevelOuter = bvLowered;
    BottomPanel->BevelInner = bvRaised;
    BottomPanel->BevelWidth = 2;

    StatusLabel->Parent = BottomPanel;
    StatusLabel->Left = 8;
    StatusLabel->Top = 12;
    StatusLabel->Width = 36;
    StatusLabel->Height = 17;
    StatusLabel->Caption = "Ready";
    StatusLabel->Font->Size = 10;
    StatusLabel->Font->Color = clBlue;

    HintPanel->Parent = BottomPanel;
    HintPanel->Left = 295;
    HintPanel->Top = 5;
    HintPanel->Width = 100;
    HintPanel->Height = 31;
    HintPanel->Caption = "Hint";
    HintPanel->Align = alRight;
    HintPanel->BorderStyle = bsSingle;
    HintPanel->Alignment = taRightJustify;
    HintPanel->VerticalAlignment = taAlignTop;
    HintPanel->BevelOuter = bvNone;

    ColorList->Parent = this;
    ColorList->Left = 16;
    ColorList->Top = 176;
    ColorList->Width = 160;
    ColorList->Height = 76;
    ColorList->Items->Add("Red");
    ColorList->Items->Add("Green");
    ColorList->Items->Add("Blue");
    ColorList->Style = lbOwnerDrawFixed;
    ColorList->ItemHeight = 20;
    ColorList->OnDrawItem = [this](TObject* Sender, int Index, TRect ARect, TOwnerDrawState State) { ColorListDrawItem(Sender, Index, ARect, State); };

    Tree1->Parent = this;
    Tree1->Left = 200;
    Tree1->Top = 176;
    Tree1->Width = 179;
    Tree1->Height = 76;
    Tree1->MultiSelect = true;
    Tree1->SortType = stText;
    Tree1->Indent = 20;
    Tree1->OnEdited = [this](TObject* Sender, TTreeNode* Node, std::string& S) { Tree1Edited(Sender, Node, S); };
    Tree1->OnCustomDrawItem = [this](TObject* Sender, TTreeNode* Node, TCustomDrawState State, bool& DefaultDraw) { Tree1CustomDrawItem(Sender, Node, State, DefaultDraw); };

    PageControl1->ActivePage = MemoSheet;

    FileMenu->Caption = "&File";
    MainMenu1->Items->Add(FileMenu);

    FileOpenItem->Caption = "&Open...";
    FileOpenItem->ShortCut = TextToShortCut("Ctrl+O");
    FileOpenItem->OnClick = [this](TObject* Sender) { FileOpenItemClick(Sender); };
    FileMenu->Add(FileOpenItem);

    FileMenu->Add(FileSaveItem);

    N1->Caption = "-";
    FileMenu->Add(N1);

    FileExitItem->Caption = "E&xit";
    FileExitItem->OnClick = [this](TObject* Sender) { FileExitItemClick(Sender); };
    FileMenu->Add(FileExitItem);

    ClearItem->Caption = "Clear";
    ClearItem->OnClick = [this](TObject* Sender) { ClearItemClick(Sender); };
    PopupMenu1->Items->Add(ClearItem);

    OpenDialog1->Filter = "Text files|*.txt|All files|*.*";
    OpenDialog1->Options = ofEnableSizing | ofViewDetail | ofFileMustExist;

    Timer1->Interval = 500;
    Timer1->Enabled = false;
    Timer1->OnTimer = [this](TObject* Sender) { Timer1Timer(Sender); };

    FileSaveAction->ActionList = ActionList1;
    FileSaveAction->Category = "File";
    FileSaveAction->Caption = "&Save";
    FileSaveAction->ShortCut = TextToShortCut("Ctrl+S");
    FileSaveAction->OnExecute = [this](TObject* Sender) { FileSaveActionExecute(Sender); };

    FileSaveItem->Action = FileSaveAction;
}
// <bethany-designer:end id="beth_CreateComponents" hash="0d69c0ef">

// <bethany-designer:handler-stubs>

void TMainForm::FormCreate(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::FormCloseQuery(TObject* Sender, bool& CanClose)
{
    // TODO: implement
}

void TMainForm::FormDropFiles(TObject* Sender, const std::vector<std::string>& FileNames)
{
    // TODO: implement
}

void TMainForm::NameEditChange(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::OkButtonClick(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::WrapCheckClick(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::RadioGroup1SelectionChanged(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::ScrollBar1Scroll(TObject* Sender, TScrollCode ScrollCode, int& ScrollPos)
{
    // TODO: implement
}

void TMainForm::List1Compare(TObject* Sender, TListItem* Item1, TListItem* Item2, int Data, int& Compare)
{
    // TODO: implement
}

void TMainForm::List1Data(TObject* Sender, TListItem* Item)
{
    // TODO: implement
}

void TMainForm::List1Edited(TObject* Sender, TListItem* Item, std::string& AValue)
{
    // TODO: implement
}

void TMainForm::Grid1ValidateEntry(TObject* Sender, int ACol, int ARow, const std::string& OldValue, std::string& NewValue)
{
    // TODO: implement
}

void TMainForm::Grid1PrepareCanvas(TObject* Sender, int ACol, int ARow, TGridDrawState AState)
{
    // TODO: implement
}

void TMainForm::Grid1CompareCells(TObject* Sender, int ACol, int ARow, int BCol, int BRow, int& Result)
{
    // TODO: implement
}

void TMainForm::Grid1CheckboxToggled(TObject* Sender, int ACol, int ARow, TCheckBoxState AState)
{
    // TODO: implement
}

void TMainForm::ColorListDrawItem(TObject* Sender, int Index, TRect ARect, TOwnerDrawState State)
{
    // TODO: implement
}

void TMainForm::Tree1Edited(TObject* Sender, TTreeNode* Node, std::string& S)
{
    // TODO: implement
}

void TMainForm::Tree1CustomDrawItem(TObject* Sender, TTreeNode* Node, TCustomDrawState State, bool& DefaultDraw)
{
    // TODO: implement
}

void TMainForm::FileOpenItemClick(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::FileExitItemClick(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::ClearItemClick(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::Timer1Timer(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::FileSaveActionExecute(TObject* Sender)
{
    // TODO: implement
}

// <bethany-designer:begin id="beth_NamespaceEnd">
// <bethany-designer:end id="beth_NamespaceEnd" hash="811c9dc5">
