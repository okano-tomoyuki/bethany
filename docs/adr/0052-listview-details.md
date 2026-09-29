# 0052. TListView の細部(Tier B の B13)

- 状態: 承認
- 日付: 2026-09-30

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B13。TListView が対応していたのは次だけだった。

- 項目・列の追加と選択
- 列見出しのクリック
- SortType による並べ替え

仮想モード(OwnerData)とラベルの編集は、[component-coverage.md](../component-coverage.md) でも未対応としていた。独自の描画にも対応していなかった。

## 決定

### ライブラリ

- **TCustomListView**(LCL の public):
  - `Canvas`
  - `OwnerData`・`HotTrack`
  - `IsEditing()`・`AlphaSort()`・`Sort()`
- **TListView**(LCL の published):
  - `ShowColumnHeaders`・`ColumnClick`・`ToolTips`・`OwnerDraw`・`AutoSort`
  - イベントの `OnCompare`・`OnData`・`OnEditing`・`OnEdited`・`OnCustomDrawItem`・`OnCustomDrawSubItem`・`OnDrawItem`
- **TListItem**: `DisplayRect(TDisplayCode)`(`drBounds`・`drIcon`・`drLabel`・`drSelectBounds`)と `EditCaption()`。
- **TListItems**: `Count` を設定できるようにする(OwnerData のときの項目の数)。
- **イベントの形**(LCL・VCL と同じ):
  - `TLVCompareEvent(Sender, Item1, Item2, int Data, int& Compare)`
  - `TLVEditingEvent(Sender, Item, bool& AllowEdit)`
  - `TLVEditedEvent(Sender, Item, std::string& AValue)`(AValue を書き換えると、それが Caption になる)
  - `TLVDataEvent(Sender, Item)`(LCL と同じく `TLVDeletedEvent` の別名)
  - `TLVCustomDrawItemEvent(TCustomListView* Sender, Item, TCustomDrawState State, bool& DefaultDraw)`
  - `TLVCustomDrawSubItemEvent(TCustomListView* Sender, Item, int SubItem, TCustomDrawState State, bool& DefaultDraw)`
  - `TLVDrawItemEvent(TCustomListView* Sender, Item, TRect ARect, TOwnerDrawState State)`
- **コールバックの形**: OnEdited・OnCustomDrawItem は TTreeView の形([ADR 0051](0051-treeview-details.md))を使う。
  OnEditing は既存の `item_allow_callback_t`、OnData は `item_callback_t` を使う。
  新しい形は OnCompare(Data を持つ)・OnCustomDrawSubItem・OnDrawItem の 3 つ。
- **範囲外**: `OnAdvancedCustomDraw…`・`OnDataFind`・`OnDataHint`・`OnDataStateChange`・`OnCustomDraw`(コントロール全体)・`AutoSortIndicator` は
  今回は扱わない。

### デザイナー

- カタログを抽出し直す。次が出る。
  - `ShowColumnHeaders`・`ColumnClick`・`ToolTips`・`OwnerDraw`・`AutoSort`・`OwnerData`・`HotTrack`
  - 7 つのイベント
- キャンバスの TListView は、これまでどおり枠だけを描く(デザイン時に列・項目を持たないため、ShowColumnHeaders 等は見た目に影響しない)。
- 見本のフォームの PageControl1 に 3 枚目のページ(ListSheet)を加える。そこに次の設定の TListView(List1)を置いて照合する。
  - vsReport・OwnerData
  - ShowColumnHeaders と AutoSort を false
  - OnData・OnCompare・OnEdited
  - OnData は、照合のプログラムが `Items->Count` を設定して項目を求めたときに、LCL から呼ばれる。

## 実装で決めたこと

- **OwnerData と Items**: LCL は OwnerData を切り替えると、Items(TListItems)を破棄して作り直す。
  - C++ のリストビューは Items のビューを値メンバで持つ。そこで OwnerData の設定の後に、新しい一覧のハンドルに付け替える(`TListItems::Rebind`)。
  - Python は Items を読むたびにハンドルを取るので、何もしなくてよい。
  - 切り替えると、それまでの項目はすべて削除される(LCL の仕様。ヘッダのコメントに書いた)。
- **OwnerData の項目**: `Items->Item[i]` は、LCL が持つ 1 つの共有の項目(TOwnerDataListItem)に i の位置を設定して返す。
  - そのため、位置が違っても同じポインタが返り、`Index`・`Caption` は最後に求めた位置のものになる。
  - `Caption` 等を読むと、LCL が OnData を呼んで内容を埋める。
- **EditCaption の戻り値**: LCL の `TListItem.EditCaption` は、OnEditing で断られても True を返す。
  TTreeNode の EditText と同じく、DLL は呼び出しの後の `IsEditing` を返す。
- **OnCompare と SortDirection**: OnCompare があると、LCL は並べ替えに SortDirection を使わない。
  - 降順にするときは、ハンドラで Compare の符号を変える(ヘッダのコメントに書いた)。
  - `AlphaSort()` も、OnCompare があればそれで並べる。
- **AlphaSort の副作用**: LCL の `AlphaSort` は、SortType を stText、SortColumn を 0、SortDirection を sdAscending に書き換えて並べる。
- **AutoSort**: 既定の true では、列見出しのクリックで SortColumn がその列になり、同じ列なら SortDirection が逆になる(SortType が stNone でないとき)。
  - 処理は OnColumnClick の後に行われる。
  - そのため、OnColumnClick で自分で SortColumn を設定すると、同じ列とみなされて降順になる。デモ(test_cpp・test_beth)は AutoSort に任せる形にした。
- **Exchange・Move の後の行の項目(LCL の不具合の回避)**: LCL の Win32 実装の `Items->Exchange`・`Move` は、コントロールの行の文字列等だけを
  書き換え、行が指す項目(LVITEM の lParam)を付け替えない。
  - Win32 の並べ替えは lParam の項目の順で行を並べる。そのため、その後に並べ替えると、表示される文字列と行の項目がずれる。
    Gamma と書かれた行をダブルクリックすると Alpha の編集が始まる、などのずれ方をする。
  - ラベルの編集を加えたデモで見つかった(前からある不具合)。
  - DLL は Exchange・Move の後に、書き換えた範囲の行の lParam を、その位置の項目に付け直す。
- **独自の描画の色**: TTreeView と違い、OnCustomDrawItem で `Sender->Canvas.Font.Color` を変えると、既定の描画がその色で描く。
  テーマの設定は要らないことを、画面の画素で確かめた。
- **呼ばれることを確かめた**: 確認のプログラムで次を確かめた。
  - `OnCompare`: `Sort()` から呼ばれ、決めた順に並ぶこと。
  - `OnEditing`: false にすると `EditCaption()` が false を返し、編集が始まらないこと。
  - `OnEdited`: フォーカスの移動で編集が終わると呼ばれ、書き換えた文字列が Caption になること(Python では日本語も)。
  - `OnCustomDrawItem`: 選んだ項目で `cdsSelected` が付くこと。
  - `OnCustomDrawSubItem`: vsReport で 2 列目(SubItem が 1)に呼ばれること。
  - `OnDrawItem`: OwnerDraw・vsReport で項目ごとに呼ばれること。
  - `OnData`: OwnerData で `Items->Item[i]` の内容を埋めること。
  - `OwnerData` を false に戻すと、通常の項目を加えられること。

## 影響

- 仮想モードで、数万行の一覧を項目を作らずに表示できる。
- `TListItems::Count` が読み書きのプロパティになる(OwnerData でなければ、設定しても何もしない)。
