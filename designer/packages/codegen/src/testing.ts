/** テスト用のドキュメント(designer/samples の見本。verify-cpp でビルド・実行して確かめているもの) */
import { NvformDocument } from '@no-vcl-designer/core';
import raw from '../../../samples/MainForm.nvform.json' with { type: 'json' };

export const SAMPLE: NvformDocument = NvformDocument.parse(raw);
export const DSL_FILE = 'MainForm.nvform.json';

/** C++ だけを生成する見本 */
export const CPP_SAMPLE: NvformDocument = { ...SAMPLE, codegen: { cpp: {} } };

/** Python だけを生成する見本 */
export const PYTHON_SAMPLE: NvformDocument = { ...SAMPLE, codegen: { python: {} } };
