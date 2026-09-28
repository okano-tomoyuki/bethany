import { configureL10n, type L10nBundle } from '@no-vcl-designer/core';
import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { App } from './App.tsx';
import { connectToHost } from './bridge.ts';
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

// 倍率は Webview の状態に保存し、開き直したときに戻す
const { zoom } = loadState();
if (ZOOM_LEVELS.some((z) => z === zoom)) uiStore.getState().setZoom(zoom ?? 1);
uiStore.subscribe((state, previous) => {
  if (state.zoom !== previous.zoom) saveState({ ...loadState(), zoom: state.zoom });
});

connectToHost();
installShortcuts();

createRoot(container).render(
  <StrictMode>
    <App />
  </StrictMode>,
);
