import { describe, expect, it } from 'vitest';
import { parseDocument } from './parse.ts';

function form(controls: unknown[]): string {
  return JSON.stringify({
    formatVersion: 1,
    form: { name: 'Form1', class: 'TForm', controls },
  });
}

describe('古い書き方の値', () => {
  it('列挙型の整数は要素の名前にして読む(TMemo の ScrollBars は int だった)', () => {
    const { document, diagnostics } = parseDocument(
      form([{ name: 'Memo1', class: 'TMemo', properties: { ScrollBars: 3 } }]),
    );
    expect(diagnostics).toEqual([]);
    expect(document?.form.controls?.[0]?.properties?.ScrollBars).toBe('ssBoth');
  });

  it('範囲の外の整数・選べない要素はそのまま(検証でエラーになる)', () => {
    const outOfRange = parseDocument(
      form([{ name: 'Memo1', class: 'TMemo', properties: { ScrollBars: 99 } }]),
    );
    expect(outOfRange.diagnostics.map((d) => d.code)).toEqual(['invalid-property-value']);
    // TBorderStyle は bsNone・bsSingle だけ(2 は bsSizeable)
    const notAllowed = parseDocument(
      form([{ name: 'Edit1', class: 'TEdit', properties: { BorderStyle: 2 } }]),
    );
    expect(notAllowed.document?.form.controls?.[0]?.properties?.BorderStyle).toBe(2);
    expect(notAllowed.diagnostics.map((d) => d.code)).toEqual(['invalid-property-value']);
  });

  it('TBorderStyle は bsNone・bsSingle だけを選べる', () => {
    const ok = parseDocument(
      form([{ name: 'Edit1', class: 'TEdit', properties: { BorderStyle: 'bsNone' } }]),
    );
    expect(ok.diagnostics).toEqual([]);
    const ng = parseDocument(
      form([{ name: 'Edit1', class: 'TEdit', properties: { BorderStyle: 'bsDialog' } }]),
    );
    expect(ng.diagnostics.map((d) => d.code)).toEqual(['invalid-property-value']);
  });
});
