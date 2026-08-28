#include <cstdio>

#include "../no_vcl_impl.h"

namespace
{

int g_clickCount = 0;

void NO_VCL_CALL OnButtonClick(no_vcl_obj_t /*sender*/)
{
    ++g_clickCount;
    std::printf("Button clicked! (count=%d)\n", g_clickCount);
    std::fflush(stdout);
}

void NO_VCL_CALL OnCheckBoxClick(no_vcl_obj_t sender)
{
    std::printf("CheckBox clicked! Checked=%d\n", no_vcl_TCheckBox_GetChecked(sender));
    std::fflush(stdout);
}

void NO_VCL_CALL OnRadioButtonClick(no_vcl_obj_t sender)
{
    std::printf("RadioButton clicked! Checked=%d\n", no_vcl_TRadioButton_GetChecked(sender));
    std::fflush(stdout);
}

void NO_VCL_CALL OnEditChange(no_vcl_obj_t sender)
{
    std::printf("Edit changed! Text=%s\n", no_vcl_TEdit_GetText(sender));
    std::fflush(stdout);
}

} // namespace

int main()
{
    no_vcl_obj_t form = no_vcl_TForm_Create(nullptr);
    if (!form)
    {
        std::printf("TForm_Create failed\n");
        return 1;
    }

    no_vcl_TForm_SetCaption(form, "Hello from FPC DLL");
    no_vcl_TForm_SetWidth(form, 400);
    no_vcl_TForm_SetHeight(form, 300);
    std::printf("Caption: %s\n", no_vcl_TForm_GetCaption(form));

    no_vcl_obj_t button = no_vcl_TButton_Create(form);
    no_vcl_TButton_SetParent(button, form);
    no_vcl_TButton_SetCaption(button, "Click me");
    no_vcl_TButton_SetLeft(button, 20);
    no_vcl_TButton_SetTop(button, 20);
    no_vcl_TButton_SetWidth(button, 100);
    no_vcl_TButton_SetHeight(button, 30);
    no_vcl_TButton_SetOnClick(button, OnButtonClick);
    std::printf("Button caption: %s\n", no_vcl_TButton_GetCaption(button));

    no_vcl_obj_t label = no_vcl_TLabel_Create(form);
    no_vcl_TLabel_SetParent(label, form);
    no_vcl_TLabel_SetCaption(label, "Label text");
    no_vcl_TLabel_SetLeft(label, 20);
    no_vcl_TLabel_SetTop(label, 60);

    no_vcl_obj_t edit = no_vcl_TEdit_Create(form);
    no_vcl_TEdit_SetParent(edit, form);
    no_vcl_TEdit_SetText(edit, "Edit me");
    no_vcl_TEdit_SetLeft(edit, 20);
    no_vcl_TEdit_SetTop(edit, 90);
    no_vcl_TEdit_SetWidth(edit, 150);
    no_vcl_TEdit_SetOnChange(edit, OnEditChange);

    no_vcl_obj_t checkBox = no_vcl_TCheckBox_Create(form);
    no_vcl_TCheckBox_SetParent(checkBox, form);
    no_vcl_TCheckBox_SetCaption(checkBox, "Check me");
    no_vcl_TCheckBox_SetLeft(checkBox, 20);
    no_vcl_TCheckBox_SetTop(checkBox, 130);
    no_vcl_TCheckBox_SetOnClick(checkBox, OnCheckBoxClick);

    no_vcl_obj_t radio1 = no_vcl_TRadioButton_Create(form);
    no_vcl_TRadioButton_SetParent(radio1, form);
    no_vcl_TRadioButton_SetCaption(radio1, "Option A");
    no_vcl_TRadioButton_SetLeft(radio1, 20);
    no_vcl_TRadioButton_SetTop(radio1, 160);
    no_vcl_TRadioButton_SetChecked(radio1, 1);
    no_vcl_TRadioButton_SetOnClick(radio1, OnRadioButtonClick);

    no_vcl_obj_t radio2 = no_vcl_TRadioButton_Create(form);
    no_vcl_TRadioButton_SetParent(radio2, form);
    no_vcl_TRadioButton_SetCaption(radio2, "Option B");
    no_vcl_TRadioButton_SetLeft(radio2, 20);
    no_vcl_TRadioButton_SetTop(radio2, 190);
    no_vcl_TRadioButton_SetOnClick(radio2, OnRadioButtonClick);

    std::printf("Showing form (click the button, then close the window to continue)...\n");
    no_vcl_TForm_ShowModal(form);

    no_vcl_TForm_Destroy(form);

    std::printf("OK\n");
    return 0;
}
