/** 実物の LCL で記録した配置(<見本>.lcl.json。tools/layout/record.mts が書く) */
export interface LayoutRecord {
  /** 表示した後の配置(name → [Left, Top, Width, Height]) */
  readonly shown: Readonly<Record<string, readonly number[]>>;
  /** フォームを広げた量 */
  readonly resize: { readonly width: number; readonly height: number };
  /** フォームを広げた後の配置 */
  readonly resized: Readonly<Record<string, readonly number[]>>;
}
