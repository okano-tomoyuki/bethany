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
  fileNameOf,
  relativePath,
  resolveTargets,
  type CppTarget,
  type FormCodegenSettings,
  type PythonTarget,
} from './names.ts';
import { createFile, mergeFile, type GeneratedCode, type MergeResult } from './region.ts';
import { findStaleNames, staleNamesWarning, type StaleCheck } from './stale.ts';

export { regionHash } from './hash.ts';
export { findHandler, type HandlerLanguage, type HandlerPosition } from './locate.ts';
export {
  baseName,
  DEFAULT_FORM_CODEGEN,
  formCodegenSettings,
  resolveTargets,
  type FormCodegenSettings,
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
): GenerateResult {
  const { className, file } = pythonTarget(doc, dslFileName);
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
 */
export function generateCpp(
  doc: BfmDocument,
  dslFileName: string,
  existingHeader: string | undefined,
  existingSource: string | undefined,
  commentLocale?: CommentLocale,
): CppGenerateResult {
  const target = cppTarget(doc, dslFileName);
  const files = emitCpp(
    buildModel(doc, target.className),
    fileNameOf(dslFileName),
    relativePath(target.source, target.header),
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
    const result = generatePython(doc, dslFileName, existing, commentLocale);
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
  return { files, warnings: stale ? [stale] : [] };
}

function cppTarget(doc: BfmDocument, dslFileName: string): CppTarget {
  const target = resolveTargets(doc, dslFileName, { cpp: true, python: false }).cpp;
  if (!target) throw new Error('unreachable');
  return target;
}

function pythonTarget(doc: BfmDocument, dslFileName: string): PythonTarget {
  const target = resolveTargets(doc, dslFileName, { cpp: false, python: true }).python;
  if (!target) throw new Error('unreachable');
  return target;
}
