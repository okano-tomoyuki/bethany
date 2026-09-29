// <bethany-designer:begin id="beth_HeaderBegin">
#ifndef NOTEPAD_MAINFORM_HPP
#define NOTEPAD_MAINFORM_HPP

#include <bethany/beth.hpp>
// <bethany-designer:end id="beth_HeaderBegin" hash="54a4bba8">

#include <string>

// <bethany-designer:begin id="beth_NamespaceBegin">
namespace notepad
{
// <bethany-designer:end id="beth_NamespaceBegin" hash="3be6c88e">

/** Form created with the Bethany designer (MainForm.bfm.json). Regions enclosed in markers are overwritten when regenerated. */
class TMainForm : public beth::TForm
{
public:
    // <bethany-designer:begin id="declarations">
    beth::TMemo* Memo1;
    beth::TStatusBar* StatusBar1;
    beth::TMainMenu* MainMenu1;
    beth::TMenuItem* FileMenu;
    beth::TMenuItem* FileNewItem;
    beth::TMenuItem* FileOpenItem;
    beth::TMenuItem* FileSaveItem;
    beth::TMenuItem* FileSaveAsItem;
    beth::TMenuItem* N1;
    beth::TMenuItem* FileExitItem;
    beth::TMenuItem* FormatMenu;
    beth::TMenuItem* FormatFontItem;
    beth::TMenuItem* HelpMenu;
    beth::TMenuItem* HelpAboutItem;
    beth::TOpenDialog* OpenDialog1;
    beth::TSaveDialog* SaveDialog1;
    beth::TFontDialog* FontDialog1;

    void FormCreate(beth::TObject* Sender);
    void FormCloseQuery(beth::TObject* Sender, bool& CanClose);
    void Memo1Change(beth::TObject* Sender);
    void FileNewItemClick(beth::TObject* Sender);
    void FileOpenItemClick(beth::TObject* Sender);
    void FileSaveItemClick(beth::TObject* Sender);
    void FileSaveAsItemClick(beth::TObject* Sender);
    void FileExitItemClick(beth::TObject* Sender);
    void FormatFontItemClick(beth::TObject* Sender);
    void HelpAboutItemClick(beth::TObject* Sender);
    // <bethany-designer:end id="declarations" hash="3c2fecfc">

    explicit TMainForm(beth::TComponent* AOwner);

private:
    std::string FileName;   // 開いているファイル(新規なら空)
    bool Modified = false;  // 保存していない変更があるか

    std::string DisplayName() const;
    void UpdateCaption();
    bool Save();
    bool SaveAs();
    // 変更を保存するか聞く。続けてよければ true(保存した・保存しない)、取りやめなら false
    bool ConfirmDiscard();

protected:
    ~TMainForm() override = default;

private:
    void beth_CreateComponents();
};

extern TMainForm* MainForm;

// <bethany-designer:begin id="beth_HeaderEnd">
} // namespace notepad

#endif // NOTEPAD_MAINFORM_HPP
// <bethany-designer:end id="beth_HeaderEnd" hash="6ecdd49f">
