# 0039. Python のバインディングを beth パッケージにまとめ、beth.dll を同梱した Windows x64 の wheel を PyPI に出す

- 状態: 承認
- 日付: 2026-09-28

## 背景

Python のバインディングを PyPI に出す。配布名は `bethany-lcl`、import の名前は `beth` と決めてある([ADR 0037](0037-rename-to-bethany.md))。
対象は C++ と同じく Windows x64 に限る([ADR 0038](0038-cpp-distribution.md))。

これまでのバインディングは `py/` に平置きの 3 つのモジュール(`beth.py`・`beth_core.py`・`beth_internal.py`)で、互いを
別々のトップレベルのモジュールとして import し、`beth_internal.py` と同じフォルダの `beth.dll` を読み込んでいた。
このまま配ると、site-packages の直下に `beth_core`・`beth_internal`・`beth.dll` がばらばらに入り、他のパッケージとぶつかりうる。

## 検討した選択肢

- 選択肢A: 平置きのまま、3 つのモジュールと `beth.dll` を site-packages の直下に入れる。変更は要らないが、上の衝突の恐れが残る。
- 選択肢B: `beth` をパッケージ(フォルダ)にし、内部のモジュールと `beth.dll` をその中に入れる。利用者の書き方(`from beth import *`)は変わらない。
- wheel の種類: バインディングは ctypes で `beth.dll` を呼ぶだけで、Python の拡張モジュールを持たない。
  Python の版ごとに wheel を作る必要は無く、`py3-none-win_amd64` の 1 つで足りる。

## 決定

**選択肢B を選ぶ。** `py/beth/` に `__init__.py`(これまでの beth.py。gen_api.py が生成する)・`_core.py`・`_internal.py`(gen.py が生成する)を置き、
`beth.dll` はそのフォルダから読み込む。build-windows.sh が `beth.dll` を `py/beth/` へ写す。

- **wheel**: `package-python.sh` が `dist/bethany_lcl-<バージョン>-py3-none-win_amd64.whl` を作る。材料(`py/pyproject.toml`・`py/beth/`・`beth.dll`・ライセンス)を
  `dist/python/` に集めてから hatchling でビルドする。ライセンスのファイルはリポジトリ直下にあり、`py/` の外にあるため。
  タグは `hatch_build.py` で `py3-none-win_amd64` にする。ビルドの道具は `dist/.venv-python` に入れ、利用者の Python の環境を変えない。
- **ライセンス**: C++ の zip と同じく、`LICENSE`・`THIRD-PARTY-NOTICES.md`・`licenses/` を wheel のメタデータ(`.dist-info/licenses/`)に入れる。
  ライセンスの式は `MIT AND LicenseRef-LGPL-2.0-with-static-linking-exception` とする。FPC と LCL の「静的リンクの例外付きの LGPL」に
  当たる SPDX の識別子が無いため、`LicenseRef-` で表す。
- **バージョン**は CMakeLists.txt と同じにする(`package-python.sh` が食い違いを検査する)。
- **対応する Python**は 3.8 以降とする。コードに 3.8 より新しい文法は使っていないが、確かめたのは 3.13 と 3.14 だけである。
- 公開は、TestPyPI で `pip install` を確かめてから PyPI に出す。アップロードは twine と API トークンで行う。

## 影響

- 内部のモジュールの名前が `beth_core`・`beth_internal` から `beth._core`・`beth._internal` に変わる。利用者は `from beth import *` だけを使う前提なので
  影響は無い。デザイナーの道具(verify-python・layout の記録)は、`py/beth` を丸ごと写すように直した。
- リポジトリの `py/` で実行するテストとデザイナーの道具は、これまでどおりインストールせずに動く(`py/beth` がカレントディレクトリから import される)。
- Linux の wheel(manylinux)は作らない。作るなら `libbeth.so` が GTK2 に依存する点を扱う必要がある。
