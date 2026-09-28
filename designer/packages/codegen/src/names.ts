import type { BfmDocument, BfprojDocument, CommentLocale } from '@bethany-designer/core';

/** パスを区切りで分割する(Windows と POSIX の両方の区切りに対応する) */
function pathSegments(path: string): string[] {
  return path.split(/[\\/]/).filter((s) => s !== '');
}

/** パスの最後の要素(ファイル名) */
export function fileNameOf(path: string): string {
  return pathSegments(path).pop() ?? path;
}

/** DSL のファイル名(例: "MainForm.bfm.json")から拡張子を除いた名前("MainForm") */
export function baseName(dslFileName: string): string {
  return fileNameOf(dslFileName)
    .replace(/\.bfm\.json$/, '')
    .replace(/\.json$/, '');
}

export interface CppTarget {
  readonly className: string;
  /** DSL ファイルのあるフォルダからの相対パス */
  readonly header: string;
  readonly source: string;
}

export interface PythonTarget {
  readonly className: string;
  readonly file: string;
}

export interface ResolvedTargets {
  readonly cpp?: CppTarget;
  readonly python?: PythonTarget;
}

/** フォームのコード生成の設定(docs/designer/project-spec.md §5)。フォームが属するプロジェクトの codegen から決める */
export interface FormCodegenSettings {
  readonly cpp: boolean;
  readonly python: boolean;
  readonly commentLocale?: CommentLocale | undefined;
}

/** プロジェクトに属さないフォームの設定: C++ と Python の両方、コメントは英語 */
export const DEFAULT_FORM_CODEGEN: FormCodegenSettings = { cpp: true, python: true };

/**
 * フォームが属するプロジェクト(複数可)の codegen から、フォームのコード生成の設定を決める。
 * 言語はすべてのプロジェクトの和集合、コメントの言語は最初に書かれているもの(食い違いは拡張がプロジェクトファイルに警告を出す)。
 * プロジェクトに属さなければ DEFAULT_FORM_CODEGEN。
 */
export function formCodegenSettings(projects: readonly BfprojDocument[]): FormCodegenSettings {
  if (projects.length === 0) return DEFAULT_FORM_CODEGEN;
  return {
    cpp: projects.some((p) => p.codegen?.cpp !== undefined),
    python: projects.some((p) => p.codegen?.python !== undefined),
    commentLocale: projects.find((p) => p.codegen?.commentLocale)?.codegen?.commentLocale,
  };
}

/**
 * 生成するターゲットの出力先とクラス名。settings で有効な言語だけを返す。
 * クラス名はフォームの名前から(MainForm → TMainForm。dsl-spec.md §10 Q2)、ファイル名は DSL のファイル名から決め、
 * DSL と同じフォルダに置く(C++Builder の Unit1.cpp・Unit1.h と Form1 のように、ファイル名とフォームの名前は別のもの)。
 */
export function resolveTargets(
  doc: BfmDocument,
  dslFileName: string,
  settings: FormCodegenSettings = DEFAULT_FORM_CODEGEN,
): ResolvedTargets {
  const base = baseName(dslFileName);
  const className = `T${doc.form.name}`;
  return {
    ...(settings.cpp && {
      cpp: { className, header: `${base}.hpp`, source: `${base}.cpp` },
    }),
    ...(settings.python && { python: { className, file: `${base}.py` } }),
  };
}

/**
 * from のファイルから見た to のファイルの相対パス(どちらも同じフォルダからの相対パス。区切りは "/")。
 * C++ のソースからヘッダを include するパスに使う。
 */
export function relativePath(from: string, to: string): string {
  const split = (p: string) => pathSegments(p).filter((s) => s !== '.');
  const fromDir = split(from).slice(0, -1);
  const target = split(to);
  let common = 0;
  while (
    common < fromDir.length &&
    common < target.length - 1 &&
    fromDir[common] === target[common]
  )
    common++;
  return [...fromDir.slice(common).map(() => '..'), ...target.slice(common)].join('/');
}
