from beth import *


class TAboutForm(TForm):
    """Form created with the Bethany designer (AboutForm.bfm.json). Regions enclosed in markers are overwritten when regenerated."""

    def __init__(self, AOwner):
        super().__init__(AOwner)
        # <bethany-designer:begin id="declarations">
        self.TitleLabel: TLabel
        self.InfoLabel: TLabel
        self.OkButton: TButton
        # <bethany-designer:end id="declarations" hash="f2729f13">
        self.beth_CreateComponents()

    # <bethany-designer:begin id="beth_CreateComponents">
    def beth_CreateComponents(self):
        """Creates the components and sets their properties (generated)."""
        self.TitleLabel = TLabel(self)
        self.InfoLabel = TLabel(self)
        self.OkButton = TButton(self)

        self.Width = 360
        self.Height = 150
        self.Caption = "About Bethany Notepad"
        self.BorderStyle = bsDialog
        self.Position = poMainFormCenter

        self.TitleLabel.Parent = self
        self.TitleLabel.Left = 16
        self.TitleLabel.Top = 16
        self.TitleLabel.Width = 150
        self.TitleLabel.Height = 21
        self.TitleLabel.Caption = "Bethany Notepad"
        self.TitleLabel.Font.Size = 12
        self.TitleLabel.Font.Style = fsBold

        self.InfoLabel.Parent = self
        self.InfoLabel.Left = 16
        self.InfoLabel.Top = 48
        self.InfoLabel.Width = 320
        self.InfoLabel.Height = 15
        self.InfoLabel.Caption = "A sample application of Bethany, the Lazarus LCL for C++ and Python."

        self.OkButton.Parent = self
        self.OkButton.Left = 264
        self.OkButton.Top = 104
        self.OkButton.Width = 80
        self.OkButton.Height = 25
        self.OkButton.Caption = "OK"
        self.OkButton.ModalResult = mrOk
        self.OkButton.Default = True
        self.OkButton.Cancel = True
        self.OkButton.Anchors = {akRight, akBottom}
    # <bethany-designer:end id="beth_CreateComponents" hash="52767e88">

    # <bethany-designer:handler-stubs>


# <bethany-designer:begin id="beth_FormVariable">
# The form created at startup by Application.CreateForm in the project (the global variable of the form in C++Builder).
# Other forms use it as "import AboutForm" and "AboutForm.AboutForm" ("from AboutForm import AboutForm" copies None).
AboutForm: "TAboutForm" = None
# <bethany-designer:end id="beth_FormVariable" hash="086d7674">
