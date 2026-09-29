/**
 * ハンドラの定義へ移動する(docs/designer/editor-design.md §7)。デザイナーのダブルクリック・イベントのタブのボタンから呼ばれる。
 * C++Builder と同じく、先にコードを生成して(足りない雛形を追記して)から、定義の本体にカーソルを置いて開く。
 */
import { findHandler, type HandlerLanguage } from '@bethany-designer/codegen';
import * as vscode from 'vscode';
import { generateFormCode } from './generateCode.ts';

const SETTING = 'goToHandler.language';
/** 「最初に選んだ言語を覚える」ときの保存先(ワークスペースごと) */
const REMEMBERED = 'bethanyDesigner.goToHandler.language';

type LanguageSetting = 'auto' | 'ask' | HandlerLanguage;

interface Candidate {
  readonly language: HandlerLanguage;
  readonly label: string;
  readonly uri: vscode.Uri;
  readonly className: string;
}

export async function goToHandler(
  context: vscode.ExtensionContext,
  document: vscode.TextDocument,
  handler: string,
): Promise<void> {
  const result = await generateFormCode(document, { quiet: true });
  if (!result) return;
  const { directory, targets } = result;
  const candidates: Candidate[] = [
    ...(targets.cpp
      ? [
          {
            language: 'cpp' as const,
            label: 'C++',
            uri: vscode.Uri.joinPath(directory, targets.cpp.source),
            className: targets.cpp.className,
          },
        ]
      : []),
    ...(targets.python
      ? [
          {
            language: 'python' as const,
            label: 'Python',
            uri: vscode.Uri.joinPath(directory, targets.python.file),
            className: targets.python.className,
          },
        ]
      : []),
  ];
  const target = await chooseTarget(context, candidates);
  if (!target) return;

  const code = await vscode.workspace.openTextDocument(target.uri);
  const position = findHandler(target.language, code.getText(), target.className, handler);
  if (!position) {
    void vscode.window.showWarningMessage(
      vscode.l10n.t('The handler {0} was not found in {1}.', handler, fileName(target.uri)),
    );
    return;
  }
  const at = new vscode.Position(position.line, position.character);
  // 既にどこかのエディタで開いていればそこで、なければデザイナーと同じグループで開く(C++Builder の F12 と同じ)
  const visible = vscode.window.visibleTextEditors.find(
    (e) => e.document.uri.toString() === target.uri.toString(),
  );
  await vscode.window.showTextDocument(code, {
    viewColumn: visible?.viewColumn ?? vscode.ViewColumn.Active,
    selection: new vscode.Range(at, at),
    preview: false,
  });
}

/** 移動先の言語を決める。両方を生成するときは、設定(既定は最初に選んだものを覚える)に従う */
async function chooseTarget(
  context: vscode.ExtensionContext,
  candidates: readonly Candidate[],
): Promise<Candidate | undefined> {
  if (candidates.length <= 1) return candidates[0];
  const setting = vscode.workspace
    .getConfiguration('bethanyDesigner')
    .get<LanguageSetting>(SETTING, 'auto');
  const preferred =
    setting === 'cpp' || setting === 'python'
      ? setting
      : setting === 'auto'
        ? context.workspaceState.get<HandlerLanguage>(REMEMBERED)
        : undefined;
  const found = candidates.find((c) => c.language === preferred);
  if (found) return found;

  const picked = await vscode.window.showQuickPick(
    candidates.map((c) => ({ label: c.label, description: fileName(c.uri), candidate: c })),
    {
      placeHolder: vscode.l10n.t('Open the handler in which language?'),
      // ダブルクリックの直後はフォーカスが Webview に戻るため、フォーカスが外れても閉じないようにする
      ignoreFocusOut: true,
    },
  );
  if (!picked) return undefined;
  if (setting === 'auto') {
    await context.workspaceState.update(REMEMBERED, picked.candidate.language);
    const change = vscode.l10n.t('Change');
    void vscode.window
      .showInformationMessage(
        vscode.l10n.t(
          'Handlers will be opened in {0} in this workspace. You can change this in the settings.',
          picked.label,
        ),
        change,
      )
      .then((answer) => {
        if (answer === change)
          void vscode.commands.executeCommand(
            'workbench.action.openSettings',
            `bethanyDesigner.${SETTING}`,
          );
      });
  }
  return picked.candidate;
}

function fileName(uri: vscode.Uri): string {
  return uri.path.split('/').pop() ?? uri.path;
}
