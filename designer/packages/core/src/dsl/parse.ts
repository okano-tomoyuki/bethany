import type { Diagnostic } from './diagnostics.ts';
import { l10n } from '../l10n.ts';
import { FORMAT_VERSION, NvformDocument } from './schema.ts';
import { validateDocument } from './validate.ts';

export interface ParseResult {
  /** 構造の検証を通過した場合のみ存在する(意味の検証でエラーがあっても存在する) */
  readonly document?: NvformDocument;
  readonly diagnostics: readonly Diagnostic[];
}

/** *.nvform.json のテキストを読み込み、構造と意味を検証する */
export function parseDocument(text: string): ParseResult {
  let json: unknown;
  try {
    json = JSON.parse(text);
  } catch (e) {
    const message = e instanceof Error ? e.message : String(e);
    return { diagnostics: [{ severity: 'error', code: 'json-syntax', message, path: [] }] };
  }

  const version = isObject(json) ? json.formatVersion : undefined;
  if (typeof version === 'number' && version !== FORMAT_VERSION) {
    return {
      diagnostics: [
        {
          severity: 'error',
          code: 'unsupported-version',
          message: l10n.t(
            'formatVersion {0} is not supported (supported: {1})',
            String(version),
            FORMAT_VERSION,
          ),
          path: ['formatVersion'],
        },
      ],
    };
  }

  const result = NvformDocument.safeParse(json);
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
  return { document: result.data, diagnostics: validateDocument(result.data) };
}

function isObject(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}
