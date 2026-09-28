# 0027. TStrings を表すクラスを入れ、Items・Lines・Tabs・SubItems を VCL と同じく TStrings* として公開する

- 状態: 承認
- 日付: 2026-09-27

## 背景

これまで、コントロールが持つ文字列の一覧(LCL の TStrings)は、C++ でも C API でも平たいメンバ関数で表してきた。

| 所有者 | LCL のプロパティ | これまでの C++ | これまでの C API |
|---|---|---|---|
| TCustomListBox(TListBox・TCheckListBox) | `Items` | `ItemsAdd`・`ItemsClear`・`ItemsCount`・`ItemsGetText` | `beth_TCustomListBox_Items_*` |
| TCustomComboBox | `Items` | 同上 | `beth_TCustomComboBox_Items_*` |
| TCustomRadioGroup | `Items` | 同上 | `beth_TCustomRadioGroup_Items_*` |
| TCustomCheckGroup | `Items` | 同上 | `beth_TCustomCheckGroup_Items_*` |
| TCustomMemo | `Lines` | `LinesAdd` 等 | `beth_TCustomMemo_Lines_*` |
| TTabControl | `Tabs` | `TabsAdd` 等 | `beth_TTabControl_Tabs_*` |
| TListItem | `SubItems` | `SubItemsAdd` 等(SetText を含む) | `beth_TListItem_SubItems_*` |

平たいメンバ関数には、次の問題があった。
- VCL からの移植で最もよく使う書き方(`ListBox1->Items->Add("x")`・`Memo1->Lines->Text = ...`・`Items->Strings[i]`)を書き換える必要がある。
- 使える操作が Add・Clear・Count・GetText に限られていた(Insert・Delete・IndexOf・Text・Assign 等が無い)。

LCL のソース(`include/customlistbox.inc`・`customcombobox.inc`・`custommemo.inc`・`listitem.inc`)で確認したこと:

- **中身の TStrings のオブジェクトが差し替わる**:
  - 対象は TCustomListBox・TCustomComboBox・TCustomMemo。
  - ウィンドウを作るとき(InitializeWnd)に、中身の TStrings を OS のリストを表す TStrings(ウィジェットセット側)に差し替え、内容を写して古い方を破棄する。
  - ウィンドウを破棄するとき(FinalizeWnd)には、元の形に戻す。
  - つまり、**同じコントロールでも Items のオブジェクト(ハンドル)は変わる**。テストで、表示の前後でハンドルが変わることを確認した(Win32・GTK2 の両方)。
- `TListItem.SubItems` は、初めて参照したときに作られる。
- TTabControl・TCustomRadioGroup・TCustomCheckGroup の TStrings は、生成時に作られて差し替わらない。

## 検討した選択肢

C++ の TStrings の持ち方:

- 選択肢A: TListColumns 等と同じく、中身の TStrings のハンドルを覚える値メンバにする。
  ウィンドウの生成でハンドルが変わると、破棄済みのオブジェクトを指してしまうため使えない。
- 選択肢B: ハンドルを覚えず、所有者(コントロール)と「所有者から中身を取り出す C API の関数」を持つビューにし、操作のたびに中身を取り直す。

## 決定

選択肢B を採る。

- **C API**:
  - 取得関数: 各所有者に TStrings のハンドルを返す関数を 1 つずつ置いた
    (`beth_TCustomListBox_GetItems`・`TCustomComboBox_GetItems`・`TCustomRadioGroup_GetItems`・`TCustomCheckGroup_GetItems`・
    `TCustomMemo_GetLines`・`TTabControl_GetTabs`・`TListItem_GetSubItems`)。
  - 操作: TStrings の操作は、どの所有者のものでも共通の `beth_TStrings_*`。
    GetCount・Get/SetStrings・Get/SetObjects・Add・AddObject・Insert・Delete・Clear・IndexOf・Exchange・Move・BeginUpdate・EndUpdate・
    Get/SetText・Get/SetCommaText・Assign・AddStrings。
  - **ハンドルは保存せず、使うたびに取得する**ことを、ヘッダーに明記した。
  - 旧関数: 平たい関数(`*_Items_Add` 等)は削除した。
- **C++**:
  - `TStrings`(TPersistent)を追加した。持つのは所有者の `TObject*` と取得関数だけ。
    - `Count`・`Strings[i]`・`Objects[i]`・`Text`・`CommaText` はプロパティ。
    - Add・AddObject・Insert・Delete・Clear・IndexOf・Exchange・Move・BeginUpdate・EndUpdate・Assign・AddStrings はメソッド。
  - 所有者のクラス: 各所有者は `ReadOnlyProperty<TStrings*> Items`(Lines・Tabs・SubItems)と、値メンバの `TStrings` を持つ。
    `ListBox1->Items->Add("x")`・`Memo1->Lines->Text = "..."`・`ListBox1->Items->Assign(Memo1->Lines)` のように VCL と同じく書ける。
  - `Handle()`: TStrings のビューは中身のハンドルを持たないので `nullptr` を返す。C API に渡すときは `Current()` で現在のハンドルを得る。
  - 旧メソッド: 平たいメソッド(`ItemsAdd` 等)は削除した(ADR 0022 と同じく、同じことをする書き方を 2 つにしない)。
- `Objects[i]`:
  - LCL では TObject だが、利用者データとして `void*` で表す(LCL は解釈も解放もしない)。
  - C++ の TObject とは別物なので、VCL のように `(TObject*)` で格納するコードは、`void*` への暗黙の変換でそのまま書ける。
  - 取り出すときは `(MyType*)(void*)Items->Objects[i]` のように、一度 `void*` にする。
- 見送ったもの:
  - TStringList 固有のもの: Sort・Sorted・Duplicates・CaseSensitive。
  - Names/Values/ValueFromIndex、Delimiter・DelimitedText・QuoteChar。
  - LoadFromFile/SaveToFile。
  - 単独で生成する TStringList。

## 実装して分かったこと

- **差し替えの確認**:
  - ウィンドウを作る前に覚えた ListBox1->Items の中身のハンドルは、フォームの表示後(OnShow)には別のものになっていた。
  - 内容は引き継がれており、ビュー越しの操作はそのまま動いた。Win32・GTK2 の両方で同じ。
- `CommaText` は、空白・カンマを含む要素を二重引用符で囲む(`a,"b c",d`)。
  ListBox の項目 `List 1` のような空白を含む文字列も、`"List 1"` になる。
- `Strings[i]` の値の取り出し:
  - `Strings[i]` はプロキシ(ADR 0022)なので、`printf` に渡すときは `std::string(Items->Strings[i]).c_str()` とする。
  - `Count` も `(int)Items->Count` とする(`Property<T>` と同じ)。
- Linux/GTK2(WSL)でも同じ結果になった。

## 影響

- VCL の TStrings を使うコード(`Items->Add`・`Items->Strings[i]`・`Lines->Text`・`Items->Assign` 等)を、書き換えずに移植できる。
- 今後 TStrings を持つコントロールを追加するときは、取得関数を 1 つ置き、値メンバの `TStrings` を持たせるだけでよい。
- 単独の TStringList(利用者が生成する文字列の一覧)は、必要になったら別途追加する
  (TComponent ではないので、生成と破棄の方法の設計が要る)。
