# コード生成の設計 — 草案

フォームの定義ファイル([dsl-spec.md](dsl-spec.md))から、no_vcl の C++ と Python のコードを生成する方法。
生成したコードと利用者のコードの共存には、tk-designer と同じマーカー区間の更新方式を使う(tk-designer ADR 0004。[ADR 0035](../adr/0035-designer-in-this-repository.md) で流用を決めた)。

## 1. 生成の流れ

```
DSL (*.nvform.json)
  └─ 検証(Zod のスキーマ + カタログとの照合 + 意味の検証)
       └─ 中間表現(言語に依らない: 生成・プロパティの設定・親子・イベントの接続の並び)
            ├─ C++ エミッタ(no_vcl.hpp)  → 区間ごとのコード片
            └─ Python エミッタ(no_vcl.py) → 区間ごとのコード片
                 └─ マージ: 既存のファイルが無ければ雛形を作り、あればマーカー区間だけを置き換える
```

生成の単位は 1 つの DSL ファイル = 1 フォーム = 1 クラス。生成するクラスは TForm を継承する
(C++Builder の `class TForm1 : public TForm` と同じ。test/main.cpp の TMainForm がこの形の手書きの見本)。

## 2. マーカー区間

| 区間 | 内容 | C++ の置き場所 | Python の置き場所 |
|---|---|---|---|
| ① 宣言 | コンポーネントのメンバ、**ハンドラの宣言**(C++Builder の `__published` に当たる。public に置く) | ヘッダ(クラス定義の中) | `__init__` の `super().__init__()` の後(型の注釈) |
| ② 生成 | コンポーネントの生成・プロパティ・親子・イベントの接続 | ソース(`nvd_CreateComponents`) | クラスのメソッド(`nvd_CreateComponents`) |

tk-designer は「生成」「配置」「イベント」の 3 つのメソッドに分けていたが、LCL には配置だけを後から行う段階(pack・grid の呼び出し)が無く、
配置はプロパティ(Left・Align・Anchors 等)の設定そのものなので、1 つのメソッドにまとめる(§4 の順で書き出す)。

### マーカーの書式

```
// <no_vcl-designer:begin id="nvd_CreateComponents">
...
// <no_vcl-designer:end id="nvd_CreateComponents" hash="3f9a1c...">
```

Python では `#` を使う。`id` で区間を識別し、`hash` は区間の内容のハッシュ(手編集の検出。§5 M2)。

## 3. 生成例

[dsl-spec.md §2](dsl-spec.md#2-例) の例(抜粋)から生成するコード。

### C++

`MainForm.hpp`

```cpp
#pragma once

#include "no_vcl.hpp"

class TMainForm : public no_vcl::TForm
{
public:
    // <no_vcl-designer:begin id="declarations">
    no_vcl::TEdit*       NameEdit;
    no_vcl::TButton*     OkButton;
    no_vcl::TMemo*       Memo1;
    no_vcl::TMainMenu*   MainMenu1;
    no_vcl::TMenuItem*   FileMenu;
    no_vcl::TMenuItem*   FileOpenItem;
    no_vcl::TMenuItem*   N1;
    no_vcl::TMenuItem*   FileExitItem;
    no_vcl::TPopupMenu*  PopupMenu1;
    no_vcl::TOpenDialog* OpenDialog1;

    void FormCreate(no_vcl::TObject* Sender);
    void FormCloseQuery(no_vcl::TObject* Sender, bool& CanClose);
    void NameEditChange(no_vcl::TObject* Sender);
    void OkButtonClick(no_vcl::TObject* Sender);
    void FileOpenItemClick(no_vcl::TObject* Sender);
    void FileExitItemClick(no_vcl::TObject* Sender);
    // <no_vcl-designer:end id="declarations" hash="...">

    explicit TMainForm(no_vcl::TComponent* AOwner);

protected:
    // コンポーネントの派生はデストラクタを protected にする(docs/adr/0008)。
    ~TMainForm() override = default;

private:
    // 予約メソッド(宣言は変わらないので、区間の外の雛形に置く)。
    void nvd_CreateComponents();
};

extern TMainForm* MainForm;
```

`MainForm.cpp`

```cpp
#include "MainForm.hpp"

using namespace no_vcl;

TMainForm* MainForm = nullptr;

TMainForm::TMainForm(TComponent* AOwner)
    : TForm(AOwner)
{
    nvd_CreateComponents();
}

// <no_vcl-designer:begin id="nvd_CreateComponents">
void TMainForm::nvd_CreateComponents()
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
// <no_vcl-designer:end id="nvd_CreateComponents" hash="...">

// <no_vcl-designer:handler-stubs>
void TMainForm::FormCreate(TObject* Sender)
{
}

void TMainForm::FormCloseQuery(TObject* Sender, bool& CanClose)
{
}
// ...
```

利用側(`main` は生成しない。C++Builder の プロジェクトファイル(.cpp)に当たる):

```cpp
#include "MainForm.hpp"

int main()
{
    no_vcl::Application->Initialize();
    no_vcl::Application->CreateForm(&MainForm);
    no_vcl::Application->Run();
}
```

### Python

`MainForm.py`

```python
from no_vcl import *


class TMainForm(TForm):
    def __init__(self, AOwner):
        super().__init__(AOwner)
        # <no_vcl-designer:begin id="declarations">
        self.NameEdit: TEdit
        self.OkButton: TButton
        # ...
        # <no_vcl-designer:end id="declarations" hash="...">
        self.nvd_CreateComponents()

    # <no_vcl-designer:begin id="nvd_CreateComponents">
    def nvd_CreateComponents(self):
        self.NameEdit = TEdit(self)
        # ...
        self.Caption = "Sample"
        self.OnCreate = self.FormCreate
        self.NameEdit.Parent = self
        self.NameEdit.Anchors = {akLeft, akTop, akRight}
        self.NameEdit.OnChange = self.NameEditChange
        # ...
    # <no_vcl-designer:end id="nvd_CreateComponents" hash="...">

    # <no_vcl-designer:handler-stubs>
    def FormCreate(self, Sender):
        pass

    def FormCloseQuery(self, Sender, CanClose):
        pass
```

## 4. `nvd_CreateComponents` の中の順序

LCL ではプロパティを設定する順で結果が変わるものがあるため、次の順で書き出す。

1. **すべてのコンポーネントを生成する**(`new T(this)`。DSL の順。`controls` を深さ優先、次に `components` とメニュー項目)。
   参照するプロパティ(Menu・PopupMenu・Images 等)の相手が、設定の時点で必ず存在するようにするため。
2. **フォーム自身のプロパティとイベント。** 子の Anchors の距離は親の大きさで決まるため、フォームの大きさを先に決める。
3. **コントロールを木の順(親が先)に**、それぞれ次の順で設定する。
   1. Parent(TTabSheet は PageControl)。TabOrder の既定値は Parent を設定した順になる。
   2. Left・Top・Width・Height。
   3. その他のプロパティ(カタログの順。入れ子のオブジェクト・TStrings・参照を含む)。
   4. Anchors(Parent と大きさが決まった後に設定する。ADR 0034 で確かめた、距離が設定の時点の親の大きさで決まる動作のため)。
   5. イベント。
4. **非ビジュアルコンポーネント**のプロパティとイベント。メニューは項目のプロパティを設定してから、親の項目(ルートは `Items`)に Add する。

この順で生成したフォームを表示したときの配置が、デザイナーの配置と一致することを、生成したコードの実行で確かめる(§7)。

## 5. 決定事項(tk-designer から引き継ぐもの)

tk-designer の codegen-design.md の決定を、名前だけ変えて引き継ぐ。

| # | 論点 | 決定 |
|---|---|---|
| M1 | ハンドラの宣言・実装の置き場所 | 宣言は区間①に入れる。実装の雛形は `<no_vcl-designer:handler-stubs>` の後に追記するだけで、削除はしない(C++ では DSL から消えたハンドラの実装がコンパイルエラーになり、気づける) |
| M2 | 区間の中の手編集の検出 | 終了マーカーに区間の内容のハッシュを持たせ、再生成の時に一致しなければ、上書きの前に警告・確認する |
| M3 | フォーマッタとの共存 | ハッシュは空白(改行・字下げを含む)を取り除いてから計算する |
| M4 | 異常系 | マーカーの欠落・重複・入れ子・対応の誤りがあれば、何も書き込まずにエラーにする |
| M6 | 予約メソッド名 | 接頭辞 `nvd_` を付ける(`nvd_CreateComponents`)。VCL の命名(PascalCase)の利用者のメソッドと衝突しない |
| M7 | 名前の変更 | 警告のみ。区間の外の参照は書き換えない(なくなった名前が区間の外で使われていれば警告する) |
| M8 | VS Code 上での書き込み | WorkspaceEdit で書き込む。書き込む前に未保存の変更が無かったファイルは、書き込んだ後に保存する |

## 6. no_vcl 固有の決定(案)

| # | 論点 | 案 |
|---|---|---|
| N1 | イベントの接続 | C++ は `[this](引数...) { ハンドラ(引数...); }` のラムダ(test/main.cpp と同じ書き方)。引数はカタログのイベントの型から作る。Python はバインドしたメソッドをそのまま代入する |
| N2 | 名前空間 | ヘッダは `no_vcl::` で修飾し(ヘッダで `using namespace` しない)、ソースは `using namespace no_vcl;` とする。利用者が書くハンドラの実装は修飾しなくてよい |
| N3 | フォームのグローバル変数 | C++ は `extern TMainForm* MainForm;` と定義を生成する(dsl-spec.md §10 Q3) |
| N4 | 値の書き出し | 列挙型は要素名、TAnchors は `TAnchors() << ...`、ビット集合は `|` でつなぐ(空なら `0`)、TColor は定数名か `0x00BBGGRR`、TShortCut は `TextToShortCut("...")`、TStrings は `Add` の並び(Python も同じ形) |
| N5 | 既定値の扱い | DSL に書いたプロパティだけを書き出す(既定値と同じ値が書かれていても、そのまま書き出す) |
| N6 | 雛形の基底クラスの照合 | 既存のファイルのクラスが TForm を継承していなければ、何も書き込まずにエラーにする(tk-designer と同じ) |

## 7. 検証

- **ゴールデンファイル**: DSL → 生成コードの入出力を、codegen のテストで比較する(tk-designer と同じ)。
- **ビルドと実行**: 見本の DSL(`designer/samples/`)から生成した C++ を、本リポジトリの no_vcl と一緒に CMake でビルドして実行し、次を確かめる。
  Python も同じ内容を py/no_vcl.py で実行して確かめる。
  1. 表示後の各コントロールの位置と大きさが、DSL に書いた値(デザイナーが計算した配置)と一致すること(Align・Anchors・BorderSpacing を含む)。
  2. イベントが接続されていること(ボタンの Click・メニューの Click でハンドラが呼ばれる)。
  3. 参照(Menu・PopupMenu・Images)・入れ子のオブジェクト(Font 等)・TStrings が設定されていること。
- 検証のプログラムは、生成したフォームのコンストラクタの後にテスト用の処理を加える形で、`designer/tools/codegen/` に置く。
