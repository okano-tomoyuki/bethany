/**
 * 生成した C++ のコードを、本リポジトリの no_vcl と一緒にビルド・実行して確かめる(docs/designer/codegen-design.md §7)。
 *
 *   node tools/codegen/verify-cpp.mts
 *
 * 見本(samples/*.nvform.json)ごとに、コードを生成して CMake でビルドし、フォームを表示した後に次を確かめる。
 * 1. 各コントロールの位置と大きさが、DSL に書いた値(デザイナーが計算した配置)と一致すること。
 * 2. イベントが接続されていること(LCL を通して発生させたイベントで、ハンドラが呼ばれる)。
 * 3. 参照・入れ子のオブジェクト・TStrings・メニューが設定されていること。
 *
 * 必要なもの: C++ コンパイラ・CMake・Ninja と、ビルド済みの no_vcl の DLL(リポジトリ直下の no_vcl.dll / libno_vcl.so。
 * build-windows.sh・build-linux.sh で作る)。ビルドの作業フォルダは .cache/verify-cpp(2 回目以降は差分ビルドになる)。
 */
import { copyFileSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import {
  findClass,
  parseDocument,
  walkNodes,
  type NvformDocument,
} from '../../packages/core/src/index.ts';
import { generateCpp } from '../../packages/codegen/src/index.ts';

const designerRoot = fileURLToPath(new URL('../../', import.meta.url));
const repoRoot = join(designerRoot, '..');
const workDir = join(designerRoot, '.cache/verify-cpp');
const buildDir = join(workDir, 'build');
const isWindows = process.platform === 'win32';
const dllName = isWindows ? 'no_vcl.dll' : 'libno_vcl.so';
let failures = 0;

// ---- コードの生成 --------------------------------------------------------------

mkdirSync(workDir, { recursive: true });
const sampleFile = join(designerRoot, 'samples/MainForm.nvform.json');
const doc = load(readFileSync(sampleFile, 'utf8'));
const result = generateCpp(doc, 'MainForm.nvform.json', undefined, undefined);
if ('error' in result) throw new Error(result.error);
if (!result.header.ok || !result.source.ok) throw new Error('生成に失敗しました');
writeIfChanged(join(workDir, 'MainForm.hpp'), result.header.text);
writeIfChanged(join(workDir, 'MainForm.cpp'), recordHandlerCalls(result.source.text, 'TMainForm'));
writeIfChanged(join(workDir, 'main.cpp'), harness(doc));
writeIfChanged(join(workDir, 'CMakeLists.txt'), cmakeLists());

// ---- ビルド ---------------------------------------------------------------------

console.log(`ビルドしています(no_vcl: ${repoRoot})…`);
execFileSync(
  'cmake',
  ['-S', workDir, '-B', buildDir, '-G', 'Ninja', `-DNO_VCL_DIR=${repoRoot.replace(/\\/g, '/')}`],
  { stdio: ['ignore', 'ignore', 'inherit'] },
);
try {
  execFileSync('cmake', ['--build', buildDir], { stdio: ['ignore', 'pipe', 'inherit'] });
} catch (e) {
  // Ninja はコンパイルエラーを標準出力に書く
  console.error((e as { stdout?: Buffer }).stdout?.toString() ?? '');
  throw new Error('ビルドに失敗しました', { cause: e });
}
copyFileSync(join(repoRoot, dllName), join(buildDir, dllName));

// ---- 実行 ---------------------------------------------------------------------

interface Report {
  readonly bounds: Readonly<Record<string, readonly number[]>>;
  readonly calls: readonly string[];
  readonly checks: Readonly<Record<string, string>>;
}

const output = execFileSync(join(buildDir, isWindows ? 'verify.exe' : 'verify'), {
  encoding: 'utf8',
  cwd: buildDir,
  timeout: 60_000,
  env: { ...process.env, LD_LIBRARY_PATH: buildDir },
});
const report = JSON.parse(output.trim().split(/\r?\n/).pop() ?? '') as Report;

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
const expectedCalls = [
  'FormCreate',
  'NameEditChange',
  'WrapCheckClick',
  'FileOpenItemClick',
  'ClearItemClick',
  'OkButtonClick',
  'Timer1Timer',
  'FormCloseQuery',
];
const missingCalls = expectedCalls.filter((c) => !report.calls.includes(c));
if (missingCalls.length === 0) {
  console.log(`✓ イベント: ${report.calls.join(', ')}`);
} else {
  failures++;
  console.error(
    `  ✗ イベント: 呼ばれていない ${missingCalls.join(', ')} / 実際 ${report.calls.join(', ')}`,
  );
}

// 3. 参照・入れ子のオブジェクト・TStrings・メニュー
const expectedChecks: Record<string, string> = {
  menu: '1',
  activePage: '1',
  popupMenu: '1',
  memoLines: 'line 1|line "2"',
  okFontBold: '1',
  statusFont: '10/16711680',
  panelColor: '12644607',
  shortCut: 'Ctrl+O',
  menuCounts: '1/3/1',
  dialogOptions: String((1 << 20) | (1 << 23) | (1 << 9)),
  timer: '0/500',
  caption: 'Sample',
  spinValue: '150',
  anchors: '404/380/500',
};
const wrongChecks = Object.entries(expectedChecks).filter(([k, v]) => report.checks[k] !== v);
if (wrongChecks.length === 0) {
  console.log(`✓ プロパティ: ${Object.keys(expectedChecks).join(', ')}`);
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

// ---- 補助 ---------------------------------------------------------------------

function load(text: string): NvformDocument {
  const { document, diagnostics } = parseDocument(text);
  if (!document || diagnostics.length > 0)
    throw new Error(`見本が検証を通りません: ${JSON.stringify(diagnostics)}`);
  return document;
}

/** 内容が同じなら書き換えない(差分ビルドを効かせる) */
function writeIfChanged(path: string, text: string): void {
  let current: string | undefined;
  try {
    current = readFileSync(path, 'utf8');
  } catch {
    current = undefined;
  }
  if (current !== text) writeFileSync(path, text);
}

/** ハンドラの雛形の中身を、呼ばれたことを記録する処理に置き換える */
function recordHandlerCalls(source: string, className: string): string {
  const withDecl = source.replace(
    'using namespace no_vcl;\n',
    'using namespace no_vcl;\n\n#include <string>\n#include <vector>\nextern std::vector<std::string> nvd_calls;\n',
  );
  // className は識別子のため、正規表現の特殊文字を含まない
  return withDecl.replace(
    new RegExp(`void ${className}::(\\w+)\\(([^)]*)\\)\\n\\{\\n {4}// TODO: implement\\n\\}`, 'g'),
    (_, name: string, params: string) =>
      `void ${className}::${name}(${params})\n{\n    nvd_calls.push_back("${name}");\n}`,
  );
}

function harness(document: NvformDocument): string {
  const controls = [...walkNodes(document)].flatMap((n) =>
    n.kind === 'control' ? [n.node.name] : [],
  );
  return `#include "MainForm.hpp"
#include <cstdio>
#include <string>
#include <vector>

using namespace no_vcl;

std::vector<std::string> nvd_calls;

static std::string json(const std::string& s)
{
    std::string out = "\\"";
    for (char c : s) {
        if (c == '"' || c == '\\\\') out += '\\\\';
        out += c;
    }
    return out + "\\"";
}

static void check(bool& first, const char* key, const std::string& value)
{
    std::printf("%s%s:%s", first ? "" : ",", json(key).c_str(), json(value).c_str());
    first = false;
}

int main()
{
    Application->Initialize();
    Application->CreateForm(&MainForm);
    TMainForm* f = MainForm;
    f->Show();
    for (int i = 0; i < 10; ++i) Application->ProcessMessages();

    std::printf("{\\"bounds\\":{");
    bool first = true;
${controls
  .map(
    (name) =>
      `    std::printf("%s\\"${name}\\":[%d,%d,%d,%d]", first ? "" : ",", (int)f->${name}->Left, (int)f->${name}->Top, (int)f->${name}->Width, (int)f->${name}->Height);\n    first = false;`,
  )
  .join('\n')}
    std::printf("},\\"checks\\":{");
    first = true;
    check(first, "menu", f->Menu == f->MainMenu1 ? "1" : "0");
    check(first, "activePage", std::to_string(f->PageControl1->ActivePageIndex));
    check(first, "popupMenu", f->Memo1->PopupMenu == f->PopupMenu1 ? "1" : "0");
    check(first, "memoLines", std::string(f->Memo1->Lines->Strings[0]) + "|" + std::string(f->Memo1->Lines->Strings[1]));
    check(first, "okFontBold", (f->OkButton->Font->Style & fsBold) ? "1" : "0");
    check(first, "statusFont", std::to_string(f->StatusLabel->Font->Size) + "/" + std::to_string(f->StatusLabel->Font->Color));
    check(first, "panelColor", std::to_string(f->BottomPanel->Color));
    check(first, "shortCut", ShortCutToText(f->FileOpenItem->ShortCut));
    check(first, "menuCounts", std::to_string(f->MainMenu1->Items->Count) + "/" + std::to_string(f->FileMenu->Count) + "/" + std::to_string(f->PopupMenu1->Items->Count));
    check(first, "dialogOptions", std::to_string(f->OpenDialog1->Options));
    check(first, "timer", std::to_string((int)f->Timer1->Enabled) + "/" + std::to_string(f->Timer1->Interval));
    check(first, "caption", f->Caption);
    check(first, "spinValue", std::to_string(f->SizeSpin->Value));

    // Anchors: フォームを広げると、右に寄せたものは動き、左右に寄せたものは広がる
    f->Width = f->Width + 100;
    for (int i = 0; i < 5; ++i) Application->ProcessMessages();
    check(first, "anchors", std::to_string(f->OkButton->Left) + "/" + std::to_string(f->NameEdit->Width) + "/" + std::to_string(f->BottomPanel->Width));

    // LCL を通してイベントを発生させる
    f->NameEdit->Text = "changed";
    f->WrapCheck->Checked = false;
    f->FileOpenItem->Click();
    f->ClearItem->Click();
    // TButton・TTimer はプログラムから発生させる手段が無いので、接続されたハンドラを呼ぶ
    TNotifyEvent onClick = f->OkButton->OnClick;
    if (onClick) onClick(f->OkButton);
    TNotifyEvent onTimer = f->Timer1->OnTimer;
    if (onTimer) onTimer(f->Timer1);
    f->Close();
    for (int i = 0; i < 5; ++i) Application->ProcessMessages();

    std::printf("},\\"calls\\":[");
    for (size_t i = 0; i < nvd_calls.size(); ++i) std::printf("%s%s", i ? "," : "", json(nvd_calls[i]).c_str());
    std::printf("]}\\n");
    std::fflush(stdout);
    return 0;
}
`;
}

function cmakeLists(): string {
  return `cmake_minimum_required(VERSION 3.15)
project(nvd_verify_cpp CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(no_vcl STATIC \${NO_VCL_DIR}/no_vcl.cpp \${NO_VCL_DIR}/internal/api.cpp)
target_include_directories(no_vcl PUBLIC \${NO_VCL_DIR})
target_link_libraries(no_vcl PRIVATE \${CMAKE_DL_LIBS})

add_executable(verify MainForm.cpp main.cpp)
target_include_directories(verify PRIVATE \${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(verify PRIVATE no_vcl)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # 生成したコードに警告が出ないこと(未使用の引数はハンドラの雛形なので除く)も確かめる
    target_compile_options(verify PRIVATE -Wall -Wextra -Wno-unused-parameter -Werror)
endif()
set_target_properties(verify PROPERTIES RUNTIME_OUTPUT_DIRECTORY \${CMAKE_BINARY_DIR})
`;
}
