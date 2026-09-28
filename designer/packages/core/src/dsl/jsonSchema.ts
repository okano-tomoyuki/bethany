import * as z from 'zod';
import { NvformDocument } from './schema.ts';

/**
 * テキストエディタでの補完・検証用の JSON Schema を生成する(tk-designer ADR 0009)。
 * VS Code の JSON 言語機能が確実に扱える draft-07 で出力する。
 */
export function documentJsonSchema(): Record<string, unknown> {
  return z.toJSONSchema(NvformDocument, { target: 'draft-7', io: 'input' });
}
