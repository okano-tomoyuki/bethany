/** テスト用のドキュメント(designer/samples の見本。verify-cpp でビルド・実行して確かめているもの) */
import { BfmDocument } from '@bethany-designer/core';
import raw from '../../../samples/MainForm.bfm.json' with { type: 'json' };

export const SAMPLE: BfmDocument = BfmDocument.parse(raw);
export const DSL_FILE = 'MainForm.bfm.json';

/** C++ だけを生成する見本 */
export const CPP_SAMPLE: BfmDocument = { ...SAMPLE, codegen: { cpp: {} } };

/** Python だけを生成する見本 */
export const PYTHON_SAMPLE: BfmDocument = { ...SAMPLE, codegen: { python: {} } };
