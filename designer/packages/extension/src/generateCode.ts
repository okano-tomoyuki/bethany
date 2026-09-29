/**
 * コード生成(tk-designer ADR 0010。docs/designer/editor-design.md §7)。コマンド「コードを生成」と、デザイナーの画面から呼ばれる。
 * 生成する言語とコメントの言語は、フォームが属するプロジェクトの codegen から決める(どのプロジェクトにも属さないフォームは生成しない)。
 * プロジェクトファイル(*.bfproj.json)なら起動部分を生成する(docs/designer/project-spec.md §5)。
 */
import {
  formCodegenSettings,
  generateAll,
  generateProject,
  resolveProjectTargets,
  resolveTargets,
  type FormSource,
  type GenerateAllResult,
  type OutputFile,
  type ResolvedTargets,
} from '@bethany-designer/codegen';
import {
  autoCreateForms,
  dirname,
  hasErrors,
  parseDocument,
  parseProject,
  PROJECT_EXTENSION,
  resolvePath,
} from '@bethany-designer/core';
import * as vscode from 'vscode';
import { findProjects, samePath, type ProjectInfo } from './projects.ts';

const output = { channel: undefined as vscode.OutputChannel | undefined };

export async function generateCode(document: vscode.TextDocument): Promise<void> {
  if (document.uri.path.endsWith(PROJECT_EXTENSION)) {
    await generateProjectCode(document, { quiet: false });
    return;
  }
  await generateFormCode(document, { quiet: false });
}

/**
 * プロジェクト全体のコード(forms のすべてのフォームと起動部分)を生成する(project-spec.md §5)。
 * 個々の成功の通知は出さず、最後にまとめて知らせる。生成できなかったフォームは飛ばして続ける。
 */
export async function generateWholeProject(document: vscode.TextDocument): Promise<void> {
  const { project, diagnostics } = parseProject(document.getText());
  if (!project || hasErrors(diagnostics)) {
    void vscode.window.showErrorMessage(
      vscode.l10n.t(
        'Cannot generate code because the project has validation errors. See the Problems panel.',
      ),
    );
    return;
  }
  const failed: string[] = [];
  let generated = 0;
  for (const path of project.forms ?? []) {
    const uri = document.uri.with({ path: resolvePath(dirname(document.uri.path), path) });
    const text = await readText(uri);
    const parsed = text === undefined ? undefined : parseDocument(text);
    if (!parsed?.document || hasErrors(parsed.diagnostics)) {
      failed.push(
        text === undefined
          ? vscode.l10n.t('"{0}" not found', path)
          : vscode.l10n.t('{0} has validation errors', path),
      );
      continue;
    }
    if (await generateFormCode(await vscode.workspace.openTextDocument(uri), { quiet: true }))
      generated++;
    else failed.push(path);
  }
  const startup = await generateProjectCode(document, { quiet: true });
  const name = (document.uri.path.split('/').pop() ?? '').replace(PROJECT_EXTENSION, '');
  if (failed.length === 0 && startup) {
    void vscode.window.showInformationMessage(
      vscode.l10n.t(
        'Generated the code of {0} forms and the startup code of {1}.',
        generated,
        name,
      ),
    );
    return;
  }
  void vscode.window.showWarningMessage(
    vscode.l10n.t(
      'Generated the code of {0} forms of {1}. Not generated: {2}',
      generated,
      name,
      [...failed, ...(startup ? [] : [vscode.l10n.t('the startup code')])].join(', '),
    ),
  );
}

/** フォームのコードを生成した結果。出力先はフォームのフォルダからの相対パス */
export interface FormCodeResult {
  readonly directory: vscode.Uri;
  readonly targets: ResolvedTargets;
}

/**
 * フォームのコードを生成する。生成できなかった・取りやめたときは undefined。
 * quiet なら、成功の通知(「生成しました」「最新です」)を出さない(ハンドラへの移動から呼ぶとき)。
 * 出力先・言語はフォームが属するプロジェクトの設定で決まるので、どのプロジェクトにも属さないフォームは生成しない
 * (プロジェクトへの追加を案内し、追加されたら続ける。project-spec.md §5)。
 */
export async function generateFormCode(
  document: vscode.TextDocument,
  { quiet }: { readonly quiet: boolean },
): Promise<FormCodeResult | undefined> {
  const parsed = parseDocument(document.getText());
  if (!parsed.document || hasErrors(parsed.diagnostics)) {
    void vscode.window.showErrorMessage(
      vscode.l10n.t(
        'Cannot generate code because the form has validation errors. See the Problems panel.',
      ),
    );
    return undefined;
  }

  const fileName = document.uri.path.split('/').pop() ?? 'Form.bfm.json';
  const directory = vscode.Uri.joinPath(document.uri, '..');
  const projects = await projectsContaining(document.uri);
  if (projects.length === 0) return undefined;
  // 各プロジェクトの forms に書かれた、このフォームのパス(headerDir・sourceDir の下の置き場所を決める)
  const settings = formCodegenSettings(
    projects.flatMap((p) => {
      const formPath = p.doc?.forms?.[p.forms.findIndex((form) => samePath(form, document.uri))];
      return p.doc && formPath !== undefined ? [{ doc: p.doc, formPath }] : [];
    }),
  );
  // 出力先の既存の内容を先に読んでおく(生成は同期的に行う)
  const targets = resolveTargets(parsed.document, fileName, settings);
  const existing = await readAll(directory, [
    targets.cpp?.header,
    targets.cpp?.source,
    targets.python?.file,
  ]);
  const written = await writeGenerated(
    directory,
    generateAll(parsed.document, fileName, settings, (path) => existing.get(path)),
    existing,
    quiet,
  );
  return written ? { directory, targets } : undefined;
}

/**
 * フォームを含むプロジェクト。どれにも属さなければ、プロジェクトへの追加を案内する(追加されたらもう一度探す)。
 * 取りやめたら空
 */
async function projectsContaining(uri: vscode.Uri): Promise<ProjectInfo[]> {
  const find = async () =>
    (await findProjects()).filter((p) => p.forms.some((form) => samePath(form, uri)));
  const found = await find();
  if (found.length > 0) return found;
  const add = vscode.l10n.t('Add to Project...');
  const answer = await vscode.window.showWarningMessage(
    vscode.l10n.t(
      '{0} is not in a project. Code is generated with the settings of the project that contains the form (languages, folders and so on).',
      uri.path.split('/').pop() ?? '',
    ),
    add,
  );
  if (answer !== add) return [];
  await vscode.commands.executeCommand('bethanyDesigner.addToProject', { kind: 'form', uri });
  return find();
}

/** プロジェクトの起動部分(Project1.cpp・Project1.py)を生成する。生成できなかった・取りやめたときは false */
async function generateProjectCode(
  document: vscode.TextDocument,
  { quiet }: { readonly quiet: boolean },
): Promise<boolean> {
  const { project, diagnostics } = parseProject(document.getText());
  if (!project || hasErrors(diagnostics)) {
    void vscode.window.showErrorMessage(
      vscode.l10n.t(
        'Cannot generate code because the project has validation errors. See the Problems panel.',
      ),
    );
    return false;
  }
  const fileName = document.uri.path.split('/').pop() ?? `Project${PROJECT_EXTENSION}`;
  const directory = vscode.Uri.joinPath(document.uri, '..');

  // 起動時に作るフォーム(メインフォームが先頭)の名前とファイル名から、include するヘッダ・import するモジュールを決める
  const forms: FormSource[] = [];
  for (const path of autoCreateForms(project)) {
    const uri = document.uri.with({ path: resolvePath(dirname(document.uri.path), path) });
    const text = await readText(uri);
    const parsed = text === undefined ? undefined : parseDocument(text);
    if (!parsed?.document || hasErrors(parsed.diagnostics)) {
      void vscode.window.showErrorMessage(
        text === undefined
          ? vscode.l10n.t('"{0}" not found', path)
          : vscode.l10n.t('Cannot generate code because the form {0} has validation errors.', path),
      );
      return false;
    }
    forms.push({ doc: parsed.document, path });
  }

  const targets = resolveProjectTargets(project, fileName);
  const existing = await readAll(directory, [targets.cpp?.main, targets.python?.main]);
  return writeGenerated(
    directory,
    generateProject(project, fileName, forms, (path) => existing.get(path)),
    existing,
    quiet,
  );
}

/** 出力先の既存の内容(なければ undefined)を読んでおく */
async function readAll(
  directory: vscode.Uri,
  paths: readonly (string | undefined)[],
): Promise<Map<string, string | undefined>> {
  const existing = new Map<string, string | undefined>();
  for (const path of paths)
    if (path !== undefined)
      existing.set(path, await readText(vscode.Uri.joinPath(directory, path)));
  return existing;
}

/**
 * 生成した結果を確かめて書き込む(手で編集された区間があれば上書きしてよいか聞く)。
 * 書き込んだ(変更が無かったときを含む)なら true、エラー・取りやめなら false。
 */
async function writeGenerated(
  directory: vscode.Uri,
  generated: GenerateAllResult | { readonly error: string },
  existing: ReadonlyMap<string, string | undefined>,
  quiet = false,
): Promise<boolean> {
  if ('error' in generated) {
    void vscode.window.showErrorMessage(generated.error);
    return false;
  }
  const failed = generated.files.find((f) => !f.result.ok);
  if (failed && !failed.result.ok) {
    void vscode.window.showErrorMessage(`${failed.path}: ${failed.result.error}`);
    return false;
  }
  if (generated.warnings.length > 0) {
    const channel = (output.channel ??= vscode.window.createOutputChannel('Bethany Designer'));
    for (const warning of generated.warnings) channel.appendLine(warning);
    const show = vscode.l10n.t('Show Details');
    void vscode.window
      .showWarningMessage(
        vscode.l10n.t('Code generation reported warnings. See the output for details.'),
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
    if (answer !== overwrite) return false;
  }

  const changed = files.filter((f) => existing.get(f.path) !== f.result.text);
  if (changed.length === 0) {
    if (!quiet)
      void vscode.window.showInformationMessage(
        vscode.l10n.t('{0} is up to date.', files.map((f) => f.path).join(', ')),
      );
    return true;
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
  if (quiet) {
    // 移動した先のエディタで結果は見えるので、ステータスバーに短く出すだけにする
    vscode.window.setStatusBarMessage(vscode.l10n.t('Generated: {0}{1}', summary, details), 5000);
    return true;
  }
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
  return true;
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
