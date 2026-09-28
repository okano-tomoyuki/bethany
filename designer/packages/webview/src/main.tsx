import { configureL10n, type L10nBundle } from '@no-vcl-designer/core';
import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { App } from './App.tsx';
import { connectToHost } from './bridge.ts';
import { installShortcuts } from './shortcuts.ts';
import './style.css';

const container = document.getElementById('root');
if (!container) throw new Error('#root not found');

// 拡張が HTML に埋め込んだ訳を、描画の前に読み込む(tk-designer ADR 0014)
configureL10n(
  container.dataset.language ?? 'en',
  JSON.parse(container.dataset.l10n ?? '{}') as L10nBundle,
);

connectToHost();
installShortcuts();

createRoot(container).render(
  <StrictMode>
    <App />
  </StrictMode>,
);
