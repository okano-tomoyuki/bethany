/**
 * アクティビティバーの Bethany Designer のビューに、ワークスペースのフォーム(*.bfm.json)を出す
 * (docs/designer/editor-design.md §8・project-spec.md §4)。
 * プロジェクトファイル(*.bfproj.json)があればプロジェクトごとに並べ、メインフォームに ★ を付ける。無ければ平らに並べる。
 * プロジェクトに属するフォームの下には生成先のコード(C++ のヘッダ・ソース、Python のモジュール)を、プロジェクトの下には
 * 起動部分のコードを出し、エクスプローラーに切り替えずに開けるようにする。
 * プロジェクトもフォームも無いときは package.json の viewsWelcome の案内(新しいプロジェクト)を出す。
 */
import {
  formCodegenSettings,
  resolveProjectTargets,
  resolveTargets,
} from '@bethany-designer/codegen';
import { FORM_EXTENSION, hasErrors, parseDocument } from '@bethany-designer/core';
import * as vscode from 'vscode';
import { DesignerEditorProvider } from './designerEditorProvider.ts';
import { findProjects, PROJECT_PATTERN, samePath, type ProjectInfo } from './projects.ts';
import {
  baseName,
  exists,
  findWorkspaceFiles,
  readText,
  workspaceRelativePath,
} from './workspaceFiles.ts';

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
  | { readonly kind: 'unassigned'; readonly forms: readonly vscode.Uri[] }
  /** 生成先のコードのファイル(まだ生成していなければ missing) */
  | { readonly kind: 'generated'; readonly uri: vscode.Uri; readonly missing: boolean };

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
      const forms = await Promise.all(
        project.forms.map(async (uri) => ({
          kind: 'form' as const,
          uri,
          project,
          main: project.mainForm !== undefined && samePath(uri, project.mainForm),
          missing: !(await exists(uri)),
          manual: !project.autoCreate.some((f) => samePath(f, uri)),
        })),
      );
      return [...(await generatedNodes(projectOutputs(project))), ...forms];
    }
    if (node.kind === 'form' && node.project && !node.missing)
      return generatedNodes(await formOutputs(node.uri));
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
      case 'generated':
        return generatedItem(node);
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
  // プロジェクトに属するフォームは、生成先のコードを子に持つ(属さないフォームはコードを生成しない)
  if (node.project?.doc?.codegen?.cpp || node.project?.doc?.codegen?.python)
    item.collapsibleState = vscode.TreeItemCollapsibleState.Collapsed;
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

function generatedItem(node: Extract<ViewNode, { kind: 'generated' }>): vscode.TreeItem {
  const item = new vscode.TreeItem(baseName(node.uri));
  // resourceUri と ThemeIcon.File で、ファイルアイコンのテーマの拡張子ごとのアイコンにする
  item.resourceUri = node.uri;
  item.iconPath = vscode.ThemeIcon.File;
  item.tooltip = workspaceRelativePath(node.uri);
  item.contextValue = node.missing ? 'generatedMissing' : 'generated';
  if (node.missing) {
    item.description = vscode.l10n.t('Not generated yet');
    return item;
  }
  item.description = folderOf(node.uri);
  item.command = { title: '', command: 'vscode.open', arguments: [node.uri] };
  return item;
}

/** プロジェクトの起動部分(Project1.cpp・Project1.py)の出力先 */
function projectOutputs(project: ProjectInfo): vscode.Uri[] {
  if (!project.doc) return [];
  const directory = vscode.Uri.joinPath(project.uri, '..');
  const targets = resolveProjectTargets(project.doc, baseName(project.uri));
  return [targets.cpp?.main, targets.python?.main]
    .filter((path) => path !== undefined)
    .map((path) => vscode.Uri.joinPath(directory, path));
}

/**
 * フォームの生成先(C++ のヘッダ・ソース、Python のモジュール)。コード生成と同じく、フォームを含むすべてのプロジェクトの
 * codegen から決める(generateCode.ts の generateFormCode)。フォームが読めなければ空
 */
async function formOutputs(form: vscode.Uri): Promise<vscode.Uri[]> {
  const [text, projects] = await Promise.all([
    readText(form).catch(() => undefined),
    findProjects(),
  ]);
  const parsed = text === undefined ? undefined : parseDocument(text);
  if (!parsed?.document || hasErrors(parsed.diagnostics)) return [];
  const settings = formCodegenSettings(
    projects.flatMap((p) => {
      const formPath = p.doc?.forms?.[p.forms.findIndex((f) => samePath(f, form))];
      return p.doc && formPath !== undefined ? [{ doc: p.doc, formPath }] : [];
    }),
  );
  const { cpp, python } = resolveTargets(parsed.document, baseName(form), settings);
  const directory = vscode.Uri.joinPath(form, '..');
  return [cpp?.header, cpp?.source, python?.file]
    .filter((path) => path !== undefined)
    .map((path) => vscode.Uri.joinPath(directory, path));
}

async function generatedNodes(uris: readonly vscode.Uri[]): Promise<ViewNode[]> {
  return Promise.all(
    uris.map(async (uri) => ({ kind: 'generated' as const, uri, missing: !(await exists(uri)) })),
  );
}

/** ワークスペースからのフォルダ(直下なら空) */
function folderOf(uri: vscode.Uri): string {
  const path = workspaceRelativePath(uri);
  return path.slice(0, Math.max(0, path.lastIndexOf('/')));
}

function byRelativePath(a: vscode.Uri, b: vscode.Uri): number {
  return workspaceRelativePath(a).localeCompare(workspaceRelativePath(b));
}
