/**
 * クラスのアイコン(docs/designer/editor-design.md §5.1・§5.2)。パレット・構造の木・キャンバスの非ビジュアルコンポーネントで使う。
 * 16×16 の線画の SVG(自作)。線は currentColor で VS Code のテーマに合わせ、要所だけ強調色(--beth-icon-accent)で描く。
 * 線が画素にそろうよう、線の座標は .5 にする。
 *
 * クラス名 a: 強調色の線、af: 強調色の塗り、sf: 薄い塗り
 */
import type { ReactNode } from 'react';

const ICONS: Readonly<Record<string, ReactNode>> = {
  // ---- フォーム・メニュー ----
  TForm: (
    <>
      <rect x="1.5" y="2.5" width="13" height="11" rx="1" />
      <rect className="af" x="2" y="3" width="12" height="2.5" />
      <path d="M1.5 5.5h13" />
    </>
  ),
  TMainMenu: (
    <>
      <rect x="1.5" y="2.5" width="13" height="11" rx="1" />
      <path d="M1.5 5.5h13" />
      <path className="a" d="M3.5 4h2M7.5 4h2" />
      <rect className="sf" x="3.5" y="5.5" width="6" height="6.5" />
      <path d="M3.5 5.5v6.5h6v-6.5M5 8h3M5 10.5h3" />
    </>
  ),
  TPopupMenu: (
    <>
      <rect className="sf" x="4.5" y="3.5" width="9.5" height="10" />
      <rect x="4.5" y="3.5" width="9.5" height="10" />
      <path d="M6.5 6h5.5M6.5 8.5h5.5M6.5 11h5.5" />
      <path className="af" d="M1 1l4.5 1.8-2 .7-.7 2z" />
    </>
  ),
  TMenuItem: (
    <>
      <rect className="sf" x="1.5" y="5.5" width="13" height="5" />
      <rect x="1.5" y="5.5" width="13" height="5" />
      <path d="M3.5 8h6" />
      <path className="a" d="M11.8 6.8l1.2 1.2-1.2 1.2" />
    </>
  ),

  // ---- ボタン ----
  TButton: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" rx="2" />
      <path className="a" d="M5 8h6" />
    </>
  ),
  TBitBtn: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" rx="2" />
      <rect className="af" x="3.5" y="6.5" width="3" height="3" />
      <path d="M8.5 8h4" />
    </>
  ),
  TSpeedButton: (
    <>
      <rect x="3.5" y="3.5" width="9" height="9" rx="1.5" />
      <path className="af" d="M6.5 5.5l4 2.5-4 2.5z" />
    </>
  ),
  TToggleBox: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" rx="2" />
      <rect className="sf" x="3" y="6" width="10" height="4" rx="1" />
      <path className="a" d="M5 8h6" />
    </>
  ),
  TToolButton: (
    <>
      <rect x="3.5" y="3.5" width="9" height="9" rx="1.5" />
      <rect className="af" x="6" y="6" width="4" height="4" />
    </>
  ),

  // ---- 文字・入力 ----
  TLabel: <path d="M3.5 13.5L8 2.5l4.5 11M5.3 9.5h5.4" />,
  TStaticText: (
    <>
      <rect x="1.5" y="1.5" width="13" height="13" />
      <path d="M5 12L8 4.5 11 12M6.1 9.5h3.8" />
    </>
  ),
  TEdit: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" />
      <path className="a" d="M4.5 6.5v3" />
    </>
  ),
  TLabeledEdit: (
    <>
      <path d="M1.5 4h6" />
      <rect x="1.5" y="7.5" width="13" height="6" />
      <path className="a" d="M4.5 9.2v2.6" />
    </>
  ),
  TMaskEdit: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" />
      <circle className="af" cx="5" cy="8" r="1" />
      <circle className="af" cx="8" cy="8" r="1" />
      <circle className="af" cx="11" cy="8" r="1" />
    </>
  ),
  TSpinEdit: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" />
      <path d="M10.5 4.5v7M10.5 8h4M3.5 8h4.5" />
      <path className="a" d="M11.7 6.9l.8-.9.8.9M11.7 9.1l.8.9.8-.9" />
    </>
  ),
  TFloatSpinEdit: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" />
      <path d="M10.5 4.5v7M10.5 8h4M3.5 8h1.5M7 8h1.5" />
      <circle className="af" cx="6" cy="9.6" r=".8" />
      <path className="a" d="M11.7 6.9l.8-.9.8.9M11.7 9.1l.8.9.8-.9" />
    </>
  ),
  TMemo: (
    <>
      <rect x="1.5" y="1.5" width="13" height="13" />
      <path d="M11.5 1.5v13M3.5 4.5h6M3.5 7.5h6M3.5 10.5h4" />
      <path className="a" d="M13 3.5v3" />
    </>
  ),

  // ---- 選択 ----
  TCheckBox: (
    <>
      <rect x="1.5" y="4.5" width="7" height="7" rx="1" />
      <path className="a" d="M3.2 8l1.6 1.6 2.5-3.3" />
      <path d="M10.5 8h4" />
    </>
  ),
  TRadioButton: (
    <>
      <circle cx="5" cy="8" r="3.5" />
      <circle className="af" cx="5" cy="8" r="1.6" />
      <path d="M10.5 8h4" />
    </>
  ),
  TComboBox: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" />
      <path d="M10.5 4.5v7M3.5 8h4.5" />
      <path className="a" d="M11.6 7.3l.9 1.2.9-1.2" />
    </>
  ),
  TListBox: (
    <>
      <rect x="1.5" y="1.5" width="13" height="13" />
      <rect className="sf" x="2" y="6" width="12" height="3" />
      <path d="M3.5 4.5h8M3.5 10.5h8" />
      <path className="a" d="M3.5 7.5h8" />
    </>
  ),
  TCheckListBox: (
    <>
      <rect x="1.5" y="1.5" width="13" height="13" />
      <rect x="3.5" y="3.5" width="3" height="3" />
      <rect x="3.5" y="9.5" width="3" height="3" />
      <path className="a" d="M4 5l.8.8 1.4-1.8" />
      <path d="M8.5 5h4M8.5 11h4" />
    </>
  ),

  // ---- 入れ物 ----
  TGroupBox: (
    <>
      <path d="M4 4.5H1.5v10h13v-10H11" />
      <path className="a" d="M5.5 4.5h4" />
    </>
  ),
  TRadioGroup: (
    <>
      <path d="M4 3.5H1.5v11h13v-11H11" />
      <path className="a" d="M5.5 3.5h4" />
      <circle className="af" cx="5" cy="7.5" r="1.5" />
      <circle cx="5" cy="11.5" r="1.5" />
      <path d="M8 7.5h4M8 11.5h4" />
    </>
  ),
  TCheckGroup: (
    <>
      <path d="M4 3.5H1.5v11h13v-11H11" />
      <path className="a" d="M5.5 3.5h4" />
      <rect className="af" x="3.5" y="6" width="3" height="3" />
      <rect x="3.5" y="10.5" width="3" height="3" />
      <path d="M8 7.5h4M8 12h4" />
    </>
  ),
  TPanel: (
    <>
      <rect className="sf" x="1.5" y="3.5" width="13" height="9" />
      <rect x="1.5" y="3.5" width="13" height="9" />
      <path className="a" d="M3.5 11V5.5h9" />
    </>
  ),
  TScrollBox: (
    <>
      <rect x="1.5" y="1.5" width="13" height="13" />
      <path d="M11.5 1.5v10M1.5 11.5h13" />
      <path className="a" d="M13 3.5v4M3.5 13h4" />
    </>
  ),
  TBevel: (
    <>
      <rect x="1.5" y="1.5" width="12" height="12" />
      <path className="a" d="M2.5 14.5h12v-12" />
    </>
  ),
  TSplitter: (
    <>
      <rect x="1.5" y="2.5" width="13" height="11" />
      <rect className="af" x="7" y="3" width="2" height="10" />
      <path d="M5.5 8h-2M4.5 7l-1 1 1 1M10.5 8h2M11.5 7l1 1-1 1" />
    </>
  ),
  TStatusBar: (
    <>
      <rect x="1.5" y="2.5" width="13" height="11" />
      <rect className="af" x="2" y="10.5" width="12" height="2.5" />
      <path d="M1.5 10.5h13" />
    </>
  ),

  // ---- ページ ----
  TTabControl: (
    <>
      <rect x="1.5" y="5.5" width="13" height="9" />
      <path className="a" d="M1.5 5.5v-3h5v3" />
      <path d="M6.5 3.5h4.5v2" />
    </>
  ),
  TPageControl: (
    <>
      <rect x="1.5" y="5.5" width="13" height="9" />
      <path className="a" d="M1.5 5.5v-3h5v3" />
      <path d="M6.5 3.5h4.5v2M3.5 8.5h7M3.5 11.5h5" />
    </>
  ),
  TTabSheet: (
    <>
      <rect x="1.5" y="4.5" width="13" height="10" />
      <path className="a" d="M2.5 4.5v-2h5v2" />
      <path d="M3.5 7.5h7M3.5 10.5h5" />
    </>
  ),

  // ---- 範囲・値 ----
  TScrollBar: (
    <>
      <rect x="1.5" y="5.5" width="13" height="5" />
      <path d="M4.5 5.5v5M11.5 5.5v5" />
      <rect className="af" x="6" y="6.5" width="3.5" height="3" />
    </>
  ),
  TTrackBar: (
    <>
      <path d="M1.5 6.5h13M2.5 11.5v1.5M5.5 11.5v1.5M8.5 11.5v1.5M11.5 11.5v1.5" />
      <path className="af" d="M6.5 3.5h3v4l-1.5 1.5-1.5-1.5z" />
    </>
  ),
  TProgressBar: (
    <>
      <rect x="1.5" y="5.5" width="13" height="5" />
      <rect className="af" x="2.5" y="6.5" width="7" height="3" />
    </>
  ),
  TUpDown: (
    <>
      <rect x="4.5" y="1.5" width="7" height="13" />
      <path d="M4.5 8h7" />
      <path className="a" d="M6.5 5.5L8 4l1.5 1.5M6.5 10.5L8 12l1.5-1.5" />
    </>
  ),

  // ---- 一覧・表 ----
  TTreeView: (
    <>
      <rect className="af" x="2" y="1.5" width="4" height="3" />
      <path d="M4 4.5v8M4 8h3.5M4 12.5h3.5" />
      <rect x="7.5" y="6.5" width="6" height="3" />
      <rect x="7.5" y="11" width="6" height="3" />
    </>
  ),
  TListView: (
    <>
      <rect className="af" x="2" y="2" width="4.5" height="4.5" />
      <rect x="9.5" y="2.5" width="4" height="4" />
      <rect x="2.5" y="9.5" width="4" height="4" />
      <rect x="9.5" y="9.5" width="4" height="4" />
    </>
  ),
  TDrawGrid: (
    <>
      <rect className="af" x="2" y="2" width="12" height="3.5" />
      <rect x="1.5" y="1.5" width="13" height="13" />
      <path d="M1.5 5.5h13M1.5 10h13M5.5 1.5v13M10 1.5v13" />
    </>
  ),
  TStringGrid: (
    <>
      <rect className="af" x="2" y="2" width="12" height="3.5" />
      <rect x="1.5" y="1.5" width="13" height="13" />
      <path d="M1.5 5.5h13M1.5 10h13M5.5 1.5v13M10 1.5v13" />
      <path className="a" d="M6.8 7.8h2M6.8 12.3h2M11.3 7.8h2" />
    </>
  ),
  THeaderControl: (
    <>
      <rect x="1.5" y="5.5" width="13" height="5" />
      <path d="M6 5.5v5M10.5 5.5v5M7.5 8h1.5M12 8h1.5" />
      <path className="a" d="M3 8h1.5" />
    </>
  ),

  // ---- ツールバー ----
  TToolBar: (
    <>
      <rect x="1.5" y="4.5" width="13" height="7" />
      <rect className="af" x="3" y="6" width="3" height="4" />
      <rect x="7.5" y="6.5" width="2.5" height="3" />
      <path d="M12 6v4" />
    </>
  ),
  TCoolBar: (
    <>
      <rect x="1.5" y="2.5" width="13" height="11" />
      <path d="M1.5 8h13M6.5 5.2h6M6.5 10.8h4" />
      <path className="a" d="M3.5 4v2.5M3.5 9.5V12" />
    </>
  ),

  // ---- 描画・画像 ----
  TShape: (
    <>
      <rect x="1.5" y="5.5" width="7" height="8" />
      <circle className="a" cx="10.5" cy="6" r="4" />
    </>
  ),
  TImage: (
    <>
      <rect x="1.5" y="2.5" width="13" height="11" />
      <path className="a" d="M1.5 12l4.5-4.5 3 3 2-2 3.5 3.5" />
      <circle cx="11" cy="5.5" r="1.2" />
    </>
  ),
  TPaintBox: (
    <>
      <rect x="1.5" y="1.5" width="13" height="13" strokeDasharray="2 1.5" />
      <path className="a" d="M4 11.5c1.5-5 3.5 1 5-3.5s1.5-2 3-3" />
    </>
  ),
  TImageList: (
    <>
      <path d="M5.5 3.5v-2h9v8h-2M3.5 5.5v-2h9v8h-2" />
      <rect x="1.5" y="5.5" width="9" height="8" />
      <path className="a" d="M1.5 12.5l3-3 2 2 1.5-1.5 2.5 2.5" />
    </>
  ),

  // ---- システム ----
  TTimer: (
    <>
      <circle cx="8" cy="9" r="5.5" />
      <path d="M6.5 1.5h3M8 1.5v2" />
      <path className="a" d="M8 9V6M8 9h2.5" />
    </>
  ),

  // ---- ダイアログ ----
  TOpenDialog: (
    <>
      <path d="M1.5 13.5v-10H6L7.5 5h5v2.5" />
      <path className="a" d="M1.5 13.5l2-6H15l-2 6z" />
    </>
  ),
  TSaveDialog: (
    <>
      <path d="M2.5 2.5H11l2.5 2.5v8.5h-11z" />
      <path d="M5 2.5V6h5V2.5" />
      <rect className="af" x="4.5" y="9" width="7" height="4.5" />
    </>
  ),
  TSelectDirectoryDialog: (
    <>
      <path d="M1.5 13.5v-10H6L7.5 5h7v8.5z" />
      <path className="a" d="M1.5 7h13" />
    </>
  ),
  TColorDialog: (
    <>
      <path d="M8 1.5a6.5 6.5 0 1 0 0 13c1.2 0 1.5-1 1-2s0-2 1.3-2h1.7a2.5 2.5 0 0 0 2.5-2.5C14.5 4.3 11.6 1.5 8 1.5z" />
      <circle fill="#e5484d" stroke="none" cx="5" cy="6" r="1.3" />
      <circle fill="#30a46c" stroke="none" cx="8.5" cy="4.3" r="1.3" />
      <circle fill="#3e63dd" stroke="none" cx="11.5" cy="6.3" r="1.3" />
      <circle fill="#f5a524" stroke="none" cx="4.8" cy="9.8" r="1.3" />
    </>
  ),
  TFontDialog: (
    <>
      <path d="M1.5 13.5L5 3l3.5 10.5M2.8 9.5h4.4" />
      <circle className="a" cx="12" cy="11" r="2" />
      <path className="a" d="M14 8.5v5" />
    </>
  ),
  TFindDialog: (
    <>
      <circle cx="6.5" cy="6.5" r="4.5" />
      <path className="a" strokeWidth="2" d="M10 10l4 4" />
    </>
  ),
  TReplaceDialog: (
    <>
      <circle cx="6" cy="6" r="3.5" />
      <path d="M8.6 8.6l2 2" />
      <path className="a" d="M8.5 13.5h6M13 12l1.5 1.5L13 15M1.5 13.5h4" />
    </>
  ),
};

/** 描けないクラス(カタログに後から加えたもの等)は、点線の四角 */
const FALLBACK = <rect x="2.5" y="2.5" width="11" height="11" rx="1" strokeDasharray="2 1.5" />;

export function hasClassIcon(className: string): boolean {
  return Object.hasOwn(ICONS, className);
}

export function ClassIcon({
  className,
  size = 16,
}: {
  readonly className: string;
  readonly size?: number;
}) {
  return (
    <svg
      className="class-icon"
      width={size}
      height={size}
      viewBox="0 0 16 16"
      fill="none"
      stroke="currentColor"
      strokeWidth="1"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
      focusable="false"
    >
      {hasClassIcon(className) ? ICONS[className] : FALLBACK}
    </svg>
  );
}
