# VS Code 拡張とデザイナーの画面の設計

[ADR 0035](../adr/0035-designer-in-this-repository.md) の順序の 4 番目。VS Code 拡張(カスタムエディタ)と、
Webview のデザイナーの画面の設計。tk-designer(`cc00e22`)の拡張ホスト・Webview の骨組みを土台にし、
DSL・カタログ・キャンバスは no_vcl に合わせて作る。キャンバスの方式は [ADR 0036](../adr/0036-designer-canvas.md)。

## 1. 範囲

### 1.1 最初の版(MVP)で作るもの

- `*.nvform.json` を開くカスタムエディタ(テキストエディタでも開ける。同時に開いても整合する)。
- 画面: パレット・キャンバス・構造の木・プロパティ(オブジェクトインスペクタ)・問題の一覧。
- キャンバス: コントロールの描画(Windows の見た目の近似)、選択(複数選択を含む)、ドラッグでの移動と大きさの変更、
  パレットからの追加、親の変更、Align・Anchors の配置の計算、非ビジュアルコンポーネントのアイコン、メインメニューの表示。
- プロパティ: カタログの型に応じた入力欄(入れ子のオブジェクト・集合・参照・TStrings を含む)、イベントとハンドラ名。
- メニューの項目の編集(構造の木の上で追加・削除・並べ替え)。
- コード生成のコマンド(明示的な操作で生成する。tk-designer ADR 0010)と、生成の設定の画面。
- 新しいフォームを作るコマンド。
- 問題パネルへの診断の表示(JSON 上の位置をファイル上の範囲に変換する)。
- 日本語と英語(tk-designer ADR 0014)。JSON Schema(`jsonValidation`)。

### 1.2 後で作るもの

- 実物の LCL でのプレビューの実行(生成したコードを動かす)。
- 整列・大きさを揃える操作、タブ順の編集の画面、クリップボード(コピー・貼り付け)。
- フォントの計測を OS に合わせる仕組み(ADR 0036 の影響)。
- DSL の §10 Q6(コレクション)・Q7(画像)に当たる編集。

## 2. パッケージ

ADR 0035 のとおり、tk-designer と同じ分け方と依存の向きにする。

```
webview   ─▶ core, codegen(名前の既定値など純粋な関数のみ)
extension ─▶ core, codegen
cli       ─▶ core, codegen
codegen   ─▶ core
```

extension と webview は互いを import せず、core の `protocol.ts` のメッセージでだけ通信する。

| パッケージ | 加えるもの |
|---|---|
| core | `dsl/serialize.ts`(決まった形での書き出し)・`dsl/jsonSchema.ts`・`dsl/locate.ts`(JSON 上の位置 → テキストの範囲)・`edit/`(編集コマンド・初期値・名前の採番・入力値の変換・テキストの最小差分)・`layout/`(配置の計算・クライアント領域の余白・AutoSize の見積もり)・`protocol.ts` |
| extension | カスタムエディタ・編集の適用・コード生成・新しいフォーム・診断・l10n(tk-designer の `designerEditorProvider`・`applyEditCommand`・`generateCode`・`newScreen` を複製して直す) |
| webview | React + Zustand(+ Immer)・Vite。ストア・メッセージの橋渡しは tk-designer を複製し、キャンバス・インスペクタ・パレット・構造の木は作り直す |

tk-designer の GDI によるフォントの計測(`fontService`・`gdi`)は、最初の版では複製しない(§5.4)。

## 3. 編集モデル

tk-designer ADR 0006 と同じ(docs/editing.md の §3 の流れもそのまま)。

- **TextDocument を唯一の正とする**(`CustomTextEditorProvider`)。Undo・Redo・保存・未保存の表示は VS Code に任せる。
- 画面の操作は、core の**編集コマンド**(純粋な関数 `applyCommand(doc, command)`)で表す。Webview はローカルで適用して
  すぐに表示し(楽観的な反映)、拡張に送る。拡張は TextDocument の現在の内容に同じコマンドを適用し、決まった形で書き出して、
  変わった範囲だけを `WorkspaceEdit` で置き換える。
- **1 回の操作 = 1 回の編集 = 1 回の Undo。** ドラッグ中の表示は Webview の一時的な状態で行い、離したときに 1 つのコマンドを送る。
- JSON として読めない・スキーマに合わない内容では、画面を編集できない状態にして理由を表示する。意味の検証のエラー
  (未知のプロパティ等)があっても編集はできる(エラーは問題の一覧に出す)。

### 3.1 編集コマンド

ノードは名前で指す(DSL の `name` はフォームの中で一意)。フォームは `form.name`。

| コマンド | 内容 |
|---|---|
| `addControl` | コントロールを追加する(`parent`・`index`・`className`・`name`・`bounds`)。名前は呼び出し側で採番する(楽観的な反映と同じ名前にするため)。初期値は §3.2 |
| `addComponent` | 非ビジュアルコンポーネントを追加する(`className`・`name`・`design`) |
| `addMenuItem` | メニュー項目を追加する(`menu` か `parentItem`・`index`・`name`・`caption`) |
| `removeNodes` | ノード(複数)を子孫ごと削除する。削除したコンポーネントを参照するプロパティも削除する(参照切れを残さない)。フォームは削除できない |
| `moveControls` | コントロール(複数)の親・並びの位置を変える。子孫の中へは移動できない。親の制約(`acceptsControls`・`childClasses`・`parentClasses`)を検査する |
| `moveMenuItem` | メニュー項目の親・位置を変える(別のメニューへも移せる) |
| `renameNode` | 名前を変える。そのノードを参照するプロパティ(`PopupMenu`・`Menu`・`ActivePage` 等)も書き換える。識別子・予約名・重複を検査する |
| `setProperties` | プロパティを設定・削除する(`value: undefined` で削除)。対象は複数のノード・複数のプロパティを取れる(複数選択での変更、移動・大きさの変更)。入れ子のオブジェクトは `path`(`["Font", "Size"]`)で指し、空になったオブジェクトは削除する |
| `setEvent` | イベントのハンドラ名を設定・削除する。イベントの型が合わないハンドラ名は拒む(handler-signature-conflict を作らない) |
| `renameHandler` | ハンドラを改名し、すべての参照を書き換える。既存のハンドラ名への改名は、型が同じなら統合として許す |
| `setDesignPosition` | 非ビジュアルコンポーネントのアイコンの位置(`design`)を変える |
| `setCodegen` | コード生成の設定を置き換える(空なら `codegen` ごと削除する) |
| `batch` | 複数のコマンドを 1 回の変更として適用する(途中で失敗したら何も変えない) |

- 配置に関わるコマンド(追加・削除・移動・Align・Anchors・位置と大きさ・BorderSpacing・Constraints・フォームの大きさ・
  TPanel の枠などクライアント領域に関わるプロパティ・`Visible`)の後は、**影響を受けたコンテナの配置を core で計算し直し**、
  Align で寄せたコントロールと Anchors で追従するコントロールの Left・Top・Width・Height を書き換える(§4)。
  計算し直しはコマンドの適用の中で行うので、Webview と拡張で同じ結果になる。
- どのコマンドも、適用の後に意味の検証(dsl-spec.md §7)でエラーが増えないことをテストで確かめる(tk-designer と同じ)。

### 3.2 追加したときの初期値

C++Builder・Lazarus と同じにする。

- 名前: クラス名から `T` を除いたものに連番(`Button1`・`Button2`)。メニュー項目は Caption から作り(`&File` → `File1`)、
  区切り線は `N1`。予約名・既存の名前と衝突しない最小の番号にする。
- 大きさ: カタログの `defaultSize`。パレットからドラッグして範囲を描いたときは、その範囲。
- Caption・Text を持つクラス(TButton・TLabel・TCheckBox・TGroupBox・TPanel 等)は名前を設定する(C++Builder と同じ)。
  TEdit・TMemo 等の内容は空のまま。
- TTabSheet は TPageControl の末尾に追加し、Caption は名前。TPageControl を追加したときはタブを作らない(構造の木・
  キャンバスの右クリックの「タブを追加」で作る)。

## 4. 配置の計算(core/src/layout)

ADR 0036 の決定 2〜4。

### 4.1 入力と出力

- 入力: コンテナ(フォーム・コントロール)の**クライアント領域の大きさ**と、子の Left・Top・Width・Height・Align・Anchors・
  BorderSpacing・Constraints・Visible。
- 出力: 子の Left・Top・Width・Height。Align が alNone で Anchors が既定(akLeft・akTop)のコントロールは、
  Constraints による制限の他は変えない。

### 4.2 規則(LCL の `TWinControl.AlignControls` の移植)

- Align の順: alTop → alBottom → alLeft → alRight → alClient。残りの領域から順に切り出す。
- 同じ Align の兄弟の並び: DSL の配列の順とする。デザイナーは、並びと位置の順(alTop なら Top の昇順)が一致するように
  書き出す(LCL は位置で並べ直すので、一致させておけば生成したコードでも同じ並びになる)。
- BorderSpacing: `Around` と `Left`・`Top`・`Right`・`Bottom` の和を、Align で寄せるときの余白にする。
- Constraints: `MinWidth`・`MaxWidth`・`MinHeight`・`MaxHeight` で大きさを制限する。
- Anchors: 親のクライアント領域の大きさが変わったとき(フォームの大きさの変更・親の Align による変化)に、
  akRight だけなら右との距離を保って移動し、akLeft と akRight なら幅を広げる(akTop・akBottom も同じ)。
  どちらも無ければ中央の位置の比率を保つ。
- 非表示(`Visible: false`)のコントロールは、Align の計算では場所を取らない(LCL と同じ)。キャンバスには薄く描く。

実物と違う規則が見つかったら、記録(§4.4)で確かめてから直す。

### 4.3 クライアント領域の余白

| クラス | 余白の決まり方 |
|---|---|
| TForm | 余白なし(Width・Height がクライアント領域。dsl-spec.md §10 Q9) |
| TPanel | `BevelOuter`・`BevelInner`・`BevelWidth`・`BorderWidth` から計算する |
| TGroupBox・TRadioGroup・TCheckGroup | 見出しの高さ(フォントに依存)と枠。記録した値 |
| TTabSheet | TPageControl のタブの高さと枠を除いた大きさ(TTabSheet の Left・Top・Width・Height は DSL に書かず、親から計算する) |
| TScrollBox | 枠(`BorderStyle`) |
| その他(TToolBar・TCoolBar・TTabControl) | 記録した値 |

記録は `designer/tools/layout/record-insets.mts`(Python のバインディングで各クラスのコントロールに alClient の子を置き、
子の配置から余白を求める)で行い、`core/src/layout/insets.json` にコミットする。

### 4.4 照合

`designer/tools/layout/record.mts` で、配置の見本(`core/src/layout/fixtures/*.nvform.json`: Align の組み合わせ・
BorderSpacing・Constraints・入れ子・Anchors の追従)を Python のバインディングで実物の LCL に表示し、表示した後の配置と、
フォームの大きさを変えた後の配置を `*.lcl.json` に記録する。テストで core の計算結果と一致することを確かめる。
記録は Windows で行う(codegen-design.md §7 と同じ環境)。

### 4.5 AutoSize の見積もり

AutoSize のコントロール(dsl-spec.md §5)の大きさは、キャンバスで次のように見積もり、DSL にもその値を書く。

- TLabel・TStaticText: 文字列の幅と行の高さ(Webview で測る)。
- TCheckBox・TRadioButton: 文字列の幅 + チェックの枠と間隔(記録した値)。
- TEdit・TSpinEdit・TComboBox 等: 高さだけを固定の値(記録した値)にし、幅は利用者が決める。

見積もりは生成したコードの実行での確認の対象にしない(codegen-design.md §7.1)。

## 5. キャンバス

### 5.1 描画

- フォームは、Windows 11 の枠とタイトルバー(Caption・アイコン・最小化などのボタン。操作はできない)の内側に、
  メインメニュー(`Menu` に TMainMenu を設定していれば、その項目の Caption の並び)と、Width・Height のクライアント領域を描く。
- コントロールはクラスごとの React の部品で描く(`webview/src/canvas/controls/`)。位置は親のクライアント領域からの
  絶対配置。見た目は Windows 11 のテーマに近づける(ボタンの枠・チェックの枠・編集欄の下線・タブ等)。
  専用の部品が無いクラスは、枠とクラス名・名前だけを描く。
- 色は `Color`・`Font.Color`(と `ParentColor`・`ParentFont` による親からの継承)を反映する。`clDefault` はクラスごとの既定の色。
  フォントは `Font.Name`・`Size`・`Style` を反映し、`default` は §5.4。
- 非ビジュアルコンポーネントは、フォームの上の `design` の位置に、クラスのアイコンと名前を描く(C++Builder・Lazarus と同じ)。
  `design` が無いものは、左下から順に並べて表示する(書き込むのは移動したとき)。
- TPageControl は `ActivePage` のタブの中身だけを描く。キャンバスでタブをクリックすると、そのタブを表示する
  (表示するタブは UI の状態で、DSL の `ActivePage` は変えない。変えるのはプロパティの欄から)。
- 倍率(ズーム)は 50%〜200%。キャンバスの座標は 96dpi の論理ピクセル。

### 5.2 操作

| 操作 | 内容 |
|---|---|
| クリック | 選択する(背景はフォーム)。Shift・Ctrl で選択に加える・外す。同じ親の兄弟だけを複数選択できる |
| 背景からのドラッグ | 範囲に入ったコントロールを選択する |
| ドラッグ | 選択したコントロールを移動する。別のコンテナの上で離すと、そのコンテナに移す(親の制約を満たすときだけ) |
| 8 つのつまみのドラッグ | 大きさを変える(複数選択では 1 つだけ) |
| 矢印キー | 1px 移動する。Shift+矢印で 1px 大きさを変える(C++Builder の Ctrl・Shift と同じ意味に合わせるかは §9 E3) |
| パレットのクラスを選んでキャンバスをクリック・ドラッグ | その位置に既定の大きさで追加する、または描いた範囲の大きさで追加する |
| Delete | 選択したものを削除する |
| Esc | 親を選択する |
| ダブルクリック | 既定のイベント(TButton は OnClick、TForm は OnCreate 等。カタログの補足 overlay.json に `defaultEvent` を加える)のハンドラ名を設定する(未設定なら名前 + イベント名から `on` を除いたもの) |
| 右クリック | メニュー(削除・前面へ/背面へ・タブを追加・親を選択) |

- 移動と大きさの変更は、8px の格子に合わせる(Alt を押している間は合わせない。格子の大きさは設定で変えられる)。
- Align で寄せたコントロールは、寄せた方向と直交する向きには動かせない。ドラッグで同じ Align の兄弟の並びを変える。
- ドラッグ中は、移動後の枠と、配置を計算し直した結果を一時的に表示する。離したときに `batch`(移動・親の変更・
  配置の計算し直し)を 1 回送る。

### 5.3 選択と構造の木

構造の木には、フォーム・コントロールの入れ子・非ビジュアルコンポーネント(メニューなら項目の入れ子)を表示する。
キャンバスと選択を共有する。木の上でもドラッグで親・並びを変えられ(メニュー項目もここで編集する)、
右クリックで追加(「項目を追加」「サブメニューを追加」「区切り線を追加」)・削除ができる。

### 5.4 フォント

LCL の `default` のフォントは、Windows ではメッセージのフォント(日本語版は Yu Gothic UI 9pt、英語版は Segoe UI 9pt)。

- 最初の版では、設定 `noVclDesigner.canvas.fontFamily`・`fontSize`(空なら VS Code の UI の言語が日本語なら Yu Gothic UI、
  それ以外は Segoe UI、9pt)で決め、文字列の幅はブラウザで測る。
- ずれが問題になったら、tk-designer ADR 0013 の GDI での計測を複製する。

## 6. プロパティ(オブジェクトインスペクタ)

選択したノードのプロパティとイベントを、C++Builder のオブジェクトインスペクタと同じく 2 つのタブで表示する。
複数選択では、共通するプロパティだけを表示し、値が違うものは空欄にする(入力すると全部に設定する)。

- 並び: カタログの順(`displayOrder`。catalog.md)。「設定したものだけ」の絞り込みと、名前の検索を置く。
- 既定値は薄く表示し、設定した値は太字にする。空にすると(または「既定に戻す」で)削除する。
- 入力の確定は tk-designer と同じ(Enter かフォーカスを外したときに確定、Esc で戻す。選択肢・チェックは変えた時点で確定)。
- 形式が不正な入力は確定せず、その欄にエラーを表示する。確定した後の検証エラーも該当する欄に表示する。

| 型(カタログ) | 入力欄 |
|---|---|
| int・double | 数値の入力 |
| bool | チェック |
| string | 文字の入力(`Caption` 等の `&` はそのまま) |
| 列挙型・TCursor | 選択肢 |
| 集合型・ビット集合 | 展開して要素ごとのチェック(閉じているときは `[akLeft,akTop]` の形で表示) |
| TColor | 色の見本 + 選択肢(定数)+ `#RRGGBB` の入力 + 色の選択 |
| TShortCut | 選択肢(よく使う組み合わせ)+ 文字の入力(`TextToShortCut` で解釈できるもの) |
| 参照 | 型の合うコンポーネントの選択肢 |
| 入れ子のオブジェクト | 展開して中のプロパティ |
| TStrings | 1 行目の表示 + 「…」で複数行の入力のダイアログ |

イベントのタブ:

- 欄は、同じイベントの型の既存のハンドラ名の選択肢と、文字の入力。ダブルクリックで既定の名前(`OkButtonClick`)を設定する。
- ハンドラ名の変更は、そのイベントだけを変えるか、ハンドラを改名するか(`renameHandler`)を選べるようにする。
- コードへの移動は行わない(生成するまでハンドラのコードは無い)。§9 E4。

## 7. コード生成

tk-designer ADR 0010 と同じく、利用者の明示的な操作で生成する(保存のたびには生成しない)。

- コマンド「コードを生成」(エディタのタイトルのボタン・コマンドパレット・画面のボタン)。拡張で `generateAll` を呼び、
  結果を `WorkspaceEdit` で既存のファイルに適用する(新しいファイルは作る)。生成したファイルは保存する。
- 区間の中の手編集(`modifiedRegions`)があるときは、上書きしてよいか確認する(CLI の `--force` に当たる)。
  DSL から消えた名前の警告は、通知と出力のチャネルに出す。
- 検証のエラーがあるときは生成しない。
- 画面の「コード生成」の欄で `codegen` を編集する(ターゲットの有無・クラス名・出力先。空欄は既定値を薄く表示)。

## 8. 拡張のその他

- **新しいフォーム**: エクスプローラーの右クリックと「新しいファイル」から。フォームの名前を入力し、`<名前>.nvform.json` を
  既定の内容(TForm・320×240・Caption・`codegen` に C++ と Python)で作って開く。
- **診断**: TextDocument が変わるたびに検証し、`DiagnosticCollection` に出す。JSON 上の位置は `core/src/dsl/locate.ts`
  (JSON のテキストを走査して、パスの値の範囲を求める)で範囲にする。
- **JSON Schema**: Zod のスキーマから `schema/nvform.schema.json` を生成してコミットし(`--check` で食い違いを検査)、
  `jsonValidation` に登録する。
- **l10n**: 拡張は `vscode.l10n`、Webview・core・codegen・CLI は `@vscode/l10n`。元の言語は英語、訳は日本語(`l10n/bundle.l10n.ja.json`)。
  tk-designer の `tools/l10n`(訳の漏れの検査・統合)を複製する。
- **梱包**: vsce(`--no-dependencies`、バンドルした成果物だけ)。拡張の ID は `no-vcl-designer`、表示名は「no_vcl Designer」。

## 9. 未決の論点

| # | 論点 | 案 |
|---|---|---|
| E1 | 非ビジュアルコンポーネントのアイコン | `@vscode/codicons` の汎用のアイコンか、クラスごとに小さな SVG を作るか。最初は codicons で始める |
| E2 | 複数のフォーム・フォーム間の参照 | 扱わない(1 ファイル = 1 フォーム。別のフォームのコンポーネントは参照できない) |
| E3 | キー操作の割り当て | C++Builder は Ctrl+矢印で移動、Shift+矢印で大きさ。VS Code の Ctrl+矢印と衝突しないかを確かめて決める |
| E4 | ハンドラのコードへの移動 | 生成したファイルの中のハンドラの位置を探して開く。コード生成の後の機能として検討する |
| E5 | Linux での表示 | ADR 0036 のとおり Windows の見た目で表示する。GTK2 の余白の記録を加えるかは、Linux の利用者が出てから決める |

## 10. 作る順序

1. **core**: 決まった形での書き出し・テキストの最小差分・編集コマンド(配置の計算を除く)・名前の採番・JSON 上の位置 → 範囲。テスト。
2. **extension**: カスタムエディタ・編集の適用・診断・新しいフォーム・コード生成のコマンド・JSON Schema。
3. **webview**: ストアと橋渡し、構造の木、プロパティ、パレット、キャンバス(描画と選択。移動・大きさの変更)。
4. **配置の計算**: クライアント領域の余白の記録、Align・Anchors の計算と照合、キャンバスでのドラッグへの組み込み。
5. **メニュー・非ビジュアルコンポーネント・イベント**の編集と表示。
6. **l10n**(日本語の訳)と、画面の細部(ズーム・格子・右クリックのメニュー)。

各段階で `pnpm check` を通し、段階ごとにコミットする。拡張の動作は、VS Code の拡張の開発用のホストで確かめる。
