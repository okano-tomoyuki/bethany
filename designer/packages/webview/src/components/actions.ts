/**
 * 選択に対してできる操作の一覧(docs/designer/editor-design.md §5.2・§5.3)。構造の木の上のボタンと、右クリックのメニューで共有する。
 */
import { isSubclassOf, l10n, type NodeLocation } from '@no-vcl-designer/core';
import {
  addMenuItem,
  addTab,
  addToolButton,
  moveToEdge,
  removeSelection,
  select,
} from '../editing.ts';
import { documentStore } from '../store/stores.ts';

export interface Action {
  readonly id: string;
  readonly label: string;
  /** 構造の木の上のボタンに出すときの短い表示(無ければ label) */
  readonly short?: string;
  readonly disabled?: boolean;
  /** 構造の木の上のボタンに出すか(右クリックのメニューには全部出す) */
  readonly toolbar: boolean;
  /** 前に区切り線を引く(右クリックのメニュー) */
  readonly separator?: boolean;
  readonly run: () => void;
}

export function actionsFor(selection: readonly NodeLocation[]): Action[] {
  const [first, ...rest] = selection;
  if (!first) return [];
  const single = rest.length === 0;
  const actions: Action[] = [];

  // ---- 追加 ----
  if (single) {
    const isMenu = first.kind === 'component' && isSubclassOf(first.node.class, 'TMenu');
    if (isMenu || first.kind === 'menuItem') {
      const siblingParent =
        first.kind === 'menuItem' ? (first.parentItem?.name ?? first.menu.name) : first.node.name;
      const index = first.kind === 'menuItem' ? indexIn(first) + 1 : undefined;
      actions.push({
        id: 'addItem',
        label: l10n.t('Add Item'),
        toolbar: true,
        run: () => {
          addMenuItem(siblingParent, l10n.t('New Item'), index);
        },
      });
      if (first.kind === 'menuItem')
        actions.push({
          id: 'addSubItem',
          label: l10n.t('Add Submenu Item'),
          toolbar: true,
          run: () => {
            addMenuItem(first.node.name, l10n.t('New Item'));
          },
        });
      actions.push({
        id: 'addSeparator',
        label: l10n.t('Add Separator'),
        toolbar: true,
        run: () => {
          addMenuItem(siblingParent, '-', index);
        },
      });
    }
    const pageControl =
      first.kind === 'control' && first.node.class === 'TPageControl'
        ? first.node.name
        : first.kind === 'control' && first.node.class === 'TTabSheet'
          ? first.parent.name
          : undefined;
    if (pageControl !== undefined)
      actions.push({
        id: 'addTab',
        label: l10n.t('Add Tab'),
        toolbar: true,
        run: () => {
          addTab(pageControl);
        },
      });
    const toolBar =
      first.kind === 'control' && first.node.class === 'TToolBar'
        ? first.node.name
        : first.kind === 'control' && first.node.class === 'TToolButton'
          ? first.parent.name
          : undefined;
    if (toolBar !== undefined)
      actions.push({
        id: 'addToolButton',
        label: l10n.t('Add Button'),
        toolbar: true,
        run: () => {
          addToolButton(toolBar);
        },
      });
  }

  // ---- 並び ----
  const siblings = single ? siblingsOf(first) : undefined;
  if (siblings) {
    const index = indexIn(first);
    actions.push(
      {
        id: 'moveUp',
        label: l10n.t('Move Up'),
        short: '↑',
        toolbar: true,
        separator: true,
        disabled: index <= 0,
        run: () => {
          reorder(first, index - 1);
        },
      },
      {
        id: 'moveDown',
        label: l10n.t('Move Down'),
        short: '↓',
        toolbar: true,
        disabled: index < 0 || index >= siblings.length - 1,
        run: () => {
          reorder(first, index + 1);
        },
      },
    );
  }
  if (first.kind === 'control' && selection.every((l) => l.kind === 'control')) {
    actions.push(
      {
        id: 'bringToFront',
        label: l10n.t('Bring to Front'),
        toolbar: false,
        separator: !siblings,
        run: () => {
          moveToEdge(true);
        },
      },
      {
        id: 'sendToBack',
        label: l10n.t('Send to Back'),
        toolbar: false,
        run: () => {
          moveToEdge(false);
        },
      },
    );
  }

  // ---- 選択・削除 ----
  const parent =
    first.kind === 'control'
      ? first.parent.name
      : first.kind === 'menuItem'
        ? (first.parentItem?.name ?? first.menu.name)
        : undefined;
  if (parent !== undefined)
    actions.push({
      id: 'selectParent',
      label: l10n.t('Select Parent'),
      toolbar: false,
      separator: true,
      run: () => {
        select([parent]);
      },
    });
  if (first.kind !== 'form')
    actions.push({
      id: 'delete',
      label: l10n.t('Delete'),
      toolbar: true,
      separator: parent === undefined,
      run: removeSelection,
    });
  return actions;
}

/** 同じ親の中での並び(フォーム・非ビジュアルコンポーネントは並べ替えない) */
function siblingsOf(location: NodeLocation): readonly { name: string }[] | undefined {
  if (location.kind === 'control') return location.parent.controls;
  if (location.kind === 'menuItem') return (location.parentItem ?? location.menu).items;
  return undefined;
}

function indexIn(location: NodeLocation): number {
  return siblingsOf(location)?.findIndex((n) => n.name === location.node.name) ?? -1;
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
