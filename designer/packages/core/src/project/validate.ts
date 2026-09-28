/**
 * プロジェクトファイルの意味の検証(docs/designer/project-spec.md §3)。ファイルがあるかは拡張が調べる。
 */
import type { Diagnostic } from '../dsl/diagnostics.ts';
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
  if (doc.mainForm !== undefined && !forms.some((form) => sameFormPath(form, doc.mainForm ?? '')))
    diagnostics.push({
      severity: 'error',
      code: 'unknown-main-form',
      message: l10n.t('The main form "{0}" is not in forms', doc.mainForm),
      path: ['mainForm'],
    });
  return diagnostics;
}
