import type { Diagnostic } from '../dsl/diagnostics.ts';
import { l10n } from '../l10n.ts';
import { BfprojDocument, PROJECT_FORMAT_VERSION } from './schema.ts';
import { validateProject } from './validate.ts';

export interface ProjectParseResult {
  /** 構造の検証を通過した場合のみ存在する(意味の検証でエラーがあっても存在する) */
  readonly project?: BfprojDocument;
  readonly diagnostics: readonly Diagnostic[];
}

/** *.bfproj.json のテキストを読み込み、構造と意味を検証する(docs/designer/project-spec.md) */
export function parseProject(text: string): ProjectParseResult {
  let json: unknown;
  try {
    json = JSON.parse(text);
  } catch (e) {
    const message = e instanceof Error ? e.message : String(e);
    return { diagnostics: [{ severity: 'error', code: 'json-syntax', message, path: [] }] };
  }

  const version =
    typeof json === 'object' && json !== null && !Array.isArray(json)
      ? (json as Record<string, unknown>).formatVersion
      : undefined;
  if (typeof version === 'number' && version !== PROJECT_FORMAT_VERSION) {
    return {
      diagnostics: [
        {
          severity: 'error',
          code: 'unsupported-version',
          message: l10n.t(
            'formatVersion {0} is not supported (supported: {1})',
            String(version),
            PROJECT_FORMAT_VERSION,
          ),
          path: ['formatVersion'],
        },
      ],
    };
  }

  const result = BfprojDocument.safeParse(json);
  if (!result.success) {
    return {
      diagnostics: result.error.issues.map((issue) => ({
        severity: 'error',
        code: 'schema',
        message: issue.message,
        path: issue.path.map((key) => (typeof key === 'symbol' ? String(key) : key)),
      })),
    };
  }
  return { project: result.data, diagnostics: validateProject(result.data) };
}
