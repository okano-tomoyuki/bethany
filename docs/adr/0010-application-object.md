# 0010. Application をグローバルな TApplication* として公開し、CreateForm と終了時の破棄を C++Builder に揃える

- 状態: 承認
- 日付: 2026-09-26

## 背景

これまで C++ から Application を扱う手段が無く、フォームは `new TMainForm()`(Owner なし)で生成して `ShowModal()` で表示し、
最後に `Free()` するしかなかった。C API も `beth_Application_Run()` という引数なしの関数だけだった。

C++Builder のプロジェクトでは、次の形が定型であり、デザイナーが生成するコードもこれに従う。

```cpp
TForm1* Form1;   // フォームユニットのグローバル変数

WinMain(...)
{
    Application->Initialize();
    Application->CreateForm(__classid(TForm1), &Form1);   // 最初のフォームが MainForm になる
    Application->Run();                                   // MainForm を表示し、閉じられるまでループする
}   // Application の破棄に伴い、Application が所有するフォームも破棄される
```

LCL にも同じ構造(グローバル変数 `Application`・`CreateForm`・`Run`)があるが、次の制約がある。

- `MainForm` は読み取り専用で、設定されるのは `CreateForm` の中だけ(`UpdateMainForm` は `CreateForm` が生成中のフォームにしか効かない)。
  `CreateForm` はクラス参照を受け取るため、C/C++ で定義したクラスを渡せない。
- Application と、それが所有するフォームは DLL の切り離し時(LCL の終了処理)に破棄される。
  その時点では C++ の実行環境(レジストリ等の静的オブジェクト)が既に破棄されており、破棄通知を C++ へ送ると未定義動作になる。

## 検討した選択肢

- Application の公開の形: グローバルなポインタ変数 `TApplication* Application` / 初回アクセス時に生成する関数やプロキシ
- CreateForm の型の渡し方: `CreateForm<TForm1>(&Form1)` / 引数から推論する `CreateForm(&Form1)`
- MainForm の設定: LCL の `CreateForm` で素の TForm を生成し、C++ のコンストラクタにそのハンドルを引き取らせる /
  Pascal 側で生成の分岐フラグを持つ / MainForm の設定用の独自 API を追加する
- Application が所有するフォームの破棄: C++ の終了処理(`atexit`)で破棄する / DLL の切り離しに任せる

## 決定

1. **`extern TApplication* Application;` をグローバル変数として公開する**(C++Builder と同じ)。beth.cpp の動的初期化で生成する。
   他の翻訳単位のグローバル変数の初期化子から使うことは、初期化順序が規定されないため不可とする。
   `TApplication` は C++Builder に合わせて TComponent 直下に置く(LCL の中間クラス TCustomApplication は FCL 固有のため省く)。
2. **`Application->CreateForm(&Form1);`** とする。`__classid` は標準 C++ に無いため、型は引数(`T**`)から推論する。
   `T` は TForm の派生で、`(TComponent* AOwner)` を受け取って `TForm(AOwner)` に渡すコンストラクタを持つ(C++Builder のフォームと同じ形)。
3. **MainForm は LCL の `CreateForm` に設定させる。** `CreateForm` は、まず LCL の `CreateForm` で素の TForm を生成して「引き取り待ち」にし
   (C API `beth_TApplication_CreateForm`)、続く `new T(Application)` の中で TForm のコンストラクタが、
   Owner が Application ならそのハンドルを新規生成の代わりに使う。引き取られなかった場合(Owner を差し替えた・コンストラクタが例外を投げた)は破棄する。
   本家と違い、`Form1` への代入はコンストラクタの完了後になる。
4. **Application が所有するフォームは、main から戻った後の C++ の終了処理(`atexit`)で破棄する。**
   破棄通知によってラッパーのデストラクタも呼ばれる(C++Builder で Application の破棄に伴いフォームのデストラクタが呼ばれるのと同じ)。
   終了処理はレジストリの構築後に登録するため、レジストリの破棄より先に実行される。
   さらに Pascal 側では DLL の切り離しフック(ユニットの終了処理より前に呼ばれる)で破棄通知を止め、C から使う場合も含めて、
   終了処理中に破棄済みの呼び出し側へ通知しないようにした。C から使う場合、破棄を通知で受けたければ
   終了前に `beth_TComponent_DestroyComponents(app)` を呼ぶ。
5. **C API は `beth_GetApplication()` と `beth_TApplication_*(app, ...)` にする**(他のクラスと同じ命名規則。`beth_Application_Run()` 等は廃止)。
   あわせて `beth_TComponent_DestroyComponents` を追加した。

公開したメンバは `MainForm`・`Terminated`(読み取り専用)、`Title`・`ShowMainForm`、`Initialize()`・`CreateForm()`・`Run()`・`ProcessMessages()`・`Terminate()`。
`Initialize()` は LCL 側が DLL の読み込み時に初期化済みのため何もしない(C++Builder のコードとの互換のために置く)。

あわせて次を行った。

- **読み取り専用のプロパティ `ReadOnlyProperty<T>` を追加した**(代入はコンパイルエラー)。
- **`Property<T>`・`ReadOnlyProperty<T>` に `operator->` を追加した。** `Application->MainForm->Caption` や
  `Button1->Parent->Caption` のように、ポインタ型のプロパティからメンバへ直接たどれる(ポインタ以外の型で使うとコンパイルエラー)。

## 影響

- C++ のフォームは `TMainForm(TComponent* AOwner) : TForm(AOwner)` の形にし、`Application->CreateForm(&Form1); Application->Run();` で起動する
  (テストは追随済み)。Application が所有するフォームは `Free()` しなくてよい。
- C API の `beth_Application_Run()` / `beth_Application_ProcessMessages()` を使っていたコードは、
  `beth_TApplication_Run(beth_GetApplication())` 等に書き換えが必要(テストは追随済み)。
- `new TForm(Application)` で生成したフォームは MainForm にならない(C++Builder と同じ)。
- 既知の制約だったコンストラクタが例外を投げた場合の問題は、[ADR 0013](0013-string-return-bridge-reuse-ctor-exception.md) で対処した。
