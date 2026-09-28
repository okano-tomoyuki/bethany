import { describe, expect, it } from 'vitest';
import {
  addForm,
  createProject,
  mapFormPaths,
  removeForm,
  serializeProject,
  setMainForm,
} from './edit.ts';
import { parseProject } from './parse.ts';
import {
  isSameOrInside,
  isValidFormPath,
  normalizePath,
  relativePath,
  resolvePath,
} from './paths.ts';

describe('パス', () => {
  it('normalizePath は . と .. を畳み、先頭の .. は残す', () => {
    expect(normalizePath('./a//b/../c.bfm.json')).toBe('a/c.bfm.json');
    expect(normalizePath('../../a.bfm.json')).toBe('../../a.bfm.json');
    expect(normalizePath('/c:/w/x/../y')).toBe('/c:/w/y');
  });

  it('resolvePath・relativePath は互いに逆', () => {
    expect(resolvePath('/c:/w/app', '../shared/About.bfm.json')).toBe(
      '/c:/w/shared/About.bfm.json',
    );
    expect(relativePath('/c:/w/app', '/c:/w/shared/About.bfm.json')).toBe(
      '../shared/About.bfm.json',
    );
    expect(relativePath('/c:/w', '/c:/w/dialogs/About.bfm.json')).toBe('dialogs/About.bfm.json');
  });

  it('isValidFormPath は相対パス・/ 区切り・.bfm.json だけを受け付ける', () => {
    expect(isValidFormPath('dialogs/About.bfm.json')).toBe(true);
    expect(isValidFormPath('../About.bfm.json')).toBe(true);
    expect(isValidFormPath('/c:/About.bfm.json')).toBe(false);
    expect(isValidFormPath('C:/About.bfm.json')).toBe(false);
    expect(isValidFormPath('dialogs\\About.bfm.json')).toBe(false);
    expect(isValidFormPath('About.json')).toBe(false);
    expect(isValidFormPath('.bfm.json')).toBe(false);
  });

  it('isSameOrInside はフォルダの名前の途中では一致しない', () => {
    expect(isSameOrInside('/w/forms/A.bfm.json', '/w/forms')).toBe(true);
    expect(isSameOrInside('/w/forms2/A.bfm.json', '/w/forms')).toBe(false);
  });
});

describe('parseProject', () => {
  it('正しいプロジェクトは診断が無い', () => {
    const result = parseProject(
      '{"formatVersion": 1, "mainForm": "MainForm.bfm.json", "forms": ["MainForm.bfm.json"]}',
    );
    expect(result.project?.mainForm).toBe('MainForm.bfm.json');
    expect(result.diagnostics).toEqual([]);
  });

  it('パスの形・重複・forms に無い mainForm を知らせる', () => {
    const result = parseProject(
      JSON.stringify({
        formatVersion: 1,
        mainForm: 'Other.bfm.json',
        forms: ['A.bfm.json', './A.bfm.json', 'B.json'],
      }),
    );
    expect(result.diagnostics.map((d) => [d.code, d.path])).toEqual([
      ['duplicate-form', ['forms', 1]],
      ['invalid-form-path', ['forms', 2]],
      ['unknown-main-form', ['mainForm']],
    ]);
  });

  it('知らない formatVersion・余分なキーは読まない', () => {
    expect(parseProject('{"formatVersion": 2}').diagnostics[0]?.code).toBe('unsupported-version');
    expect(parseProject('{"formatVersion": 1, "x": 1}').diagnostics[0]?.code).toBe('schema');
  });
});

describe('編集', () => {
  it('最初に加えたフォームがメインフォームになる', () => {
    const project = addForm(addForm(createProject(), 'MainForm.bfm.json'), 'About.bfm.json');
    expect(project.mainForm).toBe('MainForm.bfm.json');
    expect(project.forms).toEqual(['MainForm.bfm.json', 'About.bfm.json']);
    expect(addForm(project, './About.bfm.json')).toBe(project);
  });

  it('setMainForm はプロジェクトに無ければ加える', () => {
    const project = setMainForm(createProject(['A.bfm.json']), './B.bfm.json');
    expect(project.mainForm).toBe('B.bfm.json');
    expect(project.forms).toEqual(['A.bfm.json', 'B.bfm.json']);
  });

  it('メインフォームを外すと残りの先頭がメインフォームになり、最後の 1 つを外すと mainForm が無くなる', () => {
    const project = createProject(['A.bfm.json', 'B.bfm.json']);
    expect(removeForm(project, 'A.bfm.json').mainForm).toBe('B.bfm.json');
    expect(removeForm(createProject(['A.bfm.json']), 'A.bfm.json')).toEqual({
      formatVersion: 1,
      forms: [],
    });
  });

  it('mapFormPaths はメインフォームも書き換え、mainForm が無いプロジェクトに付け足さない', () => {
    const project = createProject(['A.bfm.json', 'B.bfm.json']);
    const renamed = mapFormPaths(project, (f) => (f === 'A.bfm.json' ? 'sub/A2.bfm.json' : f));
    expect(renamed.mainForm).toBe('sub/A2.bfm.json');
    expect(renamed.forms).toEqual(['sub/A2.bfm.json', 'B.bfm.json']);
    const noMain = { formatVersion: 1 as const, forms: ['A.bfm.json'] };
    expect(mapFormPaths(noMain, (f) => f)).toEqual(noMain);
  });

  it('serializeProject は決まった順で、forms を 1 行に 1 つ書く', () => {
    expect(serializeProject({ forms: ['A.bfm.json'], formatVersion: 1, mainForm: 'A.bfm.json' }))
      .toBe(`{
  "formatVersion": 1,
  "mainForm": "A.bfm.json",
  "forms": [
    "A.bfm.json"
  ]
}
`);
  });
});
