# Bethany ドキュメント

FPC/LCL を薄い C++ ラッパー経由で使えるようにする、C++Builder ライクなライブラリの設計ドキュメント群。

## 構成

| ドキュメント | 内容 | 状態 |
|---|---|---|
| [vision.md](vision.md) | 目的・市場調査・想定ユーザー・スコープ | 初版 |
| [class-hierarchy.md](class-hierarchy.md) | LCL の継承関係(ソースで確認済み)・Bethany の階層・メンバの配置 | 初版 |
| [component-coverage.md](component-coverage.md) | VCL 移行を見据えたコントロールの棚卸し(未実装クラスの一覧と優先度) | 初版 |
| [member-coverage.md](member-coverage.md) | 実装済みのクラスに足りないメンバ・グローバルな関数の棚卸しと Tier 表(ModalResult・MessageDlg 等) | 初版 |
| [designer/](designer/README.md) | デザイナーアプリ(VS Code 拡張)の設計: DSL・コード生成・カタログ([ADR 0035](adr/0035-designer-in-this-repository.md)) | 草案 |
| [adr/](adr/) | 設計判断の記録（1判断1ファイル） | 随時追加 |
| [../todo.md](../todo.md) | 実装タスクの進捗・実装パターン・既知の課題(Phase 単位) | 継続更新 |

実装の細かい進捗・パターン集は引き続きリポジトリ直下の [todo.md](../todo.md) に置く。
`docs/` は目的(vision)と設計判断(ADR)のように、あとから覆りにくい・背景ごと残したい情報を置く場所とする。

## ADR 一覧

| No. | タイトル | 状態 |
|---|---|---|
| [0001](adr/0001-designer-app-bundling.md) | デザイナーアプリケーションを別途セットで配布する | 承認(一部置換→0035) |
| [0002](adr/0002-object-model-fidelity.md) | オブジェクトモデルは C++Builder に極力揃える | 承認 |
| [0003](adr/0003-two-phase-initialization.md) | コントロールの初期化を二段階(デフォルト構築 + 遅延 Create)に変更する | 置換(→0004) |
| [0004](adr/0004-pointer-members-for-deferred-declaration.md) | コントロールはポインタ(スマートポインタ)メンバとして持ち、既存の単一コンストラクタのまま生成する | 承認(一部置換→0008) |
| [0005](adr/0005-owner-as-pointer.md) | Owner/Parent 引数は参照ではなくポインタで受ける | 承認(一部置換→0007) |
| [0006](adr/0006-property-owner-typed-as-tobject.md) | Property<T> の所有者型を void* ではなく TObject* にする | 承認 |
| [0007](adr/0007-lcl-faithful-hierarchy.md) | クラス階層を LCL の継承関係に忠実に揃え、Owner と Parent を分離する | 承認 |
| [0008](adr/0008-wrapper-lifetime-follows-lcl.md) | C++ ラッパーの寿命を LCL オブジェクトに一致させ、ヒープ生成と Free() を強制する | 承認 |
| [0009](adr/0009-events-as-properties-with-sender.md) | イベントは Sender を受け取るプロパティとし、C API のコールバックに利用者データを渡す | 承認 |
| [0010](adr/0010-application-object.md) | Application をグローバルな TApplication* として公開し、CreateForm と終了時の破棄を C++Builder に揃える | 承認 |
| [0011](adr/0011-form-release-onclose-oncreate.md) | フォームに Release・OnClose・OnCreate(と OnShow)を追加し、OnCreate の発火時点を C++ 側で補う | 承認 |
| [0012](adr/0012-remaining-form-events.md) | フォームの残りのイベント(OnCloseQuery・OnHide・OnActivate・OnDeactivate・OnDestroy)を追加し、DLL の切り離し中はイベントを送らない | 承認 |
| [0013](adr/0013-string-return-bridge-reuse-ctor-exception.md) | 文字列はスレッドローカルバッファ経由で返し、イベントのブリッジは再利用し、コンストラクタが例外を投げたら LCL オブジェクトを破棄する | 承認 |
| [0014](adr/0014-control-key-mouse-events.md) | TControl / TWinControl のキー入力・マウス操作イベントを追加し、TForm もこれを継承する | 承認 |
| [0015](adr/0015-tier1-batch1-and-statusbar-issue.md) | Tier 1(19 クラス)をすべて追加し、TStatusBar の既知の問題を記録する | 承認(一部置換→0023) |
| [0016](adr/0016-control-align-and-splitter.md) | TControl.Align を追加し、それを前提とする TSplitter を追加する | 承認 |
| [0017](adr/0017-menus-and-wrapping-lcl-created-components.md) | メニュー(TMenuItem・TMainMenu・TPopupMenu)を追加し、LCL が内部で生成したコンポーネントを後からラップする仕組みを入れる | 承認(一部置換→0023) |
| [0018](adr/0018-pagecontrol-and-tabsheet.md) | Tier 2 の 1 バッチ目として TPageControl と TTabSheet を追加する | 承認(一部置換→0023) |
| [0019](adr/0019-treeview-and-non-component-items.md) | Tier 2 の 2 バッチ目として TTreeView を追加し、TComponent ではない項目(TTreeNode)の寿命を削除通知で管理する | 承認(一部置換→0020・0023・0026) |
| [0020](adr/0020-listview-and-shared-item-registry.md) | Tier 2 の 3 バッチ目として TListView を追加し、TComponent ではない項目の寿命管理を共通化する | 承認(一部置換→0026) |
| [0021](adr/0021-drawgrid-and-stringgrid.md) | Tier 2 の 4 バッチ目として TDrawGrid と TStringGrid を追加する | 承認(一部置換→0022) |
| [0022](adr/0022-indexed-property-proxy.md) | インデックス付きプロパティを添字で書ける共通のプロキシを入れ、グリッドの Cells・ColWidths・RowHeights に使う | 承認 |
| [0023](adr/0023-remaining-indexed-properties.md) | 残りのインデックス付きプロパティも添字で書けるようにし、読み取り専用のものは値を直接返す | 承認 |
| [0024](adr/0024-headercontrol.md) | Tier 2 の 5 バッチ目として THeaderControl を追加し、セクションの破棄を派生クラスのデストラクタで通知する | 承認(一部置換→0026) |
| [0025](adr/0025-toolbar-and-toolbutton.md) | Tier 2 の 6 バッチ目として TToolBar と TToolButton を追加する | 承認 |
| [0026](adr/0026-coolbar-and-item-free-observer.md) | Tier 2 の 7 バッチ目として TCoolBar を追加し、項目の破棄の通知を TPersistent の観察者にそろえる | 承認 |
| [0027](adr/0027-tstrings.md) | TStrings を表すクラスを入れ、Items・Lines・Tabs・SubItems を VCL と同じく TStrings* として公開する | 承認 |
| [0028](adr/0028-labelededit-and-stringlist.md) | TLabeledEdit と、利用者が生成する TStringList を追加する | 承認 |
| [0029](adr/0029-graphics-picture-image-glyph.md) | Tier 3 の 1 バッチ目として、グラフィックス基盤(TBitmap・TPicture)と TImage・Glyph を追加する | 承認 |
| [0030](adr/0030-imagelist-and-images.md) | Tier 3 の 2 バッチ目として、TImageList と各コントロールの Images・ImageIndex を追加する | 承認 |
| [0031](adr/0031-exceptions-across-dll.md) | DLL の境界で例外を受け渡し、C は直前のエラー、C++ は Exception として扱う | 承認(一部置換→0032) |
| [0032](adr/0032-internalize-c-api.md) | C API の提供を終了し、DLL の呼び出し層を内部層(beth::internal)にする | 承認 |
| [0033](adr/0033-dialogs.md) | Tier 4 としてダイアログを追加し、結果を適用する先の TControl.Color・Font を追加する | 承認 |
| [0034](adr/0034-designer-common-properties.md) | デザイナーで設定する共通のプロパティ(Anchors・BorderSpacing・Constraints・TabOrder 等)を追加し、集合型を Set<E> で表す | 承認 |
| [0035](adr/0035-designer-in-this-repository.md) | デザイナーアプリは本リポジトリ内に Bethany 専用として作り、tk-designer からは GUI に依存しない仕組みだけを流用する | 承認 |
| [0036](adr/0036-designer-canvas.md) | デザイナーのキャンバスは Windows の LCL の見た目を HTML で再現し、Align・Anchors の配置は core で計算して LCL の実測と照合する | 承認 |
| [0037](adr/0037-rename-to-bethany.md) | ライブラリの名前を no_vcl から Bethany(略称 beth)に変える | 承認 |
| [0038](adr/0038-cpp-distribution.md) | C++ はソースとビルド済みの beth.dll を zip で配り、C++ の部分は利用者のコンパイラでビルドする | 承認 |
| [0039](adr/0039-python-distribution.md) | Python のバインディングを beth パッケージにまとめ、beth.dll を同梱した Windows x64 の wheel を PyPI に出す | 承認 |
| [0040](adr/0040-system-include-path.md) | ヘッダを include/bethany/ に置き、利用者は #include <bethany/beth.hpp> と書く | 承認 |
| [0041](adr/0041-modal-result-and-message-dialogs.md) | ModalResult・メッセージのダイアログ・フォームの表示・既定のボタン(Tier A の 1 バッチ目) | 承認 |
| [0042](adr/0042-focus-edit-memo-label.md) | フォーカス・表示の更新・テキストの編集・TMemo・TLabel(Tier A の 2 バッチ目) | 承認 |

新しい ADR は [adr/template.md](adr/template.md) をコピーして作成する
([tk-designer](https://github.com/okano-tomoyuki/tk-designer) と同じ形式)。
