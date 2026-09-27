# 0030. Tier 3 の 2 バッチ目として、TImageList と各コントロールの Images・ImageIndex を追加する

- 状態: 承認
- 日付: 2026-09-27

## 背景

[ADR 0029](0029-graphics-picture-image-glyph.md) でグラフィックス基盤(TBitmap・TPicture)と TImage・Glyph を入れた。
Tier 3 で残っていたのは TImageList と、それを使う各コントロールの Images・ImageIndex。
Tier 2 のコントロール(TTreeView・TListView・TToolBar・THeaderControl・TCoolBar)とメニューは、
「Images/ImageIndex は Tier 3 待ち」として見送っていた。

LCL のソース(`imglist.pp`・`include/imglist.inc`・`controls.pp`・`comctrls.pp`・`menus.pp`・`buttons.pp`・`extctrls.pp`)で確認したこと:

- **継承関係**:
  - `TLCLComponent → TCustomImageList(ImgList ユニット) → TDragImageList(Controls) → TImageList`。
  - **TComponent** なので、寿命はこれまでのコンポーネントと同じ仕組み(破棄通知・Owner)で扱える。
  - TDragImageList はドラッグ中の画像の表示のためのクラスで、TImageList で新たに公開するメンバは無い(published にするだけ)。
- **画像の加え方が VCL と違う**:
  - LCL の `Add`(= `Insert`)は、画像を Width・Height の大きさに**伸縮して 1 つ**として加える。
  - VCL の Add は、幅が Width の倍数の画像を、横に並んだ複数の画像として分けて加える。
  - LCL で分けて加えるのは `AddSliced(Image, 横の数, 縦の数)`。
- **OnChange**:
  - `Change` は FChanged が立っているときだけ OnChange を呼ぶ。
  - FChanged を立てるのは Clear・Delete・Move・BkColor の変更(とストリームからの読み込み)で、**Add・Insert では呼ばれない**。
- **Images の解除**: 各コントロールは画像リストの破棄を FreeNotification で受け、Images を nil に戻す。
- **Images・ImageIndex の宣言元と公開範囲**:
  - TCustomImage・TCustomBitBtn・TCustomSpeedButton の Images・ImageIndex(public)。
  - TCustomTabControl の Images(public。TPageControl と TTabControl の共通の基底)、TCustomPage の ImageIndex(public)。
  - TCustomTreeView の Images・StateImages(public)、TTreeNode の ImageIndex・SelectedIndex・StateIndex・OverlayIndex(public)。
  - TCustomListView の LargeImages・SmallImages・StateImages は **protected** で、TListView が published にする。
    TListItem の ImageIndex・StateIndex(public)、TListColumn の ImageIndex(published)。
  - TToolBar の Images・HotImages・DisabledImages、TToolButton の ImageIndex(published)。
  - TCustomHeaderControl の Images、THeaderSection の ImageIndex(published)。
  - TCustomCoolBar の Images・Bitmap(public)、TCoolBand の ImageIndex・Bitmap(published)。
  - TMenu の Images、TMenuItem の ImageIndex・SubMenuImages・Bitmap(published)。
- **Bitmap**:
  - TCustomCoolBar・TCoolBand の Bitmap は生成時に作られ、差し替わらない(Setter は Assign)。
  - TMenuItem の Bitmap は、初めて参照したときに作られる。

## 検討した選択肢

TImageList の C++ での持ち方:

- 選択肢A: ADR 0029 のグラフィックと同じく、TPersistent として new / delete する。
  LCL では TComponent で、Owner による破棄やコントロール側の破棄通知(Images の解除)が前提になっているため、合わない。
- 選択肢B: これまでのコンポーネントと同じく、`new TImageList(Owner)` で生成し、Owner に任せるか `Free()` で破棄する。

VCL と違う Add の扱い:

- 選択肢A: C++ 側で幅を見て、Width の倍数なら AddSliced を呼ぶ(VCL の動きに寄せる)。
  LCL の Add と C API の Add で動きが変わり、同じ名前の関数の意味が層によって違ってしまう。
- 選択肢B: LCL の動きのまま公開し、分けて加えるための AddSliced を足す。違いはヘッダーと ADR に書く。

34 個のプロパティ(Images・ImageIndex・Bitmap)の追加:

- 選択肢A: これまでと同じく 4 層を手で書く。
- 選択肢B: どれも Get/Set の組で形がそろっているため、「クラス・プロパティ・種類(画像リスト・整数・TBitmap)」の定義表から 4 層を生成する。
  生成したコードは手書きのものと同じ形にし、リポジトリには生成物だけを置く(生成のスクリプトは一度きりの作業用で、置かない)。

## 決定

- **TImageList**: 選択肢B(TComponent)。
  - C API: `no_vcl_TImageList_Create`、TCustomImageList の `Get/SetWidth`・`Get/SetHeight`・`GetCount`・`Get/SetMasked`・`Get/SetBkColor`・
    `Get/SetDrawingStyle`・`Add`・`AddSliced`・`AddMasked`・`Insert`・`Replace`・`Delete`・`Clear`・`Move`・`GetBitmap`・`Draw`・
    `BeginUpdate`・`EndUpdate`・`SetOnChange`。列挙は `no_vcl_ds*`。
  - C++: `TCustomImageList`(TComponent)と `TImageList`。TDragImageList は公開するメンバが無いため省いた([ADR 0007](0007-lcl-faithful-hierarchy.md))。
    画像を受け取るメソッドは `const TCustomBitmap*`(AddMasked は LCL と同じく `const TBitmap*`)で、ADR 0029 のビュー(Picture->Bitmap 等)も渡せる。
    `Draw` は `TCanvas*` を受け取る(`ImageList1->Draw(Bmp->Canvas, x, y, i)`・`ImageList1->Draw(&PaintBox1->Canvas, ...)`)。
- **Add**: 選択肢B。LCL の動き(伸縮して 1 つ)のまま公開し、`AddSliced` を足した。
- **Images・ImageIndex・Bitmap**:
  - C API の関数名・C++ でメンバを置くクラスは、これまでどおり公開されるクラスに合わせた。
    TCustomListView で protected の LargeImages 等は、TComboBox の OnChange と同じく、公開する TListView に置いた(`no_vcl_TListView_GetSmallImages` 等)。
  - C++ の Images は `Property<TCustomImageList*>`。PopupMenu と同じく、Getter は FromHandle でラッパーを引く
    (C++ のラッパーを介さずに作られた画像リストなら nullptr)。
  - 画像リストの Getter は TComponent の FromHandle を使うため、Images を持つのはコンポーネントのクラスだけになる。
    項目(TTreeNode 等)が持つのは ImageIndex 等の整数だけで、この形は LCL と一致している。
  - Bitmap は ADR 0029 の Glyph と同じく、所有者が持つ TBitmap のビューを値メンバにした `Property<TBitmap*>`(代入は内容のコピー)。
  - 4 層は選択肢B(定義表からの生成)で作った。
- 見送ったもの:
  - ImagesWidth・ImageWidth 等の高 DPI 向けの指定、TImageList の Scaled・複数解像度。
  - AddIcon・TIcon、AddImages、Overlay、StretchDraw、GetIcon。
  - TCustomBitBtn・TCustomSpeedButton の HotImageIndex・PressedImageIndex・DisabledImageIndex・SelectedImageIndex。

## 実装して分かったこと

- **Add と AddSliced**(上記の LCL の仕様の確認):
  - 32x16 の画像を Width 16 の画像リストに Add すると、1 つの 16x16 の画像になった(Count は 1 増えた)。
  - AddSliced(画像, 2, 1) では 2 つの画像になり、それぞれの色も分かれていた(0 番が赤、1 番が青)。
  - 最初のテストでは VCL の動きを前提に Add で 2 つ加わるとしていたため、後の `Move(2, 0)` が範囲外になった。
    Win32 では LCL の例外がそのまま抜けてプロセスが終了した(終了コード 0xE0465043)。
    出力がバッファに残ったまま失われたため、Linux(WSL)で `stdbuf -oL` を付けて実行し、どこまで進んだかを確かめた。
    [component-coverage.md](../component-coverage.md) の横断的な課題(DLL 越しの例外)の実例になった。
- **OnChange**: AddSliced・AddMasked・Add では呼ばれず、Delete で呼ばれた。
- **GetBitmap・Draw**: 取り出した画像は 16x16 で、Draw で (2, 2) に描くと、画像の中の画素は緑、外の (0, 0) は描く前の白のままだった。
- **Images の解除**: TImage と TToolBar(HotImages)に設定した画像リストを Free() すると、C++ からも Images が nullptr になった。
- **Bitmap**: メニュー項目・クールバーに 12x12 の TBitmap を代入すると、それぞれの Bitmap が 12x12 になった。`Bitmap = nullptr` で空になった。
- **ImageIndex**: 設定したものは設定した値が、設定していないノードは -1(既定値)が返った。
- 警告: `-Wall -Wextra` で no_vcl.cpp を確かめ、生成したメンバの宣言順と初期化順の食い違い(-Wreorder)が無いことを確認した
  (出たのは変更前からある TCustomCheckGroup の 1 件だけ)。
- Linux/GTK2(WSL)でも、上記の出力はすべて Win32 と同じだった。

## 影響

- Tier 3 は完了した。Tier 2 で見送っていた画像(ツリー・リスト・ツールバー・ヘッダー・クールバー・メニュー)も使えるようになった。
- VCL の画像リストを使うコード(`ImageList1->Add(Bmp, nullptr)`・`TreeView1->Images = ImageList1`・`Node->ImageIndex = 0`)は、ほぼそのまま移植できる。
  ただし、横に並んだ画像を Add で分けて加えているコードは、`AddSliced` に書き換える必要がある。
- Add・Insert で OnChange が呼ばれることを前提にしたコードは、LCL では動かない。
