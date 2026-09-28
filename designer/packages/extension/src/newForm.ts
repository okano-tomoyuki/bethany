/**
 * 新しいフォーム(*.nvform.json)を作ってデザイナーで開く(docs/designer/editor-design.md §8)。
 */
import {
  createDocument,
  formNameProblem,
  isJapanese,
  serializeDocument,
} from '@no-vcl-designer/core';
import * as vscode from 'vscode';
import { DesignerEditorProvider } from './designerEditorProvider.ts';

const EXTENSION = '.nvform.json';

/**
 * @param target エクスプローラーのコンテキストメニューから呼ばれた場合の、選ばれたフォルダまたはファイル
 */
export async function newForm(target: vscode.Uri | undefined): Promise<void> {
  const folder = await targetFolder(target);
  if (!folder) return;

  const name = await vscode.window.showInputBox({
    title: vscode.l10n.t('New Form: Name'),
    prompt: vscode.l10n.t(
      'Creates <name>{0} in {1}. The generated class is T<name> (e.g. MainForm → TMainForm)',
      EXTENSION,
      vscode.workspace.asRelativePath(folder),
    ),
    value: 'MainForm',
    validateInput: async (value) => {
      const trimmed = value.trim();
      const problem = formNameProblem(trimmed);
      if (problem) return problem;
      return (await exists(fileUri(folder, trimmed)))
        ? vscode.l10n.t('A file with the same name already exists')
        : undefined;
    },
  });
  if (name === undefined) return;
  const formName = name.trim();
  const uri = fileUri(folder, formName);

  // 生成するコードのコメントの言語は、作成した人の表示言語を初期値にする(tk-designer ADR 0014)
  const commentLocale = isJapanese(vscode.env.language) ? 'ja' : 'en';
  const text = serializeDocument(createDocument(formName, commentLocale));
  await vscode.workspace.fs.writeFile(uri, new TextEncoder().encode(text));
  await vscode.commands.executeCommand('vscode.openWith', uri, DesignerEditorProvider.viewType);
}

/** 作成先のフォルダ。エクスプローラーで選ばれたものを優先し、なければワークスペースのフォルダを使う */
async function targetFolder(target: vscode.Uri | undefined): Promise<vscode.Uri | undefined> {
  if (target) {
    const stat = await vscode.workspace.fs.stat(target);
    return stat.type & vscode.FileType.Directory ? target : vscode.Uri.joinPath(target, '..');
  }
  const folders = vscode.workspace.workspaceFolders ?? [];
  if (folders.length === 1) return folders[0]?.uri;
  if (folders.length > 1) return (await vscode.window.showWorkspaceFolderPick())?.uri;

  // フォルダを開いていなければ、保存先を選んでもらう
  const picked = await vscode.window.showOpenDialog({
    title: vscode.l10n.t('Folder for the New Form'),
    canSelectFiles: false,
    canSelectFolders: true,
  });
  return picked?.[0];
}

function fileUri(folder: vscode.Uri, baseName: string): vscode.Uri {
  return vscode.Uri.joinPath(folder, `${baseName}${EXTENSION}`);
}

async function exists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}
