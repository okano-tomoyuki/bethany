// F5 の開発用ウィンドウで開くフォルダ(.cache/playground)を用意する。
// 見本(samples/)はテストと検証が読むので、開発用ウィンドウでの編集が混ざらないよう、無ければ写しを作る。
// 写しを見本に戻したいときは .cache/playground を消す。
import { cpSync, existsSync } from 'node:fs';

const target = new URL('../../.cache/playground/', import.meta.url);
if (!existsSync(target)) {
  cpSync(new URL('../../samples/', import.meta.url), target, { recursive: true });
  console.log('見本を .cache/playground に写しました');
}
