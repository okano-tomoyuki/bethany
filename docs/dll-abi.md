# DLL の関数の決まり

no_vcl.dll(Linux は libno_vcl.so)が公開する関数の決まり。
C++ ラッパー(no_vcl.hpp)の内部層(`internal/`)が従っているもので、別の言語(Python の ctypes・Rust 等)から
DLL を直接呼ぶ層を作るときにも、これに従う([ADR 0032](adr/0032-internalize-c-api.md))。

関数の一覧と引数の型は [internal/funcs.h](../internal/funcs.h)、実装は [no_vcl.pas](../no_vcl.pas)(`exports`)が正とする。
関数ごとの意味は、no_vcl.hpp の対応するメンバのコメントと、[class-hierarchy.md](class-hierarchy.md) の対応表を参照する。

## 呼び出し規約・名前

- **呼び出し規約**:
  - Windows(win64)は stdcall。x86_64 では cdecl と同じ規約になる。
  - Linux(x86_64)は既定の規約。
- **名前**:
  - `クラス_メンバ` の形(`TControl_GetWidth`・`TCustomListBox_GetItems` 等)。
  - クラスは、LCL でそのメンバが公開(public/published)されるクラス([class-hierarchy.md](class-hierarchy.md) の 3 章)。
  - 派生クラスのオブジェクトは、基底クラスの関数に渡してよい。
  - プロパティは `Get`/`Set` の組、イベントは `SetOnXxx`。

## 型

| internal/api.h | Pascal | 内容 |
|---|---|---|
| `obj_t` | `Pointer` | LCL のオブジェクト(コンポーネント・項目・TStrings・Canvas 等)を指すハンドル |
| `str_t` | `PChar` | UTF-8 の文字列(下の「文字列」を参照) |
| `int_t` | `Integer` | 32 ビットの整数。列挙は LCL の序数(no_vcl.hpp の同名の列挙と同じ値)、色は TColor(`$00BBGGRR`) |
| `uint_t` | `LongWord` | ビット集合(グリッド・ダイアログの Options、TFont の Style。グリッドの Options は 32 ビットすべてを使う) |
| `bool_t` | `LongBool` | 4 バイトの真偽値。0 は偽、0 以外は真。DLL が返す真は -1 |
| `real_t` | `Double` | 倍精度の実数 |

集合型(TShiftState 等)は、各要素をビットにした整数で受け渡す(ビットの値は no_vcl.hpp の `ssShift` 等と同じ)。

## 文字列

- 渡す文字列は UTF-8 で、DLL は呼び出しの間だけ参照する(保持しない)。
- 返す文字列は、DLL 内のスレッドごとのバッファを指す。
  - 同じスレッドで次に文字列を返す関数を呼ぶまで有効で、解放は不要。
  - 保持する場合や、2 つの戻り値を同時に使う場合は、呼び出し側でコピーする([ADR 0013](adr/0013-string-return-bridge-reuse-ctor-exception.md))。

## 生成・破棄とハンドルの寿命

- **生成**:
  - `Xxx_Create(Owner)` の Owner は LCL の Owner(破棄の責任を持つコンポーネント)で、nil でもよい。
  - 画面上の親は `TControl_SetParent` で別に設定する。
- **破棄**:
  - コンポーネントは `TComponent_Destroy` で破棄する(Owner に任せてもよい)。
  - `TStringList`・グラフィック・`TPicture` は TComponent ではない。`TStringList_Destroy`・`TGraphic_Destroy`・`TPicture_Destroy` で破棄する。
- **破棄通知**(`FreeNotify_SetCallback(cb, data)`):
  - `*_Create` で生成したコンポーネントと、LCL が内部で生成して DLL が返したコンポーネント(メニューのルートの項目・AddTabSheet のページ等)が対象。
  - 破棄の経路によらず、`cb(破棄されるオブジェクト, data)` が呼ばれる。
  - 登録できるのは 1 つだけで、C++ ラッパーを使う場合はラッパーが登録する。
- **項目の破棄通知**(`ItemFree_SetCallback(cb, data)`):
  - 対象は、TComponent ではない項目(ツリーのノード・リストの項目と列・ヘッダーのセクション・クールバーのバンド)のうち、一度でも DLL が返した項目。
  - 項目の破棄の最後(LCL の削除の処理とイベントの後)に呼ばれる([ADR 0026](adr/0026-coolbar-and-item-free-observer.md))。
- **所有者の持ち物のハンドル**:
  - 次のハンドルは所有者の持ち物で、LCL が差し替えることがある。保存せず、使うたびに取得する。
    - コントロールの `TStrings`(Items・Lines 等)
    - `TPicture` の中身のグラフィック
    - グラフィック・TImage の Canvas
  - 詳細は [ADR 0027](adr/0027-tstrings.md)・[0029](adr/0029-graphics-picture-image-glyph.md)。

## イベント

- **登録**: `Xxx_SetOnYyy(obj, cb, data)` で登録する。
  - 同じイベントに何度呼んでもよく、最後に登録したものだけが呼ばれる。
  - cb に nil を渡すと解除する(コールバックの実行中に解除してもよい)。
- **コールバックの引数**:
  - 形は [internal/api.h](../internal/api.h) の `callback_t` 等。
  - 最初の引数は、イベントを発生させたオブジェクト(Sender)。最後の引数は、登録時に渡した data。
- **書き換えられる引数**(OnClose の Action・OnCloseQuery の CanClose・OnKeyDown の Key 等)は、ポインタで渡される。
  - 既定値が入った状態で呼ばれ、書き換えると LCL に反映される。
- **DLL の切り離し中**: イベントと破棄通知は呼ばれない。

## 例外([ADR 0031](adr/0031-exceptions-across-dll.md))

FPC の例外は、DLL の関数をまたいで伝わらない。そのため、どちらの向きも境界の手前で捕まえて受け渡す。

- **DLL の中で起きた例外**:
  - どの公開関数も、中で起きた例外を捕まえ、戻り値を既定の値(0・nil・偽・空文字列)にして戻る。
  - 戻る前に、`Error_SetCallback(cb)` で登録したコールバックを `cb(クラス名, メッセージ)` の形で呼ぶ。
    - 呼ぶのは、失敗した呼び出しと同じスレッド。
    - クラス名は EStringListError 等。
  - 呼び出し側は、呼び出しの前にスレッドごとの印をクリアし、コールバックで印を付け、戻った後に確かめる。
  - `Error_SetCallback` だけは例外を捕まえる対象外で、`internal/funcs.h` の一覧にも入れていない(読み込み時に 1 回だけ呼ぶ)。
- **コールバックの中で起きた失敗(呼び出し側の例外)**:
  - 呼び出し側は、例外をコールバックの外へ出してはならない。
  - コールバックの中で `SetCallbackError(クラス名, メッセージ)` を呼んでから戻る。
  - DLL はコールバックから戻った後に、それを例外として送出し直す。その後の扱いは次のとおり。
    - メッセージループの中なら、LCL が処理する(メッセージを表示して処理を続ける)。
    - DLL の関数の中で起きたイベント(`TMenuItem_Click` 等)なら、その関数が上と同じく失敗を知らせる(クラス名は元のもの)。
