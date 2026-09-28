import type {
  CanvasSettings,
  EditCommand,
  ExtensionToWebviewMessage,
  WebviewToExtensionMessage,
} from '@no-vcl-designer/core';
import { isJapanese } from '@no-vcl-designer/core';
import * as vscode from 'vscode';
import { applyEditCommand } from './applyEditCommand.ts';
import { generateCode } from './generateCode.ts';

/**
 * *.nvform.json を開くデザイナー。TextDocument を唯一の正とする(tk-designer ADR 0006。docs/designer/editor-design.md §3)。
 */
export class DesignerEditorProvider implements vscode.CustomTextEditorProvider {
  static readonly viewType = 'noVclDesigner.designer';

  static register(context: vscode.ExtensionContext): vscode.Disposable {
    return vscode.window.registerCustomEditorProvider(
      DesignerEditorProvider.viewType,
      new DesignerEditorProvider(context),
    );
  }

  private readonly context: vscode.ExtensionContext;

  private constructor(context: vscode.ExtensionContext) {
    this.context = context;
  }

  resolveCustomTextEditor(document: vscode.TextDocument, panel: vscode.WebviewPanel): void {
    const webviewRoot = vscode.Uri.joinPath(this.context.extensionUri, 'dist', 'webview');
    panel.webview.options = { enableScripts: true, localResourceRoots: [webviewRoot] };
    panel.webview.html = renderHtml(panel.webview, webviewRoot);

    const post = (message: ExtensionToWebviewMessage) => panel.webview.postMessage(message);
    const postDocument = () =>
      post({
        type: 'document',
        version: document.version,
        text: document.getText(),
        fileName: document.uri.path.split('/').pop() ?? '',
      });
    const postSettings = () => post({ type: 'settings', settings: canvasSettings() });

    // 編集は届いた順に 1 つずつ適用する(WorkspaceEdit の適用が重ならないように)
    let editQueue = Promise.resolve();
    const enqueueEdit = (requestId: number, command: EditCommand) => {
      editQueue = editQueue.then(async () => {
        // 予期しない例外で後続の編集まで止まらないよう、ここで結果に変換する
        const result = await applyEditCommand(document, command).catch((e: unknown) => ({
          ok: false as const,
          error: vscode.l10n.t('Unexpected error: {0}', e instanceof Error ? e.message : String(e)),
        }));
        await post({ type: 'editResult', requestId, ...result });
      });
    };

    const subscriptions = [
      panel.webview.onDidReceiveMessage((message: WebviewToExtensionMessage) => {
        switch (message.type) {
          case 'ready':
            void postDocument();
            void postSettings();
            break;
          case 'edit':
            enqueueEdit(message.requestId, message.command);
            break;
          case 'generateCode':
            void generateCode(document);
            break;
        }
      }),
      // テキストエディタでの編集や Undo/Redo も、この経路で Webview に届く
      vscode.workspace.onDidChangeTextDocument((e) => {
        if (e.document.uri.toString() === document.uri.toString()) void postDocument();
      }),
      vscode.workspace.onDidChangeConfiguration((e) => {
        if (e.affectsConfiguration('noVclDesigner.canvas')) void postSettings();
      }),
    ];
    panel.onDidDispose(() => {
      for (const s of subscriptions) s.dispose();
    });
  }
}

/** キャンバスの設定(docs/designer/editor-design.md §5.4)。空欄・0 は表示言語から決める */
function canvasSettings(): CanvasSettings {
  const config = vscode.workspace.getConfiguration('noVclDesigner.canvas');
  const fontFamily = config.get<string>('fontFamily', '').trim();
  const fontSize = config.get<number>('fontSize', 0);
  return {
    fontFamily:
      fontFamily !== ''
        ? fontFamily
        : isJapanese(vscode.env.language)
          ? 'Yu Gothic UI'
          : 'Segoe UI',
    fontSize: fontSize > 0 ? fontSize : 9,
    gridSize: Math.max(0, Math.floor(config.get<number>('gridSize', 8))),
  };
}

function renderHtml(webview: vscode.Webview, webviewRoot: vscode.Uri): string {
  const scriptUri = webview.asWebviewUri(vscode.Uri.joinPath(webviewRoot, 'main.js'));
  const styleUri = webview.asWebviewUri(vscode.Uri.joinPath(webviewRoot, 'main.css'));
  const nonce = createNonce();
  // 訳は描画の前に必要なため、HTML に埋め込んで渡す(tk-designer ADR 0014)
  const l10n = escapeAttribute(JSON.stringify(vscode.l10n.bundle ?? {}));
  const language = escapeAttribute(vscode.env.language);

  return `<!DOCTYPE html>
<html lang="${language}">
<head>
  <meta charset="UTF-8">
  <meta http-equiv="Content-Security-Policy"
    content="default-src 'none'; style-src ${webview.cspSource} 'unsafe-inline'; font-src ${webview.cspSource}; script-src 'nonce-${nonce}';">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="stylesheet" href="${styleUri.toString()}">
  <title>no_vcl Designer</title>
</head>
<body>
  <div id="root" data-language="${language}" data-l10n="${l10n}"></div>
  <script type="module" nonce="${nonce}" src="${scriptUri.toString()}"></script>
</body>
</html>`;
}

function escapeAttribute(text: string): string {
  return text
    .replace(/&/g, '&amp;')
    .replace(/"/g, '&quot;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

function createNonce(): string {
  const bytes = new Uint8Array(16);
  crypto.getRandomValues(bytes);
  return Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join('');
}
