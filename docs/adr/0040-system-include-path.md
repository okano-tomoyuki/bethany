# 0040. ヘッダを include/bethany/ に置き、利用者は #include <bethany/beth.hpp> と書く

- 状態: 承認
- 日付: 2026-09-29

## 背景

これまで `beth.hpp` はリポジトリ(と配布の zip)の直下にあり、CMake が利用者に渡す include パスは、FetchContent ではその直下、
`cmake --install` した先では `include/beth` だった。利用者のコードとデザイナーが生成するコードは `#include "beth.hpp"` と書いていた。

Bethany は GUI の基盤のライブラリで、利用者のアプリケーションの一部ではない。標準ライブラリや他の外部のライブラリと同じく、
山括弧のシステムインクルードで、ライブラリの名前を含むパスで書けるようにしたい(利用者のヘッダとの名前の衝突を避け、どこから来たヘッダかを明らかにする)。

## 検討した選択肢

- 選択肢A: ファイルは直下のまま、CMake のビルドの中で `bethany/` の下へ写して include パスにする。リポジトリの配置と include の書き方が食い違い、
  `internal/` の相対の include も写す必要がある。
- 選択肢B: ヘッダを `include/bethany/`、ソースを `src/` へ実際に移す。一般的な C++ のライブラリの配置で、リポジトリ・zip・インストール先のどれでも
  `#include <bethany/beth.hpp>` になる。今までの `#include "beth.hpp"` は通らなくなる。
- 選択肢C: B に加えて、1 つの版だけ古いパスも通るように include パスを残す。

## 決定

**選択肢B を選ぶ。** 0.x の間はマイナーバージョンが変われば互換が無いものとしている([ADR 0038](0038-cpp-distribution.md))ので、古いパスは残さない。

- ヘッダは `include/bethany/beth.hpp` と `include/bethany/internal/`、ソースは `src/beth.cpp` と `src/internal/api.cpp`。
- CMake の include パスは `include/`(インストール先も `<prefix>/include/bethany/`)。ライブラリの中では、ヘッダどうしは相対パス
  (`"internal/api.h"`・`"funcs.h"`)、ソースからは `<bethany/...>` で include する。
- `find_package(beth)` で取り込んだターゲットの include パスは、CMake が `-isystem` で渡す(ライブラリのヘッダの警告が利用者のビルドに出ない)。
  FetchContent では、CMake 3.25 以降の `FetchContent_Declare(... SYSTEM)` で同じにできる。
- デザイナーが生成するコードも `#include <bethany/beth.hpp>` と書く(docs/designer/codegen-design.md)。
- CMake のパッケージの名前(`find_package(beth)`)とターゲット(`beth::beth`)は変えない。

## 影響

- 利用者は `#include "beth.hpp"` を `#include <bethany/beth.hpp>` に書き換える必要がある(C++ のライブラリ 0.2.0)。
- デザイナー 0.4.0 以降が生成するコードは、C++ のライブラリ 0.2.0 以降でないとビルドできない。
- Python のパッケージ(`bethany-lcl`)は C++ のヘッダを使わないため、影響は無い(生成スクリプト py/gen.py・gen_api.py の読むパスだけが変わる)。
