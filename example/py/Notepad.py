"""Application created with the Bethany designer (Notepad.bfproj.json). Regions enclosed in markers are overwritten when regenerated."""
from beth import *

# <bethany-designer:begin id="imports">
import MainForm
import dialogs.AboutForm
# <bethany-designer:end id="imports" hash="62157782">


def main():
    Application.Initialize()
    # <bethany-designer:begin id="beth_CreateForms">
    MainForm.MainForm = Application.CreateForm(MainForm.TMainForm)
    dialogs.AboutForm.AboutForm = Application.CreateForm(dialogs.AboutForm.TAboutForm)
    # <bethany-designer:end id="beth_CreateForms" hash="40a1ce77">
    Application.Run()


if __name__ == "__main__":
    main()
