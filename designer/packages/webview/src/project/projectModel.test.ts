import type { BfprojDocument } from '@bethany-designer/core';
import { describe, expect, it } from 'vitest';
import {
  addOverride,
  moveAutoCreate,
  moveOverride,
  parsePatterns,
  removeOverride,
  setCommentLocale,
  setCppField,
  setGenerateOnSave,
  setLanguage,
  setOverrideField,
  setOverrideForms,
  setPythonField,
  startupLists,
} from './projectModel.ts';

const PROJECT: BfprojDocument = {
  formatVersion: 1,
  codegen: { cpp: { namespace: 'app' }, python: {} },
  mainForm: 'Main.bfm.json',
  forms: ['Main.bfm.json', 'A.bfm.json', 'B.bfm.json', 'C.bfm.json'],
  autoCreate: ['Main.bfm.json', 'A.bfm.json', 'B.bfm.json'],
};

describe('プロジェクトの設定画面の操作', () => {
  it('言語の切り替え(外すとキーごと消し、付け直すと空の設定)', () => {
    const off = setLanguage(PROJECT, 'cpp', false);
    expect(off.codegen).toEqual({ python: {} });
    expect(setLanguage(off, 'cpp', true).codegen?.cpp).toEqual({});
    // 付いているものはそのまま
    expect(setLanguage(PROJECT, 'cpp', true).codegen?.cpp).toEqual({ namespace: 'app' });
  });

  it('保存したときの生成(既定の有効なら書かず、無効なら false を書く)', () => {
    const off = setGenerateOnSave(PROJECT, false);
    expect(off.codegen?.generateOnSave).toBe(false);
    expect(setGenerateOnSave(off, true).codegen).toEqual(PROJECT.codegen);
  });

  it('空の値は書かない(既定値)', () => {
    expect(setCppField(PROJECT, 'namespace', undefined).codegen?.cpp).toEqual({});
    expect(setCppField(PROJECT, 'headerDir', 'include').codegen?.cpp).toEqual({
      namespace: 'app',
      headerDir: 'include',
    });
    expect(setPythonField(PROJECT, 'moduleDir', 'py').codegen?.python).toEqual({ moduleDir: 'py' });
    expect(setCommentLocale(PROJECT, 'ja').codegen?.commentLocale).toBe('ja');
    expect(setCommentLocale(setCommentLocale(PROJECT, 'ja'), undefined).codegen).not.toHaveProperty(
      'commentLocale',
    );
  });

  it('上書きの追加・変更・並べ替え・削除(最後の 1 つを消すと overrides ごと消す)', () => {
    let doc = addOverride(PROJECT, 'dialogs');
    doc = addOverride(doc, 'A.bfm.json');
    doc = setOverrideField(doc, 0, 'namespace', 'app::dialogs');
    doc = setOverrideForms(doc, 1, ['A.bfm.json', 'B.bfm.json']);
    expect(doc.codegen?.cpp?.overrides).toEqual([
      { forms: ['dialogs'], namespace: 'app::dialogs' },
      { forms: ['A.bfm.json', 'B.bfm.json'] },
    ]);
    doc = moveOverride(doc, 1, -1);
    expect(doc.codegen?.cpp?.overrides?.[0]?.forms).toEqual(['A.bfm.json', 'B.bfm.json']);
    doc = setOverrideField(doc, 1, 'namespace', undefined);
    expect(doc.codegen?.cpp?.overrides?.[1]).toEqual({ forms: ['dialogs'] });
    doc = removeOverride(removeOverride(doc, 0), 0);
    expect(doc.codegen?.cpp).toEqual({ namespace: 'app' });
  });

  it('起動時に作るフォームの一覧と並べ替え(メインフォームは含めない)', () => {
    expect(startupLists(PROJECT)).toEqual({
      auto: ['A.bfm.json', 'B.bfm.json'],
      available: ['C.bfm.json'],
    });
    const moved = moveAutoCreate(PROJECT, 'B.bfm.json', -1);
    expect(moved.autoCreate).toEqual(['B.bfm.json', 'A.bfm.json']);
    expect(moveAutoCreate(PROJECT, 'A.bfm.json', -1).autoCreate).toEqual([
      'A.bfm.json',
      'B.bfm.json',
    ]);
    // autoCreate が無ければ forms のすべて
    const all = { ...PROJECT, autoCreate: undefined };
    expect(startupLists(all).auto).toEqual(['A.bfm.json', 'B.bfm.json', 'C.bfm.json']);
  });

  it('パターンの入力はカンマか改行で区切る', () => {
    expect(parsePatterns(' dialogs, **/*.bfm.json\nA.bfm.json ,, ')).toEqual([
      'dialogs',
      '**/*.bfm.json',
      'A.bfm.json',
    ]);
  });
});
