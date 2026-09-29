/**
 * 起動部分(プロジェクトの main)のコード生成(docs/designer/project-spec.md §5)。
 * C++Builder のプロジェクトのソース(Project1.cpp)に当たる。Application を初期化し、起動時に作るフォーム
 * (メインフォームが先頭。autoCreate)を作って Run する。
 *
 * - 区間 includes / imports: 起動時に作るフォームのヘッダの include(Python はモジュールの import)。
 * - 区間 beth_CreateForms: Application->CreateForm(&Form1)(Python は Form1.Form1 = Application.CreateForm(Form1.TForm1)。
 *   フォームのモジュールの変数に代入する。C++Builder のフォームのグローバル変数に当たる)。
 * - 区間の外(main の中の他の行など)は利用者が書き足せる。
 */
import { l10n, normalizePath, type BfmDocument, type BfprojDocument } from '@bethany-designer/core';
import { generatedComments } from './comments.ts';
import { cppSyntax } from './cpp/syntax.ts';
import type { GenerateAllResult, OutputFile } from './index.ts';
import {
  cppOptions,
  DEFAULT_CPP_OPTIONS,
  fileNameOf,
  relativePath,
  resolveTargets,
} from './names.ts';
import { PYTHON_SYNTAX } from './python/syntax.ts';
import { createFile, mergeFile, type GeneratedCode, type LanguageSyntax } from './region.ts';

const INDENT = '    ';
const PROJECT_EXTENSION = '.bfproj.json';

export interface ProjectTargets {
  /** プロジェクトファイルのあるフォルダからの相対パス */
  readonly cpp?: { readonly main: string };
  readonly python?: { readonly main: string };
}

/** プロジェクトの codegen に既定値を補う。出力先の既定はプロジェクト名 + .cpp / .py(C++Builder の Project1.cpp と同じ) */
export function resolveProjectTargets(
  project: BfprojDocument,
  projectFileName: string,
): ProjectTargets {
  const base = fileNameOf(projectFileName).replace(/\.bfproj\.json$/, '');
  const settings = project.codegen ?? {};
  return {
    ...(settings.cpp && { cpp: { main: settings.cpp.main ?? `${base}.cpp` } }),
    ...(settings.python && { python: { main: settings.python.main ?? `${base}.py` } }),
  };
}

export interface FormSource {
  /** 検証を通過したフォームのドキュメント */
  readonly doc: BfmDocument;
  /** プロジェクトファイルのあるフォルダからの相対パス */
  readonly path: string;
}

/** 起動部分で作る 1 つのフォームの、出力先と名前 */
interface CreatedForm {
  /** フォームの名前(グローバル変数・モジュールの変数の名前) */
  readonly name: string;
  readonly className: string;
  /** C++ の名前空間で修飾したフォームの変数(app::MainForm) */
  readonly qualifiedName: string;
  /** プロジェクトファイルのあるフォルダからの相対パス */
  readonly header: string;
  /** headerDir を使うときの、headerDir からのヘッダのパス(起動部分からはこれで include する) */
  readonly includePath: string | undefined;
  readonly source: string;
  readonly python: string;
}

/**
 * プロジェクトの codegen に書かれたすべてのターゲットの起動部分を生成する(拡張機能・CLI の共通の入口)。
 * 出力の path はプロジェクトファイルのあるフォルダからの相対パス。
 * @param forms 起動時に作るフォーム(作る順。先頭がメインフォーム。autoCreateForms の順)
 * @param readExisting 出力先の既存の内容を返す(なければ undefined)
 */
export function generateProject(
  project: BfprojDocument,
  projectFileName: string,
  forms: readonly FormSource[],
  readExisting: (path: string) => string | undefined,
): GenerateAllResult | { readonly error: string } {
  const targets = resolveProjectTargets(project, projectFileName);
  if (!targets.cpp && !targets.python)
    return { error: l10n.t('codegen is not set (e.g. {0})', '"codegen": { "cpp": {} }') };
  if (project.mainForm === undefined || forms.length === 0)
    return { error: l10n.t('The project has no main form') };

  const sourceName = fileNameOf(projectFileName);
  const comments = generatedComments(project.codegen?.commentLocale);
  // フォームの出力先・クラス名は決まった規則で決まる(dsl-spec.md §9)
  const created: CreatedForm[] = forms.map((form) => {
    const path = normalizePath(form.path);
    // フォームごとに overrides を重ねた設定(名前空間・ヘッダの置き場所がフォームで違いうる)
    const cpp = project.codegen?.cpp ? cppOptions(project.codegen.cpp, path) : DEFAULT_CPP_OPTIONS;
    const dir = path.slice(0, path.lastIndexOf('/') + 1);
    const t = resolveTargets(form.doc, fileNameOf(path), { cpp, python: true, formPath: path });
    return {
      name: form.doc.form.name,
      className: t.cpp?.className ?? `T${form.doc.form.name}`,
      qualifiedName: [...cpp.namespace, form.doc.form.name].join('::'),
      header: normalizePath(dir + (t.cpp?.header ?? '')),
      includePath: t.cpp?.includePath,
      source: normalizePath(dir + (t.cpp?.source ?? '')),
      python: normalizePath(dir + (t.python?.file ?? '')),
    };
  });
  const files: OutputFile[] = [];
  const add = (path: string, code: GeneratedCode, syntax: LanguageSyntax): void => {
    const existing = readExisting(path);
    files.push({
      path,
      result:
        existing === undefined
          ? { ok: true, text: createFile(code, syntax), modifiedRegions: [], addedStubs: [] }
          : mergeFile(existing, code, syntax),
    });
  };

  if (targets.cpp) {
    const main = normalizePath(targets.cpp.main);
    if (created.some((f) => f.header === main || f.source === main))
      return { error: l10n.t('{0} is also an output of a form', targets.cpp.main) };
    add(
      targets.cpp.main,
      emitCppMain(
        comments.projectDoc(sourceName),
        created.map((f) => ({ ...f, header: f.includePath ?? relativePath(main, f.header) })),
      ),
      cppSyntax(''),
    );
  }

  if (targets.python) {
    const main = normalizePath(targets.python.main);
    if (created.some((f) => f.python === main))
      return { error: l10n.t('{0} is also an output of a form', targets.python.main) };
    const modules: { readonly module: string; readonly form: CreatedForm }[] = [];
    for (const form of created) {
      const module = pythonModule(relativePath(main, form.python));
      if (!module)
        return {
          error: l10n.t(
            '{0} cannot be imported from {1}: put it in the same folder as {1} or below',
            form.python,
            targets.python.main,
          ),
        };
      modules.push({ module, form });
    }
    add(
      targets.python.main,
      emitPythonMain(comments.projectDoc(sourceName), modules),
      PYTHON_SYNTAX,
    );
  }
  const warnings = files.flatMap((f) =>
    f.result.ok ? (f.result.warnings ?? []).map((w) => `${f.path}: ${w}`) : [],
  );
  return { files, warnings };
}

/** プロジェクトファイルの名前か */
export function isProjectFileName(fileName: string): boolean {
  return fileName.endsWith(PROJECT_EXTENSION);
}

function emitCppMain(
  doc: string,
  forms: readonly { readonly header: string; readonly qualifiedName: string }[],
): GeneratedCode {
  return {
    regions: [
      {
        id: 'beth_Include',
        indent: 0,
        content: '#include <bethany/beth.hpp>',
        // 以前の生成物の beth.hpp の include(区間の外にあった)
        ifMissing: (lines) => {
          const at = lines.findIndex((l) => /^#include ["<](bethany\/)?beth\.hpp[">]$/.test(l));
          return at < 0 ? undefined : { start: at, end: at + 1 };
        },
        group: '#include <bethany/beth.hpp>',
      },
      {
        id: 'includes',
        indent: 0,
        content: unique(forms.map((f) => `#include "${f.header}"`)).join('\n'),
      },
      {
        id: 'beth_CreateForms',
        indent: 1,
        content: forms
          .map((f) => `${INDENT}Application->CreateForm(&${f.qualifiedName});`)
          .join('\n'),
      },
    ],
    stubs: [],
    scaffold: (rendered) =>
      [
        `// ${doc}`,
        rendered('beth_Include'),
        rendered('includes'),
        '',
        'using namespace beth;',
        '',
        'int main()',
        '{',
        `${INDENT}Application->Initialize();`,
        rendered('beth_CreateForms'),
        `${INDENT}Application->Run();`,
        `${INDENT}return 0;`,
        '}',
        '',
      ].join('\n'),
  };
}

function emitPythonMain(
  doc: string,
  forms: readonly { readonly module: string; readonly form: CreatedForm }[],
): GeneratedCode {
  return {
    regions: [
      {
        id: 'imports',
        indent: 0,
        content: unique(forms.map((f) => `import ${f.module}`)).join('\n'),
      },
      {
        id: 'beth_CreateForms',
        indent: 1,
        content: forms
          .map(
            ({ module, form }) =>
              `${INDENT}${module}.${form.name} = Application.CreateForm(${module}.${form.className})`,
          )
          .join('\n'),
      },
    ],
    stubs: [],
    scaffold: (rendered) =>
      [
        `"""${doc}"""`,
        'from beth import *',
        '',
        rendered('imports'),
        '',
        '',
        'def main():',
        `${INDENT}Application.Initialize()`,
        rendered('beth_CreateForms'),
        `${INDENT}Application.Run()`,
        '',
        '',
        'if __name__ == "__main__":',
        `${INDENT}main()`,
        '',
      ].join('\n'),
  };
}

function unique(lines: readonly string[]): string[] {
  return [...new Set(lines)];
}

/** main.py から見たフォームの .py の相対パスを、import のモジュール名にする。上のフォルダ(..)なら undefined */
function pythonModule(path: string): string | undefined {
  const parts = path.replace(/\.py$/, '').split('/');
  return parts.every((part) => /^[A-Za-z_]\w*$/.test(part)) ? parts.join('.') : undefined;
}
