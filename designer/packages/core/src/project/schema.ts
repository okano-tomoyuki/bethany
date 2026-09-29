/**
 * プロジェクトファイル(*.bfproj.json)の構造のスキーマ(docs/designer/project-spec.md)。
 * 実装後はこのスキーマを正とし、project-spec.md と食い違う場合は project-spec.md を直す。
 * パスの形・重複・mainForm が forms にあるかは validate.ts で調べる。
 */
import * as z from 'zod';
import { CommentLocale } from '../dsl/schema.ts';

export const PROJECT_FORMAT_VERSION = 1;

export const CPP_HEADER_EXTENSIONS = ['.hpp', '.h', '.hh', '.hxx'] as const;
export const CPP_SOURCE_EXTENSIONS = ['.cpp', '.cc', '.cxx'] as const;

/**
 * C++ のコード生成の設定(project-spec.md §5)。パスはプロジェクトファイルのフォルダからの相対パス。
 * namespace の各部分が識別子か・フォルダが相対パスかは validate.ts で調べる。
 */
export const ProjectCppSettings = z.strictObject({
  /** 起動部分の出力先(既定はプロジェクト名 + .cpp) */
  main: z.string().optional(),
  /** フォームのクラスとフォームの変数を入れる名前空間(`app`・`app::ui`) */
  namespace: z.string().optional(),
  /** インクルードガードの書き方(既定は macro) */
  includeGuard: z.enum(['macro', 'pragma']).optional(),
  /** macro のときのマクロ名の先頭に付ける文字列 */
  includeGuardPrefix: z.string().optional(),
  headerExtension: z.enum(CPP_HEADER_EXTENSIONS).optional(),
  sourceExtension: z.enum(CPP_SOURCE_EXTENSIONS).optional(),
  /** フォームのヘッダの出力先のフォルダ(既定はフォームと同じフォルダ) */
  headerDir: z.string().optional(),
  /** フォームのソースの出力先のフォルダ(既定はフォームと同じフォルダ) */
  sourceDir: z.string().optional(),
});
export type ProjectCppSettings = z.infer<typeof ProjectCppSettings>;

/**
 * コード生成の設定(project-spec.md §5)。書いたターゲットだけを生成する。
 * main はプロジェクトファイルのフォルダからの相対パス(既定はプロジェクト名 + .cpp / .py)。
 */
export const ProjectCodegenSettings = z.strictObject({
  commentLocale: CommentLocale.optional(),
  cpp: ProjectCppSettings.optional(),
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
