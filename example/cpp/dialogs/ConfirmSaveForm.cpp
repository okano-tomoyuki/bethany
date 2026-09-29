// <bethany-designer:begin id="beth_SourceBegin">
#include "dialogs/ConfirmSaveForm.hpp"
// <bethany-designer:end id="beth_SourceBegin" hash="a9f85a9b">

// <bethany-designer:begin id="beth_NamespaceBegin">
using namespace beth;

namespace notepad
{
namespace dialogs
{
// <bethany-designer:end id="beth_NamespaceBegin" hash="ad1e26f2">

TConfirmSaveForm* ConfirmSaveForm = nullptr;

TConfirmSaveForm::TConfirmSaveForm(TComponent* AOwner)
    : TForm(AOwner)
{
    beth_CreateComponents();
}

// <bethany-designer:begin id="beth_CreateComponents">
// Creates the components and sets their properties (generated).
void TConfirmSaveForm::beth_CreateComponents()
{
    MessageLabel = new TLabel(this);
    SaveButton = new TButton(this);
    DontSaveButton = new TButton(this);
    CancelButton = new TButton(this);

    Width = 380;
    Height = 120;
    Caption = "Bethany Notepad";
    OnShow = [this](TObject* Sender) { FormShow(Sender); };

    MessageLabel->Parent = this;
    MessageLabel->Left = 16;
    MessageLabel->Top = 16;
    MessageLabel->Width = 340;
    MessageLabel->Height = 15;
    MessageLabel->Caption = "Do you want to save the changes?";

    SaveButton->Parent = this;
    SaveButton->Left = 104;
    SaveButton->Top = 72;
    SaveButton->Width = 80;
    SaveButton->Height = 25;
    SaveButton->Caption = "&Save";
    SaveButton->Anchors = TAnchors() << akRight << akBottom;
    SaveButton->OnClick = [this](TObject* Sender) { SaveButtonClick(Sender); };

    DontSaveButton->Parent = this;
    DontSaveButton->Left = 192;
    DontSaveButton->Top = 72;
    DontSaveButton->Width = 80;
    DontSaveButton->Height = 25;
    DontSaveButton->Caption = "Do&n't Save";
    DontSaveButton->Anchors = TAnchors() << akRight << akBottom;
    DontSaveButton->OnClick = [this](TObject* Sender) { DontSaveButtonClick(Sender); };

    CancelButton->Parent = this;
    CancelButton->Left = 280;
    CancelButton->Top = 72;
    CancelButton->Width = 80;
    CancelButton->Height = 25;
    CancelButton->Caption = "Cancel";
    CancelButton->Anchors = TAnchors() << akRight << akBottom;
    CancelButton->OnClick = [this](TObject* Sender) { CancelButtonClick(Sender); };
}
// <bethany-designer:end id="beth_CreateComponents" hash="7f391009">

// <bethany-designer:handler-stubs>

void TConfirmSaveForm::FormShow(TObject* Sender)
{
    Choice = Cancel;
}

void TConfirmSaveForm::SaveButtonClick(TObject* Sender)
{
    Choice = Save;
    Close();
}

void TConfirmSaveForm::DontSaveButtonClick(TObject* Sender)
{
    Choice = DontSave;
    Close();
}

void TConfirmSaveForm::CancelButtonClick(TObject* Sender)
{
    Choice = Cancel;
    Close();
}

// <bethany-designer:begin id="beth_NamespaceEnd">
} // namespace dialogs
} // namespace notepad
// <bethany-designer:end id="beth_NamespaceEnd" hash="d95a151f">
