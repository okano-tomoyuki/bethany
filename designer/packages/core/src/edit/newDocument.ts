/**
 * 新しいフォーム(*.bfm.json)の初期内容(docs/designer/editor-design.md §8)。
 */
import { memberNameProblem } from '../identifier.ts';
import { l10n } from '../l10n.ts';
import { FORMAT_VERSION, type CommentLocale, type BfmDocument } from '../dsl/schema.ts';

/** 新しいフォームの大きさ(クライアント領域。dsl-spec.md §10 Q9) */
export const NEW_FORM_SIZE = { width: 320, height: 240 } as const;

/**
 * @param name フォームの名前(生成するクラス名の既定値は T + name)。Caption にも使う
 * @param commentLocale 生成するコードのコメントの言語(作成する人の表示言語。tk-designer ADR 0014)
 */
export function createDocument(name: string, commentLocale?: CommentLocale): BfmDocument {
  return {
    formatVersion: FORMAT_VERSION,
    codegen: { ...(commentLocale && { commentLocale }), cpp: {}, python: {} },
    form: {
      name,
      class: 'TForm',
      properties: { Width: NEW_FORM_SIZE.width, Height: NEW_FORM_SIZE.height, Caption: name },
    },
  };
}

/** フォームの名前として使えなければ、その理由(dsl-spec.md §4.1。T + name も Bethany の名前と衝突してはならない) */
export function formNameProblem(name: string): string | null {
  const problem = memberNameProblem(name);
  if (problem) return problem;
  if (memberNameProblem(`T${name}`))
    return l10n.t('The class name "T{0}" generated from the form name is used by Bethany', name);
  return null;
}
