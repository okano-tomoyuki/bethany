#pragma once

#include "no_vcl.hpp"

/** Form created with the no_vcl designer (MainForm.nvform.json). Regions enclosed in markers are overwritten when regenerated. */
class TMainForm : public no_vcl::TForm
{
public:
    // <no_vcl-designer:begin id="declarations">
    no_vcl::TEdit* NameEdit;
    no_vcl::TButton* OkButton;
    no_vcl::TPageControl* PageControl1;
    no_vcl::TTabSheet* OptionSheet;
    no_vcl::TCheckBox* WrapCheck;
    no_vcl::TSpinEdit* SizeSpin;
    no_vcl::TTabSheet* MemoSheet;
    no_vcl::TMemo* Memo1;
    no_vcl::TPanel* BottomPanel;
    no_vcl::TLabel* StatusLabel;
    no_vcl::TMainMenu* MainMenu1;
    no_vcl::TMenuItem* FileMenu;
    no_vcl::TMenuItem* FileOpenItem;
    no_vcl::TMenuItem* N1;
    no_vcl::TMenuItem* FileExitItem;
    no_vcl::TPopupMenu* PopupMenu1;
    no_vcl::TMenuItem* ClearItem;
    no_vcl::TOpenDialog* OpenDialog1;
    no_vcl::TTimer* Timer1;

    void FormCreate(no_vcl::TObject* Sender);
    void FormCloseQuery(no_vcl::TObject* Sender, bool& CanClose);
    void NameEditChange(no_vcl::TObject* Sender);
    void OkButtonClick(no_vcl::TObject* Sender);
    void WrapCheckClick(no_vcl::TObject* Sender);
    void FileOpenItemClick(no_vcl::TObject* Sender);
    void FileExitItemClick(no_vcl::TObject* Sender);
    void ClearItemClick(no_vcl::TObject* Sender);
    void Timer1Timer(no_vcl::TObject* Sender);
    // <no_vcl-designer:end id="declarations" hash="7e303dd1">

    explicit TMainForm(no_vcl::TComponent* AOwner);

protected:
    ~TMainForm() override = default;

private:
    void nvd_CreateComponents();
};

extern TMainForm* MainForm;
