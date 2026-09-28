# 0038. C++ はソースとビルド済みの beth.dll を zip で配り、C++ の部分は利用者のコンパイラでビルドする

- 状態: 承認
- 日付: 2026-09-28

## 背景

C++ のライブラリを公開する形を決める。最初の配布の対象は Windows x64 に限る([vision.md](../vision.md) のスコープのとおり)。
Linux(GTK2)のビルド(build-linux.sh)は残すが、配布物は作らない。

Bethany の C++ の部分は 2 つの層に分かれる。

- **`beth.dll`**: FPC で `beth.pas` からビルドし、LCL を静的に含む。C の ABI(Windows では stdcall)で関数を公開するため、
  どの C++ コンパイラからも呼べる。ビルドには FPC と Lazarus が要り、利用者に用意させるのは重い。
- **`beth.hpp`・`beth.cpp`・`internal/`**: C++ のクラスと、`beth.dll` を実行時に読み込む層([ADR 0032](0032-internalize-c-api.md))。
  `std::function`・`std::string` などを公開の API に含むため、C++ の ABI に依存する。

これまでの課題は [todo.md](../../todo.md) の「配布方針」にある 2 点だった。MinGW の g++ でリンクした exe が
`libgcc_s_seh-1.dll`・`libstdc++-6.dll`・`libwinpthread-1.dll` に依存すること、`beth.dll` を exe の隣に置く手段が無いこと。

## 検討した選択肢

- 選択肢A: C++ の部分もビルド済みの静的ライブラリ(`libbeth.a`)で配る。利用者のビルドは速いが、g++ のバージョンと
  ランタイム(MSYS2 の mingw64 は msvcrt、ucrt64 は UCRT)が配布元と一致しないと、リンクできないか実行時に壊れる。
  組み合わせごとにビルドして配るのは現実的でない。
- 選択肢B: すべてソースで配り、`beth.dll` も利用者が FPC でビルドする。ABI の問題は無いが、利用者に Lazarus の導入を求める。
- 選択肢C: C++ の部分はソースで、`beth.dll` はビルド済みで配る。C の ABI の境界がそのまま配布の境界になる。
  C++ の部分は 2 ファイルと内部層だけで、利用者のビルドにかかる時間は小さい。

## 決定

**選択肢C を選ぶ。** `package-windows.sh` が `dist/bethany-<バージョン>-win64.zip` を作る。中身は C++ のソース、ビルド済みの `beth.dll`、
`beth.pas`(`beth.dll` の元。LGPL の再リンクの要件のため)、CMake の設定、ライセンス。

- **2 つの使い方に対応する**。`FetchContent` で zip を取り込む方法と、zip をビルドして `cmake --install` し、`find_package(beth)` で使う方法。
  どちらも `beth::beth` のターゲットを提供する。テストとインストールの規則は、zip やリポジトリを直接ビルドしたときだけ作る。
- **`beth_deploy(<target>)` で `beth.dll` を exe の隣に写す**。`beth.dll` は実行時に読み込むので、リンクしただけでは exe の隣に置かれない。
  写す処理は、exe のリンクの後ではなくビルドのたびに(内容が変わったときだけ)行う。`beth.dll` だけを作り直したときに古いものが残らないようにするため。
- **MinGW のランタイムを exe に静的にリンクする**(`BETH_STATIC_RUNTIME`、既定で ON。`beth::beth` のリンクのオプションに `-static` を加える)。
  これで、アプリケーションを配るときに要るのは exe と `beth.dll` だけになる。他のライブラリの都合で静的リンクできない場合は OFF にする。
- **manifest**(Common-Controls 6.0)は、これまでどおり `beth::beth` をリンクした exe に埋め込む。インストールした先では、
  .rc に manifest の絶対パスを埋めるため、`bethConfig.cmake` が利用者のビルドフォルダに .rc を作り直す。
- **ライセンス**: `beth.dll` には FPC の RTL と LCL・LazUtils が含まれる(いずれも静的リンクの例外付きの LGPL)。
  zip とインストール先には、[THIRD-PARTY-NOTICES.md](../../THIRD-PARTY-NOTICES.md) と `licenses/`(LGPL と例外の本文)を入れる。
  静的リンクの例外は「実行ファイル」を対象にしているため、DLL である `beth.dll` については LGPL そのものの要件(ソースの入手先の明示と、
  利用者が LCL を差し替えて作り直せること)も満たすようにする。`beth.pas` を同梱し、`beth.dll` を作り直せば exe はリンクし直さずに使える。
- **バージョン**は CMakeLists.txt の `project(beth VERSION ...)` に置き、[CHANGELOG.md](../../CHANGELOG.md) の節と合わせる。
  0.x の間は、マイナーバージョンが変わると互換が無いものとする(`find_package` の互換性は `SameMinorVersion`)。

## 影響

- 利用者には MinGW-w64 の g++ と CMake が要る。MSVC でのビルドは確かめていない。
- `-static` は exe 全体の静的リンクを意味するため、利用者が `-l` で指定した他のライブラリにも効く(CMake がフルパスで渡すライブラリには効かない)。
- zip の作成は手作業(build-windows.sh → package-windows.sh → GitHub の Release に添付)。FPC・Lazarus を CI に用意して自動化するのは今後の課題。
- `beth.dll` のビルドを CMake に統合する(FPC を CMake から呼ぶ)のも今後の課題として残す。
