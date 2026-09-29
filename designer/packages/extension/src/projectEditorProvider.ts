/**
 * *.bfproj.json を開く設定画面(docs/designer/project-spec.md §6)。フォームのデザイナーと同じく TextDocument を唯一の正とし、
 * 画面からの編集は決まった形で書き出して最小の差分で当てる(Undo が効き、保存は利用者が行う)。
 */
import {
  minimalTextEdit,
  parseProject,
  serializeProject,
  type BfprojDocument,
  type ExtensionToWebviewMessage,
  type WebviewToExtensionMessage,
} from '@bethany-designer/core';
import * as vscode from 'vscode';
import { openAsText, renderHtml } from './designerEditorProvider.ts';
import { generateCode, generateWholeProject } from './generateCode.ts';

export class ProjectEditorProvider implements vscode.CustomTextEditorProvider {
  static readonly viewType = 'bethanyDesigner.projectEditor';

  static register(context: vscode.ExtensionContext): vscode.Disposable {
    return vscode.window.registerCustomEditorProvider(
      ProjectEditorProvider.viewType,
      new ProjectEditorProvider(context),
    );
  }

  private readonly context: vscode.ExtensionContext;

  private constructor(context: vscode.ExtensionContext) {
    this.context = context;
  }

  resolveCustomTextEditor(document: vscode.TextDocument, panel: vscode.WebviewPanel): void {
    const webviewRoot = vscode.Uri.joinPath(this.context.extensionUri, 'dist', 'webview');
    panel.webview.options = { enableScripts: true, localResourceRoots: [webviewRoot] };
    panel.webview.html = renderHtml(panel.webview, webviewRoot, 'project');

    const post = (message: ExtensionToWebviewMessage) => panel.webview.postMessage(message);
    const postDocument = () =>
      post({
        type: 'document',
        version: document.version,
        text: document.getText(),
        fileName: document.uri.path.split('/').pop() ?? '',
      });

    // 編集は届いた順に 1 つずつ適用する(WorkspaceEdit の適用が重ならないように)
    let editQueue = Promise.resolve();
    const subscriptions = [
      panel.webview.onDidReceiveMessage((message: WebviewToExtensionMessage) => {
        switch (message.type) {
          case 'ready':
            void postDocument();
            break;
          case 'editProject': {
            const { requestId, project } = message;
            editQueue = editQueue.then(async () => {
              const result = await applyProject(document, project).catch((e: unknown) => ({
                ok: false as const,
                error: e instanceof Error ? e.message : String(e),
              }));
              await post({ type: 'editResult', requestId, ...result });
            });
            break;
          }
          case 'generateCode':
            void generateCode(document);
            break;
          case 'generateProject':
            void generateWholeProject(document);
            break;
          case 'openAsText':
            void openAsText(document.uri);
            break;
          case 'edit':
          case 'goToHandler':
            break;
        }
      }),
      vscode.workspace.onDidChangeTextDocument((e) => {
        if (e.document.uri.toString() === document.uri.toString()) void postDocument();
      }),
    ];
    panel.onDidDispose(() => {
      for (const s of subscriptions) s.dispose();
    });
  }
}

/** 画面で編集したプロジェクトを、決まった形で書き出して最小の差分で当てる。構造の検証を通らないものは書かない */
async function applyProject(
  document: vscode.TextDocument,
  project: BfprojDocument,
): Promise<{ ok: true } | { ok: false; error: string }> {
  const text = serializeProject(project);
  if (!parseProject(text).project)
    return { ok: false, error: vscode.l10n.t('The project settings are invalid') };
  const edit = minimalTextEdit(document.getText(), text);
  if (!edit) return { ok: true };
  const workspaceEdit = new vscode.WorkspaceEdit();
  workspaceEdit.replace(
    document.uri,
    new vscode.Range(document.positionAt(edit.start), document.positionAt(edit.end)),
    edit.text,
  );
  return (await vscode.workspace.applyEdit(workspaceEdit))
    ? { ok: true }
    : { ok: false, error: vscode.l10n.t('The project file could not be updated') };
}
