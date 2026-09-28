#pragma once

#include "beth.hpp"

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
    beth::TTabSheet* MemoSheet;
    beth::TMemo* Memo1;
    beth::TPanel* BottomPanel;
    beth::TLabel* StatusLabel;
    beth::TMainMenu* MainMenu1;
    beth::TMenuItem* FileMenu;
    beth::TMenuItem* FileOpenItem;
    beth::TMenuItem* N1;
    beth::TMenuItem* FileExitItem;
    beth::TPopupMenu* PopupMenu1;
    beth::TMenuItem* ClearItem;
    beth::TOpenDialog* OpenDialog1;
    beth::TTimer* Timer1;

    void FormCreate(beth::TObject* Sender);
    void FormCloseQuery(beth::TObject* Sender, bool& CanClose);
    void NameEditChange(beth::TObject* Sender);
    void OkButtonClick(beth::TObject* Sender);
    void WrapCheckClick(beth::TObject* Sender);
    void FileOpenItemClick(beth::TObject* Sender);
    void FileExitItemClick(beth::TObject* Sender);
    void ClearItemClick(beth::TObject* Sender);
    void Timer1Timer(beth::TObject* Sender);
    // <bethany-designer:end id="declarations" hash="1bec7ee9">

    explicit TMainForm(beth::TComponent* AOwner);

protected:
    ~TMainForm() override = default;

private:
    void beth_CreateComponents();
};

extern TMainForm* MainForm;
