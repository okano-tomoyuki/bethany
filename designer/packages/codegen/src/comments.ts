/**
 * 生成するコードのコメント(tk-designer ADR 0014 と同じ)。
 * UI の言語ではなく DSL の codegen.commentLocale に従う(生成する人によって結果が変わらないように)。
 */
import type { CommentLocale } from '@no-vcl-designer/core';

export interface GeneratedComments {
  /** 生成するクラスの説明 */
  readonly classDoc: (sourceName: string) => string;
  /** 予約メソッド nvd_CreateComponents の説明 */
  readonly createComponents: string;
  /** ハンドラの雛形(C++。Python は pass) */
  readonly todo: string;
}

const COMMENTS: Readonly<Record<CommentLocale, GeneratedComments>> = {
  en: {
    classDoc: (sourceName) =>
      `Form created with the no_vcl designer (${sourceName}). Regions enclosed in markers are overwritten when regenerated.`,
    createComponents: 'Creates the components and sets their properties (generated).',
    todo: 'TODO: implement',
  },
  // l10n-ignore(生成するコードのコメントの定型文)
  ja: {
    classDoc: (sourceName) =>
      // l10n-ignore
      `no_vcl のデザイナーで作成したフォーム(${sourceName})。マーカーで囲まれた区間は再生成で上書きされる。`,
    // l10n-ignore
    createComponents: 'コンポーネントを生成し、プロパティを設定する(生成したコード)。',
    // l10n-ignore
    todo: 'TODO: 実装',
  },
};

/** 既定は英語 */
export function generatedComments(locale: CommentLocale | undefined): GeneratedComments {
  return COMMENTS[locale ?? 'en'];
}
