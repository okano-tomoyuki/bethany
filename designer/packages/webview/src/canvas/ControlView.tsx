/**
 * コントロールの描画(docs/designer/editor-design.md §5.1。ADR 0036)。Windows 11 のテーマの見た目に近づける。
 * 位置は親のクライアント領域からの絶対配置。子を置けるコントロールは、クライアント領域(data-client)の中に子を描く。
 */
import {
  clientMetrics,
  collectionItems,
  findClass,
  hasOwnBounds,
  propertyValue,
  type ControlNode,
  type FormNode,
  type NodeLocation,
  withActionValues,
} from '@bethany-designer/core';
import { useDocumentStore } from '../store/stores.ts';
import { createContext, useContext, type CSSProperties, type ReactNode } from 'react';
import { boundsOf } from '../editing.ts';
import {
  colorToCss,
  flag,
  fontStyle,
  number,
  resolveLook,
  splitAccelerator,
  strings,
  text,
  type Inherited,
} from './look.ts';

/** キャンバス全体で共有する表示の状態 */
export interface CanvasState {
  /** TPageControl の name → 表示する TTabSheet の name */
  readonly shownPage: (pageControl: ControlNode) => string | undefined;
  /** ドラッグ中の仮の位置と大きさ(name → 親のクライアント領域での位置) */
  readonly preview: ReadonlyMap<
    string,
    { left: number; top: number; width: number; height: number }
  >;
  /** ドロップ先として強調するコンテナ */
  readonly dropTarget: string | undefined;
}

export const CanvasContext = createContext<CanvasState>({
  shownPage: () => undefined,
  preview: new Map(),
  dropTarget: undefined,
});

/** parent の子のコントロールを描く */
export function Children({
  parent,
  path,
  inherited,
}: {
  readonly parent: FormNode | ControlNode;
  readonly path: readonly (string | number)[];
  readonly inherited: Inherited;
}) {
  return (
    <>
      {(parent.controls ?? []).map((node, i) => (
        <ControlView
          key={node.name}
          location={{ kind: 'control', node, parent, path: [...path, 'controls', i] }}
          inherited={inherited}
        />
      ))}
    </>
  );
}

function ControlView({
  location,
  inherited,
}: {
  readonly location: NodeLocation & { readonly kind: 'control' };
  readonly inherited: Inherited;
}) {
  const canvas = useContext(CanvasContext);
  const document = useDocumentStore((s) => s.document);
  // Action を割り当てたものは、Action から写る値(Caption 等)で描く(docs/adr/0046)
  location = document
    ? (withActionValues(document, location) as NodeLocation & { readonly kind: 'control' })
    : location;
  const { node } = location;
  const look = resolveLook(location, inherited);
  const own = hasOwnBounds(node.class);
  const bounds = canvas.preview.get(node.name) ?? boundsOf(location);
  const accepts = findClass(node.class)?.acceptsControls ?? false;
  // 子の座標の原点(TPanel は外側の左上、TGroupBox 等はクライアント領域の左上。記録した値)
  const { origin, insets } = clientMetrics(node.class, node.properties);

  // TTabSheet は親の TPageControl で表示しているものだけを描く
  if (node.class === 'TTabSheet' && location.parent.class === 'TPageControl') {
    if (canvas.shownPage(location.parent) !== node.name) return null;
  }

  const style: CSSProperties = {
    ...(own
      ? { left: bounds.left, top: bounds.top, width: bounds.width, height: bounds.height }
      : { inset: 0 }),
    ...fontStyle(look.font),
    // 書いた Visible だけを見る(TTabSheet の既定値は false だが、表示は TPageControl が決める)
    ...(node.properties?.Visible === false && { opacity: 0.45 }),
  };
  const classes = ['ctl', `ctl-${node.class}`];
  if (canvas.dropTarget === node.name) classes.push('drop-target');

  return (
    <div className={classes.join(' ')} style={style} data-node={node.name}>
      <Body location={location} look={look} />
      {accepts && (
        <div
          className="ctl-client"
          data-client={node.name}
          style={{
            left: origin.x,
            top: origin.y,
            right: insets.right,
            bottom: insets.bottom,
          }}
        >
          <Children parent={node} path={location.path} inherited={look} />
        </div>
      )}
    </div>
  );
}

/** Caption の & を下線にして表示する */
export function Caption({
  value,
  accel = true,
}: {
  readonly value: string;
  /** false なら & をそのまま表示する(TLabel の ShowAccelChar) */
  readonly accel?: boolean;
}) {
  if (!accel) return <>{value}</>;
  const [before, key, after] = splitAccelerator(value);
  return (
    <>
      {before}
      {key !== '' && <u>{key}</u>}
      {after}
    </>
  );
}

function Body({
  location,
  look,
}: {
  readonly location: NodeLocation & { readonly kind: 'control' };
  readonly look: Inherited;
}): ReactNode {
  const { node } = location;
  const background = { background: look.color };
  const caption = <Caption value={text(location, 'Caption')} />;

  switch (node.class) {
    case 'TButton':
    case 'TBitBtn':
      // 既定のボタン(Default。Enter で押される)は、Windows 11 と同じく強調色の枠で描く
      return (
        <div className={flag(location, 'Default') ? 'look-button default' : 'look-button'}>
          {caption}
        </div>
      );
    case 'TToggleBox':
      return (
        <div className={flag(location, 'Checked') ? 'look-button down' : 'look-button'}>
          {caption}
        </div>
      );
    case 'TSpeedButton':
    case 'TToolButton':
      return (
        <div
          className={[
            'look-button',
            'flat',
            flag(location, 'Down') ? 'down' : '',
            flag(location, 'Flat') ? '' : 'raised',
          ].join(' ')}
        >
          {node.class === 'TToolButton' && text(location, 'Style') === 'tbsSeparator'
            ? ''
            : caption}
        </div>
      );
    case 'TLabel':
      // Transparent(既定)なら背景を塗らない。横の揃え・縦の揃え・折り返しは AutoSize が false のときに見える
      return (
        <div
          className={flag(location, 'WordWrap') ? 'look-label wrap' : 'look-label'}
          style={{
            ...(flag(location, 'Transparent') || flag(location, 'ParentColor') ? {} : background),
            textAlign: textAlign(text(location, 'Alignment')),
            justifyContent: LAYOUTS[text(location, 'Layout')] ?? 'flex-start',
          }}
        >
          <span>
            <Caption value={text(location, 'Caption')} accel={flag(location, 'ShowAccelChar')} />
          </span>
        </div>
      );
    case 'TStaticText':
      return (
        <div
          className={`look-label ${text(location, 'BorderStyle') === 'sbsNone' ? '' : 'sunken'}`}
          style={background}
        >
          {caption}
        </div>
      );
    case 'TEdit':
    case 'TMaskEdit':
    case 'TLabeledEdit': {
      // 空なら TextHint を薄く、PasswordChar・EchoMode が伏せ字なら伏せて表示する
      const value = text(location, 'Text');
      return (
        <div
          className={bordered(
            location,
            value === '' && text(location, 'TextHint') !== '' ? 'look-edit hint' : 'look-edit',
          )}
          style={{
            ...background,
            justifyContent: EDIT_ALIGN[text(location, 'Alignment')] ?? 'flex-start',
          }}
        >
          {value === '' ? text(location, 'TextHint') : maskText(location, value)}
        </div>
      );
    }
    case 'TSpinEdit':
    case 'TFloatSpinEdit': {
      const value = number(location, 'Value');
      const decimals = node.class === 'TFloatSpinEdit' ? number(location, 'DecimalPlaces') : 0;
      return (
        <div className={bordered(location, 'look-edit spin')} style={background}>
          <span>{value.toFixed(Math.max(0, decimals))}</span>
          <span className="look-spin" />
        </div>
      );
    }
    case 'TComboBox':
      return (
        <div className="look-edit combo" style={background}>
          <span>{text(location, 'Text')}</span>
          <span className="look-dropdown">⌄</span>
        </div>
      );
    case 'TMemo':
      return (
        <div
          className={bordered(
            location,
            flag(location, 'WordWrap') ? 'look-box lines wrap' : 'look-box lines',
          )}
          style={{ ...background, textAlign: textAlign(text(location, 'Alignment')) }}
        >
          {strings(location, 'Lines').map((line, i) => (
            <div key={i}>{line === '' ? ' ' : line}</div>
          ))}
        </div>
      );
    case 'TListBox':
    case 'TCheckListBox': {
      // オーナードロー(lbOwnerDraw…)で ItemHeight があれば、その高さで並べる(中身は OnDrawItem が描く。docs/adr/0050)
      const ownerDraw = text(location, 'Style').startsWith('lbOwnerDraw');
      const itemHeight = number(location, 'ItemHeight');
      const itemStyle = ownerDraw && itemHeight > 0 ? { height: itemHeight } : undefined;
      return (
        <div className={bordered(location, 'look-box lines')} style={background}>
          {strings(location, 'Items').map((item, i) => (
            <div
              key={i}
              className={i === number(location, 'ItemIndex') ? 'item selected' : 'item'}
              style={itemStyle}
            >
              {node.class === 'TCheckListBox' && <span className="look-check small" />}
              {item}
            </div>
          ))}
        </div>
      );
    }
    case 'TCheckBox':
    case 'TRadioButton':
      return (
        <div className="look-check-row" style={flag(location, 'ParentColor') ? {} : background}>
          <span
            className={[
              'look-check',
              node.class === 'TRadioButton' ? 'radio' : '',
              // Checked と State(cbChecked・cbGrayed)のどちらで設定しても描く
              flag(location, 'Checked') || text(location, 'State') === 'cbChecked' ? 'checked' : '',
              text(location, 'State') === 'cbGrayed' ? 'grayed' : '',
            ].join(' ')}
          />
          <span>{caption}</span>
        </div>
      );
    case 'TGroupBox':
      return <Frame caption={caption} />;
    case 'TRadioGroup':
    case 'TCheckGroup':
      return (
        <Frame caption={caption}>
          <div className="look-group-items" style={groupColumns(location)}>
            {strings(location, 'Items').map((item, i) => (
              <div key={i} className="look-check-row">
                <span
                  className={[
                    'look-check',
                    node.class === 'TRadioGroup' ? 'radio' : '',
                    node.class === 'TRadioGroup' && i === number(location, 'ItemIndex')
                      ? 'checked'
                      : '',
                  ].join(' ')}
                />
                <span>{item}</span>
              </div>
            ))}
          </div>
        </Frame>
      );
    case 'TPanel':
      return <Panel location={location} background={background} caption={caption} />;
    case 'TPageControl':
      return <PageTabs location={location} />;
    case 'TTabControl':
      return (
        <div className="look-tabs-frame">
          <div className="look-tabs">
            {strings(location, 'Tabs').map((tab, i) => (
              <span
                key={i}
                className={i === number(location, 'TabIndex') ? 'look-tab active' : 'look-tab'}
              >
                {tab}
              </span>
            ))}
          </div>
          <div className="look-tabs-body" />
        </div>
      );
    case 'TTabSheet':
      return <div className="look-fill" style={{ background: '#f9f9f9' }} />;
    case 'TScrollBox':
      return <div className={bordered(location, 'look-box')} style={background} />;
    case 'TProgressBar': {
      const range = number(location, 'Max') - number(location, 'Min');
      const ratio =
        range > 0 ? (number(location, 'Position') - number(location, 'Min')) / range : 0;
      // 向き(pbVertical は下から、pbTopDown は上から、pbRightToLeft は右から)と、マーキー(一部だけを描く)。docs/adr/0049
      const orientation = text(location, 'Orientation');
      const marquee = text(location, 'Style') === 'pbstMarquee';
      const size = `${String((marquee ? 0.3 : Math.max(0, Math.min(1, ratio))) * 100)}%`;
      const start = marquee ? '35%' : 0;
      const bar: CSSProperties =
        orientation === 'pbVertical'
          ? { left: 0, right: 0, bottom: start, height: size }
          : orientation === 'pbTopDown'
            ? { left: 0, right: 0, top: start, height: size }
            : orientation === 'pbRightToLeft'
              ? { top: 0, bottom: 0, right: start, width: size }
              : { top: 0, bottom: 0, left: start, width: size };
      return (
        <div className="look-progress">
          <div style={bar} />
        </div>
      );
    }
    case 'TTrackBar':
      return (
        <div
          className={
            text(location, 'Orientation') === 'trVertical' ? 'look-track vertical' : 'look-track'
          }
        >
          <div className="look-track-line" />
        </div>
      );
    case 'TScrollBar':
      return (
        <div
          className={
            text(location, 'Kind') === 'sbVertical' ? 'look-scroll vertical' : 'look-scroll'
          }
        />
      );
    case 'TUpDown':
      return (
        <div
          className={
            text(location, 'Orientation') === 'udHorizontal'
              ? 'look-updown horizontal'
              : 'look-updown'
          }
        />
      );
    case 'TStatusBar':
      return <StatusBar location={location} />;
    case 'TToolBar':
    case 'TCoolBar':
      return <div className="look-bar" style={background} />;
    case 'THeaderControl':
      return <div className="look-header" />;
    case 'TTreeView':
    case 'TListView':
      return <div className={bordered(location, 'look-box')} style={background} />;
    case 'TStringGrid':
    case 'TDrawGrid':
      return <Grid location={location} background={look.color} />;
    case 'TShape':
      return (
        <div
          className={`look-shape ${/Circle|Ellipse/.test(text(location, 'Shape')) ? 'round' : ''}`}
        />
      );
    case 'TBevel':
      return <div className="look-bevel" />;
    case 'TSplitter':
      return <div className="look-splitter" />;
    case 'TImage':
    case 'TPaintBox':
      return <div className="look-placeholder" />;
    default:
      return <div className="look-placeholder">{node.class}</div>;
  }
}

function Frame({
  caption,
  children,
}: {
  readonly caption: ReactNode;
  readonly children?: ReactNode;
}) {
  return (
    <div className="look-frame">
      <span className="look-frame-caption">{caption}</span>
      {children}
    </div>
  );
}

function PageTabs({
  location,
}: {
  readonly location: NodeLocation & { readonly kind: 'control' };
}) {
  const canvas = useContext(CanvasContext);
  const shown = canvas.shownPage(location.node);
  const sheets = (location.node.controls ?? []).filter((c) => c.class === 'TTabSheet');
  return (
    <div className="look-tabs-frame">
      <div className="look-tabs">
        {sheets.map((sheet) => (
          <span
            key={sheet.name}
            className={sheet.name === shown ? 'look-tab active' : 'look-tab'}
            data-tab={sheet.name}
            data-page-control={location.node.name}
          >
            <Caption
              value={typeof sheet.properties?.Caption === 'string' ? sheet.properties.Caption : ''}
            />
          </span>
        ))}
      </div>
      <div className="look-tabs-body" />
    </div>
  );
}

/** ステータスバー。SimplePanel なら SimpleText、そうでなければパネルを並べる(最後のパネルは残りの幅いっぱい。docs/adr/0044) */
function StatusBar({
  location,
}: {
  readonly location: NodeLocation & { readonly kind: 'control' };
}) {
  if (flag(location, 'SimplePanel'))
    return <div className="look-status">{text(location, 'SimpleText')}</div>;
  const panels = collectionItems(location, 'Panels');
  return (
    <div className="look-status panels">
      {panels.map((panel, i) => {
        const last = i === panels.length - 1;
        const width = typeof panel.Width === 'number' ? Math.max(0, panel.Width) : 0;
        const bevel = typeof panel.Bevel === 'string' ? panel.Bevel : 'pbLowered';
        return (
          <div
            key={i}
            className={`look-status-panel ${bevel}`}
            style={{
              ...(last ? { flex: '1 1 0' } : { flex: 'none', width: `${String(width)}px` }),
              textAlign: textAlign(typeof panel.Alignment === 'string' ? panel.Alignment : ''),
            }}
          >
            {panel.Style === 'psOwnerDraw' ? (
              <span className="look-owner-draw">OnDrawPanel</span>
            ) : typeof panel.Text === 'string' ? (
              panel.Text
            ) : (
              ''
            )}
          </div>
        );
      })}
    </div>
  );
}

/**
 * TStringGrid・TDrawGrid。固定行・固定列を FixedColor、それ以外を Color で塗り、1 行おきに AlternateColor で塗る
 * (LCL の既定の AltColorStartNormal と同じく、固定行の次の行は Color)。線は GridLineColor・GridLineWidth で描き、
 * AutoFillColumns なら固定列以外の列を広げて(縮めて)幅に合わせる(docs/adr/0053)。
 * Columns があれば、固定列の後の列は Columns の列になり(ColCount は使わない)、列の Width(無ければ DefaultColWidth)の幅で、
 * 1 行目の見出しに Title.Caption を TitleFont の太字・斜体で書く。Visible が false の列は幅 0(docs/adr/0054)。
 */
function Grid({
  location,
  background,
}: {
  readonly location: NodeLocation & { readonly kind: 'control' };
  readonly background: string;
}) {
  const columns = collectionItems(location, 'Columns');
  const rawColumns = propertyValue(location, ['Columns']);
  const fixedCols = number(location, 'FixedCols');
  const fixedRows = number(location, 'FixedRows');
  const cols = Math.min(
    columns.length > 0 ? fixedCols + columns.length : number(location, 'ColCount'),
    50,
  );
  const rows = Math.min(number(location, 'RowCount'), 50);
  const width = number(location, 'DefaultColWidth');
  const height = number(location, 'DefaultRowHeight');
  const fixedColor = colorToCss(propertyValue(location, ['FixedColor']));
  const alternateColor = colorToCss(propertyValue(location, ['AlternateColor'])) ?? background;
  const line = `${String(number(location, 'GridLineWidth'))}px solid ${colorToCss(propertyValue(location, ['GridLineColor'])) ?? '#c0c0c0'}`;
  const autoFill = flag(location, 'AutoFillColumns');
  const titleFontStyle = propertyValue(location, ['TitleFont', 'Style']);
  const titleStyles: readonly unknown[] = Array.isArray(titleFontStyle) ? titleFontStyle : [];
  const titleStyle = {
    fontWeight: titleStyles.includes('fsBold') ? 700 : 400,
    fontStyle: titleStyles.includes('fsItalic') ? 'italic' : 'normal',
  };
  const column = (c: number) => (c >= fixedCols ? columns[c - fixedCols] : undefined);
  const columnWidth = (c: number): number => {
    const col = column(c);
    if (col === undefined) return width;
    if (col.Visible === false) return 0;
    const raw = Array.isArray(rawColumns)
      ? (rawColumns[c - fixedCols] as Record<string, unknown> | undefined)
      : undefined;
    return typeof raw?.Width === 'number' ? raw.Width : width;
  };
  const caption = (c: number): string => {
    const title = column(c)?.Title;
    const text =
      typeof title === 'object' && title !== null
        ? (title as Record<string, unknown>).Caption
        : undefined;
    return typeof text === 'string' ? text : 'Title';
  };
  return (
    <div className={bordered(location, 'look-box grid')} style={{ background }}>
      {Array.from({ length: rows }, (_, r) => (
        <div key={r} className="look-grid-row" style={{ height }}>
          {Array.from({ length: cols }, (_, c) => {
            const fixed = r < fixedRows || c < fixedCols;
            const color = fixed
              ? fixedColor
              : (r - fixedRows) % 2 === 1
                ? alternateColor
                : background;
            const w = columnWidth(c);
            const title = r === 0 && fixedRows > 0 && column(c) !== undefined;
            return (
              <div
                key={c}
                className={fixed ? 'look-grid-cell fixed' : 'look-grid-cell'}
                style={{
                  ...(autoFill && c >= fixedCols && w > 0
                    ? { flex: `1 1 ${String(w)}px`, minWidth: 0 }
                    : { width: w }),
                  ...(color ? { background: color } : {}),
                  ...(w > 0 ? { borderRight: line } : {}),
                  borderBottom: line,
                  ...(title ? titleStyle : {}),
                }}
              >
                {title && w > 0 && <span className="look-grid-title">{caption(c)}</span>}
              </div>
            );
          })}
        </div>
      ))}
    </div>
  );
}

/**
 * TRadioGroup・TCheckGroup の項目の並べ方(docs/adr/0049)。Columns の列に、ColumnLayout の順で並べる。
 * AutoFill なら項目を高さいっぱいに広げ、そうでなければ上から詰める。
 */
function groupColumns(location: NodeLocation): CSSProperties {
  const columns = Math.max(1, number(location, 'Columns'));
  const rows = Math.max(1, Math.ceil(strings(location, 'Items').length / columns));
  const track = flag(location, 'AutoFill') ? '1fr' : 'auto';
  return {
    display: 'grid',
    gridTemplateColumns: `repeat(${String(columns)}, minmax(0, 1fr))`,
    gridTemplateRows: `repeat(${String(rows)}, ${track})`,
    gridAutoFlow: text(location, 'ColumnLayout') === 'clVerticalThenHorizontal' ? 'column' : 'row',
    alignContent: flag(location, 'AutoFill') ? 'stretch' : 'start',
    alignItems: 'center',
  };
}

/** BorderStyle が bsNone なら枠を描かない(docs/adr/0048) */
function bordered(location: NodeLocation, className: string): string {
  return text(location, 'BorderStyle') === 'bsNone' ? `${className} no-border` : className;
}

/** TVerticalAlignment → 縦の位置(flex) */
const VERTICAL_ALIGN: Readonly<Record<string, string>> = {
  taAlignTop: 'flex-start',
  taAlignBottom: 'flex-end',
};

/**
 * TPanel(docs/adr/0048)。外から枠(BorderStyle が bsSingle)・外側の縁・内側の縁・BorderWidth の余白を重ね、
 * Caption を Alignment・VerticalAlignment・WordWrap に従って置く。
 */
function Panel({
  location,
  background,
  caption,
}: {
  readonly location: NodeLocation & { readonly kind: 'control' };
  readonly background: CSSProperties;
  readonly caption: ReactNode;
}) {
  const width = number(location, 'BevelWidth');
  const color = colorToCss(propertyValue(location, ['BevelColor']));
  const edge = (bevel: string): CSSProperties => {
    if (bevel === 'bvNone' || bevel === '') return {};
    if (bevel === 'bvSpace') return { border: `${String(width)}px solid transparent` };
    const [light, dark] = color ? [color, color] : ['#fff', '#a0a0a0'];
    const [topLeft, bottomRight] = bevel === 'bvLowered' ? [dark, light] : [light, dark];
    return {
      borderStyle: 'solid',
      borderWidth: width,
      borderColor: `${topLeft} ${bottomRight} ${bottomRight} ${topLeft}`,
    };
  };
  const alignment = text(location, 'Alignment');
  const wrap = flag(location, 'WordWrap');
  return (
    <div
      className={text(location, 'BorderStyle') === 'bsSingle' ? 'look-panel framed' : 'look-panel'}
      style={background}
    >
      <div className="look-panel-part" style={edge(text(location, 'BevelOuter'))}>
        <div className="look-panel-part" style={edge(text(location, 'BevelInner'))}>
          <div
            className="look-panel-part"
            style={{
              padding: number(location, 'BorderWidth'),
              justifyContent: EDIT_ALIGN[alignment] ?? 'flex-start',
              alignItems: VERTICAL_ALIGN[text(location, 'VerticalAlignment')] ?? 'center',
              textAlign: textAlign(alignment),
              whiteSpace: wrap ? 'pre-wrap' : 'pre',
            }}
          >
            <span className={wrap ? 'look-panel-text wrap' : 'look-panel-text'}>{caption}</span>
          </div>
        </div>
      </div>
    </div>
  );
}

/** TAlignment → CSS の text-align */
function textAlign(alignment: string): 'left' | 'right' | 'center' {
  return alignment === 'taRightJustify' ? 'right' : alignment === 'taCenter' ? 'center' : 'left';
}

/** TAlignment → 1 行の入力欄の中での位置(flex) */
const EDIT_ALIGN: Readonly<Record<string, string>> = {
  taRightJustify: 'flex-end',
  taCenter: 'center',
};

/** TTextLayout → 縦の位置(flex) */
const LAYOUTS: Readonly<Record<string, string>> = {
  tlTop: 'flex-start',
  tlCenter: 'center',
  tlBottom: 'flex-end',
};

/** 伏せ字(EchoMode が emPassword なら PasswordChar か *、emNone なら表示しない。PasswordChar だけでも伏せる) */
function maskText(location: NodeLocation, value: string): string {
  const mode = text(location, 'EchoMode');
  if (mode === 'emNone') return '';
  const passwordChar = text(location, 'PasswordChar');
  const mask =
    passwordChar !== '' && passwordChar !== '\u0000'
      ? passwordChar
      : mode === 'emPassword'
        ? '*'
        : '';
  return mask === '' ? value : mask.repeat(Array.from(value).length);
}
