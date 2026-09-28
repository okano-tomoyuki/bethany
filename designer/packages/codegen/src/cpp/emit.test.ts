import type { NvformDocument } from '@no-vcl-designer/core';
import { describe, expect, it } from 'vitest';
import { generateAll, generateCpp } from '../index.ts';
import { CPP_SAMPLE, DSL_FILE, SAMPLE } from '../testing.ts';

function generate(doc: NvformDocument = CPP_SAMPLE, dslFile = DSL_FILE) {
  const result = generateCpp(doc, dslFile, undefined, undefined);
  if ('error' in result) throw new Error(result.error);
  if (!result.header.ok) throw new Error(result.header.error);
  if (!result.source.ok) throw new Error(result.source.error);
  return { header: result.header, source: result.source };
}

/** nvd_CreateComponents の中身の文(字下げを除く) */
function statements(doc: NvformDocument): string[] {
  const text = generate(doc).source.text;
  const body = text.slice(
    text.indexOf('::nvd_CreateComponents()\n{\n') + '::nvd_CreateComponents()\n{\n'.length,
    text.indexOf('\n}\n// <no_vcl-designer:end'),
  );
  return body.split('\n').map((l) => l.trim());
}

function form(props: NvformDocument['form']): NvformDocument {
  return { formatVersion: 1, codegen: { cpp: {} }, form: props };
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
      { ...CPP_SAMPLE, form: { ...CPP_SAMPLE.form, name: 'Form1' } },
      'forms/Unit1.nvform.json',
    );
    expect([header.path, source.path]).toEqual(['Unit1.hpp', 'Unit1.cpp']);
    expect(header.text).toContain('class TForm1 : public no_vcl::TForm');
    expect(header.text).toContain('extern TForm1* Form1;');
    expect(source.text).toContain('TForm1* Form1 = nullptr;');
  });

  it('codegen.cpp の指定(クラス名・出力先)に従い、ソースからヘッダへの相対パスで include する', () => {
    const doc = {
      ...CPP_SAMPLE,
      codegen: { cpp: { className: 'TMyForm', header: 'include/my.h', source: 'src/my.cpp' } },
    };
    const { header, source } = generate(doc);
    expect([header.path, source.path]).toEqual(['include/my.h', 'src/my.cpp']);
    expect(source.text).toContain('#include "../include/my.h"');
    expect(source.text).toContain('TMyForm::TMyForm(TComponent* AOwner)');
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
    const doc: NvformDocument = {
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

  it('ハンドラの引数はイベントの型から作り、ヘッダでは no_vcl の型を修飾する', () => {
    const doc = form({
      name: 'F',
      class: 'TForm',
      events: { OnClose: 'FormClose', OnKeyDown: 'FormKeyDown', OnClick: 'FormClick' },
    });
    const { header, source } = generate(doc);
    expect(header.text).toContain(
      '    void FormKeyDown(no_vcl::TObject* Sender, int& Key, no_vcl::TShiftState Shift);',
    );
    expect(header.text).toContain(
      '    void FormClose(no_vcl::TObject* Sender, no_vcl::TCloseAction& Action);',
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
    const lines = statements(CPP_SAMPLE);
    expect(lines.indexOf('PageControl1->ActivePage = MemoSheet;')).toBeGreaterThan(
      lines.indexOf('MemoSheet->PageControl = PageControl1;'),
    );
    expect(lines.indexOf('PageControl1->ActivePage = MemoSheet;')).toBeLessThan(
      lines.indexOf('FileMenu->Caption = "&File";'),
    );
  });

  it('commentLocale が ja なら日本語のコメント', () => {
    const { header, source } = generate({
      ...CPP_SAMPLE,
      codegen: { commentLocale: 'ja', cpp: {} },
    });
    expect(header.text).toContain(
      '/** no_vcl のデザイナーで作成したフォーム(MainForm.nvform.json)。',
    );
    expect(source.text).toContain('// TODO: 実装');
  });
});

describe('generateAll', () => {
  it('Python はまだ生成せず、警告で知らせる', () => {
    const result = generateAll(SAMPLE, DSL_FILE, () => undefined);
    if ('error' in result) throw new Error(result.error);
    expect(result.files.map((f) => f.path)).toEqual(['MainForm.hpp', 'MainForm.cpp']);
    expect(result.warnings).toEqual(['Python code generation is not implemented yet']);
  });

  it('codegen が無ければエラー', () => {
    const result = generateAll({ ...SAMPLE, codegen: undefined }, DSL_FILE, () => undefined);
    expect('error' in result).toBe(true);
  });
});
