/**
 * JSON 上の位置(診断の path)を、テキストの範囲に変換する(docs/designer/editor-design.md §8)。
 * 拡張が問題パネルに出すときに使う。
 */
import { findNodeAtLocation, parseTree, type Node } from 'jsonc-parser';
import type { JsonPath } from './diagnostics.ts';

export interface TextRange {
  /** 開始位置(文字のオフセット) */
  readonly start: number;
  /** 終了位置(この位置は含まない) */
  readonly end: number;
}

/**
 * path の値の範囲。path が無ければ、たどれたところまでの最も近い祖先の範囲。
 * オブジェクトのプロパティの値なら、キーを含めた範囲にする(`"Caption": "OK"`)。
 * テキストが JSON として読めなければ先頭の 1 文字。
 */
export function locate(text: string, path: JsonPath): TextRange {
  const root = parseTree(text);
  if (!root) return { start: 0, end: Math.min(1, text.length) };
  let node: Node = root;
  for (let i = path.length; i > 0; i--) {
    const found = findNodeAtLocation(root, [...path.slice(0, i)]);
    if (found) {
      node = found;
      break;
    }
  }
  // オブジェクトのプロパティの値なら、キーから
  const target = node.parent?.type === 'property' ? node.parent : node;
  // 構造の大きいもの(ノード全体等)は、先頭の行だけにする
  if (target.type === 'object' || target.type === 'array' || hasContainerValue(target)) {
    const lineEnd = text.indexOf('\n', target.offset);
    const end =
      lineEnd < 0 ? target.offset + target.length : lineEnd - (text[lineEnd - 1] === '\r' ? 1 : 0);
    return { start: target.offset, end: Math.min(end, target.offset + target.length) };
  }
  return { start: target.offset, end: target.offset + target.length };
}

function hasContainerValue(node: Node): boolean {
  const value = node.type === 'property' ? node.children?.[1] : undefined;
  return value?.type === 'object' || value?.type === 'array';
}
