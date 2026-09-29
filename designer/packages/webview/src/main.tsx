import { configureL10n, type L10nBundle } from '@bethany-designer/core';
import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { App } from './App.tsx';
import { connectToHost } from './bridge.ts';
import { ProjectApp } from './project/ProjectApp.tsx';
import { connectProjectToHost } from './project/projectStore.ts';
import { installShortcuts } from './shortcuts.ts';
import { uiStore } from './store/stores.ts';
import { ZOOM_LEVELS } from './store/uiStore.ts';
import { loadState, saveState } from './vscode.ts';
import './style.css';

const container = document.getElementById('root');
if (!container) throw new Error('#root not found');

// 拡張が HTML に埋め込んだ訳を、描画の前に読み込む(tk-designer ADR 0014)
configureL10n(
  container.dataset.language ?? 'en',
  JSON.parse(container.dataset.l10n ?? '{}') as L10nBundle,
);

if (container.dataset.mode === 'project') startProjectSettings(container);
else startDesigner(container);

/** プロジェクトの設定画面(*.bfproj.json。docs/designer/project-spec.md §6) */
function startProjectSettings(root: HTMLElement): void {
  connectProjectToHost();
  createRoot(root).render(
    <StrictMode>
      <ProjectApp />
    </StrictMode>,
  );
}

/** フォームのデザイナー(*.bfm.json) */
function startDesigner(root: HTMLElement): void {
  // 倍率とパレットの並べ方は Webview の状態に保存し、開き直したときに戻す
  const { zoom, paletteView } = loadState();
  if (ZOOM_LEVELS.some((z) => z === zoom)) uiStore.getState().setZoom(zoom ?? 1);
  if (paletteView === 'list' || paletteView === 'icons')
    uiStore.getState().setPaletteView(paletteView);
  uiStore.subscribe((state, previous) => {
    if (state.zoom !== previous.zoom || state.paletteView !== previous.paletteView)
      saveState({ ...loadState(), zoom: state.zoom, paletteView: state.paletteView });
  });

  connectToHost();
  installShortcuts();

  createRoot(root).render(
    <StrictMode>
      <App />
    </StrictMode>,
  );
}
