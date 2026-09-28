// CLI のビルド(ESM の 1 ファイルにまとめる)。
// 依存するパッケージのうち CommonJS のもの(@vscode/l10n など)が require('fs') を使うため、require を用意する。
import { build } from 'esbuild';

await build({
  entryPoints: ['src/main.ts'],
  bundle: true,
  platform: 'node',
  format: 'esm',
  target: 'node24',
  // jsonc-parser の UMD 版(main)は require を実行時に解決するため、まとめられる ESM 版(module)を優先する
  mainFields: ['module', 'main'],
  outfile: 'dist/cli.js',
  banner: {
    js: "import { createRequire as __nvdCreateRequire } from 'node:module'; const require = __nvdCreateRequire(import.meta.url);",
  },
});
