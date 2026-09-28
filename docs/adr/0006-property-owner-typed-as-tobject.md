# 0006. Property<T> の所有者型を void* ではなく TObject* にする

- 状態: 承認
- 日付: 2026-09-26

## 背景

`Property<T>` は所有者への生ポインタ + 固定の Getter/Setter 関数ポインタを持つ軽量プロキシだが、
これまで所有者は `void*` として扱っていた(`using Getter = T (*)(void*);` 等)。
`TObject` は `Property<T>` より後で定義されていたため、`Property<T>` の時点では `TObject` を型として使えず、
`void*` にせざるを得なかった。この結果、各クラスの `XxxImpl(void* owner)` を読んでも
「owner が何であるべきか」がシグネチャからは分からず、実装の意図がわかりにくいという指摘があった。

## 検討した選択肢

- **現状維持(void*)**: `TObject` を継承しない `TPen`/`TBrush`/`TFont`/`TCanvas` もそのまま使えるが、
  シグネチャからは所有者の型が読み取れない。
- **`TObject` を `Property<T>` より前に定義し、Getter/Setter を `TObject*` にする**:
  シグネチャが「`TObject` 派生インスタンスを受け取る」ことを明示でき、意図が明確になる。
  ただし `TPen`/`TBrush`/`TFont`/`TCanvas` が `TObject` を継承していないと成立しない。

## 決定

**`TObject` の定義を `Property<T>` より前に移動し、`Property<T>::Getter`/`Setter` を
`TObject*` を受け取るシグネチャに変更する。**

これに伴い、`TPen`/`TBrush`/`TFont`/`TCanvas`(非所有ラッパー。[todo.md](../../todo.md) Phase 5 参照)も
`TObject` を継承する形に変更した。

- これらのクラスが持っていた独自の `beth_obj_t handle_` メンバは削除し、
  `TObject` から継承する `protected: beth_obj_t handle_` を使うよう統一した。
- コンストラクタは `TObject(handle)`(既存の、派生クラスがハンドルを基底クラス初期化の時点で
  受け取るための第二コンストラクタ)に処理を委譲する形に変更した。
- `TCanvas` が独自に持っていた `beth_obj_t Handle() const { ... }` は
  `TObject::Handle()` と完全に重複するため削除した。
- `TCanvas` が明示的に宣言していたコピー禁止(`TCanvas(const TCanvas&) = delete;` 等)も、
  `TObject` の禁止が継承されるため削除した(重複の解消)。

`TObject` は「`beth_obj_t` のハンドルを保持し `Handle()` で公開する」という役割に過ぎず、
「Create/Destroy を自前で行う(所有する)」ことを強制するものではない、という位置づけに整理した。
所有するかどうかは各派生クラスの**デストラクタが Destroy を呼ぶかどうか**で決まる。

## 影響

- 各クラスの `XxxImpl` 関数のシグネチャは `TObject* owner` になり、内部で
  `static_cast<TButton*>(owner)->handle_` のように具象型へダウンキャストする、という実装の流れが
  シグネチャからも読み取れるようになった。
- `TPen`/`TBrush`/`TFont`/`TCanvas` は `TObject` 派生になったが、Create/Destroy を自前で行わない
  (非所有)という性質は変わらない。コメントもその旨に更新した。
- ビルド・実行(`test/main.cpp` の全コントロール・Canvas描画)で問題ないことを確認済み(2026-09-26)。
