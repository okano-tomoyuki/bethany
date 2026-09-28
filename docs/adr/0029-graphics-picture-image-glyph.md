# 0029. Tier 3 の 1 バッチ目として、グラフィックス基盤(TBitmap・TPicture)と TImage・Glyph を追加する

- 状態: 承認
- 日付: 2026-09-27

## 背景

[component-coverage.md](../component-coverage.md) の Tier 3 は、画像を扱うもの(TImage・TBitBtn/TSpeedButton の Glyph・TImageList)で、
いずれも Bethany にまだ無いグラフィックス基盤(TBitmap・TPicture)を前提にしていた。
Tier 3 は大きいため、これまでと同じくバッチに分ける。

- 1 バッチ目(この ADR): グラフィックス基盤、TImage、Glyph。
- 2 バッチ目: TImageList と、各コントロールの Images・ImageIndex。

LCL のソース(`graphics.pp`・`include/picture.inc`・`include/rasterimage.inc`・`extctrls.pp`・`include/customimage.inc`・
`buttons.pp`・`include/bitbtn.inc`・`include/buttonglyph.inc`)で確認したこと:

- **継承関係**: `TPersistent → TGraphic → TRasterImage → TCustomBitmap → TFPImageBitmap → TBitmap / TPortableNetworkGraphic / TJPEGImage`。
  TPicture は TPersistent の直接の派生。いずれも TComponent ではない。
- **TPicture の中身は差し替わる**:
  - `Bitmap`・`PNG`・`Jpeg` の Getter は `ForceType` を呼ぶ。中身が別のクラスなら、そのクラスのオブジェクトを作って `Assign` で写し、
    古い方を破棄する(空なら空のものを作る)。VCL の Picture->Bitmap は中身を捨てるが、LCL は画素を引き継ぐ。
  - `Graphic` の Setter は、渡したものと同じクラスのオブジェクトを作って `Assign` で写す(渡したものは呼び出し側の持ち物のまま)。
  - `LoadFromFile` は拡張子からクラスを選び、中身を作り直す。
  - つまり TStrings([ADR 0027](0027-tstrings.md))と同じく、**同じ TPicture でも中身のオブジェクト(ハンドル)は変わる**。
- **TRasterImage.Canvas** はグラフィックが所有し、初めて参照したときに作られる。中身が作り直されると別のものになる。
- **TCustomImage**:
  - `Picture` は生成時に作られて差し替わらない(Setter は `Assign`)。
  - `Canvas` は、Picture が空ならコントロールの大きさの TBitmap を作ってから、その Canvas を返す。
  - `Images`・`ImageIndex`(TImageList)も持つが、2 バッチ目に回す。
- **Glyph**:
  - TCustomBitBtn・TCustomSpeedButton の Glyph は、ボタンの生成時に作られる TBitmap で、差し替わらない(Setter は `Assign`)。
  - Glyph を設定すると、`NumGlyphs` は画像の幅と高さの比から決め直される。
  - **TCustomBitBtn は Glyph を設定すると `Kind` を bkCustom に戻す**(`SetGlyph` の先頭で `Kind := bkCustom`。Caption はそのまま)。
- **AutoSize** は TControl の public。TCustomImage を含め、多くのコントロールが使うが Bethany には無かった。

## 検討した選択肢

グラフィックの C++ での持ち方:

- 選択肢A: TCanvas と同じく、ハンドルを覚える値メンバにする。
  TPicture は中身を作り直すため、`Image1->Picture->Bitmap` のビューが破棄済みのオブジェクトを指してしまう。
- 選択肢B: TStrings と TStringList(ADR 0027・[0028](0028-labelededit-and-stringlist.md))と同じく、2 通りの持ち方を 1 つのクラスで表す。
  - 所有者の中身のビュー: 所有者と「所有者から中身を取り出す C API の関数」を持ち、操作のたびに中身を取り直す。
  - 利用者が生成するもの: 自分のハンドルを中身とし(取得関数は恒等関数)、`delete` で LCL のオブジェクトも破棄する。

Canvas の返し方(グラフィック・TImage の Canvas):

- 選択肢A: 取得のたびに TCanvas のラッパーを作る。前に返したポインタが無効になるか、リークする。
- 選択肢B: 所有者ごとにラッパーを 1 つ持ち、取得のたびに現在の Canvas のハンドルを確かめ、変わっていれば作り直す。
  TCanvas のラッパーは Pen・Brush・Font のハンドルを構築時に覚えるため、同じアドレスに別の Canvas が作られた場合に備え、
  それらのハンドルも比べる。

Picture・Graphic・Glyph への代入:

- 選択肢A: TStrings の Items と同じく読み取り専用にし、内容の置き換えは Assign で書く。
- 選択肢B: LCL が Setter を持つものは代入できるようにし、代入は LCL と同じく内容のコピーにする。
  `Image1->Picture->Graphic = Png;`・`BitBtn1->Glyph = Bmp;` は VCL でよく使う書き方。

## 決定

- **C API**:
  - TGraphic: `beth_TGraphic_Destroy`・`Get/SetWidth`・`Get/SetHeight`・`GetEmpty`・`Get/SetTransparent`・`LoadFromFile`・`SaveToFile`・`Assign`・`Clear`。
  - TRasterImage: `GetCanvas`・`Get/SetPixelFormat`・`Get/SetTransparentColor`・`Get/SetTransparentMode`。TCustomBitmap: `SetSize`。
  - 生成: `beth_TBitmap_Create`・`TPortableNetworkGraphic_Create`・`TJPEGImage_Create`(破棄は共通の `beth_TGraphic_Destroy`)。
    TJPEGImage: `Get/SetCompressionQuality`。
  - TPicture: `Create`・`Destroy`・`Get/SetGraphic`・`GetBitmap`・`GetPNG`・`GetJpeg`・`GetWidth`・`GetHeight`・`LoadFromFile`・`SaveToFile`・`Assign`・`Clear`。
  - TImage: `beth_TImage_Create`、TCustomImage の `Get/SetPicture`・`GetCanvas`・`GetHasGraphic`・`Center`・`Stretch`・`StretchOutEnabled`・
    `StretchInEnabled`・`Proportional`・`Transparent`・`SetOnPictureChanged`。
  - Glyph: TCustomBitBtn・TCustomSpeedButton の `Get/SetGlyph`・`NumGlyphs`・`Layout`・`Margin`・`Spacing`。
  - TCanvas: `Draw`・`StretchDraw`・`FillRect`・`Get/SetPixels`。TControl: `Get/SetAutoSize`。
  - 列挙は `beth_pf*`・`beth_tm*`・`beth_blGlyph*`。
  - TPicture の中身・Canvas のハンドルは**保存せず、使うたびに取得する**ことを、ヘッダーに明記した(TStrings と同じ)。
- **C++**:
  - 階層: `TGraphic → TRasterImage → TCustomBitmap → TBitmap / TPortableNetworkGraphic / TJPEGImage` と `TPicture`(いずれも TPersistent)。
    TFPImageBitmap は LCL 固有で公開するメンバも無いため省いた([ADR 0007](0007-lcl-faithful-hierarchy.md) の方針どおり)。
  - グラフィックの持ち方は選択肢B。`new TBitmap` / `delete`(スタック・値メンバも可)と、所有者の値メンバのビューがある。
    ビューの `Handle()` は nullptr で、C API に渡すハンドルは `Current()` で得る(TStrings と同じ)。
  - TPicture は、利用者が `new` / `delete` するものと、TCustomImage の値メンバ(破棄しない)がある。
    TCustomImage の Picture は差し替わらないため、TPicture 自身はハンドルを覚える。中身の `Graphic`・`Bitmap`・`PNG`・`Jpeg` はビュー。
  - `Picture->Graphic` は、空なら nullptr(VCL と同じ)、それ以外はクラスを問わない TGraphic のビューを返す。
    C++ の型は常に TGraphic なので `dynamic_cast<TBitmap*>` はできない。クラスごとの操作は `Bitmap`・`PNG`・`Jpeg` を使う。
  - Canvas は選択肢B(`CanvasHolder`)。`ReadOnlyProperty<TCanvas*>` で返し、`Bitmap->Canvas->Pen.Color = clRed;` のように書く
    (Pen 等は既存の TCanvas と同じく値メンバ)。
  - 代入は選択肢B。`Picture`・`Graphic`・`Bitmap`・`PNG`・`Jpeg`・`Glyph` は `Property<T*>` で、代入は内容のコピー、nullptr は空にする。
    TStrings の Items(読み取り専用)は、この ADR では変えない。
  - TCanvas に `Pixels[X][Y]`(`IndexedProperty2<TColor>`)・`FillRect`・`Draw`・`StretchDraw` を足した。
    これらのために、TRect・TPoint の定義を Grid の節から前に移した。
  - TControl に `AutoSize` を足した。
  - Glyph を持つ TCustomBitBtn は hpp の前半にあるため、Canvas とグラフィックスのクラスを TStringList の直後に移した。
- **例外**: 読み込めないファイル・形式の違うファイル(TBitmap に PNG を読む等)では LCL が例外を送出し、呼び出し側では捕捉できない。
  ADR 0028 の LoadFromFile 等と同じく、使い方の制約としてヘッダーに書いた(DLL 越しの例外の扱いは横断的な課題として残す)。
- 見送ったもの:
  - TImageList と、TImage・ボタン・ツリー等の `Images`・`ImageIndex`(2 バッチ目)。
  - TIcon・TPixmap 等の他の形式、`TPicture.OnChange`(TImage の OnPictureChanged で足りる)、`TGraphic.OnChange`。
  - ScanLine・RawImage 等の画素への直接のアクセス、LoadFromStream 等のストリーム、クリップボード。
  - TCustomImage の `KeepOriginXWhenClipped`・`AntialiasingMode`・`OnPaintBackground`・`DestRect`。
  - TCustomBitBtn・TCustomSpeedButton の `GlyphShowMode`・`ShowCaption` 等。

## 実装して分かったこと

- **Win32 での名前の衝突**(Pascal 側):
  - `TBitmap` は Windows ユニットの構造体(BITMAP)に隠されるため、`Graphics.TBitmap` と書く。
  - `Rect(...)` も既存のコード(グリッド)と同じく Windows ユニットの型名に隠されるため、TRect をフィールドで組み立てる。
- **TPicture の変換**: PNG を読み込んだ TPicture の `Bitmap` を操作すると、LCL が中身をビットマップに変換し、画素は引き継がれた。
- **Canvas のラッパー**: 同じ TBitmap の Canvas を 2 回取得すると、同じラッパーが返った。
- **TImage**:
  - 空の TImage の Canvas に描くと、Picture はコントロールの大きさ(50x30)のビットマップになり、描いた内容が残った。
  - `AutoSize` の TImage は、表示されていないページにあっても、フォームの表示後(OnShow)には画像の大きさ(60x40)になっていた。
  - Picture の中身を変えると OnPictureChanged が呼ばれた。
  - C のテストで、Picture の設定後に元の TPicture を Clear しても、TImage 側の画像は残った(内容のコピー)。
- **Glyph**:
  - bkOK のボタンに Glyph を設定すると、Kind は bkCustom(0)になり、Caption(`&OK`)は残った(上記の LCL の仕様)。
    既存のテストの BitBtn1(Kind の確認用)は変えず、Glyph は別のボタンで確認した。
  - 32x16 の画像を SpeedButton の Glyph にすると、NumGlyphs は 2 になった。
  - `Glyph = nullptr` で画像は空になった(Empty が true)。
- **保存と読み込み**: TBitmap から Assign した PNG・JPEG(CompressionQuality 90)を保存し、TPicture の LoadFromFile で読み込めた。
- Linux/GTK2(WSL)でも、上記の出力はすべて Win32 と同じだった。

## 影響

- VCL の画像を扱うコード(`new Graphics::TBitmap`・`Bmp->Canvas->...`・`Image1->Picture->LoadFromFile(...)`・`Image1->Picture->Graphic = Png;`・
  `BitBtn1->Glyph = Bmp;`・`Canvas->Draw(x, y, Bmp)`)を、ほぼ書き換えずに移植できる。
  ただし、次の点は書き換えが要る。
  - `dynamic_cast<TBitmap*>(Picture->Graphic)` で中身のクラスを調べるコード。
  - VCL の TPngImage(LCL では TPortableNetworkGraphic)。
  - Kind を設定したボタンに Glyph を設定し、Kind が残ることを前提にしたコード。
- 2 バッチ目の TImageList は、TBitmap を受け取る Add 等をこの基盤の上に作れる。
- 所有者から都度取得するビュー(TStrings・グラフィック)と、ハンドルを覚える値メンバ(TCanvas・TListItems 等)の使い分けは、
  「LCL が中身のオブジェクトを差し替えるか」で決める、という判断基準がはっきりした。
