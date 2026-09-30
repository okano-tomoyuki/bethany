# 0063. イベントの引数・Handle を C++Builder にそろえ、VCL の名前の別名を加える(総点検のクラスごとの点検)

- 状態: 承認
- 日付: 2026-09-30

## 背景

1.0 に向けた C++Builder との違いの総点検の 2 回目。[ADR 0061](0061-property-ergonomics-and-sets.md) の書き方の土台に続いて、
イベントの形と、よく使う API を VCL の宣言と比べた。C++Builder のコードを移すときに書き換えが要る違いが見つかった。

- イベントの引数の型: OnKeyDown の Key(`int&`。VCL は `Word&`)、OnMouseWheel の位置(`int X, int Y`。VCL は `const TPoint& MousePos`)、
  Application の OnException(`const Exception&`。VCL は `Exception*`)、ActionList の OnExecute・OnUpdate(先頭に Sender。VCL・LCL には無い)。
- **Handle**: Bethany の `Handle()` は、DLL に渡す LCL のオブジェクト(利用者が解釈しない値)だった。VCL の `Handle` はウィンドウのハンドル
  (HWND)で、`SendMessage(Edit1->Handle, ...)` のように Win32 の API に渡す。Bethany にはこれを得る方法が無く、`Handle()` を HWND のつもりで
  使うと、コンパイルが通るのに誤った値になる。
- OnKeyPress の Key(`char&`)は 1 バイトで、日本語等の 1 バイトでない文字を受けられなかった(VCL は `WideChar&`)。
- VCL の型名・書き方が無いもの(`TDrawCellEvent`・`TSelectCellEvent`・`TSectionNotifyEvent`、TTreeNode の `Item[i]`、Canvas の
  `Rectangle(TRect)`・`PenPos`・`ClipRect`、TGridDrawState の `gdHotTrack` 等)。
- 描画のイベントの矩形が値渡しの `TRect ARect` だった(VCL は `const TRect& Rect`)。

## 決定

### イベントの引数(C++ の互換が無い変更)

- OnKeyDown・OnKeyUp の Key は `Word&`(`using Word = unsigned short;`。VCL の System::Word)。
- OnKeyPress の Key は `std::string&`(入力された 1 文字の UTF-8)。
  - 日本語等も 1 文字で来る。空にすると入力を LCL に渡さず、別の文字にするとその文字が入力される(2 文字以上なら先頭の 1 文字)。
  - `const std::string&` にしなかったのは、C++Builder で `Key = 0;`(入力を捨てる)・`Key = toupper(Key);` のように書き換えるのがよくある書き方のため。
  - DLL は LCL の OnKeyPress ではなく OnUTF8KeyPress(`var UTF8Key: TUTF8Char`)につなぐ。文字列は UTF-8 が前提。
- OnMouseWheel の位置は `const TPoint& MousePos`(コントロールのクライアント座標)。
- Application の OnException の E は `Exception*`(`E->Message`)。ハンドラの中だけ有効。
- ActionList の OnExecute・OnUpdate は `(TBasicAction* Action, bool& Handled)`(VCL・LCL と同じく Sender は無い)。
- 描画のイベントの矩形を `const TRect& Rect` にする(TDrawItemEvent・TLVDrawItemEvent・TOnDrawCell。メニューの TMenuDrawItemEvent は
  VCL と同じく `const TRect& ARect`)。値渡しの `TRect` を受けるラムダも、そのまま割り当てられる。TOnDrawCell の状態の名前も VCL と同じ `State` にする。

### Handle(C++ の互換が無い変更)

- 内部の `TObject::Handle()`(LCL のオブジェクト)を `ObjHandle()` に改める。Python の `ObjHandle` も同じ。
- `TWinControl::Handle`(`ReadOnlyProperty<TWindowHandle>`)と `TCanvas::Handle`(`ReadOnlyProperty<TDCHandle>`)を加える。
  - VCL と同じく、読んだときにまだウィンドウ(描画先)が無ければ作ってから返す(LCL の Handle の読み出しと同じ)。
- **windows.h を含めない**: `beth.hpp` はグローバル名前空間で `struct HWND__; struct HDC__;` を前方宣言し、`TWindowHandle` を `::HWND__*`、
  `TDCHandle` を `::HDC__*` とする。
  - windows.h(STRICT。MinGW・Windows SDK の既定)の `HWND`・`HDC` と同じ型になるので、windows.h を含めたコードでは `SendMessage(Edit1->Handle, ...)` と
    そのまま渡せる(C++Builder と同じ)。含めないコードでは中身の見えないポインタとして扱うだけ。
  - `beth` の名前空間に `HWND` という名前は作らない。`using namespace beth;` と windows.h を併用したとき、名前が曖昧になるため。
  - Windows 以外では `void*` にする(`#ifdef _WIN32`)。LCL の Handle はどの OS でもポインタの大きさの値なので、他の OS への対応の妨げにならない。
- Python では、`Handle` はポインタの値の整数(ctypes・pywin32 に渡せる)。
- **windows.h のマクロとの衝突**: windows.h は `TextOut`・`MessageBox`・`FindText`・`ReplaceText` を、`TextOutA`・`TextOutW` 等に置き換えるマクロとして
  定義する(Bethany の名前でマクロになっているのは、この 4 つだけであることを `g++ -dM -E` で確かめた)。`Handle` を使うために windows.h を含めると、
  - beth.hpp の後に含めた場合: 利用者の `Canvas->TextOut(...)` が `TextOutA` になり、コンパイルエラーになる。
  - beth.hpp の前に含めた場合: ヘッダの宣言も `TextOutA` になり、ライブラリ(windows.h を含めずにコンパイルした `beth.cpp`)の名前とずれて、リンクできない。
  - そのため、beth.hpp の先頭で 4 つのマクロを `#pragma push_macro`・`#undef` で外し、末尾で `#pragma pop_macro` で戻す(宣言は常に本来の名前になる)。
  - 利用者のコードでマクロが置き換えた名前は、同じものの別名として持つ(`_WIN32` のときだけ)。メソッド(`TextOutA`・`TextOutW`・`MessageBoxA`・
    `MessageBoxW`)は呼び出しを渡し、プロパティ(`FindTextA`・`FindTextW`・`ReplaceTextA`・`ReplaceTextW`)は同じプロパティへの参照にする。
    Python の生成器は、これらの別名を Python に出さない。
  - windows.h を beth.hpp の前・後に含めた場合と、`UNICODE` を定義した場合のどれでも、`TextOut`・`MessageBox`・`FindText`・`ReplaceText` が
    コンパイルでき、動くことを確かめた。
- Python の生成器は、ヘッダのプリプロセッサの行を読まない(`#ifdef _WIN32` の両方の分岐の宣言は、どちらも Python に出さない型)。

### VCL の名前・書き方を加える(互換を壊さない)

- 型の別名: `TDrawCellEvent`(= `TOnDrawCell`)・`TSelectCellEvent`(= `TOnSelectCellEvent`)・`TSectionNotifyEvent`(= `TCustomSectionNotifyEvent`)。
- `TTreeNode::Item[i]`(`Items[i]` と同じ)。
- Canvas: `Rectangle(const TRect&)`・`Ellipse(const TRect&)`・`PenPos`・`ClipRect`。Python の `Rectangle`・`Ellipse` は、4 つの整数と TRect のどちらでも受ける。
- TGridDrawState の要素の VCL の名前: `gdHotTrack`・`gdPressed`・`gdRowSelected`(LCL の `gdHot`・`gdPushed`・`gdRowHighlight` と同じ値)。

### LCL に合わせて残すもの

- TTreeView の OnCompare の `Data`(VCL にはあるが LCL には無く、値を渡せないため加えない)。
- メニューの OnDrawItem の状態(VCL は `bool Selected`、LCL は `TOwnerDrawState State`。LCL のほうが情報が多い)。
- LCL にしか無いメンバ(`EchoMode` 等)。

### Python

- 書き方が変わるのは次の 2 つ。
  - OnKeyPress の Key は、1 文字の文字列の `Ref`(`Key.value`)。空にすると入力を捨てる(以前は `chr(0)` 等)。
  - OnMouseWheel は、X・Y の代わりに `TPoint` の MousePos を受ける(C++ と同じ形)。
- ActionList の OnExecute・OnUpdate は、これまでどおり `(Sender, Action, Handled)`(生成器で Sender の無いイベントを区別する)。
- OnException の E・OnKeyDown の Key は変わらない。

## 影響

- **C++ の互換が無い変更**(0.5.0 に含める)。
  - `Handle()` は `ObjHandle()` に(利用者が使うことはほぼ無い)。`Edit1->Handle` は HWND になる。
  - ハンドラの引数: `int& Key` → `Word& Key`、`char& Key` → `std::string& Key`、`int X, int Y` → `const TPoint& MousePos`、
    `const Exception& E` → `Exception* E`(`E.Message` → `E->Message`)、ActionList の `(TObject* Sender, TBasicAction* Action, bool& Handled)` →
    `(TBasicAction* Action, bool& Handled)`。
- デザイナーの生成するハンドラの引数も変わる(`beth::Word& Key`・`const beth::TRect& Rect` 等)。生成するヘッダでは `Word` も `beth::` で修飾する。
- 確認のプログラム(C++・Python)で、次を確かめた: `Handle` が実際のウィンドウで `SendMessage` が効くこと、`Canvas->Handle` が HDC であること、
  `PenPos`・`ClipRect`・`Rectangle(TRect)`、`Item[i]`、OnKeyPress の日本語の 1 文字と書き換え(`!` → `?`、`#` を捨てる、`あ` → `い`)、
  OnKeyDown の `Word`、OnMouseWheel の MousePos(クライアント座標)、OnException の `Exception*`、ActionList の OnExecute と Handled、VCL の別名。
