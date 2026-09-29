/**
 * コード生成(docs/designer/codegen-design.md)。
 * 文字列を受け取り文字列を返すだけで、ファイルの入出力は呼び出し側(拡張機能・CLI)が行う。
 */
import { l10n, type BfmDocument, type CommentLocale } from '@bethany-designer/core';
import { emitCpp } from './cpp/emit.ts';
import { CPP_NAMES, cppSyntax } from './cpp/syntax.ts';
import { buildModel } from './model.ts';
import { emitPython } from './python/emit.ts';
import { PYTHON_NAMES, PYTHON_SYNTAX } from './python/syntax.ts';
import {
  DEFAULT_CPP_OPTIONS,
  DEFAULT_PYTHON_OPTIONS,
  fileNameOf,
  resolveTargets,
  type CppOptions,
  type CppTarget,
  type FormCodegenSettings,
  type PythonOptions,
  type PythonTarget,
} from './names.ts';
import { createFile, mergeFile, type GeneratedCode, type MergeResult } from './region.ts';
import { findStaleNames, staleNamesWarning, type StaleCheck } from './stale.ts';

export { regionHash } from './hash.ts';
export { findHandler, type HandlerLanguage, type HandlerPosition } from './locate.ts';
export {
  baseName,
  cppOptions,
  DEFAULT_CPP_OPTIONS,
  DEFAULT_FORM_CODEGEN,
  DEFAULT_PYTHON_OPTIONS,
  formCodegenSettings,
  resolveTargets,
  type CppOptions,
  type FormCodegenSettings,
  type FormProject,
  type PythonOptions,
  type CppTarget,
  type PythonTarget,
  type ResolvedTargets,
} from './names.ts';
export type { MergeResult } from './region.ts';
export { buildModel, type FormModel, type Statement, type Value } from './model.ts';
export {
  generateProject,
  isProjectFileName,
  resolveProjectTargets,
  type FormSource,
  type ProjectTargets,
} from './project.ts';

export type GenerateResult = MergeResult & {
  /** 出力先(DSL ファイルのあるフォルダからの相対パス) */
  readonly path?: string;
};

export interface CppGenerateResult {
  readonly header: GenerateResult;
  readonly source: GenerateResult;
}

/**
 * Python のコードを生成する。existing があればマーカー区間だけを置き換え、なければ新規ファイルを作る。
 * @param doc 検証を通過したドキュメント
 * @param dslFileName DSL のファイル名(出力先と、生成物の説明に使う)
 * @param commentLocale 生成するコードのコメントの言語(プロジェクトの codegen.commentLocale)
 */
export function generatePython(
  doc: BfmDocument,
  dslFileName: string,
  existing: string | undefined,
  commentLocale?: CommentLocale,
  options: PythonOptions = DEFAULT_PYTHON_OPTIONS,
): GenerateResult {
  const { className, file } = pythonTarget(doc, dslFileName, options);
  const generated = emitPython(buildModel(doc, className), fileNameOf(dslFileName), commentLocale);
  if (existing === undefined) {
    return {
      ok: true,
      text: createFile(generated, PYTHON_SYNTAX),
      modifiedRegions: [],
      addedStubs: [],
      path: file,
    };
  }
  return { ...mergeFile(existing, generated, PYTHON_SYNTAX), path: file };
}

/**
 * C++ のコードを生成する。ヘッダとソースのそれぞれについて、既存の内容があればマーカー区間だけを置き換え、
 * なければ新規ファイルを作る。
 * @param doc 検証を通過したドキュメント
 * @param dslFileName DSL のファイル名(出力先と、生成物の説明に使う)
 * @param commentLocale 生成するコードのコメントの言語(プロジェクトの codegen.commentLocale)
 * @param options プロジェクトの C++ の設定と、プロジェクトのフォルダからのフォームのパス(formCodegenSettings)
 */
export function generateCpp(
  doc: BfmDocument,
  dslFileName: string,
  existingHeader: string | undefined,
  existingSource: string | undefined,
  commentLocale?: CommentLocale,
  options: { readonly cpp?: CppOptions | undefined; readonly formPath?: string | undefined } = {},
): CppGenerateResult {
  const target = cppTarget(doc, dslFileName, options.cpp ?? DEFAULT_CPP_OPTIONS, options.formPath);
  const files = emitCpp(
    buildModel(doc, target.className),
    fileNameOf(dslFileName),
    target,
    commentLocale,
  );
  const syntax = cppSyntax(target.className);
  const generate = (
    code: GeneratedCode,
    existing: string | undefined,
    path: string,
  ): GenerateResult =>
    existing === undefined
      ? { ok: true, text: createFile(code, syntax), modifiedRegions: [], addedStubs: [], path }
      : { ...mergeFile(existing, code, syntax), path };
  return {
    header: generate(files.header, existingHeader, target.header),
    source: generate(files.source, existingSource, target.source),
  };
}

/** 出力する 1 ファイル分の生成結果 */
export interface OutputFile {
  /** DSL ファイルのあるフォルダからの相対パス */
  readonly path: string;
  readonly result: MergeResult;
}

export interface GenerateAllResult {
  readonly files: readonly OutputFile[];
  readonly warnings: readonly string[];
}

/**
 * settings で有効なすべての言語のコードを生成する(拡張機能・CLI の共通の入口)。
 * @param settings フォームが属するプロジェクトから決めた設定(formCodegenSettings)
 * @param readExisting 出力先の既存の内容を返す(なければ undefined)
 */
export function generateAll(
  doc: BfmDocument,
  dslFileName: string,
  settings: FormCodegenSettings,
  readExisting: (path: string) => string | undefined,
): GenerateAllResult | { readonly error: string } {
  const targets = resolveTargets(doc, dslFileName, settings);
  if (!targets.cpp && !targets.python) {
    return {
      error: l10n.t(
        'No language to generate. Set codegen in the project file (e.g. {0})',
        '"codegen": { "cpp": {} }',
      ),
    };
  }
  const { commentLocale } = settings;
  const files: OutputFile[] = [];
  const staleChecks: StaleCheck[] = [];
  if (targets.cpp) {
    const existingHeader = readExisting(targets.cpp.header);
    const result = generateCpp(
      doc,
      dslFileName,
      existingHeader,
      readExisting(targets.cpp.source),
      commentLocale,
      settings,
    );
    files.push({ path: targets.cpp.header, result: result.header });
    files.push({ path: targets.cpp.source, result: result.source });
    if (existingHeader !== undefined && result.header.ok && result.source.ok) {
      // 生成したメンバはヘッダの宣言の区間から取り出し、使われているかはヘッダとソースの両方で調べる
      staleChecks.push({
        rules: CPP_NAMES,
        before: existingHeader,
        after: result.header.text,
        files: [
          { path: targets.cpp.header, text: result.header.text },
          { path: targets.cpp.source, text: result.source.text },
        ],
      });
    }
  }
  if (targets.python) {
    const existing = readExisting(targets.python.file);
    const result = generatePython(doc, dslFileName, existing, commentLocale, settings.python);
    files.push({ path: targets.python.file, result });
    if (existing !== undefined && result.ok) {
      staleChecks.push({
        rules: PYTHON_NAMES,
        before: existing,
        after: result.text,
        files: [{ path: targets.python.file, text: result.text }],
      });
    }
  }
  const stale = staleNamesWarning(staleChecks.flatMap(findStaleNames));
  const merged = files.flatMap((f) =>
    f.result.ok ? (f.result.warnings ?? []).map((w) => `${f.path}: ${w}`) : [],
  );
  return { files, warnings: [...merged, ...(stale ? [stale] : [])] };
}

function cppTarget(
  doc: BfmDocument,
  dslFileName: string,
  cpp: CppOptions,
  formPath: string | undefined,
): CppTarget {
  const target = resolveTargets(doc, dslFileName, { cpp, python: undefined, formPath }).cpp;
  if (!target) throw new Error('unreachable');
  return target;
}

function pythonTarget(doc: BfmDocument, dslFileName: string, options: PythonOptions): PythonTarget {
  const target = resolveTargets(doc, dslFileName, { cpp: undefined, python: options }).python;
  if (!target) throw new Error('unreachable');
  return target;
}
