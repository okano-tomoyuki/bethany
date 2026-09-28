#include "MainForm.hpp"

using namespace no_vcl;

TMainForm* MainForm = nullptr;

TMainForm::TMainForm(TComponent* AOwner)
    : TForm(AOwner)
{
    nvd_CreateComponents();
}

// <no_vcl-designer:begin id="nvd_CreateComponents">
// Creates the components and sets their properties (generated).
void TMainForm::nvd_CreateComponents()
{
    NameEdit = new TEdit(this);
    OkButton = new TButton(this);
    PageControl1 = new TPageControl(this);
    OptionSheet = new TTabSheet(this);
    WrapCheck = new TCheckBox(this);
    SizeSpin = new TSpinEdit(this);
    MemoSheet = new TTabSheet(this);
    Memo1 = new TMemo(this);
    BottomPanel = new TPanel(this);
    StatusLabel = new TLabel(this);
    MainMenu1 = new TMainMenu(this);
    FileMenu = new TMenuItem(this);
    FileOpenItem = new TMenuItem(this);
    N1 = new TMenuItem(this);
    FileExitItem = new TMenuItem(this);
    PopupMenu1 = new TPopupMenu(this);
    ClearItem = new TMenuItem(this);
    OpenDialog1 = new TOpenDialog(this);
    Timer1 = new TTimer(this);

    Width = 400;
    Height = 300;
    Caption = "Sample";
    Menu = MainMenu1;
    OnCreate = [this](TObject* Sender) { FormCreate(Sender); };
    OnCloseQuery = [this](TObject* Sender, bool& CanClose) { FormCloseQuery(Sender, CanClose); };

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

    MemoSheet->PageControl = PageControl1;
    MemoSheet->Caption = "Memo";

    Memo1->Parent = MemoSheet;
    Memo1->Left = 0;
    Memo1->Top = 0;
    Memo1->Width = 355;
    Memo1->Height = 92;
    Memo1->Align = alClient;
    Memo1->PopupMenu = PopupMenu1;
    Memo1->ScrollBars = 3;
    Memo1->Lines->Add("line 1");
    Memo1->Lines->Add("line \"2\"");

    BottomPanel->Parent = this;
    BottomPanel->Left = 0;
    BottomPanel->Top = 259;
    BottomPanel->Width = 400;
    BottomPanel->Height = 41;
    BottomPanel->Caption = "";
    BottomPanel->Align = alBottom;
    BottomPanel->Color = 0x00C0F0FF /* #FFF0C0 */;

    StatusLabel->Parent = BottomPanel;
    StatusLabel->Left = 8;
    StatusLabel->Top = 12;
    StatusLabel->Width = 36;
    StatusLabel->Height = 17;
    StatusLabel->Caption = "Ready";
    StatusLabel->Font->Size = 10;
    StatusLabel->Font->Color = clBlue;

    PageControl1->ActivePage = MemoSheet;

    FileMenu->Caption = "&File";
    MainMenu1->Items->Add(FileMenu);

    FileOpenItem->Caption = "&Open...";
    FileOpenItem->ShortCut = TextToShortCut("Ctrl+O");
    FileOpenItem->OnClick = [this](TObject* Sender) { FileOpenItemClick(Sender); };
    FileMenu->Add(FileOpenItem);

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
}
// <no_vcl-designer:end id="nvd_CreateComponents" hash="43c1d0c7">

// <no_vcl-designer:handler-stubs>

void TMainForm::FormCreate(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::FormCloseQuery(TObject* Sender, bool& CanClose)
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
