import {
  matchesFormPattern,
  normalizePath,
  type BfmDocument,
  type BfprojDocument,
  type CommentLocale,
  type ProjectCppSettings,
} from '@bethany-designer/core';

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
  /** フォームのソースからヘッダを include するパス */
  readonly headerInclude: string;
  /**
   * ほかのファイル(起動部分)からヘッダを include するパス。headerDir からの相対パス(headerDir を include パスに入れる前提)。
   * headerDir を使わないときは undefined(include する側のファイルからの相対パスにする)
   */
  readonly includePath: string | undefined;
  /** 名前空間(`app::ui` なら ["app", "ui"]。無ければ空) */
  readonly namespace: readonly string[];
  /** インクルードガードのマクロ名。#pragma once なら undefined */
  readonly includeGuard: string | undefined;
}

export interface PythonTarget {
  readonly className: string;
  readonly file: string;
}

export interface ResolvedTargets {
  readonly cpp?: CppTarget;
  readonly python?: PythonTarget;
}

/** C++ のコード生成の設定(プロジェクトの codegen.cpp に既定値を補ったもの。project-spec.md §5) */
export interface CppOptions {
  readonly namespace: readonly string[];
  readonly includeGuard: 'macro' | 'pragma';
  readonly includeGuardPrefix: string;
  readonly headerExtension: string;
  readonly sourceExtension: string;
  /** プロジェクトファイルのフォルダからの相対パス。undefined ならフォームと同じフォルダ */
  readonly headerDir: string | undefined;
  readonly sourceDir: string | undefined;
}

export const DEFAULT_CPP_OPTIONS: CppOptions = {
  namespace: [],
  includeGuard: 'macro',
  includeGuardPrefix: '',
  headerExtension: '.hpp',
  sourceExtension: '.cpp',
  headerDir: undefined,
  sourceDir: undefined,
};

/**
 * プロジェクトの codegen.cpp に既定値を補う。formPath(プロジェクトのフォルダからのフォームのパス)を渡すと、
 * overrides のうちそのフォームに当てはまるものを書いた順に重ねる(後に書いたものが勝つ)。
 */
export function cppOptions(project: ProjectCppSettings, formPath?: string): CppOptions {
  const settings = (project.overrides ?? [])
    .filter((o) => formPath !== undefined && o.forms.some((p) => matchesFormPattern(p, formPath)))
    .reduce<ProjectCppSettings>(
      (merged, override) => ({ ...merged, ...withoutUndefined(override) }),
      project,
    );
  return {
    namespace: settings.namespace === undefined ? [] : settings.namespace.split('::'),
    includeGuard: settings.includeGuard ?? DEFAULT_CPP_OPTIONS.includeGuard,
    includeGuardPrefix: settings.includeGuardPrefix ?? '',
    headerExtension: settings.headerExtension ?? DEFAULT_CPP_OPTIONS.headerExtension,
    sourceExtension: settings.sourceExtension ?? DEFAULT_CPP_OPTIONS.sourceExtension,
    headerDir: settings.headerDir,
    sourceDir: settings.sourceDir,
  };
}

function withoutUndefined<T extends object>(value: T): Partial<T> {
  return Object.fromEntries(Object.entries(value).filter(([, v]) => v !== undefined)) as Partial<T>;
}

/** Python のコード生成の設定(プロジェクトの codegen.python。project-spec.md §5) */
export interface PythonOptions {
  /** プロジェクトファイルのフォルダからの相対パス。undefined ならフォームと同じフォルダ */
  readonly moduleDir: string | undefined;
  /** この設定を取ったプロジェクトのフォルダからの、フォームのファイルの相対パス(プロジェクトに属さなければ undefined) */
  readonly formPath: string | undefined;
}

/** フォームのコード生成の設定(docs/designer/project-spec.md §5)。フォームが属するプロジェクトの codegen から決める */
export interface FormCodegenSettings {
  /** C++ を生成しなければ undefined */
  readonly cpp: CppOptions | undefined;
  /** Python を生成しなければ undefined */
  readonly python: PythonOptions | undefined;
  readonly commentLocale?: CommentLocale | undefined;
  /**
   * C++ の設定を取ったプロジェクトのフォルダからの、フォームのファイルの相対パス(headerDir・sourceDir の下の置き場所を決める)。
   * プロジェクトに属さなければ undefined
   */
  readonly formPath?: string | undefined;
}

export const DEFAULT_PYTHON_OPTIONS: PythonOptions = { moduleDir: undefined, formPath: undefined };

/** プロジェクトに属さないフォームの設定: C++ と Python の両方、コメントは英語 */
export const DEFAULT_FORM_CODEGEN: FormCodegenSettings = {
  cpp: DEFAULT_CPP_OPTIONS,
  python: DEFAULT_PYTHON_OPTIONS,
};

/** フォームが属する 1 つのプロジェクト */
export interface FormProject {
  readonly doc: BfprojDocument;
  /** プロジェクトファイルのフォルダからの、フォームのファイルの相対パス(forms に書かれたもの) */
  readonly formPath: string;
}

/**
 * フォームが属するプロジェクト(複数可)の codegen から、フォームのコード生成の設定を決める。
 * 言語はすべてのプロジェクトの和集合、コメントの言語と C++ の設定は最初に書かれているもの
 * (食い違いは拡張がプロジェクトファイルに警告を出す)。プロジェクトに属さなければ DEFAULT_FORM_CODEGEN
 * (拡張・CLI はプロジェクトに属さないフォームを生成しない。テスト・確認のツールが使う)。
 */
export function formCodegenSettings(projects: readonly FormProject[]): FormCodegenSettings {
  if (projects.length === 0) return DEFAULT_FORM_CODEGEN;
  const cpp = projects.find((p) => p.doc.codegen?.cpp !== undefined);
  const python = projects.find((p) => p.doc.codegen?.python !== undefined);
  return {
    cpp: cpp?.doc.codegen?.cpp && cppOptions(cpp.doc.codegen.cpp, cpp.formPath),
    python: python?.doc.codegen?.python && {
      moduleDir: python.doc.codegen.python.moduleDir,
      formPath: python.formPath,
    },
    commentLocale: projects.find((p) => p.doc.codegen?.commentLocale)?.doc.codegen?.commentLocale,
    ...(cpp && { formPath: cpp.formPath }),
  };
}

/**
 * 生成するターゲットの出力先とクラス名。settings で有効な言語だけを返す。
 * クラス名はフォームの名前から(MainForm → TMainForm。dsl-spec.md §10 Q2)、ファイル名は DSL のファイル名から決め、
 * DSL と同じフォルダに置く(C++Builder の Unit1.cpp・Unit1.h と Form1 のように、ファイル名とフォームの名前は別のもの)。
 * C++ は headerDir・sourceDir があれば、そのフォルダの下の、プロジェクトのフォルダからフォームのフォルダまでと同じ場所に置く。
 */
export function resolveTargets(
  doc: BfmDocument,
  dslFileName: string,
  settings: FormCodegenSettings = DEFAULT_FORM_CODEGEN,
): ResolvedTargets {
  const base = baseName(dslFileName);
  const className = `T${doc.form.name}`;
  const { cpp } = settings;
  return {
    ...(cpp && { cpp: cppTarget(className, base, cpp, settings.formPath) }),
    ...(settings.python && {
      python: {
        className,
        file: placeOutput(`${base}.py`, settings.python.moduleDir, settings.python.formPath).path,
      },
    }),
  };
}

function cppTarget(
  className: string,
  base: string,
  options: CppOptions,
  formPath: string | undefined,
): CppTarget {
  const header = placeOutput(`${base}${options.headerExtension}`, options.headerDir, formPath);
  const source = placeOutput(`${base}${options.sourceExtension}`, options.sourceDir, formPath);
  const includePath = header.inDir;
  return {
    className,
    header: header.path,
    source: source.path,
    headerInclude: includePath ?? relativePath(source.path, header.path),
    includePath,
    namespace: options.namespace,
    includeGuard:
      options.includeGuard === 'pragma'
        ? undefined
        : guardMacro([
            options.includeGuardPrefix,
            ...withoutOverlap(
              options.namespace,
              (includePath ?? fileNameOf(header.path)).split('/'),
            ),
          ]),
  };
}

/**
 * 出力するファイルの置き場所。path は DSL ファイルのあるフォルダからの相対パス、inDir は dir からの相対パス(dir を使うときだけ)。
 * プロジェクトのフォルダの外にあるフォーム(forms のパスが .. で始まる)は、dir を使わずフォームと同じフォルダに置く。
 */
function placeOutput(
  file: string,
  dir: string | undefined,
  formPath: string | undefined,
): { readonly path: string; readonly inDir?: string } {
  if (dir === undefined || formPath === undefined) return { path: file };
  const form = normalizePath(formPath);
  if (form.startsWith('../')) return { path: file };
  const formDir = form.includes('/') ? form.slice(0, form.lastIndexOf('/')) : '';
  const inDir = formDir === '' ? file : normalizePath(`${formDir}/${file}`);
  return { path: relativePath(form, normalizePath(`${dir}/${inDir}`)), inDir };
}

/**
 * 名前空間とヘッダのパスをつなぐ。名前空間の末尾とパスの先頭が同じ(`app::dialogs` と `dialogs/About.hpp`)なら、重なりを 1 つにする
 * (APP_DIALOGS_DIALOGS_ABOUT_HPP ではなく APP_DIALOGS_ABOUT_HPP)。大文字と小文字は区別しない
 */
function withoutOverlap(namespace: readonly string[], path: readonly string[]): string[] {
  const same = (a: string | undefined, b: string | undefined) =>
    a !== undefined && b !== undefined && a.toLowerCase() === b.toLowerCase();
  for (let n = Math.min(namespace.length, path.length - 1); n > 0; n--) {
    const tail = namespace.slice(namespace.length - n);
    if (tail.every((part, i) => same(part, path[i]))) return [...namespace, ...path.slice(n)];
  }
  return [...namespace, ...path];
}

/** インクルードガードのマクロ名(大文字。識別子に使えない文字と、連続した・端の _ を畳む。__ は処理系の予約) */
function guardMacro(parts: readonly string[]): string {
  return parts
    .map((part) =>
      part
        .toUpperCase()
        .replace(/[^A-Z0-9]+/g, '_')
        .replace(/_+/g, '_')
        .replace(/^_|_$/g, ''),
    )
    .filter((part) => part !== '')
    .join('_')
    .replace(/^(?=[0-9])/, '_');
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
