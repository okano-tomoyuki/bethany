/**
 * C++(beth.hpp)のエミッタ(docs/designer/codegen-design.md §3 の C++)。
 *
 * - ヘッダ: 区間 beth_HeaderBegin(インクルードガードと <bethany/beth.hpp> の include)、beth_NamespaceBegin(名前空間の始まり)、
 *   生成するクラス(beth::TForm の派生。区間 declarations にコンポーネントのメンバとハンドラの宣言)、
 *   beth_HeaderEnd(名前空間の終わりとインクルードガードの #endif)。ヘッダでは using namespace せず beth:: で修飾する(N2)。
 * - ソース: 区間 beth_SourceBegin(ヘッダの include)、beth_NamespaceBegin(using namespace beth; と名前空間の始まり)、
 *   フォームのグローバル変数(N3)、コンストラクタ、区間 beth_CreateComponents、ハンドラの雛形、beth_NamespaceEnd。
 * - 先頭と末尾の区間が無い以前の生成物は、以前の生成物の決まった行を区間に置き換える(見つからなければ組ごと加えずに警告)。
 */
import { l10n, type CommentLocale, type EventParam } from '@bethany-designer/core';
import { generatedComments } from '../comments.ts';
import type { FormModel, Statement, Target, Value } from '../model.ts';
import type { CppTarget } from '../names.ts';
import type { GeneratedCode, HandlerStub, LineRange, Region } from '../region.ts';

const INDENT = '    ';

export interface CppFiles {
  readonly header: GeneratedCode;
  readonly source: GeneratedCode;
}

/**
 * @param sourceName DSL のファイル名(生成物の説明に使う)
 * @param target 出力先・名前空間・インクルードガード(resolveTargets)
 */
export function emitCpp(
  model: FormModel,
  sourceName: string,
  target: CppTarget,
  locale: CommentLocale | undefined,
): CppFiles {
  const comments = generatedComments(locale);
  const { className, formName } = model;
  const { includeGuard, namespace } = target;
  // 以前の生成物に加える区間の組(警告に出す名前)
  const group = l10n.t('the include guard and the namespace');
  const namespaceEnd = [...namespace].reverse().map((n) => `} // namespace ${n}`);

  const declarations: Region = {
    id: 'declarations',
    indent: 1,
    content: [
      ...model.members.map((m) => `${INDENT}beth::${m.class}* ${m.name};`),
      ...(model.members.length > 0 && model.handlers.length > 0 ? [''] : []),
      ...model.handlers.map((h) => `${INDENT}void ${h.name}(${paramList(h.params, true)});`),
    ].join('\n'),
  };

  const headerBegin: Region = {
    id: 'beth_HeaderBegin',
    indent: 0,
    content: [
      ...(includeGuard === undefined
        ? ['#pragma once']
        : [`#ifndef ${includeGuard}`, `#define ${includeGuard}`]),
      '',
      '#include <bethany/beth.hpp>',
    ].join('\n'),
    ifMissing: (lines) =>
      // 以前の生成物の先頭: #pragma once・空行・beth.hpp の include
      lines[0] === '#pragma once' &&
      lines[1] === '' &&
      /^#include ["<](bethany\/)?beth\.hpp[">]$/.test(lines[2] ?? '')
        ? { start: 0, end: 3 }
        : undefined,
    group,
  };
  const headerNamespace: Region = {
    id: 'beth_NamespaceBegin',
    indent: 0,
    content: namespaceBegin(namespace),
    // クラスの説明のコメント(無ければクラスの定義)の前
    ifMissing: (lines) => beforeLine(lines, new RegExp(`^class ${className}\\b`), true),
    group,
  };
  const headerEnd: Region = {
    id: 'beth_HeaderEnd',
    indent: 0,
    content: [
      ...namespaceEnd,
      ...(namespaceEnd.length > 0 && includeGuard !== undefined ? [''] : []),
      ...(includeGuard === undefined ? [] : [`#endif // ${includeGuard}`]),
    ].join('\n'),
    ifMissing: 'append',
    group,
  };

  const header: GeneratedCode = {
    regions: [headerBegin, headerNamespace, declarations, headerEnd],
    stubs: [],
    scaffold: (rendered) =>
      [
        rendered('beth_HeaderBegin'),
        '',
        rendered('beth_NamespaceBegin'),
        '',
        `/** ${comments.classDoc(sourceName)} */`,
        `class ${className} : public beth::TForm`,
        '{',
        'public:',
        rendered('declarations'),
        '',
        `${INDENT}explicit ${className}(beth::TComponent* AOwner);`,
        '',
        'protected:',
        `${INDENT}~${className}() override = default;`,
        '',
        'private:',
        `${INDENT}void beth_CreateComponents();`,
        '};',
        '',
        `extern ${className}* ${formName};`,
        '',
        rendered('beth_HeaderEnd'),
        '',
      ].join('\n'),
  };

  const create: Region = {
    id: 'beth_CreateComponents',
    indent: 0,
    content: [
      `// ${comments.createComponents}`,
      `void ${className}::beth_CreateComponents()`,
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

  const sourceBegin: Region = {
    id: 'beth_SourceBegin',
    indent: 0,
    content: `#include "${target.headerInclude}"`,
    // 以前の生成物の先頭: ヘッダの include
    ifMissing: (lines) =>
      /^#include "[^"]+"$/.test(lines[0] ?? '') ? { start: 0, end: 1 } : undefined,
    group,
  };
  const sourceNamespace: Region = {
    id: 'beth_NamespaceBegin',
    indent: 0,
    content: [
      'using namespace beth;',
      ...(namespace.length > 0 ? ['', namespaceBegin(namespace)] : []),
    ].join('\n'),
    // 以前の生成物の using namespace beth;(フォームのグローバル変数の定義より前)
    ifMissing: (lines) => {
      const using = lines.indexOf('using namespace beth;');
      const variable = lines.findIndex((l) => l.startsWith(`${className}* ${formName} =`));
      return using >= 0 && (variable < 0 || using < variable)
        ? { start: using, end: using + 1 }
        : undefined;
    },
    group,
  };
  const sourceEnd: Region = {
    id: 'beth_NamespaceEnd',
    indent: 0,
    content: namespaceEnd.join('\n'),
    ifMissing: 'append',
    group,
  };

  const source: GeneratedCode = {
    regions: [sourceBegin, sourceNamespace, create, sourceEnd],
    stubs,
    scaffold: (rendered) =>
      [
        rendered('beth_SourceBegin'),
        '',
        rendered('beth_NamespaceBegin'),
        '',
        `${className}* ${formName} = nullptr;`,
        '',
        `${className}::${className}(TComponent* AOwner)`,
        `${INDENT}: TForm(AOwner)`,
        '{',
        `${INDENT}beth_CreateComponents();`,
        '}',
        '',
        rendered('beth_CreateComponents'),
        '',
        '// <bethany-designer:handler-stubs>',
        ...stubs.flatMap((s) => ['', s.code]),
        '',
        rendered('beth_NamespaceEnd'),
        '',
      ].join('\n'),
  };

  return { header, source };
}

/** 名前空間の始まり(C++11 でも通るよう、入れ子の名前空間は 1 つずつ書く。波括弧はこのファイルの書き方と同じく次の行) */
function namespaceBegin(namespace: readonly string[]): string {
  return namespace.flatMap((n) => [`namespace ${n}`, '{']).join('\n');
}

/** pattern に合う最初の行の前(直前が説明のコメント /** … *\/ なら、その前)。見つからなければ undefined */
function beforeLine(
  lines: readonly string[],
  pattern: RegExp,
  withDoc: boolean,
): LineRange | undefined {
  let at = lines.findIndex((l) => pattern.test(l));
  if (at < 0) return undefined;
  if (withDoc && lines[at - 1]?.trimStart().startsWith('/**')) at--;
  return { start: at, end: at };
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

/** ハンドラの引数の並び。ヘッダでは Bethany の型を修飾する */
function paramList(params: readonly EventParam[], qualify: boolean): string {
  return params
    .map((p) => `${qualify && /^T[A-Z]/.test(p.type) ? `beth::${p.type}` : p.type} ${p.name}`)
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
