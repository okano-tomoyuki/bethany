# bethany-lcl

Python bindings for the [Lazarus](https://www.lazarus-ide.org/) LCL (Lazarus Component Library), with an API similar to C++Builder.
Part of [Bethany](https://github.com/okano-tomoyuki/bethany), which also provides the same API for C++ and a visual form designer for VS Code.

```sh
pip install bethany-lcl
```

The distribution name is `bethany-lcl`, and the module name is `beth`.

> **Preview.** Bethany is at version 0.x. The API may change between minor versions.
> Only Windows x64 is supported for now.

## Example

```python
from beth import *


class TForm1(TForm):
    def __init__(self, AOwner):
        super().__init__(AOwner)
        self.Caption = "Hello"
        self.Button1 = TButton(self)
        self.Button1.Parent = self
        self.Button1.Caption = "OK"
        self.Button1.OnClick = self.Button1Click

    def Button1Click(self, Sender):
        self.Caption = "Clicked"


Application.Initialize()
Form1 = Application.CreateForm(TForm1)
Application.Run()
```

Classes, properties and events have the same names as in C++Builder and the LCL (`TForm`, `TButton`, `Caption`, `OnClick` and so on).
The differences from C++ are:

- Arguments passed by reference in C++ (`int& Key`, `bool& CanClose`, `TCloseAction& Action`, ...) are `Ref` objects. Read and write them with `.value`.
- `Application->CreateForm(&Form1)` is `Form1 = Application.CreateForm(TForm1)`.
- Objects you create yourself (`TStringList`, `TBitmap`, `TPicture`, ...) are destroyed with `Free()`, or when no reference to them remains.
- Using a wrapper after its LCL object has been destroyed raises `ReferenceError`.
- Exceptions raised in the LCL are `BethError` (`E.Message`, `E.ClassName()`).

## Designing forms

[Bethany Designer](https://marketplace.visualstudio.com/items?itemName=okano-tomoyuki.bethany-designer) is a VS Code extension
to design forms on a canvas and generate the Python (or C++) code for them.

## License

Bethany is distributed under the MIT License. The package contains `beth.dll`, which is built with Free Pascal and statically contains
the Free Pascal run-time library and the LCL. They are distributed under the LGPL with a static linking exception.
See `THIRD-PARTY-NOTICES.md` and the `licenses` folder in the package metadata, or the
[repository](https://github.com/okano-tomoyuki/bethany/blob/master/THIRD-PARTY-NOTICES.md).

## 日本語

Lazarus の LCL を、C++Builder に似た API で Python から使うためのパッケージです。`pip install bethany-lcl` で入れ、`from beth import *` で使います。
今のところ Windows x64 だけに対応しています。C++ 版と、フォームを画面で設計する VS Code 拡張は [Bethany のリポジトリ](https://github.com/okano-tomoyuki/bethany)を参照してください。
