import type { JsonPath } from './diagnostics.ts';
import type {
  ComponentNode,
  ControlNode,
  FormNode,
  MenuItemNode,
  NvformDocument,
} from './schema.ts';

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
export function* walkNodes(doc: NvformDocument): Generator<NodeLocation> {
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
