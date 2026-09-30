# 0058. デザイナーで TShape の Pen・Brush を設定し、キャンバスに形を描く

- 状態: 承認
- 日付: 2026-09-30

## 背景

C++Builder では、TShape の `Pen`(縁の線)と `Brush`(中の塗りつぶし)を Object Inspector で設定できる。

Bethany のデザイナーでは、この 2 つを設定できなかった。

- `Pen`・`Brush` は値メンバだったため、カタログの抽出がプロパティとして扱わなかった。
- [ADR 0057](0057-canvas-pen-brush-as-pointers.md) で、C++Builder と同じくポインタのプロパティ(`Property<TPen*>`・`Property<TBrush*>`)にした。

また、キャンバスは TShape を、黒い枠の白い矩形(円・楕円なら丸)としか描いていなかった。

## 決定

### カタログとコード生成

- カタログの抽出で、`TPen`・`TBrush` を入れ子のオブジェクト(`OBJECT_CLASSES`。`Font` と同じ扱い)にする。
  - TShape に `Pen`(`Color`・`Width`・`Style`・`Mode`)と `Brush`(`Color`・`Style`)が出る。
  - 既定値は、LCL の実物から抽出する(Pen は clBlack・1・psSolid・pmCopy、Brush は clWhite・bsSolid)。
- Object Inspector とコード生成は、`Font` と同じ入れ子のオブジェクトの仕組みをそのまま使う。
  - C++ は `Shape1->Pen->Color = clRed;`、Python は `self.Shape1.Pen.Color = clRed` を生成する。

### キャンバス

TShape を SVG で描く(`webview/src/canvas/Shape.tsx`)。形の求め方は、LCL の `TCustomShape.Paint` に合わせる。

- 線の幅の半分だけ内側に描く。
- `stSquare`・`stRoundSquare`・`stCircle`・`stSquaredDiamond` は、短い辺に合わせて中央に置く。
- 角の丸い矩形の角の大きさは、短い辺の 1/4(角の楕円の幅)にする。
- 形の種類ごとの描き方:
  - 矩形・角の丸い矩形・楕円は、そのまま描く。
  - ひし形は、各辺の中点を結ぶ。
  - 三角形は、上下左右の 4 つの向きを描き分ける。
  - 星は 5 つの角で、上向きと下向き(`stStarDown`)がある。
  - `stPolygon` は頂点(`Points`)をデザイナーで設定しないため、LCL と同じく何も描かない。場所が分かるように点線の枠だけを出す。
- `Pen`:
  - `Color`・`Width` を反映する。
  - `Style` の破線は Windows の長さ(線の幅に合わせて伸ばす)で描く。`psClear` は線を描かない。
- `Brush`:
  - `Color` を反映する。
  - `bsClear` は塗りつぶさない。
  - ハッチ(`bsHorizontal`・`bsCross`・`bsDiagCross` 等)は、8 ピクセルの升目の模様で描く。

## 影響

- 見本のフォーム(OptionSheet)に、`Pen`・`Brush`・`Shape` を設定した TShape(Shape1)を加え、生成したコードを C++ と Python で実行して照合する
  (`shape: 2/255/2/1/65535/7`)。
- 入れ子のオブジェクトの一覧に TPen・TBrush が加わる。今のところ、これらを持つ部品は TShape だけ。
