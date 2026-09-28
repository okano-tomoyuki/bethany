/**
 * 配置の計算(docs/designer/editor-design.md §4。ADR 0036)。LCL の Align・BorderSpacing・Constraints と、親の大きさが変わったときの
 * Anchors の追従を移植したもの。規則は実物の LCL で記録した見本(fixtures/*.lcl.json)と照合して決めた。
 *
 * - Align の順: alTop → alBottom → alLeft → alRight → alClient。残りの範囲から順に切り出す。
 * - 同じ Align の兄弟は、今の位置で並べる(alTop は Top の昇順、alBottom は下端の降順、alLeft は Left の昇順、alRight は右端の降順。
 *   同じ位置なら配列の順)。LCL も位置で並べる。
 * - BorderSpacing: 各辺は Around + その辺。隣り合う 2 つの間は、両者の値の大きいほう(和ではない)。親の端との間は自分の値。
 * - Constraints: Min で広げてから Max で狭める(Max が優先)。alClient が Max で狭まったときは左上に寄せる。
 * - Visible が false のコントロールは寄せない(位置も変えない)。
 * - Anchors(Align が alNone のもの): 親の範囲の大きさが変わったとき、akRight だけなら右との距離を、akLeft と akRight なら
 *   両側の距離を保つ。どちらも無ければ中央の位置の比率を保つ(縦も同じ)。
 * - TToolBar・TCoolBar の子は、バーが自分で並べるので扱わない。TTabSheet は親の TPageControl のクライアント領域いっぱい。
 */
import { findClass } from '../catalog/catalog.ts';
import type { ControlNode, FormNode, BfmDocument, PropertyValue } from '../dsl/schema.ts';
import { clientMetrics } from './insets.ts';

export interface Rect {
  readonly left: number;
  readonly top: number;
  readonly width: number;
  readonly height: number;
}

type Container = FormNode | ControlNode;

/** 子を自分で並べるコンテナ(Align の計算をしない) */
const SELF_ARRANGED: ReadonlySet<string> = new Set(['TToolBar', 'TCoolBar']);

const ALIGN_ORDER = ['alTop', 'alBottom', 'alLeft', 'alRight', 'alClient'] as const;
type Align = (typeof ALIGN_ORDER)[number];

/**
 * after の配置を計算し直す。before は変更前のドキュメント(Anchors の追従に、コンテナの前の大きさを使う。
 * 新しく作ったものは追従しない)。計算で変わったコントロールの Left・Top・Width・Height だけを書き換えた結果を返す。
 */
export function computeLayout(
  before: BfmDocument | undefined,
  after: BfmDocument,
): ReadonlyMap<string, Rect> {
  const previous = new Map<string, Container>();
  if (before) {
    previous.set(before.form.name, before.form);
    for (const c of allControls(before.form)) previous.set(c.name, c);
  }
  const result = new Map<string, Rect>();
  const form = after.form;
  const formArea = areaOf(form, sizeOf(form));
  const beforeForm = before?.form;
  layoutChildren(
    form,
    formArea,
    beforeForm && areaOf(beforeForm, sizeOf(beforeForm)),
    previous,
    result,
  );
  return result;
}

/** コンテナの Align の範囲(子の座標) */
function areaOf(node: Container, size: { width: number; height: number }): Rect {
  if (node.class === 'TForm') return { left: 0, top: 0, ...size };
  const { origin, insets } = clientMetrics(node.class);
  return {
    left: insets.left - origin.x,
    top: insets.top - origin.y,
    width: Math.max(0, size.width - insets.left - insets.right),
    height: Math.max(0, size.height - insets.top - insets.bottom),
  };
}

function layoutChildren(
  container: Container,
  area: Rect,
  previousArea: Rect | undefined,
  previous: ReadonlyMap<string, Container>,
  result: Map<string, Rect>,
): void {
  const children = container.controls ?? [];
  const arranged = !SELF_ARRANGED.has(container.class);
  const bounds = new Map<ControlNode, Rect>(children.map((c) => [c, rectOf(c)]));

  if (arranged) {
    // Anchors(Align で寄せないもの)
    if (
      previousArea &&
      (previousArea.width !== area.width || previousArea.height !== area.height)
    ) {
      for (const child of children) {
        if (child.class === 'TTabSheet' || alignOf(child) !== undefined) continue;
        const r = bounds.get(child);
        if (r) bounds.set(child, constrain(child, anchored(child, r, previousArea, area)));
      }
    }
    // Align
    alignChildren(children, area, bounds);
  }

  for (const child of children) {
    const r = bounds.get(child) ?? rectOf(child);
    if (child.class !== 'TTabSheet' && !sameRect(r, rectOf(child))) result.set(child.name, r);
    if (!findClass(child.class)?.acceptsControls) continue;
    const before = previous.get(child.name);
    if (child.class === 'TTabSheet') {
      // TTabSheet は TPageControl のクライアント領域いっぱい(子の座標の原点は TTabSheet の左上)
      const sheetArea = { left: 0, top: 0, width: area.width, height: area.height };
      const sheetBefore = previousArea && {
        left: 0,
        top: 0,
        width: previousArea.width,
        height: previousArea.height,
      };
      layoutChildren(child, sheetArea, before ? sheetBefore : undefined, previous, result);
      continue;
    }
    const childArea = areaOf(child, { width: r.width, height: r.height });
    const beforeArea = before && areaOf(before, sizeOf(before));
    layoutChildren(child, childArea, beforeArea, previous, result);
  }
}

function alignChildren(
  children: readonly ControlNode[],
  area: Rect,
  bounds: Map<ControlNode, Rect>,
): void {
  // 残りの範囲(right・bottom は右端・下端の座標)と、各辺を決めたコントロールのその側の BorderSpacing
  let left = area.left;
  let top = area.top;
  let right = area.left + area.width;
  let bottom = area.top + area.height;
  const edge = { left: 0, top: 0, right: 0, bottom: 0 };

  for (const align of ALIGN_ORDER) {
    const group = children
      .map((node, index) => ({ node, index, r: bounds.get(node) ?? rectOf(node) }))
      .filter(
        ({ node }) =>
          node.class !== 'TTabSheet' &&
          alignOf(node) === align &&
          node.properties?.Visible !== false,
      )
      .sort((a, b) => sortKey(align, a.r) - sortKey(align, b.r) || a.index - b.index);

    for (const { node, r } of group) {
      const s = spacingOf(node);
      const size = constrainSize(node, r.width, r.height);
      switch (align) {
        case 'alTop': {
          const y = top + Math.max(edge.top, s.top);
          const x = left + Math.max(edge.left, s.left);
          const width = constrainSize(
            node,
            right - Math.max(edge.right, s.right) - x,
            size.height,
          ).width;
          bounds.set(node, { left: x, top: y, width, height: size.height });
          top = y + size.height;
          edge.top = s.bottom;
          break;
        }
        case 'alBottom': {
          const x = left + Math.max(edge.left, s.left);
          const width = constrainSize(
            node,
            right - Math.max(edge.right, s.right) - x,
            size.height,
          ).width;
          const y = bottom - Math.max(edge.bottom, s.bottom) - size.height;
          bounds.set(node, { left: x, top: y, width, height: size.height });
          bottom = y;
          edge.bottom = s.top;
          break;
        }
        case 'alLeft': {
          const x = left + Math.max(edge.left, s.left);
          const y = top + Math.max(edge.top, s.top);
          const height = constrainSize(
            node,
            size.width,
            bottom - Math.max(edge.bottom, s.bottom) - y,
          ).height;
          bounds.set(node, { left: x, top: y, width: size.width, height });
          left = x + size.width;
          edge.left = s.right;
          break;
        }
        case 'alRight': {
          const y = top + Math.max(edge.top, s.top);
          const height = constrainSize(
            node,
            size.width,
            bottom - Math.max(edge.bottom, s.bottom) - y,
          ).height;
          const x = right - Math.max(edge.right, s.right) - size.width;
          bounds.set(node, { left: x, top: y, width: size.width, height });
          right = x;
          edge.right = s.left;
          break;
        }
        case 'alClient': {
          const x = left + Math.max(edge.left, s.left);
          const y = top + Math.max(edge.top, s.top);
          const fit = constrainSize(
            node,
            right - Math.max(edge.right, s.right) - x,
            bottom - Math.max(edge.bottom, s.bottom) - y,
          );
          bounds.set(node, { left: x, top: y, ...fit });
          break;
        }
      }
    }
  }
}

function sortKey(align: Align, r: Rect): number {
  switch (align) {
    case 'alTop':
      return r.top;
    case 'alBottom':
      return -(r.top + r.height);
    case 'alLeft':
      return r.left;
    case 'alRight':
      return -(r.left + r.width);
    case 'alClient':
      return 0;
  }
}

/** 親の範囲が before から after に変わったときの、Anchors による位置と大きさ */
function anchored(node: ControlNode, r: Rect, before: Rect, after: Rect): Rect {
  const anchors = anchorsOf(node);
  const [left, width] = follow(
    r.left,
    r.width,
    before.width,
    after.width,
    anchors.has('akLeft'),
    anchors.has('akRight'),
  );
  const [top, height] = follow(
    r.top,
    r.height,
    before.height,
    after.height,
    anchors.has('akTop'),
    anchors.has('akBottom'),
  );
  return { left, top, width, height };
}

function follow(
  position: number,
  size: number,
  before: number,
  after: number,
  near: boolean,
  far: boolean,
): [number, number] {
  const delta = after - before;
  if (near && far) return [position, Math.max(0, size + delta)];
  if (far) return [position + delta, size];
  if (near || before <= 0) return [position, size];
  // どちらも無ければ、中央の位置の比率を保つ
  return [Math.round(((position + size / 2) * after) / before - size / 2), size];
}

// ---- プロパティの読み取り ---------------------------------------------------------

function num(value: PropertyValue | undefined, fallback = 0): number {
  return typeof value === 'number' ? value : fallback;
}

function rectOf(node: ControlNode): Rect {
  const p = node.properties ?? {};
  return { left: num(p.Left), top: num(p.Top), width: num(p.Width), height: num(p.Height) };
}

function sizeOf(node: Container): { width: number; height: number } {
  const p = node.properties ?? {};
  const size = findClass(node.class)?.defaultSize;
  return { width: num(p.Width, size?.width ?? 0), height: num(p.Height, size?.height ?? 0) };
}

/** 書いた Align か、クラスの既定値(TStatusBar は alBottom)。alNone・alCustom は undefined */
function alignOf(node: ControlNode): Align | undefined {
  const value = node.properties?.Align ?? classDefault(node, 'Align');
  return ALIGN_ORDER.find((a) => a === value);
}

function anchorsOf(node: ControlNode): ReadonlySet<string> {
  const value = node.properties?.Anchors ?? classDefault(node, 'Anchors');
  return new Set(
    Array.isArray(value)
      ? value.filter((v): v is string => typeof v === 'string')
      : ['akLeft', 'akTop'],
  );
}

function classDefault(node: ControlNode, name: string): PropertyValue | undefined {
  const properties = findClass(node.class)?.properties;
  return properties && Object.hasOwn(properties, name)
    ? (properties[name]?.default as PropertyValue)
    : undefined;
}

function objectOf(node: ControlNode, name: string): Readonly<Record<string, PropertyValue>> {
  const value = node.properties?.[name];
  return typeof value === 'object' && value !== null && !Array.isArray(value) ? value : {};
}

function spacingOf(node: ControlNode) {
  const s = objectOf(node, 'BorderSpacing');
  const around = num(s.Around);
  return {
    left: around + num(s.Left),
    top: around + num(s.Top),
    right: around + num(s.Right),
    bottom: around + num(s.Bottom),
  };
}

function constrainSize(
  node: ControlNode,
  width: number,
  height: number,
): { width: number; height: number } {
  const c = objectOf(node, 'Constraints');
  const limit = (value: number, min: number, max: number) => {
    let v = Math.max(0, value);
    if (min > 0) v = Math.max(v, min);
    if (max > 0) v = Math.min(v, max);
    return v;
  };
  return {
    width: limit(width, num(c.MinWidth), num(c.MaxWidth)),
    height: limit(height, num(c.MinHeight), num(c.MaxHeight)),
  };
}

function constrain(node: ControlNode, r: Rect): Rect {
  return { left: r.left, top: r.top, ...constrainSize(node, r.width, r.height) };
}

function sameRect(a: Rect, b: Rect): boolean {
  return a.left === b.left && a.top === b.top && a.width === b.width && a.height === b.height;
}

function* allControls(parent: Container): Generator<ControlNode> {
  for (const c of parent.controls ?? []) {
    yield c;
    yield* allControls(c);
  }
}
