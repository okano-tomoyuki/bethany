#include <stdio.h>

#include "../no_vcl_c.h"

static int g_clickCount = 0;

static void NO_VCL_CALL OnButtonClick(no_vcl_obj_t sender)
{
    (void)sender;
    ++g_clickCount;
    printf("Button clicked! (count=%d)\n", g_clickCount);
    fflush(stdout);
}

static void NO_VCL_CALL OnCheckBoxClick(no_vcl_obj_t sender)
{
    printf("CheckBox clicked! Checked=%d\n", no_vcl_TCheckBox_GetChecked(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnRadioButtonClick(no_vcl_obj_t sender)
{
    printf("RadioButton clicked! Checked=%d\n", no_vcl_TRadioButton_GetChecked(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnEditChange(no_vcl_obj_t sender)
{
    printf("Edit changed! Text=%s\n", no_vcl_TEdit_GetText(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnComboBoxChange(no_vcl_obj_t sender)
{
    printf("ComboBox changed! ItemIndex=%d Text=%s\n",
           no_vcl_TComboBox_GetItemIndex(sender), no_vcl_TComboBox_GetText(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnListBoxClick(no_vcl_obj_t sender)
{
    printf("ListBox clicked! ItemIndex=%d\n", no_vcl_TListBox_GetItemIndex(sender));
    fflush(stdout);
}

static void NO_VCL_CALL OnMemoChange(no_vcl_obj_t sender)
{
    printf("Memo changed! LineCount=%d\n", no_vcl_TMemo_Lines_Count(sender));
    fflush(stdout);
}

static no_vcl_obj_t g_tickLabel = NULL;
static int g_tickCount = 0;

static void NO_VCL_CALL OnTimerTick(no_vcl_obj_t sender)
{
    char buf[64];
    (void)sender;
    ++g_tickCount;
    snprintf(buf, sizeof(buf), "Tick: %d", g_tickCount);
    no_vcl_TLabel_SetCaption(g_tickLabel, buf);
    printf("Timer tick! count=%d\n", g_tickCount);
    fflush(stdout);
}

int main(void)
{
    no_vcl_obj_t form = no_vcl_TForm_Create(NULL);
    no_vcl_obj_t button;
    no_vcl_obj_t label;
    no_vcl_obj_t edit;
    no_vcl_obj_t checkBox;
    no_vcl_obj_t radio1;
    no_vcl_obj_t radio2;
    no_vcl_obj_t panel;
    no_vcl_obj_t groupBox;
    no_vcl_obj_t comboBox;
    no_vcl_obj_t listBox;
    no_vcl_obj_t memo;
    no_vcl_obj_t timer;

    if (!form)
    {
        printf("TForm_Create failed\n");
        return 1;
    }

    no_vcl_TForm_SetCaption(form, "Hello from FPC DLL");
    no_vcl_TForm_SetWidth(form, 640);
    no_vcl_TForm_SetHeight(form, 420);
    printf("Caption: %s\n", no_vcl_TForm_GetCaption(form));

    button = no_vcl_TButton_Create(form);
    no_vcl_TButton_SetParent(button, form);
    no_vcl_TButton_SetCaption(button, "Click me");
    no_vcl_TButton_SetLeft(button, 20);
    no_vcl_TButton_SetTop(button, 20);
    no_vcl_TButton_SetWidth(button, 100);
    no_vcl_TButton_SetHeight(button, 30);
    no_vcl_TButton_SetOnClick(button, OnButtonClick);
    printf("Button caption: %s\n", no_vcl_TButton_GetCaption(button));

    label = no_vcl_TLabel_Create(form);
    no_vcl_TLabel_SetParent(label, form);
    no_vcl_TLabel_SetCaption(label, "Label text");
    no_vcl_TLabel_SetLeft(label, 20);
    no_vcl_TLabel_SetTop(label, 60);

    edit = no_vcl_TEdit_Create(form);
    no_vcl_TEdit_SetParent(edit, form);
    no_vcl_TEdit_SetText(edit, "Edit me");
    no_vcl_TEdit_SetLeft(edit, 20);
    no_vcl_TEdit_SetTop(edit, 90);
    no_vcl_TEdit_SetWidth(edit, 150);
    no_vcl_TEdit_SetOnChange(edit, OnEditChange);

    checkBox = no_vcl_TCheckBox_Create(form);
    no_vcl_TCheckBox_SetParent(checkBox, form);
    no_vcl_TCheckBox_SetCaption(checkBox, "Check me");
    no_vcl_TCheckBox_SetLeft(checkBox, 20);
    no_vcl_TCheckBox_SetTop(checkBox, 130);
    no_vcl_TCheckBox_SetOnClick(checkBox, OnCheckBoxClick);

    radio1 = no_vcl_TRadioButton_Create(form);
    no_vcl_TRadioButton_SetParent(radio1, form);
    no_vcl_TRadioButton_SetCaption(radio1, "Option A");
    no_vcl_TRadioButton_SetLeft(radio1, 20);
    no_vcl_TRadioButton_SetTop(radio1, 160);
    no_vcl_TRadioButton_SetChecked(radio1, 1);
    no_vcl_TRadioButton_SetOnClick(radio1, OnRadioButtonClick);

    radio2 = no_vcl_TRadioButton_Create(form);
    no_vcl_TRadioButton_SetParent(radio2, form);
    no_vcl_TRadioButton_SetCaption(radio2, "Option B");
    no_vcl_TRadioButton_SetLeft(radio2, 20);
    no_vcl_TRadioButton_SetTop(radio2, 190);
    no_vcl_TRadioButton_SetOnClick(radio2, OnRadioButtonClick);

    panel = no_vcl_TPanel_Create(form);
    no_vcl_TPanel_SetParent(panel, form);
    no_vcl_TPanel_SetCaption(panel, "");
    no_vcl_TPanel_SetLeft(panel, 220);
    no_vcl_TPanel_SetTop(panel, 20);
    no_vcl_TPanel_SetWidth(panel, 180);
    no_vcl_TPanel_SetHeight(panel, 60);

    groupBox = no_vcl_TGroupBox_Create(form);
    no_vcl_TGroupBox_SetParent(groupBox, form);
    no_vcl_TGroupBox_SetCaption(groupBox, "Group");
    no_vcl_TGroupBox_SetLeft(groupBox, 220);
    no_vcl_TGroupBox_SetTop(groupBox, 90);
    no_vcl_TGroupBox_SetWidth(groupBox, 180);
    no_vcl_TGroupBox_SetHeight(groupBox, 60);

    comboBox = no_vcl_TComboBox_Create(form);
    no_vcl_TComboBox_SetParent(comboBox, form);
    no_vcl_TComboBox_Items_Add(comboBox, "Combo A");
    no_vcl_TComboBox_Items_Add(comboBox, "Combo B");
    no_vcl_TComboBox_Items_Add(comboBox, "Combo C");
    no_vcl_TComboBox_SetItemIndex(comboBox, 0);
    no_vcl_TComboBox_SetLeft(comboBox, 220);
    no_vcl_TComboBox_SetTop(comboBox, 160);
    no_vcl_TComboBox_SetWidth(comboBox, 150);
    no_vcl_TComboBox_SetOnChange(comboBox, OnComboBoxChange);

    listBox = no_vcl_TListBox_Create(form);
    no_vcl_TListBox_SetParent(listBox, form);
    no_vcl_TListBox_Items_Add(listBox, "List 1");
    no_vcl_TListBox_Items_Add(listBox, "List 2");
    no_vcl_TListBox_Items_Add(listBox, "List 3");
    no_vcl_TListBox_SetLeft(listBox, 220);
    no_vcl_TListBox_SetTop(listBox, 190);
    no_vcl_TListBox_SetWidth(listBox, 150);
    no_vcl_TListBox_SetHeight(listBox, 80);
    no_vcl_TListBox_SetOnClick(listBox, OnListBoxClick);

    memo = no_vcl_TMemo_Create(form);
    no_vcl_TMemo_SetParent(memo, form);
    no_vcl_TMemo_Lines_Add(memo, "Memo line 1");
    no_vcl_TMemo_Lines_Add(memo, "Memo line 2");
    no_vcl_TMemo_SetLeft(memo, 220);
    no_vcl_TMemo_SetTop(memo, 280);
    no_vcl_TMemo_SetWidth(memo, 150);
    no_vcl_TMemo_SetHeight(memo, 80);
    no_vcl_TMemo_SetOnChange(memo, OnMemoChange);

    g_tickLabel = no_vcl_TLabel_Create(form);
    no_vcl_TLabel_SetParent(g_tickLabel, form);
    no_vcl_TLabel_SetCaption(g_tickLabel, "Tick: 0");
    no_vcl_TLabel_SetLeft(g_tickLabel, 20);
    no_vcl_TLabel_SetTop(g_tickLabel, 230);

    timer = no_vcl_TTimer_Create(form);
    no_vcl_TTimer_SetInterval(timer, 500);
    no_vcl_TTimer_SetOnTimer(timer, OnTimerTick);
    no_vcl_TTimer_SetEnabled(timer, 1);

    printf("Showing form (click the button, then close the window to continue)...\n");
    no_vcl_TForm_ShowModal(form);

    no_vcl_TForm_Destroy(form);

    printf("OK\n");
    return 0;
}
