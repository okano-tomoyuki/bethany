# デザイナー

no_vcl 専用の GUI デザイナー(VS Code 拡張)の設計ドキュメント。フォームの定義ファイル(`*.nvform.json`)から、
no_vcl の C++(no_vcl.hpp)と Python(py/no_vcl.py)のコードを生成する。

本リポジトリ内に作る理由と、tk-designer から流用するもの・しないものは [ADR 0035](../adr/0035-designer-in-this-repository.md) を参照。
実装は [designer/](../../designer/README.md) に置く。

## 構成

| ドキュメント | 内容 | 状態 |
|---|---|---|
| [dsl-spec.md](dsl-spec.md) | フォームの定義ファイル(DSL)の仕様 | 実装済み(スキーマ・意味の検証。§10 の Q5〜Q8 は未決) |
| [codegen-design.md](codegen-design.md) | コード生成(マーカー区間の更新)の設計と生成例 | C++・Python とも実装済み(no_vcl でのビルド・実行で確認) |
| [catalog.md](catalog.md) | コンポーネントカタログ(no_vcl.hpp からの抽出・既定値・補足情報) | 初版(抽出を実装済み) |
| [editor-design.md](editor-design.md) | VS Code 拡張とデザイナーの画面(編集モデル・配置の計算・キャンバス・プロパティ)。キャンバスの方式は [ADR 0036](../adr/0036-designer-canvas.md) | 草案 |
| [development.md](development.md) | 開発の手順(拡張の起動・デバッグ。Windows で F5 が不安定なときの対処) | |

## 進める順序(ADR 0035)

1. DSL 仕様・コード生成・カタログの設計(このフォルダ)
2. カタログの抽出(`designer/tools/catalog/`、Python)
3. コード生成と CLI(C++ を先に作り、生成したコードを no_vcl でビルドして確かめてから Python に広げる)← 済み
4. VS Code 拡張とデザイナーの画面([editor-design.md](editor-design.md) §10 の順)← §10 の 3 まで済み
