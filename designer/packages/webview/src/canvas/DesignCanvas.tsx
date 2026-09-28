/**
 * キャンバス(docs/designer/editor-design.md §5)。フォームの枠・タイトルバー・メインメニューと、クライアント領域の
 * コントロール・非ビジュアルコンポーネントのアイコンを描き、選択・移動・大きさの変更・追加を行う。
 *
 * 座標: キャンバスの座標は 96dpi の論理ピクセル。倍率は枠全体に transform: scale で掛ける。
 * ポインタの位置は、対象のクライアント領域(data-client)の画面上の位置からの差を倍率で割って求める。
 */
import {
  childProblem,
  classOf,
  defaultEventOf,
  defaultSizeOf,
  findClass,
  findNode,
  hasOwnBounds,
  l10n,
  propertyValue,
  walkNodes,
  type Bounds,
  type ComponentNode,
  type ControlNode,
  type MenuItemNode,
  type NodeLocation,
  type NvformDocument,
} from '@no-vcl-designer/core';
import {
  useEffect,
  useMemo,
  useRef,
  useState,
  type MouseEvent as ReactMouseEvent,
  type PointerEvent as ReactPointerEvent,
} from 'react';
import { useShallow } from 'zustand/shallow';
import {
  acceptsControls,
  addComponentAt,
  addControlAt,
  boundsOf,
  select,
  setBounds,
  setDefaultHandler,
  setDesignPosition,
  setFormSize,
  type BoundsChange,
} from '../editing.ts';
import { uiStore, useDocumentStore, useUiStore } from '../store/stores.ts';
import { ZOOM_LEVELS } from '../store/uiStore.ts';
import { Caption, CanvasContext, Children, type CanvasState } from './ControlView.tsx';
import { fontStyle, resolveLook, rootInherited, text } from './look.ts';

/** 枠(描画上の太さ)・タイトルバー・メニューバーの高さ(Windows 11 の見た目の近似) */
const FRAME = 1;
const TITLE_HEIGHT = 31;
const MENU_HEIGHT = 20;
/** 非ビジュアルコンポーネントのアイコンの大きさ */
const ICON_SIZE = 28;
/** これより小さいポインタの移動はクリックとみなす */
const CLICK_SLOP = 3;

type Handle = 'n' | 's' | 'e' | 'w' | 'ne' | 'nw' | 'se' | 'sw';
const HANDLES: readonly Handle[] = ['nw', 'n', 'ne', 'e', 'se', 's', 'sw', 'w'];
/** フォームは右と下の端だけで大きさを変える(左上はクライアント領域の原点) */
const FORM_HANDLES: readonly Handle[] = ['e', 'se', 's'];

type Drag =
  | {
      readonly kind: 'move';
      readonly names: readonly string[];
      readonly parent: string;
      readonly start: Point;
      /** 移すコントロールの元の位置(親のクライアント領域)と、画面上の左上 */
      readonly origin: ReadonlyMap<string, { bounds: Bounds; screen: Point }>;
    }
  | {
      readonly kind: 'resize';
      readonly name: string;
      readonly handle: Handle;
      readonly start: Point;
      readonly origin: Bounds;
    }
  | {
      readonly kind: 'create';
      readonly className: string;
      readonly parent: string;
      readonly start: Point;
    }
  | { readonly kind: 'band'; readonly parent: string; readonly start: Point }
  | { readonly kind: 'icon'; readonly name: string; readonly start: Point; readonly origin: Point };

interface Point {
  readonly x: number;
  readonly y: number;
}

/** ドラッグ中の表示(確定前の一時的な状態) */
interface Preview {
  readonly bounds: ReadonlyMap<string, Bounds>;
  readonly icons: ReadonlyMap<string, Point>;
  readonly dropTarget: string | undefined;
  /** 範囲選択・追加の範囲(parent のクライアント領域の座標) */
  readonly rect: { readonly parent: string; readonly bounds: Bounds } | undefined;
}

const EMPTY_PREVIEW: Preview = {
  bounds: new Map(),
  icons: new Map(),
  dropTarget: undefined,
  rect: undefined,
};

export function DesignCanvas() {
  const document = useDocumentStore((s) => s.document);
  const selection = useUiStore(useShallow((s) => s.selection));
  const zoom = useUiStore((s) => s.zoom);
  const tool = useUiStore((s) => s.tool);
  const settings = useUiStore((s) => s.settings);
  const shownPages = useUiStore((s) => s.shownPages);
  const [preview, setPreview] = useState<Preview>(EMPTY_PREVIEW);
  const drag = useRef<Drag | undefined>(undefined);
  const stage = useRef<HTMLDivElement>(null);

  // 選択したノードが隠れたタブの中にあれば、そのタブを表示する
  useEffect(() => {
    if (!document) return;
    const parents = parentMap(document);
    for (const name of selection) {
      for (
        let child = name, parent = parents.get(name);
        parent;
        child = parent.name, parent = parents.get(parent.name)
      ) {
        if (parent.class === 'TPageControl' && uiStore.getState().shownPages[parent.name] !== child)
          uiStore.getState().showPage(parent.name, child);
      }
    }
  }, [document, selection]);

  const canvasState = useMemo<CanvasState>(
    () => ({
      shownPage: (pageControl: ControlNode) => {
        const sheets = (pageControl.controls ?? []).filter((c) => c.class === 'TTabSheet');
        const wanted = shownPages[pageControl.name] ?? pageControl.properties?.ActivePage;
        return sheets.find((s) => s.name === wanted)?.name ?? sheets[0]?.name;
      },
      preview: preview.bounds,
      dropTarget: preview.dropTarget,
    }),
    [shownPages, preview],
  );

  if (!document) return null;
  const form = document.form;
  const formLocation: NodeLocation = { kind: 'form', node: form, path: ['form'] };
  const formLook = resolveLook(formLocation, rootInherited(settings));
  // フォームの大きさの変更中は、仮の大きさで描く
  const formPreview = preview.bounds.get(form.name);
  const width = formPreview?.width ?? numberOr(propertyValue(formLocation, ['Width']), 320);
  const height = formPreview?.height ?? numberOr(propertyValue(formLocation, ['Height']), 240);
  const menu = mainMenuOf(document);
  const outerWidth = width + FRAME * 2;
  const outerHeight = height + FRAME * 2 + TITLE_HEIGHT + (menu ? MENU_HEIGHT : 0);
  const selected = new Set(selection);
  // メニュー項目(とメニュー)を選んでいれば、その項目までのドロップダウンを開く
  const opened = openedMenu(document, selection[0]);
  const iconAt = (index: number): Point => {
    const component = document.components?.[index];
    return (
      (component && preview.icons.get(component.name)) ?? {
        x: component?.design?.left ?? 8 + index * (ICON_SIZE + 8),
        y: component?.design?.top ?? height - ICON_SIZE - 20,
      }
    );
  };

  // ---- 座標 ----

  const clientElement = (name: string) =>
    stage.current?.querySelector<HTMLElement>(`[data-client="${CSS.escape(name)}"]`) ?? null;

  /** 画面上の点を、name のクライアント領域の座標にする */
  const toClient = (name: string, x: number, y: number): Point => {
    const rect = clientElement(name)?.getBoundingClientRect();
    return rect ? { x: (x - rect.left) / zoom, y: (y - rect.top) / zoom } : { x: 0, y: 0 };
  };

  const snap = (value: number, e: { altKey: boolean }) => {
    const grid = settings.gridSize;
    return grid > 0 && !e.altKey ? Math.round(value / grid) * grid : Math.round(value);
  };

  /** ポインタの下の、className を置けるいちばん内側のコンテナ(except とその子孫は除く) */
  const containerAt = (x: number, y: number, className: string, except: readonly string[]) => {
    for (const element of globalThis.document.elementsFromPoint(x, y)) {
      if (!(element instanceof HTMLElement)) continue;
      const name = element.dataset.client;
      if (name === undefined) continue;
      if (except.some((n) => element.closest(`[data-node="${CSS.escape(n)}"]`))) continue;
      const location = findNode(document, name);
      if (!location || !acceptsControls(location)) continue;
      if (childProblem(classOf(location), className)) continue;
      return name;
    }
    return undefined;
  };

  // ---- ポインタの操作 ----

  const onPointerDown = (e: ReactPointerEvent<HTMLDivElement>) => {
    if (e.button !== 0) return;
    const target = e.target as HTMLElement;
    const start = { x: e.clientX, y: e.clientY };
    const capture = () => {
      e.currentTarget.setPointerCapture(e.pointerId);
    };

    // パレットで選んだクラスの追加
    if (tool !== undefined) {
      const info = findClass(tool);
      if (info?.kind === 'component') {
        drag.current = { kind: 'create', className: tool, parent: form.name, start };
      } else {
        const parent = containerAt(e.clientX, e.clientY, tool, []);
        if (parent === undefined) return;
        drag.current = { kind: 'create', className: tool, parent, start };
      }
      capture();
      return;
    }

    const tab = target.closest<HTMLElement>('[data-tab]');
    if (tab?.dataset.tab !== undefined && tab.dataset.pageControl !== undefined) {
      uiStore.getState().showPage(tab.dataset.pageControl, tab.dataset.tab);
      select([tab.dataset.tab]);
      return;
    }

    const menuItem = target.closest<HTMLElement>('[data-menu-item]')?.dataset.menuItem;
    if (menuItem !== undefined) {
      select([menuItem]);
      return;
    }

    const handle = target.closest<HTMLElement>('[data-handle]');
    const handleOwner = handle?.closest<HTMLElement>('[data-frame]')?.dataset.frame;
    if (handle && handleOwner !== undefined) {
      const location = findNode(document, handleOwner);
      if (location?.kind === 'form') {
        drag.current = {
          kind: 'resize',
          name: handleOwner,
          handle: handle.dataset.handle as Handle,
          start,
          origin: { left: 0, top: 0, width, height },
        };
        capture();
      } else if (location) {
        drag.current = {
          kind: 'resize',
          name: handleOwner,
          handle: handle.dataset.handle as Handle,
          start,
          origin: boundsOf(location),
        };
        capture();
      }
      return;
    }

    const icon = target.closest<HTMLElement>('[data-icon]')?.dataset.icon;
    if (icon !== undefined) {
      const location = findNode(document, icon);
      if (location?.kind !== 'component') return;
      toggleOrSelect(icon, e);
      const origin = location.node.design ?? { left: 0, top: 0 };
      drag.current = { kind: 'icon', name: icon, start, origin: { x: origin.left, y: origin.top } };
      capture();
      return;
    }

    const name = target.closest<HTMLElement>('[data-node]')?.dataset.node;
    const location = name === undefined ? undefined : findNode(document, name);
    if (!location || location.kind !== 'control') {
      // フォームの背景: フォームを選択し、範囲選択を始める
      select([form.name]);
      drag.current = { kind: 'band', parent: form.name, start };
      capture();
      return;
    }

    const names = toggleOrSelect(location.node.name, e);
    const moving = names
      .map((n) => findNode(document, n))
      .filter(
        (l): l is NodeLocation & { kind: 'control' } =>
          l?.kind === 'control' && l.parent === location.parent && hasOwnBounds(l.node.class),
      );
    if (moving.length === 0 || !moving.some((l) => l.node.name === location.node.name)) return;
    const origin = new Map(
      moving.map((l) => {
        const rect = stage.current
          ?.querySelector(`[data-node="${CSS.escape(l.node.name)}"]`)
          ?.getBoundingClientRect();
        return [
          l.node.name,
          { bounds: boundsOf(l), screen: { x: rect?.left ?? 0, y: rect?.top ?? 0 } },
        ];
      }),
    );
    drag.current = {
      kind: 'move',
      names: moving.map((l) => l.node.name),
      parent: location.parent.name,
      start,
      origin,
    };
    capture();
  };

  /** Shift・Ctrl なら選択に加える・外す(同じ親の兄弟だけ)。それ以外は、選択に無ければ選び直す */
  const toggleOrSelect = (name: string, e: { shiftKey: boolean; ctrlKey: boolean }) => {
    const current = uiStore.getState().selection;
    if (e.shiftKey || e.ctrlKey) {
      const next = current.includes(name)
        ? current.filter((n) => n !== name)
        : [...current.filter((n) => sameParent(document, n, name)), name];
      select(next);
      return next;
    }
    if (current.includes(name)) return current;
    select([name]);
    return [name];
  };

  const onPointerMove = (e: ReactPointerEvent<HTMLDivElement>) => {
    const d = drag.current;
    if (!d) return;
    const dx = (e.clientX - d.start.x) / zoom;
    const dy = (e.clientY - d.start.y) / zoom;
    const moved = Math.hypot(e.clientX - d.start.x, e.clientY - d.start.y) >= CLICK_SLOP;
    switch (d.kind) {
      case 'move': {
        if (!moved) return;
        const next = moveResult(d, e);
        setPreview({
          ...EMPTY_PREVIEW,
          bounds: next.preview,
          dropTarget: next.parent !== d.parent ? next.parent : undefined,
        });
        return;
      }
      case 'resize':
        setPreview({ ...EMPTY_PREVIEW, bounds: new Map([[d.name, resized(d, dx, dy, e)]]) });
        return;
      case 'icon':
        if (!moved) return;
        setPreview({
          ...EMPTY_PREVIEW,
          icons: new Map([
            [
              d.name,
              {
                x: Math.max(0, snap(d.origin.x + dx, e)),
                y: Math.max(0, snap(d.origin.y + dy, e)),
              },
            ],
          ]),
        });
        return;
      case 'create':
      case 'band':
        if (!moved) return;
        setPreview({ ...EMPTY_PREVIEW, rect: { parent: d.parent, bounds: rectOf(d, e) } });
        return;
    }
  };

  const onPointerUp = (e: ReactPointerEvent<HTMLDivElement>) => {
    const d = drag.current;
    drag.current = undefined;
    setPreview(EMPTY_PREVIEW);
    if (!d) return;
    const moved = Math.hypot(e.clientX - d.start.x, e.clientY - d.start.y) >= CLICK_SLOP;
    switch (d.kind) {
      case 'move': {
        if (!moved) return;
        const next = moveResult(d, e);
        const changes: BoundsChange[] = [...next.bounds].map(([name, bounds]) => ({
          name,
          bounds,
        }));
        setBounds(changes, next.parent !== d.parent ? { name: next.parent } : undefined);
        return;
      }
      case 'resize':
        if (d.name === form.name) {
          const r = resized(d, (e.clientX - d.start.x) / zoom, (e.clientY - d.start.y) / zoom, e);
          setFormSize(r.width, r.height);
          return;
        }
        setBounds([
          {
            name: d.name,
            bounds: resized(d, (e.clientX - d.start.x) / zoom, (e.clientY - d.start.y) / zoom, e),
          },
        ]);
        return;
      case 'icon': {
        if (!moved) return;
        const x = Math.max(0, snap(d.origin.x + (e.clientX - d.start.x) / zoom, e));
        const y = Math.max(0, snap(d.origin.y + (e.clientY - d.start.y) / zoom, e));
        setDesignPosition(d.name, x, y);
        return;
      }
      case 'create': {
        uiStore.getState().setTool(undefined);
        const rect = rectOf(d, e);
        if (findClass(d.className)?.kind === 'component') {
          addComponentAt(d.className, rect.left, rect.top);
          return;
        }
        const size = defaultSizeOf(d.className);
        addControlAt(
          d.className,
          d.parent,
          moved ? rect : { left: rect.left, top: rect.top, width: size.width, height: size.height },
        );
        return;
      }
      case 'band': {
        if (!moved) return;
        const rect = rectOf(d, e);
        const hit = (form.controls ?? []).filter((c) => {
          const b = boundsOf({ kind: 'control', node: c, parent: form, path: [] });
          return (
            b.left < rect.left + rect.width &&
            rect.left < b.left + b.width &&
            b.top < rect.top + rect.height &&
            rect.top < b.top + b.height
          );
        });
        select(hit.length > 0 ? hit.map((c) => c.name) : [form.name]);
        return;
      }
    }
  };

  /**
   * 移動の結果: 移し先の親と、各コントロールの位置。
   * preview は元の親の座標での表示用の位置、bounds は移し先の親のクライアント領域での位置
   */
  const moveResult = (
    d: Drag & { kind: 'move' },
    e: { clientX: number; clientY: number; altKey: boolean },
  ) => {
    const classes = d.names.flatMap((n) => {
      const l = findNode(document, n);
      return l ? [classOf(l)] : [];
    });
    const candidate = containerAt(e.clientX, e.clientY, classes[0] ?? '', d.names);
    const candidateLocation = candidate === undefined ? undefined : findNode(document, candidate);
    const parent =
      candidateLocation && classes.every((c) => !childProblem(classOf(candidateLocation), c))
        ? candidateLocation.node.name
        : d.parent;
    const dx = e.clientX - d.start.x;
    const dy = e.clientY - d.start.y;
    // 先頭のコントロールの左上を格子に合わせ、他は同じだけ動かす
    const layout = (place: (o: { bounds: Bounds; screen: Point }) => Point) => {
      const [first] = d.origin.values();
      const at = first ? place(first) : { x: 0, y: 0 };
      const offset = { x: snap(at.x, e) - at.x, y: snap(at.y, e) - at.y };
      const result = new Map<string, Bounds>();
      for (const [name, o] of d.origin) {
        const p = place(o);
        result.set(name, {
          left: Math.round(p.x + offset.x),
          top: Math.round(p.y + offset.y),
          width: o.bounds.width,
          height: o.bounds.height,
        });
      }
      return result;
    };
    const preview = layout((o) => ({ x: o.bounds.left + dx / zoom, y: o.bounds.top + dy / zoom }));
    const bounds =
      parent === d.parent
        ? preview
        : layout((o) => toClient(parent, o.screen.x + dx, o.screen.y + dy));
    return { parent, preview, bounds };
  };

  const resized = (
    d: Drag & { kind: 'resize' },
    dx: number,
    dy: number,
    e: { altKey: boolean },
  ): Bounds => {
    const o = d.origin;
    let left = o.left;
    let top = o.top;
    let right = o.left + o.width;
    let bottom = o.top + o.height;
    if (d.handle.includes('w')) left = Math.min(snap(o.left + dx, e), right - 1);
    if (d.handle.includes('e')) right = Math.max(snap(right + dx, e), left + 1);
    if (d.handle.includes('n')) top = Math.min(snap(o.top + dy, e), bottom - 1);
    if (d.handle.includes('s')) bottom = Math.max(snap(bottom + dy, e), top + 1);
    return { left, top, width: right - left, height: bottom - top };
  };

  const rectOf = (
    d: Drag & { kind: 'create' | 'band' },
    e: { clientX: number; clientY: number; altKey: boolean },
  ): Bounds => {
    const a = toClient(d.parent, d.start.x, d.start.y);
    const b = toClient(d.parent, e.clientX, e.clientY);
    const left = snap(Math.min(a.x, b.x), e);
    const top = snap(Math.min(a.y, b.y), e);
    return {
      left,
      top,
      width: Math.max(1, snap(Math.max(a.x, b.x), e) - left),
      height: Math.max(1, snap(Math.max(a.y, b.y), e) - top),
    };
  };

  const onDoubleClick = (e: ReactMouseEvent) => {
    const target = e.target as HTMLElement;
    const name =
      target.closest<HTMLElement>('[data-icon]')?.dataset.icon ??
      target.closest<HTMLElement>('[data-menu-item]')?.dataset.menuItem ??
      target.closest<HTMLElement>('[data-node]')?.dataset.node ??
      form.name;
    const location = findNode(document, name);
    const event = location ? defaultEventOf(classOf(location)) : undefined;
    if (!location || event === undefined) return;
    setDefaultHandler(location, event);
    uiStore.getState().setInspectorTab('events');
  };

  // 右クリック: 指したものが選択に無ければ選び直してから、メニューを開く
  const onContextMenu = (e: ReactMouseEvent) => {
    e.preventDefault();
    const target = e.target as HTMLElement;
    const name =
      target.closest<HTMLElement>('[data-icon]')?.dataset.icon ??
      target.closest<HTMLElement>('[data-menu-item]')?.dataset.menuItem ??
      target.closest<HTMLElement>('[data-tab]')?.dataset.tab ??
      target.closest<HTMLElement>('[data-frame]')?.dataset.frame ??
      target.closest<HTMLElement>('[data-node]')?.dataset.node ??
      form.name;
    if (!uiStore.getState().selection.includes(name)) select([name]);
    uiStore.getState().openContextMenu({ x: e.clientX, y: e.clientY });
  };

  const rect = preview.rect;

  return (
    <section className="panel canvas-panel" aria-label={l10n.t('Canvas')}>
      <div className="canvas-toolbar">
        {tool !== undefined && (
          <span className="canvas-tool">
            {l10n.t('Click or drag on the form to add {0} (Esc to cancel)', tool)}
          </span>
        )}
        <label>
          {l10n.t('Zoom')}{' '}
          <select
            value={zoom}
            onChange={(e) => {
              uiStore.getState().setZoom(Number(e.target.value));
            }}
          >
            {ZOOM_LEVELS.map((z) => (
              <option key={z} value={z}>
                {Math.round(z * 100)}%
              </option>
            ))}
          </select>
        </label>
      </div>
      <div className="canvas-scroll">
        <div
          className="canvas-stage"
          style={{ width: outerWidth * zoom, height: outerHeight * zoom }}
        >
          <div
            ref={stage}
            className={tool === undefined ? 'form-window' : 'form-window placing'}
            style={{ width: outerWidth, height: outerHeight, transform: `scale(${String(zoom)})` }}
            onPointerDown={onPointerDown}
            onPointerMove={onPointerMove}
            onPointerUp={onPointerUp}
            onPointerCancel={() => {
              drag.current = undefined;
              setPreview(EMPTY_PREVIEW);
            }}
            onDoubleClick={onDoubleClick}
            onContextMenu={onContextMenu}
          >
            <div className={selected.has(form.name) ? 'form-title selected' : 'form-title'}>
              <span className="form-title-text">{text(formLocation, 'Caption')}</span>
              <span className="form-title-buttons">─ ☐ ✕</span>
            </div>
            {menu && (
              <div className="form-menu" style={fontStyle(formLook.font)}>
                {(menu.items ?? []).map((item) => (
                  <span
                    key={item.name}
                    data-menu-item={item.name}
                    className={
                      selected.has(item.name) ? 'form-menu-item selected' : 'form-menu-item'
                    }
                  >
                    <Caption
                      value={
                        typeof item.properties?.Caption === 'string' ? item.properties.Caption : ''
                      }
                    />
                    {opened?.menu === menu && opened.open.has(item.name) && (
                      <MenuList items={item.items ?? []} opened={opened.open} selected={selected} />
                    )}
                  </span>
                ))}
              </div>
            )}
            <div
              className="form-client"
              data-client={form.name}
              style={{ width, height, background: formLook.color, ...fontStyle(formLook.font) }}
            >
              <CanvasContext.Provider value={canvasState}>
                <Children parent={form} path={['form']} inherited={formLook} />
              </CanvasContext.Provider>
              <SelectionOverlay
                document={document}
                selection={selection}
                preview={preview.bounds}
                form={{ width, height }}
              />
              {(document.components ?? []).map((component, i) => {
                const at = iconAt(i);
                return (
                  <div
                    key={component.name}
                    className={
                      selected.has(component.name) ? 'component-icon selected' : 'component-icon'
                    }
                    style={{ left: at.x, top: at.y }}
                    data-icon={component.name}
                    title={`${component.name}: ${component.class}`}
                  >
                    <span className="component-glyph">{glyphOf(component.class)}</span>
                    <span className="component-name">{component.name}</span>
                  </div>
                );
              })}
            </div>
            {opened && opened.menu !== menu && (
              // TPopupMenu はアイコンの下に開く(クライアント領域で切れないよう、枠の上に描く)
              <div
                className="popup-anchor"
                style={{
                  left: iconAt(document.components?.indexOf(opened.menu) ?? 0).x,
                  top:
                    TITLE_HEIGHT +
                    (menu ? MENU_HEIGHT : 0) +
                    iconAt(document.components?.indexOf(opened.menu) ?? 0).y +
                    ICON_SIZE +
                    14,
                  ...fontStyle(formLook.font),
                }}
              >
                <MenuList
                  items={opened.menu.items ?? []}
                  opened={opened.open}
                  selected={selected}
                />
              </div>
            )}
            {rect && (
              <RectOverlay
                stage={stage.current}
                zoom={zoom}
                parent={rect.parent}
                bounds={rect.bounds}
              />
            )}
          </div>
        </div>
      </div>
    </section>
  );
}

/** 選択の枠とつまみ(選択が 1 つのときだけつまみを出す)。位置は DOM から測る */
function SelectionOverlay({
  document,
  selection,
  preview,
  form,
}: {
  readonly document: NvformDocument;
  readonly selection: readonly string[];
  readonly preview: ReadonlyMap<string, Bounds>;
  /** フォームを選択しているときに、右と下の端につまみを出す */
  readonly form: { readonly width: number; readonly height: number };
}) {
  const [frames, setFrames] = useState<
    readonly { name: string; bounds: Bounds; handles: boolean }[]
  >([]);
  const zoom = useUiStore((s) => s.zoom);
  const shownPages = useUiStore((s) => s.shownPages);
  const ref = useRef<HTMLDivElement>(null);

  // 描画の後に、選択したコントロールの位置をフォームのクライアント領域の座標で測る
  useEffect(() => {
    const client = ref.current?.parentElement;
    if (!client) return;
    const origin = client.getBoundingClientRect();
    const controls = selection.filter((n) => findNode(document, n)?.kind === 'control');
    setFrames(
      controls.flatMap((name) => {
        const element = client.querySelector(`[data-node="${CSS.escape(name)}"]`);
        if (!element) return [];
        const r = element.getBoundingClientRect();
        const location = findNode(document, name);
        return [
          {
            name,
            bounds: {
              left: (r.left - origin.left) / zoom,
              top: (r.top - origin.top) / zoom,
              width: r.width / zoom,
              height: r.height / zoom,
            },
            handles: controls.length === 1 && location !== undefined && canResize(location),
          },
        ];
      }),
    );
  }, [document, selection, preview, zoom, shownPages]);

  const formSelected = selection.includes(document.form.name);
  return (
    <div ref={ref} className="selection-layer">
      {formSelected && (
        <div
          className="selection-frame form"
          style={{ left: 0, top: 0, width: form.width, height: form.height }}
          data-frame={document.form.name}
        >
          {FORM_HANDLES.map((h) => (
            <span key={h} className={`handle handle-${h}`} data-handle={h} />
          ))}
        </div>
      )}
      {frames.map(({ name, bounds, handles }) => (
        <div
          key={name}
          className={handles ? 'selection-frame' : 'selection-frame multi'}
          style={{ left: bounds.left, top: bounds.top, width: bounds.width, height: bounds.height }}
          data-frame={name}
        >
          {handles &&
            HANDLES.map((h) => <span key={h} className={`handle handle-${h}`} data-handle={h} />)}
        </div>
      ))}
    </div>
  );
}

/** 範囲選択・追加の範囲(parent のクライアント領域の座標を、枠の中の座標にして描く) */
function RectOverlay({
  stage,
  zoom,
  parent,
  bounds,
}: {
  readonly stage: HTMLDivElement | null;
  readonly zoom: number;
  readonly parent: string;
  readonly bounds: Bounds;
}) {
  const client = stage?.querySelector(`[data-client="${CSS.escape(parent)}"]`);
  if (!stage || !client) return null;
  const s = stage.getBoundingClientRect();
  const c = client.getBoundingClientRect();
  return (
    <div
      className="rect-overlay"
      style={{
        left: (c.left - s.left) / zoom + bounds.left,
        top: (c.top - s.top) / zoom + bounds.top,
        width: bounds.width,
        height: bounds.height,
      }}
    />
  );
}

/** つまみで大きさを変えられるか(位置と大きさを書くコントロール) */
function canResize(location: NodeLocation): boolean {
  return location.kind === 'control' && hasOwnBounds(location.node.class);
}

function sameParent(document: NvformDocument, a: string, b: string): boolean {
  const la = findNode(document, a);
  const lb = findNode(document, b);
  if (la?.kind === 'control' && lb?.kind === 'control') return la.parent === lb.parent;
  return la?.kind === lb?.kind && la?.kind === 'component';
}

/** コントロールの name → 親(フォームかコントロール) */
function parentMap(document: NvformDocument): Map<string, { name: string; class: string }> {
  const map = new Map<string, { name: string; class: string }>();
  for (const location of walkNodes(document))
    if (location.kind === 'control') map.set(location.node.name, location.parent);
  return map;
}

/** 開いて表示するメニュー: 選んだメニュー項目の祖先と、その項目自身(サブメニューを持つもの) */
function openedMenu(
  document: NvformDocument,
  name: string | undefined,
): { menu: ComponentNode; open: ReadonlySet<string> } | undefined {
  const location = name === undefined ? undefined : findNode(document, name);
  if (location?.kind === 'component' && location.node.class === 'TPopupMenu')
    return { menu: location.node, open: new Set() };
  if (location?.kind !== 'menuItem') return undefined;
  const target = location.node.name;
  const open = new Set<string>();
  const walk = (items: readonly MenuItemNode[] | undefined): boolean => {
    for (const item of items ?? []) {
      if (item.name === target || walk(item.items)) {
        if ((item.items ?? []).length > 0) open.add(item.name);
        return true;
      }
    }
    return false;
  };
  walk(location.menu.items);
  return { menu: location.menu, open };
}

/** メニューのドロップダウン(開いた項目のサブメニューは右に出す) */
function MenuList({
  items,
  opened,
  selected,
}: {
  readonly items: readonly MenuItemNode[];
  readonly opened: ReadonlySet<string>;
  readonly selected: ReadonlySet<string>;
}) {
  return (
    <div className="menu-dropdown">
      {items.length === 0 && <div className="menu-row empty">{l10n.t('(no items)')}</div>}
      {items.map((item) => {
        const p = item.properties ?? {};
        const caption = typeof p.Caption === 'string' ? p.Caption : '';
        if (caption === '-')
          return (
            <div
              key={item.name}
              data-menu-item={item.name}
              className={selected.has(item.name) ? 'menu-separator selected' : 'menu-separator'}
            />
          );
        const children = item.items ?? [];
        return (
          <div
            key={item.name}
            data-menu-item={item.name}
            className={[
              'menu-row',
              selected.has(item.name) ? 'selected' : '',
              p.Enabled === false ? 'disabled' : '',
            ].join(' ')}
          >
            <span className="menu-check">{p.Checked === true ? '✓' : ''}</span>
            <span className="menu-caption">
              <Caption value={caption} />
            </span>
            <span className="menu-shortcut">
              {typeof p.ShortCut === 'string' ? p.ShortCut : ''}
            </span>
            <span className="menu-arrow">{children.length > 0 ? '›' : ''}</span>
            {opened.has(item.name) && (
              <div className="menu-cascade">
                <MenuList items={children} opened={opened} selected={selected} />
              </div>
            )}
          </div>
        );
      })}
    </div>
  );
}

function mainMenuOf(document: NvformDocument) {
  const name = document.form.properties?.Menu;
  return typeof name === 'string'
    ? document.components?.find((c) => c.name === name && c.class === 'TMainMenu')
    : undefined;
}

function numberOr(value: unknown, fallback: number): number {
  return typeof value === 'number' && value > 0 ? value : fallback;
}

/** アイコンの代わりの短い表示(クラス名の大文字。TMainMenu → MM) */
function glyphOf(className: string): string {
  const capitals = className.slice(1).replace(/[^A-Z]/g, '');
  return (capitals.length > 0 ? capitals : className.slice(1, 3)).slice(0, 3);
}
