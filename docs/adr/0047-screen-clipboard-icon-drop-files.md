# 0047. Screen・Clipboard・アイコン・ファイルのドロップ(Tier B の B2・B3・B15・B16)

- 状態: 承認
- 日付: 2026-09-30

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B2・B3・B15・B16。どれもアプリケーション・フォームの全体に関わる小さな機能で、
C++Builder のアプリでよく使う。

- **Screen**(LCL の Forms の `TScreen`): 画面の大きさ・作業領域・カーソル・開いているフォームの一覧・アクティブなフォームとコントロール。
  C++Builder と同じく、グローバル変数 `Screen` の 1 つだけ。
- **Clipboard**(LCL の Clipbrd の `TClipboard`): プログラムからクリップボードの文字列・画像を読み書きする。C++Builder では関数 `Clipboard()` で得る。
- **アイコン**(LCL の Graphics の `TIcon`): フォームのタイトルバー・タスクバーのアイコン(`Form1->Icon`)と、アプリケーションのアイコン
  (`Application->Icon`)。
- **ファイルのドロップ**(LCL の `TCustomForm.AllowDropFiles`・`OnDropFiles`): エクスプローラーからフォームにファイルをドロップしたとき、
  ファイル名の配列を受ける。VCL には無い(VCL は WM_DROPFILES を自分で処理する)、LCL のもの。

## 決定

### B2: Screen

- `TScreen`(TComponent)を加え、`extern TScreen* Screen;` で公開する(`Application` と同じく DLL の読み込み時に作られている LCL の Screen を包む)。
  Python は `beth.Screen`。
- メンバ:
  - `Cursor`(`TCursor`。crHourGlass 等にすると、すべてのコントロールの上でそのカーソルになる。crDefault で戻す)
  - `Width`・`Height`(主モニタ)、`DesktopLeft`・`DesktopTop`・`DesktopWidth`・`DesktopHeight`(すべてのモニタを合わせた範囲)、
    `WorkAreaLeft`・`WorkAreaTop`・`WorkAreaWidth`・`WorkAreaHeight`・`WorkAreaRect`(タスクバーを除いた範囲)、`PixelsPerInch`、`MonitorCount`
  - `FormCount`・`Forms[i]`(TForm の派生だけ。LCL と同じ)、`ActiveForm`・`ActiveControl`
  - `Fonts`(インストールされているフォントの名前。読み取り専用の TStrings)
  - `OnActiveFormChange`・`OnActiveControlChange`(TNotifyEvent。Sender は Screen)
- `Forms[i]`・`ActiveForm`・`ActiveControl` は、プログラムが作ったもの(ラッパーのあるもの)だけを返す。LCL が内部で作るフォーム
  (MessageDlg・InputBox のダイアログ等)は nullptr(Python は None)になる。
- `Monitors`(TMonitor)・`HintFont` 等のフォント・`Cursors`(カーソルの登録)は対象外(Tier C)。

### B3: Clipboard

- `TClipboard`(TPersistent)と、関数 `TClipboard* Clipboard();` を加える(C++Builder と同じく関数。Python も `Clipboard()`)。
  ラッパーは 1 つだけで、利用者は破棄しない。
- メンバ: `AsText`(読み書き)・`HasFormat(Format)`・`HasPictureFormat()`・`Clear()`・`Open()`・`Close()`・`FormatCount`・`Formats[i]`、
  画像を写す `Assign(const TPicture*)`・`Assign(const TGraphic*)`。
- 画像を読むのは C++Builder と同じく `Image1->Picture->Assign(Clipboard())`(`TPicture::Assign`・`TGraphic::Assign` に `const TClipboard*` の
  多重定義を加える)。クリップボードに画像が無ければ何もしない(LCL)。
- **形式の値**: LCL の形式の値は実行時に決まる(Windows では文字列が CF_UNICODETEXT)。`TClipboardFormat`(整数)と、LCL と同じ名前の関数
  `CF_Text()`・`CF_Bitmap()`・`CF_Picture()` を加える(`Clipboard()->HasFormat(CF_Text())`)。`<windows.h>` の `CF_TEXT` 等も
  (Windows が変換して持つ形式として)渡せるが、`CF_Picture()` は LCL だけのもの(読み込める画像の形式のどれか)。
- `AsHtml`・`SetComponent` 等・`OnRequest`・`PrimarySelection` 等(Windows 以外のもの)は対象外。

### B15: アイコン

- `TIcon`(LCL の TCustomIcon・TIcon。TRasterImage の派生)を加える。`new TIcon` で生成して delete で破棄するものと、所有者の中身のビューの
  2 通りがある(TBitmap と同じ。[ADR 0029](0029-graphics-picture-image-glyph.md))。`LoadFromFile` で .ico を読む。
- `TCustomForm::Icon`・`TApplication::Icon`・`TPicture::Icon`(`Property<TIcon*>`。ビュー。代入は内容のコピー)を加える。
- フォームの `Icon` が空なら、LCL は `Application->Icon` を使う。
- デザイナーは、ほかの画像のプロパティ(Picture・Glyph)と同じく `Icon` を扱わない(画像のファイルをフォームのファイルに持つ仕組みが無い)。
  コードで設定する。

### B16: ファイルのドロップ

- `TCustomForm` に `AllowDropFiles`(bool)・`OnDropFiles` を加える(TForm で公開)。
- `using TDropFilesEvent = std::function<void(TObject* Sender, const std::vector<std::string>& FileNames)>;`
  (ファイル名は UTF-8 のフルパス)。Python のハンドラは `(Sender, FileNames)` で、FileNames は str の list。
- DLL のコールバックは、ファイル名の数と、UTF-8 の文字列の配列で渡す(`void (obj_t sender, int_t count, str_t* fileNames, void* data)`)。
- デザイナー: フォームのプロパティに `AllowDropFiles`、イベントに `OnDropFiles` が出る(カタログを抽出し直すだけ)。
- `Application->OnDropFiles`(どのフォームにドロップしても呼ばれる)は対象外(フォームの OnDropFiles で足りる)。

## 実装で決めたこと

- `TClipboardFormat` は `std::uint32_t`(Windows の形式の番号は UINT)。DLL は Cardinal で受け渡す。`CF_Text()` は Windows では
  `CF_UNICODETEXT`(13)で、`<windows.h>` の `CF_TEXT`(1)を `HasFormat` に渡しても、Windows が変換して持つ形式として true になる。
- `TIcon` は LCL の TCustomIcon を挟まず、TRasterImage の直下に置く(TCustomIcon だけの使い道が無いため)。`Assign` で TBitmap から作れる。
- LCL は、ファイル・クリップボードから読み込んだビットマップの大きさを `SetSize` だけで変えても、描き換えるまでは読み込んだときのデータを
  保存する(`Clipboard()->Assign` に渡すと、元の大きさの画像が置かれる)。LCL の動作なので、ライブラリでは手を入れない。
- Python の `Clipboard()` は、最初に呼んだときに作ったラッパーを返し続ける(C++ の関数内 static と同じ)。`Screen` は `Application` と同じく
  パッケージの読み込み時に作る。

## 影響

- グローバルなオブジェクトが `Application` のほかに `Screen` と `Clipboard()` の 2 つになる。どちらも LCL が持つもので、ラッパーは破棄しない。
- イベントの引数に文字列の配列が初めて加わる(Python の生成器に変換を足す)。
