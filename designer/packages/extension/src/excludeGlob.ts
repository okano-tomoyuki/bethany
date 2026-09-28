/**
 * VS Code の除外の設定(search.exclude・files.exclude)から、workspace.findFiles の除外のグロブを作る。
 * findFiles に除外を渡すと files.exclude の既定の除外が効かなくなるため、両方をまとめて渡す。
 */

/**
 * @param settings 除外の設定(パターン → true / false / { when })。値が true のパターンだけを使う
 * @returns `{a,b}` の形のグロブ。除外が無ければ undefined
 */
export function excludeGlob(
  ...settings: (Record<string, unknown> | undefined)[]
): string | undefined {
  const patterns = new Set<string>();
  for (const setting of settings)
    for (const [pattern, value] of Object.entries(setting ?? {}))
      // `{}` を含むパターンは、まとめた `{a,b}` の中に入れると意味が変わるため使わない
      if (value === true && !/[{},]/.test(pattern)) patterns.add(pattern);
  if (patterns.size === 0) return undefined;
  return `{${[...patterns].join(',')}}`;
}
