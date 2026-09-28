/**
 * DSL の name・ハンドラ名は生成するクラスのメンバ名になるため、C++ と Python の両方で識別子として使えなければならない
 * (docs/designer/dsl-spec.md §4)。
 */
import { getCatalog } from './catalog/catalog.ts';
import { l10n } from './l10n.ts';

const CPP_KEYWORDS: ReadonlySet<string> = new Set([
  'alignas',
  'alignof',
  'and',
  'and_eq',
  'asm',
  'auto',
  'bitand',
  'bitor',
  'bool',
  'break',
  'case',
  'catch',
  'char',
  'char8_t',
  'char16_t',
  'char32_t',
  'class',
  'compl',
  'concept',
  'const',
  'consteval',
  'constexpr',
  'constinit',
  'const_cast',
  'continue',
  'co_await',
  'co_return',
  'co_yield',
  'decltype',
  'default',
  'delete',
  'do',
  'double',
  'dynamic_cast',
  'else',
  'enum',
  'explicit',
  'export',
  'extern',
  'false',
  'float',
  'for',
  'friend',
  'goto',
  'if',
  'inline',
  'int',
  'long',
  'mutable',
  'namespace',
  'new',
  'noexcept',
  'not',
  'not_eq',
  'nullptr',
  'operator',
  'or',
  'or_eq',
  'private',
  'protected',
  'public',
  'register',
  'reinterpret_cast',
  'requires',
  'return',
  'short',
  'signed',
  'sizeof',
  'static',
  'static_assert',
  'static_cast',
  'struct',
  'switch',
  'template',
  'this',
  'thread_local',
  'throw',
  'true',
  'try',
  'typedef',
  'typeid',
  'typename',
  'union',
  'unsigned',
  'using',
  'virtual',
  'void',
  'volatile',
  'wchar_t',
  'while',
  'xor',
  'xor_eq',
]);

const PYTHON_KEYWORDS: ReadonlySet<string> = new Set([
  'False',
  'None',
  'True',
  'and',
  'as',
  'assert',
  'async',
  'await',
  'break',
  'class',
  'continue',
  'def',
  'del',
  'elif',
  'else',
  'except',
  'finally',
  'for',
  'from',
  'global',
  'if',
  'import',
  'in',
  'is',
  'lambda',
  'nonlocal',
  'not',
  'or',
  'pass',
  'raise',
  'return',
  'try',
  'while',
  'with',
  'yield',
]);

/** 予約メソッドの接頭辞(docs/designer/codegen-design.md M6)。利用者の名前には使わせない */
export const RESERVED_PREFIX = 'nvd_';

const IDENTIFIER_PATTERN = /^[A-Za-z_][A-Za-z0-9_]*$/;

export type IdentifierProblem =
  | 'empty'
  | 'invalid-characters'
  | 'cpp-keyword'
  | 'python-keyword'
  | 'reserved-prefix'
  | 'cpp-reserved';

/**
 * name が C++ / Python 双方の識別子として使えるかを判定する。
 * @returns 問題がなければ null、あれば最初に見つかった問題
 */
export function isValidIdentifier(name: string): IdentifierProblem | null {
  if (name.length === 0) return 'empty';
  // 非 ASCII は C++ / Python で扱いが異なるため ASCII に限定する
  if (!IDENTIFIER_PATTERN.test(name)) return 'invalid-characters';
  if (CPP_KEYWORDS.has(name)) return 'cpp-keyword';
  if (PYTHON_KEYWORDS.has(name)) return 'python-keyword';
  if (name.startsWith(RESERVED_PREFIX)) return 'reserved-prefix';
  // C++ では "__" を含む名前と "_大文字" で始まる名前は処理系予約
  if (name.includes('__') || /^_[A-Z]/.test(name)) return 'cpp-reserved';
  return null;
}

export function describeIdentifierProblem(problem: IdentifierProblem): string {
  switch (problem) {
    case 'empty':
      return l10n.t('Cannot be empty');
    case 'invalid-characters':
      return l10n.t('Only letters, digits and _ can be used, and it cannot start with a digit');
    case 'cpp-keyword':
      return l10n.t('C++ keywords cannot be used');
    case 'python-keyword':
      return l10n.t('Python keywords cannot be used');
    case 'reserved-prefix':
      return l10n.t('Names starting with "{0}" are reserved', RESERVED_PREFIX);
    case 'cpp-reserved':
      return l10n.t(
        'Names containing "__" and names starting with "_" and an uppercase letter are reserved in C++',
      );
  }
}

let reservedNames: ReadonlySet<string> | undefined;

/**
 * 生成するクラスの中で、メンバ名にすると生成したコードが壊れる名前:
 * TForm のメンバ(Caption・Show 等)と、生成したコードが使う no_vcl の名前(クラス・列挙型の要素・定数)。
 * C++ ではメンバ名がこれらを隠す(`TButton* TButton;` の後の `new TButton(this)` が壊れる)。
 */
function isReservedName(name: string): boolean {
  if (!reservedNames) {
    const c = getCatalog();
    reservedNames = new Set([
      ...c.formMembers,
      ...Object.keys(c.classes),
      ...Object.values(c.classes).flatMap((info) => info.ancestors),
      ...Object.keys(c.objects),
      ...Object.keys(c.enums),
      ...Object.values(c.enums).flat(),
      ...Object.keys(c.flags),
      ...Object.values(c.flags).flat(),
      ...Object.keys(c.sets),
      ...Object.keys(c.constants),
      ...Object.values(c.constants).flatMap((values) => Object.keys(values)),
      ...Object.keys(c.eventTypes),
      'TextToShortCut',
    ]);
  }
  return reservedNames.has(name);
}

/**
 * 生成するクラスのメンバ(コンポーネント・ハンドラ)の名前として使えるかを判定する。
 * @returns 問題がなければ null、あれば利用者向けの説明
 */
export function memberNameProblem(name: string): string | null {
  const problem = isValidIdentifier(name);
  if (problem) return describeIdentifierProblem(problem);
  if (isReservedName(name))
    return l10n.t(
      '"{0}" is a member of TForm or a name used by no_vcl, so it cannot be used',
      name,
    );
  return null;
}
