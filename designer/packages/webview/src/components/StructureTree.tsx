/**
 * 構造の木(docs/designer/editor-design.md §5.3)。フォーム・コントロールの入れ子・非ビジュアルコンポーネント
 * (メニューなら項目の入れ子)を表示し、キャンバスと選択を共有する。上のボタンと右クリックのメニューで、
 * メニュー項目・タブ・ツールボタンの追加、並べ替え、削除を行う(actions.ts)。
 */
import {
  l10n,
  type ComponentNode,
  type ControlNode,
  type MenuItemNode,
} from '@no-vcl-designer/core';
import type { MouseEvent } from 'react';
import { useShallow } from 'zustand/shallow';
import { select } from '../editing.ts';
import { uiStore, useDocumentStore, useSelectedNodes, useUiStore } from '../store/stores.ts';
import { actionsFor } from './actions.ts';

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

  // 右クリック: 選択に無ければ選び直してから、メニューを開く
  const onContextMenu = (name: string) => (e: MouseEvent) => {
    e.preventDefault();
    if (!uiStore.getState().selection.includes(name)) select([name]);
    uiStore.getState().openContextMenu({ x: e.clientX, y: e.clientY });
  };

  const label = (name: string, detail: string) => (
    <button
      type="button"
      className={selected.has(name) ? 'tree-label selected' : 'tree-label'}
      aria-selected={selected.has(name)}
      onClick={onClick(name)}
      onContextMenu={onContextMenu(name)}
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

/** 選択に応じた操作のうち、木の上にボタンで出すもの */
function TreeToolbar() {
  const actions = actionsFor(useSelectedNodes()).filter((a) => a.toolbar);
  if (actions.length === 0) return null;
  return (
    <div className="toolbar">
      {actions.map((action) => (
        <button
          key={action.id}
          type="button"
          disabled={action.disabled}
          title={action.short !== undefined ? action.label : undefined}
          onClick={action.run}
        >
          {action.short ?? action.label}
        </button>
      ))}
    </div>
  );
}

function captionOf(item: MenuItemNode): string {
  const caption = item.properties?.Caption;
  return typeof caption === 'string' ? caption : '';
}
