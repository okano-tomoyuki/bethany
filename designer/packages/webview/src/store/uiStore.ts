/**
 * UI ストア(tk-designer ADR 0006)。選択・表示の切り替え・キャンバスの設定など、Webview 内で閉じる一時的な状態を持つ。
 * ドキュメントには保存されない。
 */
import type { CanvasSettings } from '@bethany-designer/core';
import { createStore } from 'zustand/vanilla';

export type InspectorTab = 'properties' | 'events';

export const ZOOM_LEVELS = [0.5, 0.75, 1, 1.25, 1.5, 2] as const;

/** 倍率を 1 段階上げる(step = 1)・下げる(step = -1)。端ではそのまま */
export function stepZoom(zoom: number, step: 1 | -1): number {
  const next =
    step > 0 ? ZOOM_LEVELS.find((z) => z > zoom) : [...ZOOM_LEVELS].reverse().find((z) => z < zoom);
  return next ?? zoom;
}

export interface UiState {
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
  /** 右クリックのメニューを開いている位置(画面の座標) */
  readonly contextMenu: { readonly x: number; readonly y: number } | undefined;
}

export interface UiActions {
  readonly select: (names: readonly string[]) => void;
  readonly setInspectorTab: (tab: InspectorTab) => void;
  readonly setTool: (tool: string | undefined) => void;
  readonly setZoom: (zoom: number) => void;
  readonly receiveSettings: (settings: CanvasSettings) => void;
  readonly showPage: (pageControl: string, sheet: string) => void;
  readonly openContextMenu: (at: { x: number; y: number } | undefined) => void;
}

export type UiStore = ReturnType<typeof createUiStore>;

export function createUiStore() {
  return createStore<UiState & UiActions>()((set, get) => ({
    selection: [],
    inspectorTab: 'properties',
    tool: undefined,
    zoom: 1,
    settings: { fontFamily: 'Segoe UI', fontSize: 9, gridSize: 8, showGrid: true },
    shownPages: {},
    contextMenu: undefined,
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
    openContextMenu(at) {
      set({ contextMenu: at });
    },
  }));
}
