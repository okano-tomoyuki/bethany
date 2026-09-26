# 0014. TControl / TWinControl のキー入力・マウス操作イベントを追加し、TForm もこれを継承する

- 状態: 承認
- 日付: 2026-09-26

## 背景

TForm(TCustomForm)が公開しているイベントのうち、[ADR 0009](0009-events-as-properties-with-sender.md)〜[0012](0012-remaining-form-events.md)で
対応していなかった、TControl・TWinControl 由来のイベントが残っていた。
LCL のソースで宣言元を確認したところ、次のように分かれていた。

- **OnClick・OnResize は TControl の public。**(OnClick は既に対応済み。)
- **OnDblClick・OnMouseDown・OnMouseUp・OnMouseMove・OnMouseEnter・OnMouseLeave・OnMouseWheel は TControl の protected。**
  Text・Checked と同じく protected hack(`TControlAccess`)が必要。
- **OnKeyDown・OnKeyUp・OnKeyPress は TWinControl の public。**

いずれも TControl・TWinControl で宣言されているため、docs/class-hierarchy.md の「LCL で公開しているメンバはそのクラスに置く」
方針に従えば、TForm 専用にするのではなく TControl・TWinControl に置くべきものだった。
これにより、Button や Edit など他のコントロールも含めて、すべてのコントロールでこれらのイベントが使えるようになる
(LCL・VCL でもマウス/キーイベントはコントロール全般が持つ機能で、フォーム固有ではない)。

なお、TForm が公開するイベントのうち、ドッキング・ドラッグ関連(OnDockDrop・OnDragDrop・OnStartDock 等)、
OnContextPopup・OnGetSiteInfo・OnChangeBounds・OnConstrainedResize・OnShortCut・OnShowHint・OnWindowStateChange・
OnUTF8KeyPress・OnMouseWheelUp/Down/Horz/Left/Right は、利用頻度が低いため今回は見送り、todo.md に残した。

## 検討した選択肢

- Shift 状態(TShiftState、17 要素の集合型)の C API での表し方: 各要素をビットとして表した整数(ビット集合) /
  真偽値の配列 / 個別の bool 引数
- 実装場所: TCustomForm に追加(TForm 専用) / TControl・TWinControl に追加(全コントロール共通、docs/class-hierarchy.md の方針どおり)

## 決定

1. **Shift 状態はビット集合として表す。** Pascal 側は `TShiftStateEnum` を 1 ビットずつ走査して `LongWord` に変換する
   (`{$packset}` の効果で集合の実バイト数が変わっても影響を受けないループ実装)。C API・C++ とも
   `no_vcl_ss*` / `no_vcl::ss*`(`ssShift`, `ssCtrl`, `ssLeft` 等)の定数をビット OR して使う、`unsigned int` の単純なビット集合とする
   (TColor と同様、Pascal の集合型を素の整数として扱う実務的な選択)。
2. **マウスボタンは `TMouseButton` の序数と同じ整数(`no_vcl_mb*` / `no_vcl::TMouseButton`)。**
3. **実装場所は TControl・TWinControl。** docs/class-hierarchy.md の「LCL で公開している階層に置く」方針に従い、
   TForm 専用にはしない。TForm は TWinControl の子孫として、これらをそのまま継承する。
4. **書き換え可能な引数を持つイベント(OnKeyDown・OnKeyUp・OnKeyPress・OnMouseWheel)は、OnClose と同じくポインタ経由で書き換える。**
   C++ 側は該当の引数を参照(`int& Key` 等)で渡す。Key を 0 にすると、その入力を LCL に渡さない
   (LCL の `var Key: Word` を 0 にする処理と同じ)。
5. **ブリッジは種類ごとに専用のクラスを用意し、[ADR 0013](0013-string-return-bridge-reuse-ctor-exception.md)と同じ
   「イベントに現在設定されているブリッジを判定して再利用する」パターンに従う。**

追加したメンバ:

| メンバ | 型 | 配置 |
|---|---|---|
| OnDblClick, OnResize, OnMouseEnter, OnMouseLeave | `Property<TNotifyEvent>` | TControl |
| OnMouseDown, OnMouseUp | `Property<TMouseEvent>`(`std::function<void(TObject*, TMouseButton, TShiftState, int X, int Y)>`) | TControl |
| OnMouseMove | `Property<TMouseMoveEvent>`(`std::function<void(TObject*, TShiftState, int X, int Y)>`) | TControl |
| OnMouseWheel | `Property<TMouseWheelEvent>`(`std::function<void(TObject*, TShiftState, int WheelDelta, int X, int Y, bool& Handled)>`) | TControl |
| OnKeyDown, OnKeyUp | `Property<TKeyEvent>`(`std::function<void(TObject*, int& Key, TShiftState)>`) | TWinControl |
| OnKeyPress | `Property<TKeyPressEvent>`(`std::function<void(TObject*, char& Key)>`) | TWinControl |

あわせて、TCustomForm の各イベントの Setter で使っていた共通ヘルパ(ハンドラの保持 + 初回のブリッジ登録)を
`SetSimpleEvent`(no_vcl.cpp 内の関数テンプレート)としてファイル冒頭に移動し、TControl・TWinControl からも使えるようにした。

## 影響

- Button・Edit・Panel など、ウィンドウを持つすべてのコントロールでマウス操作を受け取れるようになった
  (実測では、ボタンのクリックで OnMouseDown → OnClick → OnMouseUp の順に発生し、OnMouseEnter/OnMouseLeave も
  実際のマウスメッセージから正しく発生することを確認した)。
- OnDblClick・OnMouseWheel は自動テストでは未検証(ダブルクリック/ホイールメッセージを送っていない)。
- ドッキング・ドラッグ関連、OnContextPopup 等の残りの TForm 公開イベントは未対応(todo.md に記録)。
