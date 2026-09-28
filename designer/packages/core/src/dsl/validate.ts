/**
 * 意味の検証(docs/designer/dsl-spec.md §7)。構造の検証(schema.ts)を通ったドキュメントを、カタログと照合する。
 */
import { findClass, getCatalog, isSubclassOf } from '../catalog/catalog.ts';
import type { ClassInfo, PropertyInfo, PropertyType } from '../catalog/types.ts';
import { isValidIdentifier, memberNameProblem } from '../identifier.ts';
import { l10n } from '../l10n.ts';
import { childProblem } from './constraints.ts';
import type { Diagnostic, DiagnosticCode, JsonPath } from './diagnostics.ts';
import type { BfmDocument, Properties } from './schema.ts';
import { classOf, walkNodes, type NodeLocation } from './tree.ts';

const COLOR_PATTERN = /^#[0-9A-Fa-f]{6}$/;

export function validateDocument(doc: BfmDocument): Diagnostic[] {
  const diagnostics: Diagnostic[] = [];
  const error = (code: DiagnosticCode, message: string, path: JsonPath) =>
    diagnostics.push({ severity: 'error', code, message, path });

  const nodes = [...walkNodes(doc)];

  // ---- 名前 ----
  /** name → クラス(参照の解決に使う。重複したものは最初のもの) */
  const classByName = new Map<string, string>();
  for (const location of nodes) {
    const { name } = location.node;
    const path = [...location.path, 'name'];
    const problem = memberNameProblem(name);
    if (problem) error(nameProblemCode(name), problem, path);
    else if (location.kind === 'form' && memberNameProblem(`T${name}`))
      error(
        'reserved-name',
        l10n.t('The class name "T{0}" generated from the form name is used by Bethany', name),
        path,
      );
    if (classByName.has(name)) error('duplicate-name', l10n.t('"{0}" is duplicated', name), path);
    else classByName.set(name, classOf(location));
  }

  // ---- クラス・親子 ----
  const infoOf = new Map<NodeLocation, ClassInfo>();
  for (const location of nodes) {
    const info = checkClass(location, error);
    if (info) infoOf.set(location, info);
  }

  // ---- プロパティ・イベント ----
  /** ハンドラ名 → 最初に書いたイベントの型 */
  const handlerTypes = new Map<string, string>();
  for (const location of nodes) {
    const info = infoOf.get(location);
    if (!info) continue;
    const className = classOf(location);
    checkProperties(location.node.properties, info.properties, className, [
      ...location.path,
      'properties',
    ]);

    for (const [event, handler] of Object.entries(location.node.events ?? {})) {
      const path = [...location.path, 'events', event];
      const eventInfo = Object.hasOwn(info.events, event) ? info.events[event] : undefined;
      if (!eventInfo) {
        error('unknown-event', l10n.t('{0} has no event {1}', className, event), [
          ...location.path,
          'events',
        ]);
        continue;
      }
      const problem = memberNameProblem(handler);
      if (problem) {
        error(nameProblemCode(handler), problem, path);
        continue;
      }
      if (classByName.has(handler)) {
        error(
          'duplicate-name',
          l10n.t('The handler name "{0}" is also the name of a component', handler),
          path,
        );
        continue;
      }
      const first = handlerTypes.get(handler);
      if (first === undefined) handlerTypes.set(handler, eventInfo.type);
      else if (first !== eventInfo.type)
        error(
          'handler-signature-conflict',
          l10n.t(
            'The handler "{0}" is used for events of different types ({1} and {2})',
            handler,
            first,
            eventInfo.type,
          ),
          path,
        );
    }
  }
  return diagnostics;

  function checkProperties(
    properties: Properties | undefined,
    known: Readonly<Record<string, PropertyInfo>>,
    className: string,
    /** properties そのものの位置 */
    propertiesPath: JsonPath,
  ): void {
    for (const [name, value] of Object.entries(properties ?? {})) {
      const path = [...propertiesPath, name];
      const info = Object.hasOwn(known, name) ? known[name] : undefined;
      if (!info) {
        error(
          'unknown-property',
          l10n.t('{0} has no property {1} that can be set at design time', className, name),
          propertiesPath,
        );
        continue;
      }
      checkValue(info.type, value, path);
    }
  }

  function checkValue(type: PropertyType, value: unknown, path: JsonPath): void {
    const invalid = (expected: string) =>
      error('invalid-property-value', l10n.t('Expected {0}', expected), path);
    const catalog = getCatalog();
    switch (type.kind) {
      case 'int':
        if (!Number.isInteger(value)) invalid(l10n.t('an integer'));
        return;
      case 'float':
        if (typeof value !== 'number') invalid(l10n.t('a number'));
        return;
      case 'bool':
        if (typeof value !== 'boolean') invalid('true / false');
        return;
      case 'string':
        if (typeof value !== 'string') invalid(l10n.t('a string'));
        return;
      case 'char':
        if (typeof value !== 'string' || value.length !== 1) invalid(l10n.t('a single character'));
        return;
      case 'alias': {
        const constants = catalog.constants[type.alias] ?? {};
        if (type.alias === 'TShortCut') {
          if (typeof value !== 'string') invalid(l10n.t('a shortcut such as "Ctrl+S"'));
        } else if (type.alias === 'TColor') {
          if (
            typeof value !== 'string' ||
            !(Object.hasOwn(constants, value) || COLOR_PATTERN.test(value))
          )
            invalid(
              l10n.t('a color constant ({0}) or "#RRGGBB"', Object.keys(constants).join(', ')),
            );
        } else if (typeof value !== 'string' || !Object.hasOwn(constants, value)) {
          invalid(l10n.t('one of {0}', Object.keys(constants).join(', ')));
        }
        return;
      }
      case 'enum': {
        const items = catalog.enums[type.enum] ?? [];
        if (typeof value !== 'string' || !items.includes(value))
          invalid(l10n.t('one of {0}', items.join(', ')));
        return;
      }
      case 'flags':
      case 'set': {
        const items =
          (type.kind === 'flags' ? catalog.flags[type.flags] : catalog.enums[type.enum]) ?? [];
        if (
          !Array.isArray(value) ||
          value.some((v) => typeof v !== 'string' || !items.includes(v)) ||
          new Set(value).size !== value.length
        )
          invalid(l10n.t('an array of distinct values from {0}', items.join(', ')));
        return;
      }
      case 'strings':
        if (!Array.isArray(value) || value.some((v) => typeof v !== 'string'))
          invalid(l10n.t('an array of strings'));
        return;
      case 'object': {
        if (typeof value !== 'object' || value === null || Array.isArray(value)) {
          invalid(l10n.t('an object'));
          return;
        }
        const known = catalog.objects[type.class]?.properties ?? {};
        checkProperties(value as Properties, known, type.class, path);
        return;
      }
      case 'ref': {
        if (typeof value !== 'string') {
          invalid(l10n.t('the name of a component'));
          return;
        }
        const target = classByName.get(value);
        if (target === undefined)
          error('unknown-reference', l10n.t('No component named "{0}"', value), path);
        else if (!isSubclassOf(target, type.class))
          error(
            'reference-type-mismatch',
            l10n.t('"{0}" is {1}, but {2} is expected', value, target, type.class),
            path,
          );
        return;
      }
    }
  }
}

function nameProblemCode(name: string): DiagnosticCode {
  return isValidIdentifier(name) ? 'invalid-identifier' : 'reserved-name';
}

function checkClass(
  location: NodeLocation,
  error: (code: DiagnosticCode, message: string, path: JsonPath) => void,
): ClassInfo | undefined {
  const className = classOf(location);
  const info = findClass(className);
  const classPath = [...location.path, 'class'];
  if (!info) {
    error('unknown-class', l10n.t('Unknown class {0}', className), classPath);
    return undefined;
  }
  switch (location.kind) {
    case 'form':
      return info;
    case 'menuItem':
      return info;
    case 'component':
      if (info.kind !== 'component') {
        error(
          'wrong-class-kind',
          l10n.t('{0} is a control. Write it in "controls"', className),
          classPath,
        );
        return undefined;
      }
      if (location.node.items && !isSubclassOf(className, 'TMenu'))
        error('items-not-allowed', l10n.t('{0} cannot have menu items', className), [
          ...location.path,
          'items',
        ]);
      return info;
    case 'control': {
      if (info.kind !== 'control') {
        error(
          'wrong-class-kind',
          l10n.t('{0} is not a control. Write it in "components"', className),
          classPath,
        );
        return undefined;
      }
      const problem = childProblem(location.parent.class, className);
      if (problem) error(problem.code, problem.message, classPath);
      return info;
    }
  }
}
