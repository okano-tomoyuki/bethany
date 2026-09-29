/**
 * パレット(docs/designer/editor-design.md §5.2)。カタログの分類ごとにクラスを並べる。
 * クリックでクラスを選び(次にキャンバスをクリック・ドラッグしたところに追加する)、ダブルクリックで既定の位置に追加する。
 * 並べ方は、アイコンと名前の一覧か、アイコンだけ(名前はツールチップ)を見出しのボタンで切り替える。
 */
import { getCatalog, l10n } from '@bethany-designer/core';
import { useMemo } from 'react';
import { addFromPalette } from '../editing.ts';
import { uiStore, useUiStore } from '../store/stores.ts';
import { ClassIcon } from './ClassIcon.tsx';

export function Palette() {
  const tool = useUiStore((s) => s.tool);
  const view = useUiStore((s) => s.paletteView);
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
      <div className="panel-header">
        <h2>{l10n.t('Palette')}</h2>
        <button
          type="button"
          className="panel-header-button"
          title={view === 'list' ? l10n.t('Show icons only') : l10n.t('Show icons and names')}
          onClick={() => {
            uiStore.getState().setPaletteView(view === 'list' ? 'icons' : 'list');
          }}
        >
          {view === 'list' ? '▦' : '☰'}
        </button>
      </div>
      {groups.map(({ category, classes }) => (
        <details key={category} open>
          <summary>{category}</summary>
          <div className={view === 'list' ? 'palette-items list' : 'palette-items icons'}>
            {classes.map((name) => (
              <button
                key={name}
                type="button"
                className={tool === name ? 'palette-item active' : 'palette-item'}
                aria-pressed={tool === name}
                aria-label={name}
                title={`${name}
${l10n.t(
  'Click, then click or drag on the form to add. Double-click to add at the default position',
)}`}
                onClick={() => {
                  uiStore.getState().setTool(tool === name ? undefined : name);
                }}
                onDoubleClick={() => {
                  uiStore.getState().setTool(undefined);
                  addFromPalette(name);
                }}
              >
                <ClassIcon className={name} />
                {view === 'list' && <span className="palette-name">{name.slice(1)}</span>}
              </button>
            ))}
          </div>
        </details>
      ))}
    </section>
  );
}
