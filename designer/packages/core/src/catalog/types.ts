/**
 * コンポーネントカタログの型(docs/designer/catalog.md §4)。
 * catalog.json は designer/tools/catalog/extract.py が no_vcl.hpp・実測・overlay.json から作る。
 */

/** プロパティの型。DSL の値の書き方(docs/designer/dsl-spec.md §5)が決まる */
export type PropertyType =
  | { readonly kind: 'int' }
  | { readonly kind: 'float' }
  | { readonly kind: 'bool' }
  | { readonly kind: 'string' }
  | { readonly kind: 'char' }
  /** 整数の別名(TColor・TCursor・TShortCut)。定数は Catalog.constants */
  | { readonly kind: 'alias'; readonly alias: string }
  | { readonly kind: 'enum'; readonly enum: string }
  /** ビット集合(TFontStyles・TOpenOptions 等。要素は Catalog.flags) */
  | { readonly kind: 'flags'; readonly flags: string }
  /** Set<E>(TAnchors。要素は Catalog.enums[enum]) */
  | { readonly kind: 'set'; readonly enum: string }
  /** 入れ子のオブジェクト(TFont・TSizeConstraints・TControlBorderSpacing) */
  | { readonly kind: 'object'; readonly class: string }
  | { readonly kind: 'strings' }
  /** 同じフォームのコンポーネントへの参照 */
  | { readonly kind: 'ref'; readonly class: string };

export interface PropertyInfo {
  readonly type: PropertyType;
  /** 宣言したクラス */
  readonly declaredIn: string;
  /** 既定値(DSL の書き方。実測したもの) */
  readonly default?: unknown;
  readonly doc?: string;
}

export interface EventInfo {
  /** イベントの型(Catalog.eventTypes のキー) */
  readonly type: string;
  readonly declaredIn: string;
  readonly doc?: string;
}

export type ClassKind = 'form' | 'control' | 'component' | 'menuItem';

export interface ClassInfo {
  /** 基底クラス(近い順。TObject まで) */
  readonly ancestors: readonly string[];
  readonly kind: ClassKind;
  /** パレットの分類。無ければパレットに出さない(TTabSheet・TToolButton・TMenuItem) */
  readonly palette?: string;
  /** 子のコントロールを置けるか */
  readonly acceptsControls: boolean;
  /** 子に置けるクラス(そのクラスか派生)。無ければ制限しない */
  readonly childClasses?: readonly string[];
  /** 置ける親のクラス(そのクラスか派生)。無ければ制限しない */
  readonly parentClasses?: readonly string[];
  readonly defaultSize?: { readonly width: number; readonly height: number };
  /** デザイン時に設定できるプロパティ(Left・Top・Width・Height が先頭。順はコード生成の順) */
  readonly properties: Readonly<Record<string, PropertyInfo>>;
  readonly events: Readonly<Record<string, EventInfo>>;
}

export interface EventParam {
  readonly name: string;
  /** no_vcl.hpp での型(例: "TObject*"・"bool&") */
  readonly type: string;
}

export interface Catalog {
  /** TForm の public なメンバの名前(生成するクラスのメンバ名と衝突してはならない) */
  readonly formMembers: readonly string[];
  readonly classes: Readonly<Record<string, ClassInfo>>;
  readonly objects: Readonly<
    Record<string, { readonly properties: Readonly<Record<string, PropertyInfo>> }>
  >;
  readonly enums: Readonly<Record<string, readonly string[]>>;
  readonly flags: Readonly<Record<string, readonly string[]>>;
  /** Set<E> の型名 → 要素の列挙型 */
  readonly sets: Readonly<Record<string, string>>;
  /** 整数の別名 → 定数名 → 値 */
  readonly constants: Readonly<Record<string, Readonly<Record<string, number>>>>;
  readonly eventTypes: Readonly<Record<string, readonly EventParam[]>>;
  readonly palette: readonly string[];
}
