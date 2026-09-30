/**
 * TShape の描画(docs/adr/0058)。Shape の形を、Pen(縁の線)と Brush(中の塗りつぶし)で SVG に描く。
 * 形の求め方は LCL の TCustomShape.Paint に合わせる(線の幅の分だけ内側に描く・正方形や円は短い辺に合わせて中央に置く等)。
 */
import { propertyValue, type NodeLocation } from '@bethany-designer/core';
import { useId, type ReactNode } from 'react';
import { colorToCss } from './look.ts';

/** Windows の線の種類の破線(線の幅 1 のときの長さ。幅に合わせて伸ばす) */
const DASHES: Readonly<Record<string, readonly number[]>> = {
  psDash: [18, 6],
  psDot: [3, 3],
  psDashDot: [9, 6, 3, 6],
  psDashDotDot: [9, 3, 3, 3, 3, 3],
};

/** ハッチの模様の線(8 ピクセルの升目の中の線分) */
const HATCHES: Readonly<Record<string, readonly string[]>> = {
  bsHorizontal: ['M0 4H8'],
  bsVertical: ['M4 0V8'],
  bsFDiagonal: ['M0 0L8 8'],
  bsBDiagonal: ['M0 8L8 0'],
  bsCross: ['M0 4H8', 'M4 0V8'],
  bsDiagCross: ['M0 0L8 8', 'M0 8L8 0'],
};

const asString = (v: unknown, fallback: string): string => (typeof v === 'string' ? v : fallback);
const asNumber = (v: unknown, fallback: number): number => (typeof v === 'number' ? v : fallback);

export function Shape({
  location,
  width,
  height,
}: {
  readonly location: NodeLocation & { readonly kind: 'control' };
  readonly width: number;
  readonly height: number;
}): ReactNode {
  const patternId = useId();
  const value = (object: 'Pen' | 'Brush', name: string) => propertyValue(location, [object, name]);
  const shape = asString(propertyValue(location, ['Shape']), 'stRectangle');

  const penStyle = asString(value('Pen', 'Style'), 'psSolid');
  const penWidth = Math.max(1, asNumber(value('Pen', 'Width'), 1));
  const penColor = colorToCss(value('Pen', 'Color')) ?? '#000000';
  const brushStyle = asString(value('Brush', 'Style'), 'bsSolid');
  const brushColor = colorToCss(value('Brush', 'Color')) ?? '#ffffff';

  const stroke = penStyle === 'psClear' ? 'none' : penColor;
  const dash = DASHES[penStyle]?.map((d) => d * penWidth).join(' ');
  const hatch = HATCHES[brushStyle];
  const fill = brushStyle === 'bsClear' ? 'none' : hatch ? `url(#${patternId})` : brushColor;

  // 線の中心が通る矩形(線の幅の半分だけ内側)
  const inset = penWidth / 2;
  let left = inset;
  let top = inset;
  let right = width - inset;
  let bottom = height - inset;
  const size = Math.max(0, Math.min(right - left, bottom - top));
  if (['stSquare', 'stRoundSquare', 'stCircle', 'stSquaredDiamond'].includes(shape)) {
    left += (right - left - size) / 2;
    top += (bottom - top - size) / 2;
    right = left + size;
    bottom = top + size;
  }
  const cx = (left + right) / 2;
  const cy = (top + bottom) / 2;
  const w = Math.max(0, right - left);
  const h = Math.max(0, bottom - top);
  const polygon = (points: readonly (readonly [number, number])[]) => (
    <polygon points={points.map(([x, y]) => `${String(x)},${String(y)}`).join(' ')} />
  );

  let figure: ReactNode;
  switch (shape) {
    case 'stRoundRect':
    case 'stRoundSquare': {
      // LCL は RoundRect の角の楕円の幅・高さを短い辺の 1/4 にする(半径はその半分)
      const r = size / 8;
      figure = <rect x={left} y={top} width={w} height={h} rx={r} ry={r} />;
      break;
    }
    case 'stEllipse':
    case 'stCircle':
      figure = <ellipse cx={cx} cy={cy} rx={w / 2} ry={h / 2} />;
      break;
    case 'stDiamond':
    case 'stSquaredDiamond':
      figure = polygon([
        [cx, top],
        [right, cy],
        [cx, bottom],
        [left, cy],
      ]);
      break;
    case 'stTriangle':
      figure = polygon([
        [cx, top],
        [right, bottom],
        [left, bottom],
      ]);
      break;
    case 'stTriangleDown':
      figure = polygon([
        [left, top],
        [right, top],
        [cx, bottom],
      ]);
      break;
    case 'stTriangleLeft':
      figure = polygon([
        [left, cy],
        [right, top],
        [right, bottom],
      ]);
      break;
    case 'stTriangleRight':
      figure = polygon([
        [left, top],
        [right, cy],
        [left, bottom],
      ]);
      break;
    case 'stStar':
    case 'stStarDown': {
      // 5 つの角の星(内側の頂点は外側の 0.382 倍)。stStarDown は下向き
      const start = shape === 'stStar' ? -Math.PI / 2 : Math.PI / 2;
      const points = Array.from({ length: 10 }, (_, i) => {
        const ratio = i % 2 === 0 ? 1 : 0.382;
        const angle = start + (i * Math.PI) / 5;
        return [
          cx + (w / 2) * ratio * Math.cos(angle),
          cy + (h / 2) * ratio * Math.sin(angle),
        ] as const;
      });
      figure = polygon(points);
      break;
    }
    case 'stPolygon':
      // 頂点(Points)はデザイナーで設定しないため、何も描かない(LCL も頂点が無ければ描かない)
      figure = null;
      break;
    default:
      figure = <rect x={left} y={top} width={w} height={h} />;
  }

  return (
    <svg
      className={shape === 'stPolygon' ? 'look-shape empty' : 'look-shape'}
      width={width}
      height={height}
      viewBox={`0 0 ${String(width)} ${String(height)}`}
    >
      {hatch && (
        <defs>
          <pattern id={patternId} width={8} height={8} patternUnits="userSpaceOnUse">
            {hatch.map((d) => (
              <path key={d} d={d} stroke={brushColor} strokeWidth={1} />
            ))}
          </pattern>
        </defs>
      )}
      <g fill={fill} stroke={stroke} strokeWidth={penWidth} strokeDasharray={dash}>
        {figure}
      </g>
    </svg>
  );
}
