/**
 * コンテナのクライアント領域(docs/designer/editor-design.md §4.3。ADR 0036)。
 * 値は Windows 11(日本語版、Yu Gothic UI 9pt)で実物の LCL を表示して記録したもの(metrics.json。tools/layout/record.mts)。
 * TGroupBox の見出しの高さはフォントで変わる(記録したフォント以外ではずれる)。TTabControl はタブが 1 行あるときの値。
 */
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

/** クラスのクライアント領域の決まり(余白の無いクラス・記録の無いクラスは余白 0、原点は外側の左上) */
export function clientMetrics(className: string): ClientMetrics {
  const recorded = Object.hasOwn(RECORDED, className) ? RECORDED[className] : undefined;
  if (!recorded) return NONE;
  const { left, top, right, bottom } = recorded;
  return {
    origin: { x: recorded.origin[0] ?? 0, y: recorded.origin[1] ?? 0 },
    insets: { left, top, right, bottom },
  };
}

/** Align で寄せる範囲の、外側からの余白(clientMetrics の insets) */
export function clientInsets(className: string): Insets {
  return clientMetrics(className).insets;
}

/** 生成しただけでは大きさが 0 のクラスの、親に置いたときの大きさ(TStatusBar の高さ) */
export function measuredSize(
  className: string,
): { readonly width?: number; readonly height?: number } | undefined {
  const sizes = metrics.sizes as Readonly<Record<string, { width?: number; height?: number }>>;
  return Object.hasOwn(sizes, className) ? sizes[className] : undefined;
}
