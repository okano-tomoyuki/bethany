/**
 * 新しいフォーム(*.bfm.json)を作ってデザイナーで開く(docs/designer/editor-design.md §8)。
 */
import { createDocument, formNameProblem, serializeDocument } from '@bethany-designer/core';
import * as vscode from 'vscode';
import { DesignerEditorProvider } from './designerEditorProvider.ts';
import { askName, pickFolder } from './nameInput.ts';
import { exists } from './workspaceFiles.ts';

const EXTENSION = '.bfm.json';

/**
 * @param target 作成先のフォルダの初期値(エクスプローラーで選ばれたフォルダまたはファイル・プロジェクトのフォルダ)
 * @returns 作ったフォーム。取り消したら undefined
 */
export async function newForm(target: vscode.Uri | undefined): Promise<vscode.Uri | undefined> {
  const folder = await initialFolder(target);
  if (!folder) return undefined;

  const answer = await askName({
    title: vscode.l10n.t('New Form: Name'),
    folder,
    value: await defaultName(folder),
    folderDialogTitle: vscode.l10n.t('Folder for the New Form'),
    prompt: (path) =>
      vscode.l10n.t(
        'Creates <name>{0} in {1}. The generated class is T<name> (e.g. MainForm → TMainForm)',
        EXTENSION,
        path,
      ),
    problem: async (name, at) =>
      formNameProblem(name) ??
      ((await exists(fileUri(at, name)))
        ? vscode.l10n.t('A file with the same name already exists')
        : undefined),
  });
  if (!answer) return undefined;
  const uri = fileUri(answer.folder, answer.name);

  const text = serializeDocument(createDocument(answer.name));
  await vscode.workspace.fs.writeFile(uri, new TextEncoder().encode(text));
  await vscode.commands.executeCommand('vscode.openWith', uri, DesignerEditorProvider.viewType);
  return uri;
}

/**
 * 作成先のフォルダの初期値。エクスプローラーで選ばれたものを優先し、なければワークスペースの先頭のフォルダ
 * (名前の入力欄のボタンで変えられる)。フォルダを開いていなければ、ダイアログで選んでもらう。
 */
async function initialFolder(target: vscode.Uri | undefined): Promise<vscode.Uri | undefined> {
  if (target) {
    const stat = await vscode.workspace.fs.stat(target);
    return stat.type & vscode.FileType.Directory ? target : vscode.Uri.joinPath(target, '..');
  }
  return (
    vscode.workspace.workspaceFolders?.[0]?.uri ??
    (await pickFolder(vscode.l10n.t('Folder for the New Form'), undefined))
  );
}

/** 名前の初期値。MainForm が既にあれば、C++Builder と同じく Form2・Form3…から空いているもの */
async function defaultName(folder: vscode.Uri): Promise<string> {
  if (!(await exists(fileUri(folder, 'MainForm')))) return 'MainForm';
  for (let n = 2; ; n++) {
    const name = `Form${String(n)}`;
    if (!(await exists(fileUri(folder, name)))) return name;
  }
}

function fileUri(folder: vscode.Uri, baseName: string): vscode.Uri {
  return vscode.Uri.joinPath(folder, `${baseName}${EXTENSION}`);
}
