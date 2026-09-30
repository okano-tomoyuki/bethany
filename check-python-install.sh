#!/usr/bin/env bash
# package-python.sh で作った Python の配布物を、指定した Python の venv に入れて動くかを確かめる(docs/adr/0055)。
#   ./check-python-install.sh                               python.org の Python(py ランチャー)に wheel を入れる
#   ./check-python-install.sh /c/msys64/ucrt64/bin/python.exe   MSYS2 の Python に sdist から入れる
# MSYS2 の Python(sysconfig のプラットフォームが mingw_*)には sdist を、それ以外には wheel を入れる。
# venv は dist/.venv-check に作り直す(利用者の Python の環境は変えない)。
set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"

PYTHON="${1:-${PYTHON:-py}}"

VERSION="$(sed -n 's/^project(beth VERSION \([0-9.]*\) .*/\1/p' CMakeLists.txt)"
PLATFORM="$("$PYTHON" -c 'import sysconfig; print(sysconfig.get_platform())')"
case "$PLATFORM" in
  mingw*) PACKAGE="dist/bethany_lcl-$VERSION.tar.gz" ;;
  *) PACKAGE="dist/bethany_lcl-$VERSION-py3-none-win_amd64.whl" ;;
esac
if [ ! -f "$PACKAGE" ]; then
  echo "$PACKAGE がありません。先に package-python.sh を実行してください" >&2
  exit 1
fi

VENV=dist/.venv-check
rm -rf "$VENV"
"$PYTHON" -m venv "$VENV"
# venv の python は、python.org の Python では Scripts/、MSYS2 の Python では bin/ にできる。
if [ -f "$VENV/Scripts/python.exe" ]; then VPY="$VENV/Scripts/python.exe"; else VPY="$VENV/bin/python.exe"; fi

echo "$PLATFORM の Python に $PACKAGE を入れます"
"$VPY" -m pip install --quiet --disable-pip-version-check --no-cache-dir "$PACKAGE"

# リポジトリの py/beth ではなく、入れたものを読み込むよう、別のフォルダで実行する。
(cd "$VENV" && PYTHONIOENCODING=utf-8 ./"${VPY#"$VENV"/}" - <<'EOF'
import sys, beth
from beth import Application, TForm, TButton

assert "site-packages" in beth.__file__, beth.__file__
Application.Initialize()
form = TForm(Application)
button = TButton(form)
button.Parent = form
button.Caption = "日本語"
assert button.Caption == "日本語", button.Caption
print(f"OK: {beth.__file__} (Python {sys.version.split()[0]})")
EOF
)
rm -rf "$VENV"
