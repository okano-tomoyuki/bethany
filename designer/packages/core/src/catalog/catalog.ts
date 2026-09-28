import raw from './catalog.json' with { type: 'json' };
import type { Catalog, ClassInfo, PropertyInfo } from './types.ts';

const CATALOG = raw as unknown as Catalog;

export function getCatalog(): Catalog {
  return CATALOG;
}

export function findClass(name: string): ClassInfo | undefined {
  return Object.hasOwn(CATALOG.classes, name) ? CATALOG.classes[name] : undefined;
}

/** クラス name が base そのものか、その派生か(カタログに無いクラスは false) */
export function isSubclassOf(name: string, base: string): boolean {
  if (name === base) return true;
  return findClass(name)?.ancestors.includes(base) ?? false;
}

export function findProperty(info: ClassInfo, name: string): PropertyInfo | undefined {
  return Object.hasOwn(info.properties, name) ? info.properties[name] : undefined;
}

/** Set<E> の要素の列挙型から、集合の型名(TAnchorKind → TAnchors) */
export function setTypeOf(enumName: string): string | undefined {
  return Object.entries(CATALOG.sets).find(([, e]) => e === enumName)?.[0];
}
