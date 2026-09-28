/**
 * 訳を packages/extension/l10n/bundle.l10n.ja.json に追加する(docs/designer/editor-design.md の l10n)。
 *
 *   node tools/l10n/merge.mts <訳の JSON ファイル>...
 *
 * 各ファイルは { "英語の文字列": "日本語訳" } の形。既存の訳は上書きし、キーの順に並べて書き出す。
 */
import { readFileSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const bundlePath = fileURLToPath(
  new URL('../../packages/extension/l10n/bundle.l10n.ja.json', import.meta.url),
);
const bundle = JSON.parse(readFileSync(bundlePath, 'utf8')) as Record<string, string>;
for (const file of process.argv.slice(2)) {
  Object.assign(bundle, JSON.parse(readFileSync(file, 'utf8')) as Record<string, string>);
}
const sorted = Object.fromEntries(Object.entries(bundle).sort(([a], [b]) => (a < b ? -1 : 1)));
writeFileSync(bundlePath, `${JSON.stringify(sorted, null, 2)}\n`);
console.log(`${bundlePath}: ${String(Object.keys(sorted).length)} 件`);
