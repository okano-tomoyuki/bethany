import {
  findNode,
  l10n,
  walkNodes,
  type Diagnostic,
  type NvformDocument,
} from '@no-vcl-designer/core';
import { select } from '../editing.ts';
import { useDocumentStore } from '../store/stores.ts';

/** 拒否された編集の理由と、検証結果(診断)を表示する。診断をクリックすると、そのノードを選択する */
export function StatusPanel() {
  const lastError = useDocumentStore((s) => s.lastError);
  const clearError = useDocumentStore((s) => s.clearError);
  const diagnostics = useDocumentStore((s) => s.diagnostics);
  const document = useDocumentStore((s) => s.document);

  if (!lastError && diagnostics.length === 0) return null;
  return (
    <section className="panel status" aria-label={l10n.t('Status')}>
      {lastError && (
        <div role="alert" className="error">
          {lastError}
          <button type="button" onClick={clearError} aria-label={l10n.t('Close')}>
            ×
          </button>
        </div>
      )}
      {diagnostics.length > 0 && (
        <ul>
          {diagnostics.map((d, i) => {
            const node = document && nodeOf(document, d);
            return (
              // 診断は毎回作り直される一覧で並べ替えも起きないため、添字をキーにしてよい
              <li key={i} className={d.severity}>
                {node ? (
                  <button
                    type="button"
                    className="link"
                    onClick={() => {
                      select([node]);
                    }}
                  >
                    {node}
                  </button>
                ) : (
                  d.path.join('.') || l10n.t('(root)')
                )}
                : {d.message}
              </li>
            );
          })}
        </ul>
      )}
    </section>
  );
}

/** 診断の位置にあるノードの name(いちばん深いもの) */
function nodeOf(document: NvformDocument, diagnostic: Diagnostic): string | undefined {
  let found: string | undefined;
  for (const location of walkNodes(document)) {
    const path = location.path;
    if (path.every((key, i) => diagnostic.path[i] === key)) found = location.node.name;
  }
  return found && findNode(document, found) ? found : undefined;
}
