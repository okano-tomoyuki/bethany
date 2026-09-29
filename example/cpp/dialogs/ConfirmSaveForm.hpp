// <bethany-designer:begin id="beth_HeaderBegin">
#ifndef NOTEPAD_DIALOGS_CONFIRMSAVEFORM_HPP
#define NOTEPAD_DIALOGS_CONFIRMSAVEFORM_HPP

#include <bethany/beth.hpp>
// <bethany-designer:end id="beth_HeaderBegin" hash="6aeb5ef8">

// <bethany-designer:begin id="beth_NamespaceBegin">
namespace notepad
{
namespace dialogs
{
// <bethany-designer:end id="beth_NamespaceBegin" hash="9e281df7">

/** Form created with the Bethany designer (ConfirmSaveForm.bfm.json). Regions enclosed in markers are overwritten when regenerated. */
class TConfirmSaveForm : public beth::TForm
{
public:
    // <bethany-designer:begin id="declarations">
    beth::TLabel* MessageLabel;
    beth::TButton* SaveButton;
    beth::TButton* DontSaveButton;
    beth::TButton* CancelButton;

    void FormShow(beth::TObject* Sender);
    void SaveButtonClick(beth::TObject* Sender);
    void DontSaveButtonClick(beth::TObject* Sender);
    void CancelButtonClick(beth::TObject* Sender);
    // <bethany-designer:end id="declarations" hash="871c0001">

    explicit TConfirmSaveForm(beth::TComponent* AOwner);

    // 押されたボタン。ShowModal() から戻った後に呼び出し側が見る(× で閉じたときは Cancel)
    enum TChoice { Save, DontSave, Cancel };
    TChoice Choice = Cancel;

protected:
    ~TConfirmSaveForm() override = default;

private:
    void beth_CreateComponents();
};

extern TConfirmSaveForm* ConfirmSaveForm;

// <bethany-designer:begin id="beth_HeaderEnd">
} // namespace dialogs
} // namespace notepad

#endif // NOTEPAD_DIALOGS_CONFIRMSAVEFORM_HPP
// <bethany-designer:end id="beth_HeaderEnd" hash="53de3164">
