// Application created with the Bethany designer (Notepad.bfproj.json). Regions enclosed in markers are overwritten when regenerated.
// <bethany-designer:begin id="beth_Include">
#include <bethany/beth.hpp>
// <bethany-designer:end id="beth_Include" hash="a7887dfb">
// <bethany-designer:begin id="includes">
#include "MainForm.hpp"
#include "dialogs/AboutForm.hpp"
#include "dialogs/ConfirmSaveForm.hpp"
// <bethany-designer:end id="includes" hash="7da80395">

using namespace beth;

int main()
{
    Application->Initialize();
    // <bethany-designer:begin id="beth_CreateForms">
    Application->CreateForm(&notepad::MainForm);
    Application->CreateForm(&notepad::dialogs::AboutForm);
    Application->CreateForm(&notepad::dialogs::ConfirmSaveForm);
    // <bethany-designer:end id="beth_CreateForms" hash="e4abdefc">
    Application->Run();
    return 0;
}
