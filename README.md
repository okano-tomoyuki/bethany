# Bethany

C++/Python bindings for the Lazarus LCL

[Free Pascal](https://www.freepascal.org/) と [Lazarus](https://www.lazarus-ide.org/) の LCL(Lazarus Component Library)を、
薄いラッパー経由で C++ と Python から使えるようにする、C++Builder に似た API のライブラリ。
フォームを画面で設計するデザイナー(VS Code 拡張)を [designer/](designer/README.md) で開発している。

設計は [docs/](docs/README.md) を、実装の進捗は [todo.md](todo.md) を参照。

## C++ で使う

対象は Windows x64 と MinGW-w64 の g++(C++11 以降)、CMake 3.15 以降。
[Releases](https://github.com/okano-tomoyuki/bethany/releases) の `bethany-<バージョン>-win64.zip` には、C++ のソースとビルド済みの `beth.dll` が入っている。
C++ の部分は利用者のコンパイラでビルドする(g++ のバージョンやランタイムの違いで壊れないように。[ADR 0038](docs/adr/0038-cpp-distribution.md))。

FetchContent で取り込む場合:

```cmake
cmake_minimum_required(VERSION 3.15)
project(app CXX)

include(FetchContent)
FetchContent_Declare(beth
    URL https://github.com/okano-tomoyuki/bethany/releases/download/v0.1.1/bethany-0.1.1-win64.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(beth)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE beth::beth)
beth_deploy(app)   # beth.dll を app.exe の隣に写す
```

```cpp
// main.cpp
#include <bethany/beth.hpp>

using namespace beth;
```

ヘッダは山括弧のシステムインクルードで `<bethany/beth.hpp>` と書く([ADR 0040](docs/adr/0040-system-include-path.md))。
CMake 3.25 以降なら、`FetchContent_Declare` に `SYSTEM` を付けると、ライブラリのヘッダの警告が利用者のビルドに出なくなる
(`find_package` で使う場合は何もしなくてもそうなる)。

インストールして使う場合は、zip を展開したフォルダで `cmake -S . -B build -G Ninja`・`cmake --build build`・`cmake --install build --prefix <場所>` を実行し、
利用側では `find_package(beth 0.1 REQUIRED)` の後に上と同じく `beth::beth` をリンクして `beth_deploy()` を呼ぶ。

アプリケーションを配るときは、exe と `beth.dll` に加えて [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) と [licenses/](licenses/) を同梱する
(`beth.dll` は LCL を含む)。MinGW のランタイムは exe に静的にリンクされるため、`libstdc++-6.dll` などを同梱する必要はない。

| CMake のオプション | 既定 | 内容 |
|---|---|---|
| `BETH_STATIC_RUNTIME` | ON | MinGW のランタイム(libgcc・libstdc++・winpthread)を exe に静的にリンクする |
| `BETH_EMBED_MANIFEST` | ON | exe に Common-Controls 6.0 の manifest を埋め込む(自前の manifest を持つ exe では OFF) |
| `BETH_DLL` | ソースのフォルダの `beth.dll` | `beth_deploy()` が写す DLL |

## Python で使う

対象は Windows x64 と Python 3.8 以降。PyPI の [bethany-lcl](https://pypi.org/project/bethany-lcl/) を入れ、`beth` を import する
(`beth.dll` は wheel に入っている)。書き方は [py/README.md](py/README.md) を参照。

```sh
pip install bethany-lcl
```

```python
from beth import *
```

## 名前

Bethany(ベタニア)は、聖書でラザロ(Lazarus)が暮らした村の名前。Delphi・Lazarus と同じく神話や聖書の固有名詞にちなみ、
このライブラリが Lazarus の成果の上に成り立っていることを表す。略称の beth はヘブライ語の文字 ב(ベート、「家」の意味)の名前で、
ファイル名・C++ の名前空間・Python のモジュール名に使う(`bethany/beth.hpp`・`namespace beth`・`import beth`)。
経緯は [ADR 0037](docs/adr/0037-rename-to-bethany.md)。

## 謝辞

画面の部品とその振る舞いは、すべて Lazarus の LCL が提供している。Free Pascal と Lazarus の開発者・貢献者に感謝する。
LCL は modified LGPL(静的リンクの例外付き)で配布されており、Bethany の共有ライブラリ(`beth.dll`・`libbeth.so`)は LCL を含む。
詳しくは [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) を参照。

## ライセンス

Bethany は [MIT License](LICENSE)。`beth.dll` に含まれる Free Pascal と LCL のライセンスは [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) を参照。
