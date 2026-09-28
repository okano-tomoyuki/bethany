import type { NvformDocument } from '@no-vcl-designer/core';

/** パスを区切りで分割する(Windows と POSIX の両方の区切りに対応する) */
function pathSegments(path: string): string[] {
  return path.split(/[\\/]/).filter((s) => s !== '');
}

/** パスの最後の要素(ファイル名) */
export function fileNameOf(path: string): string {
  return pathSegments(path).pop() ?? path;
}

/** DSL のファイル名(例: "MainForm.nvform.json")から拡張子を除いた名前("MainForm") */
export function baseName(dslFileName: string): string {
  return fileNameOf(dslFileName)
    .replace(/\.nvform\.json$/, '')
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

/**
 * codegen の設定に既定値を補う(dsl-spec.md §9)。書かれたターゲットだけを返す。
 * クラス名はフォームの名前から(MainForm → TMainForm。§10 Q2)、ファイル名は DSL のファイル名から決める
 * (C++Builder の Unit1.cpp・Unit1.h と Form1 のように、ファイル名とフォームの名前は別のもの)。
 */
export function resolveTargets(doc: NvformDocument, dslFileName: string): ResolvedTargets {
  const settings = doc.codegen ?? {};
  const base = baseName(dslFileName);
  const className = `T${doc.form.name}`;
  return {
    ...(settings.cpp && {
      cpp: {
        className: settings.cpp.className ?? className,
        header: settings.cpp.header ?? `${base}.hpp`,
        source: settings.cpp.source ?? `${base}.cpp`,
      },
    }),
    ...(settings.python && {
      python: {
        className: settings.python.className ?? className,
        file: settings.python.file ?? `${base}.py`,
      },
    }),
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
