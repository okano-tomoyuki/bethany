import { findClass, getCatalog } from '../catalog/catalog.ts';
import type { JsonPath } from './diagnostics.ts';
import type { ComponentNode, ControlNode, FormNode, MenuItemNode, BfmDocument } from './schema.ts';

/** ドキュメントの中のノードと、その位置 */
export type NodeLocation =
  | { readonly kind: 'form'; readonly node: FormNode; readonly path: JsonPath }
  | {
      readonly kind: 'control';
      readonly node: ControlNode;
      readonly path: JsonPath;
      /** 親(フォームかコントロール) */
      readonly parent: FormNode | ControlNode;
    }
  | { readonly kind: 'component'; readonly node: ComponentNode; readonly path: JsonPath }
  | {
      readonly kind: 'menuItem';
      readonly node: MenuItemNode;
      readonly path: JsonPath;
      /** 項目を持つメニュー(TMainMenu・TPopupMenu) */
      readonly menu: ComponentNode;
      /** 親の項目。メニューのルートの項目なら undefined */
      readonly parentItem: MenuItemNode | undefined;
    };

/**
 * すべてのノードを、コンポーネントを生成する順(docs/designer/codegen-design.md §4)でたどる:
 * フォーム、コントロール(深さ優先。親が先)、非ビジュアルコンポーネント(メニューなら続けてその項目を深さ優先)。
 */
export function* walkNodes(doc: BfmDocument): Generator<NodeLocation> {
  yield { kind: 'form', node: doc.form, path: ['form'] };
  yield* walkControls(doc.form, ['form']);
  for (const [i, component] of (doc.components ?? []).entries()) {
    const path = ['components', i];
    yield { kind: 'component', node: component, path };
    yield* walkItems(component, undefined, component.items, path);
  }
}

function* walkControls(
  parent: FormNode | ControlNode,
  parentPath: JsonPath,
): Generator<NodeLocation> {
  for (const [i, node] of (parent.controls ?? []).entries()) {
    const path = [...parentPath, 'controls', i];
    yield { kind: 'control', node, path, parent };
    yield* walkControls(node, path);
  }
}

function* walkItems(
  menu: ComponentNode,
  parentItem: MenuItemNode | undefined,
  items: readonly MenuItemNode[] | undefined,
  parentPath: JsonPath,
): Generator<NodeLocation> {
  for (const [i, node] of (items ?? []).entries()) {
    const path = [...parentPath, 'items', i];
    yield { kind: 'menuItem', node, path, menu, parentItem };
    yield* walkItems(menu, node, node.items, path);
  }
}

/** ノードのクラス(メニュー項目は TMenuItem) */
export function classOf(location: NodeLocation): string {
  return location.kind === 'menuItem' ? 'TMenuItem' : location.node.class;
}

/** name のノード(name が重複していれば最初のもの) */
export function findNode(doc: BfmDocument, name: string): NodeLocation | undefined {
  for (const location of walkNodes(doc)) if (location.node.name === name) return location;
  return undefined;
}

/**
 * プロパティの値。書いていなければカタログの既定値(入れ子のオブジェクトの中は path の 2 つ目で指す)。
 * カタログにも無ければ undefined。
 */
export function propertyValue(
  location: NodeLocation,
  path: readonly [string] | readonly [string, string],
): unknown {
  const [name, sub] = path;
  const own = location.node.properties?.[name];
  const info = findClass(classOf(location));
  const fallback =
    info && Object.hasOwn(info.properties, name) ? info.properties[name]?.default : undefined;
  if (sub === undefined) return own ?? fallback;
  const pick = (object: unknown) =>
    typeof object === 'object' && object !== null && !Array.isArray(object)
      ? (object as Record<string, unknown>)[sub]
      : undefined;
  return pick(own) ?? pick(fallback);
}

/**
 * コレクションのプロパティ(TStatusBar の Panels 等。docs/adr/0044)の項目。書いていない項目のプロパティはカタログの既定値で埋める。
 * コレクションでないプロパティなら空。
 */
export function collectionItems(
  location: NodeLocation,
  name: string,
): readonly Readonly<Record<string, unknown>>[] {
  const info = findClass(classOf(location));
  const type =
    info && Object.hasOwn(info.properties, name) ? info.properties[name]?.type : undefined;
  if (type?.kind !== 'collection') return [];
  const value = propertyValue(location, [name]);
  if (!Array.isArray(value)) return [];
  const known = getCatalog().objects[type.item]?.properties ?? {};
  const defaults = Object.fromEntries(Object.entries(known).map(([k, p]) => [k, p.default]));
  return value.map((item: unknown) =>
    typeof item === 'object' && item !== null && !Array.isArray(item)
      ? { ...defaults, ...(item as Record<string, unknown>) }
      : defaults,
  );
}
