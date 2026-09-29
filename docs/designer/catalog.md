# コンポーネントカタログ

デザイナーが扱うコンポーネントの一覧と、それぞれのプロパティ・イベントの情報。DSL の検証([dsl-spec.md](dsl-spec.md) §7)、
デザイナーのパレット・プロパティエディタ、コード生成(プロパティの型・ハンドラの引数)が使う。

tk-designer は Tk を実際に動かしてオプションを抽出した(tk-designer ADR 0007)。Bethany では、型の情報は beth.hpp に揃っているので、
**beth.hpp の解析(型)と、Python のバインディングでの実測(既定値)と、手書きの補足(デザイン時の扱い)の 3 つを重ねて作る。**

## 1. 情報の出どころ

| 情報 | 出どころ |
|---|---|
| クラスの一覧と継承関係(`ancestors`。参照の型・親子の制約の照合に使う)、利用者が生成できるか(`(TComponent* AOwner)` の public のコンストラクタがあるか) | beth.hpp の解析(py/gen_api.py の `parse_hpp`) |
| プロパティの名前・型・読み取り専用か・宣言したクラス(protected を `using` で公開したものを含む) | 同上 |
| イベントの名前・型と、その型の引数(ハンドラの引数) | 同上(`using TXxxEvent = std::function<...>`) |
| 列挙型・集合型(`Set<E>` とビット集合)の要素、TColor・TCursor 等の定数 | 同上 |
| 各プロパティの既定値、パレットから置いたときの大きさ | Python のバインディングで各クラスを生成して読む(実測) |
| TForm の public なメンバの名前(`formMembers`。コンポーネント・ハンドラの名前と衝突してはならないもの) | beth.hpp の解析 |
| デザイン時に設定できるか、パレットの分類と順、子を置けるか、親子の制約、プロパティエディタの種類 | 手書きの補足(オーバーレイ) |

2026-09-28 時点で、gen_api.py の解析から、利用者が生成できるクラス 56 個(TButton 等のコントロール、TTimer・ダイアログ・TImageList・
メニュー等)、列挙型 28 個、ビット集合 9 個、`Set<E>` 1 個(TAnchors)、イベントの型 24 個が取れる。

## 2. 抽出の手順

[designer/tools/catalog/extract.py](../../designer/tools/catalog/extract.py)(Python)で、開発時に実行する。
生成したカタログは [designer/packages/core/src/catalog/catalog.json](../../designer/packages/core/src/catalog/catalog.json) にコミットする。

```sh
python designer/tools/catalog/extract.py              # 静的な抽出 + 実測 + 補足(py/beth/beth.dll が要る。build-windows.sh が写す)
python designer/tools/catalog/extract.py --no-runtime # 実測せず、既定値は今のカタログから引き継ぐ
python designer/tools/catalog/extract.py --check      # カタログが beth.hpp・overlay.json と食い違っていればエラー
```

1. **静的な抽出**: `parse_hpp` の結果から、クラス・プロパティ・イベント・列挙型を取り出す。
2. **実測**: DLL をビルドした状態で、フォームを 1 つ作り、利用者が生成できる各クラスを生成して、読める各プロパティ
   (入れ子の TFont・TSizeConstraints・TControlBorderSpacing の中と、コレクションの項目(空の項目を 1 つ加えて読む)も)の値を読む。値は Windows(Win32)のものを基準にする
   (GTK2 では既定の高さ等が違う。tk-designer ADR 0013 がキャンバスの寸法を Windows の Tk に合わせたのと同じ考え方)。
3. **補足を重ねる**: 手書きの補足(`overlay.json`)を重ねて、最終的なカタログにする。

## 3. 手書きの補足(オーバーレイ)

[designer/tools/catalog/overlay.json](../../designer/tools/catalog/overlay.json)。

- デザイン時に設定できるかは、まず型で決める(読み書きできて、添字が無く、値・列挙型・集合型・参照・入れ子のオブジェクトのいずれか。
  TStrings・コレクション(ADR 0044)は読み取り専用のプロパティでも中身を設定するので含める。画像(TBitmap*・TPicture*)は dsl-spec.md §10 Q7 まで含めない)。
  そのうえで `notDesignable`(そのクラスと派生で除外)と `designable`(除外の取り消し)を重ねる。
  例: Caption は TControl で除外し、表示するクラス(TCustomLabel・TButtonControl・TCustomPanel 等)で取り消す(VCL で Caption を
  公開しているクラスに揃える)。Parent は DSL の木で表すので除外する。
- `classes` には、利用者が生成できる全クラスを書く(無いクラス・余分なクラスがあると extract.py がエラーにする)。

| 項目 | 内容 | 例 |
|---|---|---|
| `designable` | デザイン時に設定できるプロパティ。書かないものは実行時だけのもの(DSL に書くとエラー) | TButton: Caption・Left 等は可、Canvas・Handle は不可。TPageControl の ActivePage は可(参照) |
| `category` / `order` | パレットの分類と順(Delphi の Standard・Additional・Common Controls・Dialogs・System に倣う) | TButton は Standard |
| `acceptsControls` | 子のコントロールを置けるか(LCL の ControlStyle の csAcceptsControls に当たる) | TPanel・TGroupBox・TScrollBox・TTabSheet・TForm は可、TButton・TEdit は不可 |
| `childClasses` | 子に置けるクラス(そのクラスか派生) | TPageControl は TTabSheet のみ。TToolBar は TToolButton とウィンドウを持つコントロール |
| `parentClasses` | 置ける親のクラス(そのクラスか派生) | TTabSheet は TPageControl、TToolButton は TToolBar |
| `editor` | プロパティエディタの種類(型から決まらないもの) | Lines・Items は複数行、Filter はフィルターの編集、Caption は `&` を含む文字列 |
| `displayOrder` | プロパティエディタでの並び(コード生成で設定する順でもある) | Left・Top・Width・Height を先頭に。Value は MinValue・MaxValue の後 |

## 4. カタログの形(案)

```json
{
  "classes": {
    "TButton": {
      "ancestors": ["TCustomButton", "TButtonControl", "TWinControl", "TControl", "TComponent", "TObject"],
      "kind": "control",
      "creatable": true,
      "category": "Standard",
      "acceptsControls": false,
      "defaultSize": { "width": 75, "height": 25 },
      "properties": {
        "Caption": { "type": "string", "declaredIn": "TControl", "designable": true, "default": "" },
        "Align": { "type": { "enum": "TAlign" }, "declaredIn": "TControl", "designable": true, "default": "alNone" },
        "Anchors": { "type": { "set": "TAnchorKind" }, "declaredIn": "TControl", "designable": true, "default": ["akLeft", "akTop"] },
        "Font": { "type": { "object": "TFont" }, "declaredIn": "TControl", "designable": true },
        "PopupMenu": { "type": { "ref": "TPopupMenu" }, "declaredIn": "TControl", "designable": true, "default": null }
      },
      "events": {
        "OnClick": { "type": "TNotifyEvent", "declaredIn": "TControl" }
      }
    }
  },
  "objects": {
    "TFont": { "properties": { "Name": { "type": "string" }, "Size": { "type": "int" }, "Style": { "type": { "flags": "TFontStyles" } } } }
  },
  "enums": { "TAlign": ["alNone", "alTop", "alBottom", "alLeft", "alRight", "alClient", "alCustom"] },
  "flags": { "TFontStyles": ["fsBold", "fsItalic", "fsUnderline", "fsStrikeOut"] },
  "constants": { "TColor": { "clBlack": 0, "clRed": 255 }, "TCursor": { "crDefault": 0, "crHandPoint": -21 } },
  "events": { "TNotifyEvent": [{ "name": "Sender", "type": "TObject*" }] }
}
```

値(既定値・大きさ・定数)は形を示すための例で、実際の値は実測・抽出で決まる。
継承したプロパティ・イベントは、各クラスに展開して持つ(DSL の検証・プロパティエディタで継承をたどらなくて済むように)。
デザイン時に設定できないプロパティは書き出さない。`doc` は beth.hpp のコメント(日本語)。

## 4.1 実測して分かったこと(2026-09-28、Win32)

- フォントの既定値は Name が `default`、Size が 0、Color が clDefault(LCL の既定のフォント。実際の書体・大きさは OS が決める)。
- TabOrder は Parent を設定するまで -1(Parent を設定した順に 0, 1, … が付く)。
- TShortCut の 0(割り当てなし)を LCL の ShortCutToText は `Unknown` にするため、カタログでは空文字列で表す。
- ダイアログの Title の既定値は LCL の英語の文字列(TOpenDialog は `Open existing file`)。
- 既定の大きさ: TButton 75x25、TEdit 80x23、TLabel 65x17(AutoSize)、TPanel 170x50、TForm 320x240 等。

## 5. beth.hpp との整合

beth.hpp を変えたら、カタログを抽出し直す([ADR 0035](../adr/0035-designer-in-this-repository.md) の影響)。
静的な部分(クラス・プロパティ・イベント・列挙型)は DLL が無くても抽出できるので、`extract.py --check` で抽出し直した結果と
コミットされたカタログを比べ、食い違っていればエラーにする(`designer/` の TS のパッケージを作ったら `pnpm check` から呼ぶ)。
既定値(実測)は DLL が要るため、抽出し直すのは手で行う(新しいクラス・プロパティの既定値は、実測するまで空になる)。

## 6. 未決の論点

| # | 論点 | 案 |
|---|---|---|
| C1 | `acceptsControls` を実測で取るか | 当面は手書き。LCL の ControlStyle(csAcceptsControls)を読む DLL の関数を足せば実測できるが、デザイナーのためだけの API になる |
| C2 | 既定値の環境依存 | Windows の値を基準にする(上記)。フォントの既定値(Name が `default`、Size が 0)のように、実測値が意味を持たないものは補足で扱う |
| C3 | コントロールの見た目の情報 | キャンバスに描くための情報(ボタンの枠、エディットの余白等)は、カタログではなくキャンバスの実装に持たせる(後で決める) |
