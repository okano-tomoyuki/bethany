/**
 * フォームのビューから使うプロジェクトのコマンド(docs/designer/project-spec.md §4)。
 * メインフォームに設定・プロジェクトに追加・プロジェクトから外す・新しいプロジェクト・新しいフォーム(作った後にプロジェクトへ加える)。
 */
import { addForm, removeForm, setAutoCreate, setMainForm } from '@bethany-designer/core';
import * as vscode from 'vscode';
import type { FormNode, ProjectNode, ViewNode } from './formsView.ts';
import { newForm } from './newForm.ts';
import {
  createProjectFile,
  editProject,
  findProjects,
  formPathIn,
  samePath,
  type ProjectInfo,
} from './projects.ts';
import { workspaceRelativePath } from './workspaceFiles.ts';

export function registerProjectCommands(): vscode.Disposable {
  return vscode.Disposable.from(
    vscode.commands.registerCommand('bethanyDesigner.newForm', newFormAndAdd),
    vscode.commands.registerCommand('bethanyDesigner.newProject', async () => {
      await createProjectFile([]);
      refreshView();
    }),
    vscode.commands.registerCommand(
      'bethanyDesigner.openProjectFile',
      async (node: ProjectNode) => {
        // 既定のエディタ(プロジェクトの設定画面)で開く
        await vscode.commands.executeCommand('vscode.open', node.project.uri);
      },
    ),
    vscode.commands.registerCommand('bethanyDesigner.setMainForm', async (node: FormNode) => {
      const project = node.project ?? (await chooseProject(node.uri, { createIfNone: true }));
      if (project === 'new') await createProjectFile([node.uri]);
      else if (project)
        await editProject(project.uri, (doc) =>
          setMainForm(doc, formPathIn(project.uri, node.uri)),
        );
      refreshView();
    }),
    vscode.commands.registerCommand('bethanyDesigner.addToProject', async (node: FormNode) => {
      const project = await chooseProject(node.uri, { createIfNone: true });
      if (project === 'new') await createProjectFile([node.uri]);
      else if (project)
        await editProject(project.uri, (doc) => addForm(doc, formPathIn(project.uri, node.uri)));
      refreshView();
    }),
    vscode.commands.registerCommand('bethanyDesigner.createAtStartup', (node: FormNode) =>
      toggleAutoCreate(node, true),
    ),
    vscode.commands.registerCommand('bethanyDesigner.dontCreateAtStartup', (node: FormNode) =>
      toggleAutoCreate(node, false),
    ),
    vscode.commands.registerCommand('bethanyDesigner.removeFromProject', async (node: FormNode) => {
      const { project } = node;
      if (!project) return;
      await editProject(project.uri, (doc) => removeForm(doc, formPathIn(project.uri, node.uri)));
      refreshView();
    }),
  );
}

/**
 * 新しいフォームを作り、プロジェクトに加える。プロジェクトの「+」からならそのプロジェクトに、それ以外は
 * プロジェクトが 1 つならそれに加え、複数なら加える先を選んでもらう(加えないこともできる)。
 * @param target エクスプローラーで選ばれたフォルダ・ファイル、またはビューのプロジェクト
 */
async function newFormAndAdd(target?: vscode.Uri | ViewNode): Promise<void> {
  const projectNode = target && !(target instanceof vscode.Uri) && target.kind === 'project';
  const project = projectNode ? target.project : undefined;
  const folder = project
    ? vscode.Uri.joinPath(project.uri, '..')
    : target instanceof vscode.Uri
      ? target
      : undefined;
  const form = await newForm(folder);
  if (!form) return;

  const destination = project ?? (await chooseProject(form, { allowNone: true }));
  if (destination && destination !== 'new')
    await editProject(destination.uri, (doc) => addForm(doc, formPathIn(destination.uri, form)));
  refreshView();
}

/** 起動時に作るか(プロジェクトの autoCreate)を切り替える */
async function toggleAutoCreate(node: FormNode, on: boolean): Promise<void> {
  const { project } = node;
  if (!project) return;
  await editProject(project.uri, (doc) =>
    setAutoCreate(doc, formPathIn(project.uri, node.uri), on),
  );
  refreshView();
}

interface ChooseOptions {
  /** プロジェクトが無ければ、新しく作る('new' を返す) */
  readonly createIfNone?: boolean;
  /** 「加えない」を選べるようにする。プロジェクトが 1 つなら聞かずにそれを返す */
  readonly allowNone?: boolean;
}

/** フォームを加えるプロジェクトを選んでもらう。そのフォームを既に含むプロジェクトは除く */
async function chooseProject(
  form: vscode.Uri,
  options: ChooseOptions,
): Promise<ProjectInfo | 'new' | undefined> {
  const all = await findProjects();
  if (all.length === 0) return options.createIfNone ? 'new' : undefined;
  const candidates = all.filter((p) => p.doc && !p.forms.some((f) => samePath(f, form)));
  if (options.allowNone && all.length === 1) return candidates[0];

  type Item = vscode.QuickPickItem & { readonly value: ProjectInfo | 'new' | undefined };
  const items: Item[] = [
    ...candidates.map((p) => ({
      label: p.name,
      description: workspaceRelativePath(p.uri),
      iconPath: new vscode.ThemeIcon('project'),
      value: p,
    })),
    options.allowNone
      ? { label: vscode.l10n.t("Don't Add to a Project"), value: undefined }
      : {
          label: vscode.l10n.t('New Project'),
          iconPath: new vscode.ThemeIcon('add'),
          value: 'new' as const,
        },
  ];
  const picked = await vscode.window.showQuickPick(items, {
    title: vscode.l10n.t('Add {0} to a Project', workspaceRelativePath(form)),
    placeHolder: vscode.l10n.t('Choose a project'),
  });
  return picked?.value;
}

function refreshView(): void {
  void vscode.commands.executeCommand('bethanyDesigner.refreshForms');
}
