/**
 * 生成した C++ のコードを、本リポジトリの Bethany と一緒にビルド・実行して確かめる(docs/designer/codegen-design.md §7)。
 *
 *   node tools/codegen/verify-cpp.mts
 *
 * 見本(samples/*.bfm.json)ごとに、コードを生成して CMake でビルドし、フォームを表示した後に次を確かめる。
 * 1. 各コントロールの位置と大きさが、DSL に書いた値(デザイナーが計算した配置)と一致すること。
 * 2. イベントが接続されていること(LCL を通して発生させたイベントで、ハンドラが呼ばれる)。
 * 3. 参照・入れ子のオブジェクト・TStrings・メニューが設定されていること。
 *
 * 必要なもの: C++ コンパイラ・CMake・Ninja と、ビルド済みの Bethany の DLL(リポジトリ直下の beth.dll / libbeth.so。
 * build-windows.sh・build-linux.sh で作る)。ビルドの作業フォルダは .cache/verify-cpp(2 回目以降は差分ビルドになる)。
 */
import { copyFileSync, mkdirSync } from 'node:fs';
import { join } from 'node:path';
import { execFileSync } from 'node:child_process';
import { walkNodes, type BfmDocument } from '../../packages/core/src/index.ts';
import { generateCpp } from '../../packages/codegen/src/index.ts';
import {
  checkReport,
  designerRoot,
  loadSample,
  parseReport,
  repoRoot,
  SAMPLE_FILE,
  writeIfChanged,
} from './report.mts';

const workDir = join(designerRoot, '.cache/verify-cpp');
const buildDir = join(workDir, 'build');
const isWindows = process.platform === 'win32';
const dllName = isWindows ? 'beth.dll' : 'libbeth.so';

// ---- コードの生成 --------------------------------------------------------------

mkdirSync(workDir, { recursive: true });
const doc = loadSample();
const result = generateCpp(doc, SAMPLE_FILE, undefined, undefined);
if (!result.header.ok || !result.source.ok) throw new Error('生成に失敗しました');
writeIfChanged(join(workDir, 'MainForm.hpp'), result.header.text);
writeIfChanged(join(workDir, 'MainForm.cpp'), recordHandlerCalls(result.source.text, 'TMainForm'));
writeIfChanged(join(workDir, 'main.cpp'), harness(doc));
writeIfChanged(join(workDir, 'CMakeLists.txt'), cmakeLists());

// ---- ビルド ---------------------------------------------------------------------

console.log(`ビルドしています(Bethany: ${repoRoot})…`);
execFileSync(
  'cmake',
  ['-S', workDir, '-B', buildDir, '-G', 'Ninja', `-DBETH_DIR=${repoRoot.replace(/\\/g, '/')}`],
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

const output = execFileSync(join(buildDir, isWindows ? 'verify.exe' : 'verify'), {
  encoding: 'utf8',
  cwd: buildDir,
  timeout: 60_000,
  env: { ...process.env, LD_LIBRARY_PATH: buildDir },
});
checkReport(doc, parseReport(output));

// ---- 補助 ---------------------------------------------------------------------

/** ハンドラの雛形の中身を、呼ばれたことを記録する処理に置き換える */
function recordHandlerCalls(source: string, className: string): string {
  const withDecl = source.replace(
    'using namespace beth;\n',
    'using namespace beth;\n\n#include <string>\n#include <vector>\nextern std::vector<std::string> beth_calls;\n',
  );
  // className は識別子のため、正規表現の特殊文字を含まない
  return withDecl.replace(
    new RegExp(`void ${className}::(\\w+)\\(([^)]*)\\)\\n\\{\\n {4}// TODO: implement\\n\\}`, 'g'),
    (_, name: string, params: string) =>
      `void ${className}::${name}(${params})\n{\n    beth_calls.push_back("${name}");\n}`,
  );
}

function harness(document: BfmDocument): string {
  const controls = [...walkNodes(document)].flatMap((n) =>
    n.kind === 'control' ? [n.node.name] : [],
  );
  return `#include "MainForm.hpp"
#include <cstdio>
#include <string>
#include <vector>

using namespace beth;

std::vector<std::string> beth_calls;

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
    check(first, "actionLink", std::string(f->FileSaveItem->Caption) + "/" + ShortCutToText(f->FileSaveItem->ShortCut) + "/" + std::to_string(f->ActionList1->ActionCount));
    check(first, "allowDropFiles", std::to_string((int)f->AllowDropFiles));
    check(first, "panelBevel", std::to_string((int)(TPanelBevel)f->BottomPanel->BevelOuter) + "/" + std::to_string((int)(TPanelBevel)f->BottomPanel->BevelInner) + "/" + std::to_string(f->BottomPanel->BevelWidth) + "/" + std::to_string(f->BottomPanel->BorderWidth));
    check(first, "groupColumns", std::to_string(f->RadioGroup1->Columns) + "/" + std::to_string((int)(TColumnLayout)f->RadioGroup1->ColumnLayout));
    check(first, "scrollChange", std::to_string(f->ScrollBar1->LargeChange) + "/" + std::to_string(f->ScrollBar1->SmallChange));
    check(first, "memoBorder", std::to_string((int)(TBorderStyle)f->Memo1->BorderStyle) + "/" + std::to_string((int)(TScrollStyle)f->Memo1->ScrollBars));
    check(first, "timer", std::to_string((int)f->Timer1->Enabled) + "/" + std::to_string(f->Timer1->Interval));
    check(first, "caption", f->Caption);
    check(first, "spinValue", std::to_string(f->SizeSpin->Value));
    check(first, "statusPanels", std::to_string(f->OptionStatus->Panels->Count) + "/" + std::string(f->OptionStatus->Panels->Items[0]->Text) + "/" + std::to_string(f->OptionStatus->Panels->Items[0]->Width) + "/" + std::to_string((int)(TStatusPanelStyle)f->OptionStatus->Panels->Items[1]->Style) + "/" + std::to_string((int)(TAlignment)f->OptionStatus->Panels->Items[2]->Alignment) + "/" + std::to_string((int)(TStatusPanelBevel)f->OptionStatus->Panels->Items[2]->Bevel));

    // Anchors: フォームを広げると、右に寄せたものは動き、左右に寄せたものは広がる
    f->Width = f->Width + 100;
    for (int i = 0; i < 5; ++i) Application->ProcessMessages();
    check(first, "anchors", std::to_string(f->OkButton->Left) + "/" + std::to_string(f->NameEdit->Width) + "/" + std::to_string(f->BottomPanel->Width));

    // LCL を通してイベントを発生させる
    f->NameEdit->Text = "changed";
    f->WrapCheck->Checked = false;
    f->FileOpenItem->Click();
    f->FileSaveItem->Click();
    f->ClearItem->Click();
    f->RadioGroup1->ItemIndex = 1;  // LCL は代入でも OnSelectionChanged を呼ぶ
    // TButton・TTimer はプログラムから発生させる手段が無いので、接続されたハンドラを呼ぶ
    TNotifyEvent onClick = f->OkButton->OnClick;
    if (onClick) onClick(f->OkButton);
    TNotifyEvent onTimer = f->Timer1->OnTimer;
    if (onTimer) onTimer(f->Timer1);
    TScrollEvent onScroll = f->ScrollBar1->OnScroll;
    int scrollPos = 5;
    if (onScroll) onScroll(f->ScrollBar1, scLineDown, scrollPos);
    TDropFilesEvent onDropFiles = f->OnDropFiles;
    if (onDropFiles) onDropFiles(f, {"C:/temp/a.txt"});
    f->Close();
    for (int i = 0; i < 5; ++i) Application->ProcessMessages();

    std::printf("},\\"calls\\":[");
    for (size_t i = 0; i < beth_calls.size(); ++i) std::printf("%s%s", i ? "," : "", json(beth_calls[i]).c_str());
    std::printf("]}\\n");
    std::fflush(stdout);
    return 0;
}
`;
}

function cmakeLists(): string {
  return `cmake_minimum_required(VERSION 3.15)
project(beth_verify_cpp CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(beth STATIC \${BETH_DIR}/src/beth.cpp \${BETH_DIR}/src/internal/api.cpp)
target_include_directories(beth PUBLIC \${BETH_DIR}/include)
target_link_libraries(beth PRIVATE \${CMAKE_DL_LIBS})

add_executable(verify MainForm.cpp main.cpp)
target_include_directories(verify PRIVATE \${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(verify PRIVATE beth)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # 生成したコードに警告が出ないこと(未使用の引数はハンドラの雛形なので除く)も確かめる
    target_compile_options(verify PRIVATE -Wall -Wextra -Wno-unused-parameter -Werror)
endif()
set_target_properties(verify PROPERTIES RUNTIME_OUTPUT_DIRECTORY \${CMAKE_BINARY_DIR})
if(WIN32)
    # beth::beth をリンクした exe と同じく Common-Controls 6.0 の manifest を埋め込む(無いと comctl32 v5 になり、
    # TStatusBar の高さ等が実際のアプリ・python.exe と変わる)
    enable_language(RC)
    set(BETH_MANIFEST_PATH \${BETH_DIR}/win32/beth.manifest)
    configure_file(\${BETH_DIR}/win32/beth_manifest.rc.in \${CMAKE_BINARY_DIR}/beth_manifest.rc @ONLY)
    target_sources(verify PRIVATE \${CMAKE_BINARY_DIR}/beth_manifest.rc)
endif()
`;
}
