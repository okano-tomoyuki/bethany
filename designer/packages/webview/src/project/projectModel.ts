/**
 * プロジェクトの設定画面(docs/designer/project-spec.md §6)の操作を、プロジェクトの変更にする。
 * どれも新しいドキュメントを返す(元は変えない)。空の値は「書かない(既定値)」にする。
 */
import {
  autoCreateForms,
  normalizePath,
  sameFormPath,
  type BfprojDocument,
  type CommentLocale,
  type CppOverride,
  type ProjectCppSettings,
} from '@bethany-designer/core';

type Codegen = NonNullable<BfprojDocument['codegen']>;
type PythonSettings = NonNullable<Codegen['python']>;

/** C++ の設定のうち、overrides でも書ける(フォームの形を決める)もの */
export type CppFormKey = Exclude<keyof CppOverride, 'forms'>;

function withCodegen(doc: BfprojDocument, fn: (codegen: Codegen) => Codegen): BfprojDocument {
  return { ...doc, codegen: fn(doc.codegen ?? {}) };
}

/** undefined・空文字のキーを除く */
function compact<T extends object>(value: T): T {
  return Object.fromEntries(
    Object.entries(value).filter(([, v]) => v !== undefined && v !== ''),
  ) as T;
}

export function setLanguage(
  doc: BfprojDocument,
  language: 'cpp' | 'python',
  on: boolean,
): BfprojDocument {
  return withCodegen(doc, (codegen) =>
    on
      ? { ...codegen, [language]: codegen[language] ?? {} }
      : Object.fromEntries(Object.entries(codegen).filter(([key]) => key !== language)),
  );
}

export function setCommentLocale(
  doc: BfprojDocument,
  locale: CommentLocale | undefined,
): BfprojDocument {
  return withCodegen(doc, (codegen) => compact({ ...codegen, commentLocale: locale }));
}

export function setCppField<K extends keyof ProjectCppSettings>(
  doc: BfprojDocument,
  key: K,
  value: ProjectCppSettings[K] | undefined,
): BfprojDocument {
  return withCodegen(doc, (codegen) => ({
    ...codegen,
    cpp: compact({ ...codegen.cpp, [key]: value }),
  }));
}

export function setPythonField<K extends keyof PythonSettings>(
  doc: BfprojDocument,
  key: K,
  value: PythonSettings[K] | undefined,
): BfprojDocument {
  return withCodegen(doc, (codegen) => ({
    ...codegen,
    python: compact({ ...codegen.python, [key]: value }),
  }));
}

function withOverrides(
  doc: BfprojDocument,
  fn: (overrides: CppOverride[]) => CppOverride[],
): BfprojDocument {
  return withCodegen(doc, (codegen) => {
    const overrides = fn([...(codegen.cpp?.overrides ?? [])]);
    const cpp = { ...codegen.cpp };
    if (overrides.length > 0) cpp.overrides = overrides;
    else delete cpp.overrides;
    return { ...codegen, cpp };
  });
}

/** 上書きを末尾に加える。forms はパターン(空にはできないので、最初のフォームのフォルダかフォーム) */
export function addOverride(doc: BfprojDocument, pattern: string): BfprojDocument {
  return withOverrides(doc, (overrides) => [...overrides, { forms: [pattern] }]);
}

export function removeOverride(doc: BfprojDocument, index: number): BfprojDocument {
  return withOverrides(doc, (overrides) => overrides.filter((_, i) => i !== index));
}

/** 上書きの順を入れ替える(後に書いたものが勝つので、順に意味がある) */
export function moveOverride(doc: BfprojDocument, index: number, delta: -1 | 1): BfprojDocument {
  return withOverrides(doc, (overrides) => moveItem(overrides, index, delta));
}

export function setOverrideForms(
  doc: BfprojDocument,
  index: number,
  forms: readonly string[],
): BfprojDocument {
  return withOverrides(doc, (overrides) =>
    overrides.map((o, i) => (i === index ? { ...o, forms: [...forms] } : o)),
  );
}

export function setOverrideField<K extends CppFormKey>(
  doc: BfprojDocument,
  index: number,
  key: K,
  value: CppOverride[K] | undefined,
): BfprojDocument {
  return withOverrides(doc, (overrides) =>
    overrides.map((o, i) => (i === index ? compact({ ...o, [key]: value }) : o)),
  );
}

/** 起動時に作るフォーム(メインフォームを除く、作る順)と作らないフォーム(forms の順) */
export function startupLists(doc: BfprojDocument): {
  readonly auto: readonly string[];
  readonly available: readonly string[];
} {
  const main = doc.mainForm === undefined ? undefined : normalizePath(doc.mainForm);
  const auto = autoCreateForms(doc).filter((f) => main === undefined || !sameFormPath(f, main));
  const available = (doc.forms ?? [])
    .map(normalizePath)
    .filter((f) => (main === undefined || !sameFormPath(f, main)) && !auto.includes(f));
  return { auto, available };
}

/** 起動時に作るフォームの順を入れ替える(autoCreate を書き出す) */
export function moveAutoCreate(doc: BfprojDocument, form: string, delta: -1 | 1): BfprojDocument {
  const { auto } = startupLists(doc);
  const index = auto.findIndex((f) => sameFormPath(f, form));
  if (index < 0) return doc;
  return { ...doc, autoCreate: moveItem(auto, index, delta) };
}

function moveItem<T>(items: readonly T[], index: number, delta: -1 | 1): T[] {
  const to = index + delta;
  if (to < 0 || to >= items.length) return [...items];
  const result = [...items];
  const [item] = result.splice(index, 1);
  if (item !== undefined) result.splice(to, 0, item);
  return result;
}

/** 名前空間などの入力(前後の空白を除き、空なら書かない) */
export function textValue(text: string): string | undefined {
  const trimmed = text.trim();
  return trimmed === '' ? undefined : trimmed;
}

/** パターンの入力(カンマか改行で区切る) */
export function parsePatterns(text: string): string[] {
  return text
    .split(/[,\n]/)
    .map((p) => p.trim())
    .filter((p) => p !== '');
}
