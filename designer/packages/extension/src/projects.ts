/**
 * プロジェクトファイル(*.bfproj.json)の読み込み・書き換え・作成と、フォームの名前の変更・移動・削除への追従
 * (docs/designer/project-spec.md §4)。
 */
import {
  autoCreateForms,
  FORM_EXTENSION,
  createProject,
  dirname,
  isJapanese,
  isSameOrInside,
  mapFormPaths,
  minimalTextEdit,
  parseProject,
  PROJECT_EXTENSION,
  projectNameProblem,
  relativePath,
  resolvePath,
  serializeProject,
  type BfprojDocument,
} from '@bethany-designer/core';
import * as vscode from 'vscode';
import { askName } from './nameInput.ts';
import { baseName, exists, findWorkspaceFiles, readText } from './workspaceFiles.ts';

export const PROJECT_PATTERN = `**/*${PROJECT_EXTENSION}`;

export interface ProjectInfo {
  readonly uri: vscode.Uri;
  /** ファイル名から .bfproj.json を除いたもの */
  readonly name: string;
  /** 構造の検証を通らなければ undefined */
  readonly doc: BfprojDocument | undefined;
  /** forms の順のフォーム */
  readonly forms: readonly vscode.Uri[];
  readonly mainForm: vscode.Uri | undefined;
  /** 起動時に作るフォーム(作る順。メインフォームが先頭) */
  readonly autoCreate: readonly vscode.Uri[];
}

export async function findProjects(): Promise<ProjectInfo[]> {
  const uris = await findWorkspaceFiles(PROJECT_PATTERN);
  const projects = await Promise.all(uris.map(loadProject));
  return projects.sort((a, b) => a.uri.path.localeCompare(b.uri.path));
}

async function loadProject(uri: vscode.Uri): Promise<ProjectInfo> {
  const name = baseName(uri).slice(0, -PROJECT_EXTENSION.length);
  let doc: BfprojDocument | undefined;
  try {
    doc = parseProject(await readText(uri)).project;
  } catch {
    doc = undefined;
  }
  return {
    uri,
    name,
    doc,
    forms: (doc?.forms ?? []).map((form) => formUri(uri, form)),
    mainForm: doc?.mainForm === undefined ? undefined : formUri(uri, doc.mainForm),
    autoCreate: doc ? autoCreateForms(doc).map((form) => formUri(uri, form)) : [],
  };
}

/** プロジェクトファイルからの相対パスのフォーム */
export function formUri(project: vscode.Uri, form: string): vscode.Uri {
  return project.with({ path: resolvePath(dirname(project.path), form) });
}

/** プロジェクトファイルから見たフォームの相対パス */
export function formPathIn(project: vscode.Uri, form: vscode.Uri): string {
  return relativePath(dirname(project.path), form.path);
}

export function samePath(a: vscode.Uri, b: vscode.Uri): boolean {
  return a.path === b.path;
}

/**
 * プロジェクトファイルを書き換えて保存する。開いているエディタにも反映され、元に戻せる。
 * 構造の検証を通らないファイルは書き換えない(利用者の書いた内容を壊さないように)。
 */
export async function editProject(
  uri: vscode.Uri,
  fn: (doc: BfprojDocument) => BfprojDocument,
): Promise<boolean> {
  const document = await vscode.workspace.openTextDocument(uri);
  const text = document.getText();
  const { project } = parseProject(text);
  if (!project) {
    void vscode.window.showErrorMessage(
      vscode.l10n.t(
        'Cannot update {0} because it has errors. See the Problems panel.',
        baseName(uri),
      ),
    );
    return false;
  }
  const changed = fn(project);
  if (JSON.stringify(changed) === JSON.stringify(project)) return true;
  const edit = minimalTextEdit(text, serializeProject(changed));
  if (edit) {
    const workspaceEdit = new vscode.WorkspaceEdit();
    workspaceEdit.replace(
      uri,
      new vscode.Range(document.positionAt(edit.start), document.positionAt(edit.end)),
      edit.text,
    );
    await vscode.workspace.applyEdit(workspaceEdit);
  }
  return document.save();
}

/**
 * 新しいプロジェクトファイルを作る。名前(初期値は Project1・Project2…の空いているもの)と作成先のフォルダ
 * (初期値はワークスペースの先頭のフォルダ)を入力してもらう。forms の先頭がメインフォームになる。
 * @returns 作ったプロジェクトファイル。取り消したら undefined
 */
export async function createProjectFile(
  forms: readonly vscode.Uri[],
): Promise<vscode.Uri | undefined> {
  const folder = vscode.workspace.workspaceFolders?.[0]?.uri;
  if (!folder) {
    void vscode.window.showErrorMessage(vscode.l10n.t('Open a folder to create a project.'));
    return undefined;
  }
  const answer = await askName({
    title: vscode.l10n.t('New Project: Name'),
    folder,
    value: await defaultProjectName(folder),
    folderDialogTitle: vscode.l10n.t('Folder for the New Project'),
    prompt: (path) =>
      vscode.l10n.t(
        'Creates <name>{0} in {1}. The startup code is generated as <name>.cpp and <name>.py',
        PROJECT_EXTENSION,
        path,
      ),
    problem: async (name, at) => {
      const problem = projectNameProblem(name);
      if (problem) return problem;
      // ワークスペースの外のプロジェクトは、フォームのビューに出ない
      if (!vscode.workspace.getWorkspaceFolder(at))
        return vscode.l10n.t('Choose a folder in the workspace');
      if (await exists(projectUri(at, name)))
        return vscode.l10n.t('A file with the same name already exists');
      // 同じフォルダの同じ名前のフォームとは、起動部分とフォームのコードのファイル名(<name>.cpp・<name>.py)が重なる
      if (await exists(vscode.Uri.joinPath(at, `${name}${FORM_EXTENSION}`)))
        return vscode.l10n.t(
          'A form with the same name exists in this folder. The startup code would overwrite its code',
        );
      return undefined;
    },
  });
  if (!answer) return undefined;
  const uri = projectUri(answer.folder, answer.name);
  // 生成するコードのコメントの言語は、作成した人の表示言語を初期値にする(新しいフォームと同じ)
  const doc = createProject(
    forms.map((form) => formPathIn(uri, form)),
    isJapanese(vscode.env.language) ? 'ja' : 'en',
  );
  await vscode.workspace.fs.writeFile(uri, new TextEncoder().encode(serializeProject(doc)));
  return uri;
}

/** 名前の初期値。C++Builder と同じく Project1・Project2…から空いているもの */
async function defaultProjectName(folder: vscode.Uri): Promise<string> {
  for (let n = 1; ; n++) {
    const name = `Project${String(n)}`;
    if (!(await exists(projectUri(folder, name)))) return name;
  }
}

function projectUri(folder: vscode.Uri, name: string): vscode.Uri {
  return vscode.Uri.joinPath(folder, `${name}${PROJECT_EXTENSION}`);
}

/** VS Code の中でのフォーム(とフォルダ)の名前の変更・移動・削除を、プロジェクトファイルに反映する */
export function registerProjectTracking(): vscode.Disposable {
  return vscode.Disposable.from(
    vscode.workspace.onDidRenameFiles(async (e) => {
      const renames = e.files.map((f) => ({ from: f.oldUri.path, to: f.newUri.path }));
      /** 名前の変更の前のパスを、変更の後のパスにする */
      const moved = (path: string): string => {
        const rename = renames.find((r) => isSameOrInside(path, r.from));
        return rename ? rename.to + path.slice(rename.from.length) : path;
      };
      for (const project of await findProjects()) {
        if (!project.doc) continue;
        // プロジェクトファイル自体が移ったなら、相対パスは移る前の場所から読む
        const now = project.uri.path;
        const rename = renames.find((r) => isSameOrInside(now, r.to));
        const before = rename ? rename.from + now.slice(rename.to.length) : now;
        await editProject(project.uri, (doc) =>
          mapFormPaths(doc, (form) =>
            relativePath(dirname(now), moved(resolvePath(dirname(before), form))),
          ),
        );
      }
    }),
    vscode.workspace.onDidDeleteFiles(async (e) => {
      const deleted = e.files.map((f) => f.path);
      for (const project of await findProjects()) {
        if (!project.doc) continue;
        await editProject(project.uri, (doc) =>
          mapFormPaths(doc, (form) => {
            const path = resolvePath(dirname(project.uri.path), form);
            return deleted.some((d) => isSameOrInside(path, d)) ? null : form;
          }),
        );
      }
    }),
  );
}
