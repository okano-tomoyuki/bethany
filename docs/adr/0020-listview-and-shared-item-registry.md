# 0020. Tier 2 の 3 バッチ目として TListView を追加し、TComponent ではない項目の寿命管理を共通化する

- 状態: 承認
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の Tier 2、3 バッチ目として TListView に着手した。
[ADR 0019](0019-treeview-and-non-component-items.md) で、TComponent ではない TTreeNode の寿命を削除通知で管理する型を作った。
TListView の項目(TListItem)と列(TListColumn)も TComponent ではないため、同じ型が当てはまるかを LCL のソース
(`comctrls.pp`・`include/listitem.inc`・`include/listitems.inc`・`include/customlistview.inc`)で確認した。

- **項目**: `TListItem.Destroy` は必ず `TListItems.ItemDestroying` → `TCustomListView.ItemDeleted` → `DoDeletion`(protected virtual)を通り、
  `DoDeletion` が OnDeletion を発生させる。TTreeView の `Delete` と同じく、上書きすれば削除を漏れなく受け取れる。
- **ただし順序が TTreeView と異なる。** `TCustomListView.Destroy` は `inherited Destroy`(=TComponent の破棄通知)を先に行い、
  その後で項目を破棄する。このため、リストビュー自身のラッパーが delete された後に項目の削除通知が届く。
- **列**: `TListColumn` は TCollectionItem で、列の一覧(TListColumns)は LCL がリストビューのコンストラクタの中で生成する。
  クラスを差し替えられず、列の破棄を LCL の中で受け取る手段が無い。
- `Items`・`Selected`・`ItemIndex`・`SelCount`・`Checkboxes`・`GridLines`・`MultiSelect`・`ReadOnly`・`RowSelect`・`Clear` 等は
  TCustomListView の public。`Columns`・`ViewStyle`・`HideSelection`・`SortType`/`SortColumn`/`SortDirection` と各イベントは
  TCustomListView の protected を TListView が published にしている。

## 検討した選択肢

列の寿命:

- 選択肢A: 列は TCanvas のように、取得するたびに作り直す非所有のハンドルにする。寿命管理は不要だが、項目・ノードと扱いが揃わない。
- 選択肢B: 列も項目と同じくラッパーを持ち、列を破棄する経路を no_vcl の関数(TListColumns の Delete・Clear)とリストビューの破棄に限って、
  そこで通知する。LCL の中で列が消えるのは、実質的にこれらの経路だけ。

破棄通知の仕組み:

- 選択肢C: ノード・項目・列でそれぞれ別の破棄通知とレジストリを持つ。
- 選択肢D: 「TComponent ではない項目の破棄通知」として 1 つにまとめる。ハンドルは別々のオブジェクトなので、1 つの表に混在させても衝突しない。

## 決定

選択肢B と D を採る。

- **破棄通知の共通化**: ADR 0019 の `TreeNodeFree_SetCallback` を `ItemFree_SetCallback`(C API は `no_vcl_ItemFree_SetCallback`)に改名し、
  ノード・項目・列の破棄通知に共用する。コールバックの型も `no_vcl_node_callback_t` → `no_vcl_item_callback_t`、
  `no_vcl_node_allow_callback_t` → `no_vcl_item_allow_callback_t` に改名した(ADR 0019 の直後で未公開のため、互換性は考えない)。
  C++ 側は `ItemRegistry`(`TPersistent*` の表。`Wrap<T>(handle)` で初回の取得時にラッパーを作る)にまとめ、TTreeNode もこれを使う。
  レジストリは ADR 0019 と同じく、破棄しないオブジェクトにする。
- **項目**: `TListView_Create` は `DoDeletion` を上書きした `TNoVclListView` を生成し、`inherited`(利用者の OnDeletion)の後に通知する。
- **列**: `TListColumns_Delete`・`Clear` は削除する直前に通知し、`TNoVclListView.Destroy` は最初(`inherited` の前)に残っている列を通知する。
- **TListItems・TListColumns**: TTreeNodes と同じく、リストビューの値メンバとして持ち、`ReadOnlyProperty<T*>` で公開する
  (`ListView1->Items->Add()`・`ListView1->Columns->Add()` と VCL と同じに書ける)。
- **SubItems**(TStrings)は、TComboBox の Items と同じく `SubItemsAdd`・`SubItemsGetText` 等のメンバ関数で操作する。書き換えのために `SubItemsSetText` も用意した。
- **イベント**: OnDeletion/OnItemChecked/OnColumnClick(Sender, 項目または列)は既存の項目用ブリッジに型ごとのメソッドを足し、
  OnSelectItem(Sender, Item, Selected)・OnChange(Sender, Item, Change)は、項目と整数を 1 つずつ渡す `no_vcl_item_int_callback_t` を追加した。
- 見送ったもの: 画像(LargeImages/SmallImages。Tier 3 待ち)、OwnerData(仮想リスト)、ラベルの編集、オーナードロー、OnCompare・CustomSort、
  OnChanging、グループ表示。

## 実装して分かったこと

- **LCL の TListView は、ウィンドウハンドルが無いと `Selected` の設定が効かない。** `SetSelection` は内部の FSelected を覚えるだけで項目の選択状態を変えず、
  `GetSelection` は項目の状態から探し直すため nil を返す。フォームのコンストラクタ(表示前)や、表示されていないページの上のリストビューで
  `ListView1->Selected = ...` と書くのは VCL ではごく普通のため、DLL 側で補った(ハンドルが無いときは項目の Selected も設定し、
  MultiSelect でなければ他の項目を外す。ItemIndex の設定も同じ処理を通す)。ページを表示してハンドルができた後も選択が保たれることを確認した。
- **`SortColumn` が既定の -1 のままでは、`SortType` を設定しても並べ替えない**(LCL の `Sort` の条件)。0 が Caption の列。
- 列見出しのクリックでは、LCL の AutoSort(既定で有効)が並べ替えの向きを切り替える(同じ列を続けてクリックすると降順になる)。
- Win32 の TListView は OS のリストビュー(`SysListView32`)で、列見出しは `SysHeader32`。マウスのメッセージで、行のクリック → OnSelectItem
  (前の選択が外れ、新しい項目が選ばれる)、チェックボックス → OnItemChecked、列見出し → OnColumnClick が呼ばれることを確認した。
- C 版テストで、項目 1 つと列 1 つの削除に加え、フォームの破棄時にリストビューの残りの項目 2 つ・列 2 つの破棄通知が届くことを確認した。
- Linux/GTK2(WSL)でも同じ結果になった。ただし GTK2 では、表示前に設定した選択に対して、ハンドルの生成時に OnSelectItem が呼ばれる(Win32 では呼ばれない)。

## 影響

- 一覧表示(詳細表示)の UI を VCL と同じ書き方で組めるようになった。
- TComponent ではない項目の寿命管理が `ItemRegistry` と `ItemFree_SetCallback` の 1 つの仕組みになった。今後の TStringGrid 等で
  項目のオブジェクトを扱う場合も、同じ仕組みに載せる。
- 列は、no_vcl の関数を経由せずに(LCL の内部で)削除されると通知されない。現在の API の範囲ではそのような経路は無い。
