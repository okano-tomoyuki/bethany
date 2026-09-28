# designer

no_vcl 専用の GUI デザイナー(VS Code 拡張)。設計は [docs/designer/](../docs/designer/README.md)、方針は [ADR 0035](../docs/adr/0035-designer-in-this-repository.md)。

| パス                            | 内容                                                                                           | 状態                            |
| ------------------------------- | ---------------------------------------------------------------------------------------------- | ------------------------------- |
| `tools/catalog/`                | no_vcl.hpp からコンポーネントカタログを作る(Python。[catalog.md](../docs/designer/catalog.md)) | 実装済み                        |
| `packages/core`                 | カタログ(`src/catalog/catalog.json` は生成物をコミットする)、DSL のスキーマ(Zod)と意味の検証   | 実装済み                        |
| `packages/codegen`              | コード生成(マーカー区間の更新、C++ のエミッタ)                                                 | C++ は実装済み。Python は未作成 |
| `packages/cli`                  | コード生成の CLI(`nvd generate <file.nvform.json> [--force] [--check]`)                        | 実装済み                        |
| `packages/extension`・`webview` | VS Code 拡張とデザイナーの画面                                                                 | 未作成                          |
| `samples/`                      | 見本の DSL(テストのゴールデンファイルと、ビルドでの検証に使う)                                 |                                 |
| `tools/codegen/verify-cpp.mts`  | 生成した C++ を no_vcl と一緒にビルド・実行して確かめる                                        | 実装済み                        |

## 開発

Node.js 24 と pnpm が要る。カタログの抽出と検証には Python が要る。

```sh
cd designer
pnpm install
pnpm check                 # 型チェック・lint・書式・カタログの食い違い・テスト
pnpm build                 # CLI を packages/cli/dist/cli.js にまとめる
pnpm codegen:verify-cpp    # 見本から生成した C++ をビルド・実行して確かめる(C++ コンパイラ・CMake・Ninja と、ビルド済みの no_vcl.dll が要る)
pnpm codegen:verify-python # 見本から生成した Python を実行して確かめる(Python 3 と、ビルド済みの no_vcl.dll が要る)
node packages/cli/src/main.ts generate samples/MainForm.nvform.json   # ビルドせずに CLI を動かす
```

C++・FPC・Python のビルド(リポジトリ直下の build-windows.sh・build-linux.sh)は、このフォルダに依存しない。
