/**
 * パレット(docs/designer/editor-design.md §5.2)。カタログの分類ごとにクラスを並べる。
 * クリックでクラスを選び(次にキャンバスをクリック・ドラッグしたところに追加する)、ダブルクリックで既定の位置に追加する。
 */
import { getCatalog, l10n } from '@bethany-designer/core';
import { useMemo } from 'react';
import { addFromPalette } from '../editing.ts';
import { uiStore, useUiStore } from '../store/stores.ts';

export function Palette() {
  const tool = useUiStore((s) => s.tool);
  const groups = useMemo(() => {
    const catalog = getCatalog();
    return catalog.palette.map((category) => ({
      category,
      classes: Object.entries(catalog.classes)
        .filter(([, info]) => info.palette === category)
        .map(([name]) => name),
    }));
  }, []);

  return (
    <section className="panel palette" aria-label={l10n.t('Palette')}>
      <h2>{l10n.t('Palette')}</h2>
      {groups.map(({ category, classes }) => (
        <details key={category} open>
          <summary>{category}</summary>
          <div className="palette-items">
            {classes.map((name) => (
              <button
                key={name}
                type="button"
                className={tool === name ? 'palette-item active' : 'palette-item'}
                aria-pressed={tool === name}
                title={l10n.t(
                  'Click, then click or drag on the form to add. Double-click to add at the default position',
                )}
                onClick={() => {
                  uiStore.getState().setTool(tool === name ? undefined : name);
                }}
                onDoubleClick={() => {
                  uiStore.getState().setTool(undefined);
                  addFromPalette(name);
                }}
              >
                {name.slice(1)}
              </button>
            ))}
          </div>
        </details>
      ))}
    </section>
  );
}
