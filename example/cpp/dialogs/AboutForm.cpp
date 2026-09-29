// <bethany-designer:begin id="beth_SourceBegin">
#include "dialogs/AboutForm.hpp"
// <bethany-designer:end id="beth_SourceBegin" hash="5f832447">

// <bethany-designer:begin id="beth_NamespaceBegin">
using namespace beth;

namespace notepad
{
namespace dialogs
{
// <bethany-designer:end id="beth_NamespaceBegin" hash="ad1e26f2">

TAboutForm* AboutForm = nullptr;

TAboutForm::TAboutForm(TComponent* AOwner)
    : TForm(AOwner)
{
    beth_CreateComponents();
}

// <bethany-designer:begin id="beth_CreateComponents">
// Creates the components and sets their properties (generated).
void TAboutForm::beth_CreateComponents()
{
    TitleLabel = new TLabel(this);
    InfoLabel = new TLabel(this);
    OkButton = new TButton(this);

    Width = 360;
    Height = 150;
    Caption = "About Bethany Notepad";
    BorderStyle = bsDialog;
    Position = poMainFormCenter;

    TitleLabel->Parent = this;
    TitleLabel->Left = 16;
    TitleLabel->Top = 16;
    TitleLabel->Width = 150;
    TitleLabel->Height = 21;
    TitleLabel->Caption = "Bethany Notepad";
    TitleLabel->Font->Size = 12;
    TitleLabel->Font->Style = fsBold;

    InfoLabel->Parent = this;
    InfoLabel->Left = 16;
    InfoLabel->Top = 48;
    InfoLabel->Width = 320;
    InfoLabel->Height = 15;
    InfoLabel->Caption = "A sample application of Bethany, the Lazarus LCL for C++ and Python.";

    OkButton->Parent = this;
    OkButton->Left = 264;
    OkButton->Top = 104;
    OkButton->Width = 80;
    OkButton->Height = 25;
    OkButton->Caption = "OK";
    OkButton->ModalResult = mrOk;
    OkButton->Default = true;
    OkButton->Cancel = true;
    OkButton->Anchors = TAnchors() << akRight << akBottom;
}
// <bethany-designer:end id="beth_CreateComponents" hash="0db24b29">

// <bethany-designer:handler-stubs>

// <bethany-designer:begin id="beth_NamespaceEnd">
} // namespace dialogs
} // namespace notepad
// <bethany-designer:end id="beth_NamespaceEnd" hash="d95a151f">
