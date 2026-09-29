from beth import *

# 押されたボタン(TConfirmSaveForm.Choice)。ShowModal() から戻った後に呼び出し側が見る(× で閉じたときは CANCEL)
SAVE = "save"
DONT_SAVE = "dont_save"
CANCEL = "cancel"


class TConfirmSaveForm(TForm):
    """Form created with the Bethany designer (ConfirmSaveForm.bfm.json). Regions enclosed in markers are overwritten when regenerated."""

    def __init__(self, AOwner):
        super().__init__(AOwner)
        # <bethany-designer:begin id="declarations">
        self.MessageLabel: TLabel
        self.SaveButton: TButton
        self.DontSaveButton: TButton
        self.CancelButton: TButton
        # <bethany-designer:end id="declarations" hash="0b96e0a3">
        self.beth_CreateComponents()
        self.Choice = CANCEL

    # <bethany-designer:begin id="beth_CreateComponents">
    def beth_CreateComponents(self):
        """Creates the components and sets their properties (generated)."""
        self.MessageLabel = TLabel(self)
        self.SaveButton = TButton(self)
        self.DontSaveButton = TButton(self)
        self.CancelButton = TButton(self)

        self.Width = 380
        self.Height = 120
        self.Caption = "Bethany Notepad"
        self.OnShow = self.FormShow

        self.MessageLabel.Parent = self
        self.MessageLabel.Left = 16
        self.MessageLabel.Top = 16
        self.MessageLabel.Width = 340
        self.MessageLabel.Height = 15
        self.MessageLabel.Caption = "Do you want to save the changes?"

        self.SaveButton.Parent = self
        self.SaveButton.Left = 104
        self.SaveButton.Top = 72
        self.SaveButton.Width = 80
        self.SaveButton.Height = 25
        self.SaveButton.Caption = "&Save"
        self.SaveButton.Anchors = {akRight, akBottom}
        self.SaveButton.OnClick = self.SaveButtonClick

        self.DontSaveButton.Parent = self
        self.DontSaveButton.Left = 192
        self.DontSaveButton.Top = 72
        self.DontSaveButton.Width = 80
        self.DontSaveButton.Height = 25
        self.DontSaveButton.Caption = "Do&n't Save"
        self.DontSaveButton.Anchors = {akRight, akBottom}
        self.DontSaveButton.OnClick = self.DontSaveButtonClick

        self.CancelButton.Parent = self
        self.CancelButton.Left = 280
        self.CancelButton.Top = 72
        self.CancelButton.Width = 80
        self.CancelButton.Height = 25
        self.CancelButton.Caption = "Cancel"
        self.CancelButton.Anchors = {akRight, akBottom}
        self.CancelButton.OnClick = self.CancelButtonClick
    # <bethany-designer:end id="beth_CreateComponents" hash="8a1eafb8">

    # <bethany-designer:handler-stubs>

    def FormShow(self, Sender):
        self.Choice = CANCEL

    def SaveButtonClick(self, Sender):
        self.Choice = SAVE
        self.Close()

    def DontSaveButtonClick(self, Sender):
        self.Choice = DONT_SAVE
        self.Close()

    def CancelButtonClick(self, Sender):
        self.Choice = CANCEL
        self.Close()


# <bethany-designer:begin id="beth_FormVariable">
# The form created at startup by Application.CreateForm in the project (the global variable of the form in C++Builder).
# Other forms use it as "import ConfirmSaveForm" and "ConfirmSaveForm.ConfirmSaveForm" ("from ConfirmSaveForm import ConfirmSaveForm" copies None).
ConfirmSaveForm: "TConfirmSaveForm" = None
# <bethany-designer:end id="beth_FormVariable" hash="66af9d5c">
