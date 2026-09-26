# 0015. Tier 1(19 クラス)をすべて追加し、TStatusBar の既知の問題を記録する

- 状態: 承認
- 日付: 2026-09-26

## 背景

[docs/component-coverage.md](../component-coverage.md) で棚卸しした Tier 1(低コスト・高価値)のコントロールに、
1 バッチ目として着手した。いずれも基底が実装済みの TScrollingWinControl・TGraphicControl・TWinControl のいずれかで、
既存のパターン(`Property<T>`・protected hack・BridgeFor)の延長で追加できるものを選んだ。

- **TScrollBox**(forms.pp): TScrollingWinControl の直接の派生で、追加のメンバは無い。
- **TToggleBox**(stdctrls.pp): TCustomCheckBox の直接の派生で、Checked を共有する。追加のメンバは無い。
- **TBevel**(extctrls.pp): Shape(TBevelShape)・Style(TBevelStyle)。
- **TShape**(extctrls.pp の TCustomShape): Shape(TShapeType)・Pen/Brush(TCanvas と同じく、コントロールが
  所有する実体への非所有のビュー)。
- **TStaticText**(stdctrls.pp の TCustomStaticText): BorderStyle(TStaticBorderStyle)。
- **TStatusBar**(comctrls.pp): LCL に中間の TCustomStatusBar は無く、TWinControl の直接の派生。
  SimpleText/SimplePanel のみ対応し、Panels(複数区画のコレクション)は見送った。

列挙型(TBevelShape 等)は Pascal の `Ord`/型キャストで整数として C API・C++ の enum に渡す、
既存の TCloseAction 等と同じ方式にした。

## TStatusBar の既知の問題(重要)

実装後のテストで、**TStatusBar は Application->Run() がメッセージループを始める前にウィンドウハンドルを
作らせると、Win32 エラー 1406(「トップレベルの子ウィンドウを作成できません」)で失敗する**ことが分かった。
no_vcl の通常の使い方(`Application->CreateForm(&Form1); Application->Run();` で、フォームのコンストラクタの中で
子コントロールを生成し Parent を設定する)は、まさにこのタイミングでウィンドウハンドルを要求するため、
**素朴に実装すると実用上ほぼ確実にこの問題を踏む**。

### 調査で分かったこと

- 標準の Lazarus 実行ファイル(.lpr を fpc で直接コンパイルしたもの)では同じコードで問題が起きない。
  LCL の DLL を、LCL を使わない C/C++ の実行ファイルから `LoadLibrary` でホストする no_vcl 特有の現象と見られる。
- no_vcl の Pascal コード(`Watch` や `TStatusBar_Create` 自体)には依存しない。no_vcl を一切使わない、
  最小限の DLL(`TStatusBar.Create` して `Parent` を設定し `Show` するだけ)でも同じ Win32 エラーで再現した。
- **TStatusBar に固有の問題であり、ComCtrls の共通コントロール全般の問題ではない。** 同じ条件で
  TProgressBar(同じく comctl32 のネイティブコントロール)は問題なく生成・表示できた。`Align := alBottom`
  を明示的に設定しても TProgressBar では再現しなかったため、Align も原因ではない。
- 次はいずれも改善しなかった: ホスト側での `InitCommonControlsEx` の事前呼び出し、ホスト実行ファイルへの
  comctl32 v6 マニフェストの埋め込み、フォームのウィンドウハンドルを先に確保すること
  (`F.HandleNeeded`)、生成前に `Application.ProcessMessages` を 1 回呼ぶこと、`OnShow` イベントハンドラの
  中で生成すること(`OnShow` は `Show` の内部で、まだ `Application.Run` に入る前に呼ばれるため)。
- **`Application->Run()` がメッセージループに入った後(タイマーの `OnTimer` 等)で生成すると、問題なく成功する。**
  これが唯一確認できた回避策。

根本原因(comctl32 のスレッド・プロセス状態が、ホストプロセスのメッセージループが実際に走り出すまで
何らかの形で未完了になっている等)は特定できていない。LCL 自体のバグか、DLL ホスティングという
使用方法自体が LCL の想定外である可能性がある。

### 決定

1. TStatusBar の実装(Pascal・C API・C++)はそのまま採用する。問題は生成のタイミングに起因し、
   実装そのものは他のコントロールと同じ形で正しいため。
2. **既知の問題として、C API ヘッダ(no_vcl_c.h)と C++ ヘッダ(no_vcl.hpp)の TStatusBar の宣言に
   直接コメントで警告し、回避策(Interval=1 の使い捨てタイマーで Run() 開始後に生成する)を明記する。**
   利用者がこのコメントを読まずに素朴な使い方をすると確実にクラッシュするため、ドキュメントの中でも
   最も目につく場所(型の宣言そのもの)に書く。
3. test/main.c・test/main.cpp では、この回避策(1 回だけ発火するタイマーの中で生成し、発火後に
   タイマー自身を無効化する)を実装として示す。
4. 将来的に、TTrackBar・TUpDown・TTabControl 等の他の ComCtrls 系コントロールを追加する際は、
   同じ問題が起きないか都度確認する(TProgressBar は問題なかったため、全ての ComCtrls コントロールが
   影響を受けるわけではないと分かっているが、コントロールごとに確認が要る)。

## 影響

- TStatusBar を使うコードは、フォームのコンストラクタの中で直接 `Parent` を設定するのではなく、
  Interval=1 のタイマー等で Run() 開始後まで生成を遅らせる必要がある(C++Builder/Delphi の通常の
  書き方とは異なる、no_vcl 固有の制約)。
- 今回追加した他の 5 クラス(TScrollBox・TToggleBox・TBevel・TShape・TStaticText)にはこの問題は無く、
  通常どおりコンストラクタの中で生成・配置できる。

## 追記: Tier 1 の 2 バッチ目(範囲・数値系のコントロール)

続けて、TScrollBar(stdctrls.pp)・TTrackBar・TProgressBar・TUpDown(いずれも comctrls.pp)を追加した。

- **TScrollBar**: Kind(TScrollBarKind)・Min・Max・Position・PageSize・OnChange。
- **TTrackBar**: Min・Max・Position・OnChange。
- **TProgressBar**: Min・Max・Position。表示専用でイベントは無い。
- **TUpDown**: Min・Max・Position・Increment・Associate(対象の TWinControl。`TControl.Parent` と同じ
  ハンドル方式)。Min/Max/Position/Increment/Associate は LCL では `TCustomUpDown` の protected だが、
  唯一の具象クラス `TUpDown` が published にしている(`TCheckBox.Checked` と同じ形)ため、
  関数名は `TUpDown_*` にし、protected hack は使わず `TUpDown(Obj)` で直接アクセスした。
  OnClick・OnChanging は独自のシグネチャ(押されたボタンの方向・ユーザー操作かどうかを渡す)のため
  今回は見送った。

いずれも ComCtrls のネイティブコントロールだが、**TStatusBar のような生成タイミングの問題は無く**、
通常どおり Application->Run() より前(フォームのコンストラクタの中)で生成・配置できることを確認した
(最小限の再現コードで、Application->Run() 呼び出し前に 4 クラスすべてを生成・`Show()` して問題が
起きないことを確認済み)。TStatusBar の問題は ComCtrls 全般ではなく、TStatusBar 固有と改めて裏付けられた。

## 追記: Tier 1 の 3 バッチ目(Items を持つグループ・リスト系のコントロール)

続けて、TRadioGroup・TCheckGroup(いずれも extctrls.pp)・TCheckListBox(checklst.pas)を追加した。
基底はすべて実装済みの TCustomGroupBox・TCustomListBox で、ComCtrls のネイティブコントロールではないため
TStatusBar のような問題の心配は無い。

- **TRadioGroup**(TCustomRadioGroup): `Items`(TComboBox 等と同じ ItemsAdd/Clear/Count/GetText の形)・
  `ItemIndex`・`OnClick`。**`OnClick` は `TCustomRadioGroup` 自身が持つ独自のフィールドで、
  `TControl.OnClick` とは別物**(LCL の宣言で再宣言されて隠れている)。そのため `TComboBox.OnChange` と
  同じ理由で専用のブリッジ(`no_vcl_TCustomRadioGroup_SetOnClick`)を用意した。
- **TCheckGroup**(TCustomCheckGroup): `Items` + インデックス付きの `Checked[Index]`。
  インデックス付きプロパティは C++ では `GetChecked(int)`/`SetChecked(int, bool)` という素朴なメソッドの
  組として表す(`Property<T>` は単一の値しか表せないため)。
- **TCheckListBox**(TCustomCheckListBox): `Items` は基底 `TCustomListBox` のものをそのまま使い、
  インデックス付きの `Checked[Index]` と `OnClickCheck`(`TNotifyEvent`)を追加した。

## 追記: Tier 1 の 4 バッチ目(ボタンの派生)

続けて、TSpeedButton(buttons.pp、TCustomSpeedButton → TGraphicControl)・
TBitBtn(buttons.pp、TCustomBitBtn → TCustomButton、既存の TButton と同じ基底)を追加した。

- **TSpeedButton**: `Down`・`GroupIndex`・`Flat`・`AllowAllUp`。いずれも LCL では public のため
  protected hack は不要。`Caption`・`OnClick` は `TControl` のものをそのまま共有する
  (`TCustomSpeedButton` 自身は `OnClick` を再宣言していない)。
- **TBitBtn**: `Kind`(`TBitBtnKind`: bkOK・bkCancel 等の定型ボタン)。`Kind` を設定すると、
  LCL が既定の `Caption`(例: `bkOK` → `"&OK"`)を自動的に設定することをテストで確認した。

いずれもグラフィックス基盤(TBitmap/TPicture)に依存しない範囲(`Glyph` を除く)のみを実装した。
`TSpeedButton` は `TGraphicControl` のためウィンドウハンドルを持たず、実機のクリックシミュレーション
(BM_CLICK メッセージの送信)では操作できない点に注意(値の設定・読み出しと `OnClick` の配線自体は
正常に動作することを確認済み)。

## 追記: Tier 1 の 5 バッチ目(数値・書式付き Edit)

続けて、TFloatSpinEdit・TSpinEdit(いずれも spin.pp)・TMaskEdit(maskedit.pp、TCustomMaskEdit → TCustomEdit)
を追加した。

- **TFloatSpinEdit**(TCustomFloatSpinEdit): `Value`・`MinValue`・`MaxValue`・`Increment`(いずれも Double)・
  `DecimalPlaces`。C API に `no_vcl_float_t`(`double`)を新たに追加した。
- **TSpinEdit**(TCustomSpinEdit): LCL では `TCustomFloatSpinEdit` の派生で、`Value`・`MinValue`・
  `MaxValue`・`Increment` を **Integer で再宣言して Double 版を隠す**。C++ 側でも同じ隠蔽を再現するため、
  `TCustomSpinEdit` に同名の `Property<int>` を宣言し、C++ の名前隠蔽(派生クラスの同名メンバが基底の
  ものを隠す)で `TCustomFloatSpinEdit` の `Property<double>` を隠した。関数名は宣言元のクラスごとに
  分けている(`no_vcl_TCustomFloatSpinEdit_*` は double、`no_vcl_TCustomSpinEdit_*` は int)。
  テストで `SpinEdit1->Value` が int 版として振る舞うことを確認した。
- **TMaskEdit**(TCustomMaskEdit): `EditMask` のみ対応。`EditMask` は `TCustomMaskEdit` では protected だが、
  唯一の具象クラス `TMaskEdit` が published にしているため、`TUpDown` と同じ形で `TMaskEdit` に直接置いた。
  `EditText`・`SpaceChar`・`ValidationErrorMode` 等は見送った。

## 追記: Tier 1 の 6 バッチ目・最終バッチ(TTabControl)

続けて、TTabControl(comctrls.pp、TCustomTabControl → TWinControl)を追加した。これで
[docs/component-coverage.md](../component-coverage.md) の Tier 1 に挙げたクラスはすべて実装済みになった。

- **TTabControl**: `Tabs`(ItemsAdd/Clear/Count/GetText の形)・`TabIndex`・`OnChange`。
  `Tabs`/`TabIndex`/`OnChange` は LCL では `TCustomTabControl` の protected だが、唯一の具象クラス
  `TTabControl` が独自のフィールドで再宣言して published にしているため、`TUpDown` と同じ形で
  `TTabControl` に直接置いた。
- **TPageControl・TTabSheet(ページ付きのタブ)は今回見送った。** `TTabSheet` は `TCustomPage`
  (`TWinControl` 派生)で、タブごとにページというコンポーネントを所有し、その生成・破棄を no_vcl 側で
  どう表すか(`TPageControl.Pages[i]` の C++ での表現、`AddPage`/`DeletePage` 相当の API 設計)が
  単純な `Property<T>` の追加では済まないため、Tier 2 に位置づけ直した
  (component-coverage.md を更新済み)。

## 影響(3 バッチ目の追記分)

- インデックス付きプロパティ(TCheckGroup・TCheckListBox の Checked)は `Property<T>` ではなく、
  素朴な `Get`/`Set` メソッドの組として C++ に表す前例ができた。今後、同様のインデックス付き
  プロパティ(TListView の選択状態等)にもこの形を踏襲できる。
