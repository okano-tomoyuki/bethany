# 0028. TLabeledEdit と、利用者が生成する TStringList を追加する

- 状態: 承認
- 日付: 2026-09-27

## 背景

Tier 2 と TStrings([ADR 0027](0027-tstrings.md))で一区切りとした時点で、見送っていたものが 2 つ残っていた。

- **TLabeledEdit**(Tier 1 で唯一見送ったもの):
  - EditLabel は LCL が内部で生成する子コンポーネントで、当時はラップの仕組みが無かった。
  - その仕組み(`WrapExisting`)は [ADR 0017](0017-menus-and-wrapping-lcl-created-components.md) で入り、着手できる状態だった。
- **単独の TStringList**:
  - ADR 0027 の TStrings はコントロールが持つ一覧のビューで、利用者が一覧を生成する手段が無かった。
  - VCL のコードでは、設定の読み書き(Values)・区切り文字列の分解(DelimitedText)・ファイルの読み込みのために、
    TStringList を一時的に生成することが多い。

LCL・FPC のソース(`extctrls.pp`・`include/customlabelededit.inc`・`include/boundlabel.inc`・FPC の `classes/stringl.inc`)で確認したこと:

- **TCustomLabeledEdit**:
  - 生成時に `TBoundLabel`(TCustomLabel の派生)を、自分を Owner として作る。LabeledEdit と一緒に破棄される。
  - SetParent・LabelPosition・LabelSpacing の変更のたびに、ラベルの Parent をエディットと同じにし、アンカーで位置を決める
    (lpLeft なら、ラベルの右端をエディットの左端から LabelSpacing だけ離す)。
  - 既定は lpAbove・3。
- **TStringList**(FPC 3.2.2):
  - Duplicates の既定は dupIgnore。
  - Sorted のとき、Insert と Strings への代入は例外になる(Exchange はならない)。
  - Find は Sorted でないと例外になる。
  - CaseSensitive の並べ替えは AnsiCompareStr(地域の設定に従う)で、大文字が先に並ぶとは限らない(Delphi と同じ)。
- **TStrings.Values**(FPC 3.2.2):
  - `Values[Name] := ''` は行を削除せず、`Name=` にする(Delphi は削除する)。
  - 削除するのは `ValueFromIndex[i] := ''` のほう。

## 検討した選択肢

TStringList の C++ での持ち方:

- 選択肢A: TStrings とは別のクラスにし、TStrings の操作を重複して持たせる。
  `Items->Assign(List)` のように TStrings* を受け取るものに渡せない。
- 選択肢B: TStrings の派生にし、自分のハンドルを中身とする。
  TStrings は「所有者 + 取得関数」で中身を得るので、所有者を自分、取得関数を恒等関数にすれば、そのまま同じ操作が使える。

TStringList の寿命:

- 選択肢A: TComponent と同じく、Free() で破棄し、デストラクタは protected にする。
  VCL の TStringList は TComponent ではなく、`delete` で破棄するため、移植のたびに書き換えが要る。
- 選択肢B: VCL と同じく、`new` して `delete` する(デストラクタで LCL の TStringList を破棄する)。
  破棄通知の対象にする必要が無い(LCL 側から勝手に破棄されない)ので、スタックや値メンバに置いてもよい。

## 決定

- **TLabeledEdit**:
  - C API:
    - `no_vcl_TLabeledEdit_Create`。
    - `no_vcl_TCustomLabeledEdit_GetEditLabel`: 返すときに Watch する(ADR 0017 と同じ形)。
    - `no_vcl_TCustomLabeledEdit_Get/SetLabelPosition`・`Get/SetLabelSpacing`。
    - 列挙は `no_vcl_lpAbove` 等。
  - C++:
    - `TBoundLabel`(TCustomLabel)は、ハンドルを受け取るコンストラクタを private にし、`friend class TComponent` とした
      (TTabSheet・TMenuItem と同じく、利用者は生成しない)。
    - `TCustomLabeledEdit`(TCustomEdit)に `ReadOnlyProperty<TBoundLabel*> EditLabel`・`LabelPosition`・`LabelSpacing` を置き、
      EditLabel は `WrapExisting<TBoundLabel>` で返す。TLabeledEdit はその派生。
    - `LabeledEdit1->EditLabel->Caption = "Zip:";` のように VCL と同じく書ける。
- **TStringList**:
  - C API:
    - `no_vcl_TStringList_Create`・`Destroy`・`Sort`・`Find`・`Get/SetSorted`・`Get/SetDuplicates`・`Get/SetCaseSensitive`。
    - 列挙は `no_vcl_dupIgnore` 等。
    - それ以外の操作は `no_vcl_TStrings_*` をそのまま使う。
    - ハンドルは生成した側の持ち物なので、コントロールの Items と違って保存してよい。
  - C++:
    - `TStringList : public TStrings` に `Sorted`・`Duplicates`・`CaseSensitive`・`Sort`・`Find` を置いた。
    - TStrings に protected のコンストラクタ `TStrings(no_vcl_obj_t handle)` を足した。所有者を自分、取得関数を恒等関数とし、
      `Handle()` も `Current()` も自分のハンドルを返す。
    - 寿命: 選択肢B(VCL と同じく `new` / `delete`。スタックや値メンバにも置ける)。
- **TStrings に足したもの**(TStringList でよく使うが、宣言元は TStrings なのでコントロールの Items でも使える):
  - 名前=値 の行: `Names[i]`・`Values["name"]`・`ValueFromIndex[i]`・`IndexOfName`。
  - 区切り文字: `Delimiter`・`StrictDelimiter`・`DelimitedText`。
  - ファイル: `LoadFromFile`・`SaveToFile`(ファイル名・内容とも UTF-8 のまま扱う)。
  - `Values` は文字列を添字にするため、`IndexedProperty` に添字の型の引数を足した(`IndexedProperty<T, I = int>`。既存の使い方は変わらない)。
- **Values に空文字列を代入したときの動き**: LCL(FPC)に合わせ、行は残す。VCL と違うことはヘッダーに明記した
  (行を削除するには `ValueFromIndex[i] = ""` か Delete を使う)。
- 見送ったもの:
  - TStringList の OnChange/OnChanging、OwnsObjects、CustomSort。
  - NameValueSeparator・QuoteChar。
  - TBoundLabel 固有のプロパティ(Alignment・Layout・WordWrap 等。TCustomLabel 全体で未対応)。

## 実装して分かったこと

- **ラベルの位置**:
  - lpLeft・LabelSpacing 6 のとき、フォームの表示後(OnShow)にラベルの右端 + 6 がエディットの左端に一致した(Win32・GTK2 の両方)。
  - ラベルの Parent は、エディットの Parent を設定した時点でフォームになる。生成直後(Parent 未設定)はラベルの Parent も無い。
- **EditLabel のラッパー**:
  - 2 回目以降の取得でも同じラッパーが返る。
  - C のテストで、LabeledEdit と一緒に EditLabel にも破棄通知が届くことを確認した(解放数が 71 から 73 に増えた)。
- **TStringList**:
  - Sorted + dupIgnore では、大文字と小文字だけが違う重複も加えない(CaseSensitive が false のため)。
  - Find は、見つからないとき挿入すべき位置を返す。
  - `Items->Assign(list)` でコントロールに写せる。
  - 日本語のファイル名(UTF-8)で SaveToFile・LoadFromFile ができた(Win32)。
- **Values に空文字列を代入**: 上記のとおり、行は `host=` として残った(FPC の仕様)。
- **例外**: Sorted の一覧への Insert・Find の誤用などで LCL が送出する例外は、これまでの範囲外の添字と同じく、呼び出し側には捕捉されない。
  使い方の制約としてヘッダーに書いた。

## 影響

- Tier 1 で見送っていたものは無くなった。
- VCL の TStringList を使うコード(`new TStringList`・`Values[...]`・`DelimitedText`・`LoadFromFile`・`delete`)を、書き換えずに移植できる。
  ただし、`Values[Name] = ""` で行を削除しているコードは書き換えが要る。
- 文字列を添字にするプロパティが今後も出てきたら、同じ `IndexedProperty<T, std::string>` で表せる。
