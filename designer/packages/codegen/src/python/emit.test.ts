import type { ControlNode, BfmDocument } from '@bethany-designer/core';
import { describe, expect, it } from 'vitest';
import { generateAll, generatePython } from '../index.ts';
import { DSL_FILE, PYTHON_ONLY, SAMPLE } from '../testing.ts';

function generate(doc: BfmDocument = SAMPLE, existing?: string, commentLocale?: 'ja') {
  const result = generatePython(doc, DSL_FILE, existing, commentLocale);
  if (!result.ok) throw new Error(result.error);
  return result;
}

/** beth_CreateComponents の中身の文(字下げを除く。docstring の後から) */
function statements(doc: BfmDocument): string[] {
  const text = generate(doc).text;
  const start = text.indexOf('"""', text.indexOf('def beth_CreateComponents(self):'));
  const body = text.slice(
    text.indexOf('\n', start) + 1,
    text.indexOf('\n    # <bethany-designer:end id="beth_CreateComponents"'),
  );
  return body.split('\n').map((l) => l.trim());
}

function form(props: BfmDocument['form']): BfmDocument {
  return { formatVersion: 1, form: props };
}

const INITIAL = generate().text;

describe('generatePython', () => {
  it('新規ファイル(ゴールデンファイルと比較)', async () => {
    const result = generate();
    expect(result.path).toBe('MainForm.py');
    await expect(result.text).toMatchFileSnapshot('../__golden__/MainForm.py');
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
              name: 'Check1',
              class: 'TCheckBox',
              properties: { Checked: true, Anchors: ['akLeft', 'akBottom'] },
            },
          ],
        }),
      ),
    ).toEqual([
      'self.Edit1 = TFloatSpinEdit(self)',
      'self.Check1 = TCheckBox(self)',
      '',
      'self.Caption = "a\\"b\\\\c\\nあ"',
      'self.Color = 0x00302010  # #102030',
      'self.Cursor = crHandPoint',
      '',
      'self.Edit1.Parent = self',
      'self.Edit1.Font.Style = TFontStyles(0)',
      'self.Edit1.Value = 2.0',
      'self.Edit1.Increment = 0.5',
      'self.Edit1.Anchors = set()',
      '',
      'self.Check1.Parent = self',
      'self.Check1.Checked = True',
      'self.Check1.Anchors = {akLeft, akBottom}',
    ]);
  });

  it('ハンドラの引数はイベントの型から作る(参照渡しの引数も名前だけ)', () => {
    const doc = form({
      name: 'F',
      class: 'TForm',
      events: { OnClose: 'FormClose', OnKeyDown: 'FormKeyDown' },
    });
    const text = generate(doc).text;
    expect(text).toContain('    def FormKeyDown(self, Sender, Key, Shift):\n        pass\n');
    expect(text).toContain('    def FormClose(self, Sender, Action):\n        pass\n');
    expect(text).toContain('        self.OnKeyDown = self.FormKeyDown\n');
  });

  it('commentLocale が ja なら日本語のコメント', () => {
    const text = generate(SAMPLE, undefined, 'ja').text;
    expect(text).toContain('"""Bethany のデザイナーで作成したフォーム(MainForm.bfm.json)。');
  });
});

describe('マーカー区間のマージ(Python)', () => {
  it('同じ内容で再生成しても変わらない(冪等)', () => {
    const result = generate(SAMPLE, INITIAL);
    expect(result.text).toBe(INITIAL);
    expect(result.modifiedRegions).toEqual([]);
  });

  it('区間内の手編集は上書きし、その区間を知らせる', () => {
    const edited = INITIAL.replace('self.Caption = "Sample"', 'self.Caption = "Edited"');
    const result = generate(SAMPLE, edited);
    expect(result.text).toBe(INITIAL);
    expect(result.modifiedRegions).toEqual(['beth_CreateComponents']);
  });

  it('フォームの変数の区間が無い(0.2.0 より前に生成した)ファイルには、末尾に加える', () => {
    const before = `${INITIAL.slice(0, INITIAL.indexOf('\n\n# <bethany-designer:begin id="beth_FormVariable">'))}\n`;
    const result = generate(SAMPLE, before);
    expect(result.text).toBe(INITIAL);
  });

  it('stubs マーカーが無ければ、if __name__ == "__main__": の手前に雛形を追記する', () => {
    const withoutStub = INITIAL.replace('    # <bethany-designer:handler-stubs>\n', '')
      .replace('\n    def Timer1Timer(self, Sender):\n        pass\n', '')
      .concat('\n\nif __name__ == "__main__":\n    Application.Initialize()\n');
    const result = generate(SAMPLE, withoutStub);
    expect(result.addedStubs).toEqual(['Timer1Timer']);
    expect(result.text).toContain(
      '    def Timer1Timer(self, Sender):\n        pass\n\n\nif __name__ == "__main__":',
    );
  });
});

describe('DSL からなくなった名前の警告(Python)', () => {
  const renameOk = (doc: BfmDocument): BfmDocument => ({
    ...doc,
    form: {
      ...doc.form,
      controls: doc.form.controls?.map((c): ControlNode =>
        c.name === 'OkButton'
          ? { ...c, name: 'AcceptButton', events: { OnClick: 'AcceptButtonClick' } }
          : c,
      ),
    },
  });

  function warnings(doc: BfmDocument, existing: string): string[] {
    const result = generateAll(doc, DSL_FILE, PYTHON_ONLY, () => existing);
    if ('error' in result) throw new Error(result.error);
    return [...result.warnings];
  }

  it('改名したコンポーネントの参照と、使われなくなったハンドラを知らせる', () => {
    const edited = INITIAL.replace(
      '    def FormCreate(self, Sender):\n        pass',
      '    def FormCreate(self, Sender):\n        self.OkButton.Enabled = False',
    );
    const [warning, ...rest] = warnings(renameOk(SAMPLE), edited);
    expect(rest).toEqual([]);
    expect(warning).toContain('OkButton (MainForm.py line');
    expect(warning).toContain('OkButtonClick (MainForm.py line');
  });

  it('フォームのプロパティ(self.Caption 等)を DSL から消しても警告しない', () => {
    const edited = INITIAL.replace(
      '    def FormCreate(self, Sender):\n        pass',
      '    def FormCreate(self, Sender):\n        self.Caption = "x"',
    );
    const doc = { ...SAMPLE, form: { ...SAMPLE.form, properties: {} } };
    expect(warnings(doc, edited)).toEqual([]);
  });
});
