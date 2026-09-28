#!/usr/bin/env bash
set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"

# Lazarus・FPC の場所は環境変数で変えられる(Windows の形式のパス)。
LAZARUS_HOME="${LAZARUS_HOME:-C:\tool\lazarus}"
FPC_HOME="${FPC_HOME:-$LAZARUS_HOME\fpc\3.2.2}"

# FPCのbinディレクトリには古いgcc(2.95)が同梱されており、PATHの先頭に置くと
# MinGW64本来のgccを覆い隠してしまう。fpc呼び出しにだけ限定してPATHへ追加する。
PATH="$(cygpath -u "$FPC_HOME")/bin/x86_64-win64:$PATH" \
  fpc beth.pas -Px86_64 -Twin64 -Mobjfpc -dLCL -dLCLwin32 \
  -Fu"$LAZARUS_HOME\lcl\units\x86_64-win64" \
  -Fu"$LAZARUS_HOME\lcl\units\x86_64-win64\win32" \
  -Fu"$LAZARUS_HOME\components\lazutils\lib\x86_64-win64" \
  -Fu"$LAZARUS_HOME\packager\units\x86_64-win64"

# C++側(beth, test)はCMakeでビルドする。
# MinGW Makefiles生成器はPATH上にsh.exeがあると使えないため、Ninjaを使う。
cmake -S . -B build -G Ninja
cmake --build build

# テストの exe の隣には、CMake の beth_deploy が beth.dll を写す。
# Python のパッケージ(py/beth)は、自身と同じフォルダの beth.dll を読み込む。
cp beth.dll py/beth/beth.dll

echo "Build OK: beth.dll, build/test/test_internal.exe, build/test/test_cpp.exe"
