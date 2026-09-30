/**
 * プロパティのタブ(docs/designer/editor-design.md §6)。カタログの順に、型に応じた欄を並べる。
 * 複数選択では共通するプロパティだけを表示し、値が違うものは空欄にする(入力すると全部に設定する)。
 */
import {
  classOf,
  enumItems,
  findClass,
  formatValue,
  getCatalog,
  isSubclassOf,
  l10n,
  parseInput,
  propertyValue,
  type ControlNode,
  type NodeLocation,
  type BfmDocument,
  type PropertyInfo,
  type PropertyType,
  type PropertyValue,
  withActionValues,
} from '@bethany-designer/core';
import { useState } from 'react';
import { setProperty } from '../../editing.ts';
import { useDocumentStore } from '../../store/stores.ts';
import { colorToCss } from '../../canvas/look.ts';
import { messagesAt } from './diagnostics.ts';
import { useCommittedDraft } from './fields.tsx';

type Path = readonly [string] | readonly [string, string];

const BOUNDS: ReadonlySet<string> = new Set(['Left', 'Top', 'Width', 'Height']);

/** よく使うショートカット(TShortCut の欄の候補) */
const SHORTCUTS = [
  'Ctrl+N',
  'Ctrl+O',
  'Ctrl+S',
  'Ctrl+P',
  'Ctrl+Z',
  'Ctrl+Y',
  'Ctrl+X',
  'Ctrl+C',
  'Ctrl+V',
  'Ctrl+A',
  'Ctrl+F',
  'F1',
  'F2',
  'F3',
  'F5',
  'Del',
  'Alt+F4',
];

export function PropertyGrid({
  nodes,
  filter,
  onlySet,
}: {
  readonly nodes: readonly NodeLocation[];
  readonly filter: string;
  readonly onlySet: boolean;
}) {
  const document = useDocumentStore((s) => s.document);
  const diagnostics = useDocumentStore((s) => s.diagnostics);
  if (!document) return null;
  const properties = commonProperties(nodes);
  const query = filter.trim().toLowerCase();
  const shown = properties.filter(
    ([name]) =>
      (query === '' || name.toLowerCase().includes(query)) &&
      (!onlySet || nodes.some((n) => n.node.properties?.[name] !== undefined)),
  );
  const [single] = nodes.length === 1 ? nodes : [];
  // 表示する値: Action を割り当てたものは、Action から写る値(docs/adr/0046)
  const display = nodes.map((n) => withActionValues(document, n));

  return (
    <div className="property-grid" role="table">
      {shown.map(([name, info]) => (
        <PropertyRow
          key={name}
          document={document}
          nodes={nodes}
          display={display}
          path={[name]}
          info={info}
          error={single && messagesAt(diagnostics, [...single.path, 'properties', name])}
        />
      ))}
      {shown.length === 0 && <p className="muted">{l10n.t('No properties')}</p>}
    </div>
  );
}

/** 選択したノードに共通するプロパティ(先頭のノードのカタログの順。型が同じものだけ) */
function commonProperties(nodes: readonly NodeLocation[]): [string, PropertyInfo][] {
  const [first, ...rest] = nodes;
  if (!first) return [];
  const known = findClass(classOf(first))?.properties ?? {};
  return Object.entries(known).filter(([name, info]) =>
    rest.every((n) => {
      const other = findClass(classOf(n))?.properties;
      return (
        other &&
        Object.hasOwn(other, name) &&
        JSON.stringify(other[name]?.type) === JSON.stringify(info.type)
      );
    }),
  );
}

function PropertyRow({
  document,
  nodes,
  display,
  path,
  info,
  error,
}: {
  readonly document: BfmDocument;
  readonly nodes: readonly NodeLocation[];
  /** 値の表示に使うノード(Action から写る値を埋めたもの) */
  readonly display: readonly NodeLocation[];
  readonly path: Path;
  readonly info: PropertyInfo;
  readonly error: string | undefined;
}) {
  const [expanded, setExpanded] = useState(false);
  const { type } = info;
  const names = nodes.map((n) => n.node.name);
  const own = nodes.map((n) => ownValue(n, path));
  const isSet = own.some((v) => v !== undefined);
  const values = display.map((n) => propertyValue(n, path));
  const mixed = values.some((v) => JSON.stringify(v) !== JSON.stringify(values[0]));
  const value = mixed ? undefined : values[0];
  const label = path[path.length - 1] ?? '';
  // 既定値と同じ値は書かない(位置と大きさは、Align で寄せたものも含めて常に書く。dsl-spec.md §5)
  const commit = (next: PropertyValue | undefined) =>
    setProperty(
      names,
      path,
      next !== undefined &&
        !BOUNDS.has(path[0]) &&
        path.length === 1 &&
        JSON.stringify(next) === JSON.stringify(info.default)
        ? undefined
        : next,
    );
  const expandable =
    type.kind === 'object' ||
    type.kind === 'set' ||
    type.kind === 'flags' ||
    type.kind === 'strings' ||
    type.kind === 'collection';

  const rowClass = [
    'prop-row',
    isSet ? 'set' : '',
    error ? 'has-error' : '',
    path.length > 1 ? 'nested' : '',
  ]
    .filter(Boolean)
    .join(' ');

  return (
    <>
      <div className={rowClass} role="row">
        <span className="prop-name" role="cell" title={info.doc}>
          {expandable ? (
            <button
              type="button"
              className="prop-toggle"
              aria-expanded={expanded}
              onClick={() => {
                setExpanded(!expanded);
              }}
            >
              {expanded ? '▾' : '▸'} {label}
            </button>
          ) : (
            label
          )}
        </span>
        <span className="prop-value" role="cell">
          <Editor
            document={document}
            type={type}
            value={value}
            mixed={mixed}
            fallback={info.default}
            commit={commit}
          />
          {isSet && (
            <button
              type="button"
              className="prop-reset"
              title={l10n.t('Reset to the default')}
              onClick={() => commit(undefined)}
            >
              ×
            </button>
          )}
        </span>
      </div>
      {error && <div className="field-error prop-error">{error}</div>}
      {expanded && (
        <Expanded
          document={document}
          nodes={nodes}
          display={display}
          path={path}
          type={type}
          value={value}
          mixed={mixed}
          commit={commit}
        />
      )}
    </>
  );
}

/** 展開した中身: 入れ子のオブジェクトのプロパティ、集合の要素のチェック、TStrings の複数行の入力、コレクションの項目 */
function Expanded({
  document,
  nodes,
  display,
  path,
  type,
  value,
  mixed,
  commit,
}: {
  readonly document: BfmDocument;
  readonly nodes: readonly NodeLocation[];
  readonly display: readonly NodeLocation[];
  readonly path: Path;
  readonly type: PropertyType;
  readonly value: unknown;
  readonly mixed: boolean;
  readonly commit: (value: PropertyValue | undefined) => string | undefined;
}) {
  const catalog = getCatalog();
  switch (type.kind) {
    case 'object':
      return (
        <div className="prop-children">
          {Object.entries(catalog.objects[type.class]?.properties ?? {}).map(([sub, info]) => (
            <PropertyRow
              key={sub}
              document={document}
              nodes={nodes}
              display={display}
              path={[path[0], sub]}
              info={info}
              error={undefined}
            />
          ))}
        </div>
      );
    case 'set':
    case 'flags': {
      const items =
        (type.kind === 'flags' ? catalog.flags[type.flags] : catalog.enums[type.enum]) ?? [];
      const current = Array.isArray(value) ? (value as string[]) : [];
      return (
        <div className="prop-children">
          {items.map((item) => (
            <label key={item} className="prop-check">
              <input
                type="checkbox"
                checked={current.includes(item)}
                onChange={(e) => {
                  const next = e.target.checked
                    ? items.filter((i) => i === item || current.includes(i))
                    : current.filter((i) => i !== item);
                  commit(next);
                }}
              />
              {item}
            </label>
          ))}
        </div>
      );
    }
    case 'strings':
      return <StringsEditor value={value} commit={commit} />;
    case 'collection':
      return mixed ? (
        <div className="prop-children muted">{l10n.t('(different values)')}</div>
      ) : (
        <CollectionEditor document={document} itemClass={type.item} value={value} commit={commit} />
      );
    default:
      return null;
  }
}

type Item = Readonly<Record<string, PropertyValue>>;

/**
 * コレクション(TStatusBar の Panels 等。docs/adr/0044)の項目の一覧。項目の追加・削除・並べ替えと、開いた項目のプロパティの編集。
 * どの操作も、配列全体を 1 つのプロパティの値として設定する(元に戻すと 1 操作ずつ戻る)。
 */
function CollectionEditor({
  document,
  itemClass,
  value,
  commit,
}: {
  readonly document: BfmDocument;
  readonly itemClass: string;
  readonly value: unknown;
  readonly commit: (value: PropertyValue | undefined) => string | undefined;
}) {
  const [open, setOpen] = useState<number | undefined>(undefined);
  const known = getCatalog().objects[itemClass]?.properties ?? {};
  const items: readonly Item[] = Array.isArray(value)
    ? (value as unknown[]).map((v) =>
        typeof v === 'object' && v !== null && !Array.isArray(v) ? (v as Item) : {},
      )
    : [];
  // 空の一覧は既定値(書かない)
  const update = (next: readonly Item[]) => commit(next.length === 0 ? undefined : [...next]);
  const move = (from: number, to: number) => {
    const next = [...items];
    const [moved] = next.splice(from, 1);
    if (moved === undefined) return;
    next.splice(to, 0, moved);
    update(next);
    if (open === from) setOpen(to);
    else if (open === to) setOpen(from);
  };
  const remove = (index: number) => {
    update(items.filter((_, i) => i !== index));
    if (open === index) setOpen(undefined);
    else if (open !== undefined && open > index) setOpen(open - 1);
  };

  return (
    <div className="prop-children prop-collection">
      {items.map((item, i) => {
        const caption = itemCaption(item) ?? itemClass;
        return (
          <div key={i}>
            <div className="prop-row collection-item" role="row">
              <span className="prop-name" role="cell">
                <button
                  type="button"
                  className="prop-toggle"
                  aria-expanded={open === i}
                  onClick={() => {
                    setOpen(open === i ? undefined : i);
                  }}
                >
                  {open === i ? '▾' : '▸'} {`${String(i)}: ${caption}`}
                </button>
              </span>
              <span className="prop-value collection-actions" role="cell">
                <button
                  type="button"
                  className="prop-reset"
                  title={l10n.t('Move up')}
                  disabled={i === 0}
                  onClick={() => {
                    move(i, i - 1);
                  }}
                >
                  ↑
                </button>
                <button
                  type="button"
                  className="prop-reset"
                  title={l10n.t('Move down')}
                  disabled={i === items.length - 1}
                  onClick={() => {
                    move(i, i + 1);
                  }}
                >
                  ↓
                </button>
                <button
                  type="button"
                  className="prop-reset"
                  title={l10n.t('Delete the item')}
                  onClick={() => {
                    remove(i);
                  }}
                >
                  ×
                </button>
              </span>
            </div>
            {open === i && (
              <div className="prop-children">
                {Object.entries(known).map(([name, info]) => (
                  <ItemPropertyRow
                    key={name}
                    document={document}
                    name={name}
                    info={info}
                    value={item[name]}
                    commit={(next) =>
                      update(
                        items.map((it, j) => (j === i ? withProperty(it, name, info, next) : it)),
                      )
                    }
                  />
                ))}
              </div>
            )}
          </div>
        );
      })}
      <button
        type="button"
        className="prop-reset collection-add"
        title={l10n.t('Add an item ({0})', itemClass)}
        onClick={() => {
          update([...items, {}]);
          setOpen(items.length);
        }}
      >
        + {l10n.t('Add')}
      </button>
    </div>
  );
}

/** 一覧に出す項目の名前(ステータスバーのパネルは Text、グリッドの列は Title.Caption) */
function itemCaption(item: Item): string | undefined {
  if (typeof item.Text === 'string' && item.Text !== '') return item.Text;
  const title = item.Title;
  if (typeof title === 'object' && title !== null && !Array.isArray(title)) {
    const caption = (title as Item).Caption;
    if (typeof caption === 'string' && caption !== '') return caption;
  }
  return undefined;
}

/**
 * 項目のプロパティの 1 行(既定値と同じ値は書かない)。入れ子のオブジェクト(グリッドの列の Title・Font)と TStrings(PickList)は
 * 開いて中を編集する(docs/adr/0054)。入れ子の中の既定値は、項目のプロパティの既定値(オブジェクト)から取る。
 */
function ItemPropertyRow({
  document,
  name,
  info,
  value,
  commit,
}: {
  readonly document: BfmDocument;
  readonly name: string;
  readonly info: PropertyInfo;
  readonly value: PropertyValue | undefined;
  readonly commit: (value: PropertyValue | undefined) => string | undefined;
}) {
  const [expanded, setExpanded] = useState(false);
  const isSet = value !== undefined;
  const type = info.type;
  const expandable = type.kind === 'object' || type.kind === 'strings';
  return (
    <>
      <div className={isSet ? 'prop-row nested set' : 'prop-row nested'} role="row">
        <span className="prop-name" role="cell" title={info.doc}>
          {expandable ? (
            <button
              type="button"
              className="prop-toggle"
              aria-expanded={expanded}
              onClick={() => {
                setExpanded(!expanded);
              }}
            >
              {expanded ? '▾' : '▸'} {name}
            </button>
          ) : (
            name
          )}
        </span>
        <span className="prop-value" role="cell">
          <Editor
            document={document}
            type={type}
            value={value ?? info.default}
            mixed={false}
            fallback={info.default}
            commit={commit}
          />
          {isSet && (
            <button
              type="button"
              className="prop-reset"
              title={l10n.t('Reset to the default')}
              onClick={() => commit(undefined)}
            >
              ×
            </button>
          )}
        </span>
      </div>
      {expanded && type.kind === 'strings' && (
        <StringsEditor value={value ?? info.default} commit={commit} />
      )}
      {expanded && type.kind === 'object' && (
        <div className="prop-children">
          {Object.entries(getCatalog().objects[type.class]?.properties ?? {}).map(
            ([sub, subInfo]) => {
              const current = asItem(value);
              const defaults = asItem(info.default);
              const withDefault = { ...subInfo, default: defaults[sub] ?? subInfo.default };
              return (
                <ItemPropertyRow
                  key={sub}
                  document={document}
                  name={sub}
                  info={withDefault}
                  value={current[sub]}
                  commit={(next) => {
                    const updated = withProperty(current, sub, withDefault, next);
                    return commit(Object.keys(updated).length === 0 ? undefined : updated);
                  }}
                />
              );
            },
          )}
        </div>
      )}
    </>
  );
}

function asItem(value: unknown): Item {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
    ? (value as Item)
    : {};
}

/** 項目の name を value にした項目(undefined・既定値と同じ値なら、その項目から除く) */
function withProperty(
  item: Item,
  name: string,
  info: PropertyInfo,
  value: PropertyValue | undefined,
): Item {
  const rest = Object.fromEntries(Object.entries(item).filter(([k]) => k !== name));
  return value === undefined || JSON.stringify(value) === JSON.stringify(info.default)
    ? rest
    : { ...rest, [name]: value };
}

function StringsEditor({
  value,
  commit,
}: {
  readonly value: unknown;
  readonly commit: (value: PropertyValue | undefined) => string | undefined;
}) {
  const text = formatValue({ kind: 'strings' }, value);
  const [draft, setDraft] = useState(text);
  const [shown, setShown] = useState(text);
  if (text !== shown) {
    setShown(text);
    setDraft(text);
  }
  const save = () => {
    if (draft === text) return;
    const parsed = parseInput({ kind: 'strings' }, draft);
    if (parsed.ok) commit(parsed.value);
  };
  return (
    <div className="prop-children">
      <textarea
        className="prop-strings"
        rows={Math.min(12, Math.max(3, draft.split('\n').length + 1))}
        value={draft}
        onChange={(e) => {
          setDraft(e.target.value);
        }}
        onBlur={save}
        onKeyDown={(e) => {
          if (e.key === 'Enter' && e.ctrlKey) save();
          if (e.key === 'Escape') setDraft(text);
        }}
      />
      <div className="muted">
        {l10n.t('One item per line. Ctrl+Enter or leaving the field applies it.')}
      </div>
    </div>
  );
}

/** 値の欄(型に応じた入力) */
function Editor({
  document,
  type,
  value,
  mixed,
  fallback,
  commit,
}: {
  readonly document: BfmDocument;
  readonly type: PropertyType;
  readonly value: unknown;
  readonly mixed: boolean;
  readonly fallback: unknown;
  readonly commit: (value: PropertyValue | undefined) => string | undefined;
}) {
  const catalog = getCatalog();
  const select = (options: readonly string[], labelOf: (v: string) => string = (v) => v) => (
    <select
      className="prop-select"
      value={mixed ? '\u0000' : scalarText(value)}
      onChange={(e) => {
        commit(parseValue(type, e.target.value));
      }}
    >
      {mixed && <option value={'\u0000'}>{l10n.t('(different values)')}</option>}
      {options.map((o) => (
        <option key={o} value={o}>
          {labelOf(o)}
        </option>
      ))}
    </select>
  );

  switch (type.kind) {
    case 'bool':
      return (
        <input
          type="checkbox"
          checked={value === true}
          ref={(el) => {
            if (el) el.indeterminate = mixed;
          }}
          onChange={(e) => {
            commit(e.target.checked);
          }}
        />
      );
    case 'enum':
      return select(enumItems(type));
    case 'alias':
      // 定数を持つ別名(TCursor・TModalResult 等)は定数の選択肢。TColor・TShortCut は下の専用の入力
      if (type.alias !== 'TColor' && type.alias !== 'TShortCut')
        return select(Object.keys(catalog.constants[type.alias] ?? {}));
      if (type.alias === 'TColor')
        return (
          <span className="prop-color">
            <span
              className="prop-swatch"
              style={{ background: colorToCss(value) ?? 'transparent' }}
            />
            <TextEditor
              type={type}
              value={value}
              mixed={mixed}
              fallback={fallback}
              list="beth-colors"
              commit={commit}
            />
            <datalist id="beth-colors">
              {Object.keys(catalog.constants.TColor ?? {}).map((c) => (
                <option key={c} value={c} />
              ))}
            </datalist>
          </span>
        );
      // TShortCut
      return (
        <>
          <TextEditor
            type={type}
            value={value}
            mixed={mixed}
            fallback={fallback}
            list="beth-shortcuts"
            commit={commit}
          />
          <datalist id="beth-shortcuts">
            {SHORTCUTS.map((s) => (
              <option key={s} value={s} />
            ))}
          </datalist>
        </>
      );
    case 'ref': {
      const candidates = [...referenceCandidates(document, type.class)];
      return select(['', ...candidates], (v) => (v === '' ? l10n.t('(none)') : v));
    }
    case 'object':
      return <span className="muted">{`(${type.class})`}</span>;
    case 'strings': {
      const lines = Array.isArray(value) ? value.length : 0;
      return <span className="muted">{l10n.t('({0} lines)', lines)}</span>;
    }
    case 'collection': {
      const count = Array.isArray(value) ? value.length : 0;
      return <span className="muted">{l10n.t('({0} items)', count)}</span>;
    }
    default:
      break;
  }
  return <TextEditor type={type} value={value} mixed={mixed} fallback={fallback} commit={commit} />;
}

function TextEditor({
  type,
  value,
  mixed,
  fallback,
  list,
  commit,
}: {
  readonly type: PropertyType;
  readonly value: unknown;
  readonly mixed: boolean;
  readonly fallback: unknown;
  readonly list?: string;
  readonly commit: (value: PropertyValue | undefined) => string | undefined;
}) {
  const text = mixed ? '' : formatValue(type, value);
  const draft = useCommittedDraft(text, (input) => {
    const parsed = parseInput(type, input);
    if (!parsed.ok) return parsed.error;
    // 空欄は既定値に戻す(複数選択で値が違うときの空欄は、何もしない)
    if (mixed && parsed.value === undefined) return undefined;
    return commit(parsed.value);
  });
  return (
    <input
      className="prop-input"
      placeholder={mixed ? l10n.t('(different values)') : formatValue(type, fallback)}
      list={list}
      title={draft.error}
      aria-invalid={draft.error !== undefined}
      {...draft.inputProps}
    />
  );
}

function parseValue(type: PropertyType, text: string): PropertyValue | undefined {
  if (type.kind === 'ref') return text === '' ? undefined : text;
  const parsed = parseInput(type, text);
  return parsed.ok ? parsed.value : undefined;
}

/** 参照の候補: 型が合うコンポーネント・コントロールの name */
function* referenceCandidates(document: BfmDocument, className: string): Generator<string> {
  const walk = function* (
    controls: readonly ControlNode[] | undefined,
  ): Generator<{ name: string; class: string }> {
    for (const c of controls ?? []) {
      yield c;
      yield* walk(c.controls);
    }
  };
  for (const c of walk(document.form.controls)) if (isSubclassOf(c.class, className)) yield c.name;
  for (const c of document.components ?? []) if (isSubclassOf(c.class, className)) yield c.name;
  // ActionList の Action(docs/adr/0046)
  for (const c of document.components ?? [])
    for (const a of c.actions ?? [])
      if (isSubclassOf(a.class ?? 'TAction', className)) yield a.name;
}

function ownValue(location: NodeLocation, path: Path): unknown {
  const own = location.node.properties?.[path[0]];
  if (path.length === 1) return own;
  return typeof own === 'object' && own !== null && !Array.isArray(own)
    ? (own as Record<string, unknown>)[path[1]]
    : undefined;
}

/** 選択肢の値(文字列・数値・真偽以外は空) */
function scalarText(value: unknown): string {
  if (typeof value === 'string') return value;
  if (typeof value === 'number' || typeof value === 'boolean') return String(value);
  return '';
}
