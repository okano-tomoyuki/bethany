import { describe, expect, it } from 'vitest';
import sampleText from '../../../../samples/MainForm.nvform.json?raw';
import { parseDocument } from './parse.ts';
import type { NvformDocument } from './schema.ts';
import { serializeDocument } from './serialize.ts';

const SAMPLE_TEXT = sampleText.replace(/\r\n/g, '\n');

describe('serializeDocument', () => {
  it('見本は決まった形で書かれている(書き出しても変わらない)', () => {
    const { document } = parseDocument(SAMPLE_TEXT);
    if (!document) throw new Error('sample does not parse');
    expect(serializeDocument(document)).toBe(SAMPLE_TEXT);
  });

  it('キーの順(ノード・プロパティはカタログの順・入れ子のオブジェクトの中も)', () => {
    const doc = {
      form: {
        controls: [
          {
            events: { OnChange: 'C', OnClick: 'B' },
            properties: {
              Caption: 'x',
              Font: { Style: ['fsBold'], Size: 10 },
              Top: 2,
              Left: 1,
            },
            class: 'TCheckBox',
            name: 'Check1',
          },
        ],
        class: 'TForm',
        name: 'F',
      },
      codegen: { python: { file: 'a.py', className: 'A' }, commentLocale: 'ja' },
      formatVersion: 1,
    } as unknown as NvformDocument;
    expect(serializeDocument(doc)).toBe(`{
  "formatVersion": 1,
  "codegen": {
    "commentLocale": "ja",
    "python": { "className": "A", "file": "a.py" }
  },
  "form": {
    "name": "F",
    "class": "TForm",
    "controls": [
      {
        "name": "Check1",
        "class": "TCheckBox",
        "properties": {
          "Left": 1,
          "Top": 2,
          "Caption": "x",
          "Font": { "Size": 10, "Style": ["fsBold"] }
        },
        "events": { "OnClick": "B", "OnChange": "C" }
      }
    ]
  }
}
`);
  });

  it('1 行に収まらない単純な値は複数行にする', () => {
    const lines = Array.from({ length: 12 }, (_, i) => `line ${String(i)}`);
    const doc: NvformDocument = {
      formatVersion: 1,
      form: {
        name: 'F',
        class: 'TForm',
        controls: [{ name: 'Memo1', class: 'TMemo', properties: { Lines: lines } }],
      },
    };
    const text = serializeDocument(doc);
    expect(text).toContain('"Lines": [\n            "line 0",\n');
    expect(parseDocument(text).document).toEqual(doc);
  });

  it('カタログに無いプロパティ・クラスは元の順のまま後ろに置く', () => {
    const doc = {
      formatVersion: 1,
      form: {
        name: 'F',
        class: 'TForm',
        controls: [
          { name: 'X', class: 'TUnknown', properties: { B: 1, A: 2 } },
          { name: 'Y', class: 'TButton', properties: { Zzz: 1, Left: 2 } },
        ],
      },
    } as NvformDocument;
    const text = serializeDocument(doc);
    expect(text).toContain('"B": 1,\n          "A": 2');
    expect(text).toContain('"Left": 2,\n          "Zzz": 1');
  });
});
