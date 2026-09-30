# 0055. MSYS2 の Python の pip で入るよう、beth.dll を含む sdist も PyPI に出す

- 状態: 承認
- 日付: 2026-09-30

## 背景

Python のパッケージ `bethany-lcl` は、`py3-none-win_amd64` の wheel だけを PyPI に出していた([ADR 0039](0039-python-distribution.md))。
これを MSYS2 の Python(MINGW64・UCRT64 等)の pip からも入れられるようにしたい。

調べて分かったこと:

- **MSYS2 の pip は `win_amd64` の wheel を受け付けない**。MSYS2 の Python のプラットフォーム(`sysconfig.get_platform()`)は
  `mingw_x86_64_msvcrt_gnu`(MINGW64)・`mingw_x86_64_ucrt_gnu`(UCRT64)等で、`win_amd64` の wheel は「このプラットフォームに合わない」とされる。
- **PyPI は `mingw_*` のタグの wheel を受け付けない**。PyPI(warehouse)がアップロードで許すプラットフォームのタグは決まっていて
  (`win_amd64`・`manylinux*`・`macosx*` 等)、MSYS2 のタグは含まれない。
- **`beth.dll` は MSYS2 の Python でもそのまま動く**。
  - `beth.dll` は Free Pascal でビルドしていて、C のランタイム(msvcrt・ucrt)を読み込まない。読み込むのは Windows の DLL(kernel32・user32 等)だけである。
  - バインディングは ctypes で読み込むだけで、Python の拡張モジュールを持たない。
  - MINGW64 の Python 3.14 で、フォームを作り、日本語の文字列をやり取りできることを確かめた。

## 検討した選択肢

- 選択肢A: MSYS2 のタグの wheel を作って PyPI に出す。PyPI が受け付けないため、できない。
- 選択肢B: `py3-none-any` の wheel にする。MSYS2 でも入るが、Linux・macOS でも入ってしまい、import で初めて失敗する。
- 選択肢C: MSYS2 の公式のパッケージ(pacman)にする。MSYS2 には Free Pascal が無く、MSYS2 の方針(ソースからビルドする)で `beth.dll` を作れない。
- 選択肢D: `win_amd64` の wheel に加えて、ビルド済みの `beth.dll` を含む sdist を PyPI に出す。
  - MSYS2 の pip は、合う wheel が無いので sdist を取り、その場で wheel を作って入れる。
  - wheel のタグはビルドする Python から決める。

## 決定

**選択肢D を選ぶ。**

- `py/pyproject.toml` に sdist の設定を加え、`beth.dll`(ビルド済み)・`hatch_build.py`・ライセンスを含める。
- `hatch_build.py` は、wheel のタグを `py3-none-<ビルドする Python のプラットフォーム>` にする。
  - python.org の Python では、これまでどおり `py3-none-win_amd64` になる。配布の wheel は python.org の Python で作る(`package-python.sh`)。
  - MSYS2 の Python では `py3-none-mingw_x86_64_ucrt_gnu` 等になる。
  - Windows x64 以外(Linux・32 ビットの Python・ARM64 等)では、`beth.dll` が動かないため、分かりやすいメッセージでビルドを止める。
- `package-python.sh` は sdist を作り、wheel はその sdist から作る。MSYS2 の pip と同じ道筋になり、sdist に要るものが揃っているかも確かめられる。
- `check-python-install.sh <python>` で、作った配布物を venv に入れて動くかを確かめる。
  - MSYS2 の Python には sdist を入れ、それ以外には wheel を入れる。
  - 入れたものを読み込み、フォームとボタンを作って日本語の文字列をやり取りする。
- MSYS2 では venv に入れる前提とする(README に書いた)。MSYS2 の Python そのものに pip で入れると、pacman が管理するパッケージとぶつかりうる。

## 影響

- MSYS2 の pip では、入れるときに sdist から wheel を作る。そのため、ビルドの道具(hatchling)を PyPI から取得する。
- sdist にバイナリ(`beth.dll`)が入る。一般的な sdist とは違うが、MSYS2 には Free Pascal が無く、利用者の手元ではビルドできないため、そうする。
- Linux・macOS で `pip install bethany-lcl` すると、これまでは「合う配布物が無い」で止まっていたが、sdist のビルドで止まり、Windows x64 専用だというメッセージが出る。
