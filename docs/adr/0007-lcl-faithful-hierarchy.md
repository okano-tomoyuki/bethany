# 0007. クラス階層を LCL の継承関係に忠実に揃え、Owner と Parent を分離する

- 状態: 承認
- 日付: 2026-09-26
- 一部置換: [0005](0005-owner-as-pointer.md)(Owner と Parent をコンストラクタで兼ねる点・`assert` による非 null チェック)

## 背景

これまでの C++ ラッパーは、`TForm`・`TButton` 等をすべて `TObject` の直接の派生にしており、
`Left`・`Top`・`Width`・`Height`・`Visible`・`Enabled`・`Caption` といった共通プロパティを
13 クラスそれぞれに複製していた。C API も `beth_TButton_GetLeft` のようにクラスごとに同じ関数を持っていた。

また、コンストラクタの引数 `TObject* parent` は Owner(破棄の責任)と Parent(画面上の親)を兼ねており、
型としては任意の `TObject` を受け付けていた。例えば `TPaintBox` や `TTimer` を親として渡すことができ、
Pascal 側の `TWinControl(ParentObj)` ハードキャストで落ちる余地があった。

LCL のクラス階層を調べると、コントロールはかなり整然とした継承関係を持っており、
ラッパーとしてはこの構成に忠実に従うほうが安全だと判断した。

## 検討した選択肢

1. 中間クラスの再現範囲
   - (a) LCL の全階層(TLCLComponent・TCustomDesignControl・TFPCanvasHelper 等の LCL/FPC 固有クラスも含む)
   - (b) TCustomXxx 系は再現し、LCL/FPC 固有で公開メンバも持たないクラスは省く
2. Owner と Parent
   - コンストラクタで兼ねる(現状)
   - LCL/C++Builder と同じく分離する(コンストラクタは Owner だけ、Parent はプロパティ)
3. C API
   - クラス別の関数を残す
   - LCL でそのメンバが公開される階層に 1 本化し、重複は削除する

## 決定

1. **(b) を採用する。** C++ のクラス階層は LCL の継承関係の **部分列** とする
   (途中の階層を省くことはあっても、LCL に無い継承関係は作らない)。
   TCustomXxx は C++Builder にも存在し、TCustomEdit が TEdit/TMemo の共通メンバを持つように実体があるため再現する。
   TLCLComponent・TCustomDesignControl・TFPCanvasHelper 等は C++Builder に無く、省いても安全性は損なわれないため省く。
   階層図は [docs/class-hierarchy.md](../class-hierarchy.md)。
2. **Owner と Parent を分離する。** 具象クラスのコンストラクタは `(TComponent* AOwner)` のみを受け取り(`nullptr` も可)、
   画面上の親は `Property<TWinControl*> Parent` で設定する(`button.Parent = &form;`)。
   Parent の型が `TWinControl*` なので、TLabel・TPaintBox(TGraphicControl)や TTimer を親にするとコンパイルエラーになる。
3. **C API を再構成し、重複は削除する。** 関数は LCL でそのメンバが公開される階層のクラス名で 1 本にする
   (`beth_TControl_GetLeft` 等)。兄弟クラスがそれぞれ公開していて重複する場合(Text・Checked)だけ、
   宣言元の共通祖先の名前で 1 本にし、Pascal 側は protected hack でアクセスする。
   破棄は `beth_TComponent_Destroy` に 1 本化する。

あわせて次を行った。

- **メンバの公開範囲も LCL に合わせる。** LCL で protected のメンバ(TControl の Text、TButtonControl の Checked)は
  C++ でも protected にし、公開する派生クラスで `using` する。
- **ハンドル → C++ ラッパーの共通レジストリを `TComponent` に置く。** `Parent` の Getter はこれでラッパーを引く。
  クラスごとに複製していたコールバック用のレジストリもこれに統一した。
  Pascal 側のブリッジは最初の `SetOnXxx` 呼び出し時に 1 度だけ登録する。
- **`beth_c.cpp` は関数一覧(X マクロ)から生成する。** 1 関数につき「関数ポインタ型・thread_local 変数・
  マッピング・ラッパー」の 4 箇所を手で書いていたのを、一覧の 1 行から展開する形にした。
  `beth_c.h` は可読性のため手書きのまま残す(型が食い違えばコンパイルエラーになる)。

## 影響

- 共通プロパティのクラスごとの複製がなくなった(C API は 94 関数に整理。DLL のエクスポート・C 側の一覧・ヘッダが一致することを確認済み)。
- LCL で許されない操作がコンパイルエラーになることを確認済み:
  TLabel・TPaintBox を Parent にする、TButton・TLabel の `Checked` に触る、TLabel の `Text` に触る、TCustomEdit を直接生成する。
- 既存コードは次の変更が必要:
  `TForm form;` → `TForm form(nullptr);`、`TButton button(&form);` の後に `button.Parent = &form;`、
  C API の関数名(`beth_TButton_SetCaption` → `beth_TControl_SetCaption` 等)。
  [ADR 0004](0004-pointer-members-for-deferred-declaration.md) の生成コードの例も、生成の後に `Parent` の設定が 1 行加わる。
- TMemo は TCustomEdit の派生になったため、TEdit と同じく Text・MaxLength・ReadOnly・SetOnChange を持つ。
  TLabel・TPanel・TPaintBox 等も LCL と同じく OnClick を持つようになった。
- **既知の課題**(→ [0008](0008-wrapper-lifetime-follows-lcl.md) で解決): C++ ラッパーより先に LCL の Owner が破棄されると、
  Owner が所有するコントロールは LCL 側で破棄済みになり、その後ラッパーのデストラクタが `TComponent_Destroy` を呼ぶと
  二重解放になる(本 ADR 以前からある問題)。
