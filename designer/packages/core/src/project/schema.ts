/**
 * プロジェクトファイル(*.bfproj.json)の構造のスキーマ(docs/designer/project-spec.md)。
 * 実装後はこのスキーマを正とし、project-spec.md と食い違う場合は project-spec.md を直す。
 * パスの形・重複・mainForm が forms にあるかは validate.ts で調べる。
 */
import * as z from 'zod';

export const PROJECT_FORMAT_VERSION = 1;

export const BfprojDocument = z.strictObject({
  $schema: z.string().optional(),
  formatVersion: z.literal(PROJECT_FORMAT_VERSION),
  /** メインフォーム(forms のどれか) */
  mainForm: z.string().optional(),
  /** プロジェクトに属するフォーム(プロジェクトファイルのフォルダからの相対パス。区切りは /) */
  forms: z.array(z.string()).optional(),
});
export type BfprojDocument = z.infer<typeof BfprojDocument>;
