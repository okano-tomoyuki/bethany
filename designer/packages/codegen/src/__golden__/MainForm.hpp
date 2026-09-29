// <bethany-designer:begin id="beth_HeaderBegin">
#ifndef MAINFORM_HPP
#define MAINFORM_HPP

#include <bethany/beth.hpp>
// <bethany-designer:end id="beth_HeaderBegin" hash="f2acc4f8">

// <bethany-designer:begin id="beth_NamespaceBegin">
// <bethany-designer:end id="beth_NamespaceBegin" hash="811c9dc5">

/** Form created with the Bethany designer (MainForm.bfm.json). Regions enclosed in markers are overwritten when regenerated. */
class TMainForm : public beth::TForm
{
public:
    // <bethany-designer:begin id="declarations">
    beth::TEdit* NameEdit;
    beth::TButton* OkButton;
    beth::TPageControl* PageControl1;
    beth::TTabSheet* OptionSheet;
    beth::TCheckBox* WrapCheck;
    beth::TSpinEdit* SizeSpin;
    beth::TStatusBar* OptionStatus;
    beth::TTabSheet* MemoSheet;
    beth::TMemo* Memo1;
    beth::TPanel* BottomPanel;
    beth::TLabel* StatusLabel;
    beth::TPanel* HintPanel;
    beth::TMainMenu* MainMenu1;
    beth::TMenuItem* FileMenu;
    beth::TMenuItem* FileOpenItem;
    beth::TMenuItem* FileSaveItem;
    beth::TMenuItem* N1;
    beth::TMenuItem* FileExitItem;
    beth::TPopupMenu* PopupMenu1;
    beth::TMenuItem* ClearItem;
    beth::TOpenDialog* OpenDialog1;
    beth::TTimer* Timer1;
    beth::TActionList* ActionList1;
    beth::TAction* FileSaveAction;

    void FormCreate(beth::TObject* Sender);
    void FormCloseQuery(beth::TObject* Sender, bool& CanClose);
    void FormDropFiles(beth::TObject* Sender, const std::vector<std::string>& FileNames);
    void NameEditChange(beth::TObject* Sender);
    void OkButtonClick(beth::TObject* Sender);
    void WrapCheckClick(beth::TObject* Sender);
    void FileOpenItemClick(beth::TObject* Sender);
    void FileExitItemClick(beth::TObject* Sender);
    void ClearItemClick(beth::TObject* Sender);
    void Timer1Timer(beth::TObject* Sender);
    void FileSaveActionExecute(beth::TObject* Sender);
    // <bethany-designer:end id="declarations" hash="84cd6a91">

    explicit TMainForm(beth::TComponent* AOwner);

protected:
    ~TMainForm() override = default;

private:
    void beth_CreateComponents();
};

extern TMainForm* MainForm;

// <bethany-designer:begin id="beth_HeaderEnd">
#endif // MAINFORM_HPP
// <bethany-designer:end id="beth_HeaderEnd" hash="6ce5221e">
