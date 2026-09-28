import { describe, expect, it } from 'vitest';
import { locate } from './locate.ts';

const TEXT = `{
  "formatVersion": 1,
  "form": {
    "name": "F",
    "class": "TForm",
    "properties": {
      "Caption": "Sample",
      "Font": { "Size": 10 }
    }
  }
}
`;

function slice(text: string, path: (string | number)[]): string {
  const { start, end } = locate(text, path);
  return text.slice(start, end);
}

describe('locate', () => {
  it('プロパティの値はキーを含めた範囲', () => {
    expect(slice(TEXT, ['form', 'properties', 'Caption'])).toBe('"Caption": "Sample"');
    expect(slice(TEXT, ['form', 'properties', 'Font', 'Size'])).toBe('"Size": 10');
  });

  it('オブジェクトは先頭の行だけ', () => {
    expect(slice(TEXT, ['form'])).toBe('"form": {');
    expect(slice(TEXT, ['form', 'properties', 'Font'])).toBe('"Font": { "Size": 10 }');
  });

  it('無い位置は、たどれたところまでの祖先', () => {
    expect(slice(TEXT, ['form', 'properties', 'Missing'])).toBe('"properties": {');
    expect(slice(TEXT, ['form', 'controls', 0, 'name'])).toBe('"form": {');
  });

  it('CRLF の改行は範囲に含めない', () => {
    expect(slice(TEXT.replace(/\n/g, '\r\n'), ['form'])).toBe('"form": {');
  });

  it('JSON として読めないテキストは先頭', () => {
    expect(locate('', [])).toEqual({ start: 0, end: 0 });
  });
});
