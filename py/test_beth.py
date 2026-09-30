"""Bethany の Python の公開 API(beth.py)のテスト。test/main.cpp(C++ ラッパーのテスト)の移植で、同じ書式で出力する
(C++ 版の出力と突き合わせられるように)。C++ との違い:
- C++ のデストラクタが出力する行(~TMainForm 等)は無い。ラッパーが破棄済みかは ReferenceError で確かめる。
- ハンドラから送出した Python の例外のクラス名は、Python の型名(C++ の std::exception の代わりに RuntimeError 等)。
"""
import ctypes
import os

from beth import *


def pr(s):
    print(s, flush=True)


def yn(b):
    return "yes" if b else "no"


def color(v):
    return f"{v & 0xFFFFFFFF:06X}"


def is_freed(obj):
    try:
        obj.Handle
        return False
    except ReferenceError:
        return True


# ラッパーが実際に破棄済みになったかを数えるためのラベル。
traced_labels = []


class TTracedLabel(TLabel):
    def __init__(self, AOwner):
        super().__init__(AOwner)
        traced_labels.append(self)


def destroyed_labels():
    return sum(1 for label in traced_labels if is_freed(label))


# C++Builder のフォームユニットと同じく、フォームはグローバル変数で持つ。
Form1 = None


class TSubForm(TForm):
    """閉じ方の確認用のサブフォーム。
    - Release me ボタン: 自身のボタンのハンドラの中から Release() する(Free() と違い安全)。
    - ウィンドウを閉じる: OnClose で Action = caFree にし、LCL に Release させる。"""

    def __init__(self, AOwner):
        super().__init__(AOwner)
        self.Caption = "Sub form"
        self.Width = 260
        self.Height = 120

        self.ReleaseButton = TButton(self)
        self.ReleaseButton.Parent = self
        self.ReleaseButton.Caption = "Release me"
        self.ReleaseButton.Left = 20
        self.ReleaseButton.Top = 20
        self.ReleaseButton.Width = 120
        self.ReleaseButton.OnClick = self.ReleaseButtonClick

        # 直接生成したフォームの OnCreate は、最初に表示される直前に呼ばれる。
        self.OnCreate = lambda Sender: pr(f"TSubForm OnCreate: Sender is this: {yn(Sender is self)}")
        self.OnClose = self.SubFormClose
        self.OnActivate = lambda Sender: pr("TSubForm OnActivate")
        self.OnDeactivate = lambda Sender: pr("TSubForm OnDeactivate")
        self.OnHide = lambda Sender: pr("TSubForm OnHide")
        self.OnDestroy = lambda Sender: pr(f"TSubForm OnDestroy: ReleaseButton->Caption={self.ReleaseButton.Caption}")

    def ReleaseButtonClick(self, Sender):
        pr("TSubForm: Release() from its own button")
        self.Release()

    def SubFormClose(self, Sender, Action):
        pr(f"TSubForm OnClose: default Action={int(Action.value)}, set caFree")
        Action.value = caFree


class TMainForm(TForm):
    """デザイナーが生成することを想定した形のフォーム。コントロールは属性として持ち、__init__ で生成する(破棄は Owner に任せる)。
    イベントハンドラはメソッドにし、そのまま代入する。"""

    def __init__(self, AOwner):
        super().__init__(AOwner)
        self.clicks_ = 0
        self.ticks_ = 0
        self.closeAttempts_ = 0
        self.deletedNodes_ = 0
        self.deletedListItems_ = 0
        self.drawnCells_ = 0
        self.pictureChanges_ = 0
        self.imageListChanges_ = 0
        self.StatusBar1 = None

        self.Caption = "Bethany C++ wrapper"
        self.Width = 640
        self.Height = 930

        self.Button1 = TButton(self)
        self.Button1.Parent = self
        self.Button1.Caption = "Click me"
        self.Button1.Left = 20
        self.Button1.Top = 20
        self.Button1.Width = 100
        self.Button1.Height = 30
        self.Button1.OnClick = self.Button1Click
        self.Button1.OnDblClick = self.Button1DblClick
        self.Button1.OnMouseDown = self.Button1MouseDown
        self.Button1.OnMouseUp = self.Button1MouseUp
        self.Button1.OnMouseEnter = self.Button1MouseEnter
        self.Button1.OnMouseLeave = self.Button1MouseLeave

        self.Label1 = TLabel(self)
        self.Label1.Parent = self
        self.Label1.Caption = "Label text"
        self.Label1.Left = 20
        self.Label1.Top = 60

        self.Edit1 = TEdit(self)
        self.Edit1.Parent = self
        self.Edit1.Text = "Edit me"
        self.Edit1.Left = 20
        self.Edit1.Top = 90
        self.Edit1.Width = 150
        self.Edit1.OnChange = self.TextChange
        self.Edit1.OnKeyDown = self.Edit1KeyDown
        self.Edit1.OnKeyPress = self.Edit1KeyPress

        self.CheckBox1 = TCheckBox(self)
        self.CheckBox1.Parent = self
        self.CheckBox1.Caption = "Check me"
        self.CheckBox1.Left = 20
        self.CheckBox1.Top = 130
        self.CheckBox1.OnClick = self.CheckBox1Click

        # 2つのラジオボタンで同じハンドラを共有し、Sender でどちらが押されたかを区別する。
        self.RadioButton1 = TRadioButton(self)
        self.RadioButton1.Parent = self
        self.RadioButton1.Caption = "Option A"
        self.RadioButton1.Left = 20
        self.RadioButton1.Top = 160
        self.RadioButton1.Checked = True
        self.RadioButton1.OnClick = self.RadioButtonClick

        self.RadioButton2 = TRadioButton(self)
        self.RadioButton2.Parent = self
        self.RadioButton2.Caption = "Option B"
        self.RadioButton2.Left = 20
        self.RadioButton2.Top = 190
        self.RadioButton2.OnClick = self.RadioButton1.OnClick

        self.Panel1 = TPanel(self)
        self.Panel1.Parent = self
        self.Panel1.Left = 220
        self.Panel1.Top = 20
        self.Panel1.Width = 180
        self.Panel1.Height = 60

        # Panel の中にボタンを置く(Parent が TWinControl なので Panel も親にできる)。
        self.PanelButton = TButton(self)
        self.PanelButton.Parent = self.Panel1
        # パネルへの描画(TCustomControl.Canvas・OnPaint。docs/adr/0045)。ボタンの右の空きに図形を描く
        self.Panel1.OnPaint = self.Panel1Paint
        self.PanelButton.Caption = "In panel"
        self.PanelButton.Left = 10
        self.PanelButton.Top = 15
        self.PanelButton.OnClick = self.PanelButtonClick

        self.GroupBox1 = TGroupBox(self)
        self.GroupBox1.Parent = self
        self.GroupBox1.Caption = "Group"
        self.GroupBox1.Left = 220
        self.GroupBox1.Top = 90
        self.GroupBox1.Width = 180
        self.GroupBox1.Height = 60

        self.ComboBox1 = TComboBox(self)
        self.ComboBox1.Parent = self
        self.ComboBox1.Items.Add("Combo A")
        self.ComboBox1.Items.Add("Combo B")
        self.ComboBox1.Items.Add("Combo C")
        self.ComboBox1.ItemIndex = 0
        self.ComboBox1.Left = 220
        self.ComboBox1.Top = 160
        self.ComboBox1.Width = 150
        self.ComboBox1.OnChange = self.TextChange

        self.ListBox1 = TListBox(self)
        self.ListBox1.Parent = self
        self.ListBox1.Items.Add("List 1")
        self.ListBox1.Items.Add("List 2")
        self.ListBox1.Items.Add("List 3")
        # ウィンドウを作る前の ListBox1.Items の中身のハンドル(表示後に LCL が差し替えることの確認用)。
        self.listBoxItemsBeforeShow_ = self.ListBox1.Items.Handle
        self.ListBox1.Left = 220
        self.ListBox1.Top = 190
        self.ListBox1.Width = 150
        self.ListBox1.Height = 80
        self.ListBox1.OnClick = self.ListBox1Click

        # TMemo は TCustomEdit の派生なので、Edit1 と同じ TextChange を共有できる。
        self.Memo1 = TMemo(self)
        self.Memo1.Parent = self
        self.Memo1.Lines.Add("Memo line 1")
        self.Memo1.Lines.Add("Memo line 2")
        self.Memo1.Left = 220
        self.Memo1.Top = 280
        self.Memo1.Width = 150
        self.Memo1.Height = 80
        self.Memo1.OnChange = self.Edit1.OnChange

        self.TickLabel = TTracedLabel(self)
        self.TickLabel.Parent = self
        self.TickLabel.Caption = "Tick: 0"
        self.TickLabel.Left = 20
        self.TickLabel.Top = 230

        self.Timer1 = TTimer(self)
        self.Timer1.Interval = 500
        self.Timer1.OnTimer = self.Timer1Timer
        self.Timer1.Enabled = True

        self.PaintBox1 = TPaintBox(self)
        self.PaintBox1.Parent = self
        self.PaintBox1.Left = 400
        self.PaintBox1.Top = 20
        self.PaintBox1.Width = 220
        self.PaintBox1.Height = 130
        self.PaintBox1.OnPaint = self.PaintBox1Paint

        self.OpenSubButton = TButton(self)
        self.OpenSubButton.Parent = self
        self.OpenSubButton.Caption = "Open sub"
        self.OpenSubButton.Left = 400
        self.OpenSubButton.Top = 170
        self.OpenSubButton.OnClick = self.OpenSubButtonClick

        self.ScrollBox1 = TScrollBox(self)
        self.ScrollBox1.Parent = self
        self.ScrollBox1.Left = 20
        self.ScrollBox1.Top = 380
        self.ScrollBox1.Width = 180
        self.ScrollBox1.Height = 50

        # TScrollBox もウィンドウを持つコントロールなので、Parent として子を配置できる。
        self.ScrolledButton = TButton(self)
        self.ScrolledButton.Parent = self.ScrollBox1
        self.ScrolledButton.Caption = "Inside ScrollBox"
        self.ScrolledButton.Left = 10
        self.ScrolledButton.Top = 10
        self.ScrolledButton.Width = 140

        self.ToggleBox1 = TToggleBox(self)
        self.ToggleBox1.Parent = self
        self.ToggleBox1.Caption = "Toggle me"
        self.ToggleBox1.Left = 220
        self.ToggleBox1.Top = 380
        self.ToggleBox1.OnClick = self.ToggleBox1Click

        self.Bevel1 = TBevel(self)
        self.Bevel1.Parent = self
        self.Bevel1.Left = 340
        self.Bevel1.Top = 380
        self.Bevel1.Width = 100
        self.Bevel1.Height = 50
        self.Bevel1.Shape = bsFrame
        self.Bevel1.Style = bsRaised

        self.Shape1 = TShape(self)
        self.Shape1.Parent = self
        self.Shape1.Left = 460
        self.Shape1.Top = 380
        self.Shape1.Width = 60
        self.Shape1.Height = 50
        self.Shape1.Shape = stEllipse
        self.Shape1.Brush.Color = clYellow
        self.Shape1.Pen.Color = clBlue

        self.StaticText1 = TStaticText(self)
        self.StaticText1.Parent = self
        self.StaticText1.Caption = "Static text"
        self.StaticText1.Left = 20
        self.StaticText1.Top = 440
        self.StaticText1.Width = 150
        self.StaticText1.BorderStyle = sbsSunken

        self.ScrollBar1 = TScrollBar(self)
        self.ScrollBar1.Parent = self
        self.ScrollBar1.Left = 20
        self.ScrollBar1.Top = 480
        self.ScrollBar1.Width = 150
        self.ScrollBar1.Min = 0
        self.ScrollBar1.Max = 100
        self.ScrollBar1.Position = 30
        self.ScrollBar1.OnChange = self.ScrollBar1Change

        self.TrackBar1 = TTrackBar(self)
        self.TrackBar1.Parent = self
        self.TrackBar1.Left = 190
        self.TrackBar1.Top = 480
        self.TrackBar1.Width = 150
        self.TrackBar1.Min = 0
        self.TrackBar1.Max = 10
        self.TrackBar1.Position = 5
        self.TrackBar1.OnChange = self.TrackBar1Change

        self.ProgressBar1 = TProgressBar(self)
        self.ProgressBar1.Parent = self
        self.ProgressBar1.Left = 360
        self.ProgressBar1.Top = 480
        self.ProgressBar1.Width = 150
        self.ProgressBar1.Min = 0
        self.ProgressBar1.Max = 100
        self.ProgressBar1.Position = 42

        self.UpDownEdit = TEdit(self)
        self.UpDownEdit.Parent = self
        self.UpDownEdit.Text = "3"
        self.UpDownEdit.Left = 20
        self.UpDownEdit.Top = 510
        self.UpDownEdit.Width = 60

        self.UpDown1 = TUpDown(self)
        self.UpDown1.Parent = self
        self.UpDown1.Left = 80
        self.UpDown1.Top = 510
        self.UpDown1.Min = 0
        self.UpDown1.Max = 10
        self.UpDown1.Position = 3
        self.UpDown1.Increment = 1
        self.UpDown1.Associate = self.UpDownEdit

        self.RadioGroup1 = TRadioGroup(self)
        self.RadioGroup1.Parent = self
        self.RadioGroup1.Caption = "RadioGroup1"
        self.RadioGroup1.Left = 20
        self.RadioGroup1.Top = 550
        self.RadioGroup1.Width = 180
        self.RadioGroup1.Height = 90
        self.RadioGroup1.Items.Add("Option A")
        self.RadioGroup1.Items.Add("Option B")
        self.RadioGroup1.Items.Add("Option C")
        self.RadioGroup1.ItemIndex = 1
        self.RadioGroup1.OnClick = self.RadioGroup1Click

        self.CheckGroup1 = TCheckGroup(self)
        self.CheckGroup1.Parent = self
        self.CheckGroup1.Caption = "CheckGroup1"
        self.CheckGroup1.Left = 210
        self.CheckGroup1.Top = 550
        self.CheckGroup1.Width = 180
        self.CheckGroup1.Height = 90
        self.CheckGroup1.Items.Add("Feature X")
        self.CheckGroup1.Items.Add("Feature Y")
        self.CheckGroup1.Items.Add("Feature Z")
        self.CheckGroup1.Checked[0] = True
        self.CheckGroup1.Checked[2] = True

        self.CheckListBox1 = TCheckListBox(self)
        self.CheckListBox1.Parent = self
        self.CheckListBox1.Left = 400
        self.CheckListBox1.Top = 550
        self.CheckListBox1.Width = 180
        self.CheckListBox1.Height = 90
        self.CheckListBox1.Items.Add("Item 1")
        self.CheckListBox1.Items.Add("Item 2")
        self.CheckListBox1.Items.Add("Item 3")
        self.CheckListBox1.Checked[1] = True
        self.CheckListBox1.OnClickCheck = self.CheckListBox1ClickCheck

        self.SpeedButton1 = TSpeedButton(self)
        self.SpeedButton1.Parent = self
        self.SpeedButton1.Caption = "Speed"
        self.SpeedButton1.Left = 20
        self.SpeedButton1.Top = 650
        self.SpeedButton1.Width = 80
        self.SpeedButton1.GroupIndex = 1
        self.SpeedButton1.OnClick = self.SpeedButton1Click

        self.BitBtn1 = TBitBtn(self)
        self.BitBtn1.Parent = self
        self.BitBtn1.Left = 120
        self.BitBtn1.Top = 650
        self.BitBtn1.Kind = bkOK

        self.FloatSpinEdit1 = TFloatSpinEdit(self)
        self.FloatSpinEdit1.Parent = self
        self.FloatSpinEdit1.Left = 20
        self.FloatSpinEdit1.Top = 690
        self.FloatSpinEdit1.Width = 100
        self.FloatSpinEdit1.MinValue = 0.0
        self.FloatSpinEdit1.MaxValue = 10.0
        self.FloatSpinEdit1.Increment = 0.5
        self.FloatSpinEdit1.DecimalPlaces = 1
        self.FloatSpinEdit1.Value = 2.5

        self.SpinEdit1 = TSpinEdit(self)
        self.SpinEdit1.Parent = self
        self.SpinEdit1.Left = 130
        self.SpinEdit1.Top = 690
        self.SpinEdit1.Width = 100
        self.SpinEdit1.MinValue = 0
        self.SpinEdit1.MaxValue = 100
        self.SpinEdit1.Increment = 5
        self.SpinEdit1.Value = 42

        self.MaskEdit1 = TMaskEdit(self)
        self.MaskEdit1.Parent = self
        self.MaskEdit1.Left = 240
        self.MaskEdit1.Top = 690
        self.MaskEdit1.Width = 100
        self.MaskEdit1.EditMask = "000-0000;1;_"

        # TLabeledEdit。EditLabel は LCL が内部で生成したラベル。Parent を設定すると、ラベルも同じ親に置かれる。
        self.LabeledEdit1 = TLabeledEdit(self)
        self.LabeledEdit1.Parent = self
        self.LabeledEdit1.Left = 420
        self.LabeledEdit1.Top = 690
        self.LabeledEdit1.Width = 100
        self.LabeledEdit1.LabelPosition = lpLeft
        self.LabeledEdit1.LabelSpacing = 6
        self.LabeledEdit1.EditLabel.Caption = "Zip:"

        self.TabControl1 = TTabControl(self)
        self.TabControl1.Parent = self
        self.TabControl1.Left = 20
        self.TabControl1.Top = 730
        self.TabControl1.Width = 300
        self.TabControl1.Height = 90
        self.TabControl1.Tabs.Add("Tab A")
        self.TabControl1.Tabs.Add("Tab B")
        self.TabControl1.Tabs.Add("Tab C")
        self.TabControl1.TabIndex = 0
        self.TabControl1.OnChange = self.TabControl1Change

        self.StatusBar1 = TStatusBar(self)
        self.StatusBar1.Parent = self
        self.StatusBar1.SimpleText = "Ready"

        # TControl.Align と TSplitter。LayoutPanel の中を、上端の alTop、左の alLeft + Splitter、残りの alClient で分ける。
        self.LayoutPanel = TPanel(self)
        self.LayoutPanel.Parent = self
        self.LayoutPanel.Left = 340
        self.LayoutPanel.Top = 730
        self.LayoutPanel.Width = 280
        self.LayoutPanel.Height = 90
        self.LayoutPanel.Caption = ""

        self.AlignTopPanel = TPanel(self)
        self.AlignTopPanel.Parent = self.LayoutPanel
        self.AlignTopPanel.Align = alTop
        self.AlignTopPanel.Height = 20
        self.AlignTopPanel.Caption = "alTop"

        self.AlignLeftPanel = TPanel(self)
        self.AlignLeftPanel.Parent = self.LayoutPanel
        self.AlignLeftPanel.Align = alLeft
        self.AlignLeftPanel.Width = 80
        self.AlignLeftPanel.Caption = "alLeft"

        self.Splitter1 = TSplitter(self)
        self.Splitter1.Left = 100
        self.Splitter1.Parent = self.LayoutPanel
        self.Splitter1.MinSize = 40
        self.Splitter1.Beveled = True
        self.Splitter1.OnMoved = self.Splitter1Moved

        self.AlignClientPanel = TPanel(self)
        self.AlignClientPanel.Parent = self.LayoutPanel
        self.AlignClientPanel.Align = alClient
        self.AlignClientPanel.Caption = "alClient"

        # Anchors・BorderSpacing(docs/adr/0034)。Align と同じく、LCL ではフォームが表示されるまで配置されない。
        # AnchoredButton は左右の辺に付けるので、AlignClientPanel の幅が変わると同じだけ幅が変わる。
        self.AnchoredButton = TButton(self)
        self.AnchoredButton.Parent = self.AlignClientPanel
        self.AnchoredButton.Left = 5
        self.AnchoredButton.Top = 5
        self.AnchoredButton.Width = 60
        self.AnchoredButton.Height = 22
        self.AnchoredButton.Caption = "Anchored"
        self.AnchoredButton.Anchors = {akLeft, akTop, akRight}
        # SpacedButton は下端に寄せ、周りに 4px の余白を空ける。
        self.SpacedButton = TButton(self)
        self.SpacedButton.Parent = self.AlignClientPanel
        self.SpacedButton.Height = 22
        self.SpacedButton.Caption = "Spaced"
        self.SpacedButton.Align = alBottom
        self.SpacedButton.BorderSpacing.Around = 4

        # メニュー。項目の Owner はフォームにし、親子関係は Add で組む。
        self.MainMenu1 = TMainMenu(self)

        self.FileMenu = TMenuItem(self)
        self.FileMenu.Caption = "&File"
        self.MainMenu1.Items.Add(self.FileMenu)

        self.FileNewItem = TMenuItem(self)
        self.FileNewItem.Caption = "&New"
        self.FileNewItem.ShortCut = ShortCut(ord("N"), ssCtrl)
        self.FileNewItem.OnClick = self.FileNewItemClick
        self.FileMenu.Add(self.FileNewItem)

        self.FileMenu.AddSeparator()

        self.FileExitItem = TMenuItem(self)
        self.FileExitItem.Caption = "E&xit"
        self.FileExitItem.OnClick = lambda Sender: Application.Terminate()
        self.FileMenu.Add(self.FileExitItem)

        self.ViewMenu = TMenuItem(self)
        self.ViewMenu.Caption = "&View"
        self.MainMenu1.Items.Add(self.ViewMenu)

        self.ViewStatusBarItem = TMenuItem(self)
        self.ViewStatusBarItem.Caption = "&Status bar"
        self.ViewStatusBarItem.AutoCheck = True
        self.ViewStatusBarItem.Checked = True
        self.ViewStatusBarItem.OnClick = lambda Sender: setattr(self.StatusBar1, "Visible", self.ViewStatusBarItem.Checked)
        self.ViewMenu.Add(self.ViewStatusBarItem)

        self.ViewMenu.AddSeparator()

        # 同じ GroupIndex の RadioItem は、どれか 1 つだけが Checked になる。
        self.ViewSmallItem = TMenuItem(self)
        self.ViewSmallItem.Caption = "S&mall"
        self.ViewSmallItem.RadioItem = True
        self.ViewSmallItem.GroupIndex = 1
        self.ViewSmallItem.AutoCheck = True
        self.ViewSmallItem.Checked = True
        self.ViewMenu.Add(self.ViewSmallItem)

        self.ViewLargeItem = TMenuItem(self)
        self.ViewLargeItem.Caption = "&Large"
        self.ViewLargeItem.RadioItem = True
        self.ViewLargeItem.GroupIndex = 1
        self.ViewLargeItem.AutoCheck = True
        self.ViewMenu.Add(self.ViewLargeItem)

        self.Menu = self.MainMenu1

        # Panel1 を右クリックすると開くメニュー。
        self.PopupMenu1 = TPopupMenu(self)
        self.PopupHelloItem = TMenuItem(self)
        self.PopupHelloItem.Caption = "Say hello"
        self.PopupHelloItem.OnClick = lambda Sender: pr("PopupHelloItem clicked")
        self.PopupMenu1.Items.Add(self.PopupHelloItem)
        self.PopupMenu1.OnPopup = self.PopupMenu1Popup
        self.Panel1.PopupMenu = self.PopupMenu1

        # Tier 4(ダイアログ)。VCL と同じく、フォームを Owner にして生成し、メニューから Execute する。
        self.OpenDialog1 = TOpenDialog(self)
        self.OpenDialog1.Title = "Open a text file"
        self.OpenDialog1.Filter = "Text files (*.txt;*.md)|*.txt;*.md|All files (*.*)|*.*"
        self.OpenDialog1.Options = self.OpenDialog1.Options | ofFileMustExist
        self.OpenDialog1.OnShow = lambda Sender: pr(f"OpenDialog1 OnShow: Sender is OpenDialog1: {yn(Sender is self.OpenDialog1)}")
        self.OpenDialog1.OnClose = lambda Sender: pr("OpenDialog1 OnClose")
        self.OpenDialog1.OnCanClose = self.OpenDialog1CanClose
        self.SaveDialog1 = TSaveDialog(self)
        self.SaveDialog1.Filter = self.OpenDialog1.Filter
        self.SaveDialog1.DefaultExt = "txt"
        self.SaveDialog1.Options = self.SaveDialog1.Options | ofOverwritePrompt
        self.SelectDirectoryDialog1 = TSelectDirectoryDialog(self)
        self.ColorDialog1 = TColorDialog(self)
        self.FontDialog1 = TFontDialog(self)
        self.FindDialog1 = TFindDialog(self)
        self.FindDialog1.OnFind = self.FindDialog1Find
        self.ReplaceDialog1 = TReplaceDialog(self)
        self.ReplaceDialog1.OnFind = self.FindDialog1Find
        self.ReplaceDialog1.OnReplace = self.ReplaceDialog1Replace

        self.DialogsMenu = TMenuItem(self)
        self.DialogsMenu.Caption = "&Dialogs"
        self.MainMenu1.Items.Add(self.DialogsMenu)
        self.AddDialogItem("&Open... (into Memo1)", self.OpenItemClick)
        self.AddDialogItem("&Save... (Memo1)", self.SaveItemClick)
        self.AddDialogItem("Select &directory...", self.SelectDirectoryItemClick)
        self.DialogsMenu.AddSeparator()
        self.AddDialogItem("&Color... (Panel1)", self.ColorItemClick)
        self.AddDialogItem("&Font... (Memo1)", self.FontItemClick)
        self.DialogsMenu.AddSeparator()
        # TFindDialog・TReplaceDialog はモードレス(Execute はすぐ戻り、ボタンが押されるたびに OnFind・OnReplace が呼ばれる)。
        self.AddDialogItem("F&ind in Memo1...", lambda: self.FindDialog1.Execute())
        self.AddDialogItem("&Replace in Memo1...", lambda: self.ReplaceDialog1.Execute())

        # TPageControl + TTabSheet。
        self.PageControl1 = TPageControl(self)
        self.PageControl1.Parent = self
        self.PageControl1.Left = 400
        self.PageControl1.Top = 200
        self.PageControl1.Width = 220
        self.PageControl1.Height = 160

        # VCL と同じく、TTabSheet を生成して PageControl を設定する。
        self.TabSheet1 = TTabSheet(self)
        self.TabSheet1.PageControl = self.PageControl1
        self.TabSheet1.Caption = "Page 1"
        pageLabel = TLabel(self)
        pageLabel.Parent = self.TabSheet1
        pageLabel.Left = 10
        pageLabel.Top = 10
        pageLabel.Caption = "On page 1"

        # AddTabSheet のページは LCL が生成する(ラッパーは初回の取得時に作られる)。
        self.TabSheet2 = self.PageControl1.AddTabSheet()
        self.TabSheet2.Caption = "Page 2"
        pageButton = TButton(self)
        pageButton.Parent = self.TabSheet2
        pageButton.Left = 10
        pageButton.Top = 10
        pageButton.Caption = "On page 2"

        self.TabSheet3 = TTabSheet(self)
        self.TabSheet3.PageControl = self.PageControl1
        self.TabSheet3.Caption = "Hidden"
        self.TabSheet3.TabVisible = False

        self.PageControl1.ActivePage = self.TabSheet1
        self.PageControl1.OnChanging = lambda Sender, AllowChange: pr(f"PageControl1Changing (AllowChange={int(AllowChange.value)})")
        self.PageControl1.OnChange = self.PageControl1Change
        # 表示前に設定した ListView1 の選択が、ウィンドウハンドルができた後も保たれていることの確認を兼ねる。
        self.TabSheet2.OnShow = self.TabSheet2Show

        # TTreeView。TabSheet1 の上に置く。
        self.TreeView1 = TTreeView(self)
        self.TreeView1.Parent = self.TabSheet1
        self.TreeView1.Left = 10
        self.TreeView1.Top = 30
        self.TreeView1.Width = 190
        self.TreeView1.Height = 95
        self.RootNode = self.TreeView1.Items.Add(None, "Root")
        self.Child1Node = self.TreeView1.Items.AddChild(self.RootNode, "Child 1")
        self.Child2Node = self.TreeView1.Items.AddChild(self.RootNode, "Child 2")
        self.GrandchildNode = self.TreeView1.Items.AddChild(self.Child1Node, "Grandchild")
        self.Root2Node = self.TreeView1.Items.Add(self.RootNode, "Root 2")
        self.RootNode.Expanded = True
        self.TreeView1.OnChange = lambda Sender, Node: pr(f"TreeView1Change: {Node.Text if Node else '(none)'}")
        self.TreeView1.OnChanging = lambda Sender, Node, AllowChange: pr(
            f"TreeView1Changing: to {Node.Text} (AllowChange={int(AllowChange.value)})")
        self.TreeView1.OnExpanded = lambda Sender, Node: pr(f"TreeView1Expanded: {Node.Text}")
        # 名前が "Locked" のノードは折りたためないようにする。
        self.TreeView1.OnCollapsing = self.TreeView1Collapsing
        self.TreeView1.OnDeletion = self.TreeView1Deletion

        # TListView。TabSheet2 の上に、レポート表示(列見出し付き)で置く。
        self.ListView1 = TListView(self)
        self.ListView1.Parent = self.TabSheet2
        self.ListView1.Left = 10
        self.ListView1.Top = 40
        self.ListView1.Width = 190
        self.ListView1.Height = 85
        self.ListView1.ViewStyle = vsReport
        self.ListView1.RowSelect = True
        self.ListView1.Checkboxes = True
        nameColumn = self.ListView1.Columns.Add()
        nameColumn.Caption = "Name"
        nameColumn.Width = 90
        sizeColumn = self.ListView1.Columns.Add()
        sizeColumn.Caption = "Size"
        sizeColumn.Width = 60
        sizeColumn.Alignment = taRightJustify
        self.AlphaItem = self.ListView1.Items.Add()
        self.AlphaItem.Caption = "Alpha"
        self.AlphaItem.SubItems.Add("10")
        self.BetaItem = self.ListView1.Items.Add()
        self.BetaItem.Caption = "Beta"
        self.BetaItem.SubItems.Add("20")
        self.GammaItem = self.ListView1.Items.Add()
        self.GammaItem.Caption = "Gamma"
        self.GammaItem.SubItems.Add("30")
        self.ListView1.OnSelectItem = lambda Sender, Item, Selected: pr(
            f"ListView1SelectItem: {Item.Caption} Selected={int(Selected)}")
        self.ListView1.OnItemChecked = lambda Sender, Item: pr(
            f"ListView1ItemChecked: {Item.Caption} Checked={int(Item.Checked)}")
        # 列見出しのクリックで、その列の文字列の順に並べ替える。
        self.ListView1.OnColumnClick = self.ListView1ColumnClick
        self.ListView1.OnDeletion = self.ListView1Deletion

        # TStringGrid・TDrawGrid。PageControl1 の "Grids" ページに上下に並べる。
        self.GridSheet = TTabSheet(self)
        self.GridSheet.PageControl = self.PageControl1
        self.GridSheet.Caption = "Grids"

        self.StringGrid1 = TStringGrid(self)
        self.StringGrid1.Parent = self.GridSheet
        self.StringGrid1.Left = 5
        self.StringGrid1.Top = 5
        self.StringGrid1.Width = 200
        self.StringGrid1.Height = 62
        self.StringGrid1.ColCount = 3
        self.StringGrid1.RowCount = 4
        self.StringGrid1.FixedCols = 0
        self.StringGrid1.DefaultRowHeight = 18
        self.StringGrid1.Options = self.StringGrid1.Options | goEditing
        self.StringGrid1.Cells[0][0] = "Name"
        self.StringGrid1.Cells[1][0] = "Qty"
        self.StringGrid1.Cells[2][0] = "Locked"
        names = ["Cherry", "Apple", "Banana"]
        qtys = ["3", "1", "2"]
        for r in range(1, 4):
            self.StringGrid1.Cells[0, r] = names[r - 1]
            self.StringGrid1.Cells[1, r] = qtys[r - 1]
            self.StringGrid1.Cells[2, r] = "-"
        # 3 列目("Locked")のセルは選択させない。
        self.StringGrid1.OnSelectCell = self.StringGrid1SelectCell
        self.StringGrid1.OnSelection = lambda Sender, ACol, ARow: pr(
            f"StringGrid1Selection: ({ACol},{ARow}) = {self.StringGrid1.Cells[ACol][ARow]}")
        # 列見出しのクリックで、その列の値で行を並べ替える。
        self.StringGrid1.OnHeaderClick = self.StringGrid1HeaderClick

        # DrawGrid1 はセルの内容を OnDrawCell で描く(市松模様と、固定セル以外に列・行の番号)。
        self.DrawGrid1 = TDrawGrid(self)
        self.DrawGrid1.Parent = self.GridSheet
        self.DrawGrid1.Left = 5
        self.DrawGrid1.Top = 70
        self.DrawGrid1.Width = 200
        self.DrawGrid1.Height = 58
        self.DrawGrid1.ColCount = 4
        self.DrawGrid1.RowCount = 3
        self.DrawGrid1.DefaultColWidth = 45
        self.DrawGrid1.DefaultRowHeight = 18
        self.DrawGrid1.OnDrawCell = self.DrawGrid1DrawCell
        self.GridSheet.OnShow = self.GridSheetShow

        # THeaderControl。"Header" ページの上端に置く。
        self.HeaderSheet = TTabSheet(self)
        self.HeaderSheet.PageControl = self.PageControl1
        self.HeaderSheet.Caption = "Header"
        self.HeaderControl1 = THeaderControl(self)
        self.HeaderControl1.Parent = self.HeaderSheet
        self.HeaderControl1.Align = alTop
        self.HeaderControl1.DragReorder = True
        for text, width in (("Name", 80), ("Size", 50), ("Date", 60)):
            section = self.HeaderControl1.Sections.Add()
            section.Text = text
            section.Width = width
            section.MinWidth = 20
        self.HeaderControl1.Sections.Items[1].Alignment = taRightJustify
        self.HeaderControl1.OnSectionClick = lambda HeaderControl, Section: pr(
            f"HeaderControl1SectionClick: {Section.Text} (Index={Section.Index})")
        self.HeaderControl1.OnSectionResize = lambda HeaderControl, Section: pr(
            f"HeaderControl1SectionResize: {Section.Text} Width={Section.Width}")
        self.HeaderControl1.OnSectionTrack = self.HeaderControl1SectionTrack
        # "Name" は他のセクションと入れ替えさせない。
        self.HeaderControl1.OnSectionDrag = self.HeaderControl1SectionDrag
        self.HeaderControl1.OnSectionEndDrag = lambda Sender: pr("HeaderControl1SectionEndDrag")

        # TToolBar・TToolButton。"Tools" ページの上端に置く(TToolBar の既定の Align は alTop)。
        self.ToolsSheet = TTabSheet(self)
        self.ToolsSheet.PageControl = self.PageControl1
        self.ToolsSheet.Caption = "Tools"
        self.ToolBar1 = TToolBar(self)
        self.ToolBar1.Parent = self.ToolsSheet
        self.ToolBar1.ShowCaptions = True
        self.ToolBar1.SetButtonSize(30, 22)

        # ボタンは Parent をツールバーにすると、その順で末尾に追加される。
        def addButton(caption, style):
            button = TToolButton(self)
            button.Caption = caption
            button.Style = style
            button.Parent = self.ToolBar1
            return button

        self.NewToolButton = addButton("New", tbsButton)
        self.SepToolButton = addButton("", tbsDivider)
        self.BoldToolButton = addButton("B", tbsCheck)
        self.LeftToolButton = addButton("L", tbsCheck)
        self.RightToolButton = addButton("R", tbsCheck)
        self.DropToolButton = addButton("Drop", tbsDropDown)
        # 隣り合う Grouped の tbsCheck は、どれか 1 つだけが Down になる。
        self.LeftToolButton.Grouped = True
        self.RightToolButton.Grouped = True
        self.LeftToolButton.Down = True
        self.NewToolButton.OnClick = lambda Sender: pr(f"NewToolButtonClick: {Sender.Caption}")
        self.BoldToolButton.OnClick = self.printDown
        self.LeftToolButton.OnClick = self.printDown
        self.RightToolButton.OnClick = self.printDown
        self.DropToolButton.OnClick = lambda Sender: pr("DropToolButtonClick")
        # DropdownMenu は設定しない(矢印で OnArrowClick だけが呼ばれる)。
        self.DropToolButton.OnArrowClick = lambda Sender: pr("DropToolButtonArrowClick")
        self.ToolsSheet.OnShow = self.ToolsSheetShow

        # TCoolBar。"Cool" ページの上端に置く(TCoolBar の既定の Align は alTop)。
        self.CoolSheet = TTabSheet(self)
        self.CoolSheet.PageControl = self.PageControl1
        self.CoolSheet.Caption = "Cool"
        self.CoolBar1 = TCoolBar(self)
        self.CoolBar1.Parent = self.CoolSheet
        self.CoolEdit = TEdit(self)
        self.CoolEdit.Width = 80
        self.CoolEdit.Parent = self.CoolBar1
        self.CoolCombo = TComboBox(self)
        self.CoolCombo.Width = 80
        self.CoolCombo.Parent = self.CoolBar1
        self.CoolBar1.Bands.Items[0].Text = "Edit"
        self.CoolBar1.Bands.Items[1].Text = "Combo"
        self.CoolBar1.OnChange = self.CoolBar1Change
        self.CoolSheet.OnShow = self.CoolSheetShow

        self.HeaderSheet.OnShow = lambda Sender: pr(
            f"HeaderSheetShow: HeaderControl1 Width/Height={self.HeaderControl1.Width}/{self.HeaderControl1.Height}, "
            f"GetSectionAt(100, 5)={self.HeaderControl1.GetSectionAt(TPoint(100, 5))} (expected 1)")

        # TImage(docs/adr/0029)。
        self.ImageSheet = TTabSheet(self)
        self.ImageSheet.PageControl = self.PageControl1
        self.ImageSheet.Caption = "Image"

        self.Image1 = TImage(self)
        self.Image1.Parent = self.ImageSheet
        self.Image1.Left = 10
        self.Image1.Top = 10
        self.Image1.AutoSize = True
        self.Image1.OnPictureChanged = self.Image1PictureChanged
        # Picture.Bitmap は、Picture の中身をビットマップとして扱うビュー(空なら LCL が空のビットマップを作る)。
        self.Image1.Picture.Bitmap.SetSize(60, 40)
        self.Image1.Picture.Bitmap.Canvas.Brush.Color = clYellow
        self.Image1.Picture.Bitmap.Canvas.FillRect(TRect(0, 0, 60, 40))
        self.Image1.Picture.Bitmap.Canvas.Pen.Color = clBlue
        self.Image1.Picture.Bitmap.Canvas.Ellipse(0, 0, 60, 40)
        pr(f"Image1 Picture {self.Image1.Picture.Width}x{self.Image1.Picture.Height} (expected 60x40), "
           f"OnPictureChanged called: {yn(self.pictureChanges_ > 0)}, HasGraphic={int(self.Image1.HasGraphic)} (expected 1)")

        # Picture が空の TImage の Canvas は、コントロールの大きさのビットマップを作ってから返る(描いた内容は Picture に残る)。
        self.Image2 = TImage(self)
        self.Image2.Parent = self.ImageSheet
        self.Image2.Left = 100
        self.Image2.Top = 10
        self.Image2.Width = 50
        self.Image2.Height = 30
        self.Image2.Stretch = True
        pr(f"Image2 before Canvas: Graphic is null: {yn(self.Image2.Picture.Graphic is None)}, "
           f"HasGraphic={int(self.Image2.HasGraphic)} (expected 0)")
        self.Image2.Canvas.Brush.Color = clRed
        self.Image2.Canvas.FillRect(TRect(0, 0, 50, 30))
        pr(f"Image2 after Canvas: Picture {self.Image2.Picture.Width}x{self.Image2.Picture.Height} (expected 50x30), "
           f"Pixels[5][5]={color(self.Image2.Picture.Bitmap.Canvas.Pixels[5][5])} (expected 0000FF), Stretch={int(self.Image2.Stretch)}")

        # Glyph。VCL と同じく、一時的な TBitmap に描いて代入する(代入は内容のコピー)。
        self.BitBtn2 = TBitBtn(self)
        self.BitBtn2.Parent = self.ImageSheet
        self.BitBtn2.Left = 10
        self.BitBtn2.Top = 60
        self.BitBtn2.Width = 100
        self.BitBtn2.Kind = bkOK
        glyph = TBitmap()
        glyph.SetSize(16, 16)
        glyph.Canvas.Brush.Color = clGreen
        glyph.Canvas.FillRect(TRect(0, 0, 16, 16))
        # LCL では、Glyph を設定すると Kind が bkCustom に戻る(Caption はそのまま)。
        self.BitBtn2.Glyph = glyph
        self.BitBtn2.Layout = blGlyphRight
        self.BitBtn2.Spacing = 8
        # 幅が高さの 2 倍の画像は、状態別の画像が 2 つ並んだものとして NumGlyphs が 2 になる。
        glyph.SetSize(32, 16)
        self.SpeedButton1.Glyph = glyph
        glyph.Free()
        pr(f"BitBtn2 Glyph: {self.BitBtn2.Glyph.Width}x{self.BitBtn2.Glyph.Height} (expected 16x16), "
           f"NumGlyphs={self.BitBtn2.NumGlyphs} (expected 1), Pixels[8][8]={color(self.BitBtn2.Glyph.Canvas.Pixels[8][8])} (expected 008000), "
           f"Layout={int(self.BitBtn2.Layout)} (expected 1), Spacing={self.BitBtn2.Spacing} (expected 8), "
           f"Margin={self.BitBtn2.Margin} (expected -1), Kind={int(self.BitBtn2.Kind)} (expected bkCustom=0), Caption={self.BitBtn2.Caption}")
        pr(f"SpeedButton1 Glyph Width={self.SpeedButton1.Glyph.Width} (expected 32), NumGlyphs={self.SpeedButton1.NumGlyphs} (expected 2)")
        self.BitBtn2.Glyph = None
        pr(f"BitBtn2 Glyph = nullptr: Glyph->Empty={int(self.BitBtn2.Glyph.Empty)} (expected 1)")

        # TImageList と各コントロールの Images・ImageIndex(docs/adr/0030)。
        self.ImageList1 = TImageList(self)
        self.ImageList1.OnChange = self.ImageList1Change
        # 横に 2 つ並んだ画像を、AddSliced で 2 つの画像として加える(LCL の Add は分けずに 16x16 に縮める)。
        strip = TBitmap()
        strip.SetSize(32, 16)
        strip.Canvas.Brush.Color = clRed
        strip.Canvas.FillRect(TRect(0, 0, 16, 16))
        strip.Canvas.Brush.Color = clBlue
        strip.Canvas.FillRect(TRect(16, 0, 32, 16))
        first = self.ImageList1.AddSliced(strip, 2, 1)
        green = TBitmap()
        green.SetSize(16, 16)
        green.Canvas.Brush.Color = clGreen
        green.Canvas.FillRect(TRect(0, 0, 16, 16))
        masked = self.ImageList1.AddMasked(green, clWhite)
        pr(f"ImageList1 AddSliced(32x16, 2, 1): first={first} (expected 0), Count={self.ImageList1.Count} (expected 3), "
           f"AddMasked={masked} (expected 2), Width/Height={self.ImageList1.Width}/{self.ImageList1.Height} (expected 16/16), "
           f"OnChange called by Add: {yn(self.imageListChanges_ > 0)} (expected no)")
        # Add は画像を 16x16 に縮めて 1 つとして加える。OnChange は Delete 等で呼ばれる。
        scaled = self.ImageList1.Add(strip, None)
        check = TBitmap()
        self.ImageList1.GetBitmap(scaled, check)
        pr(f"ImageList1 Add(32x16): index={scaled} (expected 3), Count={self.ImageList1.Count} (expected 4), "
           f"image {check.Width}x{check.Height} (expected 16x16)")
        check.Free()
        self.ImageList1.Delete(scaled)
        pr(f"ImageList1 Delete: Count={self.ImageList1.Count} (expected 3), OnChange called: {yn(self.imageListChanges_ > 0)} (expected yes)")
        # GetBitmap で画像を取り出し、Draw で Canvas に描く。
        out = TBitmap()
        self.ImageList1.GetBitmap(1, out)
        pr(f"ImageList1 GetBitmap(1): {out.Width}x{out.Height} (expected 16x16), Pixels[8][8]={color(out.Canvas.Pixels[8][8])} (expected FF0000)")
        out.SetSize(20, 20)
        out.Canvas.Brush.Color = clWhite
        out.Canvas.FillRect(TRect(0, 0, 20, 20))
        self.ImageList1.Draw(out.Canvas, 2, 2, 2)
        pr(f"ImageList1 Draw(2) at (2,2): Pixels[10][10]={color(out.Canvas.Pixels[10][10])} (expected 008000), "
           f"Pixels[0][0]={color(out.Canvas.Pixels[0][0])} (expected FFFFFF)")
        self.ImageList1.Move(2, 0)
        self.ImageList1.GetBitmap(0, out)
        pr(f"ImageList1 Move(2, 0): image 0 Pixels[8][8]={color(out.Canvas.Pixels[8][8])} (expected 008000)")
        self.ImageList1.Move(0, 2)
        out.Free()
        green.Free()
        strip.Free()

        # 各コントロールの Images と、項目の ImageIndex。
        self.TreeView1.Images = self.ImageList1
        self.RootNode.ImageIndex = 0
        self.RootNode.SelectedIndex = 1
        self.ListView1.SmallImages = self.ImageList1
        self.ListView1.Items.Item[0].ImageIndex = 1
        self.ToolBar1.Images = self.ImageList1
        self.NewToolButton.ImageIndex = 0
        self.BoldToolButton.ImageIndex = 1
        self.PageControl1.Images = self.ImageList1
        self.TabSheet1.ImageIndex = 2
        self.HeaderControl1.Images = self.ImageList1
        self.HeaderControl1.Sections.Items[0].ImageIndex = 0
        self.CoolBar1.Images = self.ImageList1
        self.CoolBar1.Bands.Items[0].ImageIndex = 1
        self.MainMenu1.Images = self.ImageList1
        self.FileNewItem.ImageIndex = 0
        self.BitBtn2.Images = self.ImageList1
        self.BitBtn2.ImageIndex = 2
        il = self.ImageList1
        pr(f"Images set: TreeView={yn(self.TreeView1.Images is il)} ListView.Small={yn(self.ListView1.SmallImages is il)} "
           f"ToolBar={yn(self.ToolBar1.Images is il)} PageControl={yn(self.PageControl1.Images is il)} "
           f"Header={yn(self.HeaderControl1.Images is il)} CoolBar={yn(self.CoolBar1.Images is il)} "
           f"MainMenu={yn(self.MainMenu1.Images is il)} BitBtn2={yn(self.BitBtn2.Images is il)}")
        pr(f"ImageIndex: RootNode={self.RootNode.ImageIndex}/{self.RootNode.SelectedIndex} (expected 0/1), "
           f"ListItem={self.ListView1.Items.Item[0].ImageIndex} (expected 1), NewToolButton={self.NewToolButton.ImageIndex} (expected 0), "
           f"TabSheet1={self.TabSheet1.ImageIndex} (expected 2), Section={self.HeaderControl1.Sections.Items[0].ImageIndex} (expected 0), "
           f"Band={self.CoolBar1.Bands.Items[0].ImageIndex} (expected 1), FileNewItem={self.FileNewItem.ImageIndex} (expected 0), "
           f"BitBtn2={self.BitBtn2.ImageIndex} (expected 2), Child1Node={self.Child1Node.ImageIndex} (expected -1)")

        # 画像リストを破棄すると、LCL がコントロールの Images を外す(Python でも None になる)。
        temp = TImageList(self)
        self.Image2.Images = temp
        self.ToolBar1.HotImages = temp
        pr(f"Before temp->Free(): Image2->Images is temp: {yn(self.Image2.Images is temp)}, "
           f"ToolBar1->HotImages is temp: {yn(self.ToolBar1.HotImages is temp)}")
        temp.Free()
        pr(f"After temp->Free(): Image2->Images is null: {yn(self.Image2.Images is None)}, "
           f"ToolBar1->HotImages is null: {yn(self.ToolBar1.HotImages is None)}")

        # メニュー項目・クールバーの Bitmap(所有者が持つ TBitmap のビュー。代入は内容のコピー)。
        icon = TBitmap()
        icon.SetSize(12, 12)
        self.FileExitItem.Bitmap = icon
        self.CoolBar1.Bitmap = icon
        icon.Free()
        pr(f"FileExitItem->Bitmap {self.FileExitItem.Bitmap.Width}x{self.FileExitItem.Bitmap.Height} (expected 12x12), "
           f"CoolBar1->Bitmap Width={self.CoolBar1.Bitmap.Width} (expected 12)")
        self.CoolBar1.Bitmap = None
        pr(f"CoolBar1->Bitmap = nullptr: Empty={int(self.CoolBar1.Bitmap.Empty)} (expected 1)")

        self.OnCreate = self.FormCreate
        self.OnShow = self.FormShow
        self.OnResize = self.FormResize
        self.OnCloseQuery = self.FormCloseQuery
        self.OnClose = self.FormClose
        self.OnDestroy = self.FormDestroy

    # ---------------- イベントハンドラ ----------------

    def TabSheet2Show(self, Sender):
        selected = self.ListView1.Selected
        pr(f"TabSheet2Show: ListView1->Selected={selected.Caption if selected else '(none)'} (expected Beta)")

    def TreeView1Collapsing(self, Sender, Node, AllowCollapse):
        if Node.Text == "Locked":
            AllowCollapse.value = False

    def TreeView1Deletion(self, Sender, Node):
        self.deletedNodes_ += 1

    def ListView1ColumnClick(self, Sender, Column):
        pr(f"ListView1ColumnClick: {Column.Caption}")
        self.ListView1.SortType = stText
        self.ListView1.SortColumn = Column.Index

    def ListView1Deletion(self, Sender, Item):
        self.deletedListItems_ += 1

    def StringGrid1SelectCell(self, Sender, ACol, ARow, CanSelect):
        if ACol == 2:
            CanSelect.value = False
            pr(f"StringGrid1SelectCell: refused ({ACol},{ARow})")

    def StringGrid1HeaderClick(self, Sender, IsColumn, Index):
        pr(f"StringGrid1HeaderClick: IsColumn={int(IsColumn)} Index={Index}")
        if IsColumn:
            self.StringGrid1.SortColRow(True, Index)

    def DrawGrid1DrawCell(self, Sender, ACol, ARow, ARect, AState):
        self.drawnCells_ += 1
        if AState & gdFixed:
            return  # 見出しは既定の描画のまま
        canvas = self.DrawGrid1.Canvas
        canvas.Brush.Color = clYellow if (ACol + ARow) % 2 else clWhite
        canvas.Pen.Color = clBlack
        canvas.Rectangle(ARect.Left, ARect.Top, ARect.Right, ARect.Bottom)
        canvas.TextOut(ARect.Left + 3, ARect.Top + 2, f"{ACol},{ARow}")

    def GridSheetShow(self, Sender):
        r = self.DrawGrid1.CellRect(1, 1)
        col, row = Ref(-1), Ref(-1)
        self.StringGrid1.MouseToCell(60, 25, col, row)
        pr(f"GridSheetShow: DrawGrid1->CellRect(1,1)=({r.Left},{r.Top},{r.Right},{r.Bottom}), "
           f"StringGrid1->MouseToCell(60,25)=({col.value},{row.value})")

    def HeaderControl1SectionTrack(self, HeaderControl, Section, Width, State):
        if State != tsTrackMove:  # 移動中は何度も呼ばれるので、開始と終了だけ表示する
            pr(f"HeaderControl1SectionTrack: {Section.Text} Width={Width} State={int(State)}")

    def HeaderControl1SectionDrag(self, Sender, FromSection, ToSection, AllowDrag):
        AllowDrag.value = FromSection.Text != "Name" and ToSection.Text != "Name"
        pr(f"HeaderControl1SectionDrag: {FromSection.Text} -> {ToSection.Text} AllowDrag={int(AllowDrag.value)}")

    def printDown(self, Sender):
        pr(f"ToolButtonClick: {Sender.Caption} Down={int(Sender.Down)}, "
           f"B/L/R Down={int(self.BoldToolButton.Down)}/{int(self.LeftToolButton.Down)}/{int(self.RightToolButton.Down)}")

    def ToolsSheetShow(self, Sender):
        s = f"ToolsSheetShow: ToolBar1 Height={self.ToolBar1.Height} RowCount={self.ToolBar1.RowCount}, buttons (Left,Top,Width):"
        for i in range(self.ToolBar1.ButtonCount):
            b = self.ToolBar1.Buttons[i]
            s += f" {b.Left},{b.Top},{b.Width}"
        pr(s)

    def CoolBar1Change(self, Sender):
        bands = self.CoolBar1.Bands
        b0, b1 = bands.Items[0], bands.Items[1]
        pr(f"CoolBar1Change: Bands[0]={b0.Text} Width={b0.Width} Break={int(b0.Break)}, "
           f"Bands[1]={b1.Text} Width={b1.Width} Break={int(b1.Break)}")

    def CoolSheetShow(self, Sender):
        bands = self.CoolBar1.Bands
        s = f"CoolSheetShow: CoolBar1 Height={self.CoolBar1.Height}, bands (Left,Top,Right,Height):"
        for i in range(bands.Count):
            b = bands.Items[i]
            s += f" {b.Left},{b.Top},{b.Right},{b.Height}"
        pr(s)

    def Image1PictureChanged(self, Sender):
        self.pictureChanges_ += 1

    def ImageList1Change(self, Sender):
        self.imageListChanges_ += 1

    def FormCreate(self, Sender):
        pr(f"FormCreate: Sender is Form: {yn(Sender is self)}, Form1 assigned: {yn(Form1 is self)}")

    def FormShow(self, Sender):
        pr("FormShow")
        # ウィンドウを作ると、LCL は ListBox の Items の中身を OS のリストの TStrings に差し替える(内容は引き継がれる)。
        # TStrings のビューは操作のたびに所有者から中身を取り直すので、そのまま使える。
        items = self.ListBox1.Items
        pr(f"ListBox1 Items replaced after the window was created: {yn(items.Handle != self.listBoxItemsBeforeShow_)}, "
           f"Count={items.Count} (expected 3), Strings[2]={items.Strings[2]} (expected List 3)")

        # Align による配置は、LCL ではフォームが表示されるまで行われない(VCL と異なる)。OnShow の時点では済んでいる。
        def printBounds(name, c, expected):
            pr(f"{name} Bounds=({c.Left},{c.Top},{c.Width},{c.Height}) (expected {expected})")

        printBounds("AlignTopPanel", self.AlignTopPanel, "1,1,278,20")
        printBounds("AlignLeftPanel", self.AlignLeftPanel, "1,21,80,68")
        printBounds("Splitter1", self.Splitter1, "81,21,5,68")
        printBounds("AlignClientPanel", self.AlignClientPanel, "86,21,193,68")
        # LabeledEdit1 のラベル(lpLeft)は、エディットの左に LabelSpacing(6)だけ離れて置かれる。
        zipLabel = self.LabeledEdit1.EditLabel
        pr(f"LabeledEdit1 EditLabel right + 6 == Edit Left: {yn(zipLabel.Left + zipLabel.Width + 6 == self.LabeledEdit1.Left)} "
           f"({zipLabel.Left} + {zipLabel.Width} + 6 vs {self.LabeledEdit1.Left})")
        # 表示後は、座標からノードを引ける(1 行目は Root)。
        atTop = self.TreeView1.GetNodeAt(30, 5)
        pr(f"TreeView1->GetNodeAt(30, 5) is RootNode: {yn(atTop is self.RootNode)}")
        # BorderSpacing.Around(4)の分だけ、AlignClientPanel のクライアント領域(枠の 1px の内側)から離れる。
        printBounds("SpacedButton", self.SpacedButton, "5,41,183,22")
        anchoredBefore = self.AnchoredButton.Width
        clientBefore = self.AlignClientPanel.Width
        # プログラムから Splitter を動かすと、alLeft のパネルの幅と alClient のパネルが追随する。
        self.Splitter1.SetSplitterPosition(121)
        pr(f"After SetSplitterPosition(121): SplitterPosition={self.Splitter1.GetSplitterPosition()} "
           f"AlignLeftPanel->Width={self.AlignLeftPanel.Width} AlignClientPanel->Left={self.AlignClientPanel.Left}")
        # 左右の辺に付けた AnchoredButton の幅は、AlignClientPanel の幅と同じだけ変わる。
        pr(f"AnchoredButton Width change={self.AnchoredButton.Width - anchoredBefore}, "
           f"AlignClientPanel Width change={self.AlignClientPanel.Width - clientBefore} (expected equal)")
        # AutoSize の TImage は画像の大きさになる(表示されていないページにあっても)。
        pr(f"Image1 AutoSize Width/Height={self.Image1.Width}/{self.Image1.Height} (expected 60/40)")

    # 閉じる操作の 1 回目は OnCloseQuery で、2 回目は OnClose で取りやめ、
    # 3 回目は既定の動作(MainForm なので caFree = アプリケーションの終了)のままにする。
    def FormCloseQuery(self, Sender, CanClose):
        self.closeAttempts_ += 1
        pr(f"FormCloseQuery: attempt={self.closeAttempts_}, default CanClose={int(CanClose.value)}")
        if self.closeAttempts_ == 1:
            CanClose.value = False
            pr("FormCloseQuery: blocked (CanClose = false)")

    def FormClose(self, Sender, Action):
        pr(f"FormClose: attempt={self.closeAttempts_}, default Action={int(Action.value)}")
        if self.closeAttempts_ == 2:
            Action.value = caNone
            pr("FormClose: blocked (Action = caNone)")

    # 終了処理(atexit)で呼ばれる。この時点では子コントロールもまだ有効。
    def FormDestroy(self, Sender):
        pr(f"FormDestroy: Button1->Caption={self.Button1.Caption}")

    def OpenSubButtonClick(self, Sender):
        # Owner を self にしているが、caFree / Release で先に破棄されても Owner 側から外れるだけで問題ない。
        sub = TSubForm(self)
        sub.Show()

    def Button1Click(self, Sender):
        self.clicks_ += 1
        Sender.Caption = f"Clicked {self.clicks_}"
        pr(f"Button1Click: Sender is Button1: {yn(Sender is self.Button1)}, count={self.clicks_}")

    def Button1DblClick(self, Sender):
        pr("Button1DblClick")

    def Button1MouseDown(self, Sender, Button, Shift, X, Y):
        pr(f"Button1MouseDown: button={int(Button)} shift=0x{int(Shift):x} pos=({X},{Y})")

    def Button1MouseUp(self, Sender, Button, Shift, X, Y):
        pr(f"Button1MouseUp: button={int(Button)} shift=0x{int(Shift):x} pos=({X},{Y})")

    def Button1MouseEnter(self, Sender):
        pr("Button1MouseEnter")

    def Button1MouseLeave(self, Sender):
        pr("Button1MouseLeave")

    def Edit1KeyDown(self, Sender, Key, Shift):
        pr(f"Edit1KeyDown: key={Key.value} shift=0x{int(Shift):x}")

    def Edit1KeyPress(self, Sender, Key):
        pr(f"Edit1KeyPress: key={ord(Key.value)} ('{Key.value if ord(Key.value) >= 32 else '?'}')")

    def FormResize(self, Sender):
        pr(f"FormResize: {self.Width}x{self.Height}")

    def ToggleBox1Click(self, Sender):
        pr(f"ToggleBox1Click: Checked={int(Sender.Checked)}")
        if self.StatusBar1:
            self.StatusBar1.SimpleText = "Toggle: " + ("on" if self.ToggleBox1.Checked else "off")

    def ScrollBar1Change(self, Sender):
        pr(f"ScrollBar1Change: Position={Sender.Position}")

    def TrackBar1Change(self, Sender):
        pr(f"TrackBar1Change: Position={Sender.Position}")

    def RadioGroup1Click(self, Sender):
        pr(f"RadioGroup1Click: ItemIndex={Sender.ItemIndex}")

    def CheckListBox1ClickCheck(self, Sender):
        pr(f"CheckListBox1ClickCheck: Checked[0]={int(Sender.Checked[0])} Checked[1]={int(Sender.Checked[1])} "
           f"Checked[2]={int(Sender.Checked[2])}")

    def PageControl1Change(self, Sender):
        pr(f"PageControl1Change: ActivePageIndex={Sender.ActivePageIndex} Caption={Sender.ActivePage.Caption}")

    def AddDialogItem(self, caption, action):
        item = TMenuItem(self)
        item.Caption = caption
        item.OnClick = lambda Sender: action()
        self.DialogsMenu.Add(item)

    def OpenDialog1CanClose(self, Sender, CanClose):
        pr(f"OpenDialog1 OnCanClose: FileName={self.OpenDialog1.FileName}")
        CanClose.value = True

    def OpenItemClick(self):
        # VCL と同じく、Execute が true を返したら FileName を使う。
        if self.OpenDialog1.Execute():
            self.Memo1.Lines.LoadFromFile(self.OpenDialog1.FileName)
            pr(f"OpenDialog1: FileName={self.OpenDialog1.FileName}, Files->Count={self.OpenDialog1.Files.Count}, "
               f"FilterIndex={self.OpenDialog1.FilterIndex}, Memo1 lines={self.Memo1.Lines.Count}")
        else:
            pr("OpenDialog1: cancelled")

    def SaveItemClick(self):
        if self.SaveDialog1.Execute():
            self.Memo1.Lines.SaveToFile(self.SaveDialog1.FileName)
            pr(f"SaveDialog1: saved to {self.SaveDialog1.FileName}")
        else:
            pr("SaveDialog1: cancelled")

    def SelectDirectoryItemClick(self):
        if self.SelectDirectoryDialog1.Execute():
            pr(f"SelectDirectoryDialog1: {self.SelectDirectoryDialog1.FileName}")
        else:
            pr("SelectDirectoryDialog1: cancelled")

    def ColorItemClick(self):
        self.ColorDialog1.Color = self.Panel1.Color
        if self.ColorDialog1.Execute():
            self.Panel1.Color = self.ColorDialog1.Color
            pr(f"ColorDialog1: Color={color(self.ColorDialog1.Color)}")
        else:
            pr("ColorDialog1: cancelled")

    def FontItemClick(self):
        self.FontDialog1.Font = self.Memo1.Font
        if self.FontDialog1.Execute():
            self.Memo1.Font = self.FontDialog1.Font
            f = self.Memo1.Font
            pr(f"FontDialog1: Name={f.Name} Size={f.Size} Style=0x{int(f.Style):x} Color={color(f.Color)}")
        else:
            pr("FontDialog1: cancelled")

    # FindDialog1・ReplaceDialog1 の「次を検索」。Memo1 の文字列を FindText で探す(frMatchCase で大文字と小文字を区別する)。
    def FindDialog1Find(self, Sender):
        text = self.Memo1.Text
        what = Sender.FindText
        if not (Sender.Options & frMatchCase):
            text, what = text.lower(), what.lower()
        pos = text.find(what) if what else -1
        name = "FindDialog1" if Sender is self.FindDialog1 else "ReplaceDialog1"
        pr(f"{name} OnFind: FindText={Sender.FindText}, Options=0x{int(Sender.Options):x}, found at {pos}")

    # ReplaceDialog1 の「置換」「すべて置換」。どちらが押されたかは Options の frReplace・frReplaceAll で分かる。
    def ReplaceDialog1Replace(self, Sender):
        text = self.Memo1.Text
        what = self.ReplaceDialog1.FindText
        with_ = self.ReplaceDialog1.ReplaceText
        all_ = bool(self.ReplaceDialog1.Options & frReplaceAll)
        count = 0
        if what:
            count = text.count(what) if all_ else min(1, text.count(what))
            text = text.replace(what, with_, -1 if all_ else 1)
        self.Memo1.Text = text
        pr(f"ReplaceDialog1 OnReplace: {'all' if all_ else 'one'}, replaced {count}")

    def FileNewItemClick(self, Sender):
        pr(f"FileNewItemClick: Sender is FileNewItem: {yn(Sender is self.FileNewItem)}")

    def PopupMenu1Popup(self, Sender):
        pr(f"PopupMenu1Popup: PopupComponent is Panel1: {yn(Sender.PopupComponent is self.Panel1)}")

    def Splitter1Moved(self, Sender):
        pr(f"Splitter1Moved: SplitterPosition={Sender.GetSplitterPosition()}, AlignLeftPanel->Width={self.AlignLeftPanel.Width}")

    def TabControl1Change(self, Sender):
        pr(f"TabControl1Change: TabIndex={Sender.TabIndex}")

    def SpeedButton1Click(self, Sender):
        pr(f"SpeedButton1Click: Down={int(Sender.Down)}")

    def CheckBox1Click(self, Sender):
        pr(f"CheckBox1Click: Checked={int(Sender.Checked)}")

    def RadioButtonClick(self, Sender):
        pr(f"RadioButtonClick: {Sender.Caption} Checked={int(Sender.Checked)}")

    def PanelButtonClick(self, Sender):
        pr(f"PanelButtonClick: Parent is Panel1: {yn(Sender.Parent is self.Panel1)}")

    def Panel1Paint(self, Sender):
        c = self.Panel1.Canvas
        c.Pen.Color = clBlack
        c.Brush.Color = clYellow
        c.Polygon([(100, 50), (125, 8), (150, 50)])
        c.Pen.Style = psDot
        c.Brush.Style = bsClear
        c.RoundRect(95, 4, 176, 56, 12, 12)
        c.Pen.Style = psSolid
        c.Brush.Style = bsSolid
        c.Font.Orientation = 900
        c.Font.Color = clBlue
        c.TextOut(158, 50, "Paint")
        c.Font.Orientation = 0

    # Edit1 / Memo1 / ComboBox1 で共有する。isinstance で Sender の種類を判定する。
    def TextChange(self, Sender):
        kind = ("Memo" if isinstance(Sender, TMemo) else "Edit" if isinstance(Sender, TCustomEdit)
                else "ComboBox" if isinstance(Sender, TComboBox) else "?")
        pr(f"TextChange: {kind} Text={Sender.Text}")

    def ListBox1Click(self, Sender):
        pr(f"ListBox1Click: ItemIndex={Sender.ItemIndex}")

    def Timer1Timer(self, Sender):
        self.ticks_ += 1
        self.TickLabel.Caption = f"Tick: {self.ticks_}"
        pr(f"Timer tick! count={self.ticks_}")

    def PaintBox1Paint(self, Sender):
        canvas = Sender.Canvas
        canvas.Pen.Color = clRed
        canvas.Pen.Width = 2
        canvas.Brush.Color = clYellow
        canvas.Rectangle(10, 10, 110, 70)

        canvas.Pen.Color = clBlue
        canvas.Brush.Color = clWhite
        canvas.Ellipse(120, 10, 200, 70)

        canvas.Pen.Color = clBlack
        canvas.MoveTo(10, 90)
        canvas.LineTo(200, 90)

        canvas.Font.Color = clGreen
        canvas.Font.Size = 14
        canvas.TextOut(10, 100, "Canvas drawing test")


def main():
    global Form1
    Application.Initialize()
    Application.Title = "Bethany test"
    Form1 = Application.CreateForm(TMainForm)
    f = Form1

    pr(f"Title: {Application.Title}")
    pr(f"Application->MainForm is Form1: {yn(Application.MainForm is Form1)}")
    pr(f"MainForm caption: {Application.MainForm.Caption}")
    pr(f"Button1->Parent->Caption: {f.Button1.Parent.Caption}")
    pr(f"ScrolledButton->Parent is ScrollBox1: {yn(f.ScrolledButton.Parent is f.ScrollBox1)}")
    pr(f"Bevel1 Shape/Style: {int(f.Bevel1.Shape)}/{int(f.Bevel1.Style)} (expected bsFrame=1/bsRaised=1)")
    pr(f"Shape1 Shape/Brush.Color/Pen.Color: {int(f.Shape1.Shape)}/{color(f.Shape1.Brush.Color).lower()}/{color(f.Shape1.Pen.Color).lower()}")
    pr(f"StaticText1 BorderStyle: {int(f.StaticText1.BorderStyle)} (expected sbsSunken=2)")
    pr(f"ScrollBar1 Position: {f.ScrollBar1.Position} (expected 30)")
    pr(f"TrackBar1 Position: {f.TrackBar1.Position} (expected 5)")
    pr(f"ProgressBar1 Position: {f.ProgressBar1.Position} (expected 42)")
    pr(f"UpDown1 Position: {f.UpDown1.Position}, Associate is UpDownEdit: {yn(f.UpDown1.Associate is f.UpDownEdit)}")
    pr(f"RadioGroup1 Items->Count/ItemIndex: {f.RadioGroup1.Items.Count}/{f.RadioGroup1.ItemIndex} (expected 3/1)")
    pr(f"CheckGroup1 Checked[0]/[1]/[2]: {int(f.CheckGroup1.Checked[0])}/{int(f.CheckGroup1.Checked[1])}/"
       f"{int(f.CheckGroup1.Checked[2])} (expected 1/0/1)")
    pr(f"CheckListBox1 Items->Count/Checked[1]: {f.CheckListBox1.Items.Count}/{int(f.CheckListBox1.Checked[1])} (expected 3/1)")
    pr(f"BitBtn1 Kind: {int(f.BitBtn1.Kind)} (expected bkOK=1), Caption: {f.BitBtn1.Caption}")
    pr(f"FloatSpinEdit1 Value: {f.FloatSpinEdit1.Value:.1f} (expected 2.5)")
    # SpinEdit1.Value は int 版(TCustomSpinEdit)が基底の float 版を隠していることの確認。
    pr(f"SpinEdit1 Value: {f.SpinEdit1.Value} (expected 42, int hides the inherited double)")
    pr(f"MaskEdit1 EditMask: {f.MaskEdit1.EditMask}")
    # EditLabel は初めて取得したときにラッパーが作られ、以降は同じラッパーが返る。
    le = f.LabeledEdit1
    editLabel = le.EditLabel
    pr(f"LabeledEdit1 EditLabel Caption={editLabel.Caption} (expected Zip:), same wrapper: {yn(editLabel is le.EditLabel)}, "
       f"Parent is Form1: {yn(editLabel.Parent is Form1)}, LabelPosition={int(le.LabelPosition)} (expected lpLeft={int(lpLeft)}), "
       f"LabelSpacing={le.LabelSpacing} (expected 6)")
    # 生成直後の既定値(lpAbove・3)と、エディットと一緒にラベルも破棄されること(ラッパーも破棄済みになる)。
    temp = TLabeledEdit(Form1)
    tempLabel = temp.EditLabel
    pr(f"temp LabeledEdit LabelPosition={int(temp.LabelPosition)} (expected lpAbove={int(lpAbove)}), "
       f"LabelSpacing={temp.LabelSpacing} (expected 3), EditLabel Parent is null: {yn(temp.EditLabel.Parent is None)}")
    temp.Free()
    pr(f"temp LabeledEdit freed with its EditLabel: {yn(is_freed(temp) and is_freed(tempLabel))} (Python only)")
    pr(f"TabControl1 Tabs->Count/TabIndex: {f.TabControl1.Tabs.Count}/{f.TabControl1.TabIndex} (expected 3/0)")
    pr(f"StatusBar1 SimpleText: {f.StatusBar1.SimpleText} (expected Ready)")
    # TStatusBar の Align の既定値は alBottom(Left/Top を指定しなくてもフォームの下端に付く)。
    pr(f"StatusBar1 Align: {int(f.StatusBar1.Align)} (expected alBottom={int(alBottom)})")
    pr(f"AlignTopPanel/AlignLeftPanel/AlignClientPanel Align: {int(f.AlignTopPanel.Align)}/{int(f.AlignLeftPanel.Align)}/"
       f"{int(f.AlignClientPanel.Align)} (expected alTop={int(alTop)}/alLeft={int(alLeft)}/alClient={int(alClient)})")
    sp = f.Splitter1
    pr(f"Splitter1 Align={int(sp.Align)} (expected alLeft={int(alLeft)}) MinSize={sp.MinSize} Beveled={int(sp.Beveled)} "
       f"AutoSnap={int(sp.AutoSnap)} ResizeAnchor={int(sp.ResizeAnchor)} (expected akLeft={int(akLeft)}) "
       f"ResizeStyle={int(sp.ResizeStyle)} (expected rsUpdate={int(rsUpdate)})")

    # メニュー。
    root = f.MainMenu1.Items
    pr(f"Form1->Menu is MainMenu1: {yn(f.Menu is f.MainMenu1)}, Panel1->PopupMenu is PopupMenu1: {yn(f.Panel1.PopupMenu is f.PopupMenu1)}")
    # ルート項目のラッパーは初回アクセス時に作られ、以降は同じものが返る。
    pr(f"MainMenu1->Items is the same wrapper each time: {yn(root is f.MainMenu1.Items)}, Count={root.Count} (expected 3: File, View, Dialogs)")
    pr(f"FileMenu->Parent is MainMenu1->Items: {yn(f.FileMenu.Parent is root)}, Items->Items[1] is ViewMenu: {yn(root.Items[1] is f.ViewMenu)}")
    # AddSeparator の区切り線も LCL が内部で生成した項目で、Items[i] で初めてラッパーができる。
    sep = f.FileMenu.Items[1]
    pr(f"FileMenu Count={f.FileMenu.Count} (expected 3), Items[1] IsLine={int(sep.IsLine())} Caption={sep.Caption} "
       f"Parent is FileMenu: {yn(sep.Parent is f.FileMenu)}")
    pr(f"FileNewItem ShortCut={ShortCutToText(f.FileNewItem.ShortCut)} (0x{f.FileNewItem.ShortCut:04x}, "
       f"TextToShortCut(\"Ctrl+N\") matches: {yn(TextToShortCut('Ctrl+N') == f.FileNewItem.ShortCut)})")

    # Click() は利用者が選んだときと同じく、AutoCheck の反映と OnClick を行う。
    f.FileNewItem.Click()
    f.ViewLargeItem.Click()
    pr(f"After clicking Large: Small/Large Checked={int(f.ViewSmallItem.Checked)}/{int(f.ViewLargeItem.Checked)} (expected 0/1)")
    f.ViewSmallItem.Click()
    pr(f"After clicking Small: Small/Large Checked={int(f.ViewSmallItem.Checked)}/{int(f.ViewLargeItem.Checked)} (expected 1/0)")

    # Insert/Delete。Delete は外すだけで破棄しない。
    temp = TMenuItem(f)
    temp.Caption = "Temp"
    f.FileMenu.Insert(0, temp)
    pr(f"After Insert(0): IndexOf(temp)={f.FileMenu.IndexOf(temp)} (expected 0), Count={f.FileMenu.Count} (expected 4)")
    f.FileMenu.Delete(0)
    pr(f"After Delete(0): Count={f.FileMenu.Count} (expected 3), temp->Parent is null: {yn(temp.Parent is None)}")
    temp.Free()

    # TPageControl + TTabSheet。
    pc = f.PageControl1
    pr(f"PageControl1 PageCount={pc.PageCount} (expected 3), ActivePage is TabSheet1: {yn(pc.ActivePage is f.TabSheet1)}, "
       f"ActivePageIndex={pc.ActivePageIndex} (expected 0)")
    pr(f"Pages[1] is TabSheet2 (AddTabSheet): {yn(pc.Pages[1] is f.TabSheet2)}, TabSheet2->PageControl is PageControl1: "
       f"{yn(f.TabSheet2.PageControl is pc)}")
    pr(f"TabSheet3 TabVisible={int(f.TabSheet3.TabVisible)} TabIndex={f.TabSheet3.TabIndex} (expected 0/-1), "
       f"PageIndex={f.TabSheet3.PageIndex} (expected 2)")
    # PageIndex を書き換えるとページの並びが変わる。
    f.TabSheet2.PageIndex = 0
    pr(f"After TabSheet2->PageIndex = 0: Pages[0] is TabSheet2: {yn(pc.Pages[0] is f.TabSheet2)}, "
       f"TabSheet1 PageIndex={f.TabSheet1.PageIndex} (expected 1)")
    f.TabSheet2.PageIndex = 1
    pr(f"TabPosition={int(pc.TabPosition)} (expected tpTop={int(tpTop)}), ShowTabs={int(pc.ShowTabs)}, MultiLine={int(pc.MultiLine)}")

    # Clear はすべてのページを外して遅延破棄する。ここでは直後の temp.Free() で Owner と一緒に破棄される。
    temp = TPageControl(f)
    temp.AddTabSheet().Caption = "A"
    temp.AddTabSheet().Caption = "B"
    before = temp.PageCount
    temp.Clear()
    pr(f"temp PageCount before/after Clear: {before}/{temp.PageCount} (expected 2/0)")
    temp.Free()

    # TTreeView。
    tv = f.TreeView1
    items = tv.Items
    pr(f"TreeView1 Items->Count={items.Count} (expected 5), RootNode->Count={f.RootNode.Count} (expected 2), "
       f"GrandchildNode->Level={f.GrandchildNode.Level} (expected 2)")
    # 同じノードには常に同じラッパーが返る。
    pr(f"Child1Node->Parent is RootNode: {yn(f.Child1Node.Parent is f.RootNode)}, Items->Item[2] is GrandchildNode: "
       f"{yn(items.Item[2] is f.GrandchildNode)}, FindNodeWithText(\"Child 2\") is Child2Node: "
       f"{yn(items.FindNodeWithText('Child 2') is f.Child2Node)}, RootNode->GetNextSibling() is Root2Node: "
       f"{yn(f.RootNode.GetNextSibling() is f.Root2Node)}")
    pr(f"RootNode->Parent is null: {yn(f.RootNode.Parent is None)}, TreeView is TreeView1: {yn(f.RootNode.TreeView is tv)}")
    firstChild = f.RootNode.Items[0]
    pr(f"RootNode->Items[1] is Child2Node: {yn(f.RootNode.Items[1] is f.Child2Node)}, "
       f"RootNode->Items[0]->Items[0] is GrandchildNode: {yn(firstChild.Items[0] is f.GrandchildNode)}")

    userData = ctypes.c_int(42)
    f.Child2Node.Data = ctypes.addressof(userData)
    pr(f"Child2Node->Data: {ctypes.c_int.from_address(f.Child2Node.Data).value} (expected 42)")

    tv.Selected = f.Child2Node
    pr(f"Selected is Child2Node: {yn(tv.Selected is f.Child2Node)}, Child2Node->Selected={int(f.Child2Node.Selected)}")

    # MoveTo: Child2 を Root 2 の子に移す。
    f.Child2Node.MoveTo(f.Root2Node, naAddChild)
    pr(f"After MoveTo: Child2Node->Parent is Root2Node: {yn(f.Child2Node.Parent is f.Root2Node)}, "
       f"RootNode->Count={f.RootNode.Count} (expected 1)")
    f.Root2Node.Expand(False)
    pr(f"Root2Node->Expanded={int(f.Root2Node.Expanded)} (expected 1)")

    # OnCollapsing で取りやめると、折りたたまれない。
    locked = items.Add(None, "Locked")
    inside = items.AddChild(locked, "Inside")
    locked.Expanded = True
    locked.Collapse(False)
    pr(f"Locked->Expanded after Collapse={int(locked.Expanded)} (expected 1: OnCollapsing refused)")

    # Delete: 子孫も含めて削除され、ノードごとに OnDeletion が呼ばれ、ラッパーも破棄済みになる。
    before = f.deletedNodes_
    locked.Delete()
    pr(f"After Delete: deleted={f.deletedNodes_ - before} (expected 2), Items->Count={items.Count} (expected 5)")
    pr(f"Deleted node wrappers are freed: {yn(is_freed(locked) and is_freed(inside))} (Python only)")

    # TListView。
    lv = f.ListView1
    items = lv.Items
    columns = lv.Columns
    pr(f"ListView1 Columns->Count={columns.Count} (expected 2), Items[1]->Caption={columns.Items[1].Caption} "
       f"Alignment={int(columns.Items[1].Alignment)} (expected taRightJustify={int(taRightJustify)}), "
       f"same wrapper: {yn(columns.Items[1] is columns.Items[1])}")
    pr(f"Items->Count={items.Count} (expected 3), Item[1] is BetaItem: {yn(items.Item[1] is f.BetaItem)}, "
       f"SubItems[0]={f.BetaItem.SubItems.Strings[0]} (expected 20), ListView is ListView1: {yn(f.BetaItem.ListView is lv)}")
    pr(f"FindCaption(\"Gam\", partial) is GammaItem: {yn(items.FindCaption(0, 'Gam', True, True, False) is f.GammaItem)}")

    lv.Selected = f.BetaItem
    pr(f"Selected is BetaItem: {yn(lv.Selected is f.BetaItem)}, ItemIndex={lv.ItemIndex} (expected 1), SelCount={lv.SelCount} (expected 1)")
    f.GammaItem.Checked = True
    pr(f"GammaItem Checked={int(f.GammaItem.Checked)} (expected 1)")
    f.GammaItem.SubItems.Strings[0] = "33"
    pr(f"GammaItem SubItems[0]={f.GammaItem.SubItems.Strings[0]} (expected 33)")

    # Exchange で入れ替え、SortType = stText で Caption の順に並べ直す。
    items.Exchange(0, 2)
    pr(f"After Exchange(0, 2): Item[0] is GammaItem: {yn(items.Item[0] is f.GammaItem)}")
    lv.SortColumn = 0  # 既定の -1 のままでは並べ替えない
    lv.SortType = stText
    pr(f"After SortType = stText: Item[0] is AlphaItem: {yn(items.Item[0] is f.AlphaItem)}, Item[2] is GammaItem: {yn(items.Item[2] is f.GammaItem)}")
    lv.SortType = stNone

    # Delete: OnDeletion の後に項目のラッパーも破棄済みになる。列の Delete も同様。
    before = f.deletedListItems_
    temp = items.Add()
    temp.Caption = "Temp"
    temp.Delete()
    pr(f"After Delete: deleted={f.deletedListItems_ - before} (expected 1), Items->Count={items.Count} (expected 3)")
    columns.Add().Caption = "Temp"
    columns.Delete(2)
    pr(f"After column Delete: Columns->Count={columns.Count} (expected 2)")

    # TStringGrid・TDrawGrid。
    sg = f.StringGrid1
    pr(f"StringGrid1 ColCount/RowCount={sg.ColCount}/{sg.RowCount} (expected 3/4), FixedCols/FixedRows={sg.FixedCols}/{sg.FixedRows} "
       f"(expected 0/1), Cells[0][1]={sg.Cells[0][1]}, goEditing in Options: {yn(sg.Options & goEditing)}")

    # ColWidths / RowHeights: 添字で読み書きする。
    sg.ColWidths[0] = 80
    sg.ColWidths[1] = sg.ColWidths[0]
    sg.RowHeights[2] = 25
    pr(f"ColWidths[0]/[1]={sg.ColWidths[0]}/{sg.ColWidths[1]} (expected 80/80), RowHeights[2]={sg.RowHeights[2]} (expected 25)")
    sg.ColWidths[1] = 64
    sg.RowHeights[2] = 18

    # Cells: 要素同士の代入、プロパティとの間の代入。
    sg.Cells[2][1] = sg.Cells[1][1]
    savedCaption = Form1.Caption
    Form1.Caption = sg.Cells[0][1]
    sg.Cells[2][2] = Form1.Caption
    Form1.Caption = savedCaption
    cell = sg.Cells[2][1]
    pr(f"Cells[2][1]={cell} (expected 3), Cells[2][2]={sg.Cells[2][2]} (expected Cherry), same as Cells[1][1]: {yn(cell == sg.Cells[1][1])}")
    sg.Cells[2][1] = "-"
    sg.Cells[2][2] = "-"

    # 列 0 の値で行を並べ替える(固定行は除く)。
    sg.SortColRow(True, 0)
    pr(f"After SortColRow(true, 0): {sg.Cells[0][1]}/{sg.Cells[0][2]}/{sg.Cells[0][3]} (expected Apple/Banana/Cherry), "
       f"Qty of Apple={sg.Cells[1][1]} (expected 1)")

    # 行の挿入・削除・移動。
    sg.InsertColRow(False, 1)
    pr(f"After InsertColRow(false, 1): RowCount={sg.RowCount} (expected 5), Cells[0][1]='{sg.Cells[0][1]}' (expected ''), "
       f"Cells[0][2]={sg.Cells[0][2]} (expected Apple)")
    sg.DeleteColRow(False, 1)
    sg.MoveColRow(False, 3, 1)
    pr(f"After DeleteColRow and MoveColRow(false, 3, 1): RowCount={sg.RowCount} (expected 4), Cells[0][1]={sg.Cells[0][1]} (expected Cherry)")
    sg.MoveColRow(False, 1, 3)

    # 選択範囲。
    sg.Col = 1
    sg.Row = 2
    sel = sg.Selection
    pr(f"Col/Row={sg.Col}/{sg.Row}, Selection=({sel.Left},{sel.Top},{sel.Right},{sel.Bottom}) (expected 1,2,1,2)")

    dg = f.DrawGrid1
    pr(f"DrawGrid1 ColCount/RowCount={dg.ColCount}/{dg.RowCount} (expected 4/3), DefaultColWidth={dg.DefaultColWidth} (expected 45)")

    # Clean は文字列だけを消し、Clear は行・列を削除する。
    temp = TStringGrid(Form1)
    temp.Cells[1][1] = "x"
    temp.Clean()
    pr(f"temp after Clean: Cells[1][1]='{temp.Cells[1][1]}' (expected ''), ColCount={temp.ColCount} (expected 5)")
    temp.Clear()
    pr(f"temp after Clear: ColCount/RowCount={temp.ColCount}/{temp.RowCount} (expected 0/0)")
    temp.Free()

    # THeaderControl。
    hc = f.HeaderControl1
    sections = hc.Sections
    size = sections.Items[1]
    pr(f"HeaderControl1 Sections->Count={sections.Count} (expected 3), Items[1]->Text={size.Text} Width={size.Width} (expected 50), "
       f"Left/Right={size.Left}/{size.Right} (expected 80/130), Alignment={int(size.Alignment)} (expected taRightJustify={int(taRightJustify)}), "
       f"same wrapper: {yn(sections.Items[1] is size)}")

    # Index で移動しても OriginalIndex は変わらない。
    date = sections.Items[2]
    date.Index = 0
    pr(f"After Date->Index = 0: Items[0] is Date: {yn(sections.Items[0] is date)}, Date OriginalIndex={date.OriginalIndex} (expected 2), "
       f"SectionFromOriginalIndex[2] is Date: {yn(hc.SectionFromOriginalIndex[2] is date)}, Name Left={sections.Items[1].Left} (expected 60)")
    date.Index = 2

    # Visible が false のセクションは幅 0 として扱われる。
    size.Visible = False
    pr(f"Size->Visible = false: Width={size.Width} (expected 0), Date Left={date.Left} (expected 80)")
    size.Visible = True

    # Insert・Delete。Delete したセクションのラッパーは破棄済みになる。
    inserted = sections.Insert(1)
    inserted.Text = "Temp"
    pr(f"After Insert(1): Count={sections.Count} (expected 4), Items[1]->Text={sections.Items[1].Text}, "
       f"Items[2] is Size: {yn(sections.Items[2] is size)}")
    sections.Delete(1)
    pr(f"After Delete(1): Count={sections.Count} (expected 3), Items[1] is Size: {yn(sections.Items[1] is size)}, "
       f"DragReorder={int(hc.DragReorder)} (expected 1)")

    # TToolBar・TToolButton。
    tb = f.ToolBar1
    pr(f"ToolBar1 ButtonCount={tb.ButtonCount} (expected 6), Buttons[2] is BoldToolButton: {yn(tb.Buttons[2] is f.BoldToolButton)}, "
       f"DropToolButton->Index={f.DropToolButton.Index} (expected 5), Align={int(tb.Align)} (expected alTop={int(alTop)}), "
       f"EdgeBorders=0x{int(tb.EdgeBorders):x} (expected ebTop=0x{int(ebTop):x}), ButtonWidth/Height={tb.ButtonWidth}/{tb.ButtonHeight} (expected 30/22)")

    # Click は OnClick を呼ぶだけで、tbsCheck の Down は変えない。
    f.BoldToolButton.Click()
    pr(f"After BoldToolButton->Click(): B Down={int(f.BoldToolButton.Down)} (expected 0)")
    # Grouped の tbsCheck は、Down を設定すると他方が上がる。
    f.RightToolButton.Down = True
    pr(f"After RightToolButton->Down = true: L/R Down={int(f.LeftToolButton.Down)}/{int(f.RightToolButton.Down)} (expected 0/1), "
       f"Style of SepToolButton={int(f.SepToolButton.Style)} (expected tbsDivider={int(tbsDivider)})")
    f.LeftToolButton.Down = True

    # MenuItem を設定すると、その項目の Caption 等を写す。
    temp = TToolButton(f)
    temp.Parent = tb
    temp.MenuItem = f.FileNewItem
    pr(f"temp MenuItem is FileNewItem: {yn(temp.MenuItem is f.FileNewItem)}, Caption={temp.Caption} (expected &New), "
       f"ButtonCount={tb.ButtonCount} (expected 7)")
    temp.Free()
    pr(f"After temp->Free(): ButtonCount={tb.ButtonCount} (expected 6)")

    # TCoolBar。
    cb = f.CoolBar1
    bands = cb.Bands
    editBand = bands.Items[0]
    pr(f"CoolBar1 Bands->Count={bands.Count} (expected 2), Items[0]->Control is CoolEdit: {yn(editBand.Control is f.CoolEdit)}, "
       f"FindBand(CoolCombo) is Items[1]: {yn(bands.FindBand(f.CoolCombo) is bands.Items[1])}, "
       f"FindBandIndex(CoolCombo)={bands.FindBandIndex(f.CoolCombo)} (expected 1), Items[0]->Text={editBand.Text}, "
       f"same wrapper: {yn(bands.Items[0] is editBand)}, Align={int(cb.Align)} (expected alTop={int(alTop)})")

    # コントロールを置かないバンドを Add で追加し、Index で移動する。
    empty = bands.Add()
    empty.Text = "Empty"
    empty.Index = 0
    pr(f"After Add and Index = 0: Count={bands.Count} (expected 3), Items[0] is the new band: {yn(bands.Items[0] is empty)}, "
       f"Items[1] is editBand: {yn(bands.Items[1] is editBand)}, Control is null: {yn(empty.Control is None)}")
    bands.Delete(0)
    pr(f"After Delete(0): Count={bands.Count} (expected 2), Items[0] is editBand: {yn(bands.Items[0] is editBand)}")

    # コントロールを破棄すると、そのバンドも LCL が削除する(バンドのラッパーも破棄済みになる)。
    tempEdit = TEdit(f)
    tempEdit.Parent = cb
    tempBand = bands.FindBand(tempEdit)
    pr(f"tempEdit band: Count={bands.Count} (expected 3), FindBand is Items[2]: {yn(tempBand is bands.Items[2])}")
    tempEdit.Free()
    pr(f"After tempEdit->Free(): Count={bands.Count} (expected 2)")

    # Align を alLeft にすると Vertical も true になる。
    temp = TCoolBar(f)
    temp.Align = alLeft
    pr(f"temp CoolBar Align = alLeft: Vertical={int(temp.Vertical)} (expected 1), GrabStyle={int(temp.GrabStyle)} "
       f"(expected gsDouble={int(gsDouble)}), ShowText={int(temp.ShowText)} (expected 1)")
    temp.Free()

    # TStrings(Items・Lines・Tabs・SubItems)。
    box = TListBox(f)
    items = box.Items
    items.Add("Banana")
    appleIndex = items.Add("Apple")
    items.Insert(0, "Cherry")
    pr(f"TStrings Add returned {appleIndex} (expected 1), Count={items.Count} (expected 3), Strings[0]={items.Strings[0]} (expected Cherry), "
       f"IndexOf(\"Apple\")={items.IndexOf('Apple')} (expected 2), IndexOf(\"none\")={items.IndexOf('none')} (expected -1)")

    items.Exchange(0, 2)
    items.Move(0, 1)
    items.Strings[2] = "Cherry!"
    pr(f"After Exchange(0, 2), Move(0, 1), Strings[2] = \"Cherry!\": CommaText={items.CommaText} (expected Banana,Apple,Cherry!)")

    # Objects は利用者データ(ポインタ)。
    tag = ctypes.c_int(42)
    tagAddress = ctypes.addressof(tag)
    items.Objects[1] = tagAddress
    items.AddObject("Date", tagAddress)
    pr(f"Objects[1] is &tag: {yn(items.Objects[1] == tagAddress)}, Objects[3] is &tag: {yn(items.Objects[3] == tagAddress)}, "
       f"Objects[0] is null: {yn(items.Objects[0] is None)}")

    # Text は改行でつないだ文字列。CommaText は空白・カンマを含む要素を二重引用符で囲む。
    items.Text = "one\ntwo\nthree"
    pr(f"After Text = one/two/three: Count={items.Count} (expected 3), Strings[1]={items.Strings[1]} (expected two)")
    items.CommaText = 'a,"b c",d'
    pr(f"After CommaText = a,\"b c\",d: Count={items.Count} (expected 3), Strings[1]={items.Strings[1]} (expected b c), "
       f"CommaText={items.CommaText}")

    # Assign・AddStrings は別のコントロールの TStrings から写す。
    items.Assign(f.Memo1.Lines)
    pr(f"After Assign(Memo1->Lines): Count={items.Count} (expected 2), Strings[0]={items.Strings[0]} (expected Memo line 1)")
    items.AddStrings(f.ComboBox1.Items)
    items.Delete(0)
    pr(f"After AddStrings(ComboBox1->Items) and Delete(0): Count={items.Count} (expected 4), Strings[1]={items.Strings[1]} (expected Combo A)")
    items.BeginUpdate()
    items.Clear()
    items.EndUpdate()
    pr(f"After Clear: Count={items.Count} (expected 0)")
    box.Free()

    # 他の所有者の TStrings も同じ形で使える。
    pr(f"Memo1 Lines->Count={f.Memo1.Lines.Count} (expected 2), TabControl1 Tabs->Strings[1]={f.TabControl1.Tabs.Strings[1]} "
       f"(expected Tab B), BetaItem SubItems->Strings[0]={f.BetaItem.SubItems.Strings[0]} (expected 20), "
       f"RadioGroup1 Items->Strings[2]={f.RadioGroup1.Items.Strings[2]} (expected Option C)")
    pr(f"Python iteration: list(Memo1.Lines)={list(f.Memo1.Lines)}, len={len(f.Memo1.Lines)} (Python only)")

    # TStringList(利用者が生成する文字列の一覧。docs/adr/0028)。
    lst = TStringList()
    lst.Add("cherry")
    lst.Add("Banana")
    lst.Add("apple")
    lst.Sort()
    pr(f"TStringList Sort: CommaText={lst.CommaText} (expected apple,Banana,cherry: case-insensitive by default)")
    insensitive = lst.IndexOf("APPLE")
    lst.CaseSensitive = True
    pr(f"IndexOf(\"APPLE\"): {insensitive} (expected 0) / CaseSensitive: {lst.IndexOf('APPLE')} (expected -1)")
    lst.CaseSensitive = False

    # Sorted にすると、Add はソート順の位置に入り、既定の dupIgnore では重複を加えない。
    lst.Sorted = True
    blueberry = lst.Add("blueberry")
    countBefore = lst.Count
    lst.Add("APPLE")
    index = Ref(-1)
    found = lst.Find("CHERRY", index)
    missing = Ref(-1)
    foundMissing = lst.Find("avocado", missing)
    pr(f"Sorted Add(\"blueberry\")={blueberry} (expected 2), Add(\"APPLE\") ignored: {yn(lst.Count == countBefore)}, "
       f"Duplicates={int(lst.Duplicates)} (expected dupIgnore={int(dupIgnore)}), Find(\"CHERRY\")={int(found)}/{index.value} (expected 1/3), "
       f"Find(\"avocado\")={int(foundMissing)}/{missing.value} (expected 0/1)")

    # TStrings を受け取るものにそのまま渡せる。
    box = TListBox(Form1)
    box.Items.Assign(lst)
    pr(f"ListBox Items->Assign(list): Count={box.Items.Count} (expected 4), Strings[3]={box.Items.Strings[3]} (expected cherry)")
    box.Free()
    lst.Free()

    # 名前=値 の行(Values・Names・ValueFromIndex)。
    config = TStringList()
    config.Values["host"] = "localhost"
    config.Values["port"] = "8080"
    pr(f"Values: Count={config.Count} (expected 2), Names[1]={config.Names[1]} (expected port), "
       f"ValueFromIndex[1]={config.ValueFromIndex[1]} (expected 8080), Values[\"host\"]={config.Values['host']} (expected localhost), "
       f"Values[\"none\"] is empty: {yn(config.Values['none'] == '')}, IndexOfName(\"port\")={config.IndexOfName('port')} (expected 1)")
    # LCL(FPC)では、Values[Name] に空文字列を代入しても行は削除されず、値が空になる(VCL は削除する)。
    config.Values["host"] = ""
    pr(f"After Values[\"host\"] = \"\": Count={config.Count} (expected 2), Strings[0]={config.Strings[0]} (expected host=)")
    config.ValueFromIndex[0] = ""
    pr(f"After ValueFromIndex[0] = \"\": Count={config.Count} (expected 1), Strings[0]={config.Strings[0]} (expected port=8080)")

    # 任意の区切り文字(StrictDelimiter なら空白は区切りにならない)。
    fields = TStringList()
    fields.Delimiter = ";"
    fields.StrictDelimiter = True
    fields.DelimitedText = "a b;c;;d"
    pr(f"DelimitedText a b;c;;d: Count={fields.Count} (expected 4), Strings[0]={fields.Strings[0]} (expected a b), "
       f"Strings[2] is empty: {yn(fields.Strings[2] == '')}, Delimiter={fields.Delimiter}")

    # ファイルへの保存と読み込み。
    path = "beth_stringlist_test_py.txt"
    fields.SaveToFile(path)
    loaded = TStringList()
    loaded.LoadFromFile(path)
    os.remove(path)
    pr(f"SaveToFile/LoadFromFile: Count={loaded.Count} (expected 4), Strings[3]={loaded.Strings[3]} (expected d)")

    # 例外(docs/adr/0031)。LCL が送出した例外は BethError として送出され、C++ の Exception と同じ名前で読める。
    lst = TStringList()
    lst.Add("only")
    try:
        s = lst.Strings[5]
        pr(f"must not be reached: {s}")
    except BethError as E:
        pr(f"Strings[5] on 1 item threw: ClassName={E.ClassName()} (expected EStringListError), Message={E.Message}")
    pr(f"After the exception: Count={lst.Count} (expected 1)")

    try:
        with TPicture() as picture:
            picture.LoadFromFile("beth_no_such_file.png")
    except BethError as E:
        pr(f"LoadFromFile(no such file) threw: ClassName={E.ClassName()} (expected EFOpenError)")

    # ハンドラから送出した例外は、ハンドラを呼んだ DLL の関数(ここでは Click)から送出し直される。
    failing = TMenuItem(Application)

    def boom(Sender):
        raise Exception("boom")

    failing.OnClick = boom
    try:
        failing.Click()
        pr("must not be reached")
    except BethError as E:
        pr(f"Click with a throwing handler: ClassName={E.ClassName()} (expected Exception), Message={E.Message} (expected boom)")

    # クラス名を指定した例外・標準の例外・ハンドラの中で LCL が送出した例外。
    def custom(Sender):
        raise BethError("EMyError", "custom")

    failing.OnClick = custom
    try:
        failing.Click()
    except BethError as E:
        pr(f"Custom class: {E.ClassName()}/{E.Message} (expected EMyError/custom)")

    def std_error(Sender):
        raise RuntimeError("std error")

    failing.OnClick = std_error
    try:
        failing.Click()
    except BethError as E:
        pr(f"std::runtime_error: {E.ClassName()}/{E.Message} (expected std::exception/std error; Python: RuntimeError), "
           f"__cause__ is RuntimeError: {yn(isinstance(E.__cause__, RuntimeError))}")

    def inner_error(Sender):
        inner = TStringList()
        inner.Delete(3)

    failing.OnClick = inner_error
    try:
        failing.Click()
    except BethError as E:
        pr(f"LCL exception inside the handler: {E.ClassName()} (expected EStringListError)")

    # 例外を送出しないハンドラに戻すと、Click は成功する。
    clicks = [0]
    failing.OnClick = lambda Sender: clicks.__setitem__(0, clicks[0] + 1)
    failing.Click()
    pr(f"Click after the failures: clicks={clicks[0]} (expected 1)")
    failing.Free()

    # グラフィックス(docs/adr/0029)。
    bmp = TBitmap()
    pr(f"New TBitmap: Empty={int(bmp.Empty)} (expected 1)")
    bmp.SetSize(32, 16)
    bmp.Canvas.Brush.Color = clRed
    bmp.Canvas.FillRect(TRect(0, 0, 32, 16))
    bmp.Canvas.Pixels[1][2] = clBlue
    canvas1 = bmp.Canvas
    canvas2 = bmp.Canvas
    pr(f"TBitmap {bmp.Width}x{bmp.Height} (expected 32x16), Empty={int(bmp.Empty)} (expected 0), "
       f"Pixels[0][0]={color(bmp.Canvas.Pixels[0][0])} (expected 0000FF), Pixels[1][2]={color(bmp.Canvas.Pixels[1][2])} (expected FF0000), "
       f"same Canvas wrapper: {yn(canvas1.Handle == canvas2.Handle)}")

    # 別の形式への変換(Assign)と保存。TPicture.LoadFromFile は拡張子から形式を選ぶ。
    pngPath = "beth_graphic_test_py.png"
    jpgPath = "beth_graphic_test_py.jpg"
    png = TPortableNetworkGraphic()
    png.Assign(bmp)
    png.SaveToFile(pngPath)
    jpg = TJPEGImage()
    jpg.Assign(bmp)
    jpg.CompressionQuality = 90
    jpg.SaveToFile(jpgPath)
    pr(f"PNG {png.Width}x{png.Height} (expected 32x16), JPEG CompressionQuality={jpg.CompressionQuality} (expected 90)")

    pic = TPicture()
    pr(f"New TPicture: Graphic is null: {yn(pic.Graphic is None)}")
    pic.LoadFromFile(pngPath)
    pr(f"TPicture LoadFromFile(png): {pic.Width}x{pic.Height} (expected 32x16), Graphic is null: {yn(pic.Graphic is None)}, "
       f"PNG->Width={pic.PNG.Width} (expected 32)")
    # Bitmap を操作すると、LCL が中身(PNG)をビットマップに変換する。画素は引き継がれる。
    pr(f"After converting to Bitmap: Pixels[1][2]={color(pic.Bitmap.Canvas.Pixels[1][2])} (expected FF0000)")
    pic.LoadFromFile(jpgPath)
    pr(f"TPicture LoadFromFile(jpg): {pic.Width}x{pic.Height} (expected 32x16)")
    # Graphic への代入は内容のコピー(bmp はそのまま利用者の持ち物)。
    pic.Graphic = bmp
    bmp.SetSize(8, 8)
    pr(f"After Graphic = bmp and resizing bmp: Picture {pic.Width}x{pic.Height} (expected 32x16)")
    pic.Clear()
    pr(f"After Clear: Graphic is null: {yn(pic.Graphic is None)}")
    pic.Free()
    bmp.Free()
    png.Free()
    jpg.Free()
    os.remove(pngPath)
    os.remove(jpgPath)

    # デザイナーで設定する共通のプロパティ(docs/adr/0034)。Python では Anchors は要素の frozenset。
    temp = TButton(Form1)
    temp.Parent = Form1.Panel1
    pr(f"temp button defaults: Anchors={{akLeft, akTop}}: {yn(temp.Anchors == {akLeft, akTop})}, TabStop={int(temp.TabStop)} (expected 1), "
       f"ShowHint={int(temp.ShowHint)} (expected 0), ParentShowHint={int(temp.ParentShowHint)} (expected 1), "
       f"ParentFont={int(temp.ParentFont)} (expected 1), Cursor={temp.Cursor} (expected crDefault=0), Tag={temp.Tag} (expected 0)")
    temp.TabOrder = 0
    pr(f"temp->TabOrder=0: temp={temp.TabOrder} (expected 0), PanelButton={Form1.PanelButton.TabOrder} (expected 1)")
    temp.Hint = "ヒント"
    temp.ShowHint = True
    temp.Font.Size = 14
    temp.Cursor = crHandPoint
    temp.Tag = 0x123456789A
    pr(f"After setting: Hint={temp.Hint}, ParentShowHint={int(temp.ParentShowHint)} (expected 0: ShowHint was set), "
       f"ParentFont={int(temp.ParentFont)} (expected 0: Font was set), Cursor={temp.Cursor} (expected crHandPoint=-21), "
       f"Tag=0x{temp.Tag:x} (expected 0x123456789a: pointer-sized)")
    temp.Constraints.MaxWidth = 60
    temp.Width = 200
    other = TButton(Form1)
    other.Constraints = temp.Constraints
    other.BorderSpacing.Around = 7
    temp.BorderSpacing = other.BorderSpacing
    pr(f"Constraints MaxWidth=60 then Width=200: Width={temp.Width} (expected 60); other Constraints MaxWidth={other.Constraints.MaxWidth} "
       f"(expected 60), temp BorderSpacing Around={temp.BorderSpacing.Around} (expected 7)")
    tempPanel = TPanel(Form1)
    before = int(tempPanel.ParentColor)
    tempPanel.Color = clYellow
    pr(f"tempPanel ParentColor before/after setting Color: {before}/{int(tempPanel.ParentColor)} (expected 1/0)")
    tempPanel.Free()
    other.Free()
    temp.Free()
    pr(f"AnchoredButton Anchors contains akRight: {yn(akRight in Form1.AnchoredButton.Anchors)} (expected yes)")

    open_ = Form1.OpenDialog1
    pr(f"OpenDialog1 Options has ofEnableSizing|ofViewDetail|ofFileMustExist: "
       f"{yn(open_.Options == (ofEnableSizing | ofViewDetail | ofFileMustExist))}, FilterIndex={open_.FilterIndex} (expected 1), "
       f"Title={open_.Title}")
    pr(f"SaveDialog1 DefaultExt={Form1.SaveDialog1.DefaultExt} (expected .txt: LCL adds the dot), "
       f"Options has ofOverwritePrompt: {yn(Form1.SaveDialog1.Options & ofOverwritePrompt)}, "
       f"Files->Count={Form1.SaveDialog1.Files.Count} (expected 0)")
    color_ = Form1.ColorDialog1
    pr(f"ColorDialog1 Options={int(color_.Options)} (expected cdFullOpen={int(cdFullOpen)}), "
       f"CustomColors->Count={color_.CustomColors.Count} (expected 20), "
       f"Values[\"ColorB\"]={color_.CustomColors.Values['ColorB']} (expected 000080)")
    color_.Color = clBlue
    pr(f"ColorDialog1 Color={color(color_.Color)} (expected FF0000)")

    # TControl::Color・Font(ダイアログの結果を適用する先)。
    pr(f"Panel1 Color is clDefault: {yn(Form1.Panel1.Color == clDefault)}")
    font = Form1.FontDialog1
    font.Font.Name = "Arial"
    font.Font.Size = 13
    font.Font.Style = fsBold | fsItalic
    Form1.Label1.Font = font.Font
    lf = Form1.Label1.Font
    pr(f"FontDialog1 Options={int(font.Options)} (expected fdEffects={int(fdEffects)}); Label1 Font after assignment: "
       f"{lf.Name} {lf.Size} Style=0x{int(lf.Style):x} (expected Arial 13 0x3)")
    # 代入(Assign)は内容のコピーなので、後から元を変えても写した先は変わらない。
    font.Font.Size = 20
    lf.Style = fsUnderline
    pr(f"After changing the source: Label1 Font Size={lf.Size} (expected 13), Style=0x{int(lf.Style):x} (expected 0x4), "
       f"FontDialog1 Font Style=0x{int(font.Font.Style):x} (expected 0x3)")
    font.Font.Assign(lf)
    pr(f"FontDialog1 Font->Assign(Label1->Font): Size={font.Font.Size} (expected 13)")

    # TFindDialog・TReplaceDialog はモードレス。Execute はすぐ true を返し、CloseDialog で閉じる。
    replace = Form1.ReplaceDialog1
    replace.FindText = "apple"
    replace.ReplaceText = "orange"
    pr(f"FindDialog1 Options=0x{int(Form1.FindDialog1.Options):x} (expected frDown=0x1), "
       f"ReplaceDialog1 Options has frReplace|frReplaceAll: "
       f"{yn((replace.Options & (frReplace | frReplaceAll)) == (frReplace | frReplaceAll))}, "
       f"FindText={replace.FindText}, ReplaceText={replace.ReplaceText}")
    pr(f"FindDialog1 Execute (modeless) returned: {int(Form1.FindDialog1.Execute())} (expected 1)")
    Form1.FindDialog1.CloseDialog()

    # 2 つ目以降に生成したフォームは MainForm にならない。
    subForm = TForm(Application)
    pr(f"MainForm after creating another form is still Form1: {yn(Application.MainForm is Form1)}")
    subForm.Free()

    # ハンドラの解除(None の代入)。解除したボタンを押しても何も起きない。
    disabledHandler = TButton(Form1)
    disabledHandler.Parent = Form1
    disabledHandler.Caption = "No handler"
    disabledHandler.Left = 20
    disabledHandler.Top = 280
    disabledHandler.OnClick = lambda Sender: pr("must not be called")
    disabledHandler.OnClick = None

    # Application.Terminate() でメッセージループを抜ける(ウィンドウを閉じても抜ける)。
    quitButton = TButton(Form1)
    quitButton.Parent = Form1
    quitButton.Caption = "Quit"
    quitButton.Left = 20
    quitButton.Top = 320
    quitButton.OnClick = lambda Sender: Application.Terminate()

    # メッセージのダイアログと、ボタンの ModalResult で閉じるモーダルのフォーム(docs/adr/0041)。
    def dialogs_click(Sender):
        answer = MessageDlg("Save the changes?", mtConfirmation, mbYesNoCancel)
        pr(f"MessageDlg = {answer} (mrYes={mrYes}, mrNo={mrNo}, mrCancel={mrCancel})")
        name = Ref("Bethany")
        ok = InputQuery("InputQuery", "Name:", name)
        pr(f"InputQuery = {'true' if ok else 'false'}, Value = {name.value}")

        # Enter で OK(Default)、Esc で Cancel(Cancel)。押したボタンの ModalResult が ShowModal() の戻り値になる。
        dialog = TForm(Form1)
        dialog.Caption = "ModalResult"
        dialog.BorderStyle = bsDialog
        dialog.Position = poMainFormCenter
        dialog.Width = 240
        dialog.Height = 90
        ok_button = TButton(dialog)
        ok_button.Parent = dialog
        ok_button.Caption = "OK"
        ok_button.Left, ok_button.Top, ok_button.Width, ok_button.Height = 40, 30, 75, 25
        ok_button.ModalResult = mrOk
        ok_button.Default = True
        cancel_button = TButton(dialog)
        cancel_button.Parent = dialog
        cancel_button.Caption = "Cancel"
        cancel_button.Left, cancel_button.Top, cancel_button.Width, cancel_button.Height = 125, 30, 75, 25
        cancel_button.ModalResult = mrCancel
        cancel_button.Cancel = True
        pr(f"ShowModal = {dialog.ShowModal()} (mrOk={mrOk}, mrCancel={mrCancel})")
        dialog.Release()

    dialogsButton = TButton(Form1)
    dialogsButton.Parent = Form1
    dialogsButton.Caption = "Dialogs..."
    dialogsButton.Left = 110
    dialogsButton.Top = 320
    dialogsButton.OnClick = dialogs_click

    # Free() で個別に破棄すると、ラッパーも破棄済みになる(触ると ReferenceError)。
    tempLabel = TTracedLabel(Form1)
    tempLabel.Parent = Form1
    tempLabel.Free()
    pr(f"Destroyed labels after Free(): {destroyed_labels()} (expected 1)")

    # テキストの編集(docs/adr/0042)。表示の前でも、選択の置き換えは DLL がハンドルを作ってから行う。
    sel_edit = TEdit(Form1)
    sel_edit.Parent = Form1
    sel_edit.Left = 200
    sel_edit.Top = 320
    sel_edit.Text = "Hello World"
    sel_edit.SelStart = 6
    sel_edit.SelLength = 5
    sel_text = sel_edit.SelText
    sel_edit.SelText = "Bethany"
    pr(f"SelText={sel_text} (expected World), Text after SelText={sel_edit.Text} (expected Hello Bethany)")
    sel_edit.TextHint = "hint"
    sel_edit.CharCase = ecUpperCase
    pr(f"TextHint={sel_edit.TextHint}, CharCase={int(sel_edit.CharCase)} (expected ecUpperCase={int(ecUpperCase)}), "
       f"CanFocus before Show={'true' if sel_edit.CanFocus() else 'false'}")
    sel_edit.OnEnter = lambda Sender: pr("selEdit OnEnter")
    sel_edit.OnExit = lambda Sender: pr("selEdit OnExit")

    # リストの複数選択・チェックの 3 状態・Application(docs/adr/0043)。
    multi_list = TListBox(Form1)
    multi_list.Parent = Form1
    multi_list.Left, multi_list.Top, multi_list.Height = 340, 320, 60
    for s in ("one", "two", "three"):
        multi_list.Items.Add(s)
    multi_list.MultiSelect = True
    multi_list.Selected[0] = True
    multi_list.Selected[2] = True
    multi_list.OnSelectionChange = lambda Sender, User: pr(
        f"multiList OnSelectionChange User={'true' if User else 'false'} SelCount={multi_list.SelCount}")
    pr(f"multiList SelCount={multi_list.SelCount} (expected 2), "
       f"Selected[1]={'true' if multi_list.Selected[1] else 'false'} (expected false)")
    gray_check = TCheckBox(Form1)
    gray_check.Parent = Form1
    gray_check.Caption = "3 states"
    gray_check.Left, gray_check.Top = 340, 390
    gray_check.AllowGrayed = True
    gray_check.State = cbGrayed
    gray_check.OnChange = lambda Sender: pr(f"grayCheck State={int(gray_check.State)}")
    # 下のパネルのステータスバーの AutoHint・OnHint の確認用(docs/adr/0044)。
    # LCL は ShowHint が true のコントロール(か親)にだけ Application.Hint を設定する(VCL は ShowHint によらない)
    multi_list.Hint = "multi-select list"
    multi_list.ShowHint = True
    gray_check.Hint = "three-state check box"
    gray_check.ShowHint = True
    pr(f"grayCheck State={int(gray_check.State)} (expected cbGrayed={int(cbGrayed)}), "
       f"ExeName is python: {'yes' if Application.ExeName.lower().endswith('python.exe') else 'no'}")

    # Action(docs/adr/0046)。1 つの Action を、ボタンと View メニューの項目の両方に割り当てる
    action_list = TActionList(Form1)
    demo = TAction(Form1)
    demo.Caption = "&Action demo"
    demo.ShortCut = TextToShortCut("Ctrl+K")
    demo.ActionList = action_list

    def demo_execute(Sender):
        demo.Checked = not demo.Checked
        pr(f"demo OnExecute: Sender is the action: {'yes' if Sender is demo else 'no'}, "
           f"Checked={'true' if demo.Checked else 'false'}")

    demo.OnExecute = demo_execute
    demo_button = TButton(Form1)
    demo_button.Parent = Form1
    demo_button.SetBounds(440, 386, 110, 25)
    demo_button.Action = demo
    demo_item = TMenuItem(Form1)
    Form1.ViewMenu.Add(demo_item)
    demo_item.Action = demo
    pr(f"demoButton Caption={demo_button.Caption}, demoItem ShortCut={ShortCutToText(demo_item.ShortCut)} "
       f"(expected &Action demo, Ctrl+K)")

    # Screen・Clipboard・アイコン・ファイルのドロップ(docs/adr/0047)
    wa = Screen.WorkAreaRect
    pr(f"Screen {Screen.Width}x{Screen.Height}, work area ({wa.Left},{wa.Top})-({wa.Right},{wa.Bottom}), "
       f"{Screen.PixelsPerInch} dpi, {Screen.Fonts.Count} fonts, {Screen.FormCount} forms")
    # 青い丸のアイコンを描いて、アプリケーションのアイコンにする(タイトルバーとタスクバーに出る)
    bmp = TBitmap()
    bmp.SetSize(32, 32)
    bmp.Canvas.Brush.Color = clWhite
    bmp.Canvas.FillRect(TRect(0, 0, 32, 32))
    bmp.Canvas.Brush.Color = clBlue
    bmp.Canvas.Ellipse(2, 2, 30, 30)
    icon = TIcon()
    icon.Assign(bmp)
    Application.Icon = icon

    # エクスプローラーからファイルをドロップすると、名前を表示し、最初の名前をクリップボードに置く
    def form_drop_files(Sender, FileNames):
        for name in FileNames:
            pr(f"dropped: {name}")
        Clipboard().AsText = FileNames[0]
        pr(f"clipboard: {Clipboard().AsText} (HasFormat(CF_Text())={Clipboard().HasFormat(CF_Text())})")

    Form1.AllowDropFiles = True
    Form1.OnDropFiles = form_drop_files

    # パネルの縁・スクロール・コントロールの枠(docs/adr/0048)。LayoutPanel の中の alClient のパネルを、二重の縁(外側がくぼみ、
    # 内側が盛り上がり。BevelWidth 2)にし、Caption を左上に寄せる。alTop のパネルの Caption は右に寄せる。
    Form1.AlignClientPanel.BevelOuter = bvLowered
    Form1.AlignClientPanel.BevelInner = bvRaised
    Form1.AlignClientPanel.BevelWidth = 2
    Form1.AlignClientPanel.Alignment = taLeftJustify
    Form1.AlignClientPanel.VerticalAlignment = taAlignTop
    Form1.AlignTopPanel.Alignment = taRightJustify

    # 範囲のコントロール・グループの列(docs/adr/0049)。CheckGroup1 を 2 列にし、項目のクリックとスクロールバーの操作を表示する。
    # TrackBar1 の目盛りは両側に付ける。
    Form1.CheckGroup1.Columns = 2
    Form1.CheckGroup1.OnItemClick = lambda Sender, Index: pr(
        f"CheckGroup1 OnItemClick: Index={Index} Checked={'true' if Sender.Checked[Index] else 'false'}")
    Form1.ScrollBar1.LargeChange = 10
    Form1.ScrollBar1.OnScroll = lambda Sender, ScrollCode, ScrollPos: pr(
        f"ScrollBar1 OnScroll: ScrollCode={int(ScrollCode)} ScrollPos={ScrollPos.value}")
    Form1.TrackBar1.TickMarks = tmBoth

    # オーナードロー(docs/adr/0050)。ListBox1 の各項目の左に色の見本を描き、Panel1 の右クリックのメニューを黄色の地で描く。
    lb = Form1.ListBox1
    lb.Style = lbOwnerDrawFixed
    lb.ItemHeight = 22

    def list_draw_item(Control, Index, ARect, State):
        c = lb.Canvas
        selected = bool(State & odSelected)
        c.Brush.Color = clHighlight if selected else clWindow
        c.FillRect(ARect)
        c.Brush.Color = (clRed, clGreen, clBlue)[Index % 3]
        c.Rectangle(ARect.Left + 3, ARect.Top + 3, ARect.Left + 19, ARect.Bottom - 3)
        c.Brush.Style = bsClear
        c.Font.Color = clHighlightText if selected else clWindowText
        c.TextOut(ARect.Left + 24, ARect.Top + 3, lb.Items.Strings[Index])
        c.Brush.Style = bsSolid

    lb.OnDrawItem = list_draw_item
    Form1.PopupMenu1.OwnerDraw = True

    def popup_measure(Sender, ACanvas, AWidth, AHeight):
        AWidth.value = ACanvas.TextWidth("Say hello (owner draw)") + 24
        AHeight.value = 26

    def popup_draw(Sender, ACanvas, ARect, AState):
        ACanvas.Brush.Color = clHighlight if AState & odSelected else clYellow
        ACanvas.FillRect(ARect)
        ACanvas.Brush.Style = bsClear
        ACanvas.TextOut(ARect.Left + 12, ARect.Top + 5, "Say hello (owner draw)")
        ACanvas.Brush.Style = bsSolid

    Form1.PopupHelloItem.OnMeasureItem = popup_measure
    Form1.PopupHelloItem.OnDrawItem = popup_draw

    # TTreeView の細部(docs/adr/0051)。TreeView1 を Ctrl・Shift で複数選択できるようにする。ラベルの編集(F2 か、選択済みのノードの
    # クリック)は Root 2 だけ始めさせず、空にした編集は元の文字列に戻す。最上位のノードは青い文字で描く。
    tree = Form1.TreeView1
    tree.MultiSelect = True
    tree.MultiSelectStyle = msControlSelect | msShiftSelect
    # テーマで描く(既定)と、文字はテーマの色になり、OnCustomDrawItem で設定した Canvas の Font の色が使われない
    tree.Options = tree.Options & ~tvoThemedDraw

    def tree_editing(Sender, Node, AllowEdit):
        pr(f"TreeView1 OnEditing: {Node.Text}")
        AllowEdit.value = Node.Level > 0 or Node.Text != "Root 2"

    def tree_edited(Sender, Node, S):
        pr(f"TreeView1 OnEdited: {Node.Text} -> {S.value}")
        if not S.value:
            S.value = Node.Text

    def tree_custom_draw(Sender, Node, State, DefaultDraw):
        Sender.Canvas.Font.Color = clBlue if Node.Level == 0 and not State & cdsSelected else clWindowText

    tree.OnEditing = tree_editing
    tree.OnEdited = tree_edited
    tree.OnCustomDrawItem = tree_custom_draw

    # TListView の細部(docs/adr/0052)。ListView1("Page 2")は、ラベルの編集(項目のダブルクリック)を Gamma だけ始めさせず、
    # 空にした編集は元の文字列に戻す。Beta は青い文字、Size の列は緑の文字で描く。列見出しのクリックでの並べ替えは AutoSort と
    # OnCompare で、Size の列は数の順にする。"Virtual" ページには、10000 行の仮想モード(OwnerData)の一覧を OnDrawItem で縞模様に描く。
    lv = Form1.ListView1

    def lv_editing(Sender, Item, AllowEdit):
        pr(f"ListView1 OnEditing: {Item.Caption}")
        AllowEdit.value = Item.Caption != "Gamma"

    def lv_edited(Sender, Item, AValue):
        pr(f"ListView1 OnEdited: {Item.Caption} -> {AValue.value}")
        if not AValue.value:
            AValue.value = Item.Caption

    def lv_custom_draw(Sender, Item, State, DefaultDraw):
        Sender.Canvas.Font.Color = clBlue if Item.Caption == "Beta" else clWindowText

    def lv_custom_draw_sub(Sender, Item, SubItem, State, DefaultDraw):
        Sender.Canvas.Font.Color = clGreen if SubItem == 1 else clWindowText

    def lv_compare(Sender, Item1, Item2, Data, Compare):
        if lv.SortColumn == 1:
            c = int(Item1.SubItems.Strings[0]) - int(Item2.SubItems.Strings[0])
        else:
            c = (Item1.Caption > Item2.Caption) - (Item1.Caption < Item2.Caption)
        # OnCompare があると SortDirection は使われないので、自分で逆にする
        Compare.value = -c if lv.SortDirection == sdDescending else c

    lv.OnEditing = lv_editing
    lv.OnEdited = lv_edited
    lv.OnCustomDrawItem = lv_custom_draw
    lv.OnCustomDrawSubItem = lv_custom_draw_sub
    lv.OnCompare = lv_compare
    # 並べ替えは AutoSort(既定で True)に任せる: 列見出しのクリックでその列の昇順、同じ列をもう一度クリックすると降順
    lv.SortType = stText
    lv.OnColumnClick = lambda Sender, Column: pr(f"ListView1ColumnClick: {Column.Caption}")
    pr(f"ListView1 ShowColumnHeaders={int(lv.ShowColumnHeaders)} ColumnClick={int(lv.ColumnClick)} "
       f"AutoSort={int(lv.AutoSort)} ToolTips={int(lv.ToolTips)} (expected 1 1 1 1)")

    virtualSheet = TTabSheet(Form1)
    virtualSheet.PageControl = Form1.PageControl1
    virtualSheet.Caption = "Virtual"
    vlist = TListView(Form1)
    vlist.Parent = virtualSheet
    vlist.Align = alClient
    vlist.ViewStyle = vsReport
    vlist.RowSelect = True
    rowColumn = vlist.Columns.Add()
    rowColumn.Caption = "Row"
    rowColumn.Width = 100
    squareColumn = vlist.Columns.Add()
    squareColumn.Caption = "Square"
    squareColumn.Width = 90

    def vlist_data(Sender, Item):
        i = Item.Index
        Item.Caption = f"Row {i}"
        Item.SubItems.Add(str(i * i))

    def vlist_draw(Sender, Item, ARect, State):
        c = Sender.Canvas
        selected = bool(State & odSelected)
        c.Brush.Color = clHighlight if selected else (clWindow if Item.Index % 2 else clYellow)
        c.FillRect(ARect)
        c.Brush.Style = bsClear
        c.Font.Color = clHighlightText if selected else clWindowText
        c.TextOut(ARect.Left + 4, ARect.Top + 1, Item.Caption)
        c.TextOut(ARect.Left + 104, ARect.Top + 1, Item.SubItems.Strings[0])
        c.Brush.Style = bsSolid

    vlist.OnData = vlist_data
    vlist.OwnerData = True
    vlist.Items.Count = 10000
    vlist.OwnerDraw = True
    vlist.OnDrawItem = vlist_draw
    pr(f"Virtual list OwnerData={int(vlist.OwnerData)} Items->Count={vlist.Items.Count} (expected 1 10000), "
       f"Item[1234]={vlist.Items.Item[1234].Caption}")

    # グリッドの細部(docs/adr/0053)。StringGrid1("Grids" ページ)は、1 行おきに色を付け、見出しを太字にし、Qty が 3 以上のセルを
    # 赤い文字で描く。列見出しのクリックで並べ替え(ColumnClickSorts。Qty の列は OnCompareCells で数の順)、同じ列をもう一度クリックすると逆順。
    # 編集(F2 かダブルクリック)を終えるとき、Qty に数でない文字列を入れると例外で断り、Name を空にすると元に戻す。
    sg = Form1.StringGrid1
    sg.AlternateColor = clInfoBk
    sg.GridLineColor = clGray
    sg.FocusColor = clBlue
    sg.TitleFont.Style = fsBold
    sg.AutoEdit = False  # 文字を打っただけでは編集を始めない
    sg.ColumnClickSorts = True
    sg.OnHeaderClick = lambda Sender, IsColumn, Index: pr(f"StringGrid1HeaderClick: IsColumn={int(IsColumn)} Index={Index}")

    def sg_compare(Sender, ACol, ARow, BCol, BRow, Result):
        a, b = sg.Cells[ACol][ARow], sg.Cells[BCol][BRow]
        r = int(a) - int(b) if ACol == 1 else (a > b) - (a < b)
        # OnCompareCells があると SortOrder は使われないので、自分で逆にする
        Result.value = -r if sg.SortOrder == soDescending else r

    def sg_prepare(Sender, ACol, ARow, AState):
        if ACol == 1 and ARow > 0 and sg.Cells[ACol][ARow].isdigit() and int(sg.Cells[ACol][ARow]) >= 3:
            sg.Canvas.Font.Color = clRed

    def sg_validate(Sender, ACol, ARow, OldValue, NewValue):
        pr(f"StringGrid1 OnValidateEntry: ({ACol},{ARow}) {OldValue} -> {NewValue.value}")
        if ACol == 1 and not NewValue.value.isdigit():
            raise BethError("Exception", "Qty must be a number")
        if ACol == 0 and not NewValue.value:
            NewValue.value = OldValue  # 空にはさせない

    sg.OnCompareCells = sg_compare
    sg.OnPrepareCanvas = sg_prepare
    sg.OnGetEditText = lambda Sender, ACol, ARow, Value: pr(f"StringGrid1 OnGetEditText: ({ACol},{ARow}) {Value.value}")
    sg.OnValidateEntry = sg_validate
    pr(f"StringGrid1 Rows[1]->CommaText={sg.Rows[1].CommaText} Cols[0]->Count={sg.Cols[0].Count} (expected 4)")

    # グリッドの Columns(docs/adr/0054)。PageControl1 の "Columns" ページに、列を持つグリッドを置く。Color の列は一覧から選び、
    # Size の列は「…」のボタンで S → M → L と切り替え、Done の列はチェックボックス(クリックかスペース)。
    columnSheet = TTabSheet(Form1)
    columnSheet.PageControl = Form1.PageControl1
    columnSheet.Caption = "Columns"
    cg = TStringGrid(Form1)
    cg.Parent = columnSheet
    cg.Align = alClient
    cg.RowCount = 4
    cg.DefaultRowHeight = 20
    cg.Options = cg.Options | goEditing  # 編集とチェックボックスの切り替えに要る
    cg.AutoFillColumns = True
    nameColumn = cg.Columns.Add()
    nameColumn.Title.Caption = "Name"
    colorColumn = cg.Columns.Add()
    colorColumn.Title.Caption = "Color"
    colorColumn.ButtonStyle = cbsPickList
    for c in ("Red", "Green", "Blue"):
        colorColumn.PickList.Add(c)
    sizeColumn = cg.Columns.Add()
    sizeColumn.Title.Caption = "Size"
    sizeColumn.ButtonStyle = cbsEllipsis
    doneColumn = cg.Columns.Add()
    doneColumn.Title.Caption = "Done"
    doneColumn.Title.Font.Style = fsBold
    doneColumn.ButtonStyle = cbsCheckboxColumn
    doneColumn.ValueChecked = "Y"
    doneColumn.ValueUnchecked = "N"
    doneColumn.Width = 40
    for r, name in enumerate(("Apple", "Banana", "Cherry"), start=1):
        cg.Cells[1][r] = name
        cg.Cells[2][r] = "Red"
        cg.Cells[3][r] = "M"
        cg.Cells[4][r] = "N"

    def cg_button(Sender, ACol, ARow):
        v = cg.Cells[ACol][ARow]
        cg.Cells[ACol][ARow] = {"S": "M", "M": "L"}.get(v, "S")
        pr(f"Columns grid OnButtonClick: ({ACol},{ARow}) {v} -> {cg.Cells[ACol][ARow]}")

    cg.OnButtonClick = cg_button
    cg.OnPickListSelect = lambda Sender: pr(f"Columns grid OnPickListSelect: row {cg.Row}")
    cg.OnCheckboxToggled = lambda Sender, ACol, ARow, AState: pr(f"Columns grid OnCheckboxToggled: ({ACol},{ARow}) state={int(AState)}")
    pr(f"Columns grid Columns->Count={cg.Columns.Count} ColCount={cg.ColCount} (expected 4 5), "
       f"Items[1] PickList Count={cg.Columns.Items[1].PickList.Count} (expected 3)")
    pr(f"AlignClientPanel BevelOuter/BevelInner={int(Form1.AlignClientPanel.BevelOuter)}/{int(Form1.AlignClientPanel.BevelInner)} "
       f"(expected 1/2), TreeView ScrollBars={int(Form1.TreeView1.ScrollBars)} (expected 3 = ssBoth)")

    # ステータスバーのパネル(docs/adr/0044)。StatusBar1 の上に、パネルを持つ 2 つ目のステータスバーを置く。
    panel_bar = TStatusBar(Form1)
    panel_bar.Parent = Form1
    panel_bar.SimplePanel = False
    panel_bar.AutoHint = True
    hint_panel = panel_bar.Panels.Add()
    hint_panel.Width = 220
    hint_panel.Text = "Hover the multi-select list"
    draw_panel = panel_bar.Panels.Add()
    draw_panel.Width = 90
    draw_panel.Style = psOwnerDraw
    center_panel = panel_bar.Panels.Insert(1)
    center_panel.Width = 120
    center_panel.Text = "center"
    center_panel.Alignment = taCenter
    center_panel.Bevel = pbRaised
    center_panel.Index = 2  # 末尾へ移す(draw_panel が 1 番目になる)
    panel_bar.Panels.Add().Text = "Click a panel"

    def draw_owner_panel(StatusBar, Panel, Rect):
        StatusBar.Canvas.Brush.Color = clBlue
        StatusBar.Canvas.FillRect(Rect)
        StatusBar.Canvas.Font.Color = clWhite
        StatusBar.Canvas.TextOut(Rect.Left + 4, Rect.Top + 1, f"owner {Panel.Index}")

    panel_bar.OnDrawPanel = draw_owner_panel
    # AutoHint: ヒントのあるコントロールにマウスを載せると、OnHint(無ければ最初のパネル)に届く
    panel_bar.OnHint = lambda Sender: setattr(panel_bar.Panels.Items[0], "Text", "Hint: " + Application.Hint)
    panel_bar.OnMouseDown = lambda Sender, Button, Shift, X, Y: pr(
        f"panelBar GetPanelIndexAt({X}, {Y})={panel_bar.GetPanelIndexAt(X, Y)}")
    pr(f"panelBar Panels Count={panel_bar.Panels.Count} (expected 4), "
       f"Items[1] Style={int(panel_bar.Panels.Items[1].Style)} (expected psOwnerDraw={int(psOwnerDraw)}), "
       f"Items[2] Text={panel_bar.Panels.Items[2].Text} (expected center), "
       f"SizeGrip={'true' if panel_bar.SizeGrip else 'false'}")
    try:
        tempLabel.Caption
        pr("must not be reached")
    except ReferenceError:
        pr("Access after Free() raises ReferenceError: yes (Python only)")

    pr("Running (click the buttons, then close the window three times: the first two closes are blocked, or press Quit)...")
    Application.Run()

    pr(f"Run returned. Terminated={int(Application.Terminated)}")
    pr("OK (Form1 and its components are destroyed after main returns)")


if __name__ == "__main__":
    main()
