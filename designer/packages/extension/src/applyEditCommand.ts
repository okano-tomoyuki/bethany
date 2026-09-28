import {
  applyCommand,
  minimalTextEdit,
  parseDocument,
  serializeDocument,
  type EditCommand,
} from '@no-vcl-designer/core';
import * as vscode from 'vscode';

export type ApplyResult = { readonly ok: true } | { readonly ok: false; readonly error: string };

/**
 * 編集コマンドを TextDocument の現在の内容に適用し、WorkspaceEdit で反映する(tk-designer ADR 0006)。
 * コマンドは意図(どのノードをどう変えるか)で表されるので、Webview が見ている版と多少ずれていても適用できる。
 * 結果は決まった形で書き出す。変更は最小の範囲だけ置き換え、1 回の Undo で元に戻せるようにする。
 */
export async function applyEditCommand(
  document: vscode.TextDocument,
  command: EditCommand,
): Promise<ApplyResult> {
  const current = document.getText();
  const parsed = parseDocument(current);
  if (!parsed.document) {
    return {
      ok: false,
      error: vscode.l10n.t(
        'The file cannot be edited in the designer because its content is invalid',
      ),
    };
  }

  const result = applyCommand(parsed.document, command);
  if (!result.ok) return result;

  const eol = document.eol === vscode.EndOfLine.CRLF ? '\r\n' : '\n';
  const next = serializeDocument(result.document).replace(/\n/g, eol);
  const change = minimalTextEdit(current, next);
  if (!change) return { ok: true };

  const edit = new vscode.WorkspaceEdit();
  edit.replace(
    document.uri,
    new vscode.Range(document.positionAt(change.start), document.positionAt(change.end)),
    change.text,
  );
  const applied = await vscode.workspace.applyEdit(edit);
  return applied
    ? { ok: true }
    : { ok: false, error: vscode.l10n.t('Failed to apply the change to the file') };
}
