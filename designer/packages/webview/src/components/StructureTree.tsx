/**
 * 構造の木(docs/designer/editor-design.md §5.3)。フォーム・コントロールの入れ子・非ビジュアルコンポーネント
 * (メニューなら項目の入れ子)を表示し、キャンバスと選択を共有する。メニュー項目の追加・並べ替えもここで行う。
 */
import {
  isSubclassOf,
  l10n,
  type ComponentNode,
  type ControlNode,
  type MenuItemNode,
  type NodeLocation,
} from '@no-vcl-designer/core';
import type { MouseEvent } from 'react';
import { useShallow } from 'zustand/shallow';
import { addMenuItem, removeSelection, select } from '../editing.ts';
import {
  documentStore,
  uiStore,
  useDocumentStore,
  useSelectedNodes,
  useUiStore,
} from '../store/stores.ts';

export function StructureTree() {
  const document = useDocumentStore((s) => s.document);
  const selection = useUiStore(useShallow((s) => s.selection));
  if (!document) return null;
  const selected = new Set(selection);

  const onClick = (name: string) => (e: MouseEvent) => {
    const current = uiStore.getState().selection;
    if (e.shiftKey || e.ctrlKey)
      select(current.includes(name) ? current.filter((n) => n !== name) : [...current, name]);
    else select([name]);
  };

  const label = (name: string, detail: string) => (
    <button
      type="button"
      className={selected.has(name) ? 'tree-label selected' : 'tree-label'}
      aria-selected={selected.has(name)}
      onClick={onClick(name)}
    >
      <span>{name}</span>
      <span className="tree-class">{detail}</span>
    </button>
  );

  const controls = (list: readonly ControlNode[] | undefined) =>
    list && list.length > 0 ? (
      <ul>
        {list.map((c) => (
          <li key={c.name}>
            {label(c.name, c.class)}
            {controls(c.controls)}
          </li>
        ))}
      </ul>
    ) : null;

  const items = (list: readonly MenuItemNode[] | undefined) =>
    list && list.length > 0 ? (
      <ul>
        {list.map((item) => (
          <li key={item.name}>
            {label(item.name, captionOf(item))}
            {items(item.items)}
          </li>
        ))}
      </ul>
    ) : null;

  return (
    <section className="panel structure" aria-label={l10n.t('Structure')}>
      <h2>{l10n.t('Structure')}</h2>
      <TreeToolbar />
      <ul className="tree" role="tree">
        <li>
          {label(document.form.name, document.form.class)}
          {controls(document.form.controls)}
        </li>
        {(document.components ?? []).map((c: ComponentNode) => (
          <li key={c.name}>
            {label(c.name, c.class)}
            {items(c.items)}
          </li>
        ))}
      </ul>
    </section>
  );
}

/** 選択に応じた操作(メニュー項目の追加・並べ替え・削除) */
function TreeToolbar() {
  const [first, ...rest] = useSelectedNodes();
  if (!first) return null;
  const single = rest.length === 0;
  const isMenu = first.kind === 'component' && isSubclassOf(first.node.class, 'TMenu');
  const siblings = siblingsOf(first);
  const index = siblings?.findIndex((n) => n.name === first.node.name) ?? -1;

  return (
    <div className="toolbar">
      {single && (isMenu || first.kind === 'menuItem') && (
        <>
          <button
            type="button"
            onClick={() => {
              if (isMenu) addMenuItem(first.node.name, l10n.t('New Item'));
              else if (first.kind === 'menuItem')
                addMenuItem(
                  first.parentItem?.name ?? first.menu.name,
                  l10n.t('New Item'),
                  index + 1,
                );
            }}
          >
            {l10n.t('Add Item')}
          </button>
          {first.kind === 'menuItem' && (
            <button
              type="button"
              onClick={() => {
                addMenuItem(first.node.name, l10n.t('New Item'));
              }}
            >
              {l10n.t('Add Submenu Item')}
            </button>
          )}
          <button
            type="button"
            onClick={() => {
              if (isMenu) addMenuItem(first.node.name, '-');
              else if (first.kind === 'menuItem')
                addMenuItem(first.parentItem?.name ?? first.menu.name, '-', index + 1);
            }}
          >
            {l10n.t('Add Separator')}
          </button>
        </>
      )}
      {single && siblings && (
        <>
          <button
            type="button"
            disabled={index <= 0}
            title={l10n.t('Move Up')}
            onClick={() => {
              reorder(first, index - 1);
            }}
          >
            ↑
          </button>
          <button
            type="button"
            disabled={index < 0 || index >= siblings.length - 1}
            title={l10n.t('Move Down')}
            onClick={() => {
              reorder(first, index + 1);
            }}
          >
            ↓
          </button>
        </>
      )}
      {first.kind !== 'form' && (
        <button type="button" onClick={removeSelection}>
          {l10n.t('Delete')}
        </button>
      )}
    </div>
  );
}

/** 同じ親の中での並び(フォーム・非ビジュアルコンポーネントは並べ替えない) */
function siblingsOf(location: NodeLocation): readonly { name: string }[] | undefined {
  if (location.kind === 'control') return location.parent.controls;
  if (location.kind === 'menuItem') return (location.parentItem ?? location.menu).items;
  return undefined;
}

function reorder(location: NodeLocation, index: number): void {
  const store = documentStore.getState();
  if (location.kind === 'control')
    store.dispatch({
      type: 'moveControls',
      names: [location.node.name],
      parent: location.parent.name,
      index,
    });
  else if (location.kind === 'menuItem')
    store.dispatch({
      type: 'moveMenuItem',
      name: location.node.name,
      parent: (location.parentItem ?? location.menu).name,
      index,
    });
}

function captionOf(item: MenuItemNode): string {
  const caption = item.properties?.Caption;
  return typeof caption === 'string' ? caption : '';
}
