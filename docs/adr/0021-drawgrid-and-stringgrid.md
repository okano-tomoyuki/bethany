# 0021. Tier 2 の 4 バッチ目として TDrawGrid と TStringGrid を追加する

- 状態: 承認(一部置換→0022)
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の Tier 2、4 バッチ目としてグリッド(TDrawGrid・TStringGrid)に着手した。
LCL のソース(`grids.pas`)で確認したこと:

- 継承関係は `TCustomControl → TCustomGrid → TCustomDrawGrid → TDrawGrid`、`TCustomDrawGrid → TCustomStringGrid → TStringGrid`。
- 行数・列数・固定行/列・現在のセル・列幅・Options・Selection・スクロール位置・Canvas・OnDrawCell・OnSelection は
  **TCustomGrid の protected** で、**TCustomDrawGrid が public にしている**(OnSelectCell・OnHeaderClick は TCustomDrawGrid 自身の public)。
  BeginUpdate・EndUpdate・Clear・CellRect・MouseToCell は TCustomGrid の public。Cells・Clean・AutoSizeColumns は TCustomStringGrid の public。
- **セルは LCL でもオブジェクトではなく(列, 行)の位置で指定する**ため、TTreeNode・TListItem のような寿命管理は要らない。
- `TGridOptions` はちょうど 32 要素の集合(goFixedVertLine 〜 goRowHighlight)。`TGridDrawState` は名前の無い列挙型の集合。

## 検討した選択肢

`Cells[ACol][ARow]` の表し方:

- 選択肢A: これまでのインデックス付きプロパティ(TCheckGroup の `Checked[i]`・TMenuItem の `Items[i]`)と同じく、`GetCells(ACol, ARow)` / `SetCells(ACol, ARow, Value)` にする。
- 選択肢B: 添字演算子を返すプロキシで `StringGrid1->Cells[ACol][ARow] = "x"` と書けるようにする。VCL からの移植では最もよく使う書き方だが、
  インデックス付きプロパティの表し方がこのクラスだけ異なることになる。

## 決定

選択肢A を採る(インデックス付きプロパティの表し方をライブラリ全体で揃える)。`ColWidths[ACol]`・`RowHeights[ARow]` も `GetColWidths` / `SetColWidths` 等にする。
添字演算子で書けるプロキシは、インデックス付きプロパティ全般の共通の仕組み(`IndexedProperty` のようなもの)として、別途まとめて検討する。

- C API: 主なメンバは公開しているクラスの名前で `no_vcl_TCustomDrawGrid_*`(TDrawGrid・TStringGrid の両方に使える)。
  TCustomGrid の public は `no_vcl_TCustomGrid_*`、文字列は `no_vcl_TCustomStringGrid_*`。
- C++: `TCustomGrid`(public のメソッドのみ)→ `TCustomDrawGrid`(プロパティ・イベント・`TCanvas Canvas`)→ `TDrawGrid`、
  `TCustomDrawGrid` → `TCustomStringGrid`(Cells 等)→ `TStringGrid`。Canvas は TPaintBox と同じく非所有の値メンバ。
- **Options** は TShiftState と同じくビット集合(ビットの位置は TGridOption の序数)。32 ビットすべてを使う(最上位が goRowHighlight)ため、
  C API は新しい `no_vcl_uint_t`(unsigned int)で受け渡し、定数は `#define no_vcl_go*`(符号なし)にした(C の enum は int の範囲に限られるため)。
  C++ は `using TGridOptions = unsigned int` と `go*` 定数。
- **TRect / TGridRect**: C++ に VCL と同じ `struct TRect { Left, Top, Right, Bottom }` を追加し、`TGridRect` はその別名(LCL も `TGridRect = TRect`)。
  C API は 4 つの整数(取得はポインタ 4 つ)で受け渡す。
- **イベント**: OnDrawCell(Sender, ACol, ARow, ARect, AState)・OnSelectCell(Sender, ACol, ARow, var CanSelect)・
  OnSelection(Sender, ACol, ARow)・OnHeaderClick(Sender, IsColumn, Index)。それぞれに専用のブリッジとコールバック型を追加した。
- 見送ったもの: Objects[ACol, ARow]・Cols[]/Rows[](TStrings)、OnGetEditText/OnSetEditText(var 文字列の受け渡し)、TGridColumns(Columns プロパティ)、
  OnCompareCells、ソートの向き(SortOrder)、クリップボード操作。

## 実装して分かったこと

- Win32 で Pascal から `Rect(...)` を呼ぶと、uses の Windows ユニットの型名 `Rect`(TRect の別名)に隠されて型キャストとして解釈され、
  コンパイルエラーになる。TRect はフィールドで組み立てる。
- プログラムから `Col`・`Row` を設定しても OnSelection が呼ばれる(LCL の挙動)。
- `SortColRow(True, Index)` は固定行を除いて、列 Index の文字列の順に行を並べ替える(他の列も一緒に移動する)。
- `Clear` は行・列をすべて削除し(ColCount・RowCount が 0)、`Clean` はセルの文字列だけを消す。
- LCL のグリッドは OS のコントロールではなく自前で描画する。マウスのメッセージで、セルのクリック → OnSelection、
  OnSelectCell で取りやめた列のクリック → 選択されない、列見出しのクリック → OnHeaderClick が呼ばれ、
  OnDrawCell で Canvas に描いた内容(市松模様・座標)が表示されることをスクリーンショットで確認した。
- C のテストで、文字列を返す関数を 1 つの printf に並べて渡すと、スレッドごとのバッファが上書きされて同じ値が並ぶ
  ([ADR 0013](0013-string-return-bridge-reuse-ctor-exception.md) の制約どおり)。テストをコピーしてから使う形に直した。
- Linux/GTK2(WSL)でも同じ結果になった。

## 影響

- 表形式の入力・表示の UI を組めるようになった。TDrawGrid と TCanvas で、独自の描画のグリッドも作れる。
- `Cells[c][r]` の書き方を VCL と揃えるかは、インデックス付きプロパティ全般の課題として残る。
