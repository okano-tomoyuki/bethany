/**
 * コード生成(tk-designer ADR 0010。docs/designer/editor-design.md §7)。コマンド「コードを生成」と、デザイナーの画面から呼ばれる。
 * 生成する言語・クラス名・出力先は DSL の codegen に書く。
 */
import { generateAll, resolveTargets, type OutputFile } from '@no-vcl-designer/codegen';
import { hasErrors, parseDocument } from '@no-vcl-designer/core';
import * as vscode from 'vscode';

const output = { channel: undefined as vscode.OutputChannel | undefined };

export async function generateCode(document: vscode.TextDocument): Promise<void> {
  const parsed = parseDocument(document.getText());
  if (!parsed.document || hasErrors(parsed.diagnostics)) {
    void vscode.window.showErrorMessage(
      vscode.l10n.t(
        'Cannot generate code because the form has validation errors. See the Problems panel.',
      ),
    );
    return;
  }

  const fileName = document.uri.path.split('/').pop() ?? 'Form.nvform.json';
  const directory = vscode.Uri.joinPath(document.uri, '..');
  // 出力先の既存の内容を先に読んでおく(生成は同期的に行う)
  const targets = resolveTargets(parsed.document, fileName);
  const paths = [targets.cpp?.header, targets.cpp?.source, targets.python?.file].filter(
    (p): p is string => p !== undefined,
  );
  const existing = new Map<string, string | undefined>();
  for (const path of paths)
    existing.set(path, await readText(vscode.Uri.joinPath(directory, path)));

  const generated = generateAll(parsed.document, fileName, (path) => existing.get(path));
  if ('error' in generated) {
    void vscode.window.showErrorMessage(generated.error);
    return;
  }
  const failed = generated.files.find((f) => !f.result.ok);
  if (failed && !failed.result.ok) {
    void vscode.window.showErrorMessage(`${failed.path}: ${failed.result.error}`);
    return;
  }
  if (generated.warnings.length > 0) {
    const channel = (output.channel ??= vscode.window.createOutputChannel('no_vcl Designer'));
    for (const warning of generated.warnings) channel.appendLine(warning);
    const show = vscode.l10n.t('Show Details');
    void vscode.window
      .showWarningMessage(
        vscode.l10n.t(
          'Names removed from the form are still used in the generated files. See the output for details.',
        ),
        show,
      )
      .then((answer) => {
        if (answer === show) channel.show();
      });
  }

  const files = generated.files.filter(
    (f): f is OutputFile & { result: { ok: true } } => f.result.ok,
  );
  const modified = files.filter((f) => f.result.modifiedRegions.length > 0);
  if (modified.length > 0) {
    const overwrite = vscode.l10n.t('Overwrite');
    const list = modified
      .map((f) => `${f.path} (${f.result.modifiedRegions.join(', ')})`)
      .join(', ');
    const answer = await vscode.window.showWarningMessage(
      vscode.l10n.t('Generated regions have been edited by hand: {0}. Overwrite them?', list),
      { modal: true },
      overwrite,
    );
    if (answer !== overwrite) return;
  }

  const changed = files.filter((f) => existing.get(f.path) !== f.result.text);
  if (changed.length === 0) {
    void vscode.window.showInformationMessage(
      vscode.l10n.t('{0} is up to date.', files.map((f) => f.path).join(', ')),
    );
    return;
  }
  for (const file of changed) {
    await writeText(
      vscode.Uri.joinPath(directory, file.path),
      existing.get(file.path),
      file.result.text,
    );
  }

  const stubs = [...new Set(changed.flatMap((f) => f.result.addedStubs))];
  const details =
    stubs.length > 0 ? vscode.l10n.t(' (added handler stubs: {0})', stubs.join(', ')) : '';
  const summary = changed
    .map((f) =>
      existing.get(f.path) === undefined
        ? vscode.l10n.t('{0} (created)', f.path)
        : vscode.l10n.t('{0} (updated)', f.path),
    )
    .join(', ');
  const open = vscode.l10n.t('Open');
  const answer = await vscode.window.showInformationMessage(
    vscode.l10n.t('Generated: {0}{1}', summary, details),
    open,
  );
  if (answer === open) {
    for (const file of changed) {
      await vscode.window.showTextDocument(vscode.Uri.joinPath(directory, file.path), {
        preview: false,
      });
    }
  }
}

/** 開いている(未保存の変更を含む)内容、なければファイルの内容。ファイルがなければ undefined */
async function readText(uri: vscode.Uri): Promise<string | undefined> {
  const opened = vscode.workspace.textDocuments.find((d) => d.uri.toString() === uri.toString());
  if (opened) return opened.getText();
  try {
    return new TextDecoder().decode(await vscode.workspace.fs.readFile(uri));
  } catch {
    return undefined;
  }
}

/**
 * WorkspaceEdit で書き込む。開いているエディタにも反映され、Undo もできる。
 * 書き込み前に未保存の変更がなかったファイルは、書き込み後に保存する。
 */
async function writeText(
  uri: vscode.Uri,
  existing: string | undefined,
  text: string,
): Promise<void> {
  const edit = new vscode.WorkspaceEdit();
  if (existing === undefined) {
    await vscode.workspace.fs.createDirectory(vscode.Uri.joinPath(uri, '..'));
    edit.createFile(uri, { contents: new TextEncoder().encode(text) });
    await vscode.workspace.applyEdit(edit);
    return;
  }
  const document = await vscode.workspace.openTextDocument(uri);
  const wasDirty = document.isDirty;
  // 改行コードは既存のファイルに合わせる
  const eol = document.eol === vscode.EndOfLine.CRLF ? '\r\n' : '\n';
  edit.replace(
    uri,
    new vscode.Range(document.positionAt(0), document.positionAt(document.getText().length)),
    text.replace(/\r?\n/g, eol),
  );
  await vscode.workspace.applyEdit(edit);
  if (!wasDirty) await document.save();
}
