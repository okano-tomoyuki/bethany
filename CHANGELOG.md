# Changelog

Bethany の C++ ライブラリ(`beth.hpp`・`beth.dll`)と Python のパッケージ(`bethany-lcl`)の変更の記録。形式は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/) に、
バージョンは [Semantic Versioning](https://semver.org/lang/ja/) に従う(0.x の間は、マイナーバージョンが上がると互換が無い変更を含みうる)。
デザイナー(VS Code 拡張)の変更は [designer/packages/extension/CHANGELOG.md](designer/packages/extension/CHANGELOG.md) に書く。

## [Unreleased]

### 変更

- **互換が無い変更**: ヘッダを `include/bethany/` に、ソースを `src/` に移した。`#include "beth.hpp"` は `#include <bethany/beth.hpp>` に書き換える
  (システムインクルード。`find_package` で使うと、ライブラリのヘッダは `-isystem` で渡る。[ADR 0040](docs/adr/0040-system-include-path.md))。
  インストール先のヘッダも `include/beth/` から `include/bethany/` になった

## [0.1.1] - 2026-09-28

C++ の部分に変更は無い。

### 追加

- Python のパッケージ `bethany-lcl`(PyPI。import の名前は `beth`)。`beth.dll` を同梱した Windows x64 の wheel([ADR 0039](docs/adr/0039-python-distribution.md))

### 変更

- Python のバインディングを `beth` パッケージにまとめた。内部のモジュールは `beth_core`・`beth_internal` から `beth._core`・`beth._internal` になった

## [0.1.0] - 2026-09-28

最初の公開版。Windows x64(MinGW-w64 の g++)を対象とする。

### 追加

- LCL のコントロール・コンポーネントを C++Builder に似た API で使う C++ のクラス(対象の一覧は [docs/component-coverage.md](docs/component-coverage.md))
- CMake のパッケージ: `FetchContent` での取り込みと、`cmake --install` した後の `find_package(beth)` に対応し、`beth::beth` のターゲットと、
  `beth.dll` を exe の隣へ写す `beth_deploy()` を提供する
- MinGW のランタイム(libgcc・libstdc++・winpthread)を exe に静的にリンクする(`BETH_STATIC_RUNTIME`、既定で ON)

[Unreleased]: https://github.com/okano-tomoyuki/bethany/compare/v0.1.1...HEAD
[0.1.1]: https://github.com/okano-tomoyuki/bethany/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/okano-tomoyuki/bethany/releases/tag/v0.1.0
