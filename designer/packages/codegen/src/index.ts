/**
 * コード生成(docs/designer/codegen-design.md)。
 * 文字列を受け取り文字列を返すだけで、ファイルの入出力は呼び出し側(拡張機能・CLI)が行う。
 */
import { l10n, type NvformDocument } from '@no-vcl-designer/core';
import { emitCpp } from './cpp/emit.ts';
import { CPP_NAMES, cppSyntax } from './cpp/syntax.ts';
import { buildModel } from './model.ts';
import { emitPython } from './python/emit.ts';
import { PYTHON_NAMES, PYTHON_SYNTAX } from './python/syntax.ts';
import { fileNameOf, relativePath, resolveTargets } from './names.ts';
import { createFile, mergeFile, type GeneratedCode, type MergeResult } from './region.ts';
import { findStaleNames, staleNamesWarning, type StaleCheck } from './stale.ts';

export { regionHash } from './hash.ts';
export {
  baseName,
  resolveTargets,
  type CppTarget,
  type PythonTarget,
  type ResolvedTargets,
} from './names.ts';
export type { MergeResult } from './region.ts';
export { buildModel, type FormModel, type Statement, type Value } from './model.ts';

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
 * @param dslFileName DSL のファイル名(出力先の既定値と、生成物の説明に使う)
 */
export function generatePython(
  doc: NvformDocument,
  dslFileName: string,
  existing: string | undefined,
): GenerateResult {
  const target = resolveTargets(doc, dslFileName).python;
  if (!target) return { ok: false, error: l10n.t('{0} is not set', 'codegen.python') };
  const generated = emitPython(
    buildModel(doc, target.className),
    fileNameOf(dslFileName),
    doc.codegen?.commentLocale,
  );
  if (existing === undefined) {
    return {
      ok: true,
      text: createFile(generated, PYTHON_SYNTAX),
      modifiedRegions: [],
      addedStubs: [],
      path: target.file,
    };
  }
  return { ...mergeFile(existing, generated, PYTHON_SYNTAX), path: target.file };
}

/**
 * C++ のコードを生成する。ヘッダとソースのそれぞれについて、既存の内容があればマーカー区間だけを置き換え、
 * なければ新規ファイルを作る。
 * @param doc 検証を通過したドキュメント
 * @param dslFileName DSL のファイル名(出力先の既定値と、生成物の説明に使う)
 */
export function generateCpp(
  doc: NvformDocument,
  dslFileName: string,
  existingHeader: string | undefined,
  existingSource: string | undefined,
): CppGenerateResult | { readonly error: string } {
  const target = resolveTargets(doc, dslFileName).cpp;
  if (!target) return { error: l10n.t('{0} is not set', 'codegen.cpp') };
  const files = emitCpp(
    buildModel(doc, target.className),
    fileNameOf(dslFileName),
    relativePath(target.source, target.header),
    doc.codegen?.commentLocale,
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
 * codegen に書かれたすべてのターゲットのコードを生成する(拡張機能・CLI の共通の入口)。
 * @param readExisting 出力先の既存の内容を返す(なければ undefined)
 */
export function generateAll(
  doc: NvformDocument,
  dslFileName: string,
  readExisting: (path: string) => string | undefined,
): GenerateAllResult | { readonly error: string } {
  const targets = resolveTargets(doc, dslFileName);
  if (!targets.cpp && !targets.python) {
    return { error: l10n.t('codegen is not set (e.g. {0})', '"codegen": { "cpp": {} }') };
  }
  const files: OutputFile[] = [];
  const staleChecks: StaleCheck[] = [];
  if (targets.cpp) {
    const existingHeader = readExisting(targets.cpp.header);
    const result = generateCpp(doc, dslFileName, existingHeader, readExisting(targets.cpp.source));
    if ('error' in result) return result;
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
    const result = generatePython(doc, dslFileName, existing);
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
