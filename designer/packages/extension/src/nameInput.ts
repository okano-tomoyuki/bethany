/**
 * 新しいフォーム・新しいプロジェクトの名前を入力してもらう入力欄(docs/designer/editor-design.md §8・project-spec.md §4)。
 * 入力欄のボタンで作成先のフォルダを変えられる。
 */
import * as vscode from 'vscode';

export interface NameInputOptions {
  readonly title: string;
  /** 作成先のフォルダの初期値 */
  readonly folder: vscode.Uri;
  /** 名前の初期値 */
  readonly value: string;
  /** フォルダを選ぶダイアログの見出し */
  readonly folderDialogTitle: string;
  /** 入力欄の下の説明(作成先のフォルダのワークスペースからのパスを受け取る) */
  prompt(folder: string): string;
  /** 名前と作成先のフォルダで作れなければ、その理由 */
  problem(name: string, folder: vscode.Uri): Promise<string | undefined>;
}

/** 取り消したら undefined */
export function askName(
  options: NameInputOptions,
): Promise<{ folder: vscode.Uri; name: string } | undefined> {
  let { folder } = options;
  const input = vscode.window.createInputBox();
  input.title = options.title;
  input.value = options.value;
  // フォルダを選ぶダイアログを開いている間も、入力欄を閉じない
  input.ignoreFocusOut = true;
  const chooseFolder: vscode.QuickInputButton = {
    iconPath: new vscode.ThemeIcon('folder-opened'),
    tooltip: vscode.l10n.t('Choose Another Folder'),
  };
  input.buttons = [chooseFolder];

  const updatePrompt = (): void => {
    input.prompt = options.prompt(vscode.workspace.asRelativePath(folder));
  };
  const validate = async (): Promise<void> => {
    const name = input.value.trim();
    const message = await options.problem(name, folder);
    // 確かめている間に入力が変わっていれば、古い結果は捨てる
    if (input.value.trim() === name) input.validationMessage = message;
  };
  updatePrompt();
  void validate();

  return new Promise((resolve) => {
    input.onDidChangeValue(() => void validate());
    input.onDidTriggerButton(async (button) => {
      if (button !== chooseFolder) return;
      const picked = await pickFolder(options.folderDialogTitle, folder);
      if (!picked) return;
      folder = picked;
      updatePrompt();
      await validate();
    });
    input.onDidAccept(async () => {
      const name = input.value.trim();
      const message = await options.problem(name, folder);
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

export async function pickFolder(
  title: string,
  defaultUri: vscode.Uri | undefined,
): Promise<vscode.Uri | undefined> {
  const picked = await vscode.window.showOpenDialog({
    title,
    defaultUri,
    canSelectFiles: false,
    canSelectFolders: true,
  });
  return picked?.[0];
}
