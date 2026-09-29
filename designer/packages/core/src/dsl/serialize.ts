/**
 * ドキュメントを決まった形のテキストに書き出す(docs/designer/dsl-spec.md §8)。
 * 同じ内容なら常に同じテキストになるようにし、1 回の操作による差分を小さくする(tk-designer ADR 0006)。
 *
 * - キーの順: ドキュメント・ノード・codegen は決まった順。properties はカタログの順(Left・Top・Width・Height が先頭)、
 *   events もカタログの順。カタログに無いキーは、元の順のまま後ろに置く。
 * - properties は 1 行に 1 つのプロパティ(1 つの変更が 1 行の差分になる)。
 * - ノード(name を持つオブジェクト)も 1 行にしない。
 * - それ以外で、値が単純なもの(文字列・数値・真偽・それらの配列・そのようなオブジェクト)だけのオブジェクト・配列は、
 *   1 行に収まれば 1 行で書く(events・Font・Anchors 等)。
 * - インデントは 2 文字、改行は LF、末尾に改行。
 */
import { findClass, getCatalog } from '../catalog/catalog.ts';
import type { PropertyInfo } from '../catalog/types.ts';
import type {
  ComponentNode,
  ControlNode,
  FormNode,
  MenuItemNode,
  BfmDocument,
  Properties,
  PropertyValue,
} from './schema.ts';

const INDENT = '  ';
/** 1 行で書くときの行の長さの上限(Prettier の printWidth と同じ) */
const MAX_WIDTH = 100;

export function serializeDocument(doc: BfmDocument): string {
  return `${stringify(ordered(doc), '', '')}\n`;
}

// ---- キーの順 -------------------------------------------------------------------

/** キーを並べ替えた写し(値の中身は変えない) */
function ordered(doc: BfmDocument): Json {
  return pick(doc, ['$schema', 'formatVersion', 'codegen', 'form', 'components'], {
    codegen: (c) =>
      pick(c as object, ['commentLocale', 'cpp', 'python'], {
        cpp: (v) => pick(v as object, ['className', 'header', 'source']),
        python: (v) => pick(v as object, ['className', 'file']),
      }),
    form: (f) => formNode(f as FormNode),
    components: (list) => (list as ComponentNode[]).map(componentNode),
  });
}

function formNode(node: FormNode): Json {
  return controlLike(node);
}

function controlLike(node: FormNode | ControlNode): Json {
  const info = findClass(node.class);
  return pick(node, ['name', 'class', 'properties', 'events', 'controls'], {
    properties: (p) => properties(p as Properties, info?.properties),
    events: (e) => byKeys(e as Json, Object.keys(info?.events ?? {})),
    controls: (list) => (list as ControlNode[]).map(controlLike),
  });
}

function componentNode(node: ComponentNode): Json {
  const info = findClass(node.class);
  return pick(node, ['name', 'class', 'design', 'properties', 'events', 'items'], {
    design: (d) => pick(d as object, ['left', 'top']),
    properties: (p) => properties(p as Properties, info?.properties),
    events: (e) => byKeys(e as Json, Object.keys(info?.events ?? {})),
    items: (list) => (list as MenuItemNode[]).map(menuItemNode),
  });
}

function menuItemNode(node: MenuItemNode): Json {
  const info = findClass('TMenuItem');
  return pick(node, ['name', 'properties', 'events', 'items'], {
    properties: (p) => properties(p as Properties, info?.properties),
    events: (e) => byKeys(e as Json, Object.keys(info?.events ?? {})),
    items: (list) => (list as MenuItemNode[]).map(menuItemNode),
  });
}

/** properties をカタログの順に。入れ子のオブジェクト(Font 等)の中もカタログの順にする */
function properties(
  props: Properties,
  known: Readonly<Record<string, PropertyInfo>> | undefined,
): Json {
  const sorted = byKeys(props, Object.keys(known ?? {}));
  const catalog = getCatalog();
  for (const [name, value] of Object.entries(sorted)) {
    const type = known && Object.hasOwn(known, name) ? known[name]?.type : undefined;
    if (type?.kind === 'object' && isPlainObject(value))
      sorted[name] = properties(value, catalog.objects[type.class]?.properties);
    // コレクションの項目の中もカタログの順
    if (type?.kind === 'collection' && Array.isArray(value)) {
      const itemProperties = catalog.objects[type.item]?.properties;
      sorted[name] = value.map((item) =>
        isPlainObject(item) ? properties(item, itemProperties) : item,
      );
    }
  }
  return sorted;
}

/** order の順に並べ、order に無いキーは元の順で後ろに置く */
function byKeys(object: Readonly<Record<string, PropertyValue>>, order: readonly string[]): Json {
  const rank = new Map(order.map((key, i) => [key, i]));
  const entries = Object.entries(object).map(([k, v], i) => ({ k, v, i }));
  entries.sort(
    (a, b) => (rank.get(a.k) ?? order.length + a.i) - (rank.get(b.k) ?? order.length + b.i),
  );
  return Object.fromEntries(entries.map(({ k, v }) => [k, v]));
}

/** keys の順に並べ(undefined は除く)、transform があれば値を変換する。keys に無いキーは後ろに置く */
function pick(
  object: object,
  keys: readonly string[],
  transform: Readonly<Record<string, (value: unknown) => unknown>> = {},
): Json {
  const source = object as Record<string, unknown>;
  const result: Record<string, unknown> = {};
  const all = [...keys, ...Object.keys(source).filter((k) => !keys.includes(k))];
  for (const key of all) {
    const value = source[key];
    if (value === undefined) continue;
    result[key] = Object.hasOwn(transform, key) ? transform[key]?.(value) : value;
  }
  return result as Json;
}

// ---- テキスト -------------------------------------------------------------------

type Json = Record<string, PropertyValue>;

function stringify(value: unknown, indent: string, key: string): string {
  const inner = indent + INDENT;
  if (Array.isArray(value)) {
    const items: readonly unknown[] = value;
    if (items.length === 0) return '[]';
    const flat = oneLine(value, indent, key);
    if (flat !== undefined) return flat;
    return `[\n${items.map((v) => inner + stringify(v, inner, '')).join(',\n')}\n${indent}]`;
  }
  if (isPlainObject(value)) {
    const entries = Object.entries(value);
    if (entries.length === 0) return '{}';
    // properties と、ノード(name を持つもの)は 1 行にしない
    if (key !== 'properties' && !Object.hasOwn(value, 'name')) {
      const flat = oneLine(value, indent, key);
      if (flat !== undefined) return flat;
    }
    const body = entries
      .map(([k, v]) => `${inner}${JSON.stringify(k)}: ${stringify(v, inner, k)}`)
      .join(',\n');
    return `{\n${body}\n${indent}}`;
  }
  return JSON.stringify(value);
}

/** 単純な値だけで、`"key": ` の後に 1 行で収まるなら、その 1 行 */
function oneLine(value: unknown, indent: string, key: string): string | undefined {
  if (!isSimple(value, 0)) return undefined;
  const text = flat(value);
  const prefix = key === '' ? 0 : JSON.stringify(key).length + 2;
  // 後ろの "," の分も数える
  return indent.length + prefix + text.length + 1 <= MAX_WIDTH ? text : undefined;
}

/** 1 行で書ける値: 単純な値、単純な値の配列、それらを値に持つオブジェクト(オブジェクトの入れ子は 1 段まで) */
function isSimple(value: unknown, depth: number): boolean {
  if (isScalar(value)) return true;
  if (Array.isArray(value)) return value.every(isScalar);
  if (isPlainObject(value))
    return (
      depth === 0 &&
      Object.values(value).every(
        (v) => isScalar(v) || (Array.isArray(v) && v.every(isScalar)) || isEmptyObject(v),
      )
    );
  return false;
}

function flat(value: unknown): string {
  if (Array.isArray(value)) return `[${value.map(flat).join(', ')}]`;
  if (isPlainObject(value)) {
    const entries = Object.entries(value);
    return entries.length === 0
      ? '{}'
      : `{ ${entries.map(([k, v]) => `${JSON.stringify(k)}: ${flat(v)}`).join(', ')} }`;
  }
  return JSON.stringify(value);
}

function isScalar(value: unknown): boolean {
  return value === null || ['string', 'number', 'boolean'].includes(typeof value);
}

function isEmptyObject(value: unknown): boolean {
  return isPlainObject(value) && Object.keys(value).length === 0;
}

function isPlainObject(value: unknown): value is Record<string, PropertyValue> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}
