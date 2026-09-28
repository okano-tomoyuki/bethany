/**
 * 開いている *.nvform.json の診断を問題パネルに出す(docs/designer/editor-design.md §8)。
 * デザイナーで開いているか、テキストエディタで開いているかによらない。
 */
import { locate, parseDocument, type Diagnostic } from '@no-vcl-designer/core';
import * as vscode from 'vscode';

const SOURCE = 'no_vcl Designer';

export function registerDiagnostics(): vscode.Disposable {
  const collection = vscode.languages.createDiagnosticCollection('noVclDesigner');
  const update = (document: vscode.TextDocument) => {
    if (!isDsl(document.uri)) return;
    const text = document.getText();
    const { diagnostics } = parseDocument(text);
    collection.set(
      document.uri,
      diagnostics.map((d) => toVscode(document, text, d)),
    );
  };

  for (const document of vscode.workspace.textDocuments) update(document);
  return vscode.Disposable.from(
    collection,
    vscode.workspace.onDidOpenTextDocument(update),
    vscode.workspace.onDidChangeTextDocument((e) => {
      update(e.document);
    }),
    vscode.workspace.onDidCloseTextDocument((document) => {
      collection.delete(document.uri);
    }),
  );
}

function isDsl(uri: vscode.Uri): boolean {
  return uri.path.endsWith('.nvform.json');
}

function toVscode(
  document: vscode.TextDocument,
  text: string,
  diagnostic: Diagnostic,
): vscode.Diagnostic {
  const { start, end } = locate(text, diagnostic.path);
  const result = new vscode.Diagnostic(
    new vscode.Range(document.positionAt(start), document.positionAt(end)),
    diagnostic.message,
    diagnostic.severity === 'error'
      ? vscode.DiagnosticSeverity.Error
      : vscode.DiagnosticSeverity.Warning,
  );
  result.source = SOURCE;
  result.code = diagnostic.code;
  return result;
}
