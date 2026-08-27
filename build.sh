#!/usr/bin/env bash
set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"

FPC_HOME="C:\tool\lazarus\fpc\3.2.2"
LAZARUS_HOME="C:\tool\lazarus"

export PATH="/c/tool/lazarus/fpc/3.2.2/bin/x86_64-win64:$PATH"

fpc no_vcl.pas -Px86_64 -Twin64 -Mobjfpc -dLCL -dLCLwin32 \
  -Fu"$LAZARUS_HOME\lcl\units\x86_64-win64" \
  -Fu"$LAZARUS_HOME\lcl\units\x86_64-win64\win32" \
  -Fu"$LAZARUS_HOME\components\lazutils\lib\x86_64-win64" \
  -Fu"$LAZARUS_HOME\packager\units\x86_64-win64"

g++ -std=c++17 -Wall -Wextra \
    no_vcl_impl.cpp test/main.cpp \
    -o test/test.exe

cp no_vcl.dll test/no_vcl.dll

echo "Build OK: no_vcl.dll, test/test.exe"
