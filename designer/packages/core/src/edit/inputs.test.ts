import { describe, expect, it } from 'vitest';
import type { PropertyType } from '../catalog/types.ts';
import { formatValue, normalizeShortCut, parseInput } from './inputs.ts';

const value = (type: PropertyType, text: string) => {
  const result = parseInput(type, text);
  return result.ok ? result.value : `error: ${result.error}`;
};

describe('parseInput', () => {
  it('空欄は既定値に戻す(TStrings は空の配列)', () => {
    expect(value({ kind: 'int' }, '')).toBeUndefined();
    expect(value({ kind: 'string' }, '')).toBeUndefined();
    expect(value({ kind: 'strings' }, '')).toEqual([]);
  });

  it('数値・真偽・文字', () => {
    expect(value({ kind: 'int' }, ' -12 ')).toBe(-12);
    expect(value({ kind: 'int' }, '1.5')).toMatch(/^error/);
    expect(value({ kind: 'float' }, '1.5')).toBe(1.5);
    expect(value({ kind: 'float' }, 'x')).toMatch(/^error/);
    expect(value({ kind: 'bool' }, 'true')).toBe(true);
    expect(value({ kind: 'string' }, ' a ')).toBe(' a ');
    expect(value({ kind: 'char' }, '*')).toBe('*');
    expect(value({ kind: 'char' }, 'ab')).toMatch(/^error/);
  });

  it('列挙型・定数・色', () => {
    expect(value({ kind: 'enum', enum: 'TAlign' }, 'alClient')).toBe('alClient');
    expect(value({ kind: 'enum', enum: 'TAlign' }, 'client')).toMatch(/^error/);
    expect(value({ kind: 'alias', alias: 'TCursor' }, 'crHandPoint')).toBe('crHandPoint');
    expect(value({ kind: 'alias', alias: 'TColor' }, 'clRed')).toBe('clRed');
    expect(value({ kind: 'alias', alias: 'TColor' }, '#ff8000')).toBe('#FF8000');
    expect(value({ kind: 'alias', alias: 'TColor' }, 'red')).toMatch(/^error/);
  });

  it('集合はカタログの順に並べる', () => {
    expect(value({ kind: 'set', enum: 'TAnchorKind' }, '[akRight, akLeft]')).toEqual([
      'akLeft',
      'akRight',
    ]);
    expect(value({ kind: 'flags', flags: 'TFontStyles' }, 'fsBold')).toEqual(['fsBold']);
    expect(value({ kind: 'set', enum: 'TAnchorKind' }, 'akX')).toMatch(/^error/);
  });

  it('TStrings は行ごと', () => {
    expect(value({ kind: 'strings' }, 'a\r\nb\n')).toEqual(['a', 'b', '']);
  });
});

describe('ショートカット', () => {
  it('表記を揃える', () => {
    expect(normalizeShortCut('ctrl+s')).toBe('Ctrl+S');
    expect(normalizeShortCut('Alt+Ctrl+Del')).toBe('Ctrl+Alt+Del');
    expect(normalizeShortCut('shift+f5')).toBe('Shift+F5');
    expect(normalizeShortCut('Ctrl+Ctrl+S')).toBeUndefined();
    expect(normalizeShortCut('Ctrl+')).toBeUndefined();
    expect(normalizeShortCut('F25')).toBeUndefined();
    expect(normalizeShortCut('Hyper+S')).toBeUndefined();
  });
});

describe('formatValue', () => {
  it('parseInput の逆', () => {
    expect(formatValue({ kind: 'set', enum: 'TAnchorKind' }, ['akLeft', 'akTop'])).toBe(
      '[akLeft,akTop]',
    );
    expect(formatValue({ kind: 'strings' }, ['a', 'b'])).toBe('a\nb');
    expect(formatValue({ kind: 'int' }, 3)).toBe('3');
    expect(formatValue({ kind: 'bool' }, false)).toBe('false');
    expect(formatValue({ kind: 'string' }, undefined)).toBe('');
  });
});
