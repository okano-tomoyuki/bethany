# 0054. グリッドの列(Tier B の B14 の後半)

- 状態: 承認
- 日付: 2026-09-30

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B14 の後半。前半([ADR 0053](0053-grid-details.md))では、編集・描画・並べ替え・通知のイベントとプロパティを加えた。

後半は `Columns`(TGridColumns)と、それを前提とするものを扱う。

- LCL では、列ごとの見出し・幅・配置・編集欄の種類を `Columns` の列(TGridColumn)で決める。
- ボタン・一覧・チェックボックスの編集欄は、列の `ButtonStyle` で選ぶ。
- そのため、セルの編集の部品(`OnSelectEditor`・`OnButtonClick`・`OnPickListSelect`)とチェックボックスの列(`OnGetCheckboxState`・`OnSetCheckboxState`・`OnCheckboxToggled`)は、`Columns` と一緒に扱う。

## 決定

### ライブラリ

- **TGridColumns**(TCollection のビュー。グリッドの値メンバ):
  - `Count`・`VisibleCount`・`Items[i]`
  - `Add()`・`Delete()`・`Clear()`・`BeginUpdate()`・`EndUpdate()`・`ColumnByTitle()`
- **TGridColumn**(TCollectionItem):
  - `Title`(TGridColumnTitle)・`Width`・`Alignment`・`ButtonStyle`(`TColumnButtonStyle`)・`PickList`(`TStrings*`)・`ReadOnly`・`Visible`・`Color`・`Font`・`Layout`
  - `MinSize`・`MaxSize`・`SizePriority`(`AutoFillColumns` のとき)・`DropDownRows`・`ValueChecked`・`ValueUnchecked`・`Index`
  - ステータスバーのパネル([ADR 0044](0044-statusbar-panels-and-designer-collections.md))と同じく、同じ列には常に同じポインタを返す。列が破棄されると(`Delete`・`Clear`・グリッドの破棄)、ラッパーも delete する。
- **TGridColumnTitle**: `Caption`・`Alignment`・`Layout`・`Color`・`MultiLine`・`Font`。列が所有し、列と同じ寿命のビュー。
- **TCustomDrawGrid** に次を加える。
  - `Columns`・`SelectedColumn`
  - イベント: `OnSelectEditor`・`OnButtonClick`・`OnPickListSelect`・`OnGetCheckboxState`・`OnSetCheckboxState`・`OnCheckboxToggled`
  - LCL では、`OnSelectEditor`・`OnButtonClick`・`OnPickListSelect`・`OnCheckboxToggled` は TCustomGrid の protected、`OnGetCheckboxState`・`OnSetCheckboxState` は TCustomDrawGrid の protected で、TDrawGrid・TStringGrid が published にしている。前半と同じく TCustomDrawGrid に置く。
- **イベントの形**(LCL と同じ):
  - `TSelectEditorEvent(Sender, ACol, ARow, TWinControl*& Editor)`
  - `TGetCheckboxStateEvent(Sender, ACol, ARow, TCheckBoxState& Value)`
  - `TSetCheckboxStateEvent(Sender, ACol, ARow, TCheckBoxState Value)`
  - `TToggledCheckboxEvent(Sender, ACol, ARow, TCheckBoxState AState)`
  - OnButtonClick は OnSelectCell と同じ `TOnSelectEvent`。OnPickListSelect は `TNotifyEvent`。
- **Python**: コンポーネントのポインタの参照の引数(`TWinControl*&`)を、生成器が `Ref` で渡すようにする(`_a_ref_comp`)。

### デザイナー

- カタログを抽出し直す。次が出る。
  - `Columns`(コレクション)。列の `Title` は入れ子のオブジェクト、`PickList` は文字列の一覧
  - 上のイベント
- 読み取り専用の入れ子のオブジェクト(列の `Title`)も、中のプロパティを設定できるものとして抽出する。
- Object Inspector のコレクションの編集で、項目の入れ子のオブジェクトと文字列の一覧を編集できるようにする。
- コード生成は、項目の入れ子のプロパティ(`item->Title->Caption = ...`)と文字列の一覧(`item->PickList->Add(...)`)を書く。
- キャンバスのグリッドに、列の幅と見出しの文字列、`TitleFont` の太字・斜体を反映する。
- 見本のフォームの Grid1 に 3 つの列を加えて照合する。
  - 列: Name(幅 120)・Color(`cbsPickList` と PickList)・Done(`cbsCheckboxColumn` と `ValueChecked`・`ValueUnchecked`)
  - イベント: OnCheckboxToggled
  - `ColCount` は列の数で決まるので、見本から外した。

## 実装で決めたこと

- **ColCount と VisibleCount**(LCL の仕様。ヘッダのコメントに書いた):
  - `Columns` に列があると、`ColCount` は `FixedCols + Columns->Count` になる。
  - `Visible` が false の列も幅 0 の列として数える。そのため `VisibleCount` は `Count` と同じ。
- **チェックボックスの列**:
  - クリック・スペースで切り替えるには、グリッドの `Options` に `goEditing` が要る(ヘッダのコメントに書いた)。
  - TStringGrid では、`OnGetCheckboxState`・`OnSetCheckboxState` が無ければ、セルの文字列と `ValueChecked`・`ValueUnchecked` で状態を決める。
  - このため、この 2 つのイベントは、前半の比較のイベントと同じく、空のハンドラを設定したら DLL のイベントも外す(C++ の `SetRemovableEvent`、Python の `_REMOVABLE_EVENTS`)。
- **OnSelectEditor の Editor**:
  - 既定の編集欄は LCL の内部のコントロールで、ラッパーが無い。その場合は nullptr(Python では None)を渡す。
  - ハンドラが Editor を書き換えたときだけ、DLL に書き戻す。書き換えなければ既定の編集欄のまま。
- **呼ばれることを確かめた**: 確認のプログラムで次を確かめた(C++ と Python)。
  - 列の追加・削除・`Clear`・`Index` の変更、`Title`・`Font`・`PickList` の設定と読み出し、`ColumnByTitle`
  - `OnGetCheckboxState`・`OnSetCheckboxState`・`OnCheckboxToggled`: チェックボックスの列のセルの切り替えで呼ばれる。外すと、セルの文字列で決める動きに戻る。
  - `OnSelectEditor`: 編集を始めるときに呼ばれ、渡したコントロールが編集欄になる。
  - `OnButtonClick`・`OnPickListSelect` はマウスの操作が要るため、デモ(`test/main.cpp`・`py/test_beth.py` の Columns のページ)を手で操作して確かめる。

## 影響

- グリッドの列ごとに見出し・幅・編集欄の種類を決められ、デザイナーでも設定できる。
- デザイナーのコレクションの項目が、入れ子のオブジェクトと文字列の一覧を持てるようになる(ほかのコレクションにも使える)。
