/**
 * 表示する文字列の多言語対応(tk-designer ADR 0014 と同じ)。
 *
 * 文字列は英語で書き、l10n.t('...') で訳す。訳は packages/extension/l10n/bundle.l10n.<言語>.json に置く。
 * 拡張ホスト・Webview・CLI のそれぞれで、起動時に configureL10n で訳を読み込む(読み込まなければ英語のまま)。
 */
import * as l10n from '@vscode/l10n';
import * as z from 'zod';

export { l10n };

/** 訳のファイルの内容(英語の文字列 → 訳) */
export type L10nBundle = Readonly<Record<string, string | { message: string; comment: string[] }>>;

/**
 * 表示の言語と訳を設定する。
 * @param language 言語(VS Code の表示言語など。例: "ja"、"en")
 * @param bundle その言語の訳。英語、または訳がなければ undefined
 */
export function configureL10n(language: string, bundle: L10nBundle | undefined): void {
  l10n.config({ contents: bundle ?? {} });
  // Zod の標準のエラー文(スキーマの検証エラー)も同じ言語にする
  z.config(isJapanese(language) ? z.locales.ja() : z.locales.en());
}

export function isJapanese(language: string): boolean {
  return language.toLowerCase().startsWith('ja');
}
