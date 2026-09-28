import type { ExtensionToWebviewMessage, WebviewToExtensionMessage } from '@no-vcl-designer/core';

// acquireVsCodeApi は1 つの Webview につき 1 回しか呼べないため、モジュールで1 度だけ取得する
const vscode = acquireVsCodeApi();

export function postMessage(message: WebviewToExtensionMessage): void {
  vscode.postMessage(message);
}

/** Webview を隠して戻したとき・VS Code を再起動したときに復元する UI の状態 */
export interface PersistedState {
  readonly zoom?: number;
}

export function loadState(): PersistedState {
  return (vscode.getState() as PersistedState | undefined) ?? {};
}

export function saveState(state: PersistedState): void {
  vscode.setState(state);
}

export function onMessage(listener: (message: ExtensionToWebviewMessage) => void): () => void {
  const handler = (event: MessageEvent<ExtensionToWebviewMessage>) => {
    listener(event.data);
  };
  window.addEventListener('message', handler);
  return () => {
    window.removeEventListener('message', handler);
  };
}
