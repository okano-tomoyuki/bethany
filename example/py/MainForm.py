import os

from beth import *

# ほかのフォームは import してモジュールの変数で使う(from ... import ... と書かない。README の Using other forms)
import dialogs.AboutForm


class TMainForm(TForm):
    """Form created with the Bethany designer (MainForm.bfm.json). Regions enclosed in markers are overwritten when regenerated."""

    def __init__(self, AOwner):
        super().__init__(AOwner)
        # <bethany-designer:begin id="declarations">
        self.Memo1: TMemo
        self.StatusBar1: TStatusBar
        self.MainMenu1: TMainMenu
        self.FileMenu: TMenuItem
        self.FileNewItem: TMenuItem
        self.FileOpenItem: TMenuItem
        self.FileSaveItem: TMenuItem
        self.FileSaveAsItem: TMenuItem
        self.N1: TMenuItem
        self.FileExitItem: TMenuItem
        self.FormatMenu: TMenuItem
        self.FormatFontItem: TMenuItem
        self.HelpMenu: TMenuItem
        self.HelpAboutItem: TMenuItem
        self.OpenDialog1: TOpenDialog
        self.SaveDialog1: TSaveDialog
        self.FontDialog1: TFontDialog
        # <bethany-designer:end id="declarations" hash="2f216fa2">
        self.beth_CreateComponents()
        self.FileName = ""  # 開いているファイル(新規なら空)
        self.Modified = False  # 保存していない変更があるか

    # <bethany-designer:begin id="beth_CreateComponents">
    def beth_CreateComponents(self):
        """Creates the components and sets their properties (generated)."""
        self.Memo1 = TMemo(self)
        self.StatusBar1 = TStatusBar(self)
        self.MainMenu1 = TMainMenu(self)
        self.FileMenu = TMenuItem(self)
        self.FileNewItem = TMenuItem(self)
        self.FileOpenItem = TMenuItem(self)
        self.FileSaveItem = TMenuItem(self)
        self.FileSaveAsItem = TMenuItem(self)
        self.N1 = TMenuItem(self)
        self.FileExitItem = TMenuItem(self)
        self.FormatMenu = TMenuItem(self)
        self.FormatFontItem = TMenuItem(self)
        self.HelpMenu = TMenuItem(self)
        self.HelpAboutItem = TMenuItem(self)
        self.OpenDialog1 = TOpenDialog(self)
        self.SaveDialog1 = TSaveDialog(self)
        self.FontDialog1 = TFontDialog(self)

        self.Width = 640
        self.Height = 480
        self.Caption = "Bethany Notepad"
        self.Menu = self.MainMenu1
        self.OnCreate = self.FormCreate
        self.OnCloseQuery = self.FormCloseQuery

        self.Memo1.Parent = self
        self.Memo1.Left = 0
        self.Memo1.Top = 0
        self.Memo1.Width = 640
        self.Memo1.Height = 457
        self.Memo1.Align = alClient
        self.Memo1.ScrollBars = 3
        self.Memo1.OnChange = self.Memo1Change

        self.StatusBar1.Parent = self
        self.StatusBar1.Left = 0
        self.StatusBar1.Top = 457
        self.StatusBar1.Width = 640
        self.StatusBar1.Height = 23
        self.StatusBar1.SimpleText = "Ready"

        self.FileMenu.Caption = "&File"
        self.MainMenu1.Items.Add(self.FileMenu)

        self.FileNewItem.Caption = "&New"
        self.FileNewItem.ShortCut = TextToShortCut("Ctrl+N")
        self.FileNewItem.OnClick = self.FileNewItemClick
        self.FileMenu.Add(self.FileNewItem)

        self.FileOpenItem.Caption = "&Open..."
        self.FileOpenItem.ShortCut = TextToShortCut("Ctrl+O")
        self.FileOpenItem.OnClick = self.FileOpenItemClick
        self.FileMenu.Add(self.FileOpenItem)

        self.FileSaveItem.Caption = "&Save"
        self.FileSaveItem.ShortCut = TextToShortCut("Ctrl+S")
        self.FileSaveItem.OnClick = self.FileSaveItemClick
        self.FileMenu.Add(self.FileSaveItem)

        self.FileSaveAsItem.Caption = "Save &As..."
        self.FileSaveAsItem.OnClick = self.FileSaveAsItemClick
        self.FileMenu.Add(self.FileSaveAsItem)

        self.N1.Caption = "-"
        self.FileMenu.Add(self.N1)

        self.FileExitItem.Caption = "E&xit"
        self.FileExitItem.OnClick = self.FileExitItemClick
        self.FileMenu.Add(self.FileExitItem)

        self.FormatMenu.Caption = "F&ormat"
        self.MainMenu1.Items.Add(self.FormatMenu)

        self.FormatFontItem.Caption = "&Font..."
        self.FormatFontItem.OnClick = self.FormatFontItemClick
        self.FormatMenu.Add(self.FormatFontItem)

        self.HelpMenu.Caption = "&Help"
        self.MainMenu1.Items.Add(self.HelpMenu)

        self.HelpAboutItem.Caption = "&About Bethany Notepad..."
        self.HelpAboutItem.OnClick = self.HelpAboutItemClick
        self.HelpMenu.Add(self.HelpAboutItem)

        self.OpenDialog1.Filter = "Text files (*.txt)|*.txt|All files (*.*)|*.*"
        self.OpenDialog1.Options = ofEnableSizing | ofViewDetail | ofFileMustExist

        self.SaveDialog1.Filter = "Text files (*.txt)|*.txt|All files (*.*)|*.*"
        self.SaveDialog1.DefaultExt = "txt"
        self.SaveDialog1.Options = ofEnableSizing | ofViewDetail | ofOverwritePrompt
    # <bethany-designer:end id="beth_CreateComponents" hash="2b3e5197">

    # <bethany-designer:handler-stubs>

    def FormCreate(self, Sender):
        self.UpdateCaption()

    def FormCloseQuery(self, Sender, CanClose):
        # CanClose は参照渡し(Ref)なので .value に代入する
        CanClose.value = self.ConfirmDiscard()

    def Memo1Change(self, Sender):
        if self.Modified:
            return
        self.Modified = True
        self.UpdateCaption()

    def FileNewItemClick(self, Sender):
        if not self.ConfirmDiscard():
            return
        self.Memo1.Lines.Clear()
        self.FileName = ""
        self.Modified = False
        self.UpdateCaption()

    def FileOpenItemClick(self, Sender):
        if not self.ConfirmDiscard() or not self.OpenDialog1.Execute():
            return
        self.FileName = self.OpenDialog1.FileName
        self.Memo1.Lines.LoadFromFile(self.FileName)
        self.Modified = False  # 読み込みでも OnChange が呼ばれるので、後で戻す
        self.UpdateCaption()

    def FileSaveItemClick(self, Sender):
        self.Save()

    def FileSaveAsItemClick(self, Sender):
        self.SaveAs()

    def FileExitItemClick(self, Sender):
        self.Close()  # FormCloseQuery で変更の保存を聞く

    def FormatFontItemClick(self, Sender):
        self.FontDialog1.Font = self.Memo1.Font
        if self.FontDialog1.Execute():
            self.Memo1.Font = self.FontDialog1.Font

    def HelpAboutItemClick(self, Sender):
        # 起動時に作ったほかのフォームは、フォームの変数で使う(C++Builder と同じ)
        dialogs.AboutForm.AboutForm.ShowModal()

    # ---- ファイルの操作(デザイナーが生成しないメソッド) ----

    def DisplayName(self):
        return os.path.basename(self.FileName) if self.FileName else "Untitled"

    def UpdateCaption(self):
        self.Caption = ("*" if self.Modified else "") + self.DisplayName() + " - Bethany Notepad"
        self.StatusBar1.SimpleText = self.FileName or "New file"

    def Save(self):
        if not self.FileName:
            return self.SaveAs()
        self.Memo1.Lines.SaveToFile(self.FileName)
        self.Modified = False
        self.UpdateCaption()
        return True

    def SaveAs(self):
        self.SaveDialog1.FileName = self.FileName
        if not self.SaveDialog1.Execute():
            return False
        self.FileName = self.SaveDialog1.FileName
        return self.Save()

    def ConfirmDiscard(self):
        """変更を保存するか聞く。続けてよければ True(保存した・保存しない)、取りやめなら False"""
        if not self.Modified:
            return True
        answer = MessageDlg("Bethany Notepad", "Do you want to save the changes to " + self.DisplayName() + "?",
                            mtConfirmation, mbYesNoCancel)
        if answer == mrYes:
            return self.Save()
        return answer == mrNo  # mrCancel(× で閉じたときも)なら取りやめ


# <bethany-designer:begin id="beth_FormVariable">
# The form created at startup by Application.CreateForm in the project (the global variable of the form in C++Builder).
# Other forms use it as "import MainForm" and "MainForm.MainForm" ("from MainForm import MainForm" copies None).
MainForm: "TMainForm" = None
# <bethany-designer:end id="beth_FormVariable" hash="3fa89a04">
