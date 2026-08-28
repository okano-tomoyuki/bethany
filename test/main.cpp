#include <cstdio>
#include <string>

#include "../no_vcl.hpp"

int main()
{
    no_vcl::TForm form;
    form.Caption = "no_vcl C++ wrapper";
    form.Width = 640;
    form.Height = 420;
    std::printf("Caption: %s\n", std::string(form.Caption).c_str());

    no_vcl::TButton button(form);
    button.Caption = "Click me";
    button.Left = 20;
    button.Top = 20;
    button.Width = 100;
    button.Height = 30;

    int clicks = 0;
    button.SetOnClick([&clicks, &button]() {
        ++clicks;
        std::printf("Clicked! count=%d\n", clicks);
        std::fflush(stdout);
        button.Caption = "Clicked " + std::to_string(clicks);
    });

    no_vcl::TLabel label(form);
    label.Caption = "Label text";
    label.Left = 20;
    label.Top = 60;

    no_vcl::TEdit edit(form);
    edit.Text = "Edit me";
    edit.Left = 20;
    edit.Top = 90;
    edit.Width = 150;
    edit.SetOnChange([&edit]() {
        std::printf("Edit changed! Text=%s\n", std::string(edit.Text).c_str());
        std::fflush(stdout);
    });

    no_vcl::TCheckBox checkBox(form);
    checkBox.Caption = "Check me";
    checkBox.Left = 20;
    checkBox.Top = 130;
    checkBox.SetOnClick([&checkBox]() {
        std::printf("CheckBox clicked! Checked=%d\n", (bool)checkBox.Checked);
        std::fflush(stdout);
    });

    no_vcl::TRadioButton radio1(form);
    radio1.Caption = "Option A";
    radio1.Left = 20;
    radio1.Top = 160;
    radio1.Checked = true;
    radio1.SetOnClick([&radio1]() {
        std::printf("RadioButton clicked! Checked=%d\n", (bool)radio1.Checked);
        std::fflush(stdout);
    });

    no_vcl::TRadioButton radio2(form);
    radio2.Caption = "Option B";
    radio2.Left = 20;
    radio2.Top = 190;
    radio2.SetOnClick([&radio2]() {
        std::printf("RadioButton clicked! Checked=%d\n", (bool)radio2.Checked);
        std::fflush(stdout);
    });

    no_vcl::TPanel panel(form);
    panel.Left = 220;
    panel.Top = 20;
    panel.Width = 180;
    panel.Height = 60;

    no_vcl::TGroupBox groupBox(form);
    groupBox.Caption = "Group";
    groupBox.Left = 220;
    groupBox.Top = 90;
    groupBox.Width = 180;
    groupBox.Height = 60;

    no_vcl::TComboBox comboBox(form);
    comboBox.ItemsAdd("Combo A");
    comboBox.ItemsAdd("Combo B");
    comboBox.ItemsAdd("Combo C");
    comboBox.ItemIndex = 0;
    comboBox.Left = 220;
    comboBox.Top = 160;
    comboBox.Width = 150;
    comboBox.SetOnChange([&comboBox]() {
        std::printf("ComboBox changed! ItemIndex=%d Text=%s\n",
                     (int)comboBox.ItemIndex, std::string(comboBox.Text).c_str());
        std::fflush(stdout);
    });

    no_vcl::TListBox listBox(form);
    listBox.ItemsAdd("List 1");
    listBox.ItemsAdd("List 2");
    listBox.ItemsAdd("List 3");
    listBox.Left = 220;
    listBox.Top = 190;
    listBox.Width = 150;
    listBox.Height = 80;
    listBox.SetOnClick([&listBox]() {
        std::printf("ListBox clicked! ItemIndex=%d\n", (int)listBox.ItemIndex);
        std::fflush(stdout);
    });

    no_vcl::TMemo memo(form);
    memo.LinesAdd("Memo line 1");
    memo.LinesAdd("Memo line 2");
    memo.Left = 220;
    memo.Top = 280;
    memo.Width = 150;
    memo.Height = 80;
    memo.SetOnChange([&memo]() {
        std::printf("Memo changed! LineCount=%d\n", memo.LinesCount());
        std::fflush(stdout);
    });

    no_vcl::TLabel tickLabel(form);
    tickLabel.Caption = "Tick: 0";
    tickLabel.Left = 20;
    tickLabel.Top = 230;

    int ticks = 0;
    no_vcl::TTimer timer(form);
    timer.Interval = 500;
    timer.SetOnTimer([&ticks, &tickLabel]() {
        ++ticks;
        tickLabel.Caption = "Tick: " + std::to_string(ticks);
        std::printf("Timer tick! count=%d\n", ticks);
        std::fflush(stdout);
    });
    timer.Enabled = true;

    std::printf("Showing form (click the button, then close the window to continue)...\n");
    std::fflush(stdout);
    form.ShowModal();

    std::printf("OK\n");
    return 0;
}
