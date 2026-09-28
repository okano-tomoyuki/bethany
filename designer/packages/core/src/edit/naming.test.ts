import { describe, expect, it } from 'vitest';
import { defaultHandlerName, nameBaseOfClass, nameBaseOfMenuCaption, nextName } from './naming.ts';

describe('名前の採番', () => {
  it('使われていない最小の番号', () => {
    expect(nextName('Button', new Set())).toBe('Button1');
    expect(nextName('Button', new Set(['Button1', 'Button3']))).toBe('Button2');
  });

  it('クラス名・メニューの Caption から', () => {
    expect(nameBaseOfClass('TButton')).toBe('Button');
    expect(nameBaseOfClass('Timer')).toBe('Timer');
    expect(nameBaseOfMenuCaption('&File')).toBe('File');
    expect(nameBaseOfMenuCaption('Save &as...')).toBe('Saveas');
    expect(nameBaseOfMenuCaption('-')).toBe('N');
    expect(nameBaseOfMenuCaption('ファイル')).toBe('MenuItem');
    expect(nameBaseOfMenuCaption('1st')).toBe('St');
  });

  it('ハンドラ名の既定値', () => {
    expect(defaultHandlerName('OkButton', 'OnClick')).toBe('OkButtonClick');
    expect(defaultHandlerName('MainForm', 'OnCloseQuery')).toBe('MainFormCloseQuery');
  });
});
