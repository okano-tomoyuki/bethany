/**
 * プロジェクトファイルの編集と、決まった形での書き出し(docs/designer/project-spec.md §3・§4)。
 * どれも元のドキュメントを変えず、新しいドキュメントを返す。パスはプロジェクトファイルのフォルダからの相対パス。
 */
import * as z from 'zod';
import { normalizePath, sameFormPath } from './paths.ts';
import { BfprojDocument, PROJECT_FORMAT_VERSION } from './schema.ts';

export function createProject(forms: readonly string[] = []): BfprojDocument {
  return withForms({ formatVersion: PROJECT_FORMAT_VERSION }, forms.map(normalizePath));
}

export function containsForm(doc: BfprojDocument, form: string): boolean {
  return (doc.forms ?? []).some((f) => sameFormPath(f, form));
}

export function isMainForm(doc: BfprojDocument, form: string): boolean {
  return doc.mainForm !== undefined && sameFormPath(doc.mainForm, form);
}

/** フォームを末尾に加える。プロジェクトの最初のフォームはメインフォームにもする(C++Builder と同じ) */
export function addForm(doc: BfprojDocument, form: string): BfprojDocument {
  if (containsForm(doc, form)) return doc;
  return withForms(doc, [...(doc.forms ?? []), normalizePath(form)]);
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

/** 決まった形で書き出す(キーの順は $schema・formatVersion・mainForm・forms。forms は 1 行に 1 つ) */
export function serializeProject(doc: BfprojDocument): string {
  const ordered = {
    ...(doc.$schema !== undefined && { $schema: doc.$schema }),
    formatVersion: doc.formatVersion,
    ...(doc.mainForm !== undefined && { mainForm: doc.mainForm }),
    forms: doc.forms ?? [],
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
