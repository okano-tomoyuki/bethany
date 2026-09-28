/**
 * Python(py/no_vcl.py)のエミッタ(docs/designer/codegen-design.md §3 の Python)。
 *
 * - 生成するクラス(TForm の派生)の __init__ の中に、区間 declarations(コンポーネントの型の注釈)。
 * - 区間 nvd_CreateComponents(メソッド)。文の並びは C++ と同じ中間表現から作る。
 * - ハンドラの雛形(pass)。参照渡しの引数(bool& CanClose 等)は Ref で渡される(.value を読み書きする)。
 * - フォームのグローバル変数は生成しない(Form1 = Application.CreateForm(TForm1) と書く。dsl-spec.md §10 Q3)。
 */
import type { CommentLocale } from '@no-vcl-designer/core';
import { generatedComments } from '../comments.ts';
import type { FormModel, Statement, Target, Value } from '../model.ts';
import type { GeneratedCode, HandlerStub, Region } from '../region.ts';

const INDENT = '    ';

/** @param sourceName DSL のファイル名(生成物の説明に使う) */
export function emitPython(
  model: FormModel,
  sourceName: string,
  locale: CommentLocale | undefined,
): GeneratedCode {
  const comments = generatedComments(locale);
  const { className } = model;

  const declarations: Region = {
    id: 'declarations',
    indent: 2,
    content: model.members.map((m) => `${INDENT.repeat(2)}self.${m.name}: ${m.class}`).join('\n'),
  };

  const body = trimBlanks(model.statements).map((s) =>
    s.kind === 'blank'
      ? ''
      : statement(s)
          .map((line) => INDENT.repeat(2) + line)
          .join('\n'),
  );
  const create: Region = {
    id: 'nvd_CreateComponents',
    indent: 1,
    content: [
      `${INDENT}def nvd_CreateComponents(self):`,
      `${INDENT.repeat(2)}"""${comments.createComponents}"""`,
      ...body,
    ].join('\n'),
  };

  const stubs: HandlerStub[] = model.handlers.map((h) => ({
    name: h.name,
    code: [
      `${INDENT}def ${h.name}(${['self', ...h.params.map((p) => p.name)].join(', ')}):`,
      `${INDENT.repeat(2)}pass`,
    ].join('\n'),
  }));

  return {
    regions: [declarations, create],
    stubs,
    scaffold: (rendered) =>
      [
        'from no_vcl import *',
        '',
        '',
        `class ${className}(TForm):`,
        `${INDENT}"""${comments.classDoc(sourceName)}"""`,
        '',
        `${INDENT}def __init__(self, AOwner):`,
        `${INDENT.repeat(2)}super().__init__(AOwner)`,
        rendered('declarations'),
        `${INDENT.repeat(2)}self.nvd_CreateComponents()`,
        '',
        rendered('nvd_CreateComponents'),
        '',
        `${INDENT}# <no_vcl-designer:handler-stubs>`,
        ...stubs.flatMap((s) => ['', s.code]),
        '',
      ].join('\n'),
  };
}

/** 先頭の区切り(空行)を除く */
function trimBlanks(statements: readonly Statement[]): readonly Statement[] {
  const first = statements.findIndex((s) => s.kind !== 'blank');
  return first < 0 ? [] : statements.slice(first);
}

/** 1 つの文(複数行になるものがある) */
function statement(s: Exclude<Statement, { kind: 'blank' }>): string[] {
  switch (s.kind) {
    case 'create':
      return [`self.${s.name} = ${s.class}(self)`];
    case 'assign':
      return [`${access(s.target, s.path)} = ${value(s.value)}`];
    case 'parent':
      return [`self.${s.target}.${s.property} = ${access(s.parent, [])}`];
    case 'strings': {
      const target = access(s.target, [s.property]);
      return [
        ...(s.clear ? [`${target}.Clear()`] : []),
        ...s.lines.map((line) => `${target}.Add(${stringLiteral(line)})`),
      ];
    }
    case 'event':
      return [`${access(s.target, [s.event])} = self.${s.handler}`];
    case 'menuAdd':
      return [
        s.parentItem === undefined
          ? `self.${s.menu}.Items.Add(self.${s.item})`
          : `self.${s.parentItem}.Add(self.${s.item})`,
      ];
  }
}

/** target が undefined ならフォーム自身(self) */
function access(target: Target, path: readonly string[]): string {
  return ['self', ...(target === undefined ? [] : [target]), ...path].join('.');
}

function value(v: Value): string {
  switch (v.kind) {
    case 'int':
      return String(v.value);
    case 'float':
      return Number.isInteger(v.value) ? v.value.toFixed(1) : String(v.value);
    case 'bool':
      return v.value ? 'True' : 'False';
    case 'string':
    case 'char':
      return stringLiteral(v.value);
    case 'name':
      return v.name;
    case 'ref':
      return `self.${v.name}`;
    case 'flags':
      return v.names.length === 0 ? `${v.type}(0)` : v.names.join(' | ');
    case 'set':
      return v.names.length === 0 ? 'set()' : `{${v.names.join(', ')}}`;
    case 'rgb':
      // 行末のコメント(値は文の最後なので、後ろに何も続かない)
      return `0x${v.value.toString(16).toUpperCase().padStart(8, '0')}  # ${v.text}`;
    case 'shortcut':
      return v.text === '' ? '0' : `TextToShortCut(${stringLiteral(v.text)})`;
  }
}

/** str のリテラル(非 ASCII の文字はそのまま書く。ソースは UTF-8) */
export function stringLiteral(text: string): string {
  const body = Array.from(text, (ch) => {
    if (ch === '\\' || ch === '"') return `\\${ch}`;
    if (ch === '\n') return '\\n';
    if (ch === '\r') return '\\r';
    if (ch === '\t') return '\\t';
    const code = ch.codePointAt(0) ?? 0;
    if (code < 0x20 || code === 0x7f) return `\\x${code.toString(16).padStart(2, '0')}`;
    return ch;
  }).join('');
  return `"${body}"`;
}
