"""Application created with the Bethany designer (Notepad.bfproj.json). Regions enclosed in markers are overwritten when regenerated."""
from beth import *

# <bethany-designer:begin id="imports">
import MainForm
import dialogs.AboutForm
import dialogs.ConfirmSaveForm
# <bethany-designer:end id="imports" hash="2146414d">


def main():
    Application.Initialize()
    # <bethany-designer:begin id="beth_CreateForms">
    MainForm.MainForm = Application.CreateForm(MainForm.TMainForm)
    dialogs.AboutForm.AboutForm = Application.CreateForm(dialogs.AboutForm.TAboutForm)
    dialogs.ConfirmSaveForm.ConfirmSaveForm = Application.CreateForm(dialogs.ConfirmSaveForm.TConfirmSaveForm)
    # <bethany-designer:end id="beth_CreateForms" hash="9891bee5">
    Application.Run()


if __name__ == "__main__":
    main()
