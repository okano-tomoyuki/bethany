import { configureL10n } from '@bethany-designer/core';
import * as vscode from 'vscode';
import { DesignerEditorProvider } from './designerEditorProvider.ts';
import { registerDiagnostics } from './diagnostics.ts';
import { registerFormsView, type ViewNode } from './formsView.ts';
import { generateCode } from './generateCode.ts';
import { registerProjectCommands } from './projectCommands.ts';
import { ProjectEditorProvider } from './projectEditorProvider.ts';
import { registerProjectTracking } from './projects.ts';

export function activate(context: vscode.ExtensionContext): void {
  // core・codegen のメッセージも VS Code の表示言語で表示する(tk-designer ADR 0014)
  configureL10n(vscode.env.language, vscode.l10n.bundle);
  context.subscriptions.push(
    DesignerEditorProvider.register(context),
    ProjectEditorProvider.register(context),
    registerDiagnostics(),
    registerFormsView(),
    registerProjectCommands(),
    registerProjectTracking(),
    vscode.commands.registerCommand(
      'bethanyDesigner.generateCode',
      async (target?: vscode.Uri | ViewNode) => {
        // エクスプローラー・エディタのタイトルからは Uri、フォームのビューからはその項目が渡される
        const uri =
          target instanceof vscode.Uri
            ? target
            : target?.kind === 'form'
              ? target.uri
              : target?.kind === 'project'
                ? target.project.uri
                : activeDslUri();
        if (!uri) return;
        await generateCode(await vscode.workspace.openTextDocument(uri));
      },
    ),
  );
}

export function deactivate(): void {}

/** 開いている DSL(デザイナーかテキストエディタ) */
function activeDslUri(): vscode.Uri | undefined {
  const input = vscode.window.tabGroups.activeTabGroup.activeTab?.input;
  if (input instanceof vscode.TabInputCustom || input instanceof vscode.TabInputText)
    return input.uri;
  return undefined;
}
