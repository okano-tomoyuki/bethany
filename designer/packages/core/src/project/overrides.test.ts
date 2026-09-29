import { describe, expect, it } from 'vitest';
import { mapFormPaths, serializeProject } from './edit.ts';
import { parseProject } from './parse.ts';
import { matchesFormPattern } from './paths.ts';
import type { BfprojDocument } from './schema.ts';

describe('matchesFormPattern', () => {
  it.each([
    ['MainForm.bfm.json', 'MainForm.bfm.json', true],
    ['./MainForm.bfm.json', 'MainForm.bfm.json', true],
    ['*.bfm.json', 'MainForm.bfm.json', true],
    ['*.bfm.json', 'dialogs/About.bfm.json', false],
    ['**/*.bfm.json', 'dialogs/About.bfm.json', true],
    ['**/*.bfm.json', 'MainForm.bfm.json', true],
    ['dialogs', 'dialogs/About.bfm.json', true],
    ['dialogs/', 'dialogs/sub/About.bfm.json', true],
    ['dialogs/**', 'dialogs/sub/About.bfm.json', true],
    ['dialogs', 'dialogs2/About.bfm.json', false],
    ['dialogs/*.bfm.json', 'dialogs/sub/About.bfm.json', false],
    ['Form?.bfm.json', 'Form2.bfm.json', true],
    ['Form?.bfm.json', 'Form10.bfm.json', false],
    ['a+b.bfm.json', 'a+b.bfm.json', true],
  ])('%s と %s → %s', (pattern, form, expected) => {
    expect(matchesFormPattern(pattern, form)).toBe(expected);
  });
});

const PROJECT: BfprojDocument = {
  formatVersion: 1,
  codegen: {
    cpp: {
      namespace: 'app',
      overrides: [
        { forms: ['dialogs', 'Main.bfm.json'], namespace: 'app::dialogs' },
        { forms: ['Other.bfm.json'], includeGuard: 'pragma' },
      ],
    },
  },
  forms: ['Main.bfm.json', 'Other.bfm.json', 'dialogs/About.bfm.json'],
};

describe('codegen.cpp.overrides', () => {
  it('検証: パターンの形と、どのフォームにも当てはまらないパターン、上書きの値', () => {
    const text = JSON.stringify({
      formatVersion: 1,
      codegen: {
        cpp: {
          overrides: [{ forms: ['/abs', 'nothing/**', 'Main.bfm.json'], namespace: 'a::1b' }],
        },
      },
      forms: ['Main.bfm.json'],
    });
    expect(parseProject(text).diagnostics.map((d) => [d.code, d.path.join('.')])).toEqual([
      ['invalid-namespace', 'codegen.cpp.overrides.0.namespace'],
      ['invalid-form-pattern', 'codegen.cpp.overrides.0.forms.0'],
      ['unmatched-form-pattern', 'codegen.cpp.overrides.0.forms.1'],
    ]);
  });

  it('forms が空の要素は構造のエラー', () => {
    const text = JSON.stringify({
      formatVersion: 1,
      codegen: { cpp: { overrides: [{ forms: [], headerDir: 'include' }] } },
    });
    expect(parseProject(text).project).toBeUndefined();
  });

  it('フォームの名前の変更・削除: パスそのもののパターンだけを書き換え、空になった要素は除く', () => {
    const renamed = mapFormPaths(PROJECT, (form) =>
      form === 'Main.bfm.json' ? 'MainForm.bfm.json' : form === 'Other.bfm.json' ? null : form,
    );
    expect(renamed.codegen?.cpp?.overrides).toEqual([
      { forms: ['dialogs', 'MainForm.bfm.json'], namespace: 'app::dialogs' },
    ]);
    expect(renamed.codegen?.cpp?.namespace).toBe('app');
  });

  it('書き出し: overrides は codegen.cpp の最後、各要素は forms が先頭', () => {
    const text = serializeProject({
      formatVersion: 1,
      codegen: {
        cpp: {
          overrides: [{ namespace: 'x', forms: ['a.bfm.json'] }],
          namespace: 'app',
          main: 'm.cpp',
        },
      },
      forms: ['a.bfm.json'],
    });
    const cpp = (JSON.parse(text) as { codegen: { cpp: Record<string, unknown> } }).codegen.cpp;
    expect(Object.keys(cpp)).toEqual(['main', 'namespace', 'overrides']);
    expect(Object.keys((cpp.overrides as object[])[0] ?? {})).toEqual(['forms', 'namespace']);
  });
});
