export {
  describeIdentifierProblem,
  isValidIdentifier,
  memberNameProblem,
  RESERVED_PREFIX,
  type IdentifierProblem,
} from './identifier.ts';
export { configureL10n, isJapanese, l10n, type L10nBundle } from './l10n.ts';

export * from './dsl/schema.ts';
export {
  hasErrors,
  type Diagnostic,
  type DiagnosticCode,
  type JsonPath,
  type Severity,
} from './dsl/diagnostics.ts';
export { parseDocument, type ParseResult } from './dsl/parse.ts';
export { validateDocument } from './dsl/validate.ts';
export { classOf, walkNodes, type NodeLocation } from './dsl/tree.ts';

export { findClass, findProperty, getCatalog, isSubclassOf, setTypeOf } from './catalog/catalog.ts';
export type {
  Catalog,
  ClassInfo,
  ClassKind,
  EventInfo,
  EventParam,
  PropertyInfo,
  PropertyType,
} from './catalog/types.ts';
