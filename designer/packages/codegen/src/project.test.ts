import { createDocument, createProject, type BfprojDocument } from '@bethany-designer/core';
import { describe, expect, it } from 'vitest';
import type { GenerateAllResult } from './index.ts';
import {
  DEFAULT_CPP_OPTIONS,
  DEFAULT_PYTHON_OPTIONS,
  formCodegenSettings,
  resolveTargets,
} from './names.ts';
import { SAMPLE } from './testing.ts';
import { generateProject, resolveProjectTargets } from './project.ts';

const mainForm = { doc: createDocument('MainForm'), path: 'MainForm.bfm.json' };
const project = createProject(['MainForm.bfm.json']);

function files(result: GenerateAllResult | { error: string }): Record<string, string> {
  if ('error' in result) throw new Error(result.error);
  return Object.fromEntries(
    result.files.map((f) => {
      if (!f.result.ok) throw new Error(f.result.error);
      return [f.path, f.result.text];
    }),
  );
}

describe('resolveProjectTargets', () => {
  it('出力先の既定はプロジェクト名 + .cpp / .py', () => {
    expect(resolveProjectTargets(project, 'Project1.bfproj.json')).toEqual({
      cpp: { main: 'Project1.cpp' },
      python: { main: 'Project1.py' },
    });
  });
});

describe('generateProject', () => {
  it('C++ は Application を初期化し、メインフォームを CreateForm して Run する', () => {
    const cpp = files(
      generateProject(project, 'Project1.bfproj.json', [mainForm], () => undefined),
    )['Project1.cpp'];
    expect(cpp).toContain('#include <bethany/beth.hpp>');
    expect(cpp).toContain('#include "MainForm.hpp"');
    expect(cpp).toMatch(
      /Application->Initialize\(\);\n.*begin id="beth_CreateForms">\n {4}Application->CreateForm\(&MainForm\);\n.*\n {4}Application->Run\(\);/,
    );
  });

  it('Python はメインフォームのモジュールを import し、main() で CreateForm して Run する', () => {
    const py = files(generateProject(project, 'Project1.bfproj.json', [mainForm], () => undefined))[
      'Project1.py'
    ];
    expect(py).toContain('import MainForm\n');
    expect(py).toContain('    MainForm.MainForm = Application.CreateForm(MainForm.TMainForm)');
    expect(py).toContain('if __name__ == "__main__":\n    main()');
  });

  it('サブフォルダのフォームは include のパス・import のモジュール名に反映する', () => {
    const result = files(
      generateProject(
        project,
        'Project1.bfproj.json',
        [{ doc: createDocument('MainForm'), path: 'forms/MainForm.bfm.json' }],
        () => undefined,
      ),
    );
    expect(result['Project1.cpp']).toContain('#include "forms/MainForm.hpp"');
    expect(result['Project1.py']).toContain('import forms.MainForm\n');
    expect(result['Project1.py']).toContain(
      'forms.MainForm.MainForm = Application.CreateForm(forms.MainForm.TMainForm)',
    );
  });

  it('区間の外に書き足したコードは再生成しても残り、メインフォームの変更は区間に反映する', () => {
    const first = files(
      generateProject(project, 'Project1.bfproj.json', [mainForm], () => undefined),
    )['Project1.cpp'];
    const edited = (first ?? '').replace(
      '    Application->Run();',
      '    Application->Title = "My App";\n    Application->Run();',
    );
    const other = { doc: createDocument('Form2'), path: 'Form2.bfm.json' };
    const again = files(
      generateProject(project, 'Project1.bfproj.json', [other], (path) =>
        path === 'Project1.cpp' ? edited : undefined,
      ),
    )['Project1.cpp'];
    expect(again).toContain('Application->Title = "My App";');
    expect(again).toContain('#include "Form2.hpp"');
    expect(again).toContain('Application->CreateForm(&Form2);');
    expect(again).not.toContain('MainForm');
  });

  it('メインフォームが無い・codegen が無い・上のフォルダのフォームを import できないときはエラー', () => {
    const noCodegen: BfprojDocument = { formatVersion: 1, forms: [] };
    expect(generateProject(noCodegen, 'P.bfproj.json', [mainForm], () => undefined)).toHaveProperty(
      'error',
    );
    expect(generateProject(project, 'P.bfproj.json', [], () => undefined)).toHaveProperty('error');
    const outside = { doc: createDocument('MainForm'), path: '../shared/MainForm.bfm.json' };
    expect(
      generateProject(
        { ...project, codegen: { python: {} } },
        'P.bfproj.json',
        [outside],
        () => undefined,
      ),
    ).toHaveProperty('error');
  });

  it('出力先がメインフォームの出力と同じならエラー', () => {
    expect(
      generateProject(
        { ...project, codegen: { cpp: { main: 'MainForm.cpp' } } },
        'P.bfproj.json',
        [mainForm],
        () => undefined,
      ),
    ).toHaveProperty('error');
  });
});

describe('起動時に作るフォーム(autoCreate)', () => {
  it('渡した順(メインフォームが先頭)に include と CreateForm を並べる', () => {
    const form2 = { doc: createDocument('Form2'), path: 'dialogs/Form2.bfm.json' };
    const result = files(
      generateProject(project, 'Project1.bfproj.json', [mainForm, form2], () => undefined),
    );
    expect(result['Project1.cpp']).toContain(
      '#include "MainForm.hpp"\n#include "dialogs/Form2.hpp"\n',
    );
    expect(result['Project1.cpp']).toContain(
      '    Application->CreateForm(&MainForm);\n    Application->CreateForm(&Form2);\n',
    );
    expect(result['Project1.py']).toContain('import MainForm\nimport dialogs.Form2\n');
    expect(result['Project1.py']).toContain(
      '    dialogs.Form2.Form2 = Application.CreateForm(dialogs.Form2.TForm2)\n',
    );
  });
});

describe('formCodegenSettings', () => {
  it('プロジェクトに属さなければ C++ と Python の両方、コメントは英語', () => {
    expect(formCodegenSettings([])).toEqual({
      cpp: DEFAULT_CPP_OPTIONS,
      python: DEFAULT_PYTHON_OPTIONS,
    });
  });

  it('言語は属するプロジェクトの和集合、コメントの言語と C++ の設定は最初に書かれているもの', () => {
    expect(
      formCodegenSettings([
        { doc: { formatVersion: 1, codegen: { python: {} } }, formPath: 'a/Main.bfm.json' },
        {
          doc: {
            formatVersion: 1,
            codegen: { cpp: { namespace: 'app::ui' }, commentLocale: 'ja' },
          },
          formPath: 'Main.bfm.json',
        },
      ]),
    ).toEqual({
      cpp: { ...DEFAULT_CPP_OPTIONS, namespace: ['app', 'ui'] },
      python: { moduleDir: undefined, formPath: 'a/Main.bfm.json' },
      commentLocale: 'ja',
      formPath: 'Main.bfm.json',
    });
    expect(formCodegenSettings([{ doc: { formatVersion: 1 }, formPath: 'Main.bfm.json' }])).toEqual(
      {
        cpp: undefined,
        python: undefined,
        commentLocale: undefined,
      },
    );
  });
});

describe('Python の出力先のフォルダ(codegen.python.moduleDir)', () => {
  const example: BfprojDocument = {
    formatVersion: 1,
    codegen: {
      cpp: { main: 'cpp/Project1.cpp', headerDir: 'cpp', sourceDir: 'cpp' },
      python: { main: 'py/Project1.py', moduleDir: 'py' },
    },
    mainForm: 'MainForm.bfm.json',
    forms: ['MainForm.bfm.json', 'dialogs/About.bfm.json'],
  };
  const about = { ...SAMPLE, form: { ...SAMPLE.form, name: 'About' } };

  it('フォームのモジュールは moduleDir の下の、フォームのフォルダと同じ場所', () => {
    const settings = formCodegenSettings([{ doc: example, formPath: 'dialogs/About.bfm.json' }]);
    expect(resolveTargets(about, 'About.bfm.json', settings)).toMatchObject({
      cpp: { header: '../cpp/dialogs/About.hpp', source: '../cpp/dialogs/About.cpp' },
      python: { file: '../py/dialogs/About.py' },
    });
  });

  it('起動部分は moduleDir からのモジュール名で import する', () => {
    const result = files(
      generateProject(
        example,
        'Project1.bfproj.json',
        [
          { doc: SAMPLE, path: 'MainForm.bfm.json' },
          { doc: about, path: 'dialogs/About.bfm.json' },
        ],
        () => undefined,
      ),
    );
    expect(Object.keys(result)).toEqual(['cpp/Project1.cpp', 'py/Project1.py']);
    expect(result['py/Project1.py']).toContain('import MainForm\nimport dialogs.About\n');
    expect(result['cpp/Project1.cpp']).toContain(
      '#include "MainForm.hpp"\n#include "dialogs/About.hpp"\n',
    );
  });
});
