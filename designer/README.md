# designer

no_vcl 専用の GUI デザイナー(VS Code 拡張)。設計は [docs/designer/](../docs/designer/README.md)、方針は [ADR 0035](../docs/adr/0035-designer-in-this-repository.md)。

| パス                 | 内容                                                                                                             | 状態                                                                          |
| -------------------- | ---------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------- |
| `tools/catalog/`     | no_vcl.hpp からコンポーネントカタログを作る(Python。[catalog.md](../docs/designer/catalog.md))                   | 実装済み                                                                      |
| `packages/core`      | カタログ、DSL のスキーマ(Zod)・意味の検証・決まった形での書き出し、編集コマンド、拡張と Webview の間のメッセージ | 実装済み(AutoSize の見積もりは未作成)                                         |
| `packages/codegen`   | コード生成(マーカー区間の更新、C++・Python のエミッタ)                                                           | 実装済み                                                                      |
| `packages/cli`       | コード生成の CLI(`nvd generate <file.nvform.json> [--force] [--check]`)                                          | 実装済み                                                                      |
| `packages/extension` | VS Code 拡張(カスタムエディタ・診断・新しいフォーム・コード生成のコマンド・JSON Schema)                          | 実装済み                                                                      |
| `packages/webview`   | デザイナーの画面(パレット・キャンバス・構造の木・オブジェクトインスペクタ・コード生成の設定)                     | 最初の版([editor-design.md](../docs/designer/editor-design.md) §10 の 6 まで) |
| `samples/`           | 見本の DSL(テストと、ビルドでの検証に使う)                                                                       |                                                                               |
| `tools/codegen/`     | 生成した C++・Python を no_vcl でビルド・実行して確かめる                                                        | 実装済み                                                                      |
| `tools/layout/`      | 配置の見本とクライアント領域を実物の LCL で記録する(`pnpm layout:record` / `layout:check`)                       | 実装済み                                                                      |

## 開発

Node.js 24 と pnpm が要る。カタログの抽出と検証には Python が要る。

```sh
cd designer
pnpm install
pnpm check                 # 型チェック・lint・書式・カタログの食い違い・訳の漏れ・テスト
pnpm build                 # Webview・拡張・CLI をビルドする(拡張は packages/extension/dist)
pnpm codegen:verify-cpp    # 見本から生成した C++ をビルド・実行して確かめる(C++ コンパイラ・CMake・Ninja と、ビルド済みの no_vcl.dll が要る)
pnpm codegen:verify-python # 見本から生成した Python を実行して確かめる(Python 3 と、ビルド済みの no_vcl.dll が要る)
pnpm l10n:merge <訳.json>  # 訳({"英語": "日本語"})を packages/extension/l10n/bundle.l10n.ja.json に加える
node packages/cli/src/main.ts generate samples/MainForm.nvform.json   # ビルドせずに CLI を動かす
```

拡張を試すには、designer/ を VS Code で開き、実行とデバッグの「Run Extension」(F5)で開発用のホストを起動する(見本の写し .cache/playground/ が開く)。
F5 の起動が不安定なとき(Windows の js-debug の不具合)の対処は [development.md](../docs/designer/development.md)。

C++・FPC・Python のビルド(リポジトリ直下の build-windows.sh・build-linux.sh)は、このフォルダに依存しない。
