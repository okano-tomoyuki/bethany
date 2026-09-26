# 0003. コントロールの初期化を二段階(デフォルト構築 + 遅延 Create)に変更する

- 状態: 置換(→ [0004](0004-pointer-members-for-deferred-declaration.md))
- 日付: 2026-09-26

> レビューの結果、「本家 C++Builder のコンストラクタは1回呼べば生成が完了する一発方式のままで、
> 初期化子リストの順序制約を回避しているのはコンポーネントをポインタで持つからだ」という指摘を受け、
> Create() を新設する本 ADR の方針は採用しないことにした。詳細は [0004](0004-pointer-members-for-deferred-declaration.md) を参照。

## 背景

現在、`TButton` 等の各コントロールクラスは、`TObject& parent` を受け取る唯一のコンストラクタの中で、
`Property<T>` メンバの構築と `no_vcl_c.h` の `Create`/`SetParent` 呼び出しを同時に行っている。

```cpp
// 現状
explicit TButton(TObject& parent);   // このコンストラクタの中で no_vcl_TButton_Create まで行う
```

この方式は手書きコード(`no_vcl::TButton button(form);`)では問題ないが、
DSL からのコード生成([ADR 0001](0001-designer-app-bundling.md))には次の理由で不向きである。

1. C++ の「メンバ初期化子リストは宣言順と一致していなければならない」という制約により、
   **クラス宣言部(メンバの列挙)と初期化子リスト部(`: button1(*this), label1(*this), ...`)という 2 箇所を、
   常に同じ順序で機械的に同期させる必要がある**。
2. デザイナー上でウィジェットを追加・削除・並べ替えするたびにこの同期が発生し、
   tk-designer が採用しているマーカー区間ベースの差分更新方式(区間の外を壊さずに再生成する)と相性が悪い。
   初期化子リストという「1 つの式の中」に複数ウィジェット分の初期化が並ぶため、
   1 ウィジェットだけをピンポイントで追加・削除するマーカー処理が書きにくい。
3. 実際、[cpp_tk](https://github.com/okano-tomoyuki/cpp_tk)/tk-designer でも類似の理由から、
   生成するクラスがルートウィンドウを **メンバとして持つ(has-A)** 構成をやめ、
   **ルートクラスを継承する(is-A)** 構成に変更した経緯がある(tk-designer ADR 0011)。
   本件はそれと同根の問題(生成物の構造が C++ の初期化規則に強く縛られる)である。

## 検討した選択肢

- **現状維持(コンストラクタで即座に Create)**: 手書きコードは簡潔だが、上記の理由でコード生成に向かない。
- **ポインタメンバ + heap 確保**(実際の C++Builder の生成コードに近い): `TButton *button1 = nullptr;` と宣言し、
  `button1 = new TButton(parent);` のように後から生成する。見た目は C++Builder に最も忠実だが、
  delete 忘れや null チェックなど手動メモリ管理の負担が生じる(`unique_ptr` で緩和は可能)。
- **デフォルトコンストラクタ + `Create(TObject& parent)` メソッドによる二段階初期化**: 値メンバのまま(heap 確保なし)で、
  `TButton button1; button1.Create(*this);` のように宣言と生成を分離できる。
  宣言順に縛られる初期化子リストを使わずに済むため、コード生成・マーカー方式の差分更新に向く。

## 決定(提案中・未承認)

**デフォルトコンストラクタ + `Create()` メソッドによる二段階方式を推奨する。**

```cpp
// 提案する形
no_vcl::TButton button1;             // デフォルト構築(ハンドルはまだ無い)
// ... 他のメンバもここで宣言(順不同で良い)

button1.Create(*this);               // 実際の no_vcl_TButton_Create/SetParent はここで走る
button1.Caption = "OK";
```

既存の「`TObject& parent` を渡す 1 行コンストラクタ」は `Create()` を内部で呼ぶ薄いラッパーとして残し、
**後方互換を保つ**(手書きコードは今まで通り `TButton button1(form);` と書ける)。

## 影響(見込み。実装時に確定させる)

- `TObject` および各コントロールクラス(`TForm`/`TButton`/`TLabel`/…/`TTimer`/`TPaintBox`)に
  デフォルトコンストラクタを整備し、実際のハンドル生成・コールバック登録処理を `Create()` メソッドへ分離する。
- デストラクタは「`Create()` が一度も呼ばれないまま破棄される」場合に対応する必要がある
  (`handle_ == nullptr` なら `no_vcl_TXxx_Destroy` を呼ばない)。
- `Property<T>` 自体は `owner_`(`this` ポインタ)のみを保持し、`handle_` の参照は
  Getter/Setter 呼び出し時に遅延評価される設計のため、**変更は不要**と見込む。
- `TPaintBox` が持つ `TCanvas Canvas` メンバ(および `TCanvas` が持つ `Pen`/`Brush`/`Font`)は、
  現状「所有者のハンドルが確定した時点で即座に構築する」設計
  (`TObject(no_vcl_obj_t handle)` という第二コンストラクタを使った基底クラス委譲。[todo.md](../../todo.md) Phase 5 参照)
  になっており、二段階初期化に単純には乗らない。`TCanvas`/`Pen`/`Brush`/`Font` にも
  デフォルトコンストラクタと、ハンドル確定後に再束縛する仕組み(`Bind(no_vcl_obj_t handle)` 等)が別途必要になる
  (波及範囲が本件の中で最も大きい)。
- 既存の `test/main.cpp`・`test/main.c` のような 1 行コンストラクタ利用コードへの影響はない(後方互換を保つ前提のため)。

## 未決事項

- `TCanvas`/`Pen`/`Brush`/`Font` への適用方法(`Bind` 方式の詳細)。
- `Create()` の多重呼び出し時の挙動(無視する/`assert` する/例外を投げる)。
- 承認され次第、本 ADR のステータスを「承認」に更新し、実装タスクを [todo.md](../../todo.md) の Phase 6 に転記する。
