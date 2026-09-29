# プロジェクトファイルの仕様(formatVersion 1)

アプリケーションに属するフォームと、メインフォームを表すファイル `*.bfproj.json` の仕様。
C++Builder のプロジェクト(`Project1.cbproj`・`Project1.cpp`)のうち、フォームの一覧とメインフォームに当たる。
ビルドの設定(CMake 等)は持たない。

構造は Zod のスキーマ([designer/packages/core/src/project/schema.ts](../../designer/packages/core/src/project/schema.ts))、意味の検証は
[validate.ts](../../designer/packages/core/src/project/validate.ts) を正とし、本書と食い違う場合は本書を直す。

## 1. 基本方針

- **プロジェクトがフォームの一覧を持つ**(C++Builder の `.cbproj` と同じ)。同じフォルダに複数のプロジェクトを置け、
  1 つのフォームを複数のプロジェクトに入れられる(共通のダイアログ等)。フォームの側(`*.bfm.json`)はプロジェクトを知らない。
- **プロジェクトファイルは無くてもよい。** 無ければ、デザイナーはワークスペースのフォームを平らに並べる(これまでどおり)。
- 一覧と実際のファイルのずれは、デザイナーが吸収する(§4)。吸収できないもの(VS Code の外での名前の変更等)は診断で知らせる。

## 2. 例

```json
{
  "formatVersion": 1,
  "codegen": { "cpp": {}, "python": {} },
  "mainForm": "MainForm.bfm.json",
  "forms": ["MainForm.bfm.json", "dialogs/AboutForm.bfm.json"]
}
```

## 3. ドキュメント

| キー | 必須 | 内容 |
|---|---|---|
| `$schema` | | JSON Schema の場所(エディタの補完用) |
| `formatVersion` | ○ | `1` |
| `codegen` | | コード生成の設定(§5)。プロジェクトのフォームと起動部分に効く |
| `mainForm` | | メインフォーム(`Application->CreateForm` で最初に作るフォーム)。`forms` のどれか |
| `forms` | | プロジェクトに属するフォームの一覧。無ければ空 |
| `autoCreate` | | 起動時に作るフォーム(作る順)。`forms` のどれか。無ければ `forms` のすべてを `forms` の順に作る(C++Builder の既定と同じ)。メインフォームは、この一覧によらず常に最初に作る |

- パスは、**プロジェクトファイルのあるフォルダからの相対パス**で、区切りは `/`。`../` で上のフォルダも指せる。
  絶対パス・`\`・ドライブ名は使えない。

### 意味の検証

| 内容 | 重大度 |
|---|---|
| `forms` の各要素が `.bfm.json` で終わる相対パスであること | エラー |
| `forms` に同じフォームが 2 回ないこと(`./a.bfm.json` と `a.bfm.json` は同じ) | エラー |
| `mainForm` が `forms` のどれかであること | エラー |
| `autoCreate` の各要素が `forms` のどれかで、重複しないこと | エラー |
| `forms` の各要素のファイルがあること(拡張が調べる) | 警告 |
| `codegen.cpp.namespace` の `::` で区切った各部分が識別子であること | エラー |
| `codegen.cpp.includeGuardPrefix` が英字・数字・`_` だけで、数字で始まらないこと | エラー |
| `codegen.cpp.headerDir`・`sourceDir` が相対パスであること | エラー |

### 決まった形での書き出し

キーの順は `$schema`・`formatVersion`・`codegen`・`mainForm`・`forms`・`autoCreate`。一覧は 1 行に 1 つ(1 つの追加・削除が 1 行の差分になる)。
インデントは 2 文字、改行は LF、末尾に改行。

## 4. デザイナーでの扱い

- **フォームのビュー**: プロジェクトファイルがあれば、プロジェクトごとにフォームを並べ、メインフォームに ★ を付ける。
  どのプロジェクトにも属さないフォームは「プロジェクトに属さないフォーム」にまとめる。ファイルが無いフォームは警告の印を付ける。
- **メインフォームに設定**(フォームの右クリック): そのフォームのプロジェクトの `mainForm` を書き換える。プロジェクトに属さないフォームなら、
  プロジェクトを選んで加える。プロジェクトが 1 つも無ければ、ワークスペースの先頭のフォルダの直下に `Project1.bfproj.json` を作る。
- **プロジェクトに追加・プロジェクトから外す**(フォームの右クリック)。プロジェクトの最初のフォームは、メインフォームにもする
  (C++Builder で最初のフォームがメインフォームになるのと同じ)。加えたフォームは起動時に作るフォームにもする(`autoCreate` が書かれていれば末尾に加える)。
- **起動時に作成する・作成しない**(フォームの右クリック。メインフォーム以外)。`autoCreate` が無ければ、今の状態(すべてのフォーム)から書き出してから切り替える。
  起動時に作らないフォームには、ビューで「起動時に作成しない」と表示する。
- **新しいプロジェクト**: ワークスペースの先頭のフォルダの直下に、`Project1`・`Project2`…の空いている名前で作る。
  `codegen` は C++ と Python の両方にし、`commentLocale` は作成した人の表示言語にする(新しいフォームと同じ)。
- **新しいフォーム**: プロジェクトの「+」から作ると、そのプロジェクトのフォルダに作って加える。ビューの見出しの「+」から作ると、
  プロジェクトが 1 つならそれに加え、複数なら加える先を選ぶ(加えないこともできる)。
- **名前の変更・移動・削除への追従**: VS Code の中でフォーム(またはそれを含むフォルダ)の名前を変える・移す・消すと、
  それを含むプロジェクトファイルのパスを書き換える(消したものは外す)。プロジェクトファイル自体を別のフォルダへ移したときは、
  相対パスを移した先から計算し直す。

## 5. コード生成の設定(`codegen`)

コード生成の設定は、フォームのファイル(`*.bfm.json`)ではなくプロジェクトファイルに持つ(フォームのファイルは画面の設計だけを持つ。
C++Builder の `.dfm` と `.cbproj` の分け方と同じ)。

| キー | 内容 | 既定値(`Project1.bfproj.json` の場合) |
|---|---|---|
| `commentLocale` | 生成するコードのコメントの言語(`en`・`ja`) | `en` |
| `cpp` | 書けば C++ を生成する。中のキーは下の表 | |
| `python` | 書けば Python を生成する。`python.main` は起動部分の出力先 | `Project1.py` |

`cpp` の中のキー(パスはプロジェクトファイルのフォルダからの相対パスで、区切りは `/`):

| キー | 内容 | 既定値 |
|---|---|---|
| `main` | 起動部分の出力先 | `Project1.cpp` |
| `namespace` | フォームのクラスとフォームの変数を入れる名前空間。`app` や `app::ui`(C++11 でも通るよう、1 つずつ入れ子にして書く) | 無し |
| `includeGuard` | ヘッダのインクルードガード。`macro`(`#ifndef`・`#define`・`#endif`)か `pragma`(`#pragma once`) | `macro` |
| `includeGuardPrefix` | `macro` のマクロ名の先頭に付ける文字列 | 無し |
| `headerExtension` | ヘッダの拡張子(`.hpp`・`.h`・`.hh`・`.hxx`) | `.hpp` |
| `sourceExtension` | ソースの拡張子(`.cpp`・`.cc`・`.cxx`) | `.cpp` |
| `headerDir` | フォームのヘッダを置くフォルダ | フォームと同じフォルダ |
| `sourceDir` | フォームのソースを置くフォルダ | フォームと同じフォルダ |

```json
"codegen": {
  "cpp": {
    "main": "src/Project1.cpp",
    "namespace": "app::ui",
    "headerExtension": ".h",
    "headerDir": "include",
    "sourceDir": "src"
  }
}
```

- **Bethany のヘッダ**は、山括弧のシステムインクルードで `#include <bethany/beth.hpp>` と書く(設定はできない。[ADR 0040](../adr/0040-system-include-path.md))。
- **フォルダ**: `headerDir`・`sourceDir` を書くと、その下の、プロジェクトのフォルダからフォームのフォルダまでと同じ場所に置く
  (`forms/MainForm.bfm.json` なら `include/forms/MainForm.h`・`src/forms/MainForm.cc`)。プロジェクトのフォルダの外にあるフォーム
  (`../` で始まるもの)は、フォームと同じフォルダに置く。
- **include のパス**: `headerDir` を書くと、フォームのソースと起動部分はヘッダを `headerDir` からのパスで include する(`#include "forms/MainForm.h"`)。
  ビルドでは `headerDir` を include パスに入れる(CMake の `target_include_directories`)。書かなければ、include する側のファイルからの相対パスにする。
- **マクロ名**は、`includeGuardPrefix`・名前空間・ヘッダのパス(`headerDir` からの、書かなければファイル名)を `_` でつないで大文字にしたもの
  (上の例なら `APP_UI_FORMS_MAINFORM_H`)。識別子に使えない文字は `_` にし、続いた `_` は 1 つにする(`__` は処理系の予約)。
- 設定を変えて生成し直すと、既存のファイルにも反映する(ファイルの先頭と末尾をマーカー区間にしている。[codegen-design.md](codegen-design.md) §2)。
  ただし、`headerExtension`・`sourceExtension`・`headerDir`・`sourceDir` を変えると新しい場所に作られ、古いファイルは残る(利用者が消す)。

### フォームのコード生成

- 生成する言語は、そのフォームを含むすべてのプロジェクトの和集合(Project1 が C++、Project2 が Python なら両方)。
- コメントの言語は、そのフォームを含むプロジェクトのうち最初に `commentLocale` を書いたもの。フォームを共有するプロジェクトの間で
  `commentLocale` が食い違えば、プロジェクトファイルに警告を出す。
- C++ の設定(`cpp` の `main` 以外)も、そのフォームを含むプロジェクトのうち最初に `cpp` を書いたものを使う。食い違えば同じく警告を出す。
- どのプロジェクトにも属さないフォームは、C++ と Python の両方をコメントは英語で生成する(プロジェクトを作らなくても試せるように)。
- 出力先とクラス名は決まった規則で決まる([dsl-spec.md](dsl-spec.md) §9)。
- CLI(`beth generate MainForm.bfm.json`)は、フォームのフォルダから上へたどってプロジェクトファイルを探す。

### 起動部分のコード生成

C++Builder のプロジェクトのソース(`Project1.cpp`)に当たる、Application を初期化して起動時に作るフォーム(メインフォームが先頭、残りは `autoCreate` の順)を作り、Run するコードを生成する。
フォームのコード生成([codegen-design.md](codegen-design.md))と同じく、書いたターゲットだけを生成し、マーカー区間だけを更新する。

- 出力先の既定をプロジェクト名から決めるのは、同じフォルダに複数のプロジェクトを置けるようにするため(`main.cpp` ではぶつかる)。
- include するヘッダ・import するモジュール・クラス名は、各フォームの名前とファイル名から決める。Python では、フォームの .py が出力先と同じフォルダかその下に無ければ import できないのでエラーにする。
- C++ では、ヘッダに宣言したフォームのグローバル変数(`extern TForm2* Form2;`)に `Application->CreateForm(&Form2)` で代入する。
  C++Builder と同じく、ほかのフォームから `Form2->ShowModal()` と書ける。`namespace` を設定していれば `Application->CreateForm(&app::Form2)` と修飾する。
- Python では、フォームのモジュールの末尾に変数(`Form2: "TForm2" = None`。区間 `beth_FormVariable`)を生成し、起動部分で
  `Form2.Form2 = Application.CreateForm(Form2.TForm2)` と代入する。ほかのフォームからの使い方は下の「ほかのフォームを使う」。
  この区間が無い以前のファイルには、再生成で末尾に加える。
  - 型の注釈に None を含めないのは、C++Builder と同じく起動後は必ず入っている前提にし、`Form2.Form2.ShowModal()` を型チェッカーが
    「None かもしれない」と警告しないようにするため(`None` の代入そのものは、厳しい型チェックでは警告になりうる)。
- 起動時に作らないフォームは、必要になったときに利用者が作る(`new TForm2(Application)`・`TForm2(Application)`)。
- 生成の入口は、フォームのビューのプロジェクトの「コードを生成」、プロジェクトファイルを開いたエディタのタイトル、CLI(`beth generate Project1.bfproj.json`)。

C++(`Project1.cpp`):

```cpp
// Application created with the Bethany designer (Project1.bfproj.json). Regions enclosed in markers are overwritten when regenerated.
// <bethany-designer:begin id="beth_Include">
#include <bethany/beth.hpp>
// <bethany-designer:end id="beth_Include" hash="...">
// <bethany-designer:begin id="includes">
#include "MainForm.hpp"
#include "Form2.hpp"
// <bethany-designer:end id="includes" hash="...">

using namespace beth;

int main()
{
    Application->Initialize();
    // <bethany-designer:begin id="beth_CreateForms">
    Application->CreateForm(&MainForm);
    Application->CreateForm(&Form2);
    // <bethany-designer:end id="beth_CreateForms" hash="...">
    Application->Run();
    return 0;
}
```

Python(`Project1.py`):

```python
"""Application created with the Bethany designer (Project1.bfproj.json). Regions enclosed in markers are overwritten when regenerated."""
from beth import *

# <bethany-designer:begin id="imports">
import MainForm
import Form2
# <bethany-designer:end id="imports" hash="...">


def main():
    Application.Initialize()
    # <bethany-designer:begin id="beth_CreateForms">
    MainForm.MainForm = Application.CreateForm(MainForm.TMainForm)
    Form2.Form2 = Application.CreateForm(Form2.TForm2)
    # <bethany-designer:end id="beth_CreateForms" hash="...">
    Application.Run()


if __name__ == "__main__":
    main()
```

区間の外(`Application->Title` の設定など)は利用者が書き足してよく、再生成しても残る。

### ほかのフォームを使う

C++Builder と同じく、起動時に作ったフォームは、ほかのフォームからフォームの変数で使える。

```cpp
// MainForm.cpp
#include "Form2.hpp"

void TMainForm::OpenButtonClick(TObject* Sender)
{
    Form2->ShowModal();
}
```

```python
# MainForm.py
import Form2


class TMainForm(TForm):
    ...
    def OpenButtonClick(self, Sender):
        Form2.Form2.ShowModal()
```

Python では、フォームどうしが互いに import し合ってよい(MainForm が Form2 を、Form2 が MainForm を import する)。
`import Form2` は読み込みの途中のモジュールでもそのまま名前に結び付けるだけで、中身(`Form2.Form2`)を見るのはハンドラが呼ばれたとき
(すべての読み込みと CreateForm が終わった後)だから。C++ でヘッダの `extern` の宣言だけを見て、実体はリンクの後に参照するのと同じ関係になる。
そのため、次の 3 つを守る。

| 守ること | 守らないと |
| --- | --- |
| `import Form2` と書き、`Form2.Form2` で使う。`from Form2 import Form2`・`from Form2 import TForm2` と書かない | `from` は import した時点で中身を取り出す。フォームの変数は None が写されるだけで、循環していれば `ImportError`(circular import)になる |
| ほかのフォームの中身は、メソッド(ハンドラ・FormCreate)の中でだけ使う。モジュールの直下やクラスの定義(`class TForm3(Form2.TForm2)`)で使わない | 読み込みの途中で中身を見るので、循環していれば `AttributeError` になる |
| 起動は `Project1.py` から行う。フォームの .py を直接実行しない | 直接実行したファイルは `__main__` として読み込まれ、ほかのフォームの `import MainForm` で同じファイルがもう一度読み込まれる(`MainForm.MainForm` が None のままになる) |

C++Builder でほかのフォームの OnCreate からまだ作られていないフォームを使うと NULL になるのと同じく、起動時に作るフォームの
コンストラクタ・OnCreate から、後に作るフォーム(`autoCreate` で後ろにあるもの)を使うことはできない。
