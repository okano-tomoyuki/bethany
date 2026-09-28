/**
 * コード生成 CLI(docs/designer/codegen-design.md。tk-designer の tkd から流用)。
 *
 *   beth generate <file.bfm.json> [--force] [--check] [--locale <ja|en>]
 *
 * - DSL の codegen に書かれたターゲット(C++ / Python)のコードを生成する。既存のファイルはマーカー区間だけを更新する。
 * - 手で編集された区間があれば書き込まずに失敗する(--force で上書き)。
 * - --check は書き込まず、生成結果が既存のファイルと一致するか(最新か)だけを調べる(CI 向け)。
 * - メッセージの言語は --locale、なければ環境変数(LC_ALL・LC_MESSAGES・LANG)、なければ OS の設定で決まる(tk-designer ADR 0014 と同じ)。
 */
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { basename, dirname, resolve } from 'node:path';
import { parseArgs } from 'node:util';
import { generateAll } from '@bethany-designer/codegen';
import { configureL10n, isJapanese, l10n, parseDocument } from '@bethany-designer/core';
import ja from '../../extension/l10n/bundle.l10n.ja.json' with { type: 'json' };

const USAGE = 'Usage: beth generate <file.bfm.json> [--force] [--check] [--locale <ja|en>]';

/** メッセージの言語(--locale、環境変数、OS の設定の順) */
function detectLanguage(option: string | undefined): string {
  const env = process.env;
  return (
    option ??
    (env.LC_ALL || env.LC_MESSAGES || env.LANG || undefined) ??
    Intl.DateTimeFormat().resolvedOptions().locale
  );
}

function main(argv: readonly string[]): number {
  const { positionals, values } = parseArgs({
    args: [...argv],
    allowPositionals: true,
    options: {
      force: { type: 'boolean' },
      check: { type: 'boolean' },
      locale: { type: 'string' },
    },
  });
  const language = detectLanguage(values.locale);
  configureL10n(language, isJapanese(language) ? ja : undefined);
  const [command, file] = positionals;
  if (command !== 'generate' || !file) {
    console.error(USAGE);
    return 1;
  }

  const dslPath = resolve(file);
  const { document, diagnostics } = parseDocument(readFileSync(dslPath, 'utf8'));
  for (const d of diagnostics) {
    console.error(`${file}: ${d.severity}: ${d.path.join('.') || '(root)'}: ${d.message}`);
  }
  if (!document || diagnostics.some((d) => d.severity === 'error')) return 1;

  const outPath = (path: string) => resolve(dirname(dslPath), path);
  const generated = generateAll(document, basename(dslPath), (path) => readIfExists(outPath(path)));
  if ('error' in generated) {
    console.error(`${file}: ${generated.error}`);
    return 1;
  }
  for (const warning of generated.warnings) console.error(`warning: ${warning}`);

  let status = 0;
  for (const { path, result } of generated.files) {
    if (!result.ok) {
      console.error(`${path}: ${result.error}`);
      status = 1;
    }
  }
  if (status !== 0) return status;

  const files = generated.files.flatMap(({ path, result }) =>
    result.ok ? [{ path, result }] : [],
  );
  const existing = new Map(files.map((f) => [f.path, readIfExists(outPath(f.path))]));
  const changed = files.filter((f) => existing.get(f.path) !== f.result.text);

  if (values.check) {
    for (const f of changed)
      console.error(l10n.t('{0}: not up to date (run beth generate)', f.path));
    return changed.length > 0 ? 1 : 0;
  }

  const modified = files.filter((f) => f.result.modifiedRegions.length > 0);
  if (modified.length > 0 && !values.force) {
    for (const f of modified) {
      console.error(
        l10n.t(
          '{0}: generated regions ({1}) have been edited by hand. Specify --force to overwrite them',
          f.path,
          f.result.modifiedRegions.join(', '),
        ),
      );
    }
    return 2;
  }

  for (const f of files) {
    if (!changed.includes(f)) {
      console.log(l10n.t('{0}: up to date', f.path));
      continue;
    }
    mkdirSync(dirname(outPath(f.path)), { recursive: true });
    writeFileSync(outPath(f.path), f.result.text);
    const stubs =
      f.result.addedStubs.length > 0
        ? l10n.t(' (added handler stubs: {0})', f.result.addedStubs.join(', '))
        : '';
    console.log(
      existing.get(f.path) === undefined
        ? l10n.t('{0}: created{1}', f.path, stubs)
        : l10n.t('{0}: updated{1}', f.path, stubs),
    );
  }
  return 0;
}

function readIfExists(path: string): string | undefined {
  try {
    return readFileSync(path, 'utf8');
  } catch {
    return undefined;
  }
}

process.exitCode = main(process.argv.slice(2));
