#include <cstdio>
#include <string>

#include "no_vcl.hpp"

namespace
{

int g_destroyedLabels = 0;

// ラッパーが実際に delete されたかを数えるためのラベル。
// コンポーネントの派生クラスは、ヒープ生成を強制するためデストラクタを protected にする。
class TTracedLabel : public no_vcl::TLabel
{
public:
    explicit TTracedLabel(no_vcl::TComponent* AOwner) : TLabel(AOwner) {}

protected:
    ~TTracedLabel() override { ++g_destroyedLabels; }
};

} // namespace

int main()
{
    no_vcl::TForm* form = new no_vcl::TForm(nullptr);
    form->Caption = "no_vcl C++ wrapper";
    form->Width = 640;
    form->Height = 420;
    std::printf("Caption: %s\n", std::string(form->Caption).c_str());

    // Owner(破棄の責任)はコンストラクタで、Parent(画面上の親)はプロパティで指定する。
    // Owner を持つコンポーネントは Owner の破棄に連動して破棄されるので、自分で Free() しなくてよい。
    no_vcl::TButton* button = new no_vcl::TButton(form);
    button->Parent = form;
    button->Caption = "Click me";
    button->Left = 20;
    button->Top = 20;
    button->Width = 100;
    button->Height = 30;

    int clicks = 0;
    button->SetOnClick([&clicks, button]() {
        ++clicks;
        std::printf("Clicked! count=%d\n", clicks);
        std::fflush(stdout);
        button->Caption = "Clicked " + std::to_string(clicks);
    });

    no_vcl::TLabel* label = new no_vcl::TLabel(form);
    label->Parent = form;
    label->Caption = "Label text";
    label->Left = 20;
    label->Top = 60;

    no_vcl::TEdit* edit = new no_vcl::TEdit(form);
    edit->Parent = form;
    edit->Text = "Edit me";
    edit->Left = 20;
    edit->Top = 90;
    edit->Width = 150;
    edit->SetOnChange([edit]() {
        std::printf("Edit changed! Text=%s\n", std::string(edit->Text).c_str());
        std::fflush(stdout);
    });

    no_vcl::TCheckBox* checkBox = new no_vcl::TCheckBox(form);
    checkBox->Parent = form;
    checkBox->Caption = "Check me";
    checkBox->Left = 20;
    checkBox->Top = 130;
    checkBox->SetOnClick([checkBox]() {
        std::printf("CheckBox clicked! Checked=%d\n", (bool)checkBox->Checked);
        std::fflush(stdout);
    });

    no_vcl::TRadioButton* radio1 = new no_vcl::TRadioButton(form);
    radio1->Parent = form;
    radio1->Caption = "Option A";
    radio1->Left = 20;
    radio1->Top = 160;
    radio1->Checked = true;
    radio1->SetOnClick([radio1]() {
        std::printf("RadioButton clicked! Checked=%d\n", (bool)radio1->Checked);
        std::fflush(stdout);
    });

    no_vcl::TRadioButton* radio2 = new no_vcl::TRadioButton(form);
    radio2->Parent = form;
    radio2->Caption = "Option B";
    radio2->Left = 20;
    radio2->Top = 190;
    radio2->SetOnClick([radio2]() {
        std::printf("RadioButton clicked! Checked=%d\n", (bool)radio2->Checked);
        std::fflush(stdout);
    });

    no_vcl::TPanel* panel = new no_vcl::TPanel(form);
    panel->Parent = form;
    panel->Left = 220;
    panel->Top = 20;
    panel->Width = 180;
    panel->Height = 60;

    // Panel の中にボタンを置く(Parent が TWinControl* なので Panel も親にできる)。
    no_vcl::TButton* panelButton = new no_vcl::TButton(form);
    panelButton->Parent = panel;
    panelButton->Caption = "In panel";
    panelButton->Left = 10;
    panelButton->Top = 15;
    panelButton->SetOnClick([panelButton, panel]() {
        no_vcl::TWinControl* parent = panelButton->Parent;
        std::printf("Panel button clicked! Parent is panel: %s\n", parent == panel ? "yes" : "no");
        std::fflush(stdout);
    });

    no_vcl::TGroupBox* groupBox = new no_vcl::TGroupBox(form);
    groupBox->Parent = form;
    groupBox->Caption = "Group";
    groupBox->Left = 220;
    groupBox->Top = 90;
    groupBox->Width = 180;
    groupBox->Height = 60;

    no_vcl::TComboBox* comboBox = new no_vcl::TComboBox(form);
    comboBox->Parent = form;
    comboBox->ItemsAdd("Combo A");
    comboBox->ItemsAdd("Combo B");
    comboBox->ItemsAdd("Combo C");
    comboBox->ItemIndex = 0;
    comboBox->Left = 220;
    comboBox->Top = 160;
    comboBox->Width = 150;
    comboBox->SetOnChange([comboBox]() {
        std::printf("ComboBox changed! ItemIndex=%d Text=%s\n",
                    (int)comboBox->ItemIndex, std::string(comboBox->Text).c_str());
        std::fflush(stdout);
    });

    no_vcl::TListBox* listBox = new no_vcl::TListBox(form);
    listBox->Parent = form;
    listBox->ItemsAdd("List 1");
    listBox->ItemsAdd("List 2");
    listBox->ItemsAdd("List 3");
    listBox->Left = 220;
    listBox->Top = 190;
    listBox->Width = 150;
    listBox->Height = 80;
    listBox->SetOnClick([listBox]() {
        std::printf("ListBox clicked! ItemIndex=%d\n", (int)listBox->ItemIndex);
        std::fflush(stdout);
    });

    // TMemo は TCustomEdit の派生なので、Text/ReadOnly/OnChange も TEdit と共通。
    no_vcl::TMemo* memo = new no_vcl::TMemo(form);
    memo->Parent = form;
    memo->LinesAdd("Memo line 1");
    memo->LinesAdd("Memo line 2");
    memo->Left = 220;
    memo->Top = 280;
    memo->Width = 150;
    memo->Height = 80;
    memo->SetOnChange([memo]() {
        std::printf("Memo changed! LineCount=%d\n", memo->LinesCount());
        std::fflush(stdout);
    });

    TTracedLabel* tickLabel = new TTracedLabel(form);
    tickLabel->Parent = form;
    tickLabel->Caption = "Tick: 0";
    tickLabel->Left = 20;
    tickLabel->Top = 230;

    int ticks = 0;
    no_vcl::TTimer* timer = new no_vcl::TTimer(form);
    timer->Interval = 500;
    timer->SetOnTimer([&ticks, tickLabel]() {
        ++ticks;
        tickLabel->Caption = "Tick: " + std::to_string(ticks);
        std::printf("Timer tick! count=%d\n", ticks);
        std::fflush(stdout);
    });
    timer->Enabled = true;

    no_vcl::TPaintBox* paintBox = new no_vcl::TPaintBox(form);
    paintBox->Parent = form;
    paintBox->Left = 400;
    paintBox->Top = 20;
    paintBox->Width = 220;
    paintBox->Height = 130;
    paintBox->SetOnPaint([paintBox]() {
        no_vcl::TCanvas& canvas = paintBox->Canvas;
        canvas.Pen.Color = no_vcl::clRed;
        canvas.Pen.Width = 2;
        canvas.Brush.Color = no_vcl::clYellow;
        canvas.Rectangle(10, 10, 110, 70);

        canvas.Pen.Color = no_vcl::clBlue;
        canvas.Brush.Color = no_vcl::clWhite;
        canvas.Ellipse(120, 10, 200, 70);

        canvas.Pen.Color = no_vcl::clBlack;
        canvas.MoveTo(10, 90);
        canvas.LineTo(200, 90);

        canvas.Font.Color = no_vcl::clGreen;
        canvas.Font.Size = 14;
        canvas.TextOut(10, 100, "Canvas drawing test");
    });

    no_vcl::TWinControl* buttonParent = button->Parent;
    std::printf("button->Parent is form: %s\n", buttonParent == form ? "yes" : "no");

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
