#!/usr/bin/env bash
# Python の配布物(dist/bethany_lcl-<バージョン>-py3-none-win_amd64.whl と、MSYS2 の pip のための sdist
# dist/bethany_lcl-<バージョン>.tar.gz)を作る(docs/adr/0039・docs/adr/0055)。
# 先に build-windows.sh で beth.dll をビルドしておくこと。
#
# ビルドには python.org の Python を使う(既定は py ランチャー。環境変数 PYTHON で変えられる)。
# ビルドの道具(build・twine)は dist/.venv-python に入れ、利用者の Python の環境は変えない。
set -eu

cd "$(dirname "${BASH_SOURCE[0]}")"

PYTHON="${PYTHON:-py}"

VERSION="$(sed -n 's/^project(beth VERSION \([0-9.]*\) .*/\1/p' CMakeLists.txt)"
PY_VERSION="$(sed -n 's/^version = "\(.*\)"/\1/p' py/pyproject.toml)"
if [ -z "$VERSION" ] || [ "$VERSION" != "$PY_VERSION" ]; then
  echo "CMakeLists.txt($VERSION)と py/pyproject.toml($PY_VERSION)のバージョンが違います" >&2
  exit 1
fi
if [ ! -f beth.dll ] || [ beth.pas -nt beth.dll ]; then
  echo "beth.dll が無いか、beth.pas より古いです。先に build-windows.sh を実行してください" >&2
  exit 1
fi

# wheel の材料を集める(ライセンスのファイルはリポジトリ直下にあり、py/ の外なので)。
STAGE=dist/python
rm -rf "$STAGE"
mkdir -p "$STAGE/beth"
cp py/pyproject.toml py/hatch_build.py py/README.md LICENSE THIRD-PARTY-NOTICES.md "$STAGE/"
cp -r licenses "$STAGE/"
cp py/beth/__init__.py py/beth/_core.py py/beth/_internal.py "$STAGE/beth/"
cp beth.dll "$STAGE/beth/beth.dll"

VENV=dist/.venv-python
if [ ! -f "$VENV/Scripts/python.exe" ]; then
  "$PYTHON" -m venv "$VENV"
  "$VENV/Scripts/python.exe" -m pip install --quiet --upgrade pip build twine
fi

WHEEL="dist/bethany_lcl-$VERSION-py3-none-win_amd64.whl"
SDIST="dist/bethany_lcl-$VERSION.tar.gz"
rm -f "$WHEEL" "$SDIST"
# sdist を作り、wheel はその sdist から作る(MSYS2 の pip と同じ道筋で、sdist に要るものが揃っているかを確かめる)。
"$VENV/Scripts/python.exe" -m build --outdir dist "$STAGE"
"$VENV/Scripts/python.exe" -m twine check --strict "$WHEEL" "$SDIST"
rm -rf "$STAGE"

echo "Package OK: $WHEEL $SDIST"
# twine の進捗表示は、日本語の Windows のコンソール(cp932)で表示できない文字を出して止まるため、切って UTF-8 で出力する。
echo "アップロード(~/.pypirc のトークンを使う。先に testpypi で確かめる):"
echo "  PYTHONIOENCODING=utf-8 $VENV/Scripts/python.exe -m twine upload --disable-progress-bar -r testpypi $WHEEL $SDIST"
