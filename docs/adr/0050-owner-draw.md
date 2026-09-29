# 0050. オーナードロー(Tier B の B8)

- 状態: 承認
- 日付: 2026-09-30

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B8。リストボックス・コンボボックス・メニューの項目を、プログラムで描く(色の見本・アイコン付きの
一覧・独自の見た目のメニュー等)。これまでは TComboBox の `Style` に `csOwnerDraw…` があるだけで、描くためのイベントが無かった。

## 決定

### ライブラリ

- `TListBoxStyle`(`lbStandard`・`lbOwnerDrawFixed`・`lbOwnerDrawVariable`・`lbVirtual`)を加える。
- **状態の集合**: `TOwnerDrawState`(LCL の LCLType の集合)は、グリッドの `TGridDrawState` と同じく整数のビットの集合にする
  (`State & odSelected`)。要素の順は LCL と同じ(`odSelected` … `odBackgroundPainted`)。
- **TCustomListBox**: `Style`・`ItemHeight`・`Canvas`・`OnDrawItem`・`OnMeasureItem` を加える。
  `Canvas` は TStatusBar と同じく、コントロールが所有する実体への非所有のビュー。
- **TCustomComboBox**: `ItemHeight`・`Canvas`・`OnDrawItem`・`OnMeasureItem` を加える。`Style` は既にある(`csOwnerDrawFixed` 等)。
- イベントの型は LCL・VCL と同じ形にする。
  - `TDrawItemEvent(TWinControl* Control, int Index, TRect ARect, TOwnerDrawState State)`
  - `TMeasureItemEvent(TWinControl* Control, int Index, int& AHeight)`
- **メニュー**: `TMenu::OwnerDraw` と、`TMenuItem::OnDrawItem`・`OnMeasureItem` を加える。
  - `TMenuDrawItemEvent(TObject* Sender, TCanvas* ACanvas, TRect ARect, TOwnerDrawState AState)`
  - `TMenuMeasureItemEvent(TObject* Sender, TCanvas* ACanvas, int& AWidth, int& AHeight)`
  - `ACanvas` は LCL がその呼び出しのために渡すもので、呼び出しの間だけのラッパーで包む(保存しないこと)。
  - TMenu の `OnDrawItem`・`OnMeasureItem`(メニュー全体で 1 つのハンドラ)は対象外。項目ごとのものを使う。
- **色の定数**: 描くときに選択の色などが要るため、次を加える。どれも LCL・VCL と同じ値。
  - 標準の 16 色(`clMaroon`・`clNavy`・`clGray`・`clSilver`・`clLime` 等)と、LCL の追加の 4 色(`clMoneyGreen`・`clSkyBlue`・`clCream`・`clMedGray`)。
  - システムの色(`clHighlight`・`clHighlightText`・`clWindow`・`clWindowText`・`clBtnFace` 等)。値は 0x80000000 に Windows の COLOR_… の番号を足したもので、
    TColor は符号付きのため負の数で書く。
  - LCL の別名(`clLtGray`・`clDkGray` 等)は加えない(値から名前を決めるときに曖昧になるため)。
- DLL のコールバックの型を 4 つ加える(`draw_item_callback_t`・`measure_item_callback_t`・`menu_draw_callback_t`・`menu_measure_callback_t`)。
  状態は整数のビットで渡す。Python の生成器は、イベントの引数の TCanvas を呼び出しの間だけのラッパー(`_a_obj`)で渡す。

### デザイナー

- カタログを抽出し直す。リストボックスの `Style`・`ItemHeight`、コンボボックスの `ItemHeight`、メニューの `OwnerDraw` と、
  `OnDrawItem`・`OnMeasureItem` のイベントが出る。
- キャンバスは、オーナードローのリストボックスで `ItemHeight` があれば、その高さで項目を並べる(中身は実行時に OnDrawItem が描く)。
- システムの色は、キャンバスでは Windows 11 の既定の色で描く。

## 実装で決めたこと

- **既定値の誤りが直った**: これまでカタログの TTreeView・グリッドの `Color`・`FixedColor`、TSpeedButton の `Color` の既定値が `"#050000"` のような
  誤った値になっていた(システムの色を RGB として読んでいた)。システムの色の定数を加えたことで、`"clWindow"`・`"clBtnFace"` になった。
- **区切りのコメントを説明に入れない**: カタログの抽出は、ヘッダの区切りのコメント(`// ---- docs/adr/0049 ----`)をプロパティの説明に含めない。
- **呼ばれることを確かめた**: 確認のプログラムで次を確かめた。
  - リストボックスの `OnDrawItem`: 項目の矩形の高さが ItemHeight であること、選択した項目で `odSelected` が付くこと。
  - `lbOwnerDrawVariable` の `OnMeasureItem`: 決めた高さが項目の矩形になること。
  - コンボボックス: 入力欄の部分を描くときに `odComboBoxEdit` が付くこと。
  - メニューバーの項目の `OnMeasureItem`・`OnDrawItem`: メニューを表示するだけで呼ばれること。

## 影響

- TColor の定数が増える(Object Inspector の色の候補にも出る)。
