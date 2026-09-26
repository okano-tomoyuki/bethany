#include <cstdio>
#include <string>

#include "no_vcl.hpp"

using namespace no_vcl;

namespace
{

int g_destroyedLabels = 0;

// ラッパーが実際に delete されたかを数えるためのラベル。
// コンポーネントの派生クラスは、ヒープ生成を強制するためデストラクタを protected にする。
class TTracedLabel : public TLabel
{
public:
    explicit TTracedLabel(TComponent* AOwner) : TLabel(AOwner) {}

protected:
    ~TTracedLabel() override { ++g_destroyedLabels; }
};

// デザイナーが生成することを想定した形のフォーム。
// コントロールは生ポインタメンバとして持ち、コンストラクタ本体で new する(破棄は Owner に任せる)。
// イベントハンドラはメンバ関数にし、[this] のラムダで割り当てる。
class TMainForm : public TForm
{
public:
    TButton*      Button1;
    TLabel*       Label1;
    TEdit*        Edit1;
    TCheckBox*    CheckBox1;
    TRadioButton* RadioButton1;
    TRadioButton* RadioButton2;
    TPanel*       Panel1;
    TButton*      PanelButton;
    TGroupBox*    GroupBox1;
    TComboBox*    ComboBox1;
    TListBox*     ListBox1;
    TMemo*        Memo1;
    TTracedLabel* TickLabel;
    TTimer*       Timer1;
    TPaintBox*    PaintBox1;

    TMainForm() : TForm(nullptr)
    {
        Caption = "no_vcl C++ wrapper";
        Width = 640;
        Height = 420;

        Button1 = new TButton(this);
        Button1->Parent = this;
        Button1->Caption = "Click me";
        Button1->Left = 20;
        Button1->Top = 20;
        Button1->Width = 100;
        Button1->Height = 30;
        Button1->OnClick = [this](TObject* Sender) { Button1Click(Sender); };

        Label1 = new TLabel(this);
        Label1->Parent = this;
        Label1->Caption = "Label text";
        Label1->Left = 20;
        Label1->Top = 60;

        Edit1 = new TEdit(this);
        Edit1->Parent = this;
        Edit1->Text = "Edit me";
        Edit1->Left = 20;
        Edit1->Top = 90;
        Edit1->Width = 150;
        Edit1->OnChange = [this](TObject* Sender) { TextChange(Sender); };

        CheckBox1 = new TCheckBox(this);
        CheckBox1->Parent = this;
        CheckBox1->Caption = "Check me";
        CheckBox1->Left = 20;
        CheckBox1->Top = 130;
        CheckBox1->OnClick = [this](TObject* Sender) { CheckBox1Click(Sender); };

        // 2つのラジオボタンで同じハンドラを共有し、Sender でどちらが押されたかを区別する。
        RadioButton1 = new TRadioButton(this);
        RadioButton1->Parent = this;
        RadioButton1->Caption = "Option A";
        RadioButton1->Left = 20;
        RadioButton1->Top = 160;
        RadioButton1->Checked = true;
        RadioButton1->OnClick = [this](TObject* Sender) { RadioButtonClick(Sender); };

        RadioButton2 = new TRadioButton(this);
        RadioButton2->Parent = this;
        RadioButton2->Caption = "Option B";
        RadioButton2->Left = 20;
        RadioButton2->Top = 190;
        RadioButton2->OnClick = RadioButton1->OnClick;

        Panel1 = new TPanel(this);
        Panel1->Parent = this;
        Panel1->Left = 220;
        Panel1->Top = 20;
        Panel1->Width = 180;
        Panel1->Height = 60;

        // Panel の中にボタンを置く(Parent が TWinControl* なので Panel も親にできる)。
        PanelButton = new TButton(this);
        PanelButton->Parent = Panel1;
        PanelButton->Caption = "In panel";
        PanelButton->Left = 10;
        PanelButton->Top = 15;
        PanelButton->OnClick = [this](TObject* Sender) { PanelButtonClick(Sender); };

        GroupBox1 = new TGroupBox(this);
        GroupBox1->Parent = this;
        GroupBox1->Caption = "Group";
        GroupBox1->Left = 220;
        GroupBox1->Top = 90;
        GroupBox1->Width = 180;
        GroupBox1->Height = 60;

        ComboBox1 = new TComboBox(this);
        ComboBox1->Parent = this;
        ComboBox1->ItemsAdd("Combo A");
        ComboBox1->ItemsAdd("Combo B");
        ComboBox1->ItemsAdd("Combo C");
        ComboBox1->ItemIndex = 0;
        ComboBox1->Left = 220;
        ComboBox1->Top = 160;
        ComboBox1->Width = 150;
        ComboBox1->OnChange = [this](TObject* Sender) { TextChange(Sender); };

        ListBox1 = new TListBox(this);
        ListBox1->Parent = this;
        ListBox1->ItemsAdd("List 1");
        ListBox1->ItemsAdd("List 2");
        ListBox1->ItemsAdd("List 3");
        ListBox1->Left = 220;
        ListBox1->Top = 190;
        ListBox1->Width = 150;
        ListBox1->Height = 80;
        ListBox1->OnClick = [this](TObject* Sender) { ListBox1Click(Sender); };

        // TMemo は TCustomEdit の派生なので、Edit1 と同じ TextChange を共有できる。
        Memo1 = new TMemo(this);
        Memo1->Parent = this;
        Memo1->LinesAdd("Memo line 1");
        Memo1->LinesAdd("Memo line 2");
        Memo1->Left = 220;
        Memo1->Top = 280;
        Memo1->Width = 150;
        Memo1->Height = 80;
        Memo1->OnChange = Edit1->OnChange;

        TickLabel = new TTracedLabel(this);
        TickLabel->Parent = this;
        TickLabel->Caption = "Tick: 0";
        TickLabel->Left = 20;
        TickLabel->Top = 230;

        Timer1 = new TTimer(this);
        Timer1->Interval = 500;
        Timer1->OnTimer = [this](TObject* Sender) { Timer1Timer(Sender); };
        Timer1->Enabled = true;

        PaintBox1 = new TPaintBox(this);
        PaintBox1->Parent = this;
        PaintBox1->Left = 400;
        PaintBox1->Top = 20;
        PaintBox1->Width = 220;
        PaintBox1->Height = 130;
        PaintBox1->OnPaint = [this](TObject* Sender) { PaintBox1Paint(Sender); };
    }

protected:
    ~TMainForm() override = default;

private:
    int clicks_ = 0;
    int ticks_ = 0;

    void Button1Click(TObject* Sender)
    {
        TButton* button = static_cast<TButton*>(Sender);
        ++clicks_;
        button->Caption = "Clicked " + std::to_string(clicks_);
        std::printf("Button1Click: Sender is Button1: %s, count=%d\n", Sender == Button1 ? "yes" : "no", clicks_);
        std::fflush(stdout);
    }

    void CheckBox1Click(TObject* Sender)
    {
        std::printf("CheckBox1Click: Checked=%d\n", (bool)static_cast<TCheckBox*>(Sender)->Checked);
        std::fflush(stdout);
    }

    void RadioButtonClick(TObject* Sender)
    {
        TRadioButton* radio = static_cast<TRadioButton*>(Sender);
        std::printf("RadioButtonClick: %s Checked=%d\n", std::string(radio->Caption).c_str(), (bool)radio->Checked);
        std::fflush(stdout);
    }

    void PanelButtonClick(TObject* Sender)
    {
        TWinControl* parent = static_cast<TControl*>(Sender)->Parent;
        std::printf("PanelButtonClick: Parent is Panel1: %s\n", parent == Panel1 ? "yes" : "no");
        std::fflush(stdout);
    }

    // Edit1 / Memo1 / ComboBox1 で共有する。dynamic_cast で Sender の種類を判定する。
    void TextChange(TObject* Sender)
    {
        const char* kind = dynamic_cast<TMemo*>(Sender)      ? "Memo"
                         : dynamic_cast<TCustomEdit*>(Sender) ? "Edit"
                         : dynamic_cast<TComboBox*>(Sender)   ? "ComboBox"
                         : "?";
        std::string text = dynamic_cast<TComboBox*>(Sender) ? std::string(ComboBox1->Text)
                         : std::string(static_cast<TCustomEdit*>(Sender)->Text);
        std::printf("TextChange: %s Text=%s\n", kind, text.c_str());
        std::fflush(stdout);
    }

    void ListBox1Click(TObject* Sender)
    {
        std::printf("ListBox1Click: ItemIndex=%d\n", (int)static_cast<TListBox*>(Sender)->ItemIndex);
        std::fflush(stdout);
    }

    void Timer1Timer(TObject*)
    {
        ++ticks_;
        TickLabel->Caption = "Tick: " + std::to_string(ticks_);
        std::printf("Timer tick! count=%d\n", ticks_);
        std::fflush(stdout);
    }

    void PaintBox1Paint(TObject* Sender)
    {
        TCanvas& canvas = static_cast<TPaintBox*>(Sender)->Canvas;
        canvas.Pen.Color = clRed;
        canvas.Pen.Width = 2;
        canvas.Brush.Color = clYellow;
        canvas.Rectangle(10, 10, 110, 70);

        canvas.Pen.Color = clBlue;
        canvas.Brush.Color = clWhite;
        canvas.Ellipse(120, 10, 200, 70);

        canvas.Pen.Color = clBlack;
        canvas.MoveTo(10, 90);
        canvas.LineTo(200, 90);

        canvas.Font.Color = clGreen;
        canvas.Font.Size = 14;
        canvas.TextOut(10, 100, "Canvas drawing test");
    }
};

} // namespace

int main()
{
    TMainForm* form = new TMainForm();
    std::printf("Caption: %s\n", std::string(form->Caption).c_str());

    TWinControl* buttonParent = form->Button1->Parent;
    std::printf("Button1->Parent is form: %s\n", buttonParent == form ? "yes" : "no");

    // ハンドラの解除(nullptr の代入)。解除したボタンを押しても何も起きない。
    TButton* disabledHandler = new TButton(form);
    disabledHandler->Parent = form;
    disabledHandler->Caption = "No handler";
    disabledHandler->Left = 20;
    disabledHandler->Top = 280;
    disabledHandler->OnClick = [](TObject*) { std::printf("must not be called\n"); };
    disabledHandler->OnClick = nullptr;

    // Free() で個別に破棄すると、ラッパーも破棄される。
    TTracedLabel* tempLabel = new TTracedLabel(form);
    tempLabel->Parent = form;
    tempLabel->Free();
    std::printf("Destroyed labels after Free(): %d (expected 1)\n", g_destroyedLabels);

    std::printf("Showing form (click the button, then close the window to continue)...\n");
    std::fflush(stdout);
    form->ShowModal();

    // Owner である form を破棄すると、form が所有するコンポーネントのラッパーもまとめて破棄される。
    form->Free();
    std::printf("Destroyed labels after form->Free(): %d (expected 2)\n", g_destroyedLabels);

    std::printf("OK\n");
    return 0;
}
