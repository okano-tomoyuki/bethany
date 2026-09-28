/**
 * プロジェクトファイルのフォームのパス(docs/designer/project-spec.md §3)の計算。区切りは `/` だけを扱う。
 * core は Webview でも動くため、node:path を使わない。
 */

export const FORM_EXTENSION = '.bfm.json';
export const PROJECT_EXTENSION = '.bfproj.json';

/** `.`・`..`・連続した `/` を畳む。先頭の `..` は残す。絶対パス(`/` で始まる)なら `/` で始める */
export function normalizePath(path: string): string {
  const absolute = path.startsWith('/');
  const parts: string[] = [];
  for (const part of path.split('/')) {
    if (part === '' || part === '.') continue;
    if (part === '..' && parts.length > 0 && parts[parts.length - 1] !== '..') parts.pop();
    else if (part !== '..' || !absolute) parts.push(part);
  }
  return (absolute ? '/' : '') + parts.join('/');
}

/** プロジェクトファイルに書けるフォームのパスか(相対パス・区切りは `/`・`.bfm.json` で終わる) */
export function isValidFormPath(path: string): boolean {
  return (
    path.endsWith(FORM_EXTENSION) &&
    !path.startsWith('/') &&
    !path.includes('\\') &&
    !/^[A-Za-z]:/.test(path) &&
    // `.bfm.json` だけ(フォームの名前が空)は不可
    (normalizePath(path).split('/').pop() ?? '').length > FORM_EXTENSION.length
  );
}

/** 同じフォームを指すかを比べるための形 */
export function sameFormPath(a: string, b: string): boolean {
  return normalizePath(a) === normalizePath(b);
}

/** 絶対パスのフォルダ。`/a/b/c.json` → `/a/b` */
export function dirname(path: string): string {
  const index = path.lastIndexOf('/');
  return index <= 0 ? '/' : path.slice(0, index);
}

/** フォルダ(絶対パス)から相対パスをたどった先の絶対パス */
export function resolvePath(dir: string, relative: string): string {
  return normalizePath(`${dir}/${relative}`);
}

/** フォルダ(絶対パス)から target(絶対パス)への相対パス */
export function relativePath(dir: string, target: string): string {
  const from = normalizePath(dir).split('/').filter(Boolean);
  const to = normalizePath(target).split('/').filter(Boolean);
  let common = 0;
  while (common < from.length && common < to.length && from[common] === to[common]) common++;
  return [...from.slice(common).map(() => '..'), ...to.slice(common)].join('/');
}

/** path が base そのものか、その下にあるか(どちらも絶対パス) */
export function isSameOrInside(path: string, base: string): boolean {
  const p = normalizePath(path);
  const b = normalizePath(base);
  return p === b || p.startsWith(b.endsWith('/') ? b : `${b}/`);
}
