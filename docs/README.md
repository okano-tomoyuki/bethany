# no_vcl ドキュメント

FPC/LCL を薄い C++ ラッパー経由で使えるようにする、C++Builder ライクなライブラリの設計ドキュメント群。

## 構成

| ドキュメント | 内容 | 状態 |
|---|---|---|
| [vision.md](vision.md) | 目的・市場調査・想定ユーザー・スコープ | 初版 |
| [class-hierarchy.md](class-hierarchy.md) | LCL の継承関係(ソースで確認済み)・no_vcl の階層・メンバの配置 | 初版 |
| [component-coverage.md](component-coverage.md) | VCL 移行を見据えたコントロールの棚卸し(未実装クラスの一覧と優先度) | 初版 |
| [adr/](adr/) | 設計判断の記録（1判断1ファイル） | 随時追加 |
| [../todo.md](../todo.md) | 実装タスクの進捗・実装パターン・既知の課題(Phase 単位) | 継続更新 |

実装の細かい進捗・パターン集は引き続きリポジトリ直下の [todo.md](../todo.md) に置く。
`docs/` は目的(vision)と設計判断(ADR)のように、あとから覆りにくい・背景ごと残したい情報を置く場所とする。

## ADR 一覧

| No. | タイトル | 状態 |
|---|---|---|
| [0001](adr/0001-designer-app-bundling.md) | デザイナーアプリケーションを別途セットで配布する | 承認 |
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

新しい ADR は [adr/template.md](adr/template.md) をコピーして作成する
([tk-designer](https://github.com/okano-tomoyuki/tk-designer) と同じ形式)。
