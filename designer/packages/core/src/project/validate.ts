/**
 * プロジェクトファイルの意味の検証(docs/designer/project-spec.md §3)。ファイルがあるかは拡張が調べる。
 */
import type { Diagnostic } from '../dsl/diagnostics.ts';
import { describeIdentifierProblem, isValidIdentifier } from '../identifier.ts';
import { l10n } from '../l10n.ts';
import { isValidFormPath, sameFormPath } from './paths.ts';
import type { BfprojDocument } from './schema.ts';

export function validateProject(doc: BfprojDocument): Diagnostic[] {
  const diagnostics: Diagnostic[] = [];
  const forms = doc.forms ?? [];
  forms.forEach((form, index) => {
    if (!isValidFormPath(form)) {
      diagnostics.push({
        severity: 'error',
        code: 'invalid-form-path',
        message: l10n.t(
          '"{0}" is not a form path: use a relative path ending with .bfm.json, separated by /',
          form,
        ),
        path: ['forms', index],
      });
    } else if (forms.slice(0, index).some((other) => sameFormPath(other, form))) {
      diagnostics.push({
        severity: 'error',
        code: 'duplicate-form',
        message: l10n.t('"{0}" is listed more than once', form),
        path: ['forms', index],
      });
    }
  });
  (doc.autoCreate ?? []).forEach((form, index, list) => {
    if (!forms.some((f) => sameFormPath(f, form)))
      diagnostics.push({
        severity: 'error',
        code: 'unknown-auto-create',
        message: l10n.t('"{0}" in autoCreate is not in forms', form),
        path: ['autoCreate', index],
      });
    else if (list.slice(0, index).some((other) => sameFormPath(other, form)))
      diagnostics.push({
        severity: 'error',
        code: 'duplicate-form',
        message: l10n.t('"{0}" is listed more than once', form),
        path: ['autoCreate', index],
      });
  });
  diagnostics.push(...validateCpp(doc));
  if (doc.mainForm !== undefined && !forms.some((form) => sameFormPath(form, doc.mainForm ?? '')))
    diagnostics.push({
      severity: 'error',
      code: 'unknown-main-form',
      message: l10n.t('The main form "{0}" is not in forms', doc.mainForm),
      path: ['mainForm'],
    });
  return diagnostics;
}

/** codegen.cpp の名前空間・マクロ名の先頭・フォルダ */
function validateCpp(doc: BfprojDocument): Diagnostic[] {
  const cpp = doc.codegen?.cpp;
  if (!cpp) return [];
  const diagnostics: Diagnostic[] = [];
  if (cpp.namespace !== undefined) {
    for (const part of cpp.namespace.split('::')) {
      const problem = isValidIdentifier(part);
      if (problem === null) continue;
      diagnostics.push({
        severity: 'error',
        code: 'invalid-namespace',
        message: l10n.t(
          '"{0}" cannot be used as a namespace: {1}',
          cpp.namespace,
          describeIdentifierProblem(problem),
        ),
        path: ['codegen', 'cpp', 'namespace'],
      });
      break;
    }
  }
  if (
    cpp.includeGuardPrefix !== undefined &&
    !/^[A-Za-z_][A-Za-z0-9_]*$/.test(cpp.includeGuardPrefix)
  )
    diagnostics.push({
      severity: 'error',
      code: 'invalid-include-guard-prefix',
      message: l10n.t(
        'includeGuardPrefix can contain only letters, digits and _, and cannot start with a digit',
      ),
      path: ['codegen', 'cpp', 'includeGuardPrefix'],
    });
  for (const key of ['headerDir', 'sourceDir'] as const) {
    const dir = cpp[key];
    if (dir === undefined || isValidDirectory(dir)) continue;
    diagnostics.push({
      severity: 'error',
      code: 'invalid-directory',
      message: l10n.t('"{0}" is not a folder path: use a relative path separated by /', dir),
      path: ['codegen', 'cpp', key],
    });
  }
  return diagnostics;
}

/** プロジェクトファイルのフォルダからの相対パスのフォルダか(`.` はプロジェクトのフォルダ) */
function isValidDirectory(path: string): boolean {
  return path !== '' && !path.startsWith('/') && !path.includes('\\') && !/^[A-Za-z]:/.test(path);
}
