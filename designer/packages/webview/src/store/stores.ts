/**
 * アプリ全体で使うストアのインスタンスと、React から購読するためのフック。
 */
import { findNode, type NodeLocation } from '@bethany-designer/core';
import { useStore } from 'zustand';
import { useShallow } from 'zustand/shallow';
import { postMessage } from '../vscode.ts';
import { createDocumentStore, type DocumentActions, type DocumentState } from './documentStore.ts';
import { createUiStore, type UiActions, type UiState } from './uiStore.ts';

export const documentStore = createDocumentStore(postMessage);
export const uiStore = createUiStore();

/** セレクタで必要な部分だけを購読する(該当部分が変わったときだけ再描画される) */
export function useDocumentStore<T>(selector: (state: DocumentState & DocumentActions) => T): T {
  return useStore(documentStore, selector);
}

export function useUiStore<T>(selector: (state: UiState & UiActions) => T): T {
  return useStore(uiStore, selector);
}

/** 選択中のノードのうち、ドキュメントに存在するもの(選択の順) */
export function useSelectedNodes(): readonly NodeLocation[] {
  const selection = useUiStore(useShallow((s) => s.selection));
  const document = useDocumentStore((s) => s.document);
  if (!document) return [];
  return selection.flatMap((name) => {
    const found = findNode(document, name);
    return found ? [found] : [];
  });
}
