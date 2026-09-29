# Changelog

Bethany の C++ ライブラリ(`beth.hpp`・`beth.dll`)と Python のパッケージ(`bethany-lcl`)の変更の記録。形式は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/) に、
バージョンは [Semantic Versioning](https://semver.org/lang/ja/) に従う(0.x の間は、マイナーバージョンが上がると互換が無い変更を含みうる)。
デザイナー(VS Code 拡張)の変更は [designer/packages/extension/CHANGELOG.md](designer/packages/extension/CHANGELOG.md) に書く。

## [Unreleased]

### 追加

- `ModalResult`(`TCustomForm`・`TCustomButton`)と `TModalResult`・`mrOk` 等の定数。ボタンの `ModalResult` で、押したときにモーダルのフォームが閉じ、
  `ShowModal()` がその値を返す([ADR 0041](docs/adr/0041-modal-result-and-message-dialogs.md))
- メッセージのダイアログ `ShowMessage`・`MessageDlg`(`TMsgDlgType`・`TMsgDlgButtons`、`mbYesNo` 等)・`InputBox`・`InputQuery`・`PasswordBox` と、
  `Application->MessageBox`
- フォームの `BorderStyle`・`Position`・`WindowState`・`BorderIcons`・`FormStyle`・`KeyPreview`・`ActiveControl`、ボタンの `Default`・`Cancel`
- フォーカス(`SetFocus`・`CanFocus`・`Focused`・`OnEnter`・`OnExit`)、表示の更新と位置(`Invalidate`・`Repaint`・`Refresh`・`Update`・
  `BringToFront`・`SendToBack`・`SetBounds`・`ClientWidth`・`ClientHeight`)([ADR 0042](docs/adr/0042-focus-edit-memo-label.md))
- テキストの編集(`SelStart`・`SelLength`・`SelText`・`SelectAll`・`ClearSelection`・`Clear`・クリップボード・`Undo`・`CanUndo`・`Modified`・
  `PasswordChar`・`EchoMode`・`CharCase`・`Alignment`・`TextHint`・`NumbersOnly`・`AutoSelect`・`HideSelection`・`CaretPos`)。
  VCL と同じく、フォームを表示する前でも選択の置き換えやクリップボードが効く
- TMemo の `WordWrap`・`WantReturns`・`WantTabs`・`Append`、TLabel の `Alignment`・`Layout`・`WordWrap`・`Transparent`・`FocusControl`・`ShowAccelChar`
- リストボックスの複数選択(`MultiSelect`・`ExtendedSelect`・`Selected[i]`・`SelCount`・`SelectAll`・`ClearSelection`)・`Sorted`・`TopIndex`・`ItemAtPos`・
  `OnSelectionChange`、コンボボックスの `Style`(`csDropDownList` 等)・`DropDownCount`・`Sorted`・`ReadOnly`・`DroppedDown`・`AutoComplete`・
  `OnSelect`・`OnDropDown`・`OnCloseUp`([ADR 0043](docs/adr/0043-list-combo-check-application.md))
- チェックボックスの `State`(`cbGrayed`)・`AllowGrayed`・`OnChange`
- `Application` の `ExeName`・`ShowHint`・`HintPause`・`HintHidePause`・`Minimize`・`Restore`・`BringToFront`・`OnIdle`・`OnException`
  (ハンドラから送出された例外を、既定のエラーのダイアログの代わりに受ける)
- ステータスバーのパネル(`TStatusPanel`・`TStatusPanels`。`Panels->Add()` 等)と、ステータスバーの `SizeGrip`・`AutoHint`・`Canvas`・
  `OnDrawPanel`・`OnHint`・`GetPanelIndexAt`・`BeginUpdate`・`EndUpdate`、`Application->Hint`
  ([ADR 0044](docs/adr/0044-statusbar-panels-and-designer-collections.md))。パネルを表示するには `SimplePanel` を false にする(LCL の既定は true)
- フォーム・パネル等への描画: `TCustomControl` の `Canvas` と、TForm・TPanel・TScrollBox の `OnPaint`
  ([ADR 0045](docs/adr/0045-custom-control-canvas-and-drawing.md))
- Canvas の `TextWidth`・`TextHeight`・`TextRect`・`Polygon`・`Polyline`・`RoundRect`・`Arc`・`Pie`・`Chord`・`FrameRect`・`CopyRect`、
  `TPen` の `Style`・`Mode`、`TBrush` の `Style`、`TFont` の `Height`・`Orientation`・`Quality`
- Action: `TActionList`・`TAction`(`Caption`・`Enabled`・`Checked`・`ShortCut`・`ImageIndex`・`Category`・`OnExecute`・`OnUpdate`・`Execute()` 等)と、
  コントロール・メニュー項目の `Action`([ADR 0046](docs/adr/0046-actions.md))
- `Screen`(`TScreen`: `Cursor`・`Width`・`Height`・`Desktop…`・`WorkArea…`・`PixelsPerInch`・`MonitorCount`・`Forms[i]`・`FormCount`・`ActiveForm`・
  `ActiveControl`・`Fonts`・`OnActiveFormChange`・`OnActiveControlChange`)と `Clipboard()`(`TClipboard`: `AsText`・`HasFormat`・`HasPictureFormat`・
  `Clear`・`Open`・`Close`・`Formats[i]`・画像の `Assign`、`CF_Text()` 等)。クリップボードの画像は `Image1->Picture->Assign(Clipboard())` で読む
  ([ADR 0047](docs/adr/0047-screen-clipboard-icon-drop-files.md))
- アイコン(`TIcon`)と、フォーム・`Application`・`TPicture` の `Icon`、フォームのファイルのドロップ(`AllowDropFiles`・`OnDropFiles`)
- パネルの縁と Caption の揃え(`BevelOuter`・`BevelInner`・`BevelWidth`・`BevelColor`・`Alignment`・`VerticalAlignment`・`WordWrap`)、
  コントロールの枠(`BorderStyle`: `bsNone`・`bsSingle`)と `BorderWidth`、グリッド・TTreeView・TListView の `ScrollBars`(`TScrollStyle`)、
  TForm・TScrollBox の `AutoScroll`・`HorzScrollBar`・`VertScrollBar`(`TControlScrollBar`)([ADR 0048](docs/adr/0048-panel-bevel-scroll-border.md))
- TTrackBar の `Orientation`・`Frequency`・`TickMarks`・`TickStyle`・`LineSize`・`PageSize`・`SelStart`・`SelEnd`・`ShowSelRange`・`Reversed`、
  TProgressBar の `Orientation`・`Smooth`・`Step`・`Style`・`BarShowText`・`StepIt()`・`StepBy()`、TScrollBar の `LargeChange`・`SmallChange`・`OnScroll`、
  TUpDown の `Orientation`・`AlignButton`・`Wrap`・`ArrowKeys`・`Thousands`、TRadioGroup・TCheckGroup の `Columns`・`ColumnLayout`・`AutoFill`と、
  TRadioGroup の `OnSelectionChanged`、TCheckGroup の `CheckEnabled[i]`・`OnItemClick`([ADR 0049](docs/adr/0049-range-controls-and-group-columns.md))

### 変更

- グリッドの `Canvas` は `TCustomControl` のものになった(使い方は変わらない)
- `ShowModal()` の戻り値の型を `TModalResult`(中身は同じ int)にした
- TMemo の `ScrollBars` の型を `int` から `TScrollStyle` にした(`Memo1->ScrollBars = 3` は `ssBoth` に書き換える)

### 修正

- メインフォームを最小化すると、フォームが隠れたままタスクバーのボタンも消え、元に戻せなかった(`Application->Minimize()` も同じ)。
  DLL では LCL のアプリケーションのウィンドウが作られないため、メインフォームがタスクバーのボタンを持つ(`MainFormOnTaskBar`)ようにした
- FetchContent の取り込み先を変えた(`FETCHCONTENT_SOURCE_DIR_BETH` 等)後も、以前の取り込み先の `beth.dll` を exe の隣に写し続けていた。
  `BETH_DLL` の既定の値をキャッシュに保存しないようにし、以前の版が保存した値(別の Bethany のソースの DLL)は捨てる
- `beth.dll` が見つからない・`beth.dll` に関数が無い(ヘッダより古い)ときに、assert(Release のビルドではヌルポインタの呼び出し)で落ちていた。
  理由を標準エラー出力とメッセージボックスで知らせて終了する

## [0.2.0] - 2026-09-29

Python のパッケージに変更は無い(バージョンは C++ のライブラリと合わせる)。

### 変更

- **互換が無い変更**: ヘッダを `include/bethany/` に、ソースを `src/` に移した。`#include "beth.hpp"` は `#include <bethany/beth.hpp>` に書き換える
  (システムインクルード。`find_package` で使うと、ライブラリのヘッダは `-isystem` で渡る。[ADR 0040](docs/adr/0040-system-include-path.md))。
  インストール先のヘッダも `include/beth/` から `include/bethany/` になった

## [0.1.1] - 2026-09-28

C++ の部分に変更は無い。

### 追加

- Python のパッケージ `bethany-lcl`(PyPI。import の名前は `beth`)。`beth.dll` を同梱した Windows x64 の wheel([ADR 0039](docs/adr/0039-python-distribution.md))

### 変更

- Python のバインディングを `beth` パッケージにまとめた。内部のモジュールは `beth_core`・`beth_internal` から `beth._core`・`beth._internal` になった

## [0.1.0] - 2026-09-28

最初の公開版。Windows x64(MinGW-w64 の g++)を対象とする。

### 追加

- LCL のコントロール・コンポーネントを C++Builder に似た API で使う C++ のクラス(対象の一覧は [docs/component-coverage.md](docs/component-coverage.md))
- CMake のパッケージ: `FetchContent` での取り込みと、`cmake --install` した後の `find_package(beth)` に対応し、`beth::beth` のターゲットと、
  `beth.dll` を exe の隣へ写す `beth_deploy()` を提供する
- MinGW のランタイム(libgcc・libstdc++・winpthread)を exe に静的にリンクする(`BETH_STATIC_RUNTIME`、既定で ON)

[Unreleased]: https://github.com/okano-tomoyuki/bethany/compare/v0.1.1...HEAD
[0.1.1]: https://github.com/okano-tomoyuki/bethany/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/okano-tomoyuki/bethany/releases/tag/v0.1.0
