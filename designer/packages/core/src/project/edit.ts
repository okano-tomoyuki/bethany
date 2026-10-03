/**
 * プロジェクトファイルの編集と、決まった形での書き出し(docs/designer/project-spec.md §3・§4)。
 * どれも元のドキュメントを変えず、新しいドキュメントを返す。パスはプロジェクトファイルのフォルダからの相対パス。
 */
import * as z from 'zod';
import type { CommentLocale } from '../dsl/schema.ts';
import { l10n } from '../l10n.ts';
import { normalizePath, sameFormPath } from './paths.ts';
import {
  BfprojDocument,
  PROJECT_FORMAT_VERSION,
  type CppOverride,
  type ProjectCppSettings,
} from './schema.ts';

/**
 * 新しいプロジェクト。起動部分は C++ と Python の両方を生成する設定にする(新しいフォームと同じ)。
 * @param commentLocale 生成するコードのコメントの言語(作成する人の表示言語。tk-designer ADR 0014)
 */
export function createProject(
  forms: readonly string[] = [],
  commentLocale?: CommentLocale,
): BfprojDocument {
  return withForms(
    {
      formatVersion: PROJECT_FORMAT_VERSION,
      codegen: { ...(commentLocale && { commentLocale }), cpp: {}, python: {} },
    },
    forms.map(normalizePath),
  );
}

/** Windows でファイル名に使えない名前(拡張子を付けても使えない) */
const RESERVED_FILE_NAMES = /^(con|prn|aux|nul|com[1-9]|lpt[1-9])$/i;

/**
 * プロジェクトの名前として使えなければ、その理由。名前はプロジェクトファイル(<名前>.bfproj.json)と
 * 起動部分(<名前>.cpp・<名前>.py)のファイル名になるので、Windows のファイル名に使えるものに限る。
 */
export function projectNameProblem(name: string): string | null {
  if (name === '') return l10n.t('Enter a name');
  // eslint-disable-next-line no-control-regex
  if (/[\\/:*?"<>|\u0000-\u001f]/.test(name))
    return l10n.t('A file name cannot contain a backslash or any of / : * ? " < > |');
  if (/[. ]$/.test(name)) return l10n.t('A file name cannot end with a period or a space');
  if (RESERVED_FILE_NAMES.test(name))
    return l10n.t('"{0}" is reserved by Windows and cannot be used as a file name', name);
  return null;
}

export function containsForm(doc: BfprojDocument, form: string): boolean {
  return (doc.forms ?? []).some((f) => sameFormPath(f, form));
}

export function isMainForm(doc: BfprojDocument, form: string): boolean {
  return doc.mainForm !== undefined && sameFormPath(doc.mainForm, form);
}

/**
 * フォームを末尾に加える。プロジェクトの最初のフォームはメインフォームにもする(C++Builder と同じ)。
 * 起動時に作るフォームにもする(autoCreate が書かれていれば末尾に加える。無ければすべてが対象)。
 */
export function addForm(doc: BfprojDocument, form: string): BfprojDocument {
  if (containsForm(doc, form)) return doc;
  const added = withForms(doc, [...(doc.forms ?? []), normalizePath(form)]);
  return doc.autoCreate
    ? { ...added, autoCreate: [...doc.autoCreate, normalizePath(form)] }
    : added;
}

/** 起動時に作るか(メインフォームは常に作る) */
export function isAutoCreated(doc: BfprojDocument, form: string): boolean {
  return (
    isMainForm(doc, form) || (doc.autoCreate ?? doc.forms ?? []).some((f) => sameFormPath(f, form))
  );
}

/** 起動時に作るフォーム(作る順)。メインフォームが先頭、残りは autoCreate(無ければ forms)の順 */
export function autoCreateForms(doc: BfprojDocument): string[] {
  const main = doc.mainForm === undefined ? [] : [normalizePath(doc.mainForm)];
  const rest = (doc.autoCreate ?? doc.forms ?? [])
    .map(normalizePath)
    .filter((form) => !main.some((m) => sameFormPath(m, form)));
  return [...main, ...rest];
}

/** 起動時に作るかを切り替える。autoCreate が無ければ、今の状態(forms のすべて)から書き出す */
export function setAutoCreate(doc: BfprojDocument, form: string, on: boolean): BfprojDocument {
  const current = (doc.autoCreate ?? doc.forms ?? []).map(normalizePath);
  const without = current.filter((f) => !sameFormPath(f, form));
  return { ...doc, autoCreate: on ? [...without, normalizePath(form)] : without };
}

/** フォームを外す。メインフォームを外したら、残りの先頭をメインフォームにする */
export function removeForm(doc: BfprojDocument, form: string): BfprojDocument {
  return mapFormPaths(doc, (f) => (sameFormPath(f, form) ? null : f));
}

/** メインフォームにする(プロジェクトに無ければ加える) */
export function setMainForm(doc: BfprojDocument, form: string): BfprojDocument {
  const added = addForm(doc, form);
  const main = (added.forms ?? []).find((f) => sameFormPath(f, form));
  return { ...added, mainForm: main };
}

/**
 * フォームのパスを書き換える(名前の変更・移動への追従)。fn が null を返したフォームは外す。
 * メインフォームも同じく書き換え、外れたら残りの先頭をメインフォームにする。
 */
export function mapFormPaths(
  doc: BfprojDocument,
  fn: (form: string) => string | null,
): BfprojDocument {
  const forms: string[] = [];
  for (const form of doc.forms ?? []) {
    const mapped = fn(form);
    if (mapped !== null && !forms.some((f) => sameFormPath(f, mapped)))
      forms.push(normalizePath(mapped));
  }
  const result: BfprojDocument = { ...doc, forms };
  if (doc.autoCreate) {
    const autoCreate: string[] = [];
    for (const form of doc.autoCreate) {
      const mapped = fn(form);
      if (mapped !== null && !autoCreate.some((f) => sameFormPath(f, mapped)))
        autoCreate.push(normalizePath(mapped));
    }
    result.autoCreate = autoCreate;
  }
  const cpp = doc.codegen?.cpp;
  if (cpp?.overrides && doc.codegen) {
    const overrides = mapOverrides(cpp.overrides, fn);
    const rest = { ...cpp };
    delete rest.overrides;
    result.codegen = { ...doc.codegen, cpp: overrides.length > 0 ? { ...rest, overrides } : rest };
  }
  if (doc.mainForm === undefined) return result;
  const mainMapped = fn(doc.mainForm);
  const mainForm =
    mainMapped !== null && forms.some((f) => sameFormPath(f, mainMapped))
      ? normalizePath(mainMapped)
      : forms[0];
  if (mainForm === undefined) delete result.mainForm;
  else result.mainForm = mainForm;
  return result;
}

/**
 * overrides の forms のうち、フォームのパスそのもの(ワイルドカードを含まず .bfm.json で終わる)を fn で書き換える。
 * 消えたものは除き、forms が空になった要素は除く。フォルダ・ワイルドカードのパターンはそのまま
 */
function mapOverrides(
  overrides: readonly CppOverride[],
  fn: (form: string) => string | null,
): CppOverride[] {
  return overrides.flatMap((override) => {
    const forms = override.forms.flatMap((pattern) => {
      if (/[*?]/.test(pattern) || !pattern.endsWith('.bfm.json')) return [pattern];
      const mapped = fn(pattern);
      return mapped === null ? [] : [normalizePath(mapped)];
    });
    return forms.length === 0 ? [] : [{ ...override, forms }];
  });
}

const CPP_FORM_KEYS = [
  'namespace',
  'includeGuard',
  'includeGuardPrefix',
  'headerExtension',
  'sourceExtension',
  'headerDir',
  'sourceDir',
] as const satisfies readonly (keyof CppOverride)[];

/** codegen.cpp のキーを決まった順に並べる(スキーマの順。overrides の各要素は forms が先頭) */
function orderCpp(cpp: ProjectCppSettings): ProjectCppSettings {
  const pick = <T extends object>(from: T, keys: readonly (keyof T)[]) =>
    Object.fromEntries(keys.flatMap((key) => (from[key] === undefined ? [] : [[key, from[key]]])));
  return {
    ...pick(cpp, ['main', ...CPP_FORM_KEYS]),
    ...(cpp.overrides && {
      overrides: cpp.overrides.map((o) => pick(o, ['forms', ...CPP_FORM_KEYS]) as CppOverride),
    }),
  };
}

/** 決まった形で書き出す(キーの順は $schema・formatVersion・codegen・mainForm・forms・autoCreate。一覧は 1 行に 1 つ) */
export function serializeProject(doc: BfprojDocument): string {
  const codegen = doc.codegen && {
    ...(doc.codegen.commentLocale !== undefined && { commentLocale: doc.codegen.commentLocale }),
    ...(doc.codegen.cpp && { cpp: orderCpp(doc.codegen.cpp) }),
    ...(doc.codegen.python && {
      python: {
        ...(doc.codegen.python.main !== undefined && { main: doc.codegen.python.main }),
        ...(doc.codegen.python.moduleDir !== undefined && {
          moduleDir: doc.codegen.python.moduleDir,
        }),
      },
    }),
  };
  const ordered = {
    ...(doc.$schema !== undefined && { $schema: doc.$schema }),
    formatVersion: doc.formatVersion,
    ...(codegen && { codegen }),
    ...(doc.mainForm !== undefined && { mainForm: doc.mainForm }),
    forms: doc.forms ?? [],
    ...(doc.autoCreate && { autoCreate: doc.autoCreate }),
  };
  return `${JSON.stringify(ordered, null, 2)}\n`;
}

/** テキストエディタでの補完・検証用の JSON Schema(dsl/jsonSchema.ts と同じく draft-07) */
export function projectJsonSchema(): Record<string, unknown> {
  return z.toJSONSchema(BfprojDocument, { target: 'draft-7', io: 'input' });
}

/** forms を置き換え、メインフォームが無ければ先頭にする */
function withForms(doc: BfprojDocument, forms: readonly string[]): BfprojDocument {
  const mainForm = doc.mainForm ?? forms[0];
  return { ...doc, ...(mainForm !== undefined && { mainForm }), forms: [...forms] };
}
