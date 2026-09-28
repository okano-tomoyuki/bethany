/**
 * 新しいフォーム(*.bfm.json)を作ってデザイナーで開く(docs/designer/editor-design.md §8)。
 */
import { createDocument, formNameProblem, serializeDocument } from '@bethany-designer/core';
import * as vscode from 'vscode';
import { DesignerEditorProvider } from './designerEditorProvider.ts';
import { exists } from './workspaceFiles.ts';

const EXTENSION = '.bfm.json';

/**
 * @param target 作成先のフォルダの初期値(エクスプローラーで選ばれたフォルダまたはファイル・プロジェクトのフォルダ)
 * @returns 作ったフォーム。取り消したら undefined
 */
export async function newForm(target: vscode.Uri | undefined): Promise<vscode.Uri | undefined> {
  const folder = await initialFolder(target);
  if (!folder) return undefined;

  const answer = await askName(folder, await defaultName(folder));
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
  return vscode.workspace.workspaceFolders?.[0]?.uri ?? (await pickFolder(undefined));
}

async function pickFolder(defaultUri: vscode.Uri | undefined): Promise<vscode.Uri | undefined> {
  const picked = await vscode.window.showOpenDialog({
    title: vscode.l10n.t('Folder for the New Form'),
    defaultUri,
    canSelectFiles: false,
    canSelectFolders: true,
  });
  return picked?.[0];
}

/** 名前の初期値。MainForm が既にあれば、C++Builder と同じく Form2・Form3…から空いているもの */
async function defaultName(folder: vscode.Uri): Promise<string> {
  if (!(await exists(fileUri(folder, 'MainForm')))) return 'MainForm';
  for (let n = 2; ; n++) {
    const name = `Form${String(n)}`;
    if (!(await exists(fileUri(folder, name)))) return name;
  }
}

/** フォームの名前を入力してもらう。入力欄のボタンで作成先のフォルダを変えられる */
function askName(
  initial: vscode.Uri,
  value: string,
): Promise<{ folder: vscode.Uri; name: string } | undefined> {
  let folder = initial;
  const input = vscode.window.createInputBox();
  input.title = vscode.l10n.t('New Form: Name');
  input.value = value;
  // フォルダを選ぶダイアログを開いている間も、入力欄を閉じない
  input.ignoreFocusOut = true;
  const chooseFolder: vscode.QuickInputButton = {
    iconPath: new vscode.ThemeIcon('folder-opened'),
    tooltip: vscode.l10n.t('Choose Another Folder'),
  };
  input.buttons = [chooseFolder];

  const updatePrompt = (): void => {
    input.prompt = vscode.l10n.t(
      'Creates <name>{0} in {1}. The generated class is T<name> (e.g. MainForm → TMainForm)',
      EXTENSION,
      vscode.workspace.asRelativePath(folder),
    );
  };
  const problem = async (name: string): Promise<string | undefined> =>
    formNameProblem(name) ??
    ((await exists(fileUri(folder, name)))
      ? vscode.l10n.t('A file with the same name already exists')
      : undefined);
  const validate = async (): Promise<void> => {
    const name = input.value.trim();
    const message = await problem(name);
    // 確かめている間に入力が変わっていれば、古い結果は捨てる
    if (input.value.trim() === name) input.validationMessage = message;
  };
  updatePrompt();
  void validate();

  return new Promise((resolve) => {
    input.onDidChangeValue(() => void validate());
    input.onDidTriggerButton(async (button) => {
      if (button !== chooseFolder) return;
      const picked = await pickFolder(folder);
      if (!picked) return;
      folder = picked;
      updatePrompt();
      await validate();
    });
    input.onDidAccept(async () => {
      const name = input.value.trim();
      const message = await problem(name);
      if (message) {
        input.validationMessage = message;
        return;
      }
      resolve({ folder, name });
      input.hide();
    });
    input.onDidHide(() => {
      resolve(undefined);
      input.dispose();
    });
    input.show();
  });
}

function fileUri(folder: vscode.Uri, baseName: string): vscode.Uri {
  return vscode.Uri.joinPath(folder, `${baseName}${EXTENSION}`);
}
