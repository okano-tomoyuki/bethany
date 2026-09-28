/**
 * 右クリックのメニュー(docs/designer/editor-design.md §5.2)。選択に対してできる操作(actions.ts)を並べる。
 * 外側のクリック・Esc・操作の実行で閉じる。上下の矢印で項目を移り、Enter で実行する。
 */
import { l10n } from '@bethany-designer/core';
import {
  useEffect,
  useLayoutEffect,
  useRef,
  useState,
  type KeyboardEvent as ReactKeyboardEvent,
} from 'react';
import { uiStore, useSelectedNodes, useUiStore } from '../store/stores.ts';
import { actionsFor } from './actions.ts';

export function ContextMenu() {
  const at = useUiStore((s) => s.contextMenu);
  const selection = useSelectedNodes();
  const ref = useRef<HTMLDivElement>(null);
  const [position, setPosition] = useState(at);
  const actions = actionsFor(selection);
  const close = () => {
    uiStore.getState().openContextMenu(undefined);
  };

  // 画面の端からはみ出さないよう、開いた後に位置を直す
  useLayoutEffect(() => {
    const menu = ref.current;
    if (!at || !menu) return;
    const r = menu.getBoundingClientRect();
    setPosition({
      x: Math.max(0, Math.min(at.x, window.innerWidth - r.width - 4)),
      y: Math.max(0, Math.min(at.y, window.innerHeight - r.height - 4)),
    });
    menu.querySelector<HTMLButtonElement>('button:not(:disabled)')?.focus();
  }, [at]);

  useEffect(() => {
    if (!at) return;
    const onPointerDown = (e: PointerEvent) => {
      if (!ref.current?.contains(e.target as Node)) close();
    };
    const onKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        e.stopPropagation();
        close();
      }
    };
    window.addEventListener('pointerdown', onPointerDown, true);
    window.addEventListener('keydown', onKeyDown, true);
    window.addEventListener('blur', close);
    return () => {
      window.removeEventListener('pointerdown', onPointerDown, true);
      window.removeEventListener('keydown', onKeyDown, true);
      window.removeEventListener('blur', close);
    };
  }, [at]);

  if (!at || actions.length === 0) return null;
  const shown = position ?? at;

  const onKeyDown = (e: ReactKeyboardEvent<HTMLDivElement>) => {
    if (e.key !== 'ArrowDown' && e.key !== 'ArrowUp') return;
    e.preventDefault();
    const items = [
      ...(ref.current?.querySelectorAll<HTMLButtonElement>('button:not(:disabled)') ?? []),
    ];
    const current = items.indexOf(document.activeElement as HTMLButtonElement);
    const next = (current + (e.key === 'ArrowDown' ? 1 : -1) + items.length) % items.length;
    items[next]?.focus();
  };

  return (
    <div
      ref={ref}
      className="context-menu"
      role="menu"
      aria-label={l10n.t('Actions')}
      style={{ left: shown.x, top: shown.y }}
      onKeyDown={onKeyDown}
      onContextMenu={(e) => {
        e.preventDefault();
      }}
    >
      {actions.map((action) => (
        <div key={action.id}>
          {action.separator && <hr />}
          <button
            type="button"
            role="menuitem"
            disabled={action.disabled}
            onClick={() => {
              close();
              action.run();
            }}
          >
            {action.label}
          </button>
        </div>
      ))}
    </div>
  );
}
