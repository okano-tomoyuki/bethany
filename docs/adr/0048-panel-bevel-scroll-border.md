# 0048. パネルの縁・スクロール・コントロールの枠(Tier B の B7・B9・B17)

- 状態: 承認
- 日付: 2026-09-30

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B7・B9・B17。どれもコントロールの見た目(縁・枠・スクロールバー)に関わり、
デザイナーのキャンバスと配置の計算(子を置ける範囲)にも影響する。

- **B7**: `TPanel` の縁(`BevelOuter`・`BevelInner`・`BevelWidth`・`BevelColor`)と Caption の揃え(`Alignment`・`VerticalAlignment`・`WordWrap`)。
  これまで TPanel には追加のメンバが無かった。
- **B9**: スクロールバーの出し方(`TScrollStyle`)を TStringGrid・TDrawGrid・TTreeView・TListView に加え、TMemo の `ScrollBars`(`int`)も
  `TScrollStyle` にする。TForm・TScrollBox の `HorzScrollBar`・`VertScrollBar`(`TControlScrollBar`)と `AutoScroll`。
- **B17**: コントロールの枠(`BorderStyle`: `bsNone`・`bsSingle`)と、内側の余白(`BorderWidth`)。

## 決定

### ライブラリ

- 列挙型 `TPanelBevel`(`bvNone`・`bvLowered`・`bvRaised`・`bvSpace`)・`TVerticalAlignment`(`taAlignTop`・`taAlignBottom`・`taVerticalCenter`)・
  `TScrollStyle`(`ssNone` … `ssAutoBoth`)を加える。値の順は LCL と同じ。
- **TBorderStyle**: LCL・VCL の `TBorderStyle` は `TFormBorderStyle` の `bsNone..bsSingle` の部分範囲。C++ の列挙子の名前は重ねられないため、
  `using TBorderStyle = TFormBorderStyle;` とする(C++Builder のヘッダと同じ形)。`TFormBorderStyle` の宣言はこの区間に移す。
- **BorderStyle・BorderWidth**: LCL と同じく `TWinControl` の protected に置き、公開するクラスで `using` する(TCustomControl の OnPaint と同じ形)。
  - `BorderStyle`: TCustomEdit・TCustomListBox・TCustomComboBox・TCustomListView・TCustomControl(TPanel・TScrollBox・グリッド・
    TTreeView 等。TForm は自分の `BorderStyle`(TFormBorderStyle)を持つ)。
  - `BorderWidth`: TCustomPanel・TCustomForm・TCustomListView・TCustomTreeView。
  - DLL は TWinControl の仮想の SetBorderStyle で設定する。グリッドは自分の欄に持つため、読むときだけ TCustomGrid の `BorderStyle` を使う。
- **TCustomPanel**: `Alignment`・`VerticalAlignment`・`WordWrap`・`BevelOuter`・`BevelInner`・`BevelWidth`・`BevelColor` を加える。
- **ScrollBars**: TCustomDrawGrid・TCustomTreeView・TCustomListView に `Property<TScrollStyle> ScrollBars` を加え、TCustomMemo の `ScrollBars` を
  `int` から `TScrollStyle` にする(DLL の関数は変わらない。`Memo1->ScrollBars = ssBoth` は今までどおり書ける。整数の代入は書けなくなる)。
- **TControlScrollBar**(TPersistent。所有者の中身への非所有のラッパー。TSizeConstraints と同じ形): `Kind`・`Size`(読み取り専用)・
  `Increment`・`Page`・`Position`・`Range`・`Smooth`・`Tracking`・`Visible`・`IsScrollBarVisible()`。
- **TScrollingWinControl**: `AutoScroll`・`HorzScrollBar`・`VertScrollBar`(代入は内容のコピー)。

### デザイナー

- カタログを抽出し直し、`TControlScrollBar` を入れ子のオブジェクトにする(Object Inspector で `HorzScrollBar.Range` 等を設定できる)。
- **列挙型の一部の要素**: カタログのプロパティの型に `values` を加え(`{ "kind": "enum", "enum": "TFormBorderStyle", "values": ["bsNone", "bsSingle"] }`)、
  検証・入力・Object Inspector の選択肢はそれに従う。Python の生成器は列挙型の別名を読み、`TBorderStyle = TFormBorderStyle` を出す。
- LCL が公開していないクラスの `BorderStyle`(TUpDown・TSplitter・THeaderControl・TToolBar・TCoolBar は TCustomControl の派生)は、
  overlay.json でデザイン時に設定しないものにする。
- **古いファイル**: TMemo の `ScrollBars` は整数(`"ScrollBars": 3`)で保存されていた。読み込むときに、列挙型のプロパティの整数
  (要素の順番)を要素の名前にする(dsl/upgrade.ts。保存すると `"ssBoth"` になる)。
- **配置の計算**(クライアント領域の余白。[editor-design.md](../designer/editor-design.md) §4.3): これまでクラスごとの記録した値だったものを、
  プロパティから計算する。規則は実物の LCL で測って決め、テスト(layout/insets.test.ts)にした。
  - 枠(BorderStyle が bsSingle)は Windows の枠で 2 ピクセル。子の座標の原点も余白も 2 ずれる(TPanel・TScrollBox)。
  - TPanel の縁は、bvNone でない BevelOuter・BevelInner 1 つにつき BevelWidth。BorderWidth と同じく余白だけを広げる(原点は外側の左上のまま)。
  - フォームは BorderWidth の分だけ余白が広がる。
- **キャンバス**: TPanel は枠・外側の縁・内側の縁・BorderWidth の余白を重ねて描き、Caption を Alignment・VerticalAlignment・WordWrap に従って置く
  (BevelColor が clDefault 以外なら縁をその色で描く)。入力欄・メモ・リスト・グリッド・ツリービュー・リストビュー・スクロールボックスは、
  BorderStyle が bsNone なら枠を描かない。スクロールバーはキャンバスに描かない。

## 実装で決めたこと

- TTabSheet の `BorderWidth` は、LCL は公開しているが、Windows では子の配置に効かない(測ったところ alClient の子は (0, 0) のまま)。
  効かないものを出すと紛らわしいため、Bethany では TTabSheet で公開しない。
- TComboBox は LCL の既定が bsNone だが、Windows のコンボボックスは自分で枠を描くため、キャンバスでは BorderStyle によらず枠を描く。

## 影響

- `TCustomMemo::ScrollBars` の型が変わる(`int` を代入していたコードはコンパイルエラーになる。`ssBoth` 等に書き換える)。CHANGELOG の「変更」に書く。
- 配置の計算がプロパティを見るようになったので、TPanel の縁・枠・BorderWidth を変えると、子の配置もデザイナーで計算し直される。
