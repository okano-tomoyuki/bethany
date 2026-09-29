// <bethany-designer:begin id="beth_SourceBegin">
#include "MainForm.hpp"
// <bethany-designer:end id="beth_SourceBegin" hash="209f96a1">
#include "dialogs/AboutForm.hpp"
#include "dialogs/ConfirmSaveForm.hpp"

// <bethany-designer:begin id="beth_NamespaceBegin">
using namespace beth;

namespace notepad
{
// <bethany-designer:end id="beth_NamespaceBegin" hash="7e0ae019">

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
    Memo1 = new TMemo(this);
    StatusBar1 = new TStatusBar(this);
    MainMenu1 = new TMainMenu(this);
    FileMenu = new TMenuItem(this);
    FileNewItem = new TMenuItem(this);
    FileOpenItem = new TMenuItem(this);
    FileSaveItem = new TMenuItem(this);
    FileSaveAsItem = new TMenuItem(this);
    N1 = new TMenuItem(this);
    FileExitItem = new TMenuItem(this);
    FormatMenu = new TMenuItem(this);
    FormatFontItem = new TMenuItem(this);
    HelpMenu = new TMenuItem(this);
    HelpAboutItem = new TMenuItem(this);
    OpenDialog1 = new TOpenDialog(this);
    SaveDialog1 = new TSaveDialog(this);
    FontDialog1 = new TFontDialog(this);

    Width = 640;
    Height = 480;
    Caption = "Bethany Notepad";
    Menu = MainMenu1;
    OnCreate = [this](TObject* Sender) { FormCreate(Sender); };
    OnCloseQuery = [this](TObject* Sender, bool& CanClose) { FormCloseQuery(Sender, CanClose); };

    Memo1->Parent = this;
    Memo1->Left = 0;
    Memo1->Top = 0;
    Memo1->Width = 640;
    Memo1->Height = 457;
    Memo1->Align = alClient;
    Memo1->ScrollBars = 3;
    Memo1->OnChange = [this](TObject* Sender) { Memo1Change(Sender); };

    StatusBar1->Parent = this;
    StatusBar1->Left = 0;
    StatusBar1->Top = 457;
    StatusBar1->Width = 640;
    StatusBar1->Height = 23;
    StatusBar1->SimpleText = "Ready";

    FileMenu->Caption = "&File";
    MainMenu1->Items->Add(FileMenu);

    FileNewItem->Caption = "&New";
    FileNewItem->ShortCut = TextToShortCut("Ctrl+N");
    FileNewItem->OnClick = [this](TObject* Sender) { FileNewItemClick(Sender); };
    FileMenu->Add(FileNewItem);

    FileOpenItem->Caption = "&Open...";
    FileOpenItem->ShortCut = TextToShortCut("Ctrl+O");
    FileOpenItem->OnClick = [this](TObject* Sender) { FileOpenItemClick(Sender); };
    FileMenu->Add(FileOpenItem);

    FileSaveItem->Caption = "&Save";
    FileSaveItem->ShortCut = TextToShortCut("Ctrl+S");
    FileSaveItem->OnClick = [this](TObject* Sender) { FileSaveItemClick(Sender); };
    FileMenu->Add(FileSaveItem);

    FileSaveAsItem->Caption = "Save &As...";
    FileSaveAsItem->OnClick = [this](TObject* Sender) { FileSaveAsItemClick(Sender); };
    FileMenu->Add(FileSaveAsItem);

    N1->Caption = "-";
    FileMenu->Add(N1);

    FileExitItem->Caption = "E&xit";
    FileExitItem->OnClick = [this](TObject* Sender) { FileExitItemClick(Sender); };
    FileMenu->Add(FileExitItem);

    FormatMenu->Caption = "F&ormat";
    MainMenu1->Items->Add(FormatMenu);

    FormatFontItem->Caption = "&Font...";
    FormatFontItem->OnClick = [this](TObject* Sender) { FormatFontItemClick(Sender); };
    FormatMenu->Add(FormatFontItem);

    HelpMenu->Caption = "&Help";
    MainMenu1->Items->Add(HelpMenu);

    HelpAboutItem->Caption = "&About Bethany Notepad...";
    HelpAboutItem->OnClick = [this](TObject* Sender) { HelpAboutItemClick(Sender); };
    HelpMenu->Add(HelpAboutItem);

    OpenDialog1->Filter = "Text files (*.txt)|*.txt|All files (*.*)|*.*";
    OpenDialog1->Options = ofEnableSizing | ofViewDetail | ofFileMustExist;

    SaveDialog1->Filter = "Text files (*.txt)|*.txt|All files (*.*)|*.*";
    SaveDialog1->DefaultExt = "txt";
    SaveDialog1->Options = ofEnableSizing | ofViewDetail | ofOverwritePrompt;
}
// <bethany-designer:end id="beth_CreateComponents" hash="03a19671">

// <bethany-designer:handler-stubs>

void TMainForm::FormCreate(TObject* Sender)
{
    UpdateCaption();
}

void TMainForm::FormCloseQuery(TObject* Sender, bool& CanClose)
{
    CanClose = ConfirmDiscard();
}

void TMainForm::Memo1Change(TObject* Sender)
{
    if (Modified)
        return;
    Modified = true;
    UpdateCaption();
}

void TMainForm::FileNewItemClick(TObject* Sender)
{
    if (!ConfirmDiscard())
        return;
    Memo1->Lines->Clear();
    FileName.clear();
    Modified = false;
    UpdateCaption();
}

void TMainForm::FileOpenItemClick(TObject* Sender)
{
    if (!ConfirmDiscard() || !OpenDialog1->Execute())
        return;
    FileName = OpenDialog1->FileName;
    Memo1->Lines->LoadFromFile(FileName);
    Modified = false;  // 読み込みでも OnChange が呼ばれるので、後で戻す
    UpdateCaption();
}

void TMainForm::FileSaveItemClick(TObject* Sender)
{
    Save();
}

void TMainForm::FileSaveAsItemClick(TObject* Sender)
{
    SaveAs();
}

void TMainForm::FileExitItemClick(TObject* Sender)
{
    Close();  // FormCloseQuery で変更の保存を聞く
}

void TMainForm::FormatFontItemClick(TObject* Sender)
{
    FontDialog1->Font = Memo1->Font;
    if (FontDialog1->Execute())
        Memo1->Font = FontDialog1->Font;
}

void TMainForm::HelpAboutItemClick(TObject* Sender)
{
    // 起動時に作ったほかのフォームは、フォームの変数で使う(C++Builder と同じ)
    dialogs::AboutForm->ShowModal();
}

// ---- ファイルの操作(デザイナーが生成しないメソッド) ----

std::string TMainForm::DisplayName() const
{
    if (FileName.empty())
        return "Untitled";
    const std::string::size_type slash = FileName.find_last_of("/\\");
    return slash == std::string::npos ? FileName : FileName.substr(slash + 1);
}

void TMainForm::UpdateCaption()
{
    Caption = (Modified ? "*" : "") + DisplayName() + " - Bethany Notepad";
    StatusBar1->SimpleText = FileName.empty() ? std::string("New file") : FileName;
}

bool TMainForm::Save()
{
    if (FileName.empty())
        return SaveAs();
    Memo1->Lines->SaveToFile(FileName);
    Modified = false;
    UpdateCaption();
    return true;
}

bool TMainForm::SaveAs()
{
    SaveDialog1->FileName = FileName;
    if (!SaveDialog1->Execute())
        return false;
    FileName = SaveDialog1->FileName;
    return Save();
}

bool TMainForm::ConfirmDiscard()
{
    if (!Modified)
        return true;
    dialogs::ConfirmSaveForm->MessageLabel->Caption = "Do you want to save the changes to " + DisplayName() + "?";
    dialogs::ConfirmSaveForm->ShowModal();
    switch (dialogs::ConfirmSaveForm->Choice)
    {
    case dialogs::TConfirmSaveForm::Save:
        return Save();
    case dialogs::TConfirmSaveForm::DontSave:
        return true;
    default:
        return false;
    }
}

// <bethany-designer:begin id="beth_NamespaceEnd">
} // namespace notepad
// <bethany-designer:end id="beth_NamespaceEnd" hash="3b50520a">
