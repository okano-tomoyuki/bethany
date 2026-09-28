import { hasErrors, l10n } from '@bethany-designer/core';
import { DesignCanvas } from './canvas/DesignCanvas.tsx';
import { ContextMenu } from './components/ContextMenu.tsx';
import { Inspector } from './components/inspector/Inspector.tsx';
import { Palette } from './components/Palette.tsx';
import { StatusPanel } from './components/StatusPanel.tsx';
import { StructureTree } from './components/StructureTree.tsx';
import { generateCode } from './editing.ts';
import { useDocumentStore } from './store/stores.ts';

export function App() {
  const status = useDocumentStore((s) => s.status);
  const hasDocument = useDocumentStore((s) => s.document !== undefined);
  const blocked = useDocumentStore((s) => hasErrors(s.diagnostics));

  if (status === 'loading') return <p>{l10n.t('Loading…')}</p>;
  if (!hasDocument) {
    return (
      <main>
        <p>
          {l10n.t(
            'The designer cannot be shown because the file content is invalid. Fix it in the text editor.',
          )}
        </p>
        <StatusPanel />
      </main>
    );
  }
  return (
    <main className="layout">
      <header className="topbar">
        <button
          type="button"
          className="primary"
          disabled={blocked}
          title={
            blocked ? l10n.t('Cannot generate because the form has validation errors') : undefined
          }
          onClick={generateCode}
        >
          {l10n.t('Generate Code')}
        </button>
      </header>
      <div className="sidebar">
        <Palette />
        <StructureTree />
      </div>
      <DesignCanvas />
      <Inspector />
      <StatusPanel />
      <ContextMenu />
    </main>
  );
}
