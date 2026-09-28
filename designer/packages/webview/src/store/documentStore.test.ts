import {
  applyCommand,
  findNode,
  serializeDocument,
  type EditCommand,
  type BfmDocument,
  type WebviewToExtensionMessage,
} from '@bethany-designer/core';
import { beforeEach, describe, expect, it } from 'vitest';
import { createDocumentStore, type DocumentStore } from './documentStore.ts';

const DOC: BfmDocument = {
  formatVersion: 1,
  form: { name: 'MainForm', class: 'TForm' },
};
const TEXT = serializeDocument(DOC);

let sent: WebviewToExtensionMessage[];
let store: DocumentStore;

beforeEach(() => {
  sent = [];
  store = createDocumentStore((message) => sent.push(message));
  store.getState().receiveDocument(1, TEXT, 'MainForm.bfm.json');
});

function addButton(name = 'Button1'): EditCommand {
  const command: EditCommand = {
    type: 'addControl',
    parent: 'MainForm',
    className: 'TButton',
    name,
  };
  store.getState().dispatch(command);
  return command;
}

function has(name: string): boolean {
  const { document } = store.getState();
  return document !== undefined && findNode(document, name) !== undefined;
}

describe('documentStore', () => {
  it('ホストから受け取った内容を読み込む', () => {
    expect(store.getState()).toMatchObject({ status: 'loaded', confirmed: { version: 1 } });
    expect(store.getState().document?.form.name).toBe('MainForm');
  });

  it('編集はローカルに即座に反映し、コマンドをホストへ送る', () => {
    addButton();
    expect(has('Button1')).toBe(true);
    expect(sent).toHaveLength(1);
    expect(sent[0]).toMatchObject({ type: 'edit', requestId: 1, command: { type: 'addControl' } });
    expect(store.getState().pending).toEqual([1]);
  });

  it('応答待ちの間は、途中の版が届いてもローカルの表示を保つ', () => {
    const first = addButton('Button1');
    addButton('Button2');
    // 1 つ目の編集だけが反映された版が届く
    const result = applyCommand(DOC, first);
    if (!result.ok) throw new Error(result.error);
    store.getState().receiveDocument(2, serializeDocument(result.document), 'MainForm.bfm.json');
    store.getState().receiveEditResult(1, true);

    expect(has('Button2')).toBe(true);
    expect(store.getState().confirmed.version).toBe(2);
  });

  it('すべての応答が返ったら、ホストの内容に揃える', () => {
    addButton();
    store.getState().receiveEditResult(1, true);
    // ホストから新しい版が届いていなければ、最後に確定した内容に戻る
    expect(has('Button1')).toBe(false);
    expect(store.getState().pending).toEqual([]);
  });

  it('ホストが拒否した編集は取り消され、理由を表示する', () => {
    addButton();
    store.getState().receiveEditResult(1, false, 'だめでした');
    expect(has('Button1')).toBe(false);
    expect(store.getState().lastError).toBe('だめでした');
  });

  it('ローカルで適用できないコマンドは送らない', () => {
    expect(store.getState().dispatch({ type: 'removeNodes', names: ['MainForm'] })).toBe(false);
    expect(sent).toEqual([]);
    expect(store.getState().lastError).toContain('form');
  });
});
