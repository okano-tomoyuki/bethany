# 0045. フォーム・パネルへの描画と Canvas の描画の関数(Tier B の B5・B6)

- 状態: 承認
- 日付: 2026-09-29

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B5・B6。これまで Canvas は TPaintBox・TImage・グリッドにしか無く、
フォームやパネルに直接描けなかった。Canvas の描画の関数も、線・矩形・楕円・文字・塗りつぶし・画像だけだった。

## 決定

### B5: TCustomControl の Canvas・OnPaint

- `TCustomControl`(TForm・TPanel・TScrollBox・グリッド・ツリービュー等の基底)に `Canvas`(値メンバ。TPaintBox::Canvas と同じ非所有のビュー)を加える。
  LCL の TCustomControl の Canvas はコントロールの生成時に作られ、コントロールと寿命が一致する。
- `OnPaint` は LCL と同じく TCustomControl の protected に置き、LCL が published にしている TCustomForm(TForm)・TPanel・TScrollBox で
  `using` で公開する(TCheckBox の Checked と同じ形)。DLL は TCustomControl のアクセス用のクラスで protected の OnPaint を設定する。
- グリッド(TCustomDrawGrid)が持っていた専用の `Canvas` と DLL の `TCustomDrawGrid_GetCanvas` は外し、TCustomControl の Canvas を使う
  (LCL でもグリッドの Canvas は TCustomControl のもの。利用者のコードの `DrawGrid1->Canvas` はそのまま使える)。

### B6: Canvas の描画の関数

- TCanvas に `TextWidth`・`TextHeight`・`TextRect(Rect, X, Y, Text)`・`Polygon`・`Polyline`・`RoundRect`・`Arc`・`Pie`・`Chord`・`FrameRect`・
  `CopyRect(Dest, Canvas, Source)` を加える。
- **点の配列**: DLL には (X, Y) を並べた整数の配列と点の数で渡す。
  - C++ は `const std::vector<TPoint>&`(`Canvas->Polygon({{0, 0}, {10, 0}, {5, 8}})` と書ける)と、`const TPoint* Points, int Count` の 2 つ。
    C++Builder の `Polygon(const TPoint* Points, const int Points_Size)` は 2 つ目の引数が「最後の添字」(点の数 - 1)だが、
    取り違えやすいので Bethany は点の数にする。
  - Python は TPoint か `(x, y)` の並び(手書きの _core.py。`CopyRect` も)。
- TPen に `Style`(`TPenStyle`)・`Mode`(`TPenMode`)、TBrush に `Style`(`TBrushStyle`)、TFont に `Height`・`Orientation`(0.1 度単位)・
  `Quality`(`TFontQuality`)を加える。列挙型の値は LCL と同じ。
- `Pixels` で読めるのは、描画の中で塗られた範囲だけ(OnPaint が一部の範囲の再描画で呼ばれたときは、範囲の外は -1 になる。Windows の GetPixel)。

### デザイナー

- TForm・TPanel・TScrollBox のイベントに `OnPaint` が出る。フォントに `Height`・`Orientation`・`Quality` が出る(入れ子のオブジェクト)。
- キャンバスは Font の `Height` を文字の大きさに反映する(0 でなければ Size より優先。コード生成は Size の後に Height を設定するため)。
  `Orientation`・`Quality` はキャンバスには反映しない。

### あわせて直したもの: 最小化

確認の途中で、メインフォームを最小化するとフォームが隠れたまま、タスクバーのボタンも消えて元に戻せないことが分かった(以前からの問題)。
LCL は `Application.MainFormOnTaskBar` が false(既定)のとき、メインフォームの最小化をアプリケーションのウィンドウ(AppHandle)の最小化に
置き換えてメインフォームを隠すが、DLL(IsLibrary)では AppHandle が作られず 0 のままだった。DLL の初期化で `MainFormOnTaskBar` を true にし、
メインフォームがタスクバーのボタンを持つようにした(Delphi の既定と同じ。ほかのフォームはメインフォームに所有され、最小化で一緒に隠れる)。
`Application->Minimize()`・`Restore()` も同じ理由で効いていなかったが、これで効く。

## 影響

- B8(オーナードロー)の `OnDrawItem` は、この Canvas に描く。
