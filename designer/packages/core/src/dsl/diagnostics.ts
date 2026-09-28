/** JSON 上の位置。例: ["form", "controls", 2, "properties", "Left"] */
export type JsonPath = readonly (string | number)[];

export type Severity = 'error' | 'warning';

export type DiagnosticCode =
  // 読み込み・構造
  | 'json-syntax'
  | 'unsupported-version'
  | 'schema'
  // 名前
  | 'invalid-identifier'
  | 'duplicate-name'
  | 'reserved-name'
  // カタログ
  | 'unknown-class'
  | 'wrong-class-kind'
  | 'unknown-property'
  | 'invalid-property-value'
  | 'unknown-event'
  // 参照・ハンドラ
  | 'unknown-reference'
  | 'reference-type-mismatch'
  | 'handler-signature-conflict'
  // 構造上の制約
  | 'controls-not-allowed'
  | 'invalid-child-class'
  | 'invalid-parent-class'
  | 'items-not-allowed'
  // プロジェクトファイル(docs/designer/project-spec.md)
  | 'invalid-form-path'
  | 'duplicate-form'
  | 'unknown-main-form'
  | 'missing-form-file';

export interface Diagnostic {
  readonly severity: Severity;
  readonly code: DiagnosticCode;
  readonly message: string;
  readonly path: JsonPath;
}

export function hasErrors(diagnostics: readonly Diagnostic[]): boolean {
  return diagnostics.some((d) => d.severity === 'error');
}
