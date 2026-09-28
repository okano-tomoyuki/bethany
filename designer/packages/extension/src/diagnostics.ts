/**
 * 開いている *.bfm.json・*.bfproj.json の診断を問題パネルに出す(docs/designer/editor-design.md §8・project-spec.md §3)。
 * デザイナーで開いているか、テキストエディタで開いているかによらない。
 */
import {
  FORM_EXTENSION,
  locate,
  parseDocument,
  parseProject,
  PROJECT_EXTENSION,
  type Diagnostic,
} from '@bethany-designer/core';
import * as vscode from 'vscode';
import { formUri } from './projects.ts';
import { exists } from './workspaceFiles.ts';

const SOURCE = 'Bethany Designer';

export function registerDiagnostics(): vscode.Disposable {
  const collection = vscode.languages.createDiagnosticCollection('bethanyDesigner');
  const update = async (document: vscode.TextDocument): Promise<void> => {
    const text = document.getText();
    if (document.uri.path.endsWith(FORM_EXTENSION)) {
      set(document, text, parseDocument(text).diagnostics);
    } else if (document.uri.path.endsWith(PROJECT_EXTENSION)) {
      const version = document.version;
      const diagnostics = await projectDiagnostics(document.uri, text);
      // 確かめている間に編集されていれば、古い結果は捨てる
      if (document.version === version && !document.isClosed) set(document, text, diagnostics);
    }
  };
  const set = (document: vscode.TextDocument, text: string, diagnostics: readonly Diagnostic[]) => {
    collection.set(
      document.uri,
      diagnostics.map((d) => toVscode(document, text, d)),
    );
  };
  const updateProjects = (): void => {
    for (const document of vscode.workspace.textDocuments)
      if (document.uri.path.endsWith(PROJECT_EXTENSION)) void update(document);
  };

  for (const document of vscode.workspace.textDocuments) void update(document);
  // フォームのファイルができた・消えたら、開いているプロジェクトファイルの「ファイルがありません」を更新する
  const formWatcher = vscode.workspace.createFileSystemWatcher(
    `**/*${FORM_EXTENSION}`,
    false,
    true,
    false,
  );
  return vscode.Disposable.from(
    collection,
    formWatcher,
    formWatcher.onDidCreate(updateProjects),
    formWatcher.onDidDelete(updateProjects),
    vscode.workspace.onDidOpenTextDocument((document) => void update(document)),
    vscode.workspace.onDidChangeTextDocument((e) => void update(e.document)),
    vscode.workspace.onDidCloseTextDocument((document) => {
      collection.delete(document.uri);
    }),
  );
}

/** 構造と意味の検証に加えて、forms のファイルがあるかを調べる */
async function projectDiagnostics(uri: vscode.Uri, text: string): Promise<Diagnostic[]> {
  const { project, diagnostics } = parseProject(text);
  const result = [...diagnostics];
  const forms = project?.forms ?? [];
  const found = await Promise.all(forms.map((form) => exists(formUri(uri, form))));
  forms.forEach((form, index) => {
    if (found[index] || result.some((d) => d.path[0] === 'forms' && d.path[1] === index)) return;
    result.push({
      severity: 'warning',
      code: 'missing-form-file',
      message: vscode.l10n.t('"{0}" not found', form),
      path: ['forms', index],
    });
  });
  return result;
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
