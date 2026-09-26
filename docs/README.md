# no_vcl ドキュメント

FPC/LCL を薄い C++ ラッパー経由で使えるようにする、C++Builder ライクなライブラリの設計ドキュメント群。

## 構成

| ドキュメント | 内容 | 状態 |
|---|---|---|
| [vision.md](vision.md) | 目的・市場調査・想定ユーザー・スコープ | 初版 |
| [class-hierarchy.md](class-hierarchy.md) | LCL の継承関係(ソースで確認済み)・no_vcl の階層・メンバの配置 | 初版 |
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

新しい ADR は [adr/template.md](adr/template.md) をコピーして作成する
([tk-designer](https://github.com/okano-tomoyuki/tk-designer) と同じ形式)。
