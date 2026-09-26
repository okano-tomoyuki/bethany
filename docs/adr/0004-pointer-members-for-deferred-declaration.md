# 0004. コントロールはポインタ(スマートポインタ)メンバとして持ち、既存の単一コンストラクタのまま生成する

- 状態: 承認
- 日付: 2026-09-26
- 置換: [0003](0003-two-phase-initialization.md)

> [0007](0007-lcl-faithful-hierarchy.md) で Owner と Parent を分離したため、現在の書き方は
> `button1.reset(new no_vcl::TButton(this)); button1->Parent = this;` のように Parent の設定が 1 行加わる。

## 背景

[0003](0003-two-phase-initialization.md) で、DSL コード生成に向けて初期化子リストの順序制約
(クラス宣言部と初期化子リスト部を常に同じ順序で同期させる必要がある問題)を回避する方法を検討した。
0003 では「デフォルトコンストラクタ + `Create()` メソッドによる二段階初期化」を提案したが、
レビューの結果、次の点で本家 C++Builder のモデルから乖離することが分かった。

- 本家 C++Builder のコンポーネントのコンストラクタ(`__fastcall TButton(TComponent* Owner)`)は、
  それ自体が「生成」を完了させる一発呼び出しであり、`Create()` のような別メソッドは存在しない。
- 本家が初期化子リストの順序制約を回避できている本当の理由は、コンストラクタを2段階にしているからではなく、
  **生成されるクラスがコンポーネントをポインタとして持つから**である(例: `TButton *Button1;`)。
  ポインタメンバは宣言順に依存せず、`new` による生成は**コンストラクタ本体の文**として自由な順序で書ける
  (初期化子リストと違い、文には宣言順との一致を要求する言語規則が無い)。

つまり 0003 が解決しようとした問題は、`Create()` という本家に存在しない API を追加しなくても、
**consumer 側がポインタ/スマートポインタでメンバを持つだけ**で解決できる。

## 検討した選択肢

- **0003 の案(デフォルトコンストラクタ + `Create()`)**: 本家に存在しない API を追加することになり、
  オブジェクトモデルの忠実さ([ADR 0002](0002-object-model-fidelity.md))に反する。
  `TCanvas`/`Pen`/`Brush`/`Font` への波及(再束縛の仕組みが別途必要)も含め、ライブラリ側の実装コストが大きい。
- **ポインタ/スマートポインタメンバ + 既存の単一コンストラクタ**: `std::unique_ptr<TButton> button1;` と宣言し、
  コンストラクタ「本体」で `button1.reset(new TButton(*this));` のように生成する。
  本家の見た目(ポインタメンバ)に忠実で、**ライブラリ側の変更が一切不要**。

## 決定

**ライブラリの初期化方式は現状のまま変更しない。**
DSL が生成するコードは、コントロールを `std::unique_ptr<T>` のメンバとして宣言し
(所有権を明確にするため生ポインタではなく `unique_ptr` を推奨する)、コンストラクタ本体で生成する形とする。

```cpp
class MainForm : public no_vcl::TForm
{
public:
    std::unique_ptr<no_vcl::TLabel>  label1;   // 宣言順は自由
    std::unique_ptr<no_vcl::TButton> button1;

    MainForm()
    {
        // 生成順も宣言順と無関係に書ける(marker方式のコード生成と相性が良い)
        button1.reset(new no_vcl::TButton(*this));
        button1->Caption = "Click me";

        label1.reset(new no_vcl::TLabel(*this));
        label1->Caption = "Hello";
    }
};
```

実際にこのパターンでコンパイル・実行し、宣言順(`label1` → `button1`)と生成順(`button1` → `label1`)を
わざと逆にしても問題なく動作することを確認済み(2026-09-26)。

`std::make_unique` は C++14 からのため、C++11 準拠の方針([todo.md](../../todo.md))に合わせ、
DSL 生成コードでは `reset(new T(...))` または `std::unique_ptr<T>(new T(...))` を使う。

## 影響

- **`no_vcl.hpp`/`no_vcl.cpp` への変更は不要**。0003 で懸念していた `TCanvas`/`Pen`/`Brush`/`Font` の
  再束縛(`Bind`)対応も不要になった(これらは所有者(`TPaintBox`)の内部でしか使われず、
  consumer 側が `TPaintBox` 自体をポインタで持つかどうかとは独立した話のため)。
- [todo.md](../../todo.md) の Phase 6 は「実装タスク」から「規約の周知」に縮小する。
- デザイナー([ADR 0001](0001-designer-app-bundling.md))のコード生成テンプレートは、
  「コントロール = `std::unique_ptr<T>` メンバ、生成はコンストラクタ本体の文」という規約で設計する。
