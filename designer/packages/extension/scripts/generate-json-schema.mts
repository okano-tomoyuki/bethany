/**
 * core のスキーマから JSON Schema を生成する(tk-designer ADR 0009。docs/designer/editor-design.md §8)。
 * フォーム(schema/bfm.schema.json)とプロジェクト(schema/bfproj.schema.json)。
 *   node scripts/generate-json-schema.mts          … 生成して書き込む
 *   node scripts/generate-json-schema.mts --check  … コミット済みのファイルが最新か検査する
 */
import { readFile, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { documentJsonSchema, projectJsonSchema } from '@bethany-designer/core';

const schemas = [
  { file: 'bfm.schema.json', schema: documentJsonSchema() },
  { file: 'bfproj.schema.json', schema: projectJsonSchema() },
];

for (const { file, schema } of schemas) {
  const outFile = new URL(`../schema/${file}`, import.meta.url);
  const generated = `${JSON.stringify(schema, null, 2)}\n`;
  if (process.argv.includes('--check')) {
    const current = await readFile(outFile, 'utf8').catch(() => '');
    if (current.replace(/\r\n/g, '\n') !== generated) {
      console.error(
        `schema/${file} が最新ではありません。\`pnpm generate:schema\` を実行してください。`,
      );
      process.exitCode = 1;
    }
  } else {
    await writeFile(outFile, generated);
    console.log(`生成しました: ${fileURLToPath(outFile)}`);
  }
}
