/**
 * 多言語対応の検査(docs/designer/editor-design.md の l10n)。
 *
 *   node tools/l10n/check.mts
 *
 * 1. l10n.t / vscode.l10n.t に渡した文字列(英語)すべてに日本語訳があり、使われていない訳がないこと。
 *    訳の {0} などの埋め込み位置が元の文字列と一致すること。
 * 2. package.json の %key% すべてが package.nls.json と package.nls.ja.json にあること。
 * 3. テスト以外のコードの文字列リテラル・JSX のテキストに日本語を直接書いていないこと。
 *    意図したもの(生成コードのコメントの定型文など)は、その行か直前の行に `l10n-ignore` と書く。
 */
import { readdirSync, readFileSync } from 'node:fs';
import { join, relative } from 'node:path';
import { fileURLToPath } from 'node:url';
import ts from 'typescript';

const root = fileURLToPath(new URL('../../', import.meta.url));
const extensionDir = join(root, 'packages/extension');
const SOURCE_DIRS = ['core', 'codegen', 'cli', 'extension', 'webview'].map((p) =>
  join(root, 'packages', p, 'src'),
);
const JAPANESE = /[぀-ヿ㐀-鿿＀-￯]/;

const problems: string[] = [];
const keys = new Map<string, string>(); // 文字列 → 最初に使われた場所

for (const file of SOURCE_DIRS.flatMap(sourceFiles)) checkFile(file);
checkBundle();
checkPackageNls();

if (problems.length > 0) {
  for (const p of problems) console.error(p);
  console.error(
    `\n多言語対応の検査で ${String(problems.length)} 件の問題があります(docs/designer/editor-design.md の l10n)`,
  );
  process.exitCode = 1;
} else {
  console.log(`多言語対応の検査: 問題なし(${String(keys.size)} 個の文字列)`);
}

function sourceFiles(dir: string): string[] {
  return readdirSync(dir, { withFileTypes: true }).flatMap((entry) => {
    const path = join(dir, entry.name);
    if (entry.isDirectory()) return entry.name.startsWith('__') ? [] : sourceFiles(path);
    if (!/\.tsx?$/.test(entry.name) || /\.test\.tsx?$/.test(entry.name)) return [];
    return [path];
  });
}

function checkFile(path: string): void {
  const text = readFileSync(path, 'utf8');
  const source = ts.createSourceFile(path, text, ts.ScriptTarget.Latest, true);
  const lines = text.split('\n');
  const where = (node: ts.Node) => {
    const { line } = source.getLineAndCharacterOfPosition(node.getStart(source));
    return { line, label: `${relative(root, path)}:${String(line + 1)}` };
  };
  const ignored = (line: number) =>
    (lines[line] ?? '').includes('l10n-ignore') || (lines[line - 1] ?? '').includes('l10n-ignore');

  const visit = (node: ts.Node): void => {
    if (ts.isCallExpression(node) && isL10nCall(node.expression)) {
      const [first] = node.arguments;
      if (first && (ts.isStringLiteral(first) || ts.isNoSubstitutionTemplateLiteral(first))) {
        if (!keys.has(first.text)) keys.set(first.text, where(node).label);
      } else {
        problems.push(`${where(node).label}: l10n.t の最初の引数は文字列リテラルにする`);
      }
    }
    const literal =
      ts.isStringLiteral(node) ||
      ts.isNoSubstitutionTemplateLiteral(node) ||
      ts.isTemplateHead(node) ||
      ts.isTemplateMiddle(node) ||
      ts.isTemplateTail(node) ||
      ts.isJsxText(node);
    if (literal && JAPANESE.test(node.text)) {
      const { line, label } = where(node);
      if (!ignored(line))
        problems.push(`${label}: 日本語を直接書いている: ${node.text.trim().slice(0, 40)}`);
    }
    ts.forEachChild(node, visit);
  };
  visit(source);
}

/** l10n.t(...) または vscode.l10n.t(...) */
function isL10nCall(expression: ts.Expression): boolean {
  if (!ts.isPropertyAccessExpression(expression) || expression.name.text !== 't') return false;
  const target = expression.expression;
  return (
    (ts.isIdentifier(target) && target.text === 'l10n') ||
    (ts.isPropertyAccessExpression(target) && target.name.text === 'l10n')
  );
}

function checkBundle(): void {
  const bundlePath = join(extensionDir, 'l10n/bundle.l10n.ja.json');
  const bundle = JSON.parse(readFileSync(bundlePath, 'utf8')) as Record<string, unknown>;
  for (const [key, where] of keys) {
    const translation = bundle[key];
    if (typeof translation !== 'string') {
      problems.push(`${where}: 日本語訳がない: ${key}`);
      continue;
    }
    const placeholders = (s: string) =>
      [...s.matchAll(/\{\d+\}/g)]
        .map((m) => m[0])
        .sort()
        .join();
    if (placeholders(key) !== placeholders(translation)) {
      problems.push(`${relative(root, bundlePath)}: 埋め込み位置が元の文字列と違う: ${key}`);
    }
  }
  for (const key of Object.keys(bundle)) {
    if (!keys.has(key)) problems.push(`${relative(root, bundlePath)}: 使われていない訳: ${key}`);
  }
}

function checkPackageNls(): void {
  const manifest = readFileSync(join(extensionDir, 'package.json'), 'utf8');
  const used = new Set([...manifest.matchAll(/"%([^%"]+)%"/g)].map((m) => m[1] ?? ''));
  for (const name of ['package.nls.json', 'package.nls.ja.json']) {
    const nls = JSON.parse(readFileSync(join(extensionDir, name), 'utf8')) as Record<
      string,
      unknown
    >;
    for (const key of used)
      if (!(key in nls)) problems.push(`packages/extension/${name}: ${key} がない`);
    for (const key of Object.keys(nls)) {
      if (!used.has(key)) problems.push(`packages/extension/${name}: 使われていない: ${key}`);
    }
  }
  if (JAPANESE.test(manifest))
    problems.push('packages/extension/package.json: 日本語を直接書いている(%key% にする)');
}
