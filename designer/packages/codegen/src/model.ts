/**
 * 言語に依らない中間表現(docs/designer/codegen-design.md §1・§4)。
 * DSL を、生成するメンバ・ハンドラと、beth_CreateComponents の中で実行する文の並びにする。
 * C++ と Python のエミッタは、この並びをそれぞれの書き方で書き出すだけにする(順序の決まりをここに集める)。
 */
import {
  findClass,
  getCatalog,
  isSubclassOf,
  setTypeOf,
  walkNodes,
  classOf,
  type EventParam,
  type NodeLocation,
  type BfmDocument,
  type PropertyInfo,
  type PropertyType,
} from '@bethany-designer/core';

/** プロパティに設定する値 */
export type Value =
  | { readonly kind: 'int' | 'float'; readonly value: number }
  | { readonly kind: 'bool'; readonly value: boolean }
  | { readonly kind: 'string' | 'char'; readonly value: string }
  /** 列挙型の要素・TColor・TCursor の定数 */
  | { readonly kind: 'name'; readonly name: string }
  /** ビット集合(C++ は `a | b`) */
  | { readonly kind: 'flags'; readonly type: string; readonly names: readonly string[] }
  /** Set<E>(C++ は `TAnchors() << a << b`) */
  | { readonly kind: 'set'; readonly type: string; readonly names: readonly string[] }
  /** "#RRGGBB" の色。value は Bethany の TColor($00BBGGRR) */
  | { readonly kind: 'rgb'; readonly value: number; readonly text: string }
  | { readonly kind: 'shortcut'; readonly text: string }
  /** 同じフォームのコンポーネント */
  | { readonly kind: 'ref'; readonly name: string };

/** 文の対象。undefined はフォーム自身 */
export type Target = string | undefined;

export type Statement =
  | { readonly kind: 'create'; readonly name: string; readonly class: string }
  /** target.path[0].path[1]... = value */
  | {
      readonly kind: 'assign';
      readonly target: Target;
      readonly path: readonly string[];
      readonly value: Value;
    }
  /** 親の設定(TTabSheet は PageControl、それ以外は Parent)。parent が undefined ならフォーム */
  | {
      readonly kind: 'parent';
      readonly target: string;
      readonly property: 'Parent' | 'PageControl';
      readonly parent: Target;
    }
  /** TStrings の中身。clear は既定の中身が空でないとき */
  | {
      readonly kind: 'strings';
      readonly target: Target;
      readonly property: string;
      readonly lines: readonly string[];
      readonly clear: boolean;
    }
  | {
      readonly kind: 'event';
      readonly target: Target;
      readonly event: string;
      readonly handler: string;
      readonly params: readonly EventParam[];
    }
  /** メニュー項目を親に加える。parentItem が undefined ならメニューのルート(menu.Items) */
  | {
      readonly kind: 'menuAdd';
      readonly item: string;
      readonly menu: string;
      readonly parentItem: string | undefined;
    }
  /** まとまりの区切り(空行) */
  | { readonly kind: 'blank' };

export interface Member {
  readonly name: string;
  readonly class: string;
}

export interface Handler {
  readonly name: string;
  readonly params: readonly EventParam[];
}

export interface FormModel {
  readonly className: string;
  readonly formName: string;
  /** 生成するメンバ(生成の順) */
  readonly members: readonly Member[];
  /** ハンドラ(最初に現れた順。同じ名前は 1 つ) */
  readonly handlers: readonly Handler[];
  /** beth_CreateComponents の中身 */
  readonly statements: readonly Statement[];
}

/** 検証を通過したドキュメントから中間表現を作る */
export function buildModel(doc: BfmDocument, className: string): FormModel {
  const nodes = [...walkNodes(doc)];
  const members = nodes
    .filter((n) => n.kind !== 'form')
    .map((n) => ({ name: n.node.name, class: classOf(n) }));
  const statements: Statement[] = members.map((m) => ({ kind: 'create', ...m }));
  const handlers = new Map<string, Handler>();
  const classByName = new Map(members.map((m) => [m.name, m.class]));
  /** コントロールへの参照(ActivePage・Associate)。相手の親が決まった後(すべてのコントロールの後)に設定する */
  const deferred: Statement[] = [];
  const isControlRef = (s: Statement) =>
    s.kind === 'assign' &&
    s.value.kind === 'ref' &&
    findClass(classByName.get(s.value.name) ?? '')?.kind === 'control';

  for (const location of nodes) {
    if (location.kind === 'component' && deferred.length > 0) {
      statements.push({ kind: 'blank' }, ...deferred.splice(0));
    }
    const block = nodeStatements(location);
    for (const s of block) {
      if (s.kind === 'event' && !handlers.has(s.handler))
        handlers.set(s.handler, { name: s.handler, params: s.params });
    }
    deferred.push(...block.filter(isControlRef));
    const rest = block.filter((s) => !isControlRef(s));
    if (rest.length > 0) statements.push({ kind: 'blank' }, ...rest);
  }
  if (deferred.length > 0) statements.push({ kind: 'blank' }, ...deferred);
  return {
    className,
    formName: doc.form.name,
    members,
    handlers: [...handlers.values()],
    statements,
  };
}

/** 1 つのノードの文(§4 の 2〜4) */
function nodeStatements(location: NodeLocation): Statement[] {
  const info = findClass(classOf(location));
  if (!info) throw new Error(`unknown class: ${classOf(location)}`);
  const target: Target = location.kind === 'form' ? undefined : location.node.name;
  const out: Statement[] = [];

  if (location.kind === 'control') {
    const parent = location.parent;
    const viaPageControl =
      isSubclassOf(location.node.class, 'TTabSheet') && isSubclassOf(parent.class, 'TPageControl');
    out.push({
      kind: 'parent',
      target: location.node.name,
      property: viaPageControl ? 'PageControl' : 'Parent',
      // フォームの直下のコントロールの親はフォーム自身(コントロールの class が TForm になることはない)
      parent: parent.class === 'TForm' ? undefined : parent.name,
    });
  }

  // プロパティはカタログの順(Left・Top・Width・Height が先頭)。Anchors は Parent と大きさが決まった後に設定する(ADR 0034)
  const properties = location.node.properties ?? {};
  const names = Object.keys(info.properties).filter((n) => Object.hasOwn(properties, n));
  const ordered = [
    ...names.filter((n) => n !== 'Anchors'),
    ...names.filter((n) => n === 'Anchors'),
  ];
  for (const name of ordered) {
    const prop = info.properties[name];
    if (prop) out.push(...propertyStatements(target, [name], prop, properties[name]));
  }

  const events = location.node.events ?? {};
  for (const [event, eventInfo] of Object.entries(info.events)) {
    const handler = events[event];
    if (handler === undefined) continue;
    out.push({
      kind: 'event',
      target,
      event,
      handler,
      params: getCatalog().eventTypes[eventInfo.type] ?? [],
    });
  }

  if (location.kind === 'menuItem') {
    out.push({
      kind: 'menuAdd',
      item: location.node.name,
      menu: location.menu.name,
      parentItem: location.parentItem?.name,
    });
  }
  return out;
}

function propertyStatements(
  target: Target,
  path: readonly string[],
  prop: PropertyInfo,
  value: unknown,
): Statement[] {
  const type = prop.type;
  if (type.kind === 'strings') {
    const clear = Array.isArray(prop.default) && prop.default.length > 0;
    return [
      {
        kind: 'strings',
        target,
        property: path.join('.'),
        lines: value as string[],
        clear,
      },
    ];
  }
  if (type.kind === 'object') {
    const known = getCatalog().objects[type.class]?.properties ?? {};
    const given = value as Record<string, unknown>;
    return Object.entries(known)
      .filter(([name]) => Object.hasOwn(given, name))
      .flatMap(([name, sub]) => propertyStatements(target, [...path, name], sub, given[name]));
  }
  return [{ kind: 'assign', target, path, value: toValue(type, value) }];
}

function toValue(type: PropertyType, value: unknown): Value {
  switch (type.kind) {
    case 'int':
    case 'float':
      return { kind: type.kind, value: value as number };
    case 'bool':
      return { kind: 'bool', value: value as boolean };
    case 'string':
    case 'char':
      return { kind: type.kind, value: value as string };
    case 'enum':
      return { kind: 'name', name: value as string };
    case 'flags':
      return { kind: 'flags', type: type.flags, names: value as string[] };
    case 'set':
      return { kind: 'set', type: setTypeOf(type.enum) ?? type.enum, names: value as string[] };
    case 'ref':
      return { kind: 'ref', name: value as string };
    case 'alias': {
      const text = value as string;
      if (type.alias === 'TShortCut') return { kind: 'shortcut', text };
      if (text.startsWith('#')) {
        const rgb = Number.parseInt(text.slice(1), 16);
        // "#RRGGBB" → $00BBGGRR
        const bgr = ((rgb & 0xff) << 16) | (rgb & 0xff00) | ((rgb >> 16) & 0xff);
        return { kind: 'rgb', value: bgr, text: text.toUpperCase() };
      }
      return { kind: 'name', name: text };
    }
    case 'strings':
    case 'object':
      throw new Error(`not a scalar: ${type.kind}`);
  }
}
