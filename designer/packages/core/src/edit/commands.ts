/**
 * ドキュメントの編集コマンド(docs/designer/editor-design.md §3.1。tk-designer ADR 0006 と同じ方式)。
 * コマンドは純粋な関数 applyCommand(doc, command) で適用する。Webview(楽観的な反映)と拡張(実際の反映)の両方で同じ関数を使う。
 * コマンドは「どのノードをどう変えるか」という意図で表し(ノードは name で指す)、テキスト上の位置には依存しない。
 */
import { produce } from 'immer';
import { findClass, getCatalog, isSubclassOf } from '../catalog/catalog.ts';
import type { PropertyInfo } from '../catalog/types.ts';
import { childProblem } from '../dsl/constraints.ts';
import type { CodegenSettings, BfmDocument, PropertyValue } from '../dsl/schema.ts';
import { memberNameProblem } from '../identifier.ts';
import { l10n } from '../l10n.ts';
import { computeLayout } from '../layout/engine.ts';
import { initialProperties } from './defaults.ts';
import { collectHandlerNames, collectMemberNames } from './naming.ts';

/** コントロールの位置と大きさ */
export interface Bounds {
  readonly left: number;
  readonly top: number;
  readonly width: number;
  readonly height: number;
}

/** プロパティの変更 1 つ。path は ["Caption"] か、入れ子のオブジェクトの中なら ["Font", "Size"] */
export interface PropertyChange {
  readonly node: string;
  readonly path: readonly [string] | readonly [string, string];
  /** undefined なら削除する(既定値に戻す) */
  readonly value: PropertyValue | undefined;
}

export type EditCommand =
  /** name は呼び出し側で nextName() により決める(楽観的な反映と実際の反映で同じ名前にするため) */
  | {
      readonly type: 'addControl';
      /** 親(フォームかコントロール)の name */
      readonly parent: string;
      readonly className: string;
      readonly name: string;
      /** 省略時は既定の位置(8, 8)とカタログの既定の大きさ */
      readonly bounds?: Bounds;
      /** 親の controls の中の位置。省略時は末尾 */
      readonly index?: number;
    }
  /** TMainMenu は、フォームの Menu が未設定ならそこにも設定する */
  | {
      readonly type: 'addComponent';
      readonly className: string;
      readonly name: string;
      readonly design?: { readonly left: number; readonly top: number };
    }
  | {
      readonly type: 'addMenuItem';
      /** メニュー(TMainMenu・TPopupMenu)かメニュー項目の name */
      readonly parent: string;
      readonly name: string;
      readonly caption: string;
      readonly index?: number;
    }
  /** ノードを子孫ごと削除し、削除したものへの参照(プロパティ)も削除する */
  | { readonly type: 'removeNodes'; readonly names: readonly string[] }
  /**
   * コントロールを parent の controls の index の位置へ移す(並びは names の順)。
   * index は移すものを取り除いた後の位置。省略時は末尾
   */
  | {
      readonly type: 'moveControls';
      readonly names: readonly string[];
      readonly parent: string;
      readonly index?: number;
    }
  /** メニュー項目を parent(メニューかメニュー項目)の items の index の位置へ移す */
  | {
      readonly type: 'moveMenuItem';
      readonly name: string;
      readonly parent: string;
      readonly index?: number;
    }
  /** 改名し、そのノードへの参照(プロパティ)も書き換える */
  | { readonly type: 'renameNode'; readonly name: string; readonly newName: string }
  | { readonly type: 'setProperties'; readonly changes: readonly PropertyChange[] }
  /** handler が undefined ならイベントを外す */
  | {
      readonly type: 'setEvent';
      readonly node: string;
      readonly event: string;
      readonly handler: string | undefined;
    }
  /** ハンドラを改名し、すべての参照を書き換える。型が同じ既存のハンドラへの改名は統合として許す */
  | { readonly type: 'renameHandler'; readonly name: string; readonly newName: string }
  | {
      readonly type: 'setDesignPosition';
      readonly name: string;
      readonly left: number;
      readonly top: number;
    }
  /** 空なら codegen ごと削除する */
  | { readonly type: 'setCodegen'; readonly codegen: CodegenSettings }
  /** 複数のコマンドを 1 つの変更としてまとめて適用する(途中で失敗したら何も変えない。Undo も 1 回) */
  | { readonly type: 'batch'; readonly commands: readonly EditCommand[] };

export type CommandResult =
  | { readonly ok: true; readonly document: BfmDocument }
  | { readonly ok: false; readonly error: string };

class CommandError extends Error {}

// ---- 下書きの型 -----------------------------------------------------------------

/** 編集中の(Immer の下書きの)ノード。DSL の型は readonly なので、書き換えられる形で扱う */
interface Node {
  name: string;
  class?: string;
  design?: { left: number; top: number };
  properties?: Record<string, PropertyValue>;
  events?: Record<string, string>;
  controls?: Node[];
  items?: Node[];
}

interface Doc {
  codegen?: CodegenSettings;
  form: Node;
  components?: Node[];
}

type Kind = 'form' | 'control' | 'component' | 'menuItem';

interface Found {
  readonly node: Node;
  readonly kind: Kind;
  /** 親の配列(フォームは undefined) */
  readonly siblings: Node[] | undefined;
  /** 親のノード(フォーム・非ビジュアルコンポーネントは undefined) */
  readonly parent: Node | undefined;
}

const produceDocument = produce as (base: BfmDocument, recipe: (draft: Doc) => void) => BfmDocument;

export function applyCommand(doc: BfmDocument, command: EditCommand): CommandResult {
  try {
    const edited = produceDocument(doc, (draft) => {
      apply(draft, command);
    });
    return { ok: true, document: relayout(doc, edited) };
  } catch (e) {
    if (e instanceof CommandError) return { ok: false, error: e.message };
    throw e;
  }
}

/**
 * 配置を計算し直し、Align で寄せたコントロールと Anchors で追従するコントロールの位置と大きさを書き換える
 * (docs/designer/editor-design.md §3.1・§4)。Webview と拡張で同じ結果になるよう、コマンドの適用の中で行う。
 */
function relayout(before: BfmDocument, after: BfmDocument): BfmDocument {
  const changes = computeLayout(before, after);
  if (changes.size === 0) return after;
  return produceDocument(after, (draft) => {
    for (const found of walk(draft)) {
      const r = found.kind === 'control' ? changes.get(found.node.name) : undefined;
      if (!r) continue;
      const properties = (found.node.properties ??= {});
      properties.Left = r.left;
      properties.Top = r.top;
      properties.Width = r.width;
      properties.Height = r.height;
    }
  });
}

function apply(doc: Doc, command: EditCommand): void {
  switch (command.type) {
    case 'addControl': {
      const parent = requireNode(doc, command.parent);
      if (parent.kind !== 'form' && parent.kind !== 'control')
        throw new CommandError(l10n.t('{0} cannot have controls', classOf(parent)));
      const info = findClass(command.className);
      if (info?.kind !== 'control')
        throw new CommandError(l10n.t('{0} is not a control in the catalog', command.className));
      requireChildAllowed(classOf(parent), command.className);
      requireNewName(doc, command.name);
      const node: Node = {
        name: command.name,
        class: command.className,
        ...nonEmpty(
          'properties',
          initialProperties(command.className, command.name, command.bounds),
        ),
      };
      insert((parent.node.controls ??= []), node, command.index);
      return;
    }

    case 'addComponent': {
      const info = findClass(command.className);
      if (info?.kind !== 'component')
        throw new CommandError(
          l10n.t('{0} is not a non-visual component in the catalog', command.className),
        );
      requireNewName(doc, command.name);
      const node: Node = { name: command.name, class: command.className };
      if (command.design) node.design = { left: command.design.left, top: command.design.top };
      if (isSubclassOf(command.className, 'TMenu')) node.items = [];
      (doc.components ??= []).push(node);
      // C++Builder と同じく、最初の TMainMenu はフォームのメニューにする
      if (isSubclassOf(command.className, 'TMainMenu') && doc.form.properties?.Menu === undefined)
        (doc.form.properties ??= {}).Menu = command.name;
      return;
    }

    case 'addMenuItem': {
      const parent = requireMenuParent(doc, command.parent);
      requireNewName(doc, command.name);
      const node: Node = { name: command.name, properties: { Caption: command.caption } };
      insert((parent.items ??= []), node, command.index);
      return;
    }

    case 'removeNodes': {
      const removed = new Set<string>();
      for (const name of command.names) {
        const found = findNode(doc, name);
        // 先に削除した親の子孫は、もう見つからない
        if (!found) {
          if (removed.has(name)) continue;
          throw notFound(name);
        }
        if (found.kind === 'form') throw new CommandError(l10n.t('The form cannot be deleted'));
        for (const n of descendantsOrSelf(found.node)) removed.add(n.name);
        found.siblings?.splice(found.siblings.indexOf(found.node), 1);
        if (found.parent)
          dropEmptyArray(found.parent, found.kind === 'control' ? 'controls' : 'items');
      }
      if (doc.components?.length === 0) delete doc.components;
      forEachReference(doc, (holder, key, target) => {
        if (removed.has(target)) deleteKey(holder, key);
      });
      return;
    }

    case 'moveControls': {
      const parent = requireNode(doc, command.parent);
      if (parent.kind !== 'form' && parent.kind !== 'control')
        throw new CommandError(l10n.t('{0} cannot have controls', classOf(parent)));
      const moving = command.names.map((name) => {
        const found = requireNode(doc, name);
        if (found.kind !== 'control')
          throw new CommandError(l10n.t('"{0}" is not a control', name));
        if (descendantsOrSelf(found.node).includes(parent.node))
          throw new CommandError(
            l10n.t('A control cannot be moved into itself or its descendants'),
          );
        requireChildAllowed(classOf(parent), found.node.class ?? '');
        return found;
      });
      for (const found of moving) {
        found.siblings?.splice(found.siblings.indexOf(found.node), 1);
        if (found.parent && found.parent !== parent.node) dropEmptyArray(found.parent, 'controls');
      }
      const controls = (parent.node.controls ??= []);
      const at = clampIndex(command.index, controls.length);
      controls.splice(at, 0, ...moving.map((f) => f.node));
      return;
    }

    case 'moveMenuItem': {
      const found = requireNode(doc, command.name);
      if (found.kind !== 'menuItem')
        throw new CommandError(l10n.t('"{0}" is not a menu item', command.name));
      const parent = requireMenuParent(doc, command.parent);
      if (descendantsOrSelf(found.node).includes(parent))
        throw new CommandError(
          l10n.t('A menu item cannot be moved into itself or its descendants'),
        );
      found.siblings?.splice(found.siblings.indexOf(found.node), 1);
      if (found.parent && found.parent !== parent) dropEmptyArray(found.parent, 'items');
      insert((parent.items ??= []), found.node, command.index);
      return;
    }

    case 'renameNode': {
      const found = requireNode(doc, command.name);
      if (command.newName === command.name) return;
      requireNewName(doc, command.newName);
      found.node.name = command.newName;
      forEachReference(doc, (holder, key, target) => {
        if (target === command.name) holder[key] = command.newName;
      });
      return;
    }

    case 'setProperties': {
      for (const change of command.changes) setProperty(doc, change);
      return;
    }

    case 'setEvent': {
      const found = requireNode(doc, command.node);
      const events = findClass(classOf(found))?.events;
      const eventInfo =
        events && Object.hasOwn(events, command.event) ? events[command.event] : undefined;
      if (!eventInfo)
        throw new CommandError(l10n.t('{0} has no event {1}', classOf(found), command.event));
      if (command.handler === undefined) {
        deleteKey((found.node.events ??= {}), command.event);
        if (Object.keys(found.node.events).length === 0) delete found.node.events;
        return;
      }
      requireHandlerName(doc, command.handler, eventInfo.type, [found.node, command.event]);
      (found.node.events ??= {})[command.event] = command.handler;
      return;
    }

    case 'renameHandler': {
      if (command.newName === command.name) return;
      const type = handlerType(doc, command.name);
      if (type === undefined)
        throw new CommandError(l10n.t('Handler "{0}" not found', command.name));
      requireHandlerName(doc, command.newName, type, undefined, command.name);
      for (const node of allNodes(doc))
        for (const [event, handler] of Object.entries(node.events ?? {}))
          if (handler === command.name) (node.events ?? {})[event] = command.newName;
      return;
    }

    case 'setDesignPosition': {
      const found = requireNode(doc, command.name);
      if (found.kind !== 'component')
        throw new CommandError(l10n.t('"{0}" is not a non-visual component', command.name));
      found.node.design = { left: command.left, top: command.top };
      return;
    }

    case 'setCodegen': {
      const codegen = withoutUndefined(command.codegen);
      if (Object.keys(codegen).length === 0) delete doc.codegen;
      else doc.codegen = codegen;
      return;
    }

    case 'batch': {
      // produce の中で順に適用する。途中で例外になれば produce ごと破棄される
      for (const inner of command.commands) apply(doc, inner);
      return;
    }
  }
}

// ---- プロパティ -----------------------------------------------------------------

function setProperty(doc: Doc, change: PropertyChange): void {
  const found = requireNode(doc, change.node);
  const className = classOf(found);
  const [name, sub] = change.path;
  const info = propertyOf(findClass(className)?.properties, name);
  if (!info)
    throw new CommandError(
      l10n.t('{0} has no property {1} that can be set at design time', className, name),
    );
  const properties = (found.node.properties ??= {});

  if (sub === undefined) {
    if (change.value === undefined) deleteKey(properties, name);
    else properties[name] = change.value;
  } else {
    if (info.type.kind !== 'object')
      throw new CommandError(l10n.t('{0} is not an object property', name));
    const objectClass = info.type.class;
    if (!propertyOf(getCatalog().objects[objectClass]?.properties, sub))
      throw new CommandError(l10n.t('{0} has no property {1}', objectClass, sub));
    const current = properties[name];
    const object: Record<string, PropertyValue> =
      typeof current === 'object' && current !== null && !Array.isArray(current) ? current : {};
    if (change.value === undefined) deleteKey(object, sub);
    else object[sub] = change.value;
    if (Object.keys(object).length === 0) deleteKey(properties, name);
    else properties[name] = object;
  }
  if (Object.keys(properties).length === 0) delete found.node.properties;
}

function propertyOf(
  known: Readonly<Record<string, PropertyInfo>> | undefined,
  name: string,
): PropertyInfo | undefined {
  return known && Object.hasOwn(known, name) ? known[name] : undefined;
}

/**
 * 参照のプロパティ(型が ref のもの)を列挙する。target は参照先の name。
 * holder[key] を書き換える・削除することで参照を変えられる。
 */
function forEachReference(
  doc: Doc,
  visit: (holder: Record<string, PropertyValue>, key: string, target: string) => void,
): void {
  for (const found of allFound(doc)) {
    const properties = found.node.properties;
    if (!properties) continue;
    const known = findClass(classOf(found))?.properties;
    for (const [key, value] of Object.entries(properties)) {
      if (propertyOf(known, key)?.type.kind === 'ref' && typeof value === 'string')
        visit(properties, key, value);
    }
    if (Object.keys(properties).length === 0) delete found.node.properties;
  }
}

// ---- ハンドラ -------------------------------------------------------------------

/** ハンドラ名の型(最初に使っているイベントの型)。使われていなければ undefined */
function handlerType(doc: Doc, handler: string): string | undefined {
  for (const found of allFound(doc)) {
    const events = findClass(classOf(found))?.events;
    for (const [event, name] of Object.entries(found.node.events ?? {}))
      if (name === handler && events && Object.hasOwn(events, event)) return events[event]?.type;
  }
  return undefined;
}

/**
 * handler を型 type のイベントに使えるか。既存のハンドラ名なら型が同じであること(統合)、
 * そうでなければメンバ名として使えて、ノードの name と重複しないこと。
 * @param except 設定しようとしているイベント自身(その値は型の比較に含めない)
 * @param renaming 改名の元の名前(改名後の名前の検査では、元の名前の使用を除く)
 */
function requireHandlerName(
  doc: Doc,
  handler: string,
  type: string,
  except: readonly [Node, string] | undefined,
  renaming?: string,
): void {
  const problem = memberNameProblem(handler);
  if (problem)
    throw new CommandError(l10n.t('"{0}" cannot be used as a name: {1}', handler, problem));
  if (allNodes(doc).some((n) => n.name === handler))
    throw new CommandError(
      l10n.t('The handler name "{0}" is also the name of a component', handler),
    );
  for (const found of allFound(doc)) {
    const events = findClass(classOf(found))?.events;
    for (const [event, name] of Object.entries(found.node.events ?? {})) {
      if (name !== handler || name === renaming) continue;
      if (except && except[0] === found.node && except[1] === event) continue;
      const other = events && Object.hasOwn(events, event) ? events[event]?.type : undefined;
      if (other !== undefined && other !== type)
        throw new CommandError(
          l10n.t(
            'The handler "{0}" is used for events of different types ({1} and {2})',
            handler,
            other,
            type,
          ),
        );
    }
  }
}

// ---- ノード ---------------------------------------------------------------------

function* walk(doc: Doc): Generator<Found> {
  yield { node: doc.form, kind: 'form', siblings: undefined, parent: undefined };
  yield* walkControls(doc.form);
  for (const component of doc.components ?? []) {
    yield { node: component, kind: 'component', siblings: doc.components, parent: undefined };
    yield* walkItems(component);
  }
}

function* walkControls(parent: Node): Generator<Found> {
  for (const node of parent.controls ?? []) {
    yield { node, kind: 'control', siblings: parent.controls, parent };
    yield* walkControls(node);
  }
}

function* walkItems(parent: Node): Generator<Found> {
  for (const node of parent.items ?? []) {
    yield { node, kind: 'menuItem', siblings: parent.items, parent };
    yield* walkItems(node);
  }
}

function allFound(doc: Doc): Found[] {
  return [...walk(doc)];
}

function allNodes(doc: Doc): Node[] {
  return allFound(doc).map((f) => f.node);
}

function findNode(doc: Doc, name: string): Found | undefined {
  for (const found of walk(doc)) if (found.node.name === name) return found;
  return undefined;
}

function requireNode(doc: Doc, name: string): Found {
  const found = findNode(doc, name);
  if (!found) throw notFound(name);
  return found;
}

function notFound(name: string): CommandError {
  return new CommandError(l10n.t('"{0}" not found', name));
}

/** メニュー項目を置ける親(TMainMenu・TPopupMenu か、メニュー項目) */
function requireMenuParent(doc: Doc, name: string): Node {
  const found = requireNode(doc, name);
  if (found.kind === 'menuItem') return found.node;
  if (found.kind === 'component' && isSubclassOf(found.node.class ?? '', 'TMenu'))
    return found.node;
  throw new CommandError(l10n.t('{0} cannot have menu items', classOf(found)));
}

function classOf(found: Found): string {
  return found.kind === 'menuItem' ? 'TMenuItem' : (found.node.class ?? '');
}

function descendantsOrSelf(node: Node): Node[] {
  return [
    node,
    ...(node.controls ?? []).flatMap(descendantsOrSelf),
    ...(node.items ?? []).flatMap(descendantsOrSelf),
  ];
}

function requireChildAllowed(parentClass: string, childClass: string): void {
  const problem = childProblem(parentClass, childClass);
  if (problem) throw new CommandError(problem.message);
}

function requireNewName(doc: Doc, name: string): void {
  const problem = memberNameProblem(name);
  if (problem) throw new CommandError(l10n.t('"{0}" cannot be used as a name: {1}', name, problem));
  const document = doc as unknown as BfmDocument;
  if (collectMemberNames(document).has(name)) {
    throw new CommandError(
      collectHandlerNames(document).has(name)
        ? l10n.t('"{0}" is already used as a handler name', name)
        : l10n.t('"{0}" is already used', name),
    );
  }
}

// ---- 補助 -----------------------------------------------------------------------

function insert(list: Node[], node: Node, index: number | undefined): void {
  list.splice(clampIndex(index, list.length), 0, node);
}

function clampIndex(index: number | undefined, length: number): number {
  return index === undefined ? length : Math.max(0, Math.min(index, length));
}

/** 空になった controls・items を取り除く(メニューの items は空でも残す) */
function dropEmptyArray(node: Node, key: 'controls' | 'items'): void {
  if (key === 'controls' && node.controls?.length === 0) delete node.controls;
  if (key === 'items' && node.items?.length === 0 && node.class === undefined) delete node.items;
}

function deleteKey(record: Record<string, unknown>, key: string): void {
  // 利用者が決めた名前をキーとするため、動的な削除になる
  // eslint-disable-next-line @typescript-eslint/no-dynamic-delete
  delete record[key];
}

function nonEmpty<K extends string, V extends object>(key: K, value: V): Partial<Record<K, V>> {
  return Object.keys(value).length > 0 ? ({ [key]: value } as Record<K, V>) : {};
}

function withoutUndefined<T extends object>(value: T): T {
  return Object.fromEntries(Object.entries(value).filter(([, v]) => v !== undefined)) as T;
}
