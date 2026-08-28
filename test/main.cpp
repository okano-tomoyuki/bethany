#include <cstdio>
#include <string>

#include "../no_vcl.hpp"

int main()
{
    no_vcl::TForm form;
    form.Caption = "no_vcl C++ wrapper";
    form.Width = 400;
    form.Height = 300;
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

    std::printf("Showing form (click the button, then close the window to continue)...\n");
    std::fflush(stdout);
    form.ShowModal();

    std::printf("OK\n");
    return 0;
}
