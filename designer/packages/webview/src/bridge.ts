import { documentStore, uiStore } from './store/stores.ts';
import { onMessage, postMessage } from './vscode.ts';

/** 拡張ホストからのメッセージをストアにつなぐ。アプリの起動時に 1 回だけ呼ぶ */
export function connectToHost(): void {
  const store = documentStore.getState();
  onMessage((message) => {
    switch (message.type) {
      case 'document':
        store.receiveDocument(message.version, message.text, message.fileName);
        break;
      case 'editResult':
        store.receiveEditResult(message.requestId, message.ok, message.error);
        break;
      case 'settings':
        uiStore.getState().receiveSettings(message.settings);
        break;
    }
  });
  // リスナー登録後に準備完了を通知し、初回のドキュメントを取りこぼさないようにする
  postMessage({ type: 'ready' });
}
