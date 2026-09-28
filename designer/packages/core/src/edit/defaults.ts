/**
 * 追加したときの初期値(docs/designer/editor-design.md §3.2。C++Builder・Lazarus と同じ)。
 */
import { findClass } from '../catalog/catalog.ts';
import { measuredSize } from '../layout/insets.ts';
import type { Properties } from '../dsl/schema.ts';
import type { Bounds } from './commands.ts';

/** 追加したときに Caption を名前にするクラス(C++Builder と同じ。TEdit・TMemo 等の内容は空のまま) */
const CAPTION_CLASSES: ReadonlySet<string> = new Set([
  'TButton',
  'TBitBtn',
  'TToggleBox',
  'TLabel',
  'TStaticText',
  'TCheckBox',
  'TRadioButton',
  'TGroupBox',
  'TRadioGroup',
  'TCheckGroup',
  'TPanel',
  'TTabSheet',
  'TToolButton',
]);

/** 端に置くときの位置(配置の計算で親の端に寄せ直す) */
const FAR_EDGE = 10000;

/** 追加したときの既定の位置 */
export const DEFAULT_POSITION = { left: 8, top: 8 } as const;

/** 位置と大きさを書かないクラス(TTabSheet は親の TPageControl から決まる) */
export function hasOwnBounds(className: string): boolean {
  return className !== 'TTabSheet';
}

/** 既定の大きさ(カタログの defaultSize。無ければ 50×50) */
export function defaultSizeOf(className: string): { width: number; height: number } {
  const size = findClass(className)?.defaultSize ?? { width: 50, height: 50 };
  // 生成しただけでは 0 のもの(TStatusBar の高さ)は、親に置いて測った大きさ
  const measured = measuredSize(className);
  return {
    width: size.width > 0 ? size.width : (measured?.width ?? size.width),
    height: size.height > 0 ? size.height : (measured?.height ?? size.height),
  };
}

/** 追加したコントロールの初期のプロパティ */
export function initialProperties(
  className: string,
  name: string,
  bounds: Bounds | undefined,
): Properties {
  const properties: Record<string, Properties[string]> = {};
  if (hasOwnBounds(className)) {
    const size = defaultSizeOf(className);
    // 既定で下・右に寄せるクラス(TStatusBar)は、位置の指定が無ければ端に置く(寄せるときに既存のものより外側になる)
    const align = findClass(className)?.properties.Align?.default;
    properties.Left = bounds?.left ?? (align === 'alRight' ? FAR_EDGE : DEFAULT_POSITION.left);
    properties.Top = bounds?.top ?? (align === 'alBottom' ? FAR_EDGE : DEFAULT_POSITION.top);
    properties.Width = bounds?.width ?? size.width;
    properties.Height = bounds?.height ?? size.height;
  }
  if (CAPTION_CLASSES.has(className)) properties.Caption = name;
  return properties;
}

/** 既定のイベントが OnChange のクラス(C++Builder と同じ。値を入力・選択するもの) */
const CHANGE_CLASSES: ReadonlySet<string> = new Set([
  'TEdit',
  'TMaskEdit',
  'TLabeledEdit',
  'TSpinEdit',
  'TFloatSpinEdit',
  'TMemo',
  'TComboBox',
  'TTrackBar',
  'TScrollBar',
  'TPageControl',
  'TTabControl',
]);

/**
 * キャンバスでダブルクリックしたときに設定するイベント(docs/designer/editor-design.md §5.2。C++Builder と同じ)。
 * TForm は OnCreate、値を入力・選択するものは OnChange、TTimer は OnTimer、それ以外は OnClick(無ければ最初のイベント)。
 */
export function defaultEventOf(className: string): string | undefined {
  const events = Object.keys(findClass(className)?.events ?? {});
  const preferred =
    className === 'TForm'
      ? 'OnCreate'
      : CHANGE_CLASSES.has(className)
        ? 'OnChange'
        : className === 'TTimer'
          ? 'OnTimer'
          : 'OnClick';
  return events.includes(preferred) ? preferred : events[0];
}
