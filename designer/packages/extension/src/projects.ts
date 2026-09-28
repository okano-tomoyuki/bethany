/**
 * プロジェクトファイル(*.bfproj.json)の読み込み・書き換え・作成と、フォームの名前の変更・移動・削除への追従
 * (docs/designer/project-spec.md §4)。
 */
import {
  createProject,
  dirname,
  isSameOrInside,
  mapFormPaths,
  minimalTextEdit,
  parseProject,
  PROJECT_EXTENSION,
  relativePath,
  resolvePath,
  serializeProject,
  type BfprojDocument,
} from '@bethany-designer/core';
import * as vscode from 'vscode';
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
 * 新しいプロジェクトファイルを、ワークスペースの先頭のフォルダの直下に Project1・Project2…の空いている名前で作る。
 * forms の先頭がメインフォームになる。
 */
export async function createProjectFile(
  forms: readonly vscode.Uri[],
): Promise<vscode.Uri | undefined> {
  const folder = vscode.workspace.workspaceFolders?.[0]?.uri;
  if (!folder) {
    void vscode.window.showErrorMessage(vscode.l10n.t('Open a folder to create a project.'));
    return undefined;
  }
  let uri = folder;
  for (let n = 1; ; n++) {
    uri = vscode.Uri.joinPath(folder, `Project${String(n)}${PROJECT_EXTENSION}`);
    if (!(await exists(uri))) break;
  }
  const doc = createProject(forms.map((form) => formPathIn(uri, form)));
  await vscode.workspace.fs.writeFile(uri, new TextEncoder().encode(serializeProject(doc)));
  return uri;
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
