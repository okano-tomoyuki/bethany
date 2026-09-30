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
    beth::TRadioGroup* RadioGroup1;
    beth::TScrollBar* ScrollBar1;
    beth::TShape* Shape1;
    beth::TTabSheet* MemoSheet;
    beth::TMemo* Memo1;
    beth::TTabSheet* ListSheet;
    beth::TListView* List1;
    beth::TTabSheet* GridSheet;
    beth::TStringGrid* Grid1;
    beth::TPanel* BottomPanel;
    beth::TLabel* StatusLabel;
    beth::TPanel* HintPanel;
    beth::TListBox* ColorList;
    beth::TTreeView* Tree1;
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
    void RadioGroup1SelectionChanged(beth::TObject* Sender);
    void ScrollBar1Scroll(beth::TObject* Sender, beth::TScrollCode ScrollCode, int& ScrollPos);
    void List1Compare(beth::TObject* Sender, beth::TListItem* Item1, beth::TListItem* Item2, int Data, int& Compare);
    void List1Data(beth::TObject* Sender, beth::TListItem* Item);
    void List1Edited(beth::TObject* Sender, beth::TListItem* Item, std::string& AValue);
    void Grid1ValidateEntry(beth::TObject* Sender, int ACol, int ARow, const std::string& OldValue, std::string& NewValue);
    void Grid1PrepareCanvas(beth::TObject* Sender, int ACol, int ARow, beth::TGridDrawState AState);
    void Grid1CompareCells(beth::TObject* Sender, int ACol, int ARow, int BCol, int BRow, int& Result);
    void Grid1CheckboxToggled(beth::TObject* Sender, int ACol, int ARow, beth::TCheckBoxState AState);
    void ColorListDrawItem(beth::TObject* Sender, int Index, beth::TRect ARect, beth::TOwnerDrawState State);
    void Tree1Edited(beth::TObject* Sender, beth::TTreeNode* Node, std::string& S);
    void Tree1CustomDrawItem(beth::TObject* Sender, beth::TTreeNode* Node, beth::TCustomDrawState State, bool& DefaultDraw);
    void FileOpenItemClick(beth::TObject* Sender);
    void FileExitItemClick(beth::TObject* Sender);
    void ClearItemClick(beth::TObject* Sender);
    void Timer1Timer(beth::TObject* Sender);
    void FileSaveActionExecute(beth::TObject* Sender);
    // <bethany-designer:end id="declarations" hash="d7df6e7e">

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
