/**
 * コンテナのクライアント領域の余白(docs/designer/editor-design.md §4.3。ADR 0036)。
 * 子の Left・Top は、親のクライアント領域の左上からの位置になる。
 *
 * 暫定の値: TPageControl は Windows 11(日本語版、Yu Gothic UI 9pt)で実測した値(TTabSheet に alClient で置いた TMemo が
 * 363×120 の TPageControl の中で 355×92)。それ以外は見積もり。§4.3 の記録(record-insets)で置き換える。
 */

export interface Insets {
  readonly left: number;
  readonly top: number;
  readonly right: number;
  readonly bottom: number;
}

const NONE: Insets = { left: 0, top: 0, right: 0, bottom: 0 };

const INSETS: Readonly<Record<string, Insets>> = {
  TPanel: { left: 1, top: 1, right: 1, bottom: 1 },
  TGroupBox: { left: 2, top: 17, right: 2, bottom: 2 },
  TRadioGroup: { left: 2, top: 17, right: 2, bottom: 2 },
  TCheckGroup: { left: 2, top: 17, right: 2, bottom: 2 },
  TPageControl: { left: 4, top: 24, right: 4, bottom: 4 },
  TTabControl: { left: 4, top: 24, right: 4, bottom: 4 },
  TScrollBox: { left: 2, top: 2, right: 2, bottom: 2 },
};

/** クラスのクライアント領域の余白(余白の無いクラス・カタログに無いクラスは 0) */
export function clientInsets(className: string): Insets {
  return Object.hasOwn(INSETS, className) ? (INSETS[className] ?? NONE) : NONE;
}
