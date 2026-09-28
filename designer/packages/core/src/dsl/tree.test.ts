import { describe, expect, it } from 'vitest';
import sampleText from '../../../../samples/MainForm.nvform.json?raw';
import { parseDocument } from './parse.ts';
import { findNode, propertyValue, walkNodes } from './tree.ts';

const { document } = parseDocument(sampleText);
if (!document) throw new Error('sample does not parse');
const doc = document;

describe('tree', () => {
  it('生成の順にたどる(フォーム・コントロール・コンポーネントとメニュー項目)', () => {
    const names = [...walkNodes(doc)].map((l) => l.node.name);
    expect(names.slice(0, 3)).toEqual(['MainForm', 'NameEdit', 'OkButton']);
    expect(names.indexOf('FileMenu')).toBe(names.indexOf('MainMenu1') + 1);
  });

  it('findNode', () => {
    expect(findNode(doc, 'FileOpenItem')?.kind).toBe('menuItem');
    expect(findNode(doc, 'Nothing')).toBeUndefined();
  });

  it('propertyValue は書いていなければカタログの既定値', () => {
    const ok = findNode(doc, 'OkButton');
    if (!ok) throw new Error('OkButton not found');
    expect(propertyValue(ok, ['Caption'])).toBe('&OK');
    expect(propertyValue(ok, ['Visible'])).toBe(true);
    expect(propertyValue(ok, ['Font', 'Style'])).toEqual(['fsBold']);
    expect(propertyValue(ok, ['Font', 'Size'])).toBe(0);
    expect(propertyValue(ok, ['Nothing'])).toBeUndefined();
  });
});
