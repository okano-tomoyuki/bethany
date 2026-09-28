import type { LanguageSyntax } from '../region.ts';
import type { NameRules } from '../stale.ts';

export const PYTHON_SYNTAX: LanguageSyntax = {
  comment: '#',
  indentUnit: '    ',
  // name は識別子の検証を通っているため、正規表現の特殊文字を含まない
  hasHandler: (text, name) => new RegExp(`^\\s*def\\s+${name}\\s*\\(`, 'm').test(text),
  // stubs マーカーがなければ、`if __name__ == "__main__":` の手前(なければ末尾)に追記する
  fallbackStubLine: (lines) => {
    const main = lines.findIndex((l) => /^if __name__ == ["']__main__["']:/.test(l));
    if (main < 0) return lines[lines.length - 1] === '' ? lines.length - 1 : lines.length;
    // 直前の空行の前に入れる
    let at = main;
    while (at > 0 && lines[at - 1]?.trim() === '') at--;
    return at;
  },
};

/**
 * Python の名前の規則(docs/designer/codegen-design.md M1 / M7)。
 * 宣言の区間の型の注釈(`self.OkButton: TButton`)にあるコンポーネントと、イベントに代入したハンドラ
 * (`... = self.OkButtonClick`)を生成したメンバとする。フォームのプロパティの設定(`self.Caption = ...`)は含めない。
 */
export const PYTHON_NAMES: NameRules = {
  memberNames: (text) =>
    [...text.matchAll(/^\s*self\.(\w+)\s*:/gm), ...text.matchAll(/=\s*self\.(\w+)\s*$/gm)]
      .map((m) => m[1] ?? '')
      .filter((name) => !name.startsWith('beth_')),
  // 名前は識別子の検証を通っているため、正規表現の特殊文字を含まない
  uses: (line, name) =>
    new RegExp(`\\bself\\.${name}\\b`).test(line) ||
    new RegExp(`^\\s*def\\s+${name}\\s*\\(`).test(line),
};
