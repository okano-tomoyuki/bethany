/**
 * プロジェクトの設定画面の状態。拡張から届いた TextDocument の内容を正とし、画面の編集は変更後のプロジェクト全体を送る
 * (拡張が決まった形で書き出して当てる)。送った変更は、届くまで画面に先に反映しておく。
 */
import {
  parseProject,
  serializeProject,
  type BfprojDocument,
  type Diagnostic,
} from '@bethany-designer/core';
import { useStore } from 'zustand';
import { createStore } from 'zustand/vanilla';
import { onMessage, postMessage } from '../vscode.ts';

export interface ProjectState {
  readonly status: 'loading' | 'ready';
  readonly fileName: string;
  /** 構造の検証を通らなければ undefined(画面は出さず、テキストで直すよう案内する) */
  readonly project: BfprojDocument | undefined;
  readonly diagnostics: readonly Diagnostic[];
  /** 拡張が書き込めなかったときのエラー */
  readonly error: string | undefined;
}

export const projectStore = createStore<ProjectState>()(() => ({
  status: 'loading',
  fileName: '',
  project: undefined,
  diagnostics: [],
  error: undefined,
}));

export function useProjectStore<T>(selector: (state: ProjectState) => T): T {
  return useStore(projectStore, selector);
}

let nextRequestId = 1;
/** 最後に届いた TextDocument の内容(書き込めなかったときに戻す) */
let lastText = '';

/** 画面での変更を送る */
export function commitProject(project: BfprojDocument): void {
  const current = projectStore.getState().project;
  if (current && serializeProject(current) === serializeProject(project)) return;
  // 意味の検証の結果も、書き出した後の内容で先に出しておく
  const { diagnostics } = parseProject(serializeProject(project));
  projectStore.setState({ project, diagnostics, error: undefined });
  postMessage({ type: 'editProject', requestId: nextRequestId++, project });
}

/** 拡張からのメッセージをストアにつなぐ。起動時に 1 回だけ呼ぶ */
export function connectProjectToHost(): void {
  onMessage((message) => {
    switch (message.type) {
      case 'document': {
        lastText = message.text;
        const { project, diagnostics } = parseProject(message.text);
        projectStore.setState({
          status: 'ready',
          fileName: message.fileName,
          project,
          diagnostics,
        });
        break;
      }
      case 'editResult':
        // 書き込めなければ、先に反映した変更を捨てて TextDocument の内容に戻す
        if (!message.ok) projectStore.setState({ ...parseProject(lastText), error: message.error });
        break;
      case 'settings':
        break;
    }
  });
  postMessage({ type: 'ready' });
}
