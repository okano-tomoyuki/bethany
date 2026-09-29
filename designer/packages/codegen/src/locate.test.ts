import { describe, expect, it } from 'vitest';
import { generateCpp, generatePython } from './index.ts';
import { findHandler } from './locate.ts';
import { DSL_FILE, SAMPLE } from './testing.ts';

function lineAt(text: string, line: number): string {
  return text.split('\n')[line] ?? '';
}

describe('findHandler', () => {
  it('C++: 雛形の本体の行の末尾', () => {
    const { source } = generateCpp(SAMPLE, DSL_FILE, undefined, undefined);
    if (!source.ok) throw new Error(source.error);
    const at = findHandler('cpp', source.text, 'TMainForm', 'OkButtonClick');
    expect(at).toBeDefined();
    if (!at) return;
    expect(lineAt(source.text, at.line - 2)).toBe('void TMainForm::OkButtonClick(TObject* Sender)');
    expect(at.character).toBe(lineAt(source.text, at.line).length);
  });

  it('Python: def の次の行の末尾', () => {
    const result = generatePython(SAMPLE, DSL_FILE, undefined);
    if (!result.ok) throw new Error(result.error);
    const at = findHandler('python', result.text, 'TMainForm', 'OkButtonClick');
    expect(at).toEqual({
      line: result.text.split('\n').indexOf('    def OkButtonClick(self, Sender):') + 1,
      character: '        pass'.length,
    });
  });

  it('宣言(ヘッダ)や呼び出しではなく定義を見つける', () => {
    const text = [
      'void TMainForm::A(TObject* Sender)',
      '{',
      '    B(Sender);',
      '}',
      '',
      'void TMainForm::B(',
      '    TObject* Sender)',
      '{',
      '}',
    ].join('\r\n');
    expect(findHandler('cpp', text, 'TMainForm', 'B')).toEqual({ line: 7, character: 1 });
    expect(findHandler('cpp', text, 'TOther', 'A')).toBeUndefined();
  });

  it('Python: 引数が複数行・本体が同じ行', () => {
    const text = [
      '    def A(self,',
      '          Sender):',
      '        x = 1',
      '    def B(self): pass',
    ].join('\n');
    expect(findHandler('python', text, 'TMainForm', 'A')).toEqual({ line: 2, character: 13 });
    expect(findHandler('python', text, 'TMainForm', 'C')).toBeUndefined();
  });
});
