#!/usr/bin/env bash
# C++ の配布物(dist/bethany-<バージョン>-win64.zip)を作る(docs/adr/0038)。
# 先に build-windows.sh で beth.dll をビルドしておくこと。
#
# zip の中身は、C++ のソース(利用者のコンパイラでビルドする)・ビルド済みの beth.dll・beth.pas(beth.dll の元)・
# CMake の設定・ライセンス。FetchContent で取り込むか、cmake --install でインストールして find_package で使う。
set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"

VERSION="$(sed -n 's/^project(beth VERSION \([0-9.]*\) .*/\1/p' CMakeLists.txt)"
if [ -z "$VERSION" ]; then
  echo "CMakeLists.txt からバージョンを読めません" >&2
  exit 1
fi
if [ ! -f beth.dll ] || [ beth.pas -nt beth.dll ]; then
  echo "beth.dll が無いか、beth.pas より古いです。先に build-windows.sh を実行してください" >&2
  exit 1
fi
if ! grep -q "^## \[$VERSION\]" CHANGELOG.md; then
  echo "CHANGELOG.md に $VERSION の節がありません" >&2
  exit 1
fi

NAME="bethany-$VERSION-win64"
STAGE="dist/$NAME"
rm -rf "$STAGE" "dist/$NAME.zip"
mkdir -p "$STAGE"

cp -r beth.hpp beth.cpp internal beth.pas beth.dll CMakeLists.txt cmake win32 \
  README.md CHANGELOG.md LICENSE THIRD-PARTY-NOTICES.md licenses "$STAGE/"

(cd dist && cmake -E tar cf "$NAME.zip" --format=zip "$NAME")
rm -rf "$STAGE"

echo "Package OK: dist/$NAME.zip"
