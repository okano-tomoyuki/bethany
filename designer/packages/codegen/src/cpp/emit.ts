/**
 * C++(no_vcl.hpp)のエミッタ(docs/designer/codegen-design.md §3 の C++)。
 *
 * - ヘッダ: 生成するクラス(no_vcl::TForm の派生)。区間 declarations にコンポーネントのメンバとハンドラの宣言。
 *   ヘッダでは using namespace せず no_vcl:: で修飾する(N2)。
 * - ソース: using namespace no_vcl; とフォームのグローバル変数(N3)、コンストラクタ、区間 nvd_CreateComponents、
 *   ハンドラの雛形。
 */
import type { CommentLocale, EventParam } from '@no-vcl-designer/core';
import { generatedComments } from '../comments.ts';
import type { FormModel, Statement, Target, Value } from '../model.ts';
import type { GeneratedCode, HandlerStub, Region } from '../region.ts';

const INDENT = '    ';

export interface CppFiles {
  readonly header: GeneratedCode;
  readonly source: GeneratedCode;
}

/**
 * @param sourceName DSL のファイル名(生成物の説明に使う)
 * @param headerInclude ソースからヘッダを include するパス
 */
export function emitCpp(
  model: FormModel,
  sourceName: string,
  headerInclude: string,
  locale: CommentLocale | undefined,
): CppFiles {
  const comments = generatedComments(locale);
  const { className, formName } = model;

  const declarations: Region = {
    id: 'declarations',
    indent: 1,
    content: [
      ...model.members.map((m) => `${INDENT}no_vcl::${m.class}* ${m.name};`),
      ...(model.members.length > 0 && model.handlers.length > 0 ? [''] : []),
      ...model.handlers.map((h) => `${INDENT}void ${h.name}(${paramList(h.params, true)});`),
    ].join('\n'),
  };

  const header: GeneratedCode = {
    regions: [declarations],
    stubs: [],
    scaffold: (rendered) =>
      [
        '#pragma once',
        '',
        '#include "no_vcl.hpp"',
        '',
        `/** ${comments.classDoc(sourceName)} */`,
        `class ${className} : public no_vcl::TForm`,
        '{',
        'public:',
        rendered('declarations'),
        '',
        `${INDENT}explicit ${className}(no_vcl::TComponent* AOwner);`,
        '',
        'protected:',
        `${INDENT}~${className}() override = default;`,
        '',
        'private:',
        `${INDENT}void nvd_CreateComponents();`,
        '};',
        '',
        `extern ${className}* ${formName};`,
        '',
      ].join('\n'),
  };

  const create: Region = {
    id: 'nvd_CreateComponents',
    indent: 0,
    content: [
      `// ${comments.createComponents}`,
      `void ${className}::nvd_CreateComponents()`,
      '{',
      ...trimBlanks(model.statements).map((s) => (s.kind === 'blank' ? '' : INDENT + statement(s))),
      '}',
    ].join('\n'),
  };

  const stubs: HandlerStub[] = model.handlers.map((h) => ({
    name: h.name,
    code: [
      `void ${className}::${h.name}(${paramList(h.params, false)})`,
      '{',
      `${INDENT}// ${comments.todo}`,
      '}',
    ].join('\n'),
  }));

  const source: GeneratedCode = {
    regions: [create],
    stubs,
    scaffold: (rendered) =>
      [
        `#include "${headerInclude}"`,
        '',
        'using namespace no_vcl;',
        '',
        `${className}* ${formName} = nullptr;`,
        '',
        `${className}::${className}(TComponent* AOwner)`,
        `${INDENT}: TForm(AOwner)`,
        '{',
        `${INDENT}nvd_CreateComponents();`,
        '}',
        '',
        rendered('nvd_CreateComponents'),
        '',
        '// <no_vcl-designer:handler-stubs>',
        ...stubs.flatMap((s) => ['', s.code]),
        '',
      ].join('\n'),
  };

  return { header, source };
}

/** 先頭の区切り(空行)を除く */
function trimBlanks(statements: readonly Statement[]): readonly Statement[] {
  const first = statements.findIndex((s) => s.kind !== 'blank');
  return first < 0 ? [] : statements.slice(first);
}

function statement(s: Exclude<Statement, { kind: 'blank' }>): string {
  switch (s.kind) {
    case 'create':
      return `${s.name} = new ${s.class}(this);`;
    case 'assign':
      return `${access(s.target, s.path)} = ${value(s.value)};`;
    case 'parent':
      return `${s.target}->${s.property} = ${s.parent ?? 'this'};`;
    case 'strings': {
      const target = access(s.target, [s.property]);
      return [
        ...(s.clear ? [`${target}->Clear();`] : []),
        ...s.lines.map((line) => `${target}->Add(${stringLiteral(line)});`),
      ].join(`\n${INDENT}`);
    }
    case 'event': {
      const args = s.params.map((p) => p.name).join(', ');
      return `${access(s.target, [s.event])} = [this](${paramList(s.params, false)}) { ${s.handler}(${args}); };`;
    }
    case 'menuAdd':
      return s.parentItem === undefined
        ? `${s.menu}->Items->Add(${s.item});`
        : `${s.parentItem}->Add(${s.item});`;
  }
}

/** フォーム自身のプロパティは修飾しない(生成するクラスのメンバ関数の中なので) */
function access(target: Target, path: readonly string[]): string {
  return [...(target === undefined ? [] : [target]), ...path].join('->');
}

function value(v: Value): string {
  switch (v.kind) {
    case 'int':
      return String(v.value);
    case 'float':
      return Number.isInteger(v.value) ? v.value.toFixed(1) : String(v.value);
    case 'bool':
      return v.value ? 'true' : 'false';
    case 'string':
      return stringLiteral(v.value);
    case 'char':
      return charLiteral(v.value);
    case 'name':
    case 'ref':
      return v.name;
    case 'flags':
      return v.names.length === 0 ? '0' : v.names.join(' | ');
    case 'set':
      return [`${v.type}()`, ...v.names].join(' << ');
    case 'rgb':
      return `0x${v.value.toString(16).toUpperCase().padStart(8, '0')} /* ${v.text} */`;
    case 'shortcut':
      return v.text === '' ? '0' : `TextToShortCut(${stringLiteral(v.text)})`;
  }
}

/** ハンドラの引数の並び。ヘッダでは no_vcl の型を修飾する */
function paramList(params: readonly EventParam[], qualify: boolean): string {
  return params
    .map((p) => `${qualify && /^T[A-Z]/.test(p.type) ? `no_vcl::${p.type}` : p.type} ${p.name}`)
    .join(', ');
}

function escape(text: string, quote: string): string {
  return Array.from(text, (ch) => {
    if (ch === '\\' || ch === quote) return `\\${ch}`;
    if (ch === '\n') return '\\n';
    if (ch === '\r') return '\\r';
    if (ch === '\t') return '\\t';
    const code = ch.codePointAt(0) ?? 0;
    // その他の制御文字は 8 進数で書く(16 進数の \x は後ろの文字を取り込んでしまうため)
    if (code < 0x20 || code === 0x7f) return `\\${code.toString(8).padStart(3, '0')}`;
    return ch;
  }).join('');
}

/** std::string の文字列リテラル(ソースは UTF-8 で、非 ASCII の文字はそのまま書く) */
export function stringLiteral(text: string): string {
  return `"${escape(text, '"')}"`;
}

function charLiteral(text: string): string {
  return `'${escape(text, "'")}'`;
}
