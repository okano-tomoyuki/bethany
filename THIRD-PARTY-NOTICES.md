# Third-party notices

Bethany itself (`beth.pas`, `beth.hpp`, `beth.cpp`, `internal/`, `py/`, `designer/`) is distributed under the MIT License (see [LICENSE](LICENSE)).

The Bethany shared library (`beth.dll` on Windows, `libbeth.so` on Linux) is built from `beth.pas` with Free Pascal
and statically contains the following components. Their licenses apply to those parts of the shared library.

| Component | Version used for the prebuilt `beth.dll` | License | Source |
|---|---|---|---|
| Free Pascal run-time library and packages | 3.2.2 | LGPL 2 with the static linking exception ([COPYING.FPC](licenses/COPYING.FPC.txt), [COPYING.LGPL](licenses/COPYING.LGPL.txt)) | <https://www.freepascal.org/download.html> |
| Lazarus Component Library (LCL) and LazUtils | 4.8.0 | LGPL 2 with the static linking exception ([COPYING.modifiedLGPL](licenses/COPYING.modifiedLGPL.txt), [COPYING.LGPL](licenses/COPYING.LGPL.txt)) | <https://www.lazarus-ide.org/index.php?page=downloads> |

## Replacing the LCL in `beth.dll`

The complete source of the Bethany part of `beth.dll` is `beth.pas` in this distribution.
To use a modified version of the Free Pascal libraries or the LCL, rebuild `beth.dll` from `beth.pas` against them
(see `build-windows.sh` in the [Bethany repository](https://github.com/okano-tomoyuki/bethany) for the compiler options),
and put the rebuilt `beth.dll` next to the executable in place of the original one.
Applications load `beth.dll` at run time, so they do not need to be relinked.

## Acknowledgements

The controls and their behavior are provided by the LCL. Thanks to the developers and contributors of Free Pascal and Lazarus.
