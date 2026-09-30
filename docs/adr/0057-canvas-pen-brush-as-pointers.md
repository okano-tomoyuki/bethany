# 0057. Canvas・Pen・Brush・Font を C++Builder と同じくポインタのプロパティにする

- 状態: 承認
- 日付: 2026-09-30

## 背景

C++Builder(VCL)では、次のものはどれもポインタのプロパティである。

- `Canvas`(`TCanvas*`)
- Canvas の `Pen`・`Brush`・`Font`(`TPen*`・`TBrush*`・`TFont*`)

そのため、`PaintBox1->Canvas->Pen->Color = clRed;` のように、すべて `->` で書く。

Bethany の C++ では、次のものを値メンバとして持っていた。

- `TCanvas` の `Pen`・`Brush`・`Font`
- `TCustomShape` の `Pen`・`Brush`
- `Canvas`: TCustomControl(TDrawGrid・TStringGrid・TForm・TPanel 等が継承する)・TCustomListView・TCustomComboBox・TCustomListBox・TStatusBar・TPaintBox

そのため、`PaintBox1->Canvas.Pen.Color = clRed;` と書く必要があり、C++Builder のコードをそのまま移せなかった
([ADR 0002](0002-object-model-fidelity.md) のオブジェクトモデルの忠実さに反する)。

一方、次のものはすでにポインタだった。

- コントロールの `Font`(`Property<TFont*>`)
- TGraphicControl・グラフィック・TImage の `Canvas`(`ReadOnlyProperty<TCanvas*>`)

## 決定

C++Builder と同じく、すべてポインタのプロパティにする。

- **`Canvas`**: `ReadOnlyProperty<TCanvas*>`(C++Builder でも読み取り専用)。
  - ラッパーの実体(`std::unique_ptr<TCanvas>`)は、所有者のクラスの private に置き、最初に読んだときに作る。
  - LCL の Canvas はコントロールの破棄まで同じものなので、常に同じポインタを返す。
- **`TCanvas` の `Pen`・`Brush`・`Font`、`TCustomShape` の `Pen`・`Brush`**: `Property<TPen*>` 等。
  - ラッパーの実体は、所有者の private の値メンバにする。
  - 代入(`Canvas->Pen = OtherPen;`)は、VCL・LCL と同じく内容のコピー(Assign)。代入したものは、代入した側の持ち物のまま。
  - nullptr の代入は何もしない。
  - 代入のため、DLL に次を加える: `TCanvas_SetPen`・`TCanvas_SetBrush`・`TCanvas_SetFont`・`TCustomShape_SetPen`・`TCustomShape_SetBrush`
- **`TPen::Assign`・`TBrush::Assign`**: `TFont::Assign` と同じく加える(`TPen_Assign`・`TBrush_Assign`)。
- **Python**: 書き方は変わらない(`Canvas.Pen.Color`)。
  - 生成器は、値メンバとポインタのプロパティから同じコードを作る。
  - 加わるのは、`Pen`・`Brush`・`Font` への代入(内容のコピー)と `Assign` だけ。

## 影響

- **C++ の互換が無い変更**。`X->Canvas.Pen.Color` は `X->Canvas->Pen->Color` に書き換える。
  - `TCanvas& c = X->Canvas;` と参照で受けていたものは `TCanvas* c = X->Canvas;` にする。
  - `&X->Canvas` と渡していたものは `X->Canvas` にする。
  - 0.x の間はマイナーバージョンを上げる(次の版を 0.4.0 にする)。
- デザイナーの生成するコードは Canvas を使わないため、変わらない。カタログも変わらない。
- `test/main.cpp` のデモを新しい書き方に直した。
- 確認のプログラムで、次を C++ と Python で確かめた。
  - 各コントロールの Canvas が同じポインタを返すこと(TPaintBox・TDrawGrid・TListBox・TComboBox・TListView・TStatusBar)。
  - `Canvas->Pen->Color` 等の読み書き。
  - `Pen`・`Brush`・`Font` と TShape の `Pen`・`Brush` への代入が内容のコピーであること。
  - `Assign`、グラフィックの Canvas、OnPaint の中での描画。
