/**
 * 古い書き方の値を今の書き方に直す(docs/adr/0048)。読み込んだときに行い、保存すると今の書き方で書かれる。
 *
 * - 列挙型のプロパティの整数(列挙の要素の順番): 要素の名前にする。TMemo の ScrollBars は int だったため、
 *   `"ScrollBars": 3` と書かれたファイルがある(今は `"ssBoth"`)。
 */
import { findClass, findProperty, getCatalog } from '../catalog/catalog.ts';
import type { BfmDocument } from './schema.ts';
import { classOf, walkNodes } from './tree.ts';

/** 直すものが無ければ doc をそのまま返す */
export function upgradeDocument(doc: BfmDocument): BfmDocument {
  const catalog = getCatalog();
  let copy: BfmDocument | undefined;
  for (const location of walkNodes(doc)) {
    const info = findClass(classOf(location));
    const properties = location.node.properties;
    if (!info || !properties) continue;
    for (const [key, value] of Object.entries(properties)) {
      const type = findProperty(info, key)?.type;
      if (type?.kind !== 'enum' || typeof value !== 'number') continue;
      const name = catalog.enums[type.enum]?.[value];
      if (name === undefined || (type.values && !type.values.includes(name))) continue;
      const target = [...walkNodes((copy ??= clone(doc)))].find(
        (l) => l.node.name === location.node.name,
      );
      if (target?.node.properties) (target.node.properties as Record<string, unknown>)[key] = name;
    }
  }
  return copy ?? doc;
}

function clone(doc: BfmDocument): BfmDocument {
  return JSON.parse(JSON.stringify(doc)) as BfmDocument;
}
