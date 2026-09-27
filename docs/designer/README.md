# デザイナー

no_vcl 専用の GUI デザイナー(VS Code 拡張)の設計ドキュメント。フォームの定義ファイル(`*.nvform.json`)から、
no_vcl の C++(no_vcl.hpp)と Python(py/no_vcl.py)のコードを生成する。

本リポジトリ内に作る理由と、tk-designer から流用するもの・しないものは [ADR 0035](../adr/0035-designer-in-this-repository.md) を参照。
実装は `designer/`(未作成)に置く。

## 構成

| ドキュメント | 内容 | 状態 |
|---|---|---|
| [dsl-spec.md](dsl-spec.md) | フォームの定義ファイル(DSL)の仕様 | 草案(§10 の Q1〜Q4 は決定済み) |
| [codegen-design.md](codegen-design.md) | コード生成(マーカー区間の更新)の設計と生成例 | 草案 |
| [catalog.md](catalog.md) | コンポーネントカタログ(no_vcl.hpp からの抽出・既定値・補足情報) | 草案 |

## 進める順序(ADR 0035)

1. DSL 仕様・コード生成・カタログの設計(このフォルダ)
2. カタログの抽出(`designer/tools/catalog/`、Python)
3. コード生成と CLI(C++ を先に作り、生成したコードを no_vcl でビルドして確かめてから Python に広げる)
4. VS Code 拡張とデザイナーの画面
