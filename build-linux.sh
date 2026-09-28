#!/usr/bin/env bash
set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"

LAZARUS_HOME="/usr/share/lazarus/4.8.0"

fpc beth.pas -Px86_64 -Tlinux -Mobjfpc -dLCL -dLCLgtk2 \
  -Fu"$LAZARUS_HOME/lcl/units/x86_64-linux" \
  -Fu"$LAZARUS_HOME/lcl/units/x86_64-linux/gtk2" \
  -Fu"$LAZARUS_HOME/components/lazutils/lib/x86_64-linux" \
  -Fu"$LAZARUS_HOME/packager/units/x86_64-linux"

# C++側(beth, test)はCMakeでビルドする。
cmake -S . -B build-linux -G Ninja
cmake --build build-linux

# 実行時に共有ライブラリが見つかるよう、テスト実行ファイルの隣にlibbeth.soを置く。
cp libbeth.so build-linux/test/libbeth.so

echo "Build OK: libbeth.so, build-linux/test/test_internal, build-linux/test/test_cpp"
echo "実行時は LD_LIBRARY_PATH=. を指定するか、テスト実行ファイルと同じディレクトリから起動すること。"
