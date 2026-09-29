# 0044. ステータスバーのパネルと、デザイナーのコレクション(Tier B の B1)

- 状態: 承認
- 日付: 2026-09-29

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B1。C++Builder のアプリでは、ステータスバーを区画(パネル)に分けて、
位置・状態・ヒントを表示することが多い。これまでの TStatusBar は `SimpleText`・`SimplePanel` だけだった。

パネルは TCollection(`TStatusPanels`)の項目で、デザイナーはこれまでコレクションを扱えなかった(C++Builder では、
Object Inspector の Panels から開くコレクションのエディタで設定する)。

## 決定

### ライブラリ

- `TStatusPanel`(`Text`・`Width`・`Alignment`・`Bevel`(`TStatusPanelBevel`)・`Style`(`TStatusPanelStyle`)・`Index`)と
  `TStatusPanels`(`Count`・`Items[i]`・`Add`・`Insert`・`Delete`・`Clear`・`BeginUpdate`・`EndUpdate`)を加える。
  作りは THeaderSection・THeaderSections と同じ([ADR 0024](0024-headercontrol.md)・[0026](0026-coolbar-and-item-free-observer.md)):
  項目は TComponent ではないため、DLL がハンドルを渡すときに観察者を付け(WatchItem)、破棄を知らせてラッパーを delete する。
- TStatusBar に `Panels`・`SizeGrip`・`AutoHint`・`Canvas`・`OnDrawPanel`(`TDrawPanelEvent`: StatusBar・Panel・`const TRect&`)・`OnHint`・
  `GetPanelIndexAt(X, Y)`・`BeginUpdate`・`EndUpdate` を加える。`OnDrawPanel` のために、DLL のコールバックの型 `item_rect_callback_t`
  (項目と矩形)を加える。
- `AutoHint` の `OnHint` の中でヒントを読めるよう、`Application->Hint` を加える。
  **LCL は、マウスの下のコントロール(か親)の `ShowHint` が true のときだけ `Application.Hint` を設定する**(`GetHintControl`)。
  VCL は `ShowHint` によらずステータスバーにヒントを出すので、ステータスバーにだけヒントを出したいコントロールにも `ShowHint` を設定する
  (ポップアップのヒントも出る。出したくなければ `Application->ShowHint` ではなく、フォームの `ShowHint` 等で範囲を決める)。
- **LCL の `SimplePanel` の既定は true**(VCL は false)。パネルを表示するには `SimplePanel` を false にする。VCL に合わせて既定を変えることはしない
  (デザイナーのカタログは実測の既定値を使い、フォームのファイルには既定と違う値だけを書くため、LCL の既定のままのほうが一貫する)。
- Python では、イベントの引数の `const TRect&` を値と同じに扱う(gen_api.py)。

### デザイナー: コレクション

- カタログの型に `{ "kind": "collection", "item": "TStatusPanel" }` を加える。項目のプロパティ(と実測の既定値)は、入れ子のオブジェクトと同じく
  `objects` に載せる。どの一覧のクラスをコレクションとして出すかは extract.py の `COLLECTION_CLASSES` で決める(今は TStatusPanels だけ)。
  `Index` は配列の並びで決まるので載せない。項目の既定値は、生成したステータスバーに空の項目を 1 つ加えて読む。
- フォームのファイルでは、項目のオブジェクトの配列で書く(既定と同じプロパティは書かない。項目の中もカタログの順)。

  ```json
  "Panels": [
    { "Text": "Ready", "Width": 120 },
    { "Width": 60, "Style": "psOwnerDraw" },
    { "Text": "right", "Alignment": "taRightJustify" }
  ]
  ```

- コード生成は、項目ごとに `Add()` して項目のプロパティを設定する。
  C++ は `{ TStatusPanel* item = StatusBar1->Panels->Add(); item->Text = "Ready"; }`(波括弧の中の局所変数)、
  Python は `item = self.StatusBar1.Panels.Add()` の後に `item.Text = "Ready"`。
- Object Inspector では、Panels を開くと項目の一覧が出る。項目の追加・削除・上下の移動と、開いた項目のプロパティの編集ができる。
  どの操作も配列全体を 1 つのプロパティの値として設定する(元に戻すと 1 操作ずつ戻る)。複数選択で値が違うときは編集しない。
- キャンバスは、`SimplePanel` が false ならパネルを並べて描く(最後のパネルは残りの幅いっぱい。`Bevel`・`Alignment` を反映し、
  `psOwnerDraw` のパネルには「OnDrawPanel」と出す)。
- イベントのハンドラの最初の引数は、ほかのイベントと同じく `TObject* Sender` にする(`OnDrawPanel` の StatusBar も Sender で受ける)。
  描画はフォームのメンバの `StatusBar1->Canvas` に行う。C++ のヘッダに書くハンドラの宣言では、`const TRect&` のような const の付いた型にも
  `beth::` を付ける。
- 見本(designer/samples/MainForm.bfm.json)にパネルを持つステータスバーを加え、verify-cpp・verify-python で、生成したコードがパネルを作ることを確かめる。

### 照合の C++ のビルドに manifest を埋め込む

verify-cpp のビルド(`beth::beth` を使わずに組んでいる)は Common-Controls 6.0 の manifest を埋め込んでいなかったため、comctl32 v5 で動き、
TStatusBar の高さが実際のアプリ・python.exe(24)と違った(22)。`beth::beth` をリンクした exe と同じ manifest を埋め込むようにした。

## 影響

- 同じ仕組みで、THeaderControl の `Sections`・TListView の `Columns`・TCoolBar の `Bands` も、`COLLECTION_CLASSES` に加えればデザイナーで
  設定できるようになる(項目のプロパティがすべて値・列挙型なら、コード生成・Object Inspector はそのまま使える。キャンバスの描画は別に要る)。
  TCoolBand の `Control` のような、項目からコントロールへの参照は、コード生成の順(参照先の親が決まった後)を考える必要がある。
- B14(グリッドの `Columns`)もこの仕組みに載せる。
