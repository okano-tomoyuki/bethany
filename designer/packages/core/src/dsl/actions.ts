/**
 * Action(docs/adr/0046)とコントロール・メニュー項目の連動。
 * Action を割り当てると、LCL は Action のプロパティをコントロール・メニュー項目に写し(ActionChange)、以後も連動させる。
 * フォームのファイルに書いた同じプロパティは Action の値で上書きされるので、検証で警告し、表示には Action の値を使う。
 */
import { isSubclassOf } from '../catalog/catalog.ts';
import type { BfmDocument, Properties } from './schema.ts';
import { classOf, findNode, propertyValue, type NodeLocation } from './tree.ts';

/**
 * クラス className の Action が写すプロパティ → Action のプロパティ(LCL の ActionChange。TControl・TMenuItem・
 * TCustomBitBtn・TCustomSpeedButton・TToolButton)。Images は Action の ActionList の Images。
 */
export function actionLinkedProperties(className: string): ReadonlyMap<string, string> {
  const linked = new Map<string, string>([
    ['Caption', 'Caption'],
    ['Enabled', 'Enabled'],
    ['Hint', 'Hint'],
    ['Visible', 'Visible'],
  ]);
  if (className === 'TMenuItem')
    for (const name of ['AutoCheck', 'Checked', 'GroupIndex', 'ImageIndex', 'ShortCut'])
      linked.set(name, name);
  if (isSubclassOf(className, 'TCustomBitBtn') || isSubclassOf(className, 'TCustomSpeedButton')) {
    linked.set('Images', 'Images');
    linked.set('ImageIndex', 'ImageIndex');
  }
  if (isSubclassOf(className, 'TCustomSpeedButton')) linked.set('GroupIndex', 'GroupIndex');
  if (isSubclassOf(className, 'TToolButton')) {
    linked.set('Down', 'Checked');
    linked.set('ImageIndex', 'ImageIndex');
  }
  return linked;
}

/** properties(コントロール・メニュー項目のもの)の Action に書いた Action のノード。無い・見つからなければ undefined */
function actionOf(
  doc: BfmDocument,
  properties: Properties | undefined,
): (NodeLocation & { readonly kind: 'action' }) | undefined {
  const name = properties?.Action;
  if (typeof name !== 'string') return undefined;
  const found = findNode(doc, name);
  return found?.kind === 'action' ? found : undefined;
}

/** location に割り当てた Action のノード(Action が無い・見つからなければ undefined) */
export function assignedAction(
  doc: BfmDocument,
  location: NodeLocation,
): (NodeLocation & { readonly kind: 'action' }) | undefined {
  if (location.kind !== 'control' && location.kind !== 'menuItem') return undefined;
  return actionOf(doc, location.node.properties);
}

/**
 * クラス className のノードの properties に、割り当てた Action から写るプロパティを埋めたもの(表示用)。
 * Action が無ければ properties をそのまま返す。書いたプロパティも Action の値で置き換える(実行時と同じ)。
 */
export function withActionProperties(
  doc: BfmDocument,
  className: string,
  properties: Properties | undefined,
): Properties | undefined {
  const action = actionOf(doc, properties);
  if (!action) return properties;
  const result: Record<string, Properties[string]> = { ...properties };
  for (const [name, from] of actionLinkedProperties(className)) {
    const value =
      from === 'Images' ? action.list.properties?.Images : propertyValue(action, [from]);
    // Action が写すプロパティの名前(固定の集合)をキーとするため、動的な削除になる
    // eslint-disable-next-line @typescript-eslint/no-dynamic-delete
    if (value === undefined) delete result[name];
    else result[name] = value as Properties[string];
  }
  return result;
}

/** 表示用に、割り当てた Action から写るプロパティを埋めた location(Object Inspector・キャンバス用) */
export function withActionValues(doc: BfmDocument, location: NodeLocation): NodeLocation {
  if (location.kind !== 'control' && location.kind !== 'menuItem') return location;
  const properties = withActionProperties(doc, classOf(location), location.node.properties);
  if (properties === location.node.properties) return location;
  return { ...location, node: { ...location.node, properties } } as NodeLocation;
}
