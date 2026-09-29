/**
 * アクティビティバーの Bethany Designer のビューに、ワークスペースのフォーム(*.bfm.json)を出す
 * (docs/designer/editor-design.md §8・project-spec.md §4)。
 * プロジェクトファイル(*.bfproj.json)があればプロジェクトごとに並べ、メインフォームに ★ を付ける。無ければ平らに並べる。
 * フォームが無いときは package.json の viewsWelcome の案内を出す。
 */
import { FORM_EXTENSION } from '@bethany-designer/core';
import * as vscode from 'vscode';
import { DesignerEditorProvider } from './designerEditorProvider.ts';
import { findProjects, PROJECT_PATTERN, samePath, type ProjectInfo } from './projects.ts';
import { baseName, exists, findWorkspaceFiles, workspaceRelativePath } from './workspaceFiles.ts';

export const FORMS_VIEW_ID = 'bethanyDesigner.forms';
const FORM_PATTERN = `**/*${FORM_EXTENSION}`;
/** ファイルの作成・削除が続いたときに、一覧を作り直す回数を抑える */
const REFRESH_DELAY_MS = 300;

export type ViewNode =
  | { readonly kind: 'project'; readonly project: ProjectInfo }
  | {
      readonly kind: 'form';
      readonly uri: vscode.Uri;
      /** 属するプロジェクト(プロジェクトの下に出すとき) */
      readonly project?: ProjectInfo;
      readonly main?: boolean;
      readonly missing?: boolean;
      /** 起動時に作らない(プロジェクトの autoCreate に無い) */
      readonly manual?: boolean;
    }
  | { readonly kind: 'unassigned'; readonly forms: readonly vscode.Uri[] };

export type FormNode = Extract<ViewNode, { kind: 'form' }>;
export type ProjectNode = Extract<ViewNode, { kind: 'project' }>;

export function registerFormsView(): vscode.Disposable {
  const provider = new FormsProvider();
  const formWatcher = vscode.workspace.createFileSystemWatcher(FORM_PATTERN, false, true, false);
  const projectWatcher = vscode.workspace.createFileSystemWatcher(PROJECT_PATTERN);
  const refresh = (): void => {
    provider.refresh();
  };
  return vscode.Disposable.from(
    provider,
    formWatcher,
    projectWatcher,
    vscode.window.createTreeView(FORMS_VIEW_ID, { treeDataProvider: provider }),
    formWatcher.onDidCreate(refresh),
    formWatcher.onDidDelete(refresh),
    projectWatcher.onDidCreate(refresh),
    projectWatcher.onDidChange(refresh),
    projectWatcher.onDidDelete(refresh),
    // 保存する前のプロジェクトファイルの変更も反映する
    vscode.workspace.onDidChangeTextDocument((e) => {
      if (e.document.uri.path.endsWith('.bfproj.json')) refresh();
    }),
    vscode.workspace.onDidChangeWorkspaceFolders(refresh),
    vscode.workspace.onDidChangeConfiguration((e) => {
      if (e.affectsConfiguration('search.exclude') || e.affectsConfiguration('files.exclude'))
        refresh();
    }),
    vscode.commands.registerCommand('bethanyDesigner.refreshForms', () => {
      provider.refresh(0);
    }),
  );
}

class FormsProvider implements vscode.TreeDataProvider<ViewNode>, vscode.Disposable {
  private readonly changed = new vscode.EventEmitter<void>();
  readonly onDidChangeTreeData = this.changed.event;
  private timer: ReturnType<typeof setTimeout> | undefined;

  refresh(delay = REFRESH_DELAY_MS): void {
    clearTimeout(this.timer);
    this.timer = setTimeout(() => {
      this.changed.fire();
    }, delay);
  }

  async getChildren(node?: ViewNode): Promise<ViewNode[]> {
    if (!node) return this.roots();
    if (node.kind === 'unassigned') return node.forms.map((uri) => ({ kind: 'form', uri }));
    if (node.kind === 'project') {
      const { project } = node;
      return Promise.all(
        project.forms.map(async (uri) => ({
          kind: 'form' as const,
          uri,
          project,
          main: project.mainForm !== undefined && samePath(uri, project.mainForm),
          missing: !(await exists(uri)),
          manual: !project.autoCreate.some((f) => samePath(f, uri)),
        })),
      );
    }
    return [];
  }

  private async roots(): Promise<ViewNode[]> {
    const [forms, projects] = await Promise.all([
      findWorkspaceFiles(FORM_PATTERN).then((uris) => uris.sort(byRelativePath)),
      findProjects(),
    ]);
    if (projects.length === 0) return forms.map((uri) => ({ kind: 'form', uri }));
    const unassigned = forms.filter(
      (form) => !projects.some((p) => p.forms.some((f) => samePath(f, form))),
    );
    return [
      ...projects.map((project) => ({ kind: 'project' as const, project })),
      ...(unassigned.length > 0 ? [{ kind: 'unassigned' as const, forms: unassigned }] : []),
    ];
  }

  getTreeItem(node: ViewNode): vscode.TreeItem {
    switch (node.kind) {
      case 'project':
        return projectItem(node.project);
      case 'unassigned': {
        const item = new vscode.TreeItem(
          vscode.l10n.t('Forms Not in a Project'),
          vscode.TreeItemCollapsibleState.Expanded,
        );
        item.iconPath = new vscode.ThemeIcon('folder');
        item.contextValue = 'unassigned';
        return item;
      }
      case 'form':
        return formItem(node);
    }
  }

  dispose(): void {
    clearTimeout(this.timer);
    this.changed.dispose();
  }
}

function projectItem(project: ProjectInfo): vscode.TreeItem {
  const item = new vscode.TreeItem(project.name, vscode.TreeItemCollapsibleState.Expanded);
  item.resourceUri = project.uri;
  item.description = folderOf(project.uri);
  item.tooltip = workspaceRelativePath(project.uri);
  if (project.doc) {
    item.iconPath = new vscode.ThemeIcon('project');
    item.contextValue = 'project';
    // クリックで設定画面(既定のエディタ)を開く
    item.command = { title: '', command: 'vscode.open', arguments: [project.uri] };
  } else {
    // 読めないプロジェクトファイル(問題パネルに理由が出る)
    item.iconPath = new vscode.ThemeIcon('error');
    item.description = vscode.l10n.t('Cannot be read');
    item.contextValue = 'brokenProject';
    item.collapsibleState = vscode.TreeItemCollapsibleState.None;
    item.command = { title: '', command: 'vscode.open', arguments: [project.uri] };
  }
  return item;
}

function formItem(node: FormNode): vscode.TreeItem {
  const fileName = baseName(node.uri);
  const item = new vscode.TreeItem(fileName.slice(0, -FORM_EXTENSION.length));
  // resourceUri を設定すると、ソース管理の状態(変更・未追跡)の色が付く
  item.resourceUri = node.uri;
  item.tooltip = workspaceRelativePath(node.uri);
  const folder = folderOf(node.uri);
  if (node.missing) {
    item.iconPath = new vscode.ThemeIcon('warning');
    item.description = vscode.l10n.t('File not found');
    item.contextValue = 'projectMissingForm';
    return item;
  }
  if (node.main) {
    item.iconPath = new vscode.ThemeIcon('star-full');
    item.description = [vscode.l10n.t('Main form'), folder].filter(Boolean).join(' · ');
  } else {
    item.iconPath = new vscode.ThemeIcon('window');
    item.description = [node.manual ? vscode.l10n.t('Not created at startup') : '', folder]
      .filter(Boolean)
      .join(' · ');
  }
  item.contextValue = !node.project
    ? 'form'
    : node.main
      ? 'projectMainForm'
      : node.manual
        ? 'projectFormManual'
        : 'projectForm';
  item.command = {
    title: vscode.l10n.t('Open in Designer'),
    command: 'vscode.openWith',
    arguments: [node.uri, DesignerEditorProvider.viewType],
  };
  return item;
}

/** ワークスペースからのフォルダ(直下なら空) */
function folderOf(uri: vscode.Uri): string {
  const path = workspaceRelativePath(uri);
  return path.slice(0, Math.max(0, path.lastIndexOf('/')));
}

function byRelativePath(a: vscode.Uri, b: vscode.Uri): number {
  return workspaceRelativePath(a).localeCompare(workspaceRelativePath(b));
}
