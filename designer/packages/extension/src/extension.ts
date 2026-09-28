import { configureL10n } from '@no-vcl-designer/core';
import * as vscode from 'vscode';
import { DesignerEditorProvider } from './designerEditorProvider.ts';
import { registerDiagnostics } from './diagnostics.ts';
import { generateCode } from './generateCode.ts';
import { newForm } from './newForm.ts';

export function activate(context: vscode.ExtensionContext): void {
  // core・codegen のメッセージも VS Code の表示言語で表示する(tk-designer ADR 0014)
  configureL10n(vscode.env.language, vscode.l10n.bundle);
  context.subscriptions.push(
    DesignerEditorProvider.register(context),
    registerDiagnostics(),
    vscode.commands.registerCommand('noVclDesigner.newForm', (target?: vscode.Uri) =>
      newForm(target),
    ),
    vscode.commands.registerCommand('noVclDesigner.generateCode', async (target?: vscode.Uri) => {
      const uri = target ?? activeDslUri();
      if (!uri) return;
      await generateCode(await vscode.workspace.openTextDocument(uri));
    }),
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
