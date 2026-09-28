// CLI のビルド(ESM の 1 ファイルにまとめる)。
// 依存するパッケージのうち CommonJS のもの(@vscode/l10n など)が require('fs') を使うため、require を用意する。
import { build } from 'esbuild';

await build({
  entryPoints: ['src/main.ts'],
  bundle: true,
  platform: 'node',
  format: 'esm',
  target: 'node24',
  outfile: 'dist/cli.js',
  banner: {
    js: "import { createRequire as __nvdCreateRequire } from 'node:module'; const require = __nvdCreateRequire(import.meta.url);",
  },
});
