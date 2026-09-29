/**
 * verify-cpp.mts・verify-python.mts の共通部分: 見本の読み込みと、実行結果(フォームを表示した後の配置・イベント・
 * プロパティ)と期待との照合。C++ と Python で同じ結果になることを確かめるため、期待は 1 か所に置く。
 */
import { readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import {
  findClass,
  parseDocument,
  walkNodes,
  type BfmDocument,
} from '../../packages/core/src/index.ts';

export const designerRoot = fileURLToPath(new URL('../../', import.meta.url));
export const repoRoot = join(designerRoot, '..');
export const SAMPLE_FILE = 'MainForm.bfm.json';

/** 見本(samples/MainForm.bfm.json)を読み込む */
export function loadSample(): BfmDocument {
  const text = readFileSync(join(designerRoot, 'samples', SAMPLE_FILE), 'utf8');
  const { document, diagnostics } = parseDocument(text);
  if (!document || diagnostics.length > 0)
    throw new Error(`見本が検証を通りません: ${JSON.stringify(diagnostics)}`);
  return document;
}

/** 内容が同じなら書き換えない(差分ビルドを効かせる) */
export function writeIfChanged(path: string, text: string): void {
  let current: string | undefined;
  try {
    current = readFileSync(path, 'utf8');
  } catch {
    current = undefined;
  }
  if (current !== text) writeFileSync(path, text);
}

/** 検証のプログラムが最後の行に JSON で書き出すもの */
export interface Report {
  /** コントロールの名前 → [Left, Top, Width, Height](表示した後) */
  readonly bounds: Readonly<Record<string, readonly number[]>>;
  /** 呼ばれたハンドラ */
  readonly calls: readonly string[];
  /** 項目 → 値(文字列) */
  readonly checks: Readonly<Record<string, string>>;
}

export function parseReport(output: string): Report {
  return JSON.parse(output.trim().split(/\r?\n/).pop() ?? '') as Report;
}

/** LCL を通して発生させたイベント(と、接続されたハンドラを直接呼んだもの)で呼ばれるハンドラ */
export const EXPECTED_CALLS = [
  'FormCreate',
  'NameEditChange',
  'WrapCheckClick',
  'FileOpenItemClick',
  'FileSaveActionExecute',
  'ClearItemClick',
  'OkButtonClick',
  'Timer1Timer',
  'FormCloseQuery',
  'FormDropFiles',
  'RadioGroup1SelectionChanged',
  'ScrollBar1Scroll',
  'ColorListDrawItem',
  'Tree1Edited',
  'Tree1CustomDrawItem',
  'List1Data',
  'List1Compare',
  'List1Edited',
];

/** 参照・入れ子のオブジェクト・TStrings・メニュー等(検証のプログラムが読んだ値) */
export const EXPECTED_CHECKS: Readonly<Record<string, string>> = {
  menu: '1',
  activePage: '1',
  popupMenu: '1',
  memoLines: 'line 1|line "2"',
  okFontBold: '1',
  statusFont: '10/16711680',
  panelColor: '12644607',
  shortCut: 'Ctrl+O',
  menuCounts: '1/4/1',
  // Action(docs/adr/0046): Action を割り当てた項目の Caption/ShortCut/ActionList の Action の数
  actionLink: '&Save/Ctrl+S/1',
  // ファイルのドロップ(docs/adr/0047)
  allowDropFiles: '1',
  // パネルの縁・コントロールの枠(docs/adr/0048): BevelOuter/BevelInner/BevelWidth/BorderWidth と、Memo1 の BorderStyle/ScrollBars
  panelBevel: '1/2/2/1',
  memoBorder: '0/3',
  // 範囲のコントロール・グループの列(docs/adr/0049): RadioGroup1 の Columns/ColumnLayout と、ScrollBar1 の LargeChange/SmallChange
  groupColumns: '2/1',
  scrollChange: '10/2',
  // オーナードロー(docs/adr/0050): ColorList の Style(lbOwnerDrawFixed)/ItemHeight
  ownerDraw: '1/20',
  // TTreeView(docs/adr/0051): SortType(stText)/Indent/MultiSelect と、OnEdited で書き換えた文字列
  treeView: '2/20/1',
  treeEdited: 'x!',
  // TListView(docs/adr/0052): ViewStyle(vsReport)/ShowColumnHeaders/AutoSort/OwnerData、OwnerData の Items->Count と、OnEdited で書き換えた文字列
  listView: '3/0/0/1',
  listCount: '3',
  listEdited: 'y!',
  dialogOptions: String((1 << 20) | (1 << 23) | (1 << 9)),
  timer: '0/500',
  caption: 'Sample',
  spinValue: '150',
  // コレクション(docs/adr/0044): 数/Text/Width/Style(psOwnerDraw)/Alignment(taRightJustify)/Bevel(pbNone)
  statusPanels: '3/Ready/120/1/1/0',
  // フォームの Width を 100 広げた後の OkButton.Left(右寄せ)・NameEdit.Width(左右寄せ)・BottomPanel.Width(alBottom)
  anchors: '404/380/500',
};

/** 実行結果を期待と照合して表示し、不一致があれば process.exitCode を 1 にする */
export function checkReport(doc: BfmDocument, report: Report): void {
  let failures = 0;

  // 1. 配置
  const KEYS = ['Left', 'Top', 'Width', 'Height'] as const;
  for (const location of walkNodes(doc)) {
    if (location.kind !== 'control') continue;
    const { name, properties = {} } = location.node;
    const actual = report.bounds[name];
    // AutoSize のコントロールの大きさは LCL が内容(文字列・フォント)から決める。デザイナーが書くのは見積もりなので比べない
    const autoSize =
      properties.AutoSize ?? findClass(location.node.class)?.properties.AutoSize?.default;
    const keys = autoSize === true ? KEYS.slice(0, 2) : KEYS;
    const mismatched = keys.filter((key, i) => {
      const expected = properties[key];
      return typeof expected === 'number' && actual?.[i] !== expected;
    });
    if (mismatched.length > 0) {
      failures++;
      const expected = KEYS.map((k) => JSON.stringify(properties[k] ?? '-')).join(',');
      console.error(`  ✗ 配置 ${name}: 期待 [${expected}] / 実際 [${actual?.join(',') ?? '-'}]`);
    }
  }
  if (failures === 0)
    console.log(`✓ 配置: ${String(Object.keys(report.bounds).length)} コントロール`);

  // 2. イベント
  const missingCalls = EXPECTED_CALLS.filter((c) => !report.calls.includes(c));
  if (missingCalls.length === 0) {
    console.log(`✓ イベント: ${report.calls.join(', ')}`);
  } else {
    failures++;
    console.error(
      `  ✗ イベント: 呼ばれていない ${missingCalls.join(', ')} / 実際 ${report.calls.join(', ')}`,
    );
  }

  // 3. 参照・入れ子のオブジェクト・TStrings・メニュー
  const wrongChecks = Object.entries(EXPECTED_CHECKS).filter(([k, v]) => report.checks[k] !== v);
  if (wrongChecks.length === 0) {
    console.log(`✓ プロパティ: ${Object.keys(EXPECTED_CHECKS).join(', ')}`);
  } else {
    failures++;
    for (const [k, v] of wrongChecks)
      console.error(`  ✗ プロパティ ${k}: 期待 ${v} / 実際 ${report.checks[k] ?? '-'}`);
  }

  if (failures > 0) {
    console.error(`\n${String(failures)} 件の不一致があります`);
    process.exitCode = 1;
  } else {
    console.log('\nすべて一致しました');
  }
}
