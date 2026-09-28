/**
 * アクティビティバーの Bethany Designer のビューに、ワークスペースのフォーム(*.bfm.json)の一覧を出す
 * (docs/designer/editor-design.md §8)。フォームが無いときは package.json の viewsWelcome の案内を出す。
 */
import * as vscode from 'vscode';
import { DesignerEditorProvider } from './designerEditorProvider.ts';
import { excludeGlob } from './excludeGlob.ts';

export const FORMS_VIEW_ID = 'bethanyDesigner.forms';
const PATTERN = '**/*.bfm.json';
const EXTENSION = '.bfm.json';
/** ファイルの作成・削除が続いたときに、一覧を作り直す回数を抑える */
const REFRESH_DELAY_MS = 300;

export function registerFormsView(): vscode.Disposable {
  const provider = new FormsProvider();
  const watcher = vscode.workspace.createFileSystemWatcher(PATTERN, false, true, false);
  return vscode.Disposable.from(
    provider,
    watcher,
    vscode.window.createTreeView(FORMS_VIEW_ID, { treeDataProvider: provider }),
    watcher.onDidCreate(() => {
      provider.refresh();
    }),
    watcher.onDidDelete(() => {
      provider.refresh();
    }),
    vscode.workspace.onDidChangeWorkspaceFolders(() => {
      provider.refresh();
    }),
    vscode.workspace.onDidChangeConfiguration((e) => {
      if (e.affectsConfiguration('search.exclude') || e.affectsConfiguration('files.exclude'))
        provider.refresh();
    }),
    vscode.commands.registerCommand('bethanyDesigner.refreshForms', () => {
      provider.refresh(0);
    }),
  );
}

class FormsProvider implements vscode.TreeDataProvider<vscode.Uri>, vscode.Disposable {
  private readonly changed = new vscode.EventEmitter<void>();
  readonly onDidChangeTreeData = this.changed.event;
  private timer: ReturnType<typeof setTimeout> | undefined;

  refresh(delay = REFRESH_DELAY_MS): void {
    clearTimeout(this.timer);
    this.timer = setTimeout(() => {
      this.changed.fire();
    }, delay);
  }

  async getChildren(element?: vscode.Uri): Promise<vscode.Uri[]> {
    if (element) return [];
    // 検索の除外(search.exclude。既定で node_modules 等)に従う
    const exclude = excludeGlob(
      vscode.workspace.getConfiguration('search').get<Record<string, unknown>>('exclude'),
      vscode.workspace.getConfiguration('files').get<Record<string, unknown>>('exclude'),
    );
    const uris = await vscode.workspace.findFiles(PATTERN, exclude);
    return uris.sort((a, b) => relativePath(a).localeCompare(relativePath(b)));
  }

  getTreeItem(uri: vscode.Uri): vscode.TreeItem {
    const fileName = uri.path.slice(uri.path.lastIndexOf('/') + 1);
    const item = new vscode.TreeItem(fileName.slice(0, -EXTENSION.length));
    // resourceUri を設定すると、ソース管理の状態(変更・未追跡)の色が付く
    item.resourceUri = uri;
    item.iconPath = new vscode.ThemeIcon('window');
    const folder = relativePath(uri).slice(0, -fileName.length).replace(/\/$/, '');
    item.description = folder;
    item.tooltip = relativePath(uri);
    item.contextValue = 'bethanyForm';
    item.command = {
      title: vscode.l10n.t('Open in Designer'),
      command: 'vscode.openWith',
      arguments: [uri, DesignerEditorProvider.viewType],
    };
    return item;
  }

  dispose(): void {
    clearTimeout(this.timer);
    this.changed.dispose();
  }
}

/** ワークスペースからの相対パス(フォルダが複数あれば、先頭にフォルダ名を付ける) */
function relativePath(uri: vscode.Uri): string {
  const multiRoot = (vscode.workspace.workspaceFolders?.length ?? 0) > 1;
  return vscode.workspace.asRelativePath(uri, multiRoot).replace(/\\/g, '/');
}
