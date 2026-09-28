import { describe, expect, it } from 'vitest';
import { generateCpp } from './index.ts';
import { DSL_FILE, SAMPLE } from './testing.ts';

function generate(existing?: string) {
  const result = generateCpp(SAMPLE, DSL_FILE, undefined, existing);
  if (!result.source.ok) throw new Error(result.source.error);
  return result.source;
}

const INITIAL = generate().text;

describe('マーカー区間のマージ', () => {
  it('同じ内容で再生成しても変わらない(冪等)', () => {
    const result = generate(INITIAL);
    expect(result.text).toBe(INITIAL);
    expect(result.modifiedRegions).toEqual([]);
    expect(result.addedStubs).toEqual([]);
  });

  it('区間の外で書いたコード(ハンドラの実装など)は残す', () => {
    const edited = INITIAL.replace(
      'void TMainForm::OkButtonClick(TObject* Sender)\n{\n    // TODO: implement\n}',
      'void TMainForm::OkButtonClick(TObject* Sender)\n{\n    Close();\n}',
    ).replace('#include "MainForm.hpp"\n', '#include "MainForm.hpp"\n#include <cstdio>\n');
    const result = generate(edited);
    expect(result.text).toBe(edited);
    expect(result.modifiedRegions).toEqual([]);
  });

  it('区間内の手編集は上書きし、その区間を知らせる', () => {
    const edited = INITIAL.replace('Caption = "Sample";', 'Caption = "Edited";');
    const result = generate(edited);
    expect(result.text).toBe(INITIAL);
    expect(result.modifiedRegions).toEqual(['beth_CreateComponents']);
  });

  it('空白・改行の違い(フォーマッタによる整形)は手編集とみなさない', () => {
    const reformatted = INITIAL.replace('Caption = "Sample";', 'Caption =\n        "Sample";');
    expect(generate(reformatted).modifiedRegions).toEqual([]);
  });

  it('足りないハンドラの雛形だけを handler-stubs マーカーの直後に追記する', () => {
    const withoutStub = INITIAL.replace(
      '\nvoid TMainForm::FormCreate(TObject* Sender)\n{\n    // TODO: implement\n}\n',
      '',
    );
    const result = generate(withoutStub);
    expect(result.addedStubs).toEqual(['FormCreate']);
    expect(result.text).toContain(
      '// <bethany-designer:handler-stubs>\n\nvoid TMainForm::FormCreate(TObject* Sender)\n',
    );
  });

  it('改行コード(CRLF)を保つ', () => {
    const crlf = INITIAL.replace(/\n/g, '\r\n');
    expect(generate(crlf).text).toBe(crlf);
  });

  const corruptions: [string, (text: string) => string, string][] = [
    [
      '終了マーカーがない',
      (t) => t.replace(/.*<bethany-designer:end id="beth_CreateComponents".*\n/, ''),
      'no end marker',
    ],
    [
      '区間が見つからない',
      (t) => t.replace(/.*<bethany-designer:(begin|end) id="beth_CreateComponents".*\n/g, ''),
      'beth_CreateComponents',
    ],
    [
      '入れ子',
      (t) =>
        t.replace(
          '// <bethany-designer:end id="beth_CreateComponents"',
          '// <bethany-designer:begin id="x">\n// <bethany-designer:end id="beth_CreateComponents"',
        ),
      'is closed',
    ],
  ];

  it.each(corruptions)('マーカーが壊れていれば何も書き込まない: %s', (_, corrupt, message) => {
    const result = generateCpp(SAMPLE, DSL_FILE, undefined, corrupt(INITIAL));
    expect(result.source.ok).toBe(false);
    expect(!result.source.ok && result.source.error).toContain(message);
  });
});
