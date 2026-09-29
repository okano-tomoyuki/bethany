from beth import *


class TMainForm(TForm):
    """Form created with the Bethany designer (MainForm.bfm.json). Regions enclosed in markers are overwritten when regenerated."""

    def __init__(self, AOwner):
        super().__init__(AOwner)
        # <bethany-designer:begin id="declarations">
        self.NameEdit: TEdit
        self.OkButton: TButton
        self.PageControl1: TPageControl
        self.OptionSheet: TTabSheet
        self.WrapCheck: TCheckBox
        self.SizeSpin: TSpinEdit
        self.OptionStatus: TStatusBar
        self.RadioGroup1: TRadioGroup
        self.ScrollBar1: TScrollBar
        self.MemoSheet: TTabSheet
        self.Memo1: TMemo
        self.BottomPanel: TPanel
        self.StatusLabel: TLabel
        self.HintPanel: TPanel
        self.MainMenu1: TMainMenu
        self.FileMenu: TMenuItem
        self.FileOpenItem: TMenuItem
        self.FileSaveItem: TMenuItem
        self.N1: TMenuItem
        self.FileExitItem: TMenuItem
        self.PopupMenu1: TPopupMenu
        self.ClearItem: TMenuItem
        self.OpenDialog1: TOpenDialog
        self.Timer1: TTimer
        self.ActionList1: TActionList
        self.FileSaveAction: TAction
        # <bethany-designer:end id="declarations" hash="0b488329">
        self.beth_CreateComponents()

    # <bethany-designer:begin id="beth_CreateComponents">
    def beth_CreateComponents(self):
        """Creates the components and sets their properties (generated)."""
        self.NameEdit = TEdit(self)
        self.OkButton = TButton(self)
        self.PageControl1 = TPageControl(self)
        self.OptionSheet = TTabSheet(self)
        self.WrapCheck = TCheckBox(self)
        self.SizeSpin = TSpinEdit(self)
        self.OptionStatus = TStatusBar(self)
        self.RadioGroup1 = TRadioGroup(self)
        self.ScrollBar1 = TScrollBar(self)
        self.MemoSheet = TTabSheet(self)
        self.Memo1 = TMemo(self)
        self.BottomPanel = TPanel(self)
        self.StatusLabel = TLabel(self)
        self.HintPanel = TPanel(self)
        self.MainMenu1 = TMainMenu(self)
        self.FileMenu = TMenuItem(self)
        self.FileOpenItem = TMenuItem(self)
        self.FileSaveItem = TMenuItem(self)
        self.N1 = TMenuItem(self)
        self.FileExitItem = TMenuItem(self)
        self.PopupMenu1 = TPopupMenu(self)
        self.ClearItem = TMenuItem(self)
        self.OpenDialog1 = TOpenDialog(self)
        self.Timer1 = TTimer(self)
        self.ActionList1 = TActionList(self)
        self.FileSaveAction = TAction(self)

        self.Width = 400
        self.Height = 300
        self.Caption = "Sample"
        self.Menu = self.MainMenu1
        self.AllowDropFiles = True
        self.OnCreate = self.FormCreate
        self.OnCloseQuery = self.FormCloseQuery
        self.OnDropFiles = self.FormDropFiles

        self.NameEdit.Parent = self
        self.NameEdit.Left = 16
        self.NameEdit.Top = 16
        self.NameEdit.Width = 280
        self.NameEdit.Height = 23
        self.NameEdit.Hint = "Your name"
        self.NameEdit.ShowHint = True
        self.NameEdit.Anchors = {akTop, akLeft, akRight}
        self.NameEdit.OnChange = self.NameEditChange

        self.OkButton.Parent = self
        self.OkButton.Left = 304
        self.OkButton.Top = 15
        self.OkButton.Width = 75
        self.OkButton.Height = 25
        self.OkButton.Caption = "&OK"
        self.OkButton.Font.Style = fsBold
        self.OkButton.Anchors = {akTop, akRight}
        self.OkButton.OnClick = self.OkButtonClick

        self.PageControl1.Parent = self
        self.PageControl1.Left = 16
        self.PageControl1.Top = 48
        self.PageControl1.Width = 363
        self.PageControl1.Height = 120
        self.PageControl1.Anchors = {akTop, akLeft, akRight}

        self.OptionSheet.PageControl = self.PageControl1
        self.OptionSheet.Caption = "Options"

        self.WrapCheck.Parent = self.OptionSheet
        self.WrapCheck.Left = 8
        self.WrapCheck.Top = 8
        self.WrapCheck.Width = 80
        self.WrapCheck.Height = 19
        self.WrapCheck.Caption = "Word wrap"
        self.WrapCheck.Checked = True
        self.WrapCheck.OnClick = self.WrapCheckClick

        self.SizeSpin.Parent = self.OptionSheet
        self.SizeSpin.Left = 8
        self.SizeSpin.Top = 36
        self.SizeSpin.Width = 80
        self.SizeSpin.Height = 23
        self.SizeSpin.MaxValue = 200
        self.SizeSpin.Value = 150

        self.OptionStatus.Parent = self.OptionSheet
        self.OptionStatus.Left = 0
        self.OptionStatus.Top = 68
        self.OptionStatus.Width = 355
        self.OptionStatus.Height = 24
        self.OptionStatus.SimplePanel = False
        item = self.OptionStatus.Panels.Add()
        item.Text = "Ready"
        item.Width = 120
        item = self.OptionStatus.Panels.Add()
        item.Width = 60
        item.Style = psOwnerDraw
        item = self.OptionStatus.Panels.Add()
        item.Text = "right"
        item.Alignment = taRightJustify
        item.Bevel = pbNone

        self.RadioGroup1.Parent = self.OptionSheet
        self.RadioGroup1.Left = 100
        self.RadioGroup1.Top = 0
        self.RadioGroup1.Width = 120
        self.RadioGroup1.Height = 64
        self.RadioGroup1.Caption = "Size"
        self.RadioGroup1.Items.Add("S")
        self.RadioGroup1.Items.Add("M")
        self.RadioGroup1.Items.Add("L")
        self.RadioGroup1.Items.Add("XL")
        self.RadioGroup1.Columns = 2
        self.RadioGroup1.ColumnLayout = clVerticalThenHorizontal
        self.RadioGroup1.OnSelectionChanged = self.RadioGroup1SelectionChanged

        self.ScrollBar1.Parent = self.OptionSheet
        self.ScrollBar1.Left = 228
        self.ScrollBar1.Top = 8
        self.ScrollBar1.Width = 120
        self.ScrollBar1.Height = 17
        self.ScrollBar1.LargeChange = 10
        self.ScrollBar1.SmallChange = 2
        self.ScrollBar1.OnScroll = self.ScrollBar1Scroll

        self.MemoSheet.PageControl = self.PageControl1
        self.MemoSheet.Caption = "Memo"

        self.Memo1.Parent = self.MemoSheet
        self.Memo1.Left = 0
        self.Memo1.Top = 0
        self.Memo1.Width = 355
        self.Memo1.Height = 92
        self.Memo1.Align = alClient
        self.Memo1.PopupMenu = self.PopupMenu1
        self.Memo1.BorderStyle = bsNone
        self.Memo1.ScrollBars = ssBoth
        self.Memo1.Lines.Add("line 1")
        self.Memo1.Lines.Add("line \"2\"")

        self.BottomPanel.Parent = self
        self.BottomPanel.Left = 0
        self.BottomPanel.Top = 259
        self.BottomPanel.Width = 400
        self.BottomPanel.Height = 41
        self.BottomPanel.Caption = ""
        self.BottomPanel.Align = alBottom
        self.BottomPanel.Color = 0x00C0F0FF  # #FFF0C0
        self.BottomPanel.BorderWidth = 1
        self.BottomPanel.BevelOuter = bvLowered
        self.BottomPanel.BevelInner = bvRaised
        self.BottomPanel.BevelWidth = 2

        self.StatusLabel.Parent = self.BottomPanel
        self.StatusLabel.Left = 8
        self.StatusLabel.Top = 12
        self.StatusLabel.Width = 36
        self.StatusLabel.Height = 17
        self.StatusLabel.Caption = "Ready"
        self.StatusLabel.Font.Size = 10
        self.StatusLabel.Font.Color = clBlue

        self.HintPanel.Parent = self.BottomPanel
        self.HintPanel.Left = 295
        self.HintPanel.Top = 5
        self.HintPanel.Width = 100
        self.HintPanel.Height = 31
        self.HintPanel.Caption = "Hint"
        self.HintPanel.Align = alRight
        self.HintPanel.BorderStyle = bsSingle
        self.HintPanel.Alignment = taRightJustify
        self.HintPanel.VerticalAlignment = taAlignTop
        self.HintPanel.BevelOuter = bvNone

        self.PageControl1.ActivePage = self.MemoSheet

        self.FileMenu.Caption = "&File"
        self.MainMenu1.Items.Add(self.FileMenu)

        self.FileOpenItem.Caption = "&Open..."
        self.FileOpenItem.ShortCut = TextToShortCut("Ctrl+O")
        self.FileOpenItem.OnClick = self.FileOpenItemClick
        self.FileMenu.Add(self.FileOpenItem)

        self.FileMenu.Add(self.FileSaveItem)

        self.N1.Caption = "-"
        self.FileMenu.Add(self.N1)

        self.FileExitItem.Caption = "E&xit"
        self.FileExitItem.OnClick = self.FileExitItemClick
        self.FileMenu.Add(self.FileExitItem)

        self.ClearItem.Caption = "Clear"
        self.ClearItem.OnClick = self.ClearItemClick
        self.PopupMenu1.Items.Add(self.ClearItem)

        self.OpenDialog1.Filter = "Text files|*.txt|All files|*.*"
        self.OpenDialog1.Options = ofEnableSizing | ofViewDetail | ofFileMustExist

        self.Timer1.Interval = 500
        self.Timer1.Enabled = False
        self.Timer1.OnTimer = self.Timer1Timer

        self.FileSaveAction.ActionList = self.ActionList1
        self.FileSaveAction.Category = "File"
        self.FileSaveAction.Caption = "&Save"
        self.FileSaveAction.ShortCut = TextToShortCut("Ctrl+S")
        self.FileSaveAction.OnExecute = self.FileSaveActionExecute

        self.FileSaveItem.Action = self.FileSaveAction
    # <bethany-designer:end id="beth_CreateComponents" hash="79b3fb1a">

    # <bethany-designer:handler-stubs>

    def FormCreate(self, Sender):
        pass

    def FormCloseQuery(self, Sender, CanClose):
        pass

    def FormDropFiles(self, Sender, FileNames):
        pass

    def NameEditChange(self, Sender):
        pass

    def OkButtonClick(self, Sender):
        pass

    def WrapCheckClick(self, Sender):
        pass

    def RadioGroup1SelectionChanged(self, Sender):
        pass

    def ScrollBar1Scroll(self, Sender, ScrollCode, ScrollPos):
        pass

    def FileOpenItemClick(self, Sender):
        pass

    def FileExitItemClick(self, Sender):
        pass

    def ClearItemClick(self, Sender):
        pass

    def Timer1Timer(self, Sender):
        pass

    def FileSaveActionExecute(self, Sender):
        pass


# <bethany-designer:begin id="beth_FormVariable">
# The form created at startup by Application.CreateForm in the project (the global variable of the form in C++Builder).
# Other forms use it as "import MainForm" and "MainForm.MainForm" ("from MainForm import MainForm" copies None).
MainForm: "TMainForm" = None
# <bethany-designer:end id="beth_FormVariable" hash="3fa89a04">
