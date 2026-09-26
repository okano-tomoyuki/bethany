#!/usr/bin/env bash
set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"

FPC_HOME="C:\tool\lazarus\fpc\3.2.2"
LAZARUS_HOME="C:\tool\lazarus"

# FPCのbinディレクトリには古いgcc(2.95)が同梱されており、PATHの先頭に置くと
# MinGW64本来のgccを覆い隠してしまう。fpc呼び出しにだけ限定してPATHへ追加する。
PATH="/c/tool/lazarus/fpc/3.2.2/bin/x86_64-win64:$PATH" \
  fpc no_vcl.pas -Px86_64 -Twin64 -Mobjfpc -dLCL -dLCLwin32 \
  -Fu"$LAZARUS_HOME\lcl\units\x86_64-win64" \
  -Fu"$LAZARUS_HOME\lcl\units\x86_64-win64\win32" \
  -Fu"$LAZARUS_HOME\components\lazutils\lib\x86_64-win64" \
  -Fu"$LAZARUS_HOME\packager\units\x86_64-win64"

# C/C++側(no_vcl_c, no_vcl, test)はCMakeでビルドする。
# MinGW Makefiles生成器はPATH上にsh.exeがあると使えないため、Ninjaを使う。
cmake -S . -B build -G Ninja
cmake --build build

# 実行時にDLL探索されるよう、テストexeの隣にno_vcl.dllを置く。
cp no_vcl.dll build/test/no_vcl.dll

echo "Build OK: no_vcl.dll, build/test/test_c.exe, build/test/test_cpp.exe"
