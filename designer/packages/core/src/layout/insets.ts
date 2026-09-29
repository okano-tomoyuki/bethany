/**
 * コンテナのクライアント領域(docs/designer/editor-design.md §4.3。ADR 0036)。
 * 値は Windows 11(日本語版、Yu Gothic UI 9pt)で実物の LCL を表示して記録したもの(metrics.json。tools/layout/record.mts)。
 * TGroupBox の見出しの高さはフォントで変わる(記録したフォント以外ではずれる)。TTabControl はタブが 1 行あるときの値。
 */
import { findClass } from '../catalog/catalog.ts';
import metrics from './metrics.json' with { type: 'json' };

export interface Insets {
  readonly left: number;
  readonly top: number;
  readonly right: number;
  readonly bottom: number;
}

/** コンテナのクライアント領域の決まり */
export interface ClientMetrics {
  /** 子の座標の原点(子の Left・Top が 0 の位置)の、コンテナの外側の左上からの位置 */
  readonly origin: { readonly x: number; readonly y: number };
  /** Align で寄せる範囲の、コンテナの外側からの余白 */
  readonly insets: Insets;
}

interface RecordedInsets extends Insets {
  readonly origin: readonly number[];
}

const RECORDED = metrics.insets as Readonly<Record<string, RecordedInsets>>;
const NONE: ClientMetrics = {
  origin: { x: 0, y: 0 },
  insets: { left: 0, top: 0, right: 0, bottom: 0 },
};

/**
 * クラスのクライアント領域の決まり(余白の無いクラス・記録の無いクラスは余白 0、原点は外側の左上)。
 * properties があれば、枠・縁・BorderWidth による違いを加える(docs/adr/0048。規則は実物の LCL で測ったもの):
 * - 枠(BorderStyle が bsSingle)は Windows の枠で 2 ピクセル。子の座標の原点も余白も 2 ずれる。
 * - TPanel の縁(BevelOuter・BevelInner の bvNone でないもの 1 つにつき BevelWidth)と BorderWidth は、余白だけを広げる
 *   (子の座標の原点はパネルの外側の左上のまま)。
 */
export function clientMetrics(
  className: string,
  properties: Readonly<Record<string, unknown>> = {},
): ClientMetrics {
  const info = findClass(className);
  const prop = (name: string): unknown =>
    Object.hasOwn(properties, name) ? properties[name] : info?.properties[name]?.default;
  const num = (name: string): number => {
    const v = prop(name);
    return typeof v === 'number' ? v : 0;
  };
  const borderWidth = num('BorderWidth');
  if (className === 'TPanel' || className === 'TScrollBox') {
    const frame = prop('BorderStyle') === 'bsSingle' ? 2 : 0;
    const bevel =
      className === 'TPanel'
        ? (prop('BevelOuter') !== 'bvNone' ? num('BevelWidth') : 0) +
          (prop('BevelInner') !== 'bvNone' ? num('BevelWidth') : 0)
        : 0;
    const pad = frame + bevel + borderWidth;
    return {
      origin: { x: frame, y: frame },
      insets: { left: pad, top: pad, right: pad, bottom: pad },
    };
  }
  const recorded = Object.hasOwn(RECORDED, className) ? RECORDED[className] : undefined;
  const base: ClientMetrics = recorded
    ? {
        origin: { x: recorded.origin[0] ?? 0, y: recorded.origin[1] ?? 0 },
        insets: {
          left: recorded.left,
          top: recorded.top,
          right: recorded.right,
          bottom: recorded.bottom,
        },
      }
    : NONE;
  if (borderWidth === 0) return base;
  const { left, top, right, bottom } = base.insets;
  return {
    origin: base.origin,
    insets: {
      left: left + borderWidth,
      top: top + borderWidth,
      right: right + borderWidth,
      bottom: bottom + borderWidth,
    },
  };
}

/** Align で寄せる範囲の、外側からの余白(clientMetrics の insets) */
export function clientInsets(
  className: string,
  properties: Readonly<Record<string, unknown>> = {},
): Insets {
  return clientMetrics(className, properties).insets;
}

/** 生成しただけでは大きさが 0 のクラスの、親に置いたときの大きさ(TStatusBar の高さ) */
export function measuredSize(
  className: string,
): { readonly width?: number; readonly height?: number } | undefined {
  const sizes = metrics.sizes as Readonly<Record<string, { width?: number; height?: number }>>;
  return Object.hasOwn(sizes, className) ? sizes[className] : undefined;
}
