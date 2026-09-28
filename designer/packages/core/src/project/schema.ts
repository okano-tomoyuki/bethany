/**
 * プロジェクトファイル(*.bfproj.json)の構造のスキーマ(docs/designer/project-spec.md)。
 * 実装後はこのスキーマを正とし、project-spec.md と食い違う場合は project-spec.md を直す。
 * パスの形・重複・mainForm が forms にあるかは validate.ts で調べる。
 */
import * as z from 'zod';
import { CommentLocale } from '../dsl/schema.ts';

export const PROJECT_FORMAT_VERSION = 1;

/**
 * 起動部分(main)のコード生成の設定(project-spec.md §5)。書いたターゲットだけを生成する。
 * main はプロジェクトファイルのフォルダからの相対パス(既定はプロジェクト名 + .cpp / .py)。
 */
export const ProjectCodegenSettings = z.strictObject({
  commentLocale: CommentLocale.optional(),
  cpp: z.strictObject({ main: z.string().optional() }).optional(),
  python: z.strictObject({ main: z.string().optional() }).optional(),
});
export type ProjectCodegenSettings = z.infer<typeof ProjectCodegenSettings>;

export const BfprojDocument = z.strictObject({
  $schema: z.string().optional(),
  formatVersion: z.literal(PROJECT_FORMAT_VERSION),
  codegen: ProjectCodegenSettings.optional(),
  /** メインフォーム(forms のどれか) */
  mainForm: z.string().optional(),
  /** プロジェクトに属するフォーム(プロジェクトファイルのフォルダからの相対パス。区切りは /) */
  forms: z.array(z.string()).optional(),
  /**
   * 起動時に作るフォーム(作る順)。forms のどれか。無ければ forms のすべてを forms の順に作る(C++Builder の既定と同じ)。
   * メインフォームは、この一覧によらず常に最初に作る。
   */
  autoCreate: z.array(z.string()).optional(),
});
export type BfprojDocument = z.infer<typeof BfprojDocument>;
