/**
 * キャンバスの見た目の計算(docs/designer/editor-design.md §5.1): 色・フォントの解決(親からの継承を含む)と、
 * プロパティの読み取り。
 */
import {
  classOf,
  getCatalog,
  propertyValue,
  type CanvasSettings,
  type NodeLocation,
} from '@no-vcl-designer/core';

/** 解決したフォント(CSS に渡す形) */
export interface ResolvedFont {
  readonly family: string;
  /** ピクセル(96dpi) */
  readonly size: number;
  readonly color: string;
  readonly bold: boolean;
  readonly italic: boolean;
  readonly underline: boolean;
  readonly strikeOut: boolean;
}

/** 子に引き継ぐ見た目(ParentFont・ParentColor) */
export interface Inherited {
  readonly font: ResolvedFont;
  /** 背景色(CSS) */
  readonly color: string;
}

/** Windows のテーマの既定の色 */
export const SYSTEM = {
  face: '#f0f0f0',
  window: '#ffffff',
  windowText: '#000000',
  border: '#8a8a8a',
  grayText: '#6d6d6d',
} as const;

/** clDefault のときに白地になるクラス(編集欄・一覧) */
const WINDOW_CLASSES: ReadonlySet<string> = new Set([
  'TEdit',
  'TMaskEdit',
  'TLabeledEdit',
  'TSpinEdit',
  'TFloatSpinEdit',
  'TComboBox',
  'TMemo',
  'TListBox',
  'TCheckListBox',
  'TTreeView',
  'TListView',
  'TStringGrid',
  'TDrawGrid',
]);

export function rootInherited(settings: CanvasSettings): Inherited {
  return {
    font: {
      family: settings.fontFamily,
      size: (settings.fontSize * 96) / 72,
      color: SYSTEM.windowText,
      bold: false,
      italic: false,
      underline: false,
      strikeOut: false,
    },
    color: SYSTEM.face,
  };
}

/** TColor の値を CSS の色にする。clDefault・clNone・解釈できない値は undefined */
export function colorToCss(value: unknown): string | undefined {
  if (typeof value !== 'string') return undefined;
  if (/^#[0-9A-Fa-f]{6}$/.test(value)) return value;
  const constants = getCatalog().constants.TColor ?? {};
  if (value === 'clDefault' || value === 'clNone' || !Object.hasOwn(constants, value))
    return undefined;
  const bgr = constants[value] ?? 0;
  const r = bgr & 0xff;
  const g = (bgr >> 8) & 0xff;
  const b = (bgr >> 16) & 0xff;
  return `#${[r, g, b].map((c) => c.toString(16).padStart(2, '0')).join('')}`;
}

/** ノードの見た目(フォントと背景色)を、親から引き継いだものと合わせて決める */
export function resolveLook(location: NodeLocation, inherited: Inherited): Inherited {
  const own = location.node.properties ?? {};
  const className = classOf(location);

  // フォント: Font を書いたか ParentFont が false なら自分のフォント(書いていない項目は既定)
  const parentFont = propertyValue(location, ['ParentFont']) !== false && own.Font === undefined;
  const base = parentFont ? inherited.font : { ...inherited.font, color: SYSTEM.windowText };
  const font = parentFont ? base : fontOf(location, base);

  // 背景色: Color を書いていれば(ParentColor は LCL が false にする)その色。clDefault はクラスの既定
  const color = colorToCss(own.Color);
  const parentColor = propertyValue(location, ['ParentColor']) !== false && own.Color === undefined;
  const background =
    color ??
    (WINDOW_CLASSES.has(className)
      ? SYSTEM.window
      : parentColor || className === 'TForm'
        ? inherited.color
        : SYSTEM.face);
  return { font, color: background };
}

function fontOf(location: NodeLocation, base: ResolvedFont): ResolvedFont {
  const name = propertyValue(location, ['Font', 'Name']);
  const size = propertyValue(location, ['Font', 'Size']);
  const style = propertyValue(location, ['Font', 'Style']);
  const styles = Array.isArray(style) ? (style as unknown[]) : [];
  return {
    family: typeof name === 'string' && name !== '' && name !== 'default' ? name : base.family,
    size: typeof size === 'number' && size > 0 ? (size * 96) / 72 : base.size,
    color: colorToCss(propertyValue(location, ['Font', 'Color'])) ?? base.color,
    bold: styles.includes('fsBold'),
    italic: styles.includes('fsItalic'),
    underline: styles.includes('fsUnderline'),
    strikeOut: styles.includes('fsStrikeOut'),
  };
}

/** CSS の font 関連のスタイル */
export function fontStyle(font: ResolvedFont): Record<string, string | number> {
  const decorations = [font.underline && 'underline', font.strikeOut && 'line-through'].filter(
    Boolean,
  );
  return {
    fontFamily: `"${font.family}", "Segoe UI", sans-serif`,
    fontSize: `${String(font.size)}px`,
    fontWeight: font.bold ? 700 : 400,
    fontStyle: font.italic ? 'italic' : 'normal',
    textDecoration: decorations.length > 0 ? decorations.join(' ') : 'none',
    color: font.color,
  };
}

/** 文字列のプロパティ(書いていなければ既定値、それも無ければ空) */
export function text(location: NodeLocation, name: string): string {
  const value = propertyValue(location, [name]);
  return typeof value === 'string' ? value : '';
}

export function number(location: NodeLocation, name: string): number {
  const value = propertyValue(location, [name]);
  return typeof value === 'number' ? value : 0;
}

export function strings(location: NodeLocation, name: string): readonly string[] {
  const value = propertyValue(location, [name]);
  return Array.isArray(value) ? value.filter((v): v is string => typeof v === 'string') : [];
}

export function flag(location: NodeLocation, name: string): boolean {
  return propertyValue(location, [name]) === true;
}

/**
 * Caption の & を表示用に分ける(&File → F に下線、&& → &)。
 * @returns [前, 下線の文字, 後]。下線の文字が無ければ [全体, '', '']
 */
export function splitAccelerator(caption: string): readonly [string, string, string] {
  let before = '';
  for (let i = 0; i < caption.length; i++) {
    const ch = caption.charAt(i);
    if (ch !== '&') {
      before += ch;
      continue;
    }
    const next = caption.charAt(i + 1);
    if (next === '&') {
      before += '&';
      i++;
    } else if (next !== '') {
      return [before, next, caption.slice(i + 2).replace(/&&/g, '&')];
    }
  }
  return [before, '', ''];
}
