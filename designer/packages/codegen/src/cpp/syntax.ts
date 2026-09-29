import type { LanguageSyntax } from '../region.ts';
import type { NameRules } from '../stale.ts';

/** C++ のソースの規則。ハンドラが定義済みかは `クラス名::ハンドラ名(` の有無で判定する */
export function cppSyntax(className: string): LanguageSyntax {
  return {
    comment: '//',
    indentUnit: '    ',
    blankLinesBeforeAppended: 1,
    // className とハンドラ名は識別子の検証を通っているため、正規表現の特殊文字を含まない
    hasHandler: (text, name) => new RegExp(`\\b${className}::${name}\\s*\\(`).test(text),
    // stubs マーカーがなければ、名前空間の終わりの区間の前(なければ末尾)に追記する
    fallbackStubLine: (lines) => {
      const end = lines.findIndex((l) => /<bethany-designer:begin id="beth_NamespaceEnd">/.test(l));
      if (end >= 0) return end;
      return lines[lines.length - 1] === '' ? lines.length - 1 : lines.length;
    },
  };
}

/**
 * C++ の名前の規則(docs/designer/codegen-design.md M1 / M7)。
 * ヘッダの宣言の区間にあるメンバ変数(`beth::TButton* OkButton;`)とメンバ関数(ハンドラ)を生成したメンバとする。
 */
export const CPP_NAMES: NameRules = {
  memberNames: (text) =>
    [...text.matchAll(/^\s*[\w:]+\*\s*(\w+)\s*;/gm), ...text.matchAll(/^\s*void\s+(\w+)\s*\(/gm)]
      .map((m) => m[1] ?? '')
      .filter((name) => !name.startsWith('beth_')),
  // 他のオブジェクトのメンバ(`other.name`・`other->name`)は除く。名前は識別子の検証を通っている
  uses: (line, name) => new RegExp(`(?<![\\w.>])${name}\\b`).test(line),
};
