# 0005. Owner/Parent 引数は参照ではなくポインタで受ける

- 状態: 承認(一部置換 → [0007](0007-lcl-faithful-hierarchy.md))
- 日付: 2026-09-26

> ポインタで受ける方針は維持している。ただし [0007](0007-lcl-faithful-hierarchy.md) で Owner と Parent を分離し、
> コンストラクタの引数は `TComponent* AOwner`(`nullptr` も可)、Parent は `Property<TWinControl*>` になった。
> 本 ADR にある `TObject* parent` と `assert(parent != nullptr)` は現在のコードには無い。

## 背景

これまで `TButton` 等のコントロールのコンストラクタは `TObject& parent`(参照)を受け取っていた。
[ADR 0002](0002-object-model-fidelity.md) の「オブジェクトモデルは C++Builder に極力揃える」方針に照らすと、
本家 C++Builder のコンポーネントは `__fastcall TButton(TComponent* Owner)` のように
**ポインタ**で Owner を受け取っており、参照渡しは C++Builder の流儀ではない。

[ADR 0004](0004-pointer-members-for-deferred-declaration.md) でコントロール自体をポインタ/スマートポインタの
メンバとして持つ規約を決めたことで、生成コード側でも `new TButton(this)` のように `this` をそのまま渡す機会が増える。
参照だと `*this` と書く必要があり、本家の書き味(`Owner` にポインタをそのまま渡す)からわずかに外れる。

## 検討した選択肢

- **参照のまま(現状)**: 呼び出し側で `nullptr` を渡せない(コンパイル時に非 null が保証される)利点はあるが、
  本家の書き味・シグネチャと異なる。
- **ポインタに変更**: 本家に忠実。`nullptr` を渡すミスが可能になるため、コンストラクタ内で `assert` によるチェックを加える。

## 決定

**Owner/Parent を受け取るすべてのコンストラクタの引数を `TObject&` から `TObject*` に変更する。**
対象: `TButton`/`TLabel`/`TEdit`/`TCheckBox`/`TRadioButton`/`TPanel`/`TGroupBox`/`TComboBox`/`TListBox`/`TMemo`/`TPaintBox`
(`parent` 引数)、`TTimer`(`owner` 引数)。

各コンストラクタの先頭で `assert(parent != nullptr)`(`<cassert>`)によるチェックを追加し、
`nullptr` を渡した場合は(デバッグビルドで)即座に検知できるようにした。

`TForm` は元々 Owner 引数を持たない(常に `beth_TForm_Create(nullptr)`)ため、本 ADR の対象外。

## 影響

- `beth.hpp`/`beth.cpp` の全コントロールのコンストラクタ・`TPaintBox::MakeHandle` を変更。
- 呼び出し側は `TButton button(form);` から `TButton button(&form);` に変更が必要
  (`test/main.cpp` を追随済み)。ADR 0004 のポインタメンバ規約とも自然に噛み合う
  (`button1.reset(new TButton(this));` のように `this` をそのまま渡せる)。
- `beth_c.h`(C API 層)は元々すべて `beth_obj_t`(実質ポインタ)でやり取りしているため影響なし。
