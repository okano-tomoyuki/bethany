# 0035. デザイナーアプリは本リポジトリ内に no_vcl 専用として作り、tk-designer からは GUI に依存しない仕組みだけを流用する

- 状態: 承認
- 日付: 2026-09-28

## 背景

[ADR 0001](0001-designer-app-bundling.md) で、tk-designer と同じ形(VS Code 拡張 + DSL + コード生成)のデザイナーアプリを
別のリポジトリで作ると決めた。Tier 1〜5 と、デザイナーで設定する共通のプロパティ([ADR 0034](0034-designer-common-properties.md))が揃い、
着手できる段階になった。着手にあたって、次のことが分かっている。

- **GUI の構造が Tk と LCL で大きく違う。**
  - 配置: Tk は pack・grid・place のジオメトリマネージャ、LCL は座標(Left/Top/Width/Height)と Align・Anchors・BorderSpacing・Constraints。
  - プロパティ: Tk は文字列のキーのオプション、LCL は型のあるプロパティ(列挙型・集合型・TFont のような入れ子のオブジェクト・
    他のコンポーネントへの参照)。
  - 構造: LCL には Owner(破棄の責任)と Parent(表示の親子)の 2 つの関係があり、TTimer・ダイアログ・メニュー・TImageList のような
    画面に出ないコンポーネントもフォームが所有する。
  - イベント: LCL のイベントは型ごとに引数が決まっている(TNotifyEvent・TCloseEvent・TKeyEvent 等)。
  このため、tk-designer の DSL・カタログ・レイアウトの処理は、そのままでは使えない。
- **デザイナーが扱う語彙の元は、本リポジトリの no_vcl.hpp にある。** py/gen_api.py は既に no_vcl.hpp を解析して、
  クラス・プロパティ・イベント・列挙型を取り出している。既定値は Python のバインディングで各クラスを生成して読める。
- **tk-designer は作り直しの途中**で、要件の多くが未実装のまま動いている。

## 検討した選択肢

- 選択肢A: ADR 0001 のとおり別のリポジトリにし、tk-designer を複製して作り替える。
  no_vcl の API の変更とデザイナーのカタログ・コード生成の更新が、別々のリポジトリのコミットになり、ずれが起きやすい。
- 選択肢B: tk-designer を複数の GUI ライブラリに対応する汎用のデザイナーにする。
  Tk と LCL の違いが大きく、共通の DSL・レイアウトの抽象化のコストが大きい。作り直しの途中の tk-designer にも影響する。
- 選択肢C: 本リポジトリ内に no_vcl 専用のデザイナーを作り、tk-designer からは GUI に依存しない仕組みだけを複製して流用する。
  DSL・カタログ・コード生成を no_vcl の API に沿って設計でき、API の変更と同じコミットで更新・検証できる。

## 決定

**選択肢C。** ADR 0001 のうち「デザイナー本体は別リポジトリとして管理する」を置き換える
(VS Code 拡張 + DSL + コード生成という形、C++Builder に揃えたオブジェクトモデルを語彙にすることは ADR 0001 のまま)。

- **置き場所**: `designer/` に置き、その中で完結する pnpm workspaces とする。C++・FPC・Python のビルドは Node に依存しない
  (デザイナーを開発するときだけ Node.js・pnpm が要る)。
  - パッケージの分け方と依存の向きは tk-designer と同じにする: `core`(DSL・検証・カタログ・編集コマンド)・`codegen`・`cli`・
    `extension`・`webview`。core と codegen はファイル入出力を持たず、文字列の入出力だけでテストできるようにする。
  - カタログの抽出は `designer/tools/catalog/` に Python で置き、py/gen_api.py の no_vcl.hpp の解析と、Python のバインディングでの
    既定値の読み取りを使う。抽出したカタログ(JSON)はコミットする。
- **設計ドキュメント**: デザイナーの仕様(DSL・コード生成・カタログ等)は `docs/designer/` に置く。ADR は本リポジトリの `docs/adr/` の連番を続ける。
- **tk-designer(2026-09-28 時点、`cc00e22`)から流用するもの**(複製して、no_vcl に合わせて直す):
  - マーカー区間の更新方式のコード生成([tk-designer ADR 0004](https://github.com/okano-tomoyuki/tk-designer))と、その実装
    (`packages/codegen/src` の region・hash・stale・comments とテスト)。
  - モノレポとツール類(pnpm・TypeScript strict・ESLint・Prettier・Vitest・esbuild・Vite。tk-designer ADR 0005)。
  - テキスト文書を唯一の正とし、編集はコマンドで表し、Undo を VS Code に委ねる編集モデル(tk-designer ADR 0006)と、
    拡張ホストのカスタムエディタ・Webview との通信の骨組み。
  - DSL のスキーマを Zod で定義し、型と JSON Schema を導出する方式(tk-designer ADR 0009)。
  - コード生成の設定を DSL に持ち、生成は利用者の明示的な操作で行う(tk-designer ADR 0010)。
  - Webview の UI 部品の方針(tk-designer ADR 0012)と、日本語・英語への対応(tk-designer ADR 0014)。
- **流用しないもの**: Tk のカタログの抽出(tk-designer ADR 0007)、pack・grid・place のレイアウトエンジンと Tk の寸法の推定
  (tk-designer ADR 0008・0013)、DSL のスキーマ、C++・Python の生成部(エミッタ)。
  生成するクラスがルートのクラスを継承すること(tk-designer ADR 0011)は、C++Builder のフォーム(`class TForm1 : public TForm`)と
  同じ形なので、流用というより本来の形として採る。
- **進める順序**:
  1. DSL 仕様・コード生成・カタログの設計(`docs/designer/`)。
  2. カタログの抽出。
  3. コード生成と CLI。**C++ を先に作り**(no_vcl の主な対象)、生成したコードを no_vcl でビルドして動くことを確かめてから Python に広げる。
  4. VS Code 拡張とデザイナーの画面(tk-designer の拡張ホスト・Webview の骨組みを土台にする)。

## 影響

- ADR 0001 は一部置換になる(別リポジトリの部分だけ)。
- 本リポジトリに Node/TypeScript のコードが加わる。`designer/` の外のビルド手順(build-windows.sh・build-linux.sh)は変わらない。
- no_vcl.hpp の変更は、カタログの再抽出を伴う。カタログが no_vcl.hpp と食い違っていないかを検査する仕組みを、カタログの抽出とあわせて用意する。
- tk-designer とはコードを共有しない(複製)。どちらかで直した仕組み(マーカー区間の更新等)は、必要に応じて手で反映する。
