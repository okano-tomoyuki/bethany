/**
 * キーボード操作(docs/designer/editor-design.md §5.2)。入力欄にフォーカスがあるときは何もしない
 * (文字の削除などを妨げないため)。
 * - Delete: 選択したものを削除
 * - Escape: パレットで選んだクラスを取り消す。無ければ親を選択
 * - 矢印: 1px 移動。Shift+矢印: 1px 大きさを変える
 */
import { nudgeSelection, removeSelection, selectParent } from './editing.ts';
import { uiStore } from './store/stores.ts';

const ARROWS: Readonly<Record<string, readonly [number, number]>> = {
  ArrowLeft: [-1, 0],
  ArrowRight: [1, 0],
  ArrowUp: [0, -1],
  ArrowDown: [0, 1],
};

export function installShortcuts(): () => void {
  const handler = (e: KeyboardEvent) => {
    const target = e.target as HTMLElement | null;
    if (target?.closest('input, select, textarea, [contenteditable="true"]')) return;
    const arrow = ARROWS[e.key];
    if (arrow) {
      // 構造の木の上では、矢印は木の操作に使う
      if (target?.closest('.tree')) return;
      e.preventDefault();
      nudgeSelection(arrow[0], arrow[1], e.shiftKey);
      return;
    }
    switch (e.key) {
      case 'Delete':
        e.preventDefault();
        removeSelection();
        break;
      case 'Escape':
        if (uiStore.getState().tool !== undefined) uiStore.getState().setTool(undefined);
        else selectParent();
        break;
    }
  };
  window.addEventListener('keydown', handler);
  return () => {
    window.removeEventListener('keydown', handler);
  };
}
