/**
 * 生成した Python のコードを、本リポジトリの Bethany の Python のバインディング(py/beth)で実行して確かめる
 * (docs/designer/codegen-design.md §7)。
 *
 *   node tools/codegen/verify-python.mts
 *
 * 確かめる内容と期待は verify-cpp.mts と同じ(report.mts)。C++ と Python で同じ結果になることも確かめている。
 *
 * 必要なもの: Python 3(環境変数 PYTHON で指定できる。既定は Windows では python、それ以外は python3)と、
 * ビルド済みの Bethany の DLL(リポジトリ直下の beth.dll / libbeth.so)。
 * バインディング(py/*.py)と DLL を作業フォルダ .cache/verify-python に写して実行する(py/ には何も書き込まない)。
 */
import { copyFileSync, cpSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';
import { execFileSync } from 'node:child_process';
import { walkNodes, type BfmDocument } from '../../packages/core/src/index.ts';
import { generatePython } from '../../packages/codegen/src/index.ts';
import {
  checkReport,
  designerRoot,
  loadSample,
  parseReport,
  repoRoot,
  SAMPLE_FILE,
  writeIfChanged,
} from './report.mts';

const workDir = join(designerRoot, '.cache/verify-python');
const isWindows = process.platform === 'win32';
const dllName = isWindows ? 'beth.dll' : 'libbeth.so';
const python = process.env.PYTHON ?? (isWindows ? 'python' : 'python3');

// ---- コードの生成 --------------------------------------------------------------

mkdirSync(workDir, { recursive: true });
const doc = loadSample();
const result = generatePython(doc, SAMPLE_FILE, undefined);
if (!result.ok) throw new Error(result.error);
writeIfChanged(join(workDir, 'MainForm.py'), recordHandlerCalls(result.text));
writeIfChanged(join(workDir, 'verify.py'), harness(doc));
// Python のパッケージ(py/beth)を写し、その中にリポジトリ直下の DLL を置く(パッケージは自身と同じフォルダの DLL を読み込む)
cpSync(join(repoRoot, 'py', 'beth'), join(workDir, 'beth'), {
  recursive: true,
  filter: (src) => !src.endsWith('__pycache__'),
});
copyFileSync(join(repoRoot, dllName), join(workDir, 'beth', dllName));

// ---- 実行 ---------------------------------------------------------------------

console.log(`実行しています(${python})…`);
const output = execFileSync(python, ['verify.py'], {
  encoding: 'utf8',
  cwd: workDir,
  timeout: 60_000,
  env: { ...process.env, PYTHONIOENCODING: 'utf-8', PYTHONDONTWRITEBYTECODE: '1' },
});
checkReport(doc, parseReport(output));

// ---- 補助 ---------------------------------------------------------------------

/** ハンドラの雛形の中身(pass)を、呼ばれたことを記録する処理に置き換える */
function recordHandlerCalls(source: string): string {
  return source
    .replace('from beth import *\n', 'from beth import *\n\nbeth_calls = []\n')
    .replace(
      /^( {4}def (\w+)\(self[^)]*\):\n) {8}pass$/gm,
      (_, head: string, name: string) => `${head}        beth_calls.append("${name}")`,
    );
}

function harness(document: BfmDocument): string {
  const controls = [...walkNodes(document)].flatMap((n) =>
    n.kind === 'control' ? [n.node.name] : [],
  );
  return `import json

import MainForm
from beth import *

Application.Initialize()
f = Application.CreateForm(MainForm.TMainForm)
f.Show()
for _ in range(10):
    Application.ProcessMessages()

bounds = {}
for name in ${JSON.stringify(controls)}:
    c = getattr(f, name)
    bounds[name] = [c.Left, c.Top, c.Width, c.Height]

checks = {}
checks["menu"] = "1" if f.Menu is f.MainMenu1 else "0"
checks["activePage"] = str(f.PageControl1.ActivePageIndex)
checks["popupMenu"] = "1" if f.Memo1.PopupMenu is f.PopupMenu1 else "0"
checks["memoLines"] = f.Memo1.Lines.Strings[0] + "|" + f.Memo1.Lines.Strings[1]
checks["okFontBold"] = "1" if f.OkButton.Font.Style & fsBold else "0"
checks["statusFont"] = f"{f.StatusLabel.Font.Size}/{int(f.StatusLabel.Font.Color)}"
checks["panelColor"] = str(int(f.BottomPanel.Color))
checks["shortCut"] = ShortCutToText(f.FileOpenItem.ShortCut)
checks["menuCounts"] = f"{f.MainMenu1.Items.Count}/{f.FileMenu.Count}/{f.PopupMenu1.Items.Count}"
checks["dialogOptions"] = str(int(f.OpenDialog1.Options))
checks["actionLink"] = f"{f.FileSaveItem.Caption}/{ShortCutToText(f.FileSaveItem.ShortCut)}/{f.ActionList1.ActionCount}"
checks["allowDropFiles"] = str(int(f.AllowDropFiles))
checks["panelBevel"] = f"{int(f.BottomPanel.BevelOuter)}/{int(f.BottomPanel.BevelInner)}/{f.BottomPanel.BevelWidth}/{f.BottomPanel.BorderWidth}"
checks["groupColumns"] = f"{f.RadioGroup1.Columns}/{int(f.RadioGroup1.ColumnLayout)}"
checks["scrollChange"] = f"{f.ScrollBar1.LargeChange}/{f.ScrollBar1.SmallChange}"
checks["ownerDraw"] = f"{int(f.ColorList.Style)}/{f.ColorList.ItemHeight}"
checks["treeView"] = f"{int(f.Tree1.SortType)}/{f.Tree1.Indent}/{int(f.Tree1.MultiSelect)}"
checks["memoBorder"] = f"{int(f.Memo1.BorderStyle)}/{int(f.Memo1.ScrollBars)}"
checks["timer"] = f"{int(f.Timer1.Enabled)}/{f.Timer1.Interval}"
checks["caption"] = f.Caption
checks["spinValue"] = str(f.SizeSpin.Value)
_panels = f.OptionStatus.Panels
checks["statusPanels"] = f"{_panels.Count}/{_panels.Items[0].Text}/{_panels.Items[0].Width}/{int(_panels.Items[1].Style)}/{int(_panels.Items[2].Alignment)}/{int(_panels.Items[2].Bevel)}"

# Anchors: フォームを広げると、右に寄せたものは動き、左右に寄せたものは広がる
f.Width = f.Width + 100
for _ in range(5):
    Application.ProcessMessages()
checks["anchors"] = f"{f.OkButton.Left}/{f.NameEdit.Width}/{f.BottomPanel.Width}"

# LCL を通してイベントを発生させる
f.NameEdit.Text = "changed"
f.WrapCheck.Checked = False
f.FileOpenItem.Click()
f.FileSaveItem.Click()
f.ClearItem.Click()
f.RadioGroup1.ItemIndex = 1  # LCL は代入でも OnSelectionChanged を呼ぶ
# TButton・TTimer はプログラムから発生させる手段が無いので、接続されたハンドラを呼ぶ
f.OkButton.OnClick(f.OkButton)
f.Timer1.OnTimer(f.Timer1)
f.ScrollBar1.OnScroll(f.ScrollBar1, scLineDown, Ref(5))
_edited = Ref("x")
f.Tree1.OnEdited(f.Tree1, None, _edited)
checks["treeEdited"] = _edited.value + "!"
f.Tree1.OnCustomDrawItem(f.Tree1, None, cdsSelected, Ref(True))
f.OnDropFiles(f, ["C:/temp/a.txt"])
f.Close()
for _ in range(5):
    Application.ProcessMessages()

print(json.dumps({"bounds": bounds, "checks": checks, "calls": MainForm.beth_calls}, ensure_ascii=False))
`;
}
