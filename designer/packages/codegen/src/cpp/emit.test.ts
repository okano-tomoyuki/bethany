import type { BfmDocument } from '@bethany-designer/core';
import { describe, expect, it } from 'vitest';
import { DEFAULT_FORM_CODEGEN, generateAll, generateCpp } from '../index.ts';
import { DSL_FILE, SAMPLE } from '../testing.ts';

function generate(doc: BfmDocument = SAMPLE, dslFile = DSL_FILE, commentLocale?: 'ja') {
  const result = generateCpp(doc, dslFile, undefined, undefined, commentLocale);
  if (!result.header.ok) throw new Error(result.header.error);
  if (!result.source.ok) throw new Error(result.source.error);
  return { header: result.header, source: result.source };
}

/** beth_CreateComponents の中身の文(字下げを除く) */
function statements(doc: BfmDocument): string[] {
  const text = generate(doc).source.text;
  const body = text.slice(
    text.indexOf('::beth_CreateComponents()\n{\n') + '::beth_CreateComponents()\n{\n'.length,
    text.indexOf('\n}\n// <bethany-designer:end'),
  );
  return body.split('\n').map((l) => l.trim());
}

function form(props: BfmDocument['form']): BfmDocument {
  return { formatVersion: 1, form: props };
}

describe('generateCpp', () => {
  it('新規ファイル(ゴールデンファイルと比較)', async () => {
    const { header, source } = generate();
    expect([header.path, source.path]).toEqual(['MainForm.hpp', 'MainForm.cpp']);
    await expect(header.text).toMatchFileSnapshot('../__golden__/MainForm.hpp');
    await expect(source.text).toMatchFileSnapshot('../__golden__/MainForm.cpp');
  });

  it('クラス名はフォームの名前から、ファイル名は DSL のファイル名から決める', () => {
    const { header, source } = generate(
      { ...SAMPLE, form: { ...SAMPLE.form, name: 'Form1' } },
      'forms/Unit1.bfm.json',
    );
    expect([header.path, source.path]).toEqual(['Unit1.hpp', 'Unit1.cpp']);
    expect(header.text).toContain('class TForm1 : public beth::TForm');
    expect(header.text).toContain('extern TForm1* Form1;');
    expect(source.text).toContain('TForm1* Form1 = nullptr;');
  });

  it('値の書き方', () => {
    expect(
      statements(
        form({
          name: 'F',
          class: 'TForm',
          properties: { Caption: 'a"b\\c\nあ', Color: '#102030', Cursor: 'crHandPoint' },
          controls: [
            {
              name: 'Edit1',
              class: 'TFloatSpinEdit',
              properties: { Value: 2, Increment: 0.5, Anchors: [], Font: { Style: [] } },
            },
            {
              name: 'Dialog',
              class: 'TPanel',
              properties: { BorderSpacing: { Around: 4 }, Constraints: { MinWidth: 10 } },
            },
          ],
        }),
      ),
    ).toEqual([
      'Edit1 = new TFloatSpinEdit(this);',
      'Dialog = new TPanel(this);',
      '',
      'Caption = "a\\"b\\\\c\\nあ";',
      'Color = 0x00302010 /* #102030 */;',
      'Cursor = crHandPoint;',
      '',
      'Edit1->Parent = this;',
      'Edit1->Font->Style = 0;',
      'Edit1->Value = 2.0;',
      'Edit1->Increment = 0.5;',
      'Edit1->Anchors = TAnchors();',
      '',
      'Dialog->Parent = this;',
      'Dialog->BorderSpacing->Around = 4;',
      'Dialog->Constraints->MinWidth = 10;',
    ]);
  });

  it('既定の中身がある TStrings は Clear してから加える', () => {
    const doc: BfmDocument = {
      ...form({ name: 'F', class: 'TForm' }),
      components: [
        { name: 'Colors', class: 'TColorDialog', properties: { CustomColors: ['ColorA=FFFFFF'] } },
      ],
    };
    expect(statements(doc).slice(-2)).toEqual([
      'Colors->CustomColors->Clear();',
      'Colors->CustomColors->Add("ColorA=FFFFFF");',
    ]);
  });

  it('ハンドラの引数はイベントの型から作り、ヘッダでは Bethany の型を修飾する', () => {
    const doc = form({
      name: 'F',
      class: 'TForm',
      events: { OnClose: 'FormClose', OnKeyDown: 'FormKeyDown', OnClick: 'FormClick' },
    });
    const { header, source } = generate(doc);
    expect(header.text).toContain(
      '    void FormKeyDown(beth::TObject* Sender, int& Key, beth::TShiftState Shift);',
    );
    expect(header.text).toContain(
      '    void FormClose(beth::TObject* Sender, beth::TCloseAction& Action);',
    );
    expect(source.text).toContain(
      'OnKeyDown = [this](TObject* Sender, int& Key, TShiftState Shift) { FormKeyDown(Sender, Key, Shift); };',
    );
    // 同じハンドラを複数のイベントに書いても、宣言と雛形は 1 つ
    const shared = form({
      name: 'F',
      class: 'TForm',
      events: { OnClick: 'Shared', OnDblClick: 'Shared' },
    });
    expect(generate(shared).header.text.match(/void Shared\(/g)).toHaveLength(1);
    expect(generate(shared).source.text.match(/void TF::Shared\(/g)).toHaveLength(1);
  });

  it('コントロールへの参照は、すべてのコントロールの親が決まった後に設定する', () => {
    const lines = statements(SAMPLE);
    expect(lines.indexOf('PageControl1->ActivePage = MemoSheet;')).toBeGreaterThan(
      lines.indexOf('MemoSheet->PageControl = PageControl1;'),
    );
    expect(lines.indexOf('PageControl1->ActivePage = MemoSheet;')).toBeLessThan(
      lines.indexOf('FileMenu->Caption = "&File";'),
    );
  });

  it('commentLocale が ja なら日本語のコメント', () => {
    const { header, source } = generate(SAMPLE, DSL_FILE, 'ja');
    expect(header.text).toContain(
      '/** Bethany のデザイナーで作成したフォーム(MainForm.bfm.json)。',
    );
    expect(source.text).toContain('// TODO: 実装');
  });
});

describe('generateAll', () => {
  it('settings で有効なすべての言語を生成する', () => {
    const result = generateAll(SAMPLE, DSL_FILE, DEFAULT_FORM_CODEGEN, () => undefined);
    if ('error' in result) throw new Error(result.error);
    expect(result.files.map((f) => f.path)).toEqual([
      'MainForm.hpp',
      'MainForm.cpp',
      'MainForm.py',
    ]);
    expect(result.warnings).toEqual([]);
  });

  it('生成する言語が無ければエラー', () => {
    const result = generateAll(SAMPLE, DSL_FILE, { cpp: false, python: false }, () => undefined);
    expect('error' in result).toBe(true);
  });
});
