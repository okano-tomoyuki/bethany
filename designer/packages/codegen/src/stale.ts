/**
 * DSL からなくなった名前が、生成区間の外で使われていないかを調べる(docs/designer/codegen-design.md M1 / M7)。
 *
 * コンポーネント・ハンドラの名前を変えたり削除したりすると、区間の中は再生成で追従するが、
 * 区間の外(利用者のコード)の参照は古いまま残る。自動では書き換えず、警告で知らせる。
 * Python では実行するまで気づけない(AttributeError・使われないハンドラ)ため、特に重要になる。
 *
 * 前回の生成区間にあって今回の生成区間にない名前を「なくなった名前」とする。
 * 区間の中身から名前を取り出す規則は言語ごとに異なる。
 */
import { l10n } from '@no-vcl-designer/core';
import { splitRegions } from './region.ts';

export interface NameRules {
  /** 生成区間の中身から、生成したメンバ(コンポーネント・ハンドラ)の名前を取り出す */
  readonly memberNames: (regionText: string) => Iterable<string>;
  /** 区間の外の1行が name を使っているか */
  readonly uses: (line: string, name: string) => boolean;
}

/** 言語ごとの名前の規則をまとめた1ターゲット分の入力 */
export interface StaleCheck {
  readonly rules: NameRules;
  /** 名前を取り出すファイル(Python は .py、C++ はヘッダ)の、生成前と生成後の内容 */
  readonly before: string;
  readonly after: string;
  /** 区間の外を調べるファイル(生成後の内容) */
  readonly files: readonly { readonly path: string; readonly text: string }[];
}

/** なくなった名前と、それを使っている場所 */
export interface StaleName {
  readonly name: string;
  readonly uses: readonly { readonly path: string; readonly lines: readonly number[] }[];
}

export function findStaleNames(check: StaleCheck): StaleName[] {
  const before = splitRegions(check.before);
  const after = splitRegions(check.after);
  if (!before || !after) return [];
  const current = new Set(check.rules.memberNames(after.inside));
  const removed = [...new Set(check.rules.memberNames(before.inside))].filter(
    (name) => !current.has(name),
  );

  const outsides = check.files.flatMap((f) => {
    const split = splitRegions(f.text);
    return split ? [{ path: f.path, lines: split.outside }] : [];
  });
  return removed.flatMap((name) => {
    const uses = outsides.flatMap(({ path, lines }) => {
      const found = lines.flatMap((line, i) => (check.rules.uses(line, name) ? [i + 1] : []));
      return found.length > 0 ? [{ path, lines: found }] : [];
    });
    return uses.length > 0 ? [{ name, uses }] : [];
  });
}

export function staleNamesWarning(stale: readonly StaleName[]): string | undefined {
  if (stale.length === 0) return undefined;
  const list = stale
    .map(({ name, uses }) => {
      const where = uses.map((u) => l10n.t('{0} line {1}', u.path, u.lines.join(', '))).join(', ');
      return l10n.t('{0} ({1})', name, where);
    })
    .join(', ');
  return l10n.t(
    'Names removed from the DSL are still used outside the generated regions: {0}. If you renamed them, update the references; delete what is no longer needed (such as unused handlers).',
    list,
  );
}
