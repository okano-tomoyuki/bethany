/**
 * UI から呼び出す編集操作。選択状態を踏まえてコマンドを組み立て、ドキュメントストアに渡す
 * (docs/designer/editor-design.md §3・§5)。
 */
import {
  collectMemberNames,
  defaultHandlerName,
  findClass,
  findNode,
  hasOwnBounds,
  nameBaseOfClass,
  nameBaseOfMenuCaption,
  nextName,
  type Bounds,
  type EditCommand,
  type NodeLocation,
  type NvformDocument,
  type PropertyChange,
  type PropertyValue,
} from '@no-vcl-designer/core';
import { documentStore, uiStore } from './store/stores.ts';
import { postMessage } from './vscode.ts';

function dispatch(command: EditCommand): boolean {
  return documentStore.getState().dispatch(command);
}

function currentDocument(): NvformDocument | undefined {
  return documentStore.getState().document;
}

/** 選択中のノードのうち、ドキュメントに存在するもの */
export function selectedLocations(): NodeLocation[] {
  const document = currentDocument();
  if (!document) return [];
  return uiStore.getState().selection.flatMap((name) => {
    const found = findNode(document, name);
    return found ? [found] : [];
  });
}

export function select(names: readonly string[]): void {
  uiStore.getState().select(names);
}

// ---- 追加 -------------------------------------------------------------------------

/** 子を置けるか(フォームと acceptsControls のコントロール) */
export function acceptsControls(location: NodeLocation): boolean {
  if (location.kind === 'form') return true;
  return location.kind === 'control' && (findClass(location.node.class)?.acceptsControls ?? false);
}

/** コントロールを parent の中の bounds に追加して選択する */
export function addControlAt(className: string, parent: string, bounds?: Bounds): void {
  const document = currentDocument();
  if (!document) return;
  const name = nextName(nameBaseOfClass(className), collectMemberNames(document));
  const command: EditCommand = {
    type: 'addControl',
    parent,
    className,
    name,
    ...(bounds && hasOwnBounds(className) && { bounds }),
  };
  if (dispatch(command)) select([name]);
}

/** 非ビジュアルコンポーネントをフォームの上の位置に追加して選択する */
export function addComponentAt(className: string, left: number, top: number): void {
  const document = currentDocument();
  if (!document) return;
  const name = nextName(nameBaseOfClass(className), collectMemberNames(document));
  if (dispatch({ type: 'addComponent', className, name, design: { left, top } })) select([name]);
}

/**
 * パレットのダブルクリック: 選択中のコンテナ(コンテナでなければその親、無ければフォーム)の既定の位置に追加する。
 * 非ビジュアルコンポーネントは、既存のアイコンと重ならない位置に置く
 */
export function addFromPalette(className: string): void {
  const document = currentDocument();
  const info = findClass(className);
  if (!document || !info) return;
  if (info.kind === 'component') {
    const used = new Set(
      (document.components ?? []).map((c) => `${String(c.design?.left)},${String(c.design?.top)}`),
    );
    let left = 16;
    while (used.has(`${String(left)},16`)) left += 40;
    addComponentAt(className, left, 16);
    return;
  }
  const [first] = selectedLocations();
  const parent =
    first && acceptsControls(first)
      ? first.node.name
      : first?.kind === 'control'
        ? first.parent.name
        : document.form.name;
  addControlAt(className, parent);
}

/** メニュー項目を追加して選択する。parent はメニューかメニュー項目 */
export function addMenuItem(parent: string, caption: string, index?: number): void {
  const document = currentDocument();
  if (!document) return;
  const name = nextName(nameBaseOfMenuCaption(caption), collectMemberNames(document));
  if (
    dispatch({ type: 'addMenuItem', parent, name, caption, ...(index !== undefined && { index }) })
  )
    select([name]);
}

// ---- 削除・選択 ---------------------------------------------------------------------

/** 選択中のノードを削除し、親(コントロールなら親、それ以外はフォーム)を選択する */
export function removeSelection(): void {
  const document = currentDocument();
  const locations = selectedLocations().filter((l) => l.kind !== 'form');
  const [first] = locations;
  if (!document || !first) return;
  const next =
    first.kind === 'control'
      ? first.parent.name
      : first.kind === 'menuItem'
        ? (first.parentItem?.name ?? first.menu.name)
        : document.form.name;
  if (dispatch({ type: 'removeNodes', names: locations.map((l) => l.node.name) })) select([next]);
}

/** 選択中のノードの親を選択する */
export function selectParent(): void {
  const [first] = selectedLocations();
  if (!first) return;
  if (first.kind === 'control') select([first.parent.name]);
  else if (first.kind === 'menuItem') select([first.parentItem?.name ?? first.menu.name]);
}

// ---- 位置と大きさ -------------------------------------------------------------------

export interface BoundsChange {
  readonly name: string;
  readonly bounds: Bounds;
}

/** コントロールの位置と大きさを変える(変わった値だけを書く)。parent を指定すると、その親へ移す */
export function setBounds(
  changes: readonly BoundsChange[],
  parent?: { readonly name: string; readonly index?: number },
): void {
  const document = currentDocument();
  if (!document || changes.length === 0) return;
  const commands: EditCommand[] = [];
  if (parent)
    commands.push({
      type: 'moveControls',
      names: changes.map((c) => c.name),
      parent: parent.name,
      ...(parent.index !== undefined && { index: parent.index }),
    });
  const properties: PropertyChange[] = [];
  for (const { name, bounds } of changes) {
    const location = findNode(document, name);
    if (location?.kind !== 'control' || !hasOwnBounds(location.node.class)) continue;
    const current = location.node.properties ?? {};
    const entries: [string, number][] = [
      ['Left', bounds.left],
      ['Top', bounds.top],
      ['Width', bounds.width],
      ['Height', bounds.height],
    ];
    for (const [key, value] of entries)
      if (current[key] !== value) properties.push({ node: name, path: [key], value });
  }
  if (properties.length > 0) commands.push({ type: 'setProperties', changes: properties });
  if (commands.length === 0) return;
  dispatch(commands.length === 1 && commands[0] ? commands[0] : { type: 'batch', commands });
}

/** キーボードでの移動・大きさの変更(選択中のコントロール) */
export function nudgeSelection(dx: number, dy: number, resize: boolean): void {
  const changes = selectedLocations().flatMap((location): BoundsChange[] => {
    if (location.kind !== 'control' || !hasOwnBounds(location.node.class)) return [];
    const b = boundsOf(location);
    return [
      {
        name: location.node.name,
        bounds: resize
          ? { ...b, width: Math.max(0, b.width + dx), height: Math.max(0, b.height + dy) }
          : { ...b, left: b.left + dx, top: b.top + dy },
      },
    ];
  });
  setBounds(changes);
}

/** コントロールに書いた位置と大きさ(書いていなければ 0) */
export function boundsOf(location: NodeLocation): Bounds {
  const p = location.node.properties ?? {};
  const num = (v: PropertyValue | undefined) => (typeof v === 'number' ? v : 0);
  return { left: num(p.Left), top: num(p.Top), width: num(p.Width), height: num(p.Height) };
}

/** コントロールの並び(同じ親の中)を前後の端へ移す(前面へ・背面へ) */
export function moveToEdge(front: boolean): void {
  const locations = selectedLocations().filter((l) => l.kind === 'control');
  const [first] = locations;
  if (first?.kind !== 'control') return;
  const count = first.parent.controls?.length ?? 0;
  dispatch({
    type: 'moveControls',
    names: locations.map((l) => l.node.name),
    parent: first.parent.name,
    index: front ? count : 0,
  });
}

// ---- 名前・プロパティ・イベント -----------------------------------------------------------

/** 改名し、選択を追従させる */
export function renameNode(name: string, newName: string): string | undefined {
  if (!dispatch({ type: 'renameNode', name, newName })) return documentStore.getState().lastError;
  select(uiStore.getState().selection.map((n) => (n === name ? newName : n)));
  return undefined;
}

/** 選択中のノード(複数)のプロパティを設定する。value が undefined なら削除する */
export function setProperty(
  names: readonly string[],
  path: PropertyChange['path'],
  value: PropertyValue | undefined,
): string | undefined {
  const changes = names.map((node) => ({ node, path, value }));
  return dispatch({ type: 'setProperties', changes })
    ? undefined
    : documentStore.getState().lastError;
}

/** イベントのハンドラを設定する。handler が undefined なら外す */
export function setEvent(
  names: readonly string[],
  event: string,
  handler: string | undefined,
): string | undefined {
  const commands: EditCommand[] = names.map((node) => ({ type: 'setEvent', node, event, handler }));
  const ok = dispatch(
    commands.length === 1 && commands[0] ? commands[0] : { type: 'batch', commands },
  );
  return ok ? undefined : documentStore.getState().lastError;
}

/** 既定のハンドラ名(OkButtonClick)でイベントを設定する(既に設定されていれば何もしない) */
export function setDefaultHandler(location: NodeLocation, event: string): void {
  if (location.node.events?.[event] !== undefined) return;
  setEvent([location.node.name], event, defaultHandlerName(location.node.name, event));
}

export function renameHandler(name: string, newName: string): string | undefined {
  return dispatch({ type: 'renameHandler', name, newName })
    ? undefined
    : documentStore.getState().lastError;
}

export function setDesignPosition(name: string, left: number, top: number): void {
  dispatch({ type: 'setDesignPosition', name, left, top });
}

// ---- コード生成 ---------------------------------------------------------------------

export function generateCode(): void {
  postMessage({ type: 'generateCode' });
}
