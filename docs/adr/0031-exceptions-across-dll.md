# 0031. DLL の境界で例外を受け渡し、C は直前のエラー、C++ は Exception として扱う

- 状態: 承認(一部置換→0032。C API の直前のエラーは内部層の Exception の送出に置き換えた)
- 日付: 2026-09-27

## 背景

これまで、LCL が送出した例外(範囲外の添字、ソートされた TStringList への Insert、読み込めない画像ファイル等)は、
公開関数からそのまま抜けていた。これは [ADR 0028](0028-labelededit-and-stringlist.md)・[0029](0029-graphics-picture-image-glyph.md) で「使い方の制約」として残していた。

- FPC の例外は、C/C++ の関数をまたいで伝わらない(未定義動作)。
  - Win64 では、[ADR 0030](0030-imagelist-and-images.md) のテストで、プロセスが終了コード 0xE0465043(FPC の例外)で終了した。
  - そのとき、標準出力のバッファに残っていた出力も失われた。
- 逆向きも同じで、C++ のイベントハンドラが throw した例外は、ハンドラを呼んだ Pascal の関数をまたいで伝わらない。
- VCL のコードは `try { ... } catch (Exception& E) { ShowMessage(E.Message); }` の形で例外を使うため、移植にはこれらの受け渡しが要る。

## 検討した選択肢

どちらの向きでも、例外は境界の手前で捕まえるしかない。そのうえで、捕まえた例外を C API でどう渡すかを検討した。

- 選択肢A: すべての関数の末尾に、エラーを受け取る引数(`no_vcl_error_t*`。NULL 可)を足す。
  - 利点: 失敗が明示的に分かる。
  - 欠点: 約 930 個の関数のシグネチャがすべて変わる。
    既存の C の呼び出しと no_vcl.cpp の約 870 か所の呼び出しを書き換えることになり、エラーを気にしない呼び出しにも NULL を足すことになる。
- 選択肢B: Win32 の GetLastError と同じく、スレッドごとに「直前のエラー」を持つ。
  - シグネチャは変えず、各関数の呼び出しの最初にクリアし、失敗したら内容を入れる。
  - C の利用者は、必要な呼び出しの直後に確かめる。
  - C++ ラッパーは、呼び出しの直後に確かめて例外を送出する。

C++ で例外を送出する場所:

- 選択肢A: C API の中継関数(no_vcl_c.cpp)自体が throw する。
  - C API は `extern "C"` で、C++ の例外を送出しないと仮定するコンパイラ(MSVC の /EHsc 等)がある。
  - C API の利用者にとっても予想外になる。
- 選択肢B: C API は例外を送出しない。C++ ラッパー(no_vcl.cpp)が、C API を呼んだ直後に確かめて送出する。

## 決定

選択肢B(直前のエラー)と、送出する場所の選択肢B(C++ ラッパー)を採る。C++ → DLL の向きも同時に扱う。

- **Pascal(DLL)**:
  - 公開関数 918 個の本体を `try ... except` で包んだ。
    - except では、戻り値を既定の値(0・nil・False・空文字列)にし、`ReportException` を呼ぶ。
    - `ReportException` は、例外のクラス名とメッセージを、呼び出し側が登録したコールバック(`Error_SetCallback`)で知らせる。
    - どの関数も「0 桁目の begin と end;、入れ子の関数なし」の同じ形だったため、スクリプトで一括変換した。
  - 逆向き:
    - 呼び出し側がコールバックの中で `SetCallbackError(ClassName, Message)` を呼ぶと、スレッドごとの保留に入る。
    - コールバックを呼ぶブリッジ(21 か所)は、コールバックから戻った後に `CheckCallbackError` で確かめ、`ENoVclCallbackError` として送出し直す。
    - 呼び出し側へ知らせるときは、元のクラス名を使う。
    - その先の扱い:
      - メッセージループの中なら、LCL の `Application.HandleException` が処理する(VCL と同じ)。
      - 公開関数の中で起きたイベント(`TMenuItem_Click` 等)なら、その関数の except で呼び出し側へ知らせる。
  - `SysUtils` を uses に加えた(Exception のため)。
- **C API(no_vcl_c.cpp)**:
  - 関数の一覧(X マクロ `NO_VCL_FUNCS`)を、内部ヘッダー `no_vcl_funcs.h` に移した(no_vcl.cpp と共有するため)。
  - 中継関数は、呼び出しの最初にスレッドごとの直前のエラーをクリアする。DLL の読み込み時に `Error_SetCallback` へ通知先を登録する。
  - 追加した関数:
    - `no_vcl_HasLastError`・`no_vcl_GetLastErrorClassName`・`no_vcl_GetLastErrorMessage`・`no_vcl_ClearLastError`(この 4 つは直前のエラーをクリアしない)。
    - `no_vcl_SetCallbackError`。
  - シグネチャは変えていない。
- **C++**:
  - `no_vcl::Exception`(std::exception の派生)を追加した。
    - VCL と同じく `Message`(std::string)と `ClassName()` を持ち、`Exception("msg")` で送出することもできる。
  - no_vcl.cpp は、C API を `nv::` 経由で呼ぶ。`nv::` は同じ関数一覧から生成した、呼び出しの直後に直前のエラーを確かめて送出する版。
    - 生の C API のまま残したのは、次の 2 つだけ。
      - デストラクタの中の呼び出し(送出すると std::terminate になる)。
      - TStrings 等に取得関数として渡す関数ポインタ。
  - DLL から呼ばれるトランポリン(64 個)の本体は `GuardCallback` で包んだ。
    - ハンドラから送出された例外を捕まえ、`no_vcl_SetCallbackError` で知らせる。渡すクラス名は次のとおり。
      - Exception なら、そのクラス名。
      - std::exception の派生なら、`"std::exception"`。
      - それ以外なら、空文字列。
    - 正常に戻ったときは、ハンドラの中の呼び出しが残した直前のエラーをクリアする(外側の呼び出しの失敗と取り違えないため)。
- ヘッダーの「例外は呼び出し側で捕捉できない」という記述は、Exception・直前のエラーの記述に改めた。

## 実装して分かったこと

- **C++ のテスト**:
  - `Strings[5]`(1 行の一覧): EStringListError(`List index (5) out of bounds`)が送出され、一覧はその後も使えた。
  - 無いファイルの `TPicture::LoadFromFile`: EFOpenError が送出された。
- **逆向き**(MenuItem の OnClick で送出して、`Click()` から受けた):
  - `Exception("boom")` は、クラス名 Exception・メッセージ boom で受けられた。
  - `Exception("EMyError", "custom")` はクラス名 EMyError、`std::runtime_error` はクラス名 std::exception で受けられた。
  - ハンドラの中で LCL が送出した例外(TStringList の範囲外の Delete)は、ハンドラ内で Exception になり、そのまま `Click()` から EStringListError として受けられた。
  - その後、例外を送出しないハンドラに戻すと、`Click()` は成功した(保留が残らない)。
- **C のテスト**:
  - 失敗した `no_vcl_TStrings_GetStrings` は空文字列を返し、直前のエラーに EStringListError が入った。次の成功した呼び出しでクリアされた。
  - C のコールバックで `no_vcl_SetCallbackError` を呼ぶと、`no_vcl_TMenuItem_Click` が失敗し、直前のエラーに EMyError が入った。
  - 最初のテストは、`printf` の引数に「成功する呼び出し」と `no_vcl_HasLastError()` を並べていた。
    引数の評価順が決まっていないため HasLastError が先に評価され、誤って 1 になった。
    直前のエラーは「関数の直後に」確かめる必要がある(ヘッダーに明記した)。
- **既存テスト**: 出力は変更前と完全に一致した(Win32)。
- **呼び出しのコスト**(Win32、C API で 400 万回の呼び出し):
  - 変更前は 1 回あたり 5.5〜6.8 ns、変更後は 7.2 ns だった(try/except と直前のエラーのクリアによる増加は約 1 ns)。
- **警告**: `-Wall -Wextra` の警告は、変更前からある TCustomCheckGroup の 1 件だけのままだった。
- **Linux/GTK2(WSL)**: 上記のテストの出力は、すべて Win32 と同じだった。
- **確かめていないこと**: メッセージループの中のイベントで送出した場合に LCL がメッセージを表示することは、
  モーダルなダイアログで自動テストが止まるため確かめていない(LCL の `Application.HandleException` に任せる設計)。

## 影響

- VCL の `try ... catch (Exception& E)` を使うコードを、そのまま移植できる。
- ADR 0028〜0030 で「使い方の制約」としていた例外(範囲外の添字・Sorted の一覧への Insert・読み込めないファイル・画像リストの範囲外の Move 等)は、すべて Exception として受けられる。
- C の利用者は、失敗しうる呼び出しの直後に `no_vcl_HasLastError` で確かめる。シグネチャは変わっていないため、既存の C のコードはそのまま動く。
- 新しい公開関数を追加するときは、次の 2 つを守る。
  - 本体を `try ... except` で包み、except で `ReportException` を呼ぶ(戻り値があれば既定の値を入れる)。
  - C++ 側では `nv::` 経由で呼ぶ。
  - 新しいブリッジは、コールバックの後に `CheckCallbackError` を呼ぶ。新しいトランポリンは `GuardCallback` で包む。
- 未対応:
  - C++ 側で、LCL の例外クラスごとの派生クラス(EStringListError 等)を用意すること。いまは ClassName() で見分ける。
  - VCL の `Application->OnException`。
