# 0061. プロパティに Get()・複合代入・文字列の比較と連結を加え、集合型をすべて Set<E> にそろえる

- 状態: 承認
- 日付: 2026-09-30

## 背景

1.0 に向けて、C++Builder と書き方が違う箇所を総点検する。その最初として、書き方の土台(プロパティと集合)を見直した。

**プロパティ**: `Property<T>` は値への暗黙の変換を持つが、次の場面で C++Builder のように書けなかった。

- 文字列のプロパティの比較・連結(`Edit1->Text == "abc"`・`Edit1->Text + "!"`)。std::string の演算子はテンプレートのため、
  プロパティから暗黙に変換されない。`std::string(Edit1->Text)` と書く必要があった(`test/main.cpp` だけで 103 か所)。
- 文字列のメソッドの呼び出し(`Edit1->Text.c_str()`)。C++Builder の String のメソッドは多く、網羅できない。
- 複合代入(`Button1->Left += 10;`・`Tag++;`)。
- `auto` で受けたとき(添字のプロパティは要素のプロキシのまま)や、`std::max` 等のテンプレートに渡すとき。

**集合**: C++Builder の集合は `Set<T, minEl, maxEl>` で、`Shift.Contains(ssShift)`・`Font->Style = TFontStyles() << fsBold` と書く。
Bethany では、`Set<E>` にしていたのは 3 つ(`TAnchors`・`TBorderIcons`・`TMsgDlgButtons`)だけで、残りの 13 はビットを OR する
`unsigned int` だった(`Shift & ssShift`・`fsBold | fsItalic`)。

- 対象: `TShiftState`・`TFontStyles`・`TOwnerDrawState`・`TMultiSelectStyle`・`TTreeViewOptions`・`TCustomDrawState`・`TGridOptions`・
  `TGridDrawState`・`TEdgeBorders`・`TOpenOptions`・`TColorDialogOptions`・`TFontDialogOptions`・`TFindOptions`

## 検討した選択肢

値を取り出す明示的な書き方として、次を比べた。

- 選択肢A: `Get()` を加える(`Edit1->Text.Get().c_str()`)。値を取り出したことが明示的で、std::string の全メソッドが使える。
- 選択肢B: `->` で std::string のメソッドを呼べるようにする(`Edit1->Text->c_str()`)。短いが、コピーが作られることが見えにくく、
  `const char* p = Edit1->Text->c_str();` と保存すると、すぐに無効なポインタになる。
- 選択肢C: C++Builder の String のメソッド(`Length()`・`IsEmpty()`・`ToInt()` 等)を真似る。網羅できない。
- 書き込みの `Set()` も検討したが、`=` と役割が重なり、書き方が 2 通りになるだけなので加えない。

## 決定

### プロパティ

- **`Get()`**(選択肢A): すべてのプロパティと、添字のプロパティの要素に加える。値を取り出して返す。
  - 名前が重ならないことを確かめた。ヘッダで同じ名前は、内部の `CanvasHolder::Get` だけ(プロパティとは別のクラス)。
    MinGW の Windows のヘッダに `Get` のマクロは無い。C++Builder の `TStrings::Get` 等は protected の読み出し関数で、
    しかも `Get()` はプロパティのオブジェクトのメンバなので、所有者のクラスのメソッドとは衝突しない。
- **複合代入**: `+=`・`-=`・`*=`・`/=`・`++`・`--`(前置・後置)を、書き込みのできるプロパティと要素に加える。
  - 値を読み出して演算し、書き戻す。演算できない型(列挙型等)で使うとコンパイルエラーになる(使ったときだけ実体化されるテンプレート)。
  - 文字列にも使える(`Label1->Caption += "!";`)。
- **文字列のプロパティの比較・連結**: 値の型が std::string のプロパティ・要素に限り、次の演算子を加える。
  - 比較: `==`・`!=`・`<`・`>`・`<=`・`>=`(相手は std::string・`const char*`・文字列のプロパティ)
  - 連結: `+`(相手は std::string・`const char*`・`char`・文字列のプロパティ)
  - `std::ostream` への出力(`std::cout << Edit1->Text`)
- Python は、プロパティが値そのものを返すため、変わらない。

### 集合

- 13 の型を、要素の列挙型と `Set<E>` にする。要素の列挙型の名前は LCL に合わせる(`TShiftStateEnum`・`TFontStyle`・
  `TOwnerDrawStateType`・`TMultiSelectStyles`・`TTreeViewOption`・`TCustomDrawStateFlag`・`TGridOption`・`TEdgeBorder`・
  `TOpenOption`・`TColorDialogOption`・`TFontDialogOption`・`TFindOption`)。
  - `TGridDrawState` の要素は LCL では名前の無い列挙型なので、`TGridDrawStateItem` とする。
  - 要素の値(序数)は LCL と同じ。DLL との受け渡しは、これまでどおりビットの整数(`Set::ToInt`・`Set::FromInt`)。
- `Set` に `Clear()` と、要素の型 `Element` を加える。
- 集合のプロパティにも演算子を加える: `Grid1->Options = Grid1->Options << goEditing;`(`<<`・`>>`・`+`・`-`・`*`。
  新しい集合を返すので、C++Builder と同じく代入で書き戻す)。要素の確認は `Grid1->Options->Contains(goEditing)`。
- **Python は変えない**: この 13 の型は、これまでどおり `enum.IntFlag`(`fsBold | fsItalic`・`Shift & ssShift`)にする。
  - 生成器(`gen_api.py`)は、この 13 の `Set<E>` を IntFlag として出し、要素の列挙型は Python に出さない。値は同じ。
  - Python では、`TAnchors` 等のほかの `Set<E>` は frozenset のままで、2 通りの表し方が残る(C++ の書き方の変更の範囲を超えるため、ここでは変えない)。
- **デザイナー**: C++ のコード生成で、ビットの集合も `TFontStyles() << fsBold` の形で書く。カタログ・Python の生成は変わらない。

### 違いとして残すもの

次は C++Builder と違うが、標準の C++ の制約や ADR 0060 の範囲によるもので、変えない(利用者向けの文書で説明する)。

- **イベントの代入**: `Button1->OnClick = Button1Click;` は `__closure`(C++Builder の独自の拡張)によるもので、標準の C++ では書けない。
  ラムダで割り当てる(デザイナーはそう生成する)。
- **文字列の型**: `String`(UTF-16)ではなく `std::string`(UTF-8)。メソッドは `Get()` で取り出して std::string のものを使う。
- **フォームの生成**: `Application->CreateForm(__classid(TForm1), &Form1)` ではなく `Application->CreateForm(&Form1)`。
- **変換の関数**: `IntToStr`・`StrToInt`・`Format` 等は RTL の関数なので扱わない([ADR 0060](0060-scope-gui-development.md))。
  `std::to_string`・`std::stoi` 等を使う。

## 影響

- **C++ の互換が無い変更**(次の版を 0.5.0 にする)。
  - `Shift & ssShift` は `Shift.Contains(ssShift)` に。
  - `Font->Style = fsBold | fsItalic;` は `Font->Style = TFontStyles() << fsBold << fsItalic;` に。
  - `Grid1->Options = Grid1->Options | goEditing;` は `Grid1->Options = Grid1->Options << goEditing;` に。
  - `ShortCut('N', ssCtrl)` は `ShortCut('N', TShiftState() << ssCtrl)` に(C++Builder と同じ)。
  - printf 等でビットの整数が要るときは `.ToInt()`(プロパティなら `.Get().ToInt()`)。
- デザイナーの生成する C++ のコードが変わる(`TFontStyles() << fsBold`)。このコードを使うには、ライブラリ 0.5.0 以降が要る。
  example の生成済みのコード(`Font->Style = fsBold` 等)も、0.5.0 で生成し直す。
- `test/main.cpp` と、デザイナーの照合のプログラム(verify-cpp)を新しい書き方に直した。
- 確認のプログラムで、`Get()`・複合代入・文字列の演算子・集合の演算子・32 個の要素を持つ集合(TGridOptions)・
  イベントの Shift(`Shift.Contains(ssShift)`)を確かめた。Python は、生成されるコードが(値の書き方を除いて)変わらないことを確かめた。
