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

    std::printf("Showing form (click the button, then close the window to continue)...\n");
    no_vcl_TForm_ShowModal(form);

    no_vcl_TForm_Destroy(form);

    std::printf("OK\n");
    return 0;
}
