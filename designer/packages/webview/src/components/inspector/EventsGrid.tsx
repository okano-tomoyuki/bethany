/**
 * イベントのタブ(docs/designer/editor-design.md §6)。欄には、同じイベントの型の既存のハンドラ名を候補に出す。
 * ダブルクリックで既定の名前(OkButtonClick)を設定する。
 */
import {
  classOf,
  defaultHandlerName,
  findClass,
  l10n,
  walkNodes,
  type NodeLocation,
  type BfmDocument,
} from '@bethany-designer/core';
import { useState } from 'react';
import { renameHandler, setEvent } from '../../editing.ts';
import { useDocumentStore } from '../../store/stores.ts';
import { messagesAt } from './diagnostics.ts';
import { useCommittedDraft } from './fields.tsx';

export function EventsGrid({
  nodes,
  filter,
}: {
  readonly nodes: readonly NodeLocation[];
  readonly filter: string;
}) {
  const document = useDocumentStore((s) => s.document);
  const diagnostics = useDocumentStore((s) => s.diagnostics);
  const [first, ...rest] = nodes;
  if (!document || !first) return null;
  const events = Object.entries(findClass(classOf(first))?.events ?? {}).filter(
    ([name, info]) =>
      name.toLowerCase().includes(filter.trim().toLowerCase()) &&
      rest.every((n) => findClass(classOf(n))?.events[name]?.type === info.type),
  );
  const handlers = handlersByType(document);

  return (
    <div className="property-grid" role="table">
      {events.map(([event, info]) => {
        const values = nodes.map((n) => n.node.events?.[event] ?? '');
        const mixed = values.some((v) => v !== values[0]);
        return (
          <EventRow
            key={event}
            nodes={nodes}
            event={event}
            doc={info.doc}
            value={mixed ? '' : (values[0] ?? '')}
            mixed={mixed}
            listId={`beth-handlers-${info.type}`}
            candidates={handlers.get(info.type) ?? []}
            error={
              nodes.length === 1
                ? messagesAt(diagnostics, [...first.path, 'events', event])
                : undefined
            }
          />
        );
      })}
      {events.length === 0 && <p className="muted">{l10n.t('No events')}</p>}
    </div>
  );
}

function EventRow({
  nodes,
  event,
  doc,
  value,
  mixed,
  listId,
  candidates,
  error,
}: {
  readonly nodes: readonly NodeLocation[];
  readonly event: string;
  readonly doc: string | undefined;
  readonly value: string;
  readonly mixed: boolean;
  readonly listId: string;
  readonly candidates: readonly string[];
  readonly error: string | undefined;
}) {
  const names = nodes.map((n) => n.node.name);
  const [renaming, setRenaming] = useState(false);
  const draft = useCommittedDraft(value, (text) => {
    const handler = text.trim();
    if (handler === '' && mixed) return undefined;
    return renaming && value !== ''
      ? renameHandler(value, handler)
      : setEvent(names, event, handler === '' ? undefined : handler);
  });
  const [first] = nodes;
  const setDefault = () => {
    if (value !== '' || !first) return;
    setEvent(names, event, defaultHandlerName(first.node.name, event));
  };

  return (
    <>
      <div className={value !== '' ? 'prop-row set' : 'prop-row'} role="row">
        <span className="prop-name" role="cell" title={doc} onDoubleClick={setDefault}>
          {event}
        </span>
        <span className="prop-value" role="cell">
          <input
            className="prop-input"
            list={listId}
            placeholder={mixed ? l10n.t('(different values)') : ''}
            title={
              draft.error ??
              l10n.t(
                'Double-click to set the default handler name ({0})',
                first ? defaultHandlerName(first.node.name, event) : '',
              )
            }
            aria-invalid={draft.error !== undefined}
            {...draft.inputProps}
            onDoubleClick={setDefault}
            onBlur={() => {
              draft.inputProps.onBlur();
              setRenaming(false);
            }}
          />
          <datalist id={listId}>
            {candidates.map((c) => (
              <option key={c} value={c} />
            ))}
          </datalist>
          {value !== '' && (
            <button
              type="button"
              className={renaming ? 'prop-reset active' : 'prop-reset'}
              aria-pressed={renaming}
              title={l10n.t(
                'Rename the handler everywhere it is used (instead of changing only this event)',
              )}
              onClick={() => {
                setRenaming(!renaming);
              }}
            >
              ✎
            </button>
          )}
        </span>
      </div>
      {(draft.error ?? error) && (
        <div className="field-error prop-error">{draft.error ?? error}</div>
      )}
    </>
  );
}

/** イベントの型 → その型のイベントに使われているハンドラ名 */
function handlersByType(document: BfmDocument): Map<string, string[]> {
  const result = new Map<string, Set<string>>();
  for (const location of walkNodes(document)) {
    const events = findClass(classOf(location))?.events ?? {};
    for (const [event, handler] of Object.entries(location.node.events ?? {})) {
      const type = Object.hasOwn(events, event) ? events[event]?.type : undefined;
      if (type === undefined) continue;
      let set = result.get(type);
      if (!set) result.set(type, (set = new Set()));
      set.add(handler);
    }
  }
  return new Map([...result].map(([type, set]) => [type, [...set].sort()]));
}
