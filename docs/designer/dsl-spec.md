# DSL 仕様(formatVersion 1)

フォームの定義ファイル `*.bfm.json` の仕様。§10 の Q1〜Q4・Q9 は決定済み。残りの論点は MVP の後か、実装しながら決める。
構造は Zod のスキーマ([designer/packages/core/src/dsl/schema.ts](../../designer/packages/core/src/dsl/schema.ts))、意味の検証は
[validate.ts](../../designer/packages/core/src/dsl/validate.ts) を正とし、本書と食い違う場合は本書を直す(tk-designer ADR 0009 と同じ)。
見本は [designer/samples/MainForm.bfm.json](../../designer/samples/MainForm.bfm.json)。

## 1. 基本方針

- **Bethany の公開 API(beth.hpp)の語彙をそのまま使う。** クラス名・プロパティ名・イベント名・列挙型の要素名は C++ と同じ
  (`TButton`・`Caption`・`OnClick`・`alClient`)。C++ / Python 固有の表現は含めない。
- **Delphi・C++Builder のフォームファイル(.dfm)と同じ考え方にする。** フォームが所有するコンポーネントを名前つきで並べ、
  設定したプロパティ(既定値から変えたもの)だけを書き、イベントにはハンドラのメソッド名を書く。
  .dfm を知っている利用者が、そのまま読めることを目指す(ADR 0002)。
- **1 ファイル = 1 フォーム = 生成される 1 クラス**(TForm の派生。tk-designer ADR 0011 と同じ)。
- **Owner と Parent を分けて表す。** 生成するコンポーネントはすべてフォームが所有する(Owner = フォーム。寿命は Bethany の既存の仕組み)。
  画面上の親子関係(Parent)は `controls` の入れ子で表し、画面に出ないコンポーネント(TTimer・ダイアログ・メニュー・TImageList)は
  `components` に並べる。
- JSON で書く。拡張が保存するときは決まった形(キーの順・インデント)で書き出す(差分を最小にするため。tk-designer ADR 0006)。

## 2. 例

```json
{
  "formatVersion": 1,
  "form": {
    "name": "MainForm",
    "class": "TForm",
    "properties": { "Caption": "Sample", "Width": 400, "Height": 300, "Menu": "MainMenu1" },
    "events": { "OnCreate": "FormCreate", "OnCloseQuery": "FormCloseQuery" },
    "controls": [
      {
        "name": "NameEdit",
        "class": "TEdit",
        "properties": {
          "Left": 16, "Top": 16, "Width": 280, "Height": 23,
          "Anchors": ["akLeft", "akTop", "akRight"],
          "Hint": "Your name", "ShowHint": true
        },
        "events": { "OnChange": "NameEditChange" }
      },
      {
        "name": "OkButton",
        "class": "TButton",
        "properties": {
          "Left": 304, "Top": 16, "Width": 75, "Height": 25, "Caption": "OK",
          "Anchors": ["akTop", "akRight"],
          "Font": { "Style": ["fsBold"] }
        },
        "events": { "OnClick": "OkButtonClick" }
      },
      {
        "name": "Memo1",
        "class": "TMemo",
        "properties": {
          "Left": 16, "Top": 48, "Width": 363, "Height": 180,
          "Align": "alNone", "Anchors": ["akLeft", "akTop", "akRight", "akBottom"],
          "Lines": ["line 1", "line 2"], "ScrollBars": 3,
          "PopupMenu": "PopupMenu1"
        }
      }
    ]
  },
  "components": [
    {
      "name": "MainMenu1",
      "class": "TMainMenu",
      "design": { "left": 16, "top": 240 },
      "items": [
        {
          "name": "FileMenu",
          "properties": { "Caption": "&File" },
          "items": [
            { "name": "FileOpenItem", "properties": { "Caption": "&Open...", "ShortCut": "Ctrl+O" }, "events": { "OnClick": "FileOpenItemClick" } },
            { "name": "N1", "properties": { "Caption": "-" } },
            { "name": "FileExitItem", "properties": { "Caption": "E&xit" }, "events": { "OnClick": "FileExitItemClick" } }
          ]
        }
      ]
    },
    { "name": "PopupMenu1", "class": "TPopupMenu", "design": { "left": 56, "top": 240 }, "items": [] },
    {
      "name": "OpenDialog1",
      "class": "TOpenDialog",
      "design": { "left": 96, "top": 240 },
      "properties": { "Filter": "Text files|*.txt|All files|*.*", "Options": ["ofEnableSizing", "ofViewDetail", "ofFileMustExist"] }
    }
  ]
}
```

## 3. ドキュメント

| キー | 必須 | 型 | 説明 |
|---|---|---|---|
| `formatVersion` | ○ | `1` | フォーマットのバージョン。形式を変えたときに上げ、旧形式からの移行に使う |
| `codegen` | | オブジェクト | **使わない**(§9)。以前のコード生成の設定。読み込めるが、警告を出す |
| `form` | ○ | フォームノード | §4.1 |
| `components` | | 非ビジュアルコンポーネントの配列 | §4.3 |
| `$schema` | | 文字列 | エディタ向け。内容には影響しない |

## 4. ノード

どのノードも `name` を持ち、生成するクラスのメンバ名(C++ はポインタのメンバ、Python は属性)になる。
`name` はフォームの中で重複できず、C++ と Python の識別子として正しく(ASCII のみ)、予約語・接頭辞 `beth_`・TForm のメンバ名(カタログの
`formMembers`)・生成したコードが使う Bethany の名前(クラス・列挙型の要素・定数。C++ ではメンバ名がこれらを隠すため)と衝突してはならない。

### 4.1 フォームノード

| キー | 必須 | 型 | 説明 |
|---|---|---|---|
| `name` | ○ | 識別子 | フォームの名前(C++Builder の `Form1`)。生成するクラス名の既定値(`T` + name)と、グローバル変数の名前(§10 Q3)になる。`T` + name が Bethany の名前(`Form` → `TForm`)と衝突してはならない |
| `class` | ○ | `"TForm"` | 生成するクラスの基底クラス。今は TForm だけ(TFrame は Bethany に無い) |
| `properties` | | 名前 → 値 | §5。フォーム自身のプロパティ(生成したクラスのコンストラクタの中で設定する) |
| `events` | | 名前 → ハンドラ名 | §6 |
| `controls` | | コントロールノードの配列 | Parent がフォームのコントロール |

### 4.2 コントロールノード(TControl の派生)

| キー | 必須 | 型 | 説明 |
|---|---|---|---|
| `name` | ○ | 識別子 | |
| `class` | ○ | クラス名 | カタログにある、利用者が生成できる TControl の派生(`TButton` 等) |
| `properties` | | 名前 → 値 | §5 |
| `events` | | 名前 → ハンドラ名 | §6 |
| `controls` | | コントロールノードの配列 | Parent がこのコントロールのもの。子を持てるのは、子を置けるクラス(カタログの `acceptsControls`)だけ |

- 配列の順が生成の順になる。同じ Align の兄弟の並び(alTop が上から)や、TabOrder の既定値(追加した順)にも影響する。
- **親で子の置き方が決まるクラス**:

  | 親のクラス | 子に書けるクラス | 生成するコード |
  |---|---|---|
  | `TPageControl` | `TTabSheet` のみ | `TabSheet1->PageControl = PageControl1;`(Parent ではない) |
  | `TToolBar` | `TToolButton` と、ウィンドウを持つコントロール | `ToolButton1->Parent = ToolBar1;`(並びは配列の順) |
  | `TCoolBar` | ウィンドウを持つコントロール | Parent にすると LCL がバンドを加える。バンドの設定は §10 Q6 |

### 4.3 非ビジュアルコンポーネントノード(`components`)

| キー | 必須 | 型 | 説明 |
|---|---|---|---|
| `name` | ○ | 識別子 | |
| `class` | ○ | クラス名 | カタログにある、TControl ではないコンポーネント(`TTimer`・`TOpenDialog`・`TImageList`・`TMainMenu`・`TPopupMenu` 等) |
| `design` | | `{ "left": 整数, "top": 整数 }` | デザイナーのキャンバス上のアイコンの位置(.dfm の `DesignInfo` に当たる)。生成するコードには影響しない |
| `properties` | | 名前 → 値 | §5 |
| `events` | | 名前 → ハンドラ名 | §6 |
| `items` | | メニュー項目ノードの配列 | TMainMenu・TPopupMenu のときだけ書ける(§4.4) |

### 4.4 メニュー項目ノード(`items`)

| キー | 必須 | 型 | 説明 |
|---|---|---|---|
| `name` | ○ | 識別子 | 区切り線(Caption が `-`)にも名前を付ける(C++Builder の `N1` と同じ。デザイナーが自動で付ける) |
| `properties` | | 名前 → 値 | TMenuItem のプロパティ(Caption・ShortCut・Checked・RadioItem・GroupIndex・ImageIndex 等) |
| `events` | | 名前 → ハンドラ名 | OnClick |
| `items` | | メニュー項目ノードの配列 | サブメニュー |

`class` は書かない(常に TMenuItem)。

## 5. プロパティ

`properties` のキーは Bethany のプロパティ名、値の書き方はプロパティの型(カタログ)で決まる。
**書いたプロパティだけを生成する**(既定値のままのものは書かない)。どのクラスにどのプロパティがあるか、どれがデザイン時に設定できるかは
カタログ([catalog.md](catalog.md))で決まる。

| 型(Bethany) | 書き方 | 例 |
|---|---|---|
| `int`・`double` | 数値 | `75`、`2.5` |
| `bool` | 真偽 | `true` |
| `std::string` | 文字列 | `"&OK"` |
| 列挙型(`TAlign` 等) | 要素名の文字列 | `"alClient"` |
| 集合型(`Set<E>` の TAnchors と、ビット集合の TFontStyles・Options 等) | 要素名の配列 | `["akLeft", "akTop"]`、`["fsBold"]` |
| `TColor` | 定数名、または `"#RRGGBB"` | `"clYellow"`、`"#FF8000"`(§10 Q8) |
| `TCursor` | 定数名 | `"crHandPoint"` |
| `TShortCut` | 表記の文字列(`TextToShortCut` と同じ) | `"Ctrl+S"` |
| コンポーネントへの参照(`TPopupMenu*`・`TCustomImageList*`・`TMainMenu*` 等) | 同じフォームのコンポーネントの `name` | `"PopupMenu1"` |
| 入れ子のオブジェクト(`TFont*`・`TSizeConstraints*`・`TControlBorderSpacing*`) | そのプロパティのオブジェクト(一部だけ書ける) | `{ "Size": 12, "Style": ["fsBold"] }` |
| `TStrings*`(Items・Lines・Tabs 等) | 文字列の配列 | `["a", "b"]` |

- カタログにないプロパティ、デザイン時に設定できないもの(読み取り専用・実行時だけのもの)はエラー。
- 参照は、型が合うコンポーネントでなければならない(`PopupMenu` に TMainMenu は書けない)。
- **位置と大きさ(Left・Top・Width・Height)は、Align で寄せたコントロールにも書く。** デザイナーが配置を計算した結果を書き込む
  (.dfm と同じ)。Anchors の距離は、生成したコードで Parent・大きさを設定した時点の親の大きさで決まるため(ADR 0034)、
  親の大きさが実際の配置と合っている必要がある。
- AutoSize のコントロール(TLabel・TCheckBox・TEdit 等。カタログの AutoSize の既定値が true のもの)の大きさは、実行時に LCL が
  内容(文字列・フォント)から決める。デザイナーが書く Width・Height は見積もりになる。
- 画像(Glyph・Picture・TImageList の画像)は §10 Q7。

## 6. イベント

`events` のキーは Bethany のイベント名(`OnClick` 等)、値はハンドラのメソッド名(識別子)。

- ハンドラの引数はイベントの型(カタログ)で決まる。TNotifyEvent なら C++ は `void Button1Click(TObject* Sender)`、Python は `def Button1Click(self, Sender)`。
- 同じハンドラを複数のイベントに書ける(C++Builder と同じ)。ただし、イベントの型が同じでなければならない。
- ハンドラ名は、他のノードの `name`・予約メソッド(§9)と衝突してはならない。

## 7. 意味の検証

構造(形・型)の検証は Zod のスキーマで、次の意味の検証は TS の検証関数で行う(tk-designer ADR 0009 と同じ 2 段階)。

- `name`・ハンドラ名の重複と、識別子・予約語・基底クラスのメンバとの衝突
- クラス・プロパティ・イベントがカタログにあること、値が型に合うこと
- 参照の解決と型の一致
- 親子の制約(§4.2 の表、`acceptsControls`)
- 同じハンドラを書いたイベントの型が一致すること

診断は JSON 上の位置(パス)を持ち、拡張がファイル上の範囲に変換して問題パネルに出す。

## 8. 決まった形での書き出し

- キーの順: ドキュメントは `formatVersion`・`codegen`(残っていれば)・`form`・`components`。ノードは `name`・`class`・`design`・`properties`・`events`・`controls`/`items`。
  `properties` はカタログの順(Left・Top・Width・Height を先頭に)。
  `events` と入れ子のオブジェクト(Font 等)の中もカタログの順。カタログに無いキーは元の順のまま後ろに置く。
- `properties` とノードは 1 行に 1 つのキー(1 つのプロパティの変更が 1 行の差分になる)。それ以外で、値が単純なもの
  (文字列・数値・真偽とその配列)だけのオブジェクト・配列は、1 行(100 文字)に収まれば 1 行で書く(`events`・`Anchors`・`Font` 等)。
- インデントは 2 文字、改行は LF、末尾に改行。
- 実装は [serialize.ts](../../designer/packages/core/src/dsl/serialize.ts)。`*.bfm.json` は Prettier の対象から外している。

## 9. コード生成の設定

**フォームのファイルは、画面の設計だけを持つ。** 生成する言語とコメントの言語は、フォームが属するプロジェクトファイルの
`codegen` で決める([project-spec.md](project-spec.md) §5)。出力先とクラス名は決まった規則で決まり、設定はできない。

| 生成するもの | 決まり方(`form.name` が `MainForm`、ファイルが `MainForm.bfm.json` の場合) |
|---|---|
| クラス名 | `T` + フォームの名前(`TMainForm`) |
| C++ のヘッダ・ソース | DSL と同じフォルダの `MainForm.hpp`・`MainForm.cpp` |
| Python のファイル | DSL と同じフォルダの `MainForm.py` |

以前は `codegen`(`commentLocale`・`cpp.className`・`cpp.header` 等)をフォームのファイルに書いていた。
formatVersion は 1 のまま読み込めるが使わず、「プロジェクトファイルに移して削除する」よう警告を出す(0.2.0 で変更。利用者が少ないうちに変えた)。

## 10. 未決の論点

| # | 論点 | 案 |
|---|---|---|
| Q1 | ファイルの拡張子 | **決定(2026-09-28)**: `*.bfm.json`(Bethany のフォーム。JSON であることが分かり、`jsonValidation` で関連付けられる) |
| Q2 | 生成するクラス名の既定値 | **決定(2026-09-28)**: フォームの `name` から(`MainForm` → `TMainForm`)。C++Builder の `Form1` → `TForm1` と同じ。tk-designer はファイル名から決めていた |
| Q3 | フォームのグローバル変数 | **決定(2026-09-28、2026-09-28 に Python を見直し)**: C++Builder と同じく、C++ は `extern TMainForm* MainForm;`(ヘッダ)と定義(ソース)を生成し、`Application->CreateForm(&MainForm)` で使えるようにする。Python もフォームのモジュールの末尾に `MainForm = None` を生成し、プロジェクトの起動部分が `MainForm.MainForm = Application.CreateForm(MainForm.TMainForm)` で代入する(C++Builder の使用感をそろえるため。当初は生成しない決定だった。[project-spec.md](project-spec.md) §5) |
| Q4 | 非ビジュアルコンポーネントを `components` に分けるか | **決定(2026-09-28)**: 分ける(案のとおり)。.dfm は 1 つの木に混ぜるが、Parent を持たないものを `controls` に混ぜると、親子の制約の検証が複雑になる |
| Q5 | Owner がフォーム以外のコンポーネント | 扱わない(すべてフォームが所有)。C++Builder のデザイナーも同じ |
| Q6 | コレクション(TListView の Columns、THeaderControl の Sections、TCoolBar の Bands、TTreeView の Items、TStatusBar は SimpleText のみ) | MVP の後。`items` と同じく配列のプロパティとして足す(形は個別に決める) |
| Q7 | 画像(Glyph・Picture・TImageList の画像) | MVP の後。.dfm のように埋め込むか、ファイルのパスを書いて生成コードで読み込むか(実行時のパスの扱い)を決める |
| Q8 | TColor の書き方 | 定数名(`clRed`・`clDefault` 等)と `"#RRGGBB"`。Bethany の TColor は `$00BBGGRR` なので、生成時に変換する。システム色(clBtnFace 等)は Bethany に定数が無いので、足すかどうかも決める |
| Q9 | フォームの大きさ | **決定(2026-09-28、実測)**: Width・Height で書く。LCL の TForm の Width・Height は**クライアント領域の大きさ**(枠・タイトルバー・メニューを含まない。Delphi と違う。codegen-design.md §7.1)なので、キャンバスではこの大きさをそのままクライアント領域として描き、枠・タイトルバー・メニューはその外に描く |
