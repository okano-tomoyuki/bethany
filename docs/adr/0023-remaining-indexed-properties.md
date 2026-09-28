# 0023. 残りのインデックス付きプロパティも添字で書けるようにし、読み取り専用のものは値を直接返す

- 状態: 承認
- 日付: 2026-09-27

## 背景

[ADR 0022](0022-indexed-property-proxy.md) で `IndexedProperty<T>` / `IndexedProperty2<T>` を入れ、グリッドの Cells・ColWidths・RowHeights を添字で書けるようにした。
それ以外のインデックス付きプロパティは、まだ Get/Set メソッドのままだった:

| クラス | LCL のプロパティ | これまでの C++ | 読み書き |
|---|---|---|---|
| TCustomCheckGroup・TCustomCheckListBox | `Checked[Index]` | `GetChecked(i)` / `SetChecked(i, v)` | 読み書き |
| TMenuItem | `Items[Index]`(default) | `GetItem(i)` | 読み取り専用 |
| TPageControl | `Pages[Index]` | `GetPage(i)` | 読み取り専用 |
| TTreeNode | `Items[ItemIndex]`(default) | `GetItem(i)` | LCL は書き込みもできるが、Bethany では読み取り専用 |
| TTreeNodes | `Item[Index]`(default) | `GetItem(i)` | 読み取り専用 |
| TListItems | `Item[AIndex]`(default) | `GetItem(i)` | LCL は書き込みもできるが、Bethany では読み取り専用 |
| TListColumns | `Items[AIndex]`(default) | `GetItem(i)` | LCL は書き込みもできるが、Bethany では読み取り専用 |

TTreeNode・TListItems・TListColumns の書き込みは `Assign`(内容のコピー)で、使う場面が少ないため対象にしない。

## 検討した選択肢

読み取り専用のものの表し方:

- 選択肢A: `IndexedProperty<T>` と同じく要素のプロキシを返し、代入だけをできなくする。
- 選択肢B: 添字で値そのもの(ポインタ)を返す `ReadOnlyIndexedProperty<T>` を入れる。
  代入できないので、プロキシを挟む理由が無い。

## 決定

選択肢B を採る。

- **`ReadOnlyIndexedProperty<T>`** を追加した(所有者と、添字を取る Getter の関数ポインタを持つ)。
  - `operator[]` は `T` をそのまま返す。
  - そのため、`auto` で受けても、`printf` に渡しても、`==` で比べても値として扱える
    (ADR 0022 に書いたプロキシの注意が当てはまらない)。
- 名前は LCL に揃えた(ADR 0007):
  - `Items[i]`: TMenuItem・TTreeNode・TListColumns
  - `Item[i]`: TTreeNodes・TListItems
  - `Pages[i]`: TPageControl
  - VCL(C++Builder)でも同じ名前で、`TreeView1->Items->Item[i]` や `ListView1->Items->Item[i]` と書ける。
- `Checked[i]` は読み書きできるので `IndexedProperty<bool>`(`CheckGroup1->Checked[0] = true;`)。
- `GetItem` / `GetPage` / `GetChecked` / `SetChecked` の public メソッドは削除した(ADR 0022 と同じ理由)。
  `GetFirstChild`・`GetNextSibling`・`GetFirstNode`・`GetNodeAt`・`GetItemAt` などは VCL にもあるメソッドなので残す。
- C API は変更しない。

対象外:

- `Items->Strings[i]`(TListBox・TComboBox・TRadioGroup・TCheckGroup の Items、TMemo の Lines、TTabControl の Tabs、TListItem の SubItems)。
  いずれも VCL では TStrings のメンバで、今の `ItemsGetText(i)` のような平たいメソッドを置き換えるには
  TStrings 自体を表すクラスが要る。インデックス付きプロパティとは別の課題として残す。

## 実装して分かったこと

- `bool` の `Checked[i]` はプロキシなので、`printf` の `%d` に渡すときは `(bool)` で値に変換する必要がある(`Property<bool>` と同じ)。
- ポインタを返す読み取り専用の添字は、`Node->Items[0]->Items[0]` のように続けてたどれる。
- Windows・Linux/GTK2(WSL)の両方で、C++ テストの値がこれまでの Get メソッドのときと同じになった。

## 影響

- VCL の `MenuItem->Items[i]`・`PageControl1->Pages[i]`・`TreeView1->Items->Item[i]`・`ListView1->Items->Item[i]`・
  `ListView1->Columns->Items[i]`・`CheckListBox1->Checked[i]` のコードを、書き換えずに移植できる。
- 今後追加するインデックス付きプロパティ(THeaderSections・TCoolBands の `Items[i]` 等)も、この形で表す。
- 残る課題は TStrings(`Items->Strings[i]`・`Items->Add` 等)。
