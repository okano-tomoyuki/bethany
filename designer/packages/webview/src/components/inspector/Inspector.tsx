/**
 * オブジェクトインスペクタ(docs/designer/editor-design.md §6)。選択したノードの名前と、プロパティ・イベントの 2 つのタブ。
 */
import { classOf, l10n, type NodeLocation } from '@no-vcl-designer/core';
import { useState } from 'react';
import { renameNode } from '../../editing.ts';
import { uiStore, useDocumentStore, useSelectedNodes, useUiStore } from '../../store/stores.ts';
import { messagesAt } from './diagnostics.ts';
import { EventsGrid } from './EventsGrid.tsx';
import { TextField } from './fields.tsx';
import { PropertyGrid } from './PropertyGrid.tsx';

export function Inspector() {
  const nodes = useSelectedNodes();
  const tab = useUiStore((s) => s.inspectorTab);
  const diagnostics = useDocumentStore((s) => s.diagnostics);
  const [filter, setFilter] = useState('');
  const [onlySet, setOnlySet] = useState(false);
  const [first] = nodes;

  return (
    <section className="panel inspector" aria-label={l10n.t('Object Inspector')}>
      <h2>{l10n.t('Object Inspector')}</h2>
      {!first ? (
        <p className="muted">{l10n.t('Select a component on the canvas or in the structure')}</p>
      ) : (
        <>
          <Header
            nodes={nodes}
            error={
              nodes.length === 1 ? messagesAt(diagnostics, [...first.path, 'name']) : undefined
            }
          />
          <div className="tabs" role="tablist">
            {(['properties', 'events'] as const).map((t) => (
              <button
                key={t}
                type="button"
                role="tab"
                aria-selected={tab === t}
                className={tab === t ? 'tab active' : 'tab'}
                onClick={() => {
                  uiStore.getState().setInspectorTab(t);
                }}
              >
                {t === 'properties' ? l10n.t('Properties') : l10n.t('Events')}
              </button>
            ))}
          </div>
          <div className="inspector-filter">
            <input
              type="search"
              placeholder={l10n.t('Search')}
              value={filter}
              onChange={(e) => {
                setFilter(e.target.value);
              }}
            />
            {tab === 'properties' && (
              <label>
                <input
                  type="checkbox"
                  checked={onlySet}
                  onChange={(e) => {
                    setOnlySet(e.target.checked);
                  }}
                />
                {l10n.t('Only set')}
              </label>
            )}
          </div>
          {tab === 'properties' ? (
            <PropertyGrid nodes={nodes} filter={filter} onlySet={onlySet} />
          ) : (
            <EventsGrid nodes={nodes} filter={filter} />
          )}
        </>
      )}
    </section>
  );
}

function Header({
  nodes,
  error,
}: {
  readonly nodes: readonly NodeLocation[];
  readonly error: string | undefined;
}) {
  const [first] = nodes;
  if (!first) return null;
  if (nodes.length > 1)
    return <p className="inspector-header">{l10n.t('{0} selected', nodes.length)}</p>;
  return (
    <div className="inspector-header">
      <TextField
        label={l10n.t('Name')}
        value={first.node.name}
        error={error}
        onCommit={(text) => renameNode(first.node.name, text.trim())}
      />
      <div className="muted">{classOf(first)}</div>
    </div>
  );
}
