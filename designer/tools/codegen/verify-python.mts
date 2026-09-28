/**
 * 生成した Python のコードを、本リポジトリの no_vcl の Python のバインディング(py/no_vcl.py)で実行して確かめる
 * (docs/designer/codegen-design.md §7)。
 *
 *   node tools/codegen/verify-python.mts
 *
 * 確かめる内容と期待は verify-cpp.mts と同じ(report.mts)。C++ と Python で同じ結果になることも確かめている。
 *
 * 必要なもの: Python 3(環境変数 PYTHON で指定できる。既定は Windows では python、それ以外は python3)と、
 * ビルド済みの no_vcl の DLL(リポジトリ直下の no_vcl.dll / libno_vcl.so)。
 * バインディング(py/*.py)と DLL を作業フォルダ .cache/verify-python に写して実行する(py/ には何も書き込まない)。
 */
import { copyFileSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';
import { execFileSync } from 'node:child_process';
import { walkNodes, type NvformDocument } from '../../packages/core/src/index.ts';
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
const dllName = isWindows ? 'no_vcl.dll' : 'libno_vcl.so';
const python = process.env.PYTHON ?? (isWindows ? 'python' : 'python3');

// ---- コードの生成 --------------------------------------------------------------

mkdirSync(workDir, { recursive: true });
const doc = loadSample();
const result = generatePython(doc, SAMPLE_FILE, undefined);
if (!result.ok) throw new Error(result.error);
writeIfChanged(join(workDir, 'MainForm.py'), recordHandlerCalls(result.text));
writeIfChanged(join(workDir, 'verify.py'), harness(doc));
for (const file of ['no_vcl.py', 'no_vcl_core.py', 'no_vcl_internal.py'])
  copyFileSync(join(repoRoot, 'py', file), join(workDir, file));
copyFileSync(join(repoRoot, dllName), join(workDir, dllName));

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
    .replace('from no_vcl import *\n', 'from no_vcl import *\n\nnvd_calls = []\n')
    .replace(
      /^( {4}def (\w+)\(self[^)]*\):\n) {8}pass$/gm,
      (_, head: string, name: string) => `${head}        nvd_calls.append("${name}")`,
    );
}

function harness(document: NvformDocument): string {
  const controls = [...walkNodes(document)].flatMap((n) =>
    n.kind === 'control' ? [n.node.name] : [],
  );
  return `import json

import MainForm
from no_vcl import *

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
checks["timer"] = f"{int(f.Timer1.Enabled)}/{f.Timer1.Interval}"
checks["caption"] = f.Caption
checks["spinValue"] = str(f.SizeSpin.Value)

# Anchors: フォームを広げると、右に寄せたものは動き、左右に寄せたものは広がる
f.Width = f.Width + 100
for _ in range(5):
    Application.ProcessMessages()
checks["anchors"] = f"{f.OkButton.Left}/{f.NameEdit.Width}/{f.BottomPanel.Width}"

# LCL を通してイベントを発生させる
f.NameEdit.Text = "changed"
f.WrapCheck.Checked = False
f.FileOpenItem.Click()
f.ClearItem.Click()
# TButton・TTimer はプログラムから発生させる手段が無いので、接続されたハンドラを呼ぶ
f.OkButton.OnClick(f.OkButton)
f.Timer1.OnTimer(f.Timer1)
f.Close()
for _ in range(5):
    Application.ProcessMessages()

print(json.dumps({"bounds": bounds, "checks": checks, "calls": MainForm.nvd_calls}, ensure_ascii=False))
`;
}
