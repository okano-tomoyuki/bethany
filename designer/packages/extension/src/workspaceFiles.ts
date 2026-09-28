import * as vscode from 'vscode';
import { excludeGlob } from './excludeGlob.ts';

/** ワークスペースのファイルを探す。検索の除外(search.exclude。既定で node_modules 等)と files.exclude に従う */
export function findWorkspaceFiles(pattern: string): Thenable<vscode.Uri[]> {
  const exclude = excludeGlob(
    vscode.workspace.getConfiguration('search').get<Record<string, unknown>>('exclude'),
    vscode.workspace.getConfiguration('files').get<Record<string, unknown>>('exclude'),
  );
  return vscode.workspace.findFiles(pattern, exclude);
}

/** ワークスペースからの相対パス(フォルダが複数あれば、先頭にフォルダ名を付ける) */
export function workspaceRelativePath(uri: vscode.Uri): string {
  const multiRoot = (vscode.workspace.workspaceFolders?.length ?? 0) > 1;
  return vscode.workspace.asRelativePath(uri, multiRoot).replace(/\\/g, '/');
}

/** ファイル名(`/` の後ろ) */
export function baseName(uri: vscode.Uri): string {
  return uri.path.slice(uri.path.lastIndexOf('/') + 1);
}

export async function exists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

/** 開いていればエディタの内容(保存前の変更を含む)、開いていなければファイルの内容 */
export async function readText(uri: vscode.Uri): Promise<string> {
  const open = vscode.workspace.textDocuments.find((d) => d.uri.toString() === uri.toString());
  if (open) return open.getText();
  return new TextDecoder().decode(await vscode.workspace.fs.readFile(uri));
}
