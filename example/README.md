# Bethany Notepad

A small text editor built with [Bethany](../README.md) and the [Bethany Designer](../designer/packages/extension/README.md).
The same application is written in C++ (`cpp/`) and in Python (`py/`) from the same forms.

- A main form with a menu (_File_, _Format_, _Help_), a memo and a status bar
- Open, Save and Font dialogs (`TOpenDialog`, `TSaveDialog`, `TFontDialog`)
- An _About_ form shown with `ShowModal()` from the main form through its form variable, closed by its OK button (`ModalResult = mrOk`,
  `Default`, `Cancel`)
- Asking to save the changes with `MessageDlg` when closing the window (`OnCloseQuery`) or opening another file

This example uses Bethany 0.3.0 or later (`ModalResult`, `MessageDlg`). Until 0.3.0 is released, build it with the Bethany in
this repository (below).

## Files

```
example/
  Notepad.bfproj.json          the project: forms, main form and code generation settings
  MainForm.bfm.json            forms (open them in VS Code with the Bethany Designer)
  dialogs/
    AboutForm.bfm.json
  CMakeLists.txt               builds the C++ version
  cpp/                         generated C++ code and the event handlers written by hand
    Notepad.cpp                startup code (main)
    MainForm.hpp / .cpp
    dialogs/AboutForm.hpp / .cpp
  py/                          generated Python code and the event handlers written by hand
    Notepad.py                 startup code
    MainForm.py
    dialogs/AboutForm.py
```

The code generation settings in `Notepad.bfproj.json` put the C++ code in `cpp/` and the Python code in `py/`
(`headerDir`, `sourceDir`, `moduleDir`), use the namespace `notepad`, and use `notepad::dialogs` for the forms in `dialogs/`
(`overrides`). Open the project file in VS Code to see and change them in the project settings.

Only the regions between the designer's markers (`<bethany-designer:begin ...>` / `<bethany-designer:end ...>`) are generated.
The event handlers and the other code outside them are written by hand and are kept when the code is generated again.

## C++

Requires MinGW-w64 g++, CMake 3.15 or later and Ninja on Windows x64. CMake downloads Bethany from the
[Releases](https://github.com/okano-tomoyuki/bethany/releases).

```sh
cmake -S . -B build -G Ninja
cmake --build build
build/Notepad.exe
```

To build with the Bethany in this repository instead (for example while developing it), build `beth.dll` with `build-windows.sh` first and run:

```sh
cmake -S . -B build -G Ninja -DFETCHCONTENT_SOURCE_DIR_BETH=..
```

## Python

Requires Python 3.8 or later on Windows x64.

```sh
pip install bethany-lcl
python py/Notepad.py
```

Start the application from `py/Notepad.py` (not from a form's `.py`), as described in _Using other forms_ of the designer's README.

## Editing the forms

1. Open this folder in VS Code with the Bethany Designer extension, and open a `.bfm.json` file.
2. Change the form. Double-click a control to add an event handler and jump to it.
3. Run _Generate Code for the Whole Project_ from the project in the Bethany Designer view (or from `Notepad.bfproj.json`).

## 日本語

Bethany とデザイナーで作った小さなメモ帳です。同じフォームから、C++(`cpp/`)と Python(`py/`)の両方を生成しています。
C++ は `cmake -S . -B build -G Ninja`・`cmake --build build` でビルドし(Bethany は Releases の zip を取り込む)、
Python は `pip install bethany-lcl` の後に `python py/Notepad.py` で起動します。
区間(マーカー)の外のイベントハンドラなどは手で書いたもので、コードを生成し直しても残ります。
