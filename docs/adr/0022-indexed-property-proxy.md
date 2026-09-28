# 0022. インデックス付きプロパティを添字で書ける共通のプロキシを入れ、グリッドの Cells・ColWidths・RowHeights に使う

- 状態: 承認
- 日付: 2026-09-27

## 背景

これまで、インデックス付きプロパティは C++ では Get/Set メソッドの組で表してきた:

- TCheckGroup の `GetChecked(i)`([ADR 0015](0015-tier1-batch1-and-statusbar-issue.md))
- TMenuItem の `GetItem(i)`([ADR 0017](0017-menus-and-wrapping-lcl-created-components.md))
- TPageControl の `GetPage(i)`([ADR 0018](0018-pagecontrol-and-tabsheet.md))
- グリッドの `GetCells(c, r)` / `GetColWidths(c)`([ADR 0021](0021-drawgrid-and-stringgrid.md))

ただ、VCL からの移植では `StringGrid1->Cells[ACol][ARow] = "x"` が最もよく使われる書き方で、ここを書き換えずに済むことの価値が大きい。
ADR 0021 では、この書き方を「インデックス付きプロパティ全般の共通の仕組み」として別途検討することにしていた。

## 検討した選択肢

- 選択肢A: Get/Set メソッドのまま(移植時に `Cells[c][r]` をすべて書き換える必要がある)。
- 選択肢B: グリッド専用のプロキシ(グリッドへのポインタを持ち、`operator[]` で列、さらに `operator[]` で行を受ける)。
- 選択肢C: B と同じ考え方を `Property<T>` と同じ形の汎用テンプレートにする。所有者(`TObject*`)と、添字を取る固定の Getter/Setter 関数ポインタを持つ。

## 決定

選択肢C を採る。

- **`IndexedProperty<T>`**(添字 1 つ)と **`IndexedProperty2<T>`**(添字 2 つ)を beth.hpp の `Property<T>` の隣に追加した。
  - 所有者・Getter/Setter の持ち方は `Property<T>` と同じ(ヒープ確保なし、コピー・代入は禁止)。
  - `operator[]` は要素のプロキシ `Reference` を返す(`IndexedProperty2` は途中に `Slice` を挟む)。
  - `Reference` は代入で値を書き込み、`T` への変換で値を読み出し、ポインタ型なら `->` でメンバをたどれる。
  - 要素同士の代入(`Cells[2][1] = Cells[1][1]`)は値のコピーとして扱う(`Property<T>` 同士の代入と同じ)。
  - `Reference` は C++11 で値として返すためにコピー構築できる。代入が値の書き込みになる点は `std::vector<bool>::reference` と同じ考え方。
- Delphi の `Cells[ACol, ARow]` は、C++Builder と同じく **`Cells[ACol][ARow]`**(1 つ目が列、2 つ目が行)と書く。
- グリッドの `Cells`(TCustomStringGrid)と `ColWidths`・`RowHeights`(TCustomDrawGrid)をこれに置き換えた。
  `GetCells` / `SetCells` / `GetColWidths` 等の public メソッドは削除した(VCL には無く、同じことをする書き方が 2 つになるため)。
- C API は変更しない(添字の受け渡しは C++ 側だけの話)。

## 実装して分かったこと

- `Reference` は値ではないため、次の場合は値を明示的に取り出す必要がある。いずれも既存の `Property<T>` と同じ制約。
  - `printf` の可変長引数に渡すとき: `std::string(Grid->Cells[c][r]).c_str()`、`(int)Grid->ColWidths[i]`。
  - `auto` で受けるとき: 値ではなくプロキシになるので、`std::string s = Grid->Cells[c][r];` のように型を明示する。
  - テンプレートの演算子(`std::string` の `+` や `==`)を使うとき: 暗黙の変換が効かない。
- `Property<std::string>` との相互の代入(`Form1->Caption = Grid->Cells[0][1];` と `Grid->Cells[2][2] = Form1->Caption;`)は、
  それぞれの変換演算子を経由してそのまま書ける。
- Windows・Linux/GTK2(WSL)の両方で、C++ テストの結果が以前の Get/Set メソッドのときと同じになった。

## 影響

- VCL の `StringGrid1->Cells[c][r]`・`ColWidths[i]` のコードを、書き換えずに移植できる。
- 残りのインデックス付きプロパティ(`Checked[i]`・`Items[i]`・`Pages[i]` 等)は、まだ Get/Set メソッドのまま。
  - 読み書きできるものは、同じ `IndexedProperty<T>` に置き換えられる。
  - 読み取り専用のものは、代入できない版(`ReadOnlyIndexedProperty<T>` のようなもの)が別に要る。
  - 置き換えるかどうかは、別途判断する。
