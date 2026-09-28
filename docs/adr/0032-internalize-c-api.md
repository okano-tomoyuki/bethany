# 0032. C API の提供を終了し、DLL の呼び出し層を内部層(beth::internal)にする

- 状態: 承認
- 日付: 2026-09-28

## 背景

当初は、DLL(beth.dll)の呼び出し部分を分けて確かめる流れで、C 向けの API(`beth_c.h`/`beth_c.cpp`)を
C++ ラッパー(`beth.hpp`)と並べて公開していた([vision.md](../vision.md) の「二層公開構成」)。
その後、本ライブラリは VCL の代替として位置づけられ、C API を公開し続けることのコストが目立つようになった。

- **宣言の二重管理**:
  - C API の宣言(`beth_c.h`、約 1,500 行)は、関数の一覧(X マクロ `BETH_FUNCS`)と同じ内容を手で書いていた。
  - 関数を 1 つ足すたびに、Pascal・一覧・宣言・C++ の 4 か所を触る必要があった。
- **例外の制約**([ADR 0031](0031-exceptions-across-dll.md)):
  - C API は `extern "C"` なので、例外を送出できない(送出しないと仮定するコンパイラがある)。
  - そのため、C には「直前のエラー」、C++ には呼び出し後に確かめて送出する `nv::` の層を、別々に用意していた。
- **テスト**: `test/main.c` は、実質的に「DLL の関数を直接呼ぶテスト」として役に立っていた。

## 検討した選択肢

- 選択肢A: C API を公開したまま、宣言だけを一覧から生成する。
  - 宣言の二重管理は解消する。
  - `extern "C"` の制約(直前のエラーと `nv::` の二重の層)は残る。
- 選択肢B: C API の公開を終了する。
  - DLL の呼び出し層を、C++ の内部層(`beth::internal`)にする。
  - 宣言は一覧から生成し、中継関数が直接 Exception を送出する。

別の言語(Python・Rust 等)から使う道について:
- C API は「DLL を C から呼ぶためのヘッダーと読み込み処理」にすぎず、他の言語は DLL の公開関数を直接呼べる。
- 公開を終了しても、DLL の公開関数(Pascal 側の `exports`)は変わらない。
- 失うのは、DLL の関数の決まりを示した文書としての `beth_c.h` の役割だけで、これは別の文書に移せる。

## 決定

選択肢B を採る。

- **内部層(`internal/`)**:
  - `internal/funcs.h`:
    - 関数の一覧(旧 `beth_funcs.h`)。
    - 型は `beth::internal` の `obj_t`・`str_t`・`int_t`・`uint_t`・`bool_t`・`real_t`・コールバック型(`callback_t` 等)。
  - `internal/api.h`:
    - 型・コールバック型の定義と、一覧から生成した関数の宣言(`beth::internal::TControl_GetWidth` 等)。
    - 手書きの宣言は無くなった。
  - `internal/api.cpp`(旧 `beth_c.cpp`):
    - DLL の読み込みと、一覧から生成した中継関数。
    - 中継関数は、呼び出しの最初にスレッドごとのエラーの印をクリアし、DLL が `Error_SetCallback` で知らせた例外を、戻った後に Exception として送出する。
  - `beth_` で始まる C の関数・型・定数は、すべて無くなった。
- **C++ ラッパー**:
  - beth.cpp は `internal::` の関数を呼ぶ。`nv::` の層と、直前のエラーの 4 関数(`beth_HasLastError` 等)は廃止した(ADR 0031 の該当部分を置き換える)。
  - デストラクタの中の破棄は `DestroyNoThrow` で行い、例外を外へ出さない。
  - `GuardCallback` は `internal::SetCallbackError` で知らせる。
    - 中継関数が失敗をその場で送出するようになったため、ハンドラの後に直前のエラーをクリアする処理は不要になった。
  - 公開ヘッダーの型:
    - `TObject::Handle()` 等は `beth::ObjectHandle`(= `internal::obj_t`)を使う。
    - TStrings・グラフィックの取得関数(`Accessor`)は、`internal::` の関数を指す普通の関数ポインタ(`ObjectHandle (*)(ObjectHandle)`)にした。
    - これにより、取得関数の失敗も Exception になる(以前は生の C API で、確かめていなかった)。
- **コメント**:
  - `beth_c.h` のコメント(約 290 か所)は、beth.hpp の対応するメンバと突き合わせた。
  - ほとんどは hpp に同じ内容があった。無かった LCL の挙動に関するものを hpp に移した。
    - メニュー項目の GroupIndex の範囲
    - リストビューの破棄に伴う削除で OnDeletion が呼ばれないこと
    - 画像リストの破棄で Images が nullptr に戻ること
  - DLL の関数に共通の決まりは、[dll-abi.md](../dll-abi.md) にまとめた。対象は、呼び出し規約・型・文字列の有効期間・生成と破棄・破棄通知・イベント・例外の受け渡し。
- **テスト**: `test/main.c` は `test/internal/funcs.cpp`(C++)に移し、内部層から DLL の関数を直接呼ぶテスト(`test_internal`)とした。
  - 定数は beth.hpp の同名の列挙を使う。
  - 失敗の確かめ方は、直前のエラーから `catch (Exception&)` に変えた。
- **ビルド**:
  - CMake のターゲットは `Bethany`(beth.cpp と internal/api.cpp)の 1 つにし、`beth_c` は無くした。
  - C 言語は使わなくなったため、`project` の LANGUAGES は CXX だけにした。
  - Linux の dlopen のため `${CMAKE_DL_LIBS}` をリンクする。
- **DLL(Pascal)**: 変更なし。公開関数の名前・引数・呼び出し規約はそのまま。

## 実装して分かったこと

- **変換**: 関数の一覧 919 行(公開関数 918 と SetCallbackError)とコールバック型 17 個は、スクリプトで一括変換できた。
- **ビルド**: C のテストは、`beth_` を外して C++ としてコンパイルするだけで、C 特有の書き方による修正は要らなかった。
- **Win32 のテスト**:
  - C++ のテストの出力は、変更前と完全に一致した。
  - 内部層のテストの出力は、変更前の C のテストと比べて、意図した差だけだった。
    - 失敗の確かめ方(例外の捕捉)
    - タイトル
    - 定数名の表記
- **静的ライブラリのリンク**:
  - 内部層のテストは、beth.hpp の定数と Exception(いずれもヘッダーだけで完結する)だけを使う。
  - そのため、C++ ラッパーの本体(beth.cpp のオブジェクト)はリンクされない。
  - C++ ラッパーの破棄通知の登録とぶつからず、テストが自分の破棄通知で解放数を数えられる。

## 影響

- **関数の追加**: 手順は Pascal・`internal/funcs.h`・C++ の 3 か所になった(宣言を手で書かない)。
- **C の利用者**: 公開の C API を使っていた利用者はいない前提で、互換は保っていない。
- **別の言語からの利用**: 必要になれば、[dll-abi.md](../dll-abi.md) に従って DLL の関数を直接呼ぶ層を作る。
  関数の一覧は `internal/funcs.h` から機械的に読み取れる。
- **公開の範囲**: beth.hpp が `internal/api.h` を include するため、内部層の宣言は利用者からも見える。
  名前空間 `beth::internal` によって、利用者が使うものではないことを示す。
