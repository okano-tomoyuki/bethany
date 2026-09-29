/**
 * 生成したファイルの中のハンドラの位置(docs/designer/editor-design.md §7 のハンドラへの移動)。
 * 定義の見つけ方は、雛形を追記するかの判定(syntax.ts の hasHandler)と同じ。
 */

export type HandlerLanguage = 'cpp' | 'python';

/** 0 から数えた行と桁 */
export interface HandlerPosition {
  readonly line: number;
  readonly character: number;
}

/**
 * ハンドラの本体の最初の行の末尾(カーソルを置く位置)。定義が無ければ undefined。
 * @param className 生成するクラスの名前(C++ の `TMainForm::OkButtonClick(` の判定に使う)
 */
export function findHandler(
  language: HandlerLanguage,
  text: string,
  className: string,
  name: string,
): HandlerPosition | undefined {
  const lines = text.split(/\r?\n/);
  // className とハンドラ名は識別子の検証を通っているため、正規表現の特殊文字を含まない
  const definition =
    language === 'cpp'
      ? new RegExp(`\\b${className}::${name}\\s*\\(`)
      : new RegExp(`^\\s*def\\s+${name}\\s*\\(`);
  const start = lines.findIndex((line) => definition.test(line));
  if (start < 0) return undefined;

  // 本体の始まり(C++ は `{`、Python は行末の `:`)。引数が複数行にわたることがある
  const opener = language === 'cpp' ? /\{/ : /:\s*(#.*)?$/;
  let head = start;
  while (head < lines.length && !opener.test(lines[head] ?? '')) head++;
  if (head >= lines.length) return { line: start, character: 0 };

  const headLine = lines[head] ?? '';
  const body = lines[head + 1];
  // `{` の後ろに本体が続いている、または次の行が無ければ、その行の末尾
  if (body === undefined || (language === 'cpp' && /\{\s*\S/.test(headLine)))
    return { line: head, character: headLine.length };
  // 本体が空(`{` の次の行が `}`)なら `{` の後ろ
  if (language === 'cpp' && body.trim() === '}') return { line: head, character: headLine.length };
  return { line: head + 1, character: body.trimEnd().length };
}
