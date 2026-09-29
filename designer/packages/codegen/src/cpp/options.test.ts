import type { BfprojDocument } from '@bethany-designer/core';
import { describe, expect, it } from 'vitest';
import { generateAll, generateCpp } from '../index.ts';
import {
  cppOptions,
  DEFAULT_CPP_OPTIONS,
  DEFAULT_PYTHON_OPTIONS,
  resolveTargets,
} from '../names.ts';
import { generateProject } from '../project.ts';
import { DSL_FILE, SAMPLE } from '../testing.ts';

function generate(
  cpp: Parameters<typeof cppOptions>[0],
  formPath?: string,
  existing: { header?: string; source?: string } = {},
) {
  const result = generateCpp(SAMPLE, DSL_FILE, existing.header, existing.source, undefined, {
    cpp: cppOptions(cpp),
    formPath,
  });
  if (!result.header.ok) throw new Error(result.header.error);
  if (!result.source.ok) throw new Error(result.source.error);
  return { header: result.header, source: result.source };
}

/** 区間の中身(マーカーの行を除く) */
function region(text: string, id: string): string {
  const begin = text.indexOf(`<bethany-designer:begin id="${id}">`);
  const from = text.indexOf('\n', begin) + 1;
  const to = text.lastIndexOf('\n', text.indexOf(`<bethany-designer:end id="${id}"`));
  return to < from ? '' : text.slice(from, to);
}

/** 0.3.0 までの生成物(先頭と末尾の区間が無い)を作る */
function legacy(text: string): string {
  return text
    .replace(/\/\/ <bethany-designer:begin id="beth_HeaderBegin">[\s\S]*?hash="\w+">/, () =>
      ['#pragma once', '', '#include "beth.hpp"'].join('\n'),
    )
    .replace(/\/\/ <bethany-designer:begin id="beth_SourceBegin">\n(.*)\n.*hash="\w+">/, '$1')
    .replace(
      /\/\/ <bethany-designer:begin id="beth_NamespaceBegin">\n(using namespace beth;)\n.*hash="\w+">/,
      '$1',
    )
    .replace(/\/\/ <bethany-designer:begin id="beth_NamespaceBegin">\n.*hash="\w+">\n\n/, '')
    .replace(/\n\/\/ <bethany-designer:begin id="beth_(HeaderEnd|NamespaceEnd)">[\s\S]*$/, '');
}

describe('C++ の設定(codegen.cpp)', () => {
  it('名前空間とインクルードガード(macro は既定。名前空間とファイル名から)', () => {
    const { header, source } = generate({ namespace: 'app::ui' });
    expect(region(header.text, 'beth_HeaderBegin')).toBe(
      '#ifndef APP_UI_MAINFORM_HPP\n#define APP_UI_MAINFORM_HPP\n\n#include <bethany/beth.hpp>',
    );
    expect(region(header.text, 'beth_NamespaceBegin')).toBe('namespace app\n{\nnamespace ui\n{');
    expect(region(header.text, 'beth_HeaderEnd')).toBe(
      '} // namespace ui\n} // namespace app\n\n#endif // APP_UI_MAINFORM_HPP',
    );
    expect(region(source.text, 'beth_NamespaceBegin')).toBe(
      'using namespace beth;\n\nnamespace app\n{\nnamespace ui\n{',
    );
    expect(region(source.text, 'beth_NamespaceEnd')).toBe('} // namespace ui\n} // namespace app');
    // フォームの変数とハンドラの雛形は名前空間の中
    expect(source.text.indexOf('TMainForm* MainForm = nullptr;')).toBeGreaterThan(
      source.text.indexOf('namespace ui'),
    );
    expect(source.text.lastIndexOf('void TMainForm::Timer1Timer')).toBeLessThan(
      source.text.indexOf('} // namespace ui'),
    );
  });

  it('#pragma once とマクロ名の先頭(_ は畳む)', () => {
    expect(region(generate({ includeGuard: 'pragma' }).header.text, 'beth_HeaderBegin')).toBe(
      '#pragma once\n\n#include <bethany/beth.hpp>',
    );
    expect(region(generate({ includeGuard: 'pragma' }).header.text, 'beth_HeaderEnd')).toBe('');
    expect(
      region(
        generate({ includeGuardPrefix: 'MY_APP_', headerExtension: '.h' }).header.text,
        'beth_HeaderBegin',
      ),
    ).toContain('#ifndef MY_APP_MAINFORM_H\n');
  });

  it('拡張子と出力先のフォルダ(フォームのフォルダと同じ場所を headerDir・sourceDir の下に作る)', () => {
    const targets = resolveTargets(SAMPLE, DSL_FILE, {
      cpp: cppOptions({
        headerExtension: '.h',
        sourceExtension: '.cc',
        headerDir: 'include',
        sourceDir: 'src',
      }),
      python: DEFAULT_PYTHON_OPTIONS,
      formPath: 'forms/MainForm.bfm.json',
    });
    expect(targets.cpp).toMatchObject({
      header: '../include/forms/MainForm.h',
      source: '../src/forms/MainForm.cc',
      headerInclude: 'forms/MainForm.h',
      includePath: 'forms/MainForm.h',
      includeGuard: 'FORMS_MAINFORM_H',
    });
    expect(targets.python?.file).toBe('MainForm.py');
  });

  it('プロジェクトのフォルダの外のフォームは、フォルダの設定を使わずフォームと同じフォルダに置く', () => {
    const targets = resolveTargets(SAMPLE, DSL_FILE, {
      cpp: cppOptions({ headerDir: 'include' }),
      python: undefined,
      formPath: '../shared/MainForm.bfm.json',
    });
    expect(targets.cpp).toMatchObject({ header: 'MainForm.hpp', headerInclude: 'MainForm.hpp' });
  });

  it('設定を変えて再生成すると、既存のファイルの区間に反映する(区間の外は残す)', () => {
    const before = generate({});
    const edited = before.source.text.replace('// TODO: implement', 'Close();');
    const after = generate({ namespace: 'app' }, undefined, {
      header: before.header.text,
      source: edited,
    });
    expect(after.header.modifiedRegions).toEqual([]);
    expect(region(after.header.text, 'beth_NamespaceBegin')).toBe('namespace app\n{');
    expect(after.source.text).toContain('Close();');
    expect(region(after.source.text, 'beth_NamespaceEnd')).toBe('} // namespace app');
  });
});

describe('以前の生成物の移行', () => {
  const current = generate({ namespace: 'app' });
  // 0.3.0 までの生成物には名前空間が無い
  const previous = generate({});

  it('先頭と末尾の区間を加え、beth.hpp の include をシステムインクルードにする', () => {
    const header = legacy(previous.header.text);
    const source = legacy(previous.source.text);
    expect(header.startsWith('#pragma once\n\n#include "beth.hpp"\n')).toBe(true);
    expect(source.startsWith('#include "MainForm.hpp"\n\nusing namespace beth;\n')).toBe(true);

    const migrated = generate({ namespace: 'app' }, undefined, { header, source });
    expect(migrated.header.text).toBe(current.header.text);
    expect(migrated.source.text).toBe(current.source.text);
    expect(migrated.header.warnings).toBeUndefined();
  });

  it('beth.hpp の include の後ろに足した include は、区間の外(名前空間の前)に残す', () => {
    const header = legacy(previous.header.text).replace(
      '#include "beth.hpp"',
      '#include "beth.hpp"\n#include <vector>',
    );
    const migrated = generate({ namespace: 'app' }, undefined, { header });
    expect(migrated.header.text).toMatch(
      /hash="\w+">\n#include <vector>\n\n\/\/ <bethany-designer:begin id="beth_NamespaceBegin">/,
    );
  });

  it('先頭を手で変えたファイルは、組ごと加えずに警告する', () => {
    const header = legacy(previous.header.text).replace(
      '#pragma once',
      '#ifndef MY_GUARD\n#define MY_GUARD',
    );
    const migrated = generate({ namespace: 'app' }, undefined, { header });
    expect(migrated.header.text).not.toContain('beth_HeaderBegin');
    expect(migrated.header.text).not.toContain('beth_HeaderEnd');
    expect(migrated.header.warnings).toHaveLength(1);

    const all = generateAll(
      SAMPLE,
      DSL_FILE,
      { cpp: DEFAULT_CPP_OPTIONS, python: undefined },
      (path) => (path === 'MainForm.hpp' ? header : undefined),
    );
    if ('error' in all) throw new Error(all.error);
    expect(all.warnings[0]).toMatch(/^MainForm\.hpp: /);
  });
});

describe('起動部分の C++ の設定', () => {
  const project: BfprojDocument = {
    formatVersion: 1,
    codegen: { cpp: { namespace: 'app', headerDir: 'include', main: 'src/Project1.cpp' } },
    mainForm: 'forms/MainForm.bfm.json',
    forms: ['forms/MainForm.bfm.json'],
  };
  const forms = [{ doc: SAMPLE, path: 'forms/MainForm.bfm.json' }];

  function main(existing?: string): string {
    const result = generateProject(project, 'Project1.bfproj.json', forms, () => existing);
    if ('error' in result) throw new Error(result.error);
    const file = result.files[0];
    if (!file?.result.ok) throw new Error('failed');
    return file.result.text;
  }

  it('名前空間で修飾し、headerDir からのパスで include する', () => {
    const text = main();
    expect(region(text, 'beth_Include')).toBe('#include <bethany/beth.hpp>');
    expect(region(text, 'includes')).toBe('#include "forms/MainForm.hpp"');
    expect(text).toContain('Application->CreateForm(&app::MainForm);');
  });

  it('以前の生成物の #include "beth.hpp" を区間に置き換える', () => {
    const text = main();
    const old = text.replace(
      /\/\/ <bethany-designer:begin id="beth_Include">\n.*\n.*hash="\w+">/,
      '#include "beth.hpp"',
    );
    expect(old).not.toContain('beth_Include');
    expect(main(old)).toBe(text);
  });
});

it('DEFAULT_CPP_OPTIONS は #pragma once ではなくマクロ', () => {
  expect(DEFAULT_CPP_OPTIONS.includeGuard).toBe('macro');
});

describe('overrides(一部のフォームだけの設定)', () => {
  const cpp = {
    namespace: 'app',
    headerDir: 'include',
    overrides: [
      { forms: ['dialogs'], namespace: 'app::dialogs' },
      { forms: ['dialogs/About.bfm.json'], includeGuard: 'pragma' as const },
      { forms: ['**/*.bfm.json'], headerExtension: '.h' as const },
    ],
  };

  it('当てはまるものを書いた順に重ね、当てはまらないものは使わない', () => {
    expect(cppOptions(cpp, 'dialogs/About.bfm.json')).toEqual({
      ...DEFAULT_CPP_OPTIONS,
      namespace: ['app', 'dialogs'],
      includeGuard: 'pragma',
      headerExtension: '.h',
      headerDir: 'include',
    });
    expect(cppOptions(cpp, 'MainForm.bfm.json')).toEqual({
      ...DEFAULT_CPP_OPTIONS,
      namespace: ['app'],
      headerExtension: '.h',
      headerDir: 'include',
    });
    // フォームのパスが分からなければ(プロジェクトの外から)上書きは使わない
    expect(cppOptions(cpp).headerExtension).toBe('.hpp');
  });

  it('起動部分は、フォームごとの名前空間で修飾する', () => {
    const project: BfprojDocument = {
      formatVersion: 1,
      codegen: { cpp },
      mainForm: 'MainForm.bfm.json',
      forms: ['MainForm.bfm.json', 'dialogs/About.bfm.json'],
    };
    const about = { ...SAMPLE, form: { ...SAMPLE.form, name: 'About' } };
    const result = generateProject(
      project,
      'Project1.bfproj.json',
      [
        { doc: SAMPLE, path: 'MainForm.bfm.json' },
        { doc: about, path: 'dialogs/About.bfm.json' },
      ],
      () => undefined,
    );
    if ('error' in result) throw new Error(result.error);
    const text = result.files[0]?.result.ok ? result.files[0].result.text : '';
    expect(region(text, 'includes')).toBe('#include "MainForm.h"\n#include "dialogs/About.h"');
    expect(region(text, 'beth_CreateForms')).toBe(
      '    Application->CreateForm(&app::MainForm);\n    Application->CreateForm(&app::dialogs::About);',
    );
  });
});
