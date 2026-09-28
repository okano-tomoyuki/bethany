/**
 * フォームの定義ファイル(*.bfm.json)の構造のスキーマ(docs/designer/dsl-spec.md)。
 * 実装後はこのスキーマを正とし、dsl-spec.md と食い違う場合は dsl-spec.md を直す。
 *
 * ここでは形だけを検証する。クラス・プロパティ・イベントがカタログにあるか、値が型に合うか、名前の重複・参照は
 * validate.ts(意味の検証)で調べる。
 */
import * as z from 'zod';

export const FORMAT_VERSION = 1;

/** プロパティの値(型に合うかは意味の検証で調べる) */
export const PropertyValue = z.json();
export type PropertyValue = z.infer<typeof PropertyValue>;

/** プロパティ名 → 値 */
export const Properties = z.record(z.string(), PropertyValue);
export type Properties = z.infer<typeof Properties>;

/** イベント名 → ハンドラのメソッド名 */
export const Events = z.record(z.string(), z.string());
export type Events = z.infer<typeof Events>;

export interface ControlNode {
  readonly name: string;
  readonly class: string;
  readonly properties?: Properties | undefined;
  readonly events?: Events | undefined;
  /** Parent がこのコントロールのもの */
  readonly controls?: readonly ControlNode[] | undefined;
}

export const ControlNode: z.ZodType<ControlNode> = z.strictObject({
  name: z.string(),
  class: z.string(),
  properties: Properties.optional(),
  events: Events.optional(),
  get controls() {
    return z.array(ControlNode).optional();
  },
});

export interface MenuItemNode {
  readonly name: string;
  readonly properties?: Properties | undefined;
  readonly events?: Events | undefined;
  /** サブメニュー */
  readonly items?: readonly MenuItemNode[] | undefined;
}

export const MenuItemNode: z.ZodType<MenuItemNode> = z.strictObject({
  name: z.string(),
  properties: Properties.optional(),
  events: Events.optional(),
  get items() {
    return z.array(MenuItemNode).optional();
  },
});

/** デザイナーのキャンバス上のアイコンの位置(生成するコードには影響しない) */
export const DesignPosition = z.strictObject({ left: z.int(), top: z.int() });

export const ComponentNode = z.strictObject({
  name: z.string(),
  class: z.string(),
  design: DesignPosition.optional(),
  properties: Properties.optional(),
  events: Events.optional(),
  /** TMainMenu・TPopupMenu の項目 */
  items: z.array(MenuItemNode).optional(),
});
export type ComponentNode = z.infer<typeof ComponentNode>;

export const FormNode = z.strictObject({
  name: z.string(),
  class: z.literal('TForm'),
  properties: Properties.optional(),
  events: Events.optional(),
  controls: z.array(ControlNode).optional(),
});
export type FormNode = z.infer<typeof FormNode>;

export const CommentLocale = z.enum(['en', 'ja']);
export type CommentLocale = z.infer<typeof CommentLocale>;

/**
 * 以前のコード生成の設定(dsl-spec.md §9)。プロジェクトファイルに移したため使わない(読み込めるように残し、警告を出す)。
 */
export const CodegenSettings = z.strictObject({
  commentLocale: CommentLocale.optional(),
  cpp: z
    .strictObject({
      className: z.string().optional(),
      header: z.string().optional(),
      source: z.string().optional(),
    })
    .optional(),
  python: z
    .strictObject({
      className: z.string().optional(),
      file: z.string().optional(),
    })
    .optional(),
});
export type CodegenSettings = z.infer<typeof CodegenSettings>;

export const BfmDocument = z.strictObject({
  $schema: z.string().optional(),
  formatVersion: z.literal(FORMAT_VERSION),
  codegen: CodegenSettings.optional(),
  form: FormNode,
  /** 画面に出ないコンポーネント(TTimer・ダイアログ・メニュー・TImageList) */
  components: z.array(ComponentNode).optional(),
});
export type BfmDocument = z.infer<typeof BfmDocument>;
