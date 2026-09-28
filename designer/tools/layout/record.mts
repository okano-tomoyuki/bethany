/**
 * 配置の見本(packages/core/src/layout/fixtures/*.bfm.json)を実物の LCL で表示して、配置を記録する
 * (docs/designer/editor-design.md §4.4。ADR 0036)。
 *
 *   node tools/layout/record.mts           見本ごとに <見本>.lcl.json を書く
 *   node tools/layout/record.mts --check   記録と今の LCL の結果が食い違っていないか調べる(書かない)
 *
 * 見本から Python のコードを生成し(verify-python と同じ)、py/beth で表示して、各コントロールの Left・Top・Width・Height を
 * 読む。続けてフォームの Width・Height を広げ、Anchors で追従した後の配置も読む。
 *
 * 必要なもの: Python 3(環境変数 PYTHON で指定できる)と、ビルド済みの Bethany の DLL。記録は Windows で行う(ADR 0036)。
 */
import { copyFileSync, cpSync, mkdirSync, readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { execFileSync } from 'node:child_process';
import { parseDocument, walkNodes, type BfmDocument } from '../../packages/core/src/index.ts';
import { generatePython } from '../../packages/codegen/src/index.ts';
import type { LayoutRecord } from '../../packages/core/src/layout/fixtures/record.ts';
import { designerRoot, repoRoot, writeIfChanged } from '../codegen/report.mts';

export const RESIZE = { width: 100, height: 50 } as const;

const fixturesDir = join(designerRoot, 'packages/core/src/layout/fixtures');
const workDir = join(designerRoot, '.cache/layout-record');
const isWindows = process.platform === 'win32';
const dllName = isWindows ? 'beth.dll' : 'libbeth.so';
const python = process.env.PYTHON ?? (isWindows ? 'python' : 'python3');
const check = process.argv.includes('--check');

mkdirSync(workDir, { recursive: true });
// Python のパッケージ(py/beth)を写し、その中にリポジトリ直下の DLL を置く(パッケージは自身と同じフォルダの DLL を読み込む)
cpSync(join(repoRoot, 'py', 'beth'), join(workDir, 'beth'), {
  recursive: true,
  filter: (src) => !src.endsWith('__pycache__'),
});
copyFileSync(join(repoRoot, dllName), join(workDir, 'beth', dllName));

let mismatches = 0;
for (const file of readdirSync(fixturesDir)
  .filter((f) => f.endsWith('.bfm.json'))
  .sort()) {
  const doc = load(join(fixturesDir, file));
  const result = generatePython(doc, file, undefined);
  if (!result.ok) throw new Error(`${file}: ${result.error}`);
  writeIfChanged(join(workDir, 'fixture.py'), result.text);
  writeIfChanged(join(workDir, 'record.py'), harness(doc));
  const output = runPython('record.py');
  const record = JSON.parse(lastLine(output)) as LayoutRecord;
  const text = `${JSON.stringify(record, null, 2).replace(/\[\s+(-?\d+),\s+(-?\d+),\s+(-?\d+),\s+(-?\d+)\s+\]/g, '[$1, $2, $3, $4]')}\n`;
  save(join(fixturesDir, file.replace(/\.bfm\.json$/, '.lcl.json')), text);
}

// ---- クライアント領域の余白と、生成しただけでは大きさが 0 のクラスの大きさ(metrics.json) ----

writeIfChanged(join(workDir, 'metrics.py'), metricsScript());
const metrics = lastLine(runPython('metrics.py'));
save(
  join(designerRoot, 'packages/core/src/layout/metrics.json'),
  `${JSON.stringify(JSON.parse(metrics), null, 2)}\n`,
);

if (mismatches > 0) process.exitCode = 1;

function runPython(script: string): string {
  return execFileSync(python, [script], {
    encoding: 'utf8',
    cwd: workDir,
    timeout: 60_000,
    env: { ...process.env, PYTHONIOENCODING: 'utf-8', PYTHONDONTWRITEBYTECODE: '1' },
  });
}

function lastLine(output: string): string {
  return output.trim().split(/\r?\n/).pop() ?? '';
}

/** 記録を書く(--check なら、今の記録と比べるだけ) */
function save(path: string, text: string): void {
  if (!check) {
    writeIfChanged(path, text);
    console.log(`記録しました: ${path}`);
    return;
  }
  let current: string;
  try {
    current = readFileSync(path, 'utf8').replace(/\r\n/g, '\n');
  } catch {
    current = '';
  }
  if (current === text) {
    console.log(`✓ ${path}`);
  } else {
    mismatches++;
    console.error(`✗ ${path}: 記録と食い違う`);
  }
}

function load(path: string): BfmDocument {
  const { document, diagnostics } = parseDocument(readFileSync(path, 'utf8'));
  if (!document || diagnostics.length > 0)
    throw new Error(`${path}: 検証を通りません: ${JSON.stringify(diagnostics)}`);
  return document;
}

function harness(doc: BfmDocument): string {
  const controls = [...walkNodes(doc)].flatMap((n) => (n.kind === 'control' ? [n.node.name] : []));
  return `import json

import fixture
from beth import *

Application.Initialize()
f = Application.CreateForm(fixture.T${doc.form.name})
f.Show()
for _ in range(10):
    Application.ProcessMessages()


def bounds():
    result = {}
    for name in ${JSON.stringify(controls)}:
        c = getattr(f, name)
        result[name] = [c.Left, c.Top, c.Width, c.Height]
    return result


shown = bounds()
f.Width = f.Width + ${String(RESIZE.width)}
f.Height = f.Height + ${String(RESIZE.height)}
for _ in range(10):
    Application.ProcessMessages()
resized = bounds()
f.Close()
for _ in range(5):
    Application.ProcessMessages()
print(json.dumps({"shown": shown, "resize": {"width": ${String(RESIZE.width)}, "height": ${String(RESIZE.height)}}, "resized": resized}))
`;
}

/**
 * コンテナのクライアント領域を、Windows のウィンドウの位置(ctypes の GetWindowRect)で測る。
 * Bethany にはウィンドウのハンドルや座標の変換が無いため、フォームをタイトルで探し、子のウィンドウを Caption で見分ける。
 *
 * - origin: 子の座標の原点(子の Left・Top が 0 の位置)の、コンテナの外側の左上からの位置
 * - insets: Align で寄せる範囲の、コンテナの外側からの余白(alClient の子の位置から求める)
 */
function metricsScript(): string {
  return String.raw`import ctypes
import ctypes.wintypes as wt
import json

from beth import *

user32 = ctypes.windll.user32
Application.Initialize()
f = Application.CreateForm(TForm)
f.Caption = "beth-metrics"
f.Width = 1000
f.Height = 400

W, H = 190, 150
probes = {}
fills = {}
containers = {}
for i, cls in enumerate(["TPanel", "TGroupBox", "TScrollBox", "TTabControl", "TPageControl"]):
    c = globals()[cls](f)
    c.Parent = f
    c.Left = i * 200
    c.Top = 0
    c.Width = W
    c.Height = H
    host = c
    if cls == "TPageControl":
        host = TTabSheet(f)
        host.PageControl = c
        host.Caption = "Tab"
    if cls == "TTabControl":
        c.Tabs.Add("Tab")
    probe = TPanel(f)
    probe.Parent = host
    probe.Left = 5
    probe.Top = 7
    probe.Width = 20
    probe.Height = 20
    probe.Caption = "probe-" + cls
    fill = TPanel(f)
    fill.Parent = host
    fill.Align = alClient
    containers[cls] = c
    probes[cls] = probe
    fills[cls] = fill

bar = TStatusBar(f)
bar.Parent = f
f.Show()
for _ in range(10):
    Application.ProcessMessages()

top = user32.FindWindowW(None, "beth-metrics")
rects = {}


@ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
def visit(h, _):
    text = ctypes.create_unicode_buffer(256)
    user32.GetWindowTextW(h, text, 256)
    r = wt.RECT()
    user32.GetWindowRect(h, ctypes.byref(r))
    rects[text.value] = (r.left, r.top)
    return True


user32.EnumChildWindows(top, visit, 0)
client = wt.POINT(0, 0)
user32.ClientToScreen(top, ctypes.byref(client))

insets = {}
for cls, c in containers.items():
    px, py = rects["probe-" + cls]
    # コンテナの外側の左上(画面の座標)。フォームのクライアント領域からの位置で求める
    ox, oy = client.x + c.Left, client.y + c.Top
    origin_x, origin_y = px - ox - 5, py - oy - 7
    fill = fills[cls]
    left, top_ = origin_x + fill.Left, origin_y + fill.Top
    insets[cls] = {
        "origin": [origin_x, origin_y],
        "left": left,
        "top": top_,
        "right": W - left - fill.Width,
        "bottom": H - top_ - fill.Height,
    }

sizes = {"TStatusBar": {"height": bar.Height}}
print(json.dumps({"insets": insets, "sizes": sizes}))
`;
}
