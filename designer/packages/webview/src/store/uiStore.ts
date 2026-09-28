/**
 * UI ストア(tk-designer ADR 0006)。選択・表示の切り替え・キャンバスの設定など、Webview 内で閉じる一時的な状態を持つ。
 * ドキュメントには保存されない。
 */
import type { CanvasSettings } from '@no-vcl-designer/core';
import { createStore } from 'zustand/vanilla';

/** 表示中の画面。デザイナー(キャンバス)とコード生成の設定 */
export type View = 'design' | 'codegen';

export type InspectorTab = 'properties' | 'events';

export const ZOOM_LEVELS = [0.5, 0.75, 1, 1.25, 1.5, 2] as const;

export interface UiState {
  readonly view: View;
  /**
   * 選択中のノードの name(複数選択では同じ親の兄弟)。先頭がインスペクタの基準。
   * 削除・改名で存在しなくなったものは、参照側で除いて扱う
   */
  readonly selection: readonly string[];
  readonly inspectorTab: InspectorTab;
  /** パレットで選んだクラス(次にキャンバスをクリック・ドラッグしたときに追加する) */
  readonly tool: string | undefined;
  readonly zoom: number;
  readonly settings: CanvasSettings;
  /** キャンバスで表示している TPageControl のタブ(TPageControl の name → TTabSheet の name) */
  readonly shownPages: Readonly<Record<string, string>>;
}

export interface UiActions {
  readonly setView: (view: View) => void;
  readonly select: (names: readonly string[]) => void;
  readonly setInspectorTab: (tab: InspectorTab) => void;
  readonly setTool: (tool: string | undefined) => void;
  readonly setZoom: (zoom: number) => void;
  readonly receiveSettings: (settings: CanvasSettings) => void;
  readonly showPage: (pageControl: string, sheet: string) => void;
}

export type UiStore = ReturnType<typeof createUiStore>;

export function createUiStore() {
  return createStore<UiState & UiActions>()((set, get) => ({
    view: 'design',
    selection: [],
    inspectorTab: 'properties',
    tool: undefined,
    zoom: 1,
    settings: { fontFamily: 'Segoe UI', fontSize: 9, gridSize: 8 },
    shownPages: {},
    setView(view) {
      set({ view });
    },
    select(names) {
      set({ selection: [...new Set(names)] });
    },
    setInspectorTab(inspectorTab) {
      set({ inspectorTab });
    },
    setTool(tool) {
      set({ tool });
    },
    setZoom(zoom) {
      set({ zoom });
    },
    receiveSettings(settings) {
      set({ settings });
    },
    showPage(pageControl, sheet) {
      set({ shownPages: { ...get().shownPages, [pageControl]: sheet } });
    },
  }));
}
