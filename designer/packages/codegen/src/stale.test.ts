import type { ControlNode, BfmDocument } from '@bethany-designer/core';
import { describe, expect, it } from 'vitest';
import { generateAll } from './index.ts';
import { CPP_ONLY, DSL_FILE, SAMPLE } from './testing.ts';

type Files = Record<string, string>;

function generate(doc: BfmDocument, existing: Files): { files: Files; warnings: string[] } {
  const result = generateAll(doc, DSL_FILE, CPP_ONLY, (path) => existing[path]);
  if ('error' in result) throw new Error(result.error);
  const files: Files = {};
  for (const f of result.files) {
    if (!f.result.ok) throw new Error(f.result.error);
    files[f.path] = f.result.text;
  }
  return { files, warnings: [...result.warnings] };
}

/** フォームの直下のコントロールを書き換えたドキュメント */
function editControl(
  doc: BfmDocument,
  name: string,
  edit: (node: ControlNode) => ControlNode,
): BfmDocument {
  const controls = doc.form.controls?.map((c) => (c.name === name ? edit(c) : c));
  return { ...doc, form: { ...doc.form, controls } };
}

const renameOk = (doc: BfmDocument) =>
  editControl(doc, 'OkButton', (node) => ({
    ...node,
    name: 'AcceptButton',
    events: { OnClick: 'AcceptButtonClick' },
  }));

function lineOf(text: string, needle: string): number {
  return text.split('\n').findIndex((l) => l.includes(needle)) + 1;
}

describe('DSL からなくなった名前の警告(M1 / M7)', () => {
  it('改名したコンポーネントの参照と、使われなくなったハンドラを知らせる', () => {
    const first = generate(SAMPLE, {}).files;
    const edited = {
      ...first,
      'MainForm.cpp': (first['MainForm.cpp'] ?? '').replace(
        'void TMainForm::FormCreate(TObject* Sender)\n{\n    // TODO: implement\n}',
        'void TMainForm::FormCreate(TObject* Sender)\n{\n    OkButton->Enabled = false;\n}',
      ),
    };
    const { files, warnings } = generate(renameOk(SAMPLE), edited);
    const source = files['MainForm.cpp'] ?? '';
    expect(warnings).toHaveLength(1);
    expect(warnings[0]).toContain(
      `OkButton (MainForm.cpp line ${String(lineOf(source, 'OkButton->Enabled = false'))})`,
    );
    expect(warnings[0]).toContain(
      `OkButtonClick (MainForm.cpp line ${String(lineOf(source, 'TMainForm::OkButtonClick('))})`,
    );
    // 新しい名前のハンドラの雛形は追記される
    expect(source).toContain('void TMainForm::AcceptButtonClick(TObject* Sender)');
  });

  it('名前が変わらなければ警告しない', () => {
    const first = generate(SAMPLE, {}).files;
    expect(generate(SAMPLE, first).warnings).toEqual([]);
  });

  it('区間の外で使われていなければ警告しない', () => {
    const doc = editControl(SAMPLE, 'NameEdit', (node) => ({ ...node, name: 'UserEdit' }));
    const first = generate(SAMPLE, {}).files;
    expect(generate(doc, first).warnings).toEqual([]);
  });

  it('他のオブジェクトのメンバ(x->OkButton)は使っているとみなさない', () => {
    const first = generate(SAMPLE, {}).files;
    const edited = {
      ...first,
      'MainForm.cpp': (first['MainForm.cpp'] ?? '').replace(
        'void TMainForm::FormCreate(TObject* Sender)\n{\n    // TODO: implement\n}',
        'void TMainForm::FormCreate(TObject* Sender)\n{\n    MainForm->OkButton->Enabled = false;\n}',
      ),
    };
    const { warnings } = generate(renameOk(SAMPLE), edited);
    expect(warnings[0]).not.toContain('OkButton (');
  });
});
