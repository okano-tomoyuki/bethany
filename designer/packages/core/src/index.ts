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
export { classOf, findNode, propertyValue, walkNodes, type NodeLocation } from './dsl/tree.ts';

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
export { childProblem, type ChildProblem } from './dsl/constraints.ts';
export { documentJsonSchema } from './dsl/jsonSchema.ts';
export { locate, type TextRange } from './dsl/locate.ts';
export { serializeDocument } from './dsl/serialize.ts';

export {
  applyCommand,
  type Bounds,
  type CommandResult,
  type EditCommand,
  type PropertyChange,
} from './edit/commands.ts';
export {
  DEFAULT_POSITION,
  defaultEventOf,
  defaultSizeOf,
  hasOwnBounds,
  initialProperties,
} from './edit/defaults.ts';
export {
  collectHandlerNames,
  collectMemberNames,
  defaultHandlerName,
  nameBaseOfClass,
  nameBaseOfMenuCaption,
  nextName,
} from './edit/naming.ts';
export { minimalTextEdit, type TextEdit } from './edit/textEdit.ts';
export { createDocument, formNameProblem, NEW_FORM_SIZE } from './edit/newDocument.ts';

export type {
  CanvasSettings,
  ExtensionToWebviewMessage,
  WebviewToExtensionMessage,
} from './protocol.ts';
export { formatValue, normalizeShortCut, parseInput, type InputResult } from './edit/inputs.ts';

export { clientInsets, type Insets } from './layout/insets.ts';
