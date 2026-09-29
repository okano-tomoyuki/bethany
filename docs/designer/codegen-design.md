# コード生成の設計 — 草案

フォームの定義ファイル([dsl-spec.md](dsl-spec.md))から、Bethany の C++ と Python のコードを生成する方法。
生成したコードと利用者のコードの共存には、tk-designer と同じマーカー区間の更新方式を使う(tk-designer ADR 0004。[ADR 0035](../adr/0035-designer-in-this-repository.md) で流用を決めた)。

## 1. 生成の流れ

```
DSL (*.bfm.json)
  └─ 検証(Zod のスキーマ + カタログとの照合 + 意味の検証)
       └─ 中間表現(言語に依らない: 生成・プロパティの設定・親子・イベントの接続の並び)
            ├─ C++ エミッタ(beth.hpp)  → 区間ごとのコード片
            └─ Python エミッタ(beth.py) → 区間ごとのコード片
                 └─ マージ: 既存のファイルが無ければ雛形を作り、あればマーカー区間だけを置き換える
```

生成の単位は 1 つの DSL ファイル = 1 フォーム = 1 クラス。生成するクラスは TForm を継承する
(C++Builder の `class TForm1 : public TForm` と同じ。test/main.cpp の TMainForm がこの形の手書きの見本)。

## 2. マーカー区間

| 区間 | 内容 | C++ の置き場所 | Python の置き場所 |
|---|---|---|---|
| ① 宣言 | コンポーネントのメンバ、**ハンドラの宣言**(C++Builder の `__published` に当たる。public に置く) | ヘッダ(クラス定義の中) | `__init__` の `super().__init__()` の後(型の注釈) |
| ② 生成 | コンポーネントの生成・プロパティ・親子・イベントの接続 | ソース(`beth_CreateComponents`) | クラスのメソッド(`beth_CreateComponents`) |
| ③ ファイルの枠 | インクルードガード・`<bethany/beth.hpp>` と自分のヘッダの include・名前空間の始まりと終わり(プロジェクトの `codegen.cpp` の設定。[project-spec.md](project-spec.md) §5) | ヘッダ(`beth_HeaderBegin`・`beth_NamespaceBegin`・`beth_HeaderEnd`)、ソース(`beth_SourceBegin`・`beth_NamespaceBegin`・`beth_NamespaceEnd`) | — |
| ④ フォームの変数 | `MainForm: "TMainForm" = None` | — | モジュールの末尾(`beth_FormVariable`) |

③ を区間にするのは、設定(名前空間・インクルードガード・拡張子・フォルダ)を変えて生成し直したときに、既存のファイルにも反映するため。
利用者の include は `beth_HeaderBegin`・`beth_SourceBegin` の後、名前空間の区間の前に書く(名前空間の中で include しないように)。
名前空間を使わないときも、名前空間の区間はマーカーの行だけで残す(後から設定したときに入れる場所)。

#### 後の版で加えた区間

以前の版の生成物に無い区間(③・④)は、生成し直すときに加える。④ と ③ の終わりはファイルの末尾に加え、③ の始まりは以前の生成物の決まった行
(ヘッダの先頭の `#pragma once`・空行・`#include "beth.hpp"`、ソースの先頭の自分のヘッダの include と `using namespace beth;`、
クラスの説明のコメントの前)を置き換える。その行が手で変えられていて見つからなければ、③ の区間はどれも加えず(始まりだけ・終わりだけにならないように)、
警告を出す。そのファイルには ③ の設定が反映されないので、マーカーを手で書き足す。

tk-designer は「生成」「配置」「イベント」の 3 つのメソッドに分けていたが、LCL には配置だけを後から行う段階(pack・grid の呼び出し)が無く、
配置はプロパティ(Left・Align・Anchors 等)の設定そのものなので、1 つのメソッドにまとめる(§4 の順で書き出す)。

### マーカーの書式

```
// <bethany-designer:begin id="beth_CreateComponents">
...
// <bethany-designer:end id="beth_CreateComponents" hash="3f9a1c...">
```

Python では `#` を使う。`id` で区間を識別し、`hash` は区間の内容のハッシュ(手編集の検出。§5 M2)。

## 3. 生成例

[dsl-spec.md §2](dsl-spec.md#2-例) の例(抜粋)から生成するコード。実際の生成結果は
[designer/packages/codegen/src/\_\_golden\_\_/](../../designer/packages/codegen/src/__golden__/)(見本 designer/samples/MainForm.bfm.json から生成したもの)を参照。
コメントの言語は、フォームが属するプロジェクトの `codegen.commentLocale`(既定は英語)で決まる([project-spec.md](project-spec.md) §5)。

### C++

`MainForm.hpp`(名前空間を使わない既定の設定)

```cpp
// <bethany-designer:begin id="beth_HeaderBegin">
#ifndef MAINFORM_HPP
#define MAINFORM_HPP

#include <bethany/beth.hpp>
// <bethany-designer:end id="beth_HeaderBegin" hash="...">

// <bethany-designer:begin id="beth_NamespaceBegin">
// <bethany-designer:end id="beth_NamespaceBegin" hash="...">

/** Form created with the Bethany designer (MainForm.bfm.json). Regions enclosed in markers are overwritten when regenerated. */
class TMainForm : public beth::TForm
{
public:
    // <bethany-designer:begin id="declarations">
    beth::TEdit* NameEdit;
    beth::TButton* OkButton;
    beth::TMemo* Memo1;
    beth::TMainMenu* MainMenu1;
    beth::TMenuItem* FileMenu;
    beth::TMenuItem* FileOpenItem;
    beth::TMenuItem* N1;
    beth::TMenuItem* FileExitItem;
    beth::TPopupMenu* PopupMenu1;
    beth::TOpenDialog* OpenDialog1;

    void FormCreate(beth::TObject* Sender);
    void FormCloseQuery(beth::TObject* Sender, bool& CanClose);
    void NameEditChange(beth::TObject* Sender);
    void OkButtonClick(beth::TObject* Sender);
    void FileOpenItemClick(beth::TObject* Sender);
    void FileExitItemClick(beth::TObject* Sender);
    // <bethany-designer:end id="declarations" hash="...">

    explicit TMainForm(beth::TComponent* AOwner);

protected:
    ~TMainForm() override = default;

private:
    void beth_CreateComponents();
};

extern TMainForm* MainForm;

// <bethany-designer:begin id="beth_HeaderEnd">
#endif // MAINFORM_HPP
// <bethany-designer:end id="beth_HeaderEnd" hash="...">
```

`MainForm.cpp`

```cpp
// <bethany-designer:begin id="beth_SourceBegin">
#include "MainForm.hpp"
// <bethany-designer:end id="beth_SourceBegin" hash="...">

// <bethany-designer:begin id="beth_NamespaceBegin">
using namespace beth;
// <bethany-designer:end id="beth_NamespaceBegin" hash="...">

TMainForm* MainForm = nullptr;

TMainForm::TMainForm(TComponent* AOwner)
    : TForm(AOwner)
{
    beth_CreateComponents();
}

// <bethany-designer:begin id="beth_CreateComponents">
// Creates the components and sets their properties (generated).
void TMainForm::beth_CreateComponents()
{
    NameEdit = new TEdit(this);
    OkButton = new TButton(this);
    Memo1 = new TMemo(this);
    MainMenu1 = new TMainMenu(this);
    FileMenu = new TMenuItem(this);
    FileOpenItem = new TMenuItem(this);
    N1 = new TMenuItem(this);
    FileExitItem = new TMenuItem(this);
    PopupMenu1 = new TPopupMenu(this);
    OpenDialog1 = new TOpenDialog(this);

    Caption = "Sample";
    Width = 400;
    Height = 300;
    Menu = MainMenu1;
    OnCreate = [this](TObject* Sender) { FormCreate(Sender); };
    OnCloseQuery = [this](TObject* Sender, bool& CanClose) { FormCloseQuery(Sender, CanClose); };

    NameEdit->Parent = this;
    NameEdit->Left = 16;
    NameEdit->Top = 16;
    NameEdit->Width = 280;
    NameEdit->Height = 23;
    NameEdit->Hint = "Your name";
    NameEdit->ShowHint = true;
    NameEdit->Anchors = TAnchors() << akLeft << akTop << akRight;
    NameEdit->OnChange = [this](TObject* Sender) { NameEditChange(Sender); };

    OkButton->Parent = this;
    // ...(同様。Font は入れ子: OkButton->Font->Style = fsBold;)

    Memo1->Parent = this;
    // ...
    Memo1->Lines->Add("line 1");
    Memo1->Lines->Add("line 2");
    Memo1->PopupMenu = PopupMenu1;

    FileMenu->Caption = "&File";
    MainMenu1->Items->Add(FileMenu);
    FileOpenItem->Caption = "&Open...";
    FileOpenItem->ShortCut = TextToShortCut("Ctrl+O");
    FileOpenItem->OnClick = [this](TObject* Sender) { FileOpenItemClick(Sender); };
    FileMenu->Add(FileOpenItem);
    N1->Caption = "-";
    FileMenu->Add(N1);
    // ...

    OpenDialog1->Filter = "Text files|*.txt|All files|*.*";
    OpenDialog1->Options = ofEnableSizing | ofViewDetail | ofFileMustExist;
}
// <bethany-designer:end id="beth_CreateComponents" hash="...">

// <bethany-designer:handler-stubs>
void TMainForm::FormCreate(TObject* Sender)
{
    // TODO: implement
}

void TMainForm::FormCloseQuery(TObject* Sender, bool& CanClose)
{
    // TODO: implement
}
// ...

// <bethany-designer:begin id="beth_NamespaceEnd">
// <bethany-designer:end id="beth_NamespaceEnd" hash="...">
```

`"codegen": { "cpp": { "namespace": "app" } }` なら、名前空間の区間は次のようになり、フォームのクラス・フォームの変数・ハンドラは名前空間の中に入る
(C++11 でも通るよう、入れ子の名前空間 `app::ui` は `namespace app { namespace ui {` と 1 つずつ書く)。

```cpp
// MainForm.hpp
// <bethany-designer:begin id="beth_NamespaceBegin">
namespace app
{
// <bethany-designer:end id="beth_NamespaceBegin" hash="...">
...
// <bethany-designer:begin id="beth_HeaderEnd">
} // namespace app

#endif // APP_MAINFORM_HPP
// <bethany-designer:end id="beth_HeaderEnd" hash="...">
```

利用側(C++Builder のプロジェクトファイル(.cpp)に当たる。プロジェクトファイルがあれば生成できる。[project-spec.md](project-spec.md) §5):

```cpp
#include <bethany/beth.hpp>
#include "MainForm.hpp"

int main()
{
    beth::Application->Initialize();
    beth::Application->CreateForm(&MainForm);   // 名前空間を設定していれば &app::MainForm
    beth::Application->Run();
}
```

### Python

`MainForm.py`

```python
from beth import *


class TMainForm(TForm):
    """Form created with the Bethany designer (MainForm.bfm.json). Regions enclosed in markers are overwritten when regenerated."""

    def __init__(self, AOwner):
        super().__init__(AOwner)
        # <bethany-designer:begin id="declarations">
        self.NameEdit: TEdit
        self.OkButton: TButton
        # ...
        # <bethany-designer:end id="declarations" hash="...">
        self.beth_CreateComponents()

    # <bethany-designer:begin id="beth_CreateComponents">
    def beth_CreateComponents(self):
        """Creates the components and sets their properties (generated)."""
        self.NameEdit = TEdit(self)
        # ...

        self.Caption = "Sample"
        self.Menu = self.MainMenu1
        self.OnCreate = self.FormCreate

        self.NameEdit.Parent = self
        # ...
        self.NameEdit.Anchors = {akTop, akLeft, akRight}
        self.NameEdit.OnChange = self.NameEditChange
        # ...(C++ と同じ並び。値の書き方は §6 N4)
    # <bethany-designer:end id="beth_CreateComponents" hash="...">

    # <bethany-designer:handler-stubs>

    def FormCreate(self, Sender):
        pass

    def FormCloseQuery(self, Sender, CanClose):
        pass
```

- 参照渡しの引数(`bool& CanClose`・`TCloseAction& Action` 等)は、py/beth の規則どおり `Ref` で渡される(`CanClose.value = False`)。
- フォームのグローバル変数は生成しない(`MainForm = Application.CreateForm(TMainForm)` と書く。dsl-spec.md §10 Q3)。
- Python の値の書き方: 集合型は `{akTop, akLeft}`(空なら `set()`)、ビット集合は `|` でつなぐ(空なら `TFontStyles(0)` のように型の 0)、
  TColor の `"#RRGGBB"` は `0x00BBGGRR  # #RRGGBB`、参照は `self.名前`。
- 区間の外の参照の警告(M7)では、宣言の区間の型の注釈(`self.OkButton: TButton`)と、イベントに代入したハンドラを生成したメンバとする
  (`self.Caption = ...` のようなフォームのプロパティの設定は含めない)。

## 4. `beth_CreateComponents` の中の順序

LCL ではプロパティを設定する順で結果が変わるものがあるため、次の順で書き出す。

1. **すべてのコンポーネントを生成する**(`new T(this)`。DSL の順。`controls` を深さ優先、次に `components` とメニュー項目)。
   参照するプロパティ(Menu・PopupMenu・Images 等)の相手が、設定の時点で必ず存在するようにするため。
2. **フォーム自身のプロパティとイベント。** 子の Anchors の距離は親の大きさで決まるため、フォームの大きさを先に決める。
3. **コントロールを木の順(親が先)に**、それぞれ次の順で設定する。
   1. Parent(TTabSheet は PageControl)。TabOrder の既定値は Parent を設定した順になる。
   2. Left・Top・Width・Height。
   3. その他のプロパティ(カタログの順。入れ子のオブジェクト・TStrings・参照を含む)。カタログでは、範囲を持つもの
      (TSpinEdit 等)の Value を MinValue・MaxValue の後に並べている。
   4. Anchors(Parent と大きさが決まった後に設定する。ADR 0034 で確かめた、距離が設定の時点の親の大きさで決まる動作のため)。
   5. イベント。
4. **コントロールへの参照**(TPageControl の ActivePage・TUpDown の Associate)。相手の親(TTabSheet なら PageControl)が
   決まった後でなければ働かないため、すべてのコントロールの後にまとめて設定する。
5. **非ビジュアルコンポーネント**のプロパティとイベント。メニューは項目のプロパティを設定してから、親の項目(ルートは `Items`)に Add する。

この順で生成したフォームを表示したときの配置が、デザイナーの配置と一致することを、生成したコードの実行で確かめる(§7)。

## 5. 決定事項(tk-designer から引き継ぐもの)

tk-designer の codegen-design.md の決定を、名前だけ変えて引き継ぐ。

| # | 論点 | 決定 |
|---|---|---|
| M1 | ハンドラの宣言・実装の置き場所 | 宣言は区間①に入れる。実装の雛形は `<bethany-designer:handler-stubs>` の後に追記するだけで、削除はしない(C++ では DSL から消えたハンドラの実装がコンパイルエラーになり、気づける) |
| M2 | 区間の中の手編集の検出 | 終了マーカーに区間の内容のハッシュを持たせ、再生成の時に一致しなければ、上書きの前に警告・確認する |
| M3 | フォーマッタとの共存 | ハッシュは空白(改行・字下げを含む)を取り除いてから計算する |
| M4 | 異常系 | マーカーの欠落・重複・入れ子・対応の誤りがあれば、何も書き込まずにエラーにする |
| M6 | 予約メソッド名 | 接頭辞 `beth_` を付ける(`beth_CreateComponents`)。VCL の命名(PascalCase)の利用者のメソッドと衝突しない |
| M7 | 名前の変更 | 警告のみ。区間の外の参照は書き換えない(なくなった名前が区間の外で使われていれば警告する) |
| M8 | VS Code 上での書き込み | WorkspaceEdit で書き込む。書き込む前に未保存の変更が無かったファイルは、書き込んだ後に保存する |

## 6. Bethany 固有の決定

| # | 論点 | 案 |
|---|---|---|
| N1 | イベントの接続 | C++ は `[this](引数...) { ハンドラ(引数...); }` のラムダ(test/main.cpp と同じ書き方)。引数はカタログのイベントの型から作る。Python はバインドしたメソッドをそのまま代入する |
| N2 | 名前空間 | ヘッダは `beth::` で修飾し(ヘッダで `using namespace` しない)、ソースは `using namespace beth;` とする。利用者が書くハンドラの実装は修飾しなくてよい |
| N3 | フォームのグローバル変数 | C++ は `extern TMainForm* MainForm;` と定義を生成する(dsl-spec.md §10 Q3) |
| N4 | 値の書き出し | 列挙型は要素名、TAnchors は `TAnchors() << ...`、ビット集合は `\|` でつなぐ(空なら `0`)、TColor は定数名か `0x00BBGGRR /* #RRGGBB */`、TShortCut は `TextToShortCut("...")`(空なら `0`)、TStrings は `Add` の並び(既定の中身があるものは先に `Clear()`。Python も同じ形) |
| N5 | 既定値の扱い | DSL に書いたプロパティだけを書き出す(既定値と同じ値が書かれていても、そのまま書き出す) |
| N6 | 雛形の基底クラスの照合 | **照合しない**(実装で決定)。tk-designer はルートのクラス(Tk・Toplevel・Frame)を DSL で変えられたため照合していたが、ここでは基底は常に TForm で、利用者が自前の中間クラスに書き換えるのは正当な使い方のため |

## 7. 検証

- **ゴールデンファイル**: DSL → 生成コードの入出力を、codegen のテストで比較する(tk-designer と同じ)。
- **ビルドと実行**: 見本の DSL(`designer/samples/`)から生成した C++ を、本リポジトリの Bethany と一緒に CMake でビルドして実行し、次を確かめる。
  Python も同じ内容を py/beth で実行して確かめる。
  1. 表示後の各コントロールの位置と大きさが、DSL に書いた値(デザイナーが計算した配置)と一致すること(Align・Anchors・BorderSpacing を含む)。
  2. イベントが接続されていること(ボタンの Click・メニューの Click でハンドラが呼ばれる)。
  3. 参照(Menu・PopupMenu・Images)・入れ子のオブジェクト(Font 等)・TStrings が設定されていること。
- 検証のプログラムは [designer/tools/codegen/verify-cpp.mts](../../designer/tools/codegen/verify-cpp.mts)(`pnpm codegen:verify-cpp`)と
  [verify-python.mts](../../designer/tools/codegen/verify-python.mts)(`pnpm codegen:verify-python`)。期待は共通
  ([report.mts](../../designer/tools/codegen/report.mts))で、C++ と Python で同じ結果になることも確かめている。
  生成したコードのハンドラの雛形を「呼ばれたことを記録する処理」に置き換え、フォームを `Application->CreateForm` で生成・表示した後に、
  配置・プロパティを読み、LCL を通してイベントを発生させる(Text の変更・Checked の変更・メニュー項目の Click・Close)。
  C++ の生成したコードは `-Wall -Wextra -Werror`(未使用の引数を除く)でコンパイルする。
  Python はバインディング(py/*.py)と DLL を作業フォルダに写して実行する。
- Linux(GTK2)での検証はまだ行っていない(既定の大きさ・クライアント領域が Win32 と違うため、期待の扱いを決める必要がある)。

### 7.1 検証で分かったこと(2026-09-28、Win32)

- **LCL の TForm の Width・Height はクライアント領域の大きさ。** Width 400・Height 300 のフォームで、alBottom の高さ 41 のパネルは
  Top 259・Width 400 になる(メニューがあっても同じ)。Delphi・C++Builder と違い、枠・タイトルバー・メニューの分を含まない
  (dsl-spec.md §10 Q9 を改めた)。
- **AutoSize のコントロール(TLabel・TCheckBox・TEdit 等)の大きさは LCL が決める。** Width 120 と書いた TCheckBox("Word wrap")は 80 になる。
  デザイナーが書く大きさは見積もりになるため、検証では位置だけを比べる。
- ActivePage をタブシートの PageControl の設定より前に設定しても働かない(§4 の 4 を加えた)。
- Anchors は Parent・大きさの後に設定すれば、フォームを広げたときに期待どおり動く(右寄せの OkButton が 304 → 404、左右寄せの NameEdit が 280 → 380)。
