import { describe, expect, it } from 'vitest';
import raw from '../../../../samples/MainForm.bfm.json' with { type: 'json' };
import { parseDocument } from './parse.ts';
import type { BfmDocument } from './schema.ts';
import { validateDocument } from './validate.ts';

const SAMPLE_TEXT = JSON.stringify(raw, null, 2);

function sample(): BfmDocument {
  const { document } = parseDocument(SAMPLE_TEXT);
  if (!document) throw new Error('sample does not parse');
  return document;
}

/** 書き換えたドキュメントの診断の [コード, パス] */
function diagnose(edit: (doc: any) => void) {
  const doc = sample();
  edit(doc);
  return validateDocument(doc).map((d) => [d.code, d.path.join('.')]);
}

describe('構造の検証', () => {
  it('見本は診断なしで通る', () => {
    expect(parseDocument(SAMPLE_TEXT).diagnostics).toEqual([]);
  });

  it('JSON の構文エラー', () => {
    expect(parseDocument('{').diagnostics[0]?.code).toBe('json-syntax');
  });

  it('対応していない formatVersion', () => {
    const text = SAMPLE_TEXT.replace('"formatVersion": 1', '"formatVersion": 2');
    expect(parseDocument(text).diagnostics[0]?.code).toBe('unsupported-version');
  });

  it('知らないキーはエラー', () => {
    const text = SAMPLE_TEXT.replace('"class": "TForm",', '"class": "TForm", "parent": "x",');
    const { diagnostics } = parseDocument(text);
    expect(diagnostics.map((d) => d.code)).toEqual(['schema']);
    expect(diagnostics[0]?.path).toEqual(['form']);
  });
});

describe('意味の検証', () => {
  it('名前の重複・識別子・予約された名前', () => {
    expect(
      diagnose((doc) => {
        doc.form.controls[0].name = 'OkButton';
        doc.form.controls[1].controls = undefined;
        doc.components[3].name = 'beth_Timer';
        doc.components[2].name = 'Caption';
      }),
    ).toEqual([
      ['duplicate-name', 'form.controls.1.name'],
      ['reserved-name', 'components.2.name'],
      ['invalid-identifier', 'components.3.name'],
    ]);
  });

  it('フォームの名前から作るクラス名が Bethany の名前と衝突する', () => {
    expect(diagnose((doc) => (doc.form.name = 'Form'))).toEqual([['reserved-name', 'form.name']]);
  });

  it('クラスの種類と親子の制約', () => {
    expect(
      diagnose((doc) => {
        doc.form.controls[0].class = 'TTimer';
        doc.form.controls[1].controls = [{ name: 'Inner', class: 'TLabel' }];
        doc.form.controls[2].controls[0].controls.push({ name: 'Sheet3', class: 'TTabSheet' });
        doc.form.controls.push({ name: 'Orphan', class: 'TTabSheet' });
        doc.components[3].class = 'TButton';
        doc.components[2].items = [];
      }),
    ).toEqual([
      ['wrong-class-kind', 'form.controls.0.class'],
      ['controls-not-allowed', 'form.controls.1.controls.0.class'],
      ['invalid-parent-class', 'form.controls.2.controls.0.controls.3.class'],
      ['invalid-parent-class', 'form.controls.4.class'],
      ['items-not-allowed', 'components.2.items'],
      ['wrong-class-kind', 'components.3.class'],
    ]);
  });

  it('コレクションの項目(docs/adr/0044)', () => {
    expect(
      diagnose((doc) => {
        doc.form.controls[2].controls[0].controls[2].properties.Panels = [
          { Text: 'a', Index: 1 },
          'x',
          { Style: 'bad' },
        ];
      }),
    ).toEqual([
      ['unknown-property', 'form.controls.2.controls.0.controls.2.properties.Panels.0'],
      ['invalid-property-value', 'form.controls.2.controls.0.controls.2.properties.Panels.1'],
      ['invalid-property-value', 'form.controls.2.controls.0.controls.2.properties.Panels.2.Style'],
    ]);
  });

  it('TPageControl の子は TTabSheet だけ', () => {
    expect(
      diagnose((doc) => doc.form.controls[2].controls.push({ name: 'X', class: 'TButton' })),
    ).toEqual([['invalid-child-class', 'form.controls.2.controls.2.class']]);
  });

  it('プロパティの値の型', () => {
    expect(
      diagnose((doc) => {
        const edit = doc.form.controls[0].properties;
        edit.Left = 1.5;
        edit.Anchors = ['akTop', 'akTop'];
        edit.Hint = 3;
        edit.Caption = 'x';
        const button = doc.form.controls[1].properties;
        button.Font = { Style: ['bold'], Size: 12, Weight: 1 };
        button.Align = 'client';
        button.Color = '#12345';
        button.Cursor = 'crHand';
      }),
    ).toEqual([
      ['invalid-property-value', 'form.controls.0.properties.Left'],
      ['invalid-property-value', 'form.controls.0.properties.Anchors'],
      ['invalid-property-value', 'form.controls.0.properties.Hint'],
      ['unknown-property', 'form.controls.0.properties'],
      ['invalid-property-value', 'form.controls.1.properties.Font.Style'],
      ['unknown-property', 'form.controls.1.properties.Font'],
      ['invalid-property-value', 'form.controls.1.properties.Align'],
      ['invalid-property-value', 'form.controls.1.properties.Color'],
      ['invalid-property-value', 'form.controls.1.properties.Cursor'],
    ]);
  });

  it('参照の解決と型', () => {
    expect(
      diagnose((doc) => {
        doc.form.properties.Menu = 'PopupMenu1';
        doc.form.controls[2].properties.ActivePage = 'Nothing';
      }),
    ).toEqual([
      ['reference-type-mismatch', 'form.properties.Menu'],
      ['unknown-reference', 'form.controls.2.properties.ActivePage'],
    ]);
  });

  it('Action(docs/adr/0046): 置き場所と、Action から写るプロパティ・OnClick の警告', () => {
    expect(
      diagnose((doc) => {
        // FileSaveItem(components.0.items.0.items.1)は Action を割り当てている
        const item = doc.components[0].items[0].items[1];
        item.properties.Caption = 'x';
        item.properties.RadioItem = true;
        item.events = { OnClick: 'FileOpenItemClick' };
        doc.components.push({ name: 'LooseAction', class: 'TAction' });
        doc.components[3].actions = [];
        doc.components[4].actions.push({ name: 'Bad', class: 'TTimer' });
      }),
    ).toEqual([
      ['actions-not-allowed', 'components.3.actions'],
      ['wrong-class-kind', 'components.4.actions.1.class'],
      ['wrong-class-kind', 'components.5.class'],
      ['overridden-by-action', 'components.0.items.0.items.1.properties.Caption'],
      ['overridden-by-action', 'components.0.items.0.items.1.events.OnClick'],
    ]);
  });

  it('イベントとハンドラ', () => {
    expect(
      diagnose((doc) => {
        doc.form.events.OnDrawCell = 'FormDrawCell';
        doc.form.controls[0].events.OnChange = 'OkButton';
        doc.form.controls[1].events.OnClick = 'FormCloseQuery';
        doc.form.controls[1].events.OnMouseDown = 'Show';
      }),
    ).toEqual([
      ['unknown-event', 'form.events'],
      ['duplicate-name', 'form.controls.0.events.OnChange'],
      ['handler-signature-conflict', 'form.controls.1.events.OnClick'],
      ['reserved-name', 'form.controls.1.events.OnMouseDown'],
    ]);
  });

  it('同じ型のイベントには同じハンドラを書ける', () => {
    expect(diagnose((doc) => (doc.form.controls[1].events.OnDblClick = 'OkButtonClick'))).toEqual(
      [],
    );
  });
});
