/**
 * 名前(DSL の name・ハンドラ名)の収集と採番(docs/designer/editor-design.md §3.2)。
 */
import { memberNameProblem } from '../identifier.ts';
import type { BfmDocument } from '../dsl/schema.ts';
import { walkNodes } from '../dsl/tree.ts';

/** 生成するクラスのメンバになる名前(ノードの name とハンドラ名) */
export function collectMemberNames(doc: BfmDocument): Set<string> {
  const names = new Set<string>();
  for (const { node } of walkNodes(doc)) {
    names.add(node.name);
    for (const handler of Object.values(node.events ?? {})) names.add(handler);
  }
  return names;
}

/** ハンドラ名だけ */
export function collectHandlerNames(doc: BfmDocument): Set<string> {
  const names = new Set<string>();
  for (const { node } of walkNodes(doc))
    for (const handler of Object.values(node.events ?? {})) names.add(handler);
  return names;
}

/**
 * base に 1 から番号を付けた名前のうち、使われておらず、メンバ名に使える最小のもの(Button1・Button2)。
 * @param taken 既に使われている名前(同じ操作で続けて採番するときは、先に採番したものを加えて渡す)
 */
export function nextName(base: string, taken: ReadonlySet<string>): string {
  for (let i = 1; ; i++) {
    const name = `${base}${String(i)}`;
    if (!taken.has(name) && memberNameProblem(name) === null) return name;
  }
}

/** コンポーネントの名前の元(クラス名から先頭の T を除く。TButton → Button) */
export function nameBaseOfClass(className: string): string {
  return /^T[A-Z]/.test(className) ? className.slice(1) : className;
}

/**
 * メニュー項目の名前の元(C++Builder と同じ): Caption の英数字(&File → File)。区切り線(-)は N。
 * 英数字が無ければ MenuItem。
 */
export function nameBaseOfMenuCaption(caption: string): string {
  if (caption === '-') return 'N';
  const letters = caption.replace(/[^A-Za-z0-9_]/g, '');
  const base = letters.replace(/^[0-9_]+/, '');
  return base === '' ? 'MenuItem' : base.charAt(0).toUpperCase() + base.slice(1);
}

/**
 * イベントのハンドラ名の既定値(C++Builder と同じ): コンポーネントの名前 + イベント名から On を除いたもの
 * (OkButton の OnClick → OkButtonClick)。
 */
export function defaultHandlerName(componentName: string, event: string): string {
  return componentName + event.replace(/^On/, '');
}
