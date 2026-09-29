# 0051. TTreeView の細部(Tier B の B12)

- 状態: 承認
- 日付: 2026-09-30

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B12。TTreeView はノードの追加・選択・展開と、そのイベントだけに対応していた。
複数選択・並べ替え・ラベルの編集・独自の描画は、[component-coverage.md](../component-coverage.md) でも未対応としていた。

## 決定

### ライブラリ

- **TCustomTreeView**(LCL の public):
  - `Options`(`TTreeViewOptions`: `tvo…` のビットの集合)・`MultiSelectStyle`(`msControlSelect`・`msShiftSelect` 等のビットの集合)
  - `SelectionCount`・`Selections[i]`・`IsEditing()`
- **TTreeView**(LCL の published):
  - `MultiSelect`・`SortType`・`Indent`・`HotTrack`・`RightClickSelect`・`ToolTips`
  - イベントの `OnCompare`・`OnEditing`・`OnEdited`・`OnCustomDrawItem`
- **TTreeNode**: `DisplayRect(TextOnly)`・`EditText()`・`EndEdit(Cancel)`。
- **イベントの形**(LCL・VCL と同じ):
  - `TTVCompareEvent(Sender, Node1, Node2, int& Compare)`
  - `TTVEditingEvent(Sender, Node, bool& AllowEdit)`
  - `TTVEditedEvent(Sender, Node, std::string& S)`(S を書き換えると、それが Text になる)
  - `TTVCustomDrawItemEvent(TCustomTreeView* Sender, Node, TCustomDrawState State, bool& DefaultDraw)`
- **状態の集合**: `TCustomDrawState`(`cdsSelected` 等)は、`TOwnerDrawState` と同じくビットの集合にする。
- **TSortType**: `TSortType`(`stNone`・`stData`・`stText`・`stBoth`)は、TListView と共通の 1 つにする(これまで TListView の区間にあった)。
- **文字列の書き戻し**: 書き戻す文字列の引数(`std::string&`)は初めて。
  - DLL のコールバックは、文字列と「返す文字列を入れる場所」(`str_t*`)を受ける(`tv_edited_callback_t`)。
  - C++ のトランポリンは、スレッドごとの領域に置いた文字列を返す。
  - Python は `Ref`(`.value`)で受け、返す bytes を次の呼び出しまで持っておく(`_a_ref_str`)。

### デザイナー

- カタログを抽出し直す。`MultiSelect`・`MultiSelectStyle`・`SortType`・`Indent`・`HotTrack`・`RightClickSelect`・`ToolTips` と、4 つのイベントが出る。
- **`Options` はデザイン時に設定しない**(overlay.json の notDesignable)。ShowLines・ReadOnly・RowSelect 等の個別のプロパティと同じビットを持つため、
  両方を書くと生成するコードの順で片方が消える。個別のプロパティの無いもの(`tvoShowSeparators` 等)は、コードで設定する。

## 実装で決めたこと

- **EditText の戻り値**: LCL の `TTreeNode.EditText` は、編集を始める前の状態を返す(始めても False)。VCL と同じく、編集を始めたら true を返すよう、
  DLL は呼び出しの後の `IsEditing` を返す。
- **MultiSelect のときの Selected**: MultiSelect のとき、ノードの `Selected` に true を代入すると、選択に加わる(`Selections` に入る)。
- **テーマで描くときの色**: `Options` の `tvoThemedDraw`(既定で有効)のとき、LCL は文字をテーマの色で描くため、OnCustomDrawItem で
  `Sender->Canvas.Font.Color` を変えても既定の描画には使われない。色を変えて既定の描画に任せるときは、`tvoThemedDraw` を外す(ヘッダのコメントに書いた)。
- **Options と個別のプロパティ**: `Options` は、`RowSelect`・`HotTrack` 等の個別のプロパティと連動する(LCL)。
- **呼ばれることを確かめた**: 確認のプログラムで次を確かめた。
  - `OnCompare`: `AlphaSort` から呼ばれ、決めた順に並ぶこと。
  - `OnEditing`: false にすると編集が始まらないこと。
  - `OnEdited`: `EndEdit(false)` で呼ばれ、書き換えた文字列が Text になること(Python では日本語も)。
  - `OnCustomDrawItem`: 表示で呼ばれ、選んだノードで `cdsSelected` が付くこと。

## 影響

- イベントの引数に、書き戻す文字列が加わる(後の TListView・グリッドのラベルやセルの編集でも使う)。
