# 0011. フォームに Release・OnClose・OnCreate(と OnShow)を追加し、OnCreate の発火時点を C++ 側で補う

- 状態: 承認
- 日付: 2026-09-26

## 背景

フォームを閉じる・破棄するまわりに、C++Builder の定番の書き方ができない箇所が残っていた。

- フォーム自身(またはその子)のイベントハンドラの中で `Free()` すると、実行中のハンドラの持ち主を破棄することになり未定義動作になる。
  C++Builder ではこの場面で `Release()`(保留中のメッセージを処理し終えてから破棄)を使う。
- `OnClose` が無く、閉じるのを取りやめる・閉じたら破棄する(`Action = caFree`)といった制御ができない。
  `OnClose` は `TCloseEvent`(Sender と、書き換え可能な `TCloseAction& Action`)という、`TNotifyEvent` と異なる形のイベントで、
  C API での表し方は [ADR 0009](0009-events-as-properties-with-sender.md) で「実装するときに決める」としていた。
- `OnCreate` が無い。LCL の `OnCreate` は Pascal 側のコンストラクタ(`AfterConstruction`)の中で呼ばれるため、
  C++ のラッパーがハンドラを設定する前に終わってしまい、そのままでは使えない。
  C++Builder では `OnCreate` は最派生クラスのコンストラクタの完了後に呼ばれる(C++ のフォームクラスでも同じ)が、
  標準 C++ では `new T(...)` の後に自動で処理を差し込めない。

## 検討した選択肢

- OnClose の C API: 戻り値で Action を返す / Action へのポインタ(`beth_int_t*`)を渡して書き換えさせる
- OnCreate の発火時点:
  - (a) 発火させない(コンストラクタの末尾に書けばよいとする)
  - (b) `Application->CreateForm` の完了時だけ発火させる
  - (c) (b) に加え、`new` で直接生成したフォームは最初に表示される直前(LCL の OnShow の直前)に発火させる
  - (d) メッセージキュー経由で非同期に発火させる

## 決定

1. **`TCustomForm::Release()` を追加する**(LCL の `Release` = `Application.ReleaseComponent`)。破棄後は破棄通知によってラッパーも delete される。
2. **OnClose は `Property<TCloseEvent>`**、`using TCloseEvent = std::function<void(TObject* Sender, TCloseAction& Action)>;` とする。
   `TCloseAction` は C++Builder と同じ名前・値の列挙型(`caNone, caHide, caFree, caMinimize`)。
   Action には LCL が決めた既定値(MainForm なら `caFree`、それ以外は `caHide`)が入っており、書き換えると動作が変わる。
3. **C API の OnClose は `void (*)(beth_obj_t sender, beth_int_t* action, void* data)`** とし、Pascal の `var` 引数と同じく
   ポインタ先を書き換えさせる(C++ の `TCloseAction&` と 1 対 1 に対応させるため、戻り値は使わない)。値は `beth_caNone` 等の定数で表す。
   範囲外の値が書き込まれた場合は既定値のままにする。
4. **OnCreate は (c)。** `Application->CreateForm` で生成した場合は、派生クラスのコンストラクタの完了直後(C++Builder と同じ時点。
   `Form1` への代入も済んでいる)に呼ぶ。`new` で直接生成した場合は、最初に表示される直前に呼ぶ。
   どちらの場合も 1 度だけで、利用者(とデザイナーの生成コード)はコンストラクタの中で `OnCreate` を設定すればよい。
   (a) は OnCreate を持たない点で C++Builder との差が大きく、(b) は `new` で生成したフォームで呼ばれず、
   (d) は表示との順序が保証できないため採らなかった。
   表示を検知するため、フォームは生成時に常に LCL の OnShow へブリッジを登録する。
5. **OnShow も公開する。** 4. のために OnShow のブリッジが必要で、公開のコストがほぼ無いため(C++Builder にもある)。
   OnCreate がまだ呼ばれていなければ、OnShow の前に呼ぶ。

いずれも LCL で TCustomForm の public メンバのため、TCustomForm に置き、C API も `beth_TCustomForm_*` とする。
C API では OnCreate は提供しない(C では生成直後に続けて処理を書けばよく、LCL の OnCreate は生成中に終わってしまうため)。

## 影響

- ハンドラの中で自分のフォームを破棄する場合は `Free()` ではなく `Release()` を使う(既知の問題の 1 つが解消)。
- `new` で生成し一度も表示しないフォームでは OnCreate が呼ばれない。また LCL は、最初の表示時に最大化状態のフォームで OnShow を呼ばないため、
  その場合 OnCreate は次に表示されるときまで遅れる。
- OnCloseQuery・OnDestroy・OnHide 等は未対応(必要になった時点で同じ形で追加する)。
