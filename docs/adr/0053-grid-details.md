# 0053. グリッドの細部(Tier B の B14 の前半)

- 状態: 承認
- 日付: 2026-09-30

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B14。グリッド(TStringGrid・TDrawGrid)が対応していたのは次だけだった。

- セルの文字列と行・列の数・幅
- 選択とそのイベント
- OnDrawCell・見出しのクリック
- 行・列の挿入・削除・移動・並べ替えのメソッド

B14 は範囲が広いため、2 つに分ける。

- **前半(この ADR)**: 編集・描画・並べ替え・通知のイベントとプロパティ、`Objects`・`Cols`・`Rows`。
- **後半(ADR 0054)**: `Columns`(TGridColumns)と、それを前提とするもの。
  - LCL では、ボタン・リスト・チェックボックスの編集欄は列の `ButtonStyle` で選ぶ。
  - そのため、セルの編集の部品(`OnSelectEditor`・`OnButtonClick`・`OnPickListSelect`)とチェックボックスの列(`OnGetCheckboxState`・`OnSetCheckboxState`)は後半で扱う。

## 決定

### ライブラリ

- **TCustomDrawGrid** に次を加える。
  - プロパティ: `AutoEdit`・`AlternateColor`・`FocusColor`・`GridLineColor`・`GridLineWidth`・`TitleFont`・`AutoFillColumns`・`ColumnClickSorts`・`SortOrder`(`TSortOrder`)・`SortColumn`(読み取り専用)
  - メソッド: `ExchangeColRow()`
  - イベント: `OnGetEditText`・`OnSetEditText`・`OnValidateEntry`・`OnPrepareCanvas`・`OnCompareCells`・`OnTopLeftChanged`・`OnHeaderSized`・`OnColRowInserted`・`OnColRowDeleted`・`OnColRowMoved`・`OnColRowExchanged`
  - このうち `AutoEdit`・`AlternateColor`・`TitleFont`・`ColumnClickSorts`・`OnValidateEntry`・`OnPrepareCanvas`・`OnCompareCells`・`OnTopLeftChanged` は、LCL では TCustomGrid の protected で、TDrawGrid・TStringGrid が published にしている。
  - 既存のグリッドのメンバと同じく、TCustomDrawGrid に置く。
- **TCustomStringGrid** に次を加える。
  - `Objects[ACol][ARow]`(`void*`。TStrings の Objects と同じく、利用者データ)
  - `Cols[i]`・`Rows[i]`(`TStrings*`)
- **イベントの形**(LCL・VCL と同じ):
  - `TGetEditEvent(Sender, ACol, ARow, std::string& Value)`
  - `TSetEditEvent(Sender, ACol, ARow, const std::string& Value)`
  - `TValidateEntryEvent(Sender, ACol, ARow, const std::string& OldValue, std::string& NewValue)`
  - `TOnPrepareCanvasEvent(Sender, ACol, ARow, TGridDrawState AState)`
  - `TOnCompareCells(Sender, ACol, ARow, BCol, BRow, int& Result)`
  - `TGridOperationEvent(Sender, bool IsColumn, int sIndex, int tIndex)`
  - OnHeaderSized は、OnHeaderClick と同じ `THdrEvent`。OnTopLeftChanged は `TNotifyEvent`。
- **Python**: 読み取り専用の文字列の引数(`const std::string&`)を、生成器が `str` で渡すようにする(`_a_str`)。

### デザイナー

- カタログを抽出し直す。次が出る。
  - 上のプロパティ(`TitleFont` は Font と同じく入れ子で編集する)
  - 上のイベント
- キャンバスのグリッドに、次を反映する。
  - `Color`・`FixedColor`・`AlternateColor`(固定行の次の行から 1 行おき)
  - `GridLineColor`・`GridLineWidth`
  - `AutoFillColumns`(固定列以外の列を幅に合わせる)
- 見本のフォームの PageControl1 に 4 枚目のページ(GridSheet)を加える。そこに TStringGrid(Grid1)を置いて照合する。
  - 設定: AlternateColor・GridLineColor・TitleFont の太字・AutoFillColumns・ColumnClickSorts
  - イベント: OnValidateEntry・OnPrepareCanvas・OnCompareCells
  - OnCompareCells は、照合のプログラムの `SortColRow` から LCL が呼ぶ。

## 実装で決めたこと

- **比較のイベントを外す**: 並べ替えの比較のイベントは、あるか無いかで LCL の動きが変わる。
  - ハンドラがあると、LCL はそれで比べ、SortOrder・SortDirection を使わない。
  - これまでは、ハンドラを nullptr(Python では None)に戻しても、DLL のブリッジが残っていた。そのため、LCL の既定の比較に戻らず、比較が常に 0 になっていた。
  - 対象は、グリッドの `OnCompareCells` と、TListView・TTreeView の `OnCompare`。
  - 空のハンドラを設定したら、DLL のイベントも外す(C++ の `SetRemovableEvent`、Python の `_REMOVABLE_EVENTS`)。
  - ほかのイベントは、あるか無いかで動きが変わらないため、これまでどおりにする。
- **OnValidateEntry の呼ばれ方**(LCL の仕様。ヘッダのコメントに書いた):
  - 別のセルへ移るときと Enter で編集を終えるときに呼ばれる。`EditorMode = false` では呼ばれない。
  - `OldValue` は、編集を始める前のセルの文字列。OnGetEditText で変えた編集欄の文字列ではない。
  - 受け付けないときは、例外を投げる。LCL が例外を表示し、編集欄に留まる。
- **Cols・Rows**: LCL は、位置ごとの TStrings をグリッドの中に作り、グリッドの破棄まで同じものを返す。
  - C++ は、位置ごとのビューをグリッドのラッパーに持ち、同じポインタを返す。
  - Python のビューは、読むたびに添字を付けて取得する(`_ViewConv` に添字を加えた)。
- **呼ばれることを確かめた**: 確認のプログラムで次を確かめた。
  - `OnGetEditText`: `EditorMode = true` で、セルの文字列を受ける。
  - `OnSetEditText`: 編集欄の文字列を変えると呼ばれる(Python では日本語も)。
  - `OnValidateEntry`: 別のセルへ移ると呼ばれ、書き換えた NewValue がセルの値になる。
  - `OnPrepareCanvas`: 描画で呼ばれ、固定セルで `gdFixed` が付く。
  - `OnCompareCells`: `SortColRow` から呼ばれ、決めた順に並ぶ。外すと既定の比較(SortOrder)に戻る。
  - `OnTopLeftChanged`: `TopRow` の変更で呼ばれる。
  - `OnColRowInserted`・`OnColRowDeleted`・`OnColRowMoved`・`OnColRowExchanged`: それぞれの操作の後に、向きと位置を受けて呼ばれる。

## 影響

- `TCustomDrawGrid` に色・線・見出しのフォントのプロパティが加わり、デザイナーで設定できる。
- TListView・TTreeView の `OnCompare` を外すと、既定の比較に戻るようになる(以前は並べ替えが効かなくなっていた)。
