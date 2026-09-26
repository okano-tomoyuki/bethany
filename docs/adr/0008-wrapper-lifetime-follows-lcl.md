# 0008. C++ ラッパーの寿命を LCL オブジェクトに一致させ、ヒープ生成と Free() を強制する

- 状態: 承認
- 日付: 2026-09-26
- 一部置換: [0004](0004-pointer-members-for-deferred-declaration.md)(コントロールを `std::unique_ptr` で持つ規約)

## 背景

[ADR 0007](0007-lcl-faithful-hierarchy.md) の時点では、C++ ラッパーはスタック変数や `unique_ptr` で保持され、
ラッパーのデストラクタが LCL オブジェクトを破棄していた。一方で LCL は、Owner が破棄されると所有する
コンポーネントもまとめて破棄する。このため、ラッパーより先に Owner が破棄されると、
ラッパーのデストラクタが破棄済みのオブジェクトを再び破棄する(二重解放)問題があった。
将来 `OnClose` で `caFree` を返すフォームなど、LCL が自身の判断でオブジェクトを破棄する経路も同じ問題を持つ。

根本原因は「C++ ラッパーが知らないところで LCL がオブジェクトを破棄することがある」点にある。

## 検討した選択肢

破棄の検知(LCL の `FreeNotification` を使い、破棄されたことを C/C++ 側へ通知する)はどの案でも必要とし、
その上で「LCL がオブジェクトを破棄したとき、C++ ラッパー自体は誰が消すか」を検討した。

- **(a) C++ 側が持つ(従来の延長)**: ラッパーは中身が無効な抜け殻として残り、スコープや `unique_ptr` で消える。
  スタックや `unique_ptr` は使えるが、C++Builder の定番の `new TButton(this);`(delete せず Owner に任せる)で
  ラッパーがリークする。
- **(b) Owner が持つ(C++Builder と同じ意味論)**: LCL オブジェクトの破棄通知でラッパーも delete する。
  `new TButton(this);` がそのまま正しく動き、デザイナーの生成コードも C++Builder と同じ生ポインタメンバで済む。
  ただし「コンポーネントは new で生成し、自分では delete しない」というルールが必須になる。

(b) の場合のルールの強制方法として、ドキュメント上のルールに留める(`delete` も使える)か、
protected デストラクタでスタック生成と `delete` をコンパイルエラーにし、破棄は `Free()` で行うかを検討した。

## 決定

**(b) を採用し、ルールは protected デストラクタで強制する。**

1. **ラッパーの寿命 = LCL オブジェクトの寿命。** `*_Create` で生成したコンポーネントはすべて Pascal 側で
   破棄通知の対象に登録する。LCL オブジェクトが破棄されると(`Free()` でも Owner による連鎖破棄でも)、
   通知を受けた `TComponent` が共通レジストリからラッパーを引き、delete する。
   ラッパーが delete される経路はこの通知の 1 か所だけで、ラッパーのデストラクタは LCL オブジェクトを破棄しない。
   これにより二重解放は構造的に起きない。
2. **破棄は `Free()`。** `TComponent::Free()` は `no_vcl_TComponent_Destroy` を呼ぶだけで、
   ラッパーはその破棄通知で delete される(C++Builder の TObject にも `Free()` がある)。
3. **コンポーネント系の全クラスでデストラクタを protected にする。** C++ では基底クラスのデストラクタを
   protected にしても派生クラスの暗黙のデストラクタは public になるため、`TObject` から具象クラスまで
   すべてのクラスで protected と宣言する。これにより、スタック生成・`delete`・`unique_ptr`・
   基底ポインタ経由の `delete` がすべてコンパイルエラーになる。
   非所有のラッパー(`TCanvas`/`TPen`/`TBrush`/`TFont`)は `TPaintBox::Canvas` のように値メンバとして持つため、
   デストラクタを public にしている(`TObject` のデストラクタが protected なので、基底ポインタ経由の delete はできない)。
4. C API にも破棄通知の登録関数 `no_vcl_FreeNotify_SetCallback` を公開する。
   C から使う場合も、保持しているハンドルが無効になったことをこれで知ることができる。

## 影響

- 使い方は C++Builder と同じになる:
  `TForm* form = new TForm(nullptr);`、`TButton* button = new TButton(form); button->Parent = form;`、
  Owner を持たないものは最後に `form->Free();`。Owner を持つものは Owner に任せる。
- [ADR 0004](0004-pointer-members-for-deferred-declaration.md) の `std::unique_ptr` メンバの規約は使えなくなった。
  デザイナーの生成コードは C++Builder と同じく生ポインタメンバ(`TButton* Button1;`)を
  コンストラクタ本体で `new` し、Owner に破棄を任せる形になる。
- **利用者が派生クラスを作る場合は、デストラクタを protected で宣言する必要がある**(宣言し忘れると、
  その派生クラスだけスタック生成できてしまう。C++ の言語上これ以上は防げない)。デザイナーの生成コードでは必ず宣言する。
- コンパイルが通る/通らないことを確認済み: スタック生成・`delete`・`unique_ptr<TButton>`・
  `TObject*`/`TComponent*` 経由の `delete` は拒否、`new` + `Free()` と protected デストラクタを持つ派生クラスは可。
- 実行で確認済み: `Free()` による個別破棄でラッパーが delete されること、Owner の破棄に連動して
  所有するコンポーネントのラッパーも delete されること(C API ではフォーム+所有する 14 個の計 15 個の破棄通知)。
- **注意**: 自分自身(またはその Owner)を、そのコンポーネントのイベントハンドラの中で `Free()` すると、
  実行中のハンドラ(`std::function`)が delete されるため未定義動作になる。C++Builder/LCL でも同様の制約があり、
  フォームでは遅延破棄の `Release` を使うのが定石。`Release` は未実装([todo.md](../../todo.md))。
