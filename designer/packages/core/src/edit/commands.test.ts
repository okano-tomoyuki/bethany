import { describe, expect, it } from 'vitest';
import sampleText from '../../../../samples/MainForm.bfm.json?raw';
import { parseDocument } from '../dsl/parse.ts';
import type { ControlNode, BfmDocument } from '../dsl/schema.ts';
import { walkNodes } from '../dsl/tree.ts';
import { validateDocument } from '../dsl/validate.ts';
import { applyCommand, type EditCommand } from './commands.ts';

const SAMPLE_TEXT = sampleText;

function sample(): BfmDocument {
  const { document } = parseDocument(SAMPLE_TEXT);
  if (!document) throw new Error('sample does not parse');
  return document;
}

/** 適用して、検証のエラーが無いことも確かめる */
function apply(command: EditCommand, doc: BfmDocument = sample()): BfmDocument {
  const result = applyCommand(doc, command);
  if (!result.ok) throw new Error(result.error);
  expect(validateDocument(result.document)).toEqual([]);
  return result.document;
}

function reject(command: EditCommand, doc: BfmDocument = sample()): string {
  const result = applyCommand(doc, command);
  if (result.ok) throw new Error('expected the command to be rejected');
  return result.error;
}

function node(doc: BfmDocument, name: string) {
  const found = [...walkNodes(doc)].find((l) => l.node.name === name);
  if (!found) throw new Error(`${name} not found`);
  return found;
}

function names(controls: readonly ControlNode[] | undefined): string[] {
  return (controls ?? []).map((c) => c.name);
}

describe('追加', () => {
  it('addControl: 初期値(位置・大きさ・Caption)', () => {
    const doc = apply({
      type: 'addControl',
      parent: 'BottomPanel',
      className: 'TButton',
      name: 'Button1',
    });
    expect(node(doc, 'Button1').node).toEqual({
      name: 'Button1',
      class: 'TButton',
      properties: { Left: 8, Top: 8, Width: 75, Height: 25, Caption: 'Button1' },
    });
  });

  it('addControl: 範囲と位置を指定する', () => {
    const doc = apply({
      type: 'addControl',
      parent: 'MainForm',
      className: 'TEdit',
      name: 'Edit1',
      bounds: { left: 1, top: 2, width: 3, height: 4 },
      index: 0,
    });
    expect(names(doc.form.controls)[0]).toBe('Edit1');
    expect(node(doc, 'Edit1').node.properties).toEqual({ Left: 1, Top: 2, Width: 3, Height: 4 });
  });

  it('addControl: TTabSheet は位置と大きさを書かない', () => {
    const doc = apply({
      type: 'addControl',
      parent: 'PageControl1',
      className: 'TTabSheet',
      name: 'TabSheet1',
    });
    expect(node(doc, 'TabSheet1').node.properties).toEqual({ Caption: 'TabSheet1' });
  });

  it('addControl: 親子の制約・名前の重複・コントロールでないクラスは拒む', () => {
    const add = (parent: string, className: string, name = 'X1'): EditCommand => ({
      type: 'addControl',
      parent,
      className,
      name,
    });
    expect(reject(add('PageControl1', 'TButton'))).toContain('TTabSheet');
    expect(reject(add('MainForm', 'TTabSheet'))).toContain('TPageControl');
    expect(reject(add('OkButton', 'TButton'))).toContain('cannot have controls');
    expect(reject(add('MainForm', 'TTimer'))).toContain('not a control');
    expect(reject(add('MainForm', 'TButton', 'OkButton'))).toContain('already used');
    expect(reject(add('MainForm', 'TButton', 'FormCreate'))).toContain('handler name');
    expect(reject(add('MainForm', 'TButton', 'Caption'))).toContain('member of TForm');
  });

  it('addComponent: メニューには items を作る', () => {
    const doc = apply({
      type: 'addComponent',
      className: 'TPopupMenu',
      name: 'PopupMenu2',
      design: { left: 1, top: 2 },
    });
    expect(node(doc, 'PopupMenu2').node).toEqual({
      name: 'PopupMenu2',
      class: 'TPopupMenu',
      design: { left: 1, top: 2 },
      items: [],
    });
    expect(reject({ type: 'addComponent', className: 'TButton', name: 'B1' })).toContain(
      'non-visual',
    );
  });

  it('addComponent: 最初の TMainMenu はフォームのメニューにする', () => {
    let doc = apply({ type: 'removeNodes', names: ['MainMenu1'] });
    expect(doc.form.properties).not.toHaveProperty('Menu');
    doc = apply({ type: 'addComponent', className: 'TMainMenu', name: 'MainMenu2' }, doc);
    expect(doc.form.properties?.Menu).toBe('MainMenu2');
    doc = apply({ type: 'addComponent', className: 'TMainMenu', name: 'MainMenu3' }, doc);
    expect(doc.form.properties?.Menu).toBe('MainMenu2');
  });

  it('addMenuItem: メニューと項目の下に追加する', () => {
    let doc = apply({
      type: 'addMenuItem',
      parent: 'FileMenu',
      name: 'Save1',
      caption: '&Save',
      index: 1,
    });
    doc = apply(
      { type: 'addMenuItem', parent: 'Save1', name: 'SaveAs1', caption: 'Save &As' },
      doc,
    );
    const file = node(doc, 'FileMenu').node as { items?: { name: string }[] };
    expect(file.items?.map((i) => i.name)).toEqual(['FileOpenItem', 'Save1', 'N1', 'FileExitItem']);
    expect(node(doc, 'SaveAs1').kind).toBe('menuItem');
    expect(reject({ type: 'addMenuItem', parent: 'Timer1', name: 'M1', caption: 'x' })).toContain(
      'cannot have menu items',
    );
  });
});

describe('削除', () => {
  it('子孫ごと削除し、削除したものへの参照も消す', () => {
    const doc = apply({ type: 'removeNodes', names: ['PageControl1', 'PopupMenu1'] });
    const all = [...walkNodes(doc)].map((l) => l.node.name);
    expect(all).not.toContain('MemoSheet');
    expect(all).not.toContain('Memo1');
    expect(all).not.toContain('ClearItem');
  });

  it('参照だけのプロパティが無くなれば properties ごと消す', () => {
    let doc = apply({ type: 'removeNodes', names: ['MemoSheet'] });
    expect(node(doc, 'PageControl1').node.properties?.ActivePage).toBeUndefined();
    doc = apply({
      type: 'setProperties',
      changes: [{ node: 'MainForm', path: ['Caption'], value: undefined }],
    });
    expect(Object.keys(doc.form.properties ?? {})).toEqual(['Width', 'Height', 'Menu']);
  });

  it('親と子を同時に指定してもよい。フォームは削除できない', () => {
    const doc = apply({ type: 'removeNodes', names: ['BottomPanel', 'StatusLabel'] });
    expect(names(doc.form.controls)).not.toContain('BottomPanel');
    expect(reject({ type: 'removeNodes', names: ['MainForm'] })).toContain('form');
    expect(reject({ type: 'removeNodes', names: ['Nothing'] })).toContain('not found');
  });

  it('最後の子を削除すると controls を消す(メニューの items は残す)', () => {
    let doc = apply({ type: 'removeNodes', names: ['StatusLabel'] });
    expect(node(doc, 'BottomPanel').node).not.toHaveProperty('controls');
    doc = apply({ type: 'removeNodes', names: ['ClearItem'] }, doc);
    expect(node(doc, 'PopupMenu1').node).toHaveProperty('items', []);
  });
});

describe('移動', () => {
  it('moveControls: 別の親へ、並びを保って移す', () => {
    const doc = apply({
      type: 'moveControls',
      names: ['OkButton', 'NameEdit'],
      parent: 'BottomPanel',
      index: 0,
    });
    expect(names((node(doc, 'BottomPanel').node as ControlNode).controls)).toEqual([
      'OkButton',
      'NameEdit',
      'StatusLabel',
    ]);
    expect(names(doc.form.controls)).toEqual(['PageControl1', 'BottomPanel']);
  });

  it('moveControls: 同じ親の中での並べ替え(index は取り除いた後の位置)', () => {
    const doc = apply({ type: 'moveControls', names: ['NameEdit'], parent: 'MainForm', index: 2 });
    expect(names(doc.form.controls)).toEqual([
      'OkButton',
      'PageControl1',
      'NameEdit',
      'BottomPanel',
    ]);
  });

  it('moveControls: 自分の子孫の中・制約に合わない親へは移せない', () => {
    expect(reject({ type: 'moveControls', names: ['OkButton'], parent: 'StatusLabel' })).toContain(
      'cannot have controls',
    );
    expect(reject({ type: 'moveControls', names: ['PageControl1'], parent: 'Memo1' })).toContain(
      'descendants',
    );
    expect(reject({ type: 'moveControls', names: ['MemoSheet'], parent: 'MainForm' })).toContain(
      'TPageControl',
    );
  });

  it('moveMenuItem: 別のメニューへ移す', () => {
    const doc = apply({
      type: 'moveMenuItem',
      name: 'FileExitItem',
      parent: 'PopupMenu1',
      index: 0,
    });
    const popup = node(doc, 'PopupMenu1').node as { items?: { name: string }[] };
    expect(popup.items?.map((i) => i.name)).toEqual(['FileExitItem', 'ClearItem']);
    expect(reject({ type: 'moveMenuItem', name: 'FileMenu', parent: 'FileOpenItem' })).toContain(
      'descendants',
    );
  });
});

describe('改名', () => {
  it('参照も書き換える', () => {
    let doc = apply({ type: 'renameNode', name: 'MainMenu1', newName: 'AppMenu' });
    expect(doc.form.properties?.Menu).toBe('AppMenu');
    doc = apply({ type: 'renameNode', name: 'MemoSheet', newName: 'TextSheet' }, doc);
    expect(node(doc, 'PageControl1').node.properties?.ActivePage).toBe('TextSheet');
  });

  it('使えない名前・使われている名前は拒む', () => {
    expect(reject({ type: 'renameNode', name: 'OkButton', newName: 'NameEdit' })).toContain(
      'already used',
    );
    expect(reject({ type: 'renameNode', name: 'OkButton', newName: 'class' })).toContain('keyword');
  });
});

describe('プロパティ', () => {
  it('設定・削除と、入れ子のオブジェクト', () => {
    let doc = apply({
      type: 'setProperties',
      changes: [
        { node: 'OkButton', path: ['Caption'], value: 'Accept' },
        { node: 'OkButton', path: ['Font', 'Size'], value: 12 },
        { node: 'NameEdit', path: ['Hint'], value: undefined },
      ],
    });
    expect(node(doc, 'OkButton').node.properties?.Font).toEqual({ Style: ['fsBold'], Size: 12 });
    expect(node(doc, 'NameEdit').node.properties).not.toHaveProperty('Hint');

    doc = apply(
      {
        type: 'setProperties',
        changes: [
          { node: 'OkButton', path: ['Font', 'Size'], value: undefined },
          { node: 'OkButton', path: ['Font', 'Style'], value: undefined },
        ],
      },
      doc,
    );
    expect(node(doc, 'OkButton').node.properties).not.toHaveProperty('Font');
  });

  it('メニュー項目・カタログに無いプロパティ', () => {
    const doc = apply({
      type: 'setProperties',
      changes: [{ node: 'FileExitItem', path: ['ShortCut'], value: 'Alt+F4' }],
    });
    expect(node(doc, 'FileExitItem').node.properties?.ShortCut).toBe('Alt+F4');
    expect(
      reject({
        type: 'setProperties',
        changes: [{ node: 'OkButton', path: ['Lines'], value: [] }],
      }),
    ).toContain('no property Lines');
    expect(
      reject({
        type: 'setProperties',
        changes: [{ node: 'OkButton', path: ['Caption', 'X'], value: 1 }],
      }),
    ).toContain('not an object');
  });
});

describe('イベント', () => {
  it('設定・削除', () => {
    let doc = apply({ type: 'setEvent', node: 'Memo1', event: 'OnChange', handler: 'Memo1Change' });
    expect(node(doc, 'Memo1').node.events).toEqual({ OnChange: 'Memo1Change' });
    doc = apply({ type: 'setEvent', node: 'Memo1', event: 'OnChange', handler: undefined }, doc);
    expect(node(doc, 'Memo1').node).not.toHaveProperty('events');
  });

  it('同じ型のハンドラは共有できる。型が違う・コンポーネントの名前は拒む', () => {
    apply({ type: 'setEvent', node: 'Memo1', event: 'OnChange', handler: 'NameEditChange' });
    expect(
      reject({ type: 'setEvent', node: 'MainForm', event: 'OnClose', handler: 'NameEditChange' }),
    ).toContain('different types');
    expect(
      reject({ type: 'setEvent', node: 'Memo1', event: 'OnChange', handler: 'OkButton' }),
    ).toContain('name of a component');
    expect(reject({ type: 'setEvent', node: 'Memo1', event: 'OnNothing', handler: 'X' })).toContain(
      'no event',
    );
  });

  it('自分自身のイベントの型は比べない(TCloseEvent のハンドラを別の TCloseEvent に付け替える)', () => {
    let doc = apply({ type: 'setEvent', node: 'MainForm', event: 'OnClose', handler: 'FormClose' });
    doc = apply(
      { type: 'setEvent', node: 'MainForm', event: 'OnClose', handler: 'FormClose2' },
      doc,
    );
    expect(doc.form.events?.OnClose).toBe('FormClose2');
  });

  it('renameHandler: すべての参照を書き換え、同じ型のハンドラへは統合する', () => {
    let doc = apply({
      type: 'setEvent',
      node: 'Memo1',
      event: 'OnChange',
      handler: 'NameEditChange',
    });
    doc = apply({ type: 'renameHandler', name: 'NameEditChange', newName: 'TextChange' }, doc);
    expect(node(doc, 'NameEdit').node.events?.OnChange).toBe('TextChange');
    expect(node(doc, 'Memo1').node.events?.OnChange).toBe('TextChange');
    doc = apply({ type: 'renameHandler', name: 'TextChange', newName: 'OkButtonClick' }, doc);
    expect(node(doc, 'Memo1').node.events?.OnChange).toBe('OkButtonClick');
    expect(
      reject({ type: 'renameHandler', name: 'FormCloseQuery', newName: 'FormCreate' }),
    ).toContain('different types');
    expect(reject({ type: 'renameHandler', name: 'Nothing', newName: 'X' })).toContain('not found');
  });
});

describe('その他', () => {
  it('setDesignPosition', () => {
    const doc = apply({ type: 'setDesignPosition', name: 'Timer1', left: 10, top: 20 });
    expect(node(doc, 'Timer1').node).toHaveProperty('design', { left: 10, top: 20 });
    expect(reject({ type: 'setDesignPosition', name: 'OkButton', left: 0, top: 0 })).toContain(
      'non-visual',
    );
  });

  it('batch は途中で失敗したら何も変えない', () => {
    const doc = sample();
    const result = applyCommand(doc, {
      type: 'batch',
      commands: [
        { type: 'removeNodes', names: ['OkButton'] },
        { type: 'removeNodes', names: ['Nothing'] },
      ],
    });
    expect(result.ok).toBe(false);
    expect(names(doc.form.controls)).toContain('OkButton');
  });

  it('元のドキュメントは変えない', () => {
    const doc = sample();
    const before = JSON.stringify(doc);
    apply({ type: 'renameNode', name: 'OkButton', newName: 'AcceptButton' }, doc);
    expect(JSON.stringify(doc)).toBe(before);
  });
});

describe('配置の計算し直し', () => {
  const bounds = (doc: BfmDocument, name: string) => {
    const p = node(doc, name).node.properties ?? {};
    return [p.Left, p.Top, p.Width, p.Height];
  };

  it('フォームを広げると Anchors で追従する(verify-cpp の anchors と同じ値)', () => {
    const doc = apply({
      type: 'setProperties',
      changes: [{ node: 'MainForm', path: ['Width'], value: 500 }],
    });
    expect(bounds(doc, 'OkButton')[0]).toBe(404);
    expect(bounds(doc, 'NameEdit')[2]).toBe(380);
    expect(bounds(doc, 'BottomPanel')[2]).toBe(500);
    // TTabSheet の中の alClient も、TPageControl の広がりに合わせる
    expect(bounds(doc, 'Memo1')).toEqual([0, 0, 455, 92]);
  });

  it('Align を設定すると寄せる', () => {
    const doc = apply({
      type: 'setProperties',
      changes: [{ node: 'NameEdit', path: ['Align'], value: 'alTop' }],
    });
    expect(bounds(doc, 'NameEdit')).toEqual([0, 0, 400, 23]);
  });

  it('TStatusBar は追加しただけで下に寄せる(高さは親に置いて測った値)', () => {
    const doc = apply({
      type: 'addControl',
      parent: 'MainForm',
      className: 'TStatusBar',
      name: 'StatusBar1',
    });
    expect(bounds(doc, 'StatusBar1')).toEqual([0, 276, 400, 24]);
    // 既存の alBottom の BottomPanel は、その上に移る
    expect(bounds(doc, 'BottomPanel')[1]).toBe(235);
  });

  it('alTop のコントロールの Top を変えると、並びが変わる', () => {
    let doc = apply({
      type: 'batch',
      commands: [
        { type: 'addControl', parent: 'MainForm', className: 'TPanel', name: 'Top1' },
        { type: 'addControl', parent: 'MainForm', className: 'TPanel', name: 'Top2' },
        { type: 'setProperties', changes: [{ node: 'Top1', path: ['Align'], value: 'alTop' }] },
        { type: 'setProperties', changes: [{ node: 'Top2', path: ['Align'], value: 'alTop' }] },
      ],
    });
    expect(bounds(doc, 'Top1')[1]).toBe(0);
    expect(bounds(doc, 'Top2')[1]).toBe(50);
    doc = apply(
      { type: 'setProperties', changes: [{ node: 'Top1', path: ['Top'], value: 60 }] },
      doc,
    );
    expect(bounds(doc, 'Top2')[1]).toBe(0);
    expect(bounds(doc, 'Top1')[1]).toBe(50);
  });
});
