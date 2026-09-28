/**
 * 配置の計算を、実物の LCL で記録した配置(fixtures/*.lcl.json。tools/layout/record.mts)と照合する。
 */
import { describe, expect, it } from 'vitest';
import { parseDocument } from '../dsl/parse.ts';
import type { ControlNode, BfmDocument } from '../dsl/schema.ts';
import { walkNodes } from '../dsl/tree.ts';
import { computeLayout, type Rect } from './engine.ts';
import { FIXTURES } from './fixtures/index.ts';

/**
 * 書いた位置から計算して LCL と一致しないことが分かっている見本(書いた位置が LCL の並べた順と食い違う)。
 * デザイナーは計算した位置を書くので、この形のドキュメントは作らない。
 * - border-spacing: 同じ Top の alTop が 2 つ。LCL は生成の途中で先に寄せたものの位置で並べる
 * - anchors: Holder の子の Anchors の距離が、寄せる前の Holder の大きさ(Width 10)で決まる
 */
const INCONSISTENT = new Set(['border-spacing', 'anchors']);

const fixtures = FIXTURES.map(({ name, text, record }) => {
  const { document, diagnostics } = parseDocument(text);
  if (!document || diagnostics.length > 0)
    throw new Error(`${name}: ${JSON.stringify(diagnostics)}`);
  return { name, document, record };
});

/** コントロールの位置と大きさを書き換えたドキュメント */
function withBounds(
  doc: BfmDocument,
  bounds: ReadonlyMap<string, Rect>,
  form?: { width: number; height: number },
): BfmDocument {
  const rewrite = (c: ControlNode): ControlNode => {
    const r = c.class === 'TTabSheet' ? undefined : bounds.get(c.name);
    return {
      ...c,
      ...(r && {
        properties: { ...c.properties, Left: r.left, Top: r.top, Width: r.width, Height: r.height },
      }),
      ...(c.controls && { controls: c.controls.map(rewrite) }),
    };
  };
  return {
    ...doc,
    form: {
      ...doc.form,
      ...(form && {
        properties: { ...doc.form.properties, Width: form.width, Height: form.height },
      }),
      controls: doc.form.controls?.map(rewrite),
    },
  };
}

function toRects(record: Readonly<Record<string, readonly number[]>>): Map<string, Rect> {
  return new Map(
    Object.entries(record).map(([name, [left = 0, top = 0, width = 0, height = 0]]) => [
      name,
      { left, top, width, height },
    ]),
  );
}

/** 計算した後の、各コントロールの [Left, Top, Width, Height](TTabSheet は書かないので除く) */
function laidOut(before: BfmDocument | undefined, after: BfmDocument): Record<string, number[]> {
  const changes = computeLayout(before, after);
  const result: Record<string, number[]> = {};
  for (const location of walkNodes(withBounds(after, changes))) {
    if (location.kind !== 'control' || location.node.class === 'TTabSheet') continue;
    const p = location.node.properties ?? {};
    result[location.node.name] = [p.Left, p.Top, p.Width, p.Height].map((v) =>
      typeof v === 'number' ? v : 0,
    );
  }
  return result;
}

function expected(
  record: Readonly<Record<string, readonly number[]>>,
  doc: BfmDocument,
): Record<string, number[]> {
  const sheets = new Set(
    [...walkNodes(doc)]
      .filter((l) => l.kind === 'control' && l.node.class === 'TTabSheet')
      .map((l) => l.node.name),
  );
  return Object.fromEntries(
    Object.entries(record)
      .filter(([name]) => !sheets.has(name))
      .map(([k, v]) => [k, [...v]]),
  );
}

function formSize(doc: BfmDocument) {
  const p = doc.form.properties ?? {};
  return { width: Number(p.Width), height: Number(p.Height) };
}

describe.each(fixtures)('$name', ({ name, document, record }) => {
  const shown = withBounds(document, toRects(record.shown));

  it('LCL が表示した配置は、計算し直しても変わらない', () => {
    expect(laidOut(undefined, shown)).toEqual(expected(record.shown, document));
  });

  it('フォームを広げた後の配置(Anchors・Align)が LCL と一致する', () => {
    const size = formSize(document);
    const resized = withBounds(shown, new Map(), {
      width: size.width + record.resize.width,
      height: size.height + record.resize.height,
    });
    expect(laidOut(shown, resized)).toEqual(expected(record.resized, document));
  });

  it.skipIf(INCONSISTENT.has(name))('書いた位置から計算した配置が LCL と一致する', () => {
    expect(laidOut(undefined, document)).toEqual(expected(record.shown, document));
  });
});
