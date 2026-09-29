// <bethany-designer:begin id="beth_HeaderBegin">
#ifndef NOTEPAD_DIALOGS_ABOUTFORM_HPP
#define NOTEPAD_DIALOGS_ABOUTFORM_HPP

#include <bethany/beth.hpp>
// <bethany-designer:end id="beth_HeaderBegin" hash="60f28b7c">

// <bethany-designer:begin id="beth_NamespaceBegin">
namespace notepad
{
namespace dialogs
{
// <bethany-designer:end id="beth_NamespaceBegin" hash="9e281df7">

/** Form created with the Bethany designer (AboutForm.bfm.json). Regions enclosed in markers are overwritten when regenerated. */
class TAboutForm : public beth::TForm
{
public:
    // <bethany-designer:begin id="declarations">
    beth::TLabel* TitleLabel;
    beth::TLabel* InfoLabel;
    beth::TButton* OkButton;

    void OkButtonClick(beth::TObject* Sender);
    // <bethany-designer:end id="declarations" hash="236cf398">

    explicit TAboutForm(beth::TComponent* AOwner);

protected:
    ~TAboutForm() override = default;

private:
    void beth_CreateComponents();
};

extern TAboutForm* AboutForm;

// <bethany-designer:begin id="beth_HeaderEnd">
} // namespace dialogs
} // namespace notepad

#endif // NOTEPAD_DIALOGS_ABOUTFORM_HPP
// <bethany-designer:end id="beth_HeaderEnd" hash="5671ea04">
