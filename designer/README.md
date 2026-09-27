# designer

no_vcl 専用の GUI デザイナー(VS Code 拡張)。設計は [docs/designer/](../docs/designer/README.md)、方針は [ADR 0035](../docs/adr/0035-designer-in-this-repository.md)。

| パス | 内容 | 状態 |
|---|---|---|
| `tools/catalog/` | no_vcl.hpp からコンポーネントカタログを作る(Python。[catalog.md](../docs/designer/catalog.md)) | 実装済み |
| `packages/core/src/catalog/catalog.json` | 生成したカタログ(コミットする) | 生成済み |
| `packages/core`・`codegen`・`cli`・`extension`・`webview` | TypeScript のパッケージ(pnpm workspaces) | 未作成 |

C++・FPC・Python のビルド(リポジトリ直下の build-windows.sh・build-linux.sh)は、このフォルダに依存しない。
