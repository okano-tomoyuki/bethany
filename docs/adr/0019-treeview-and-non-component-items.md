# 0019. Tier 2 の 2 バッチ目として TTreeView を追加し、TComponent ではない項目(TTreeNode)の寿命を削除通知で管理する

- 状態: 承認
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の Tier 2、2 バッチ目として TTreeView に着手した。

これまでの C++ ラッパーの寿命管理([ADR 0008](0008-wrapper-lifetime-follows-lcl.md)・[ADR 0017](0017-menus-and-wrapping-lcl-created-components.md))は、
すべて TComponent の FreeNotification に頼っていた。ところが LCL の `TTreeNode`・`TTreeNodes` は **TPersistent** の派生で、
FreeNotification を持たない。ノードは利用者の操作(Delete)・ツリービューの破棄・Clear 等のさまざまな経路で破棄されるため、
ラッパーを持つなら、それらすべての破棄を知る手段が必要になる。

LCL のソース(`comctrls.pp`・`include/treeview.inc`)で確認したこと:

- `TTreeNode.Destroy` は、ツリービューに属していれば必ず `TCustomTreeView.Delete(Node)` を呼び(issue #17832 の対応)、
  それが `OnDeletion` を発生させる。ツリービュー自身の破棄(`TCustomTreeView.Destroy` → `FTreeNodes` の破棄)でも同じ経路を通る。
- `TCustomTreeView.Delete` は **protected virtual**。
- `Items`・`Selected`・`FullExpand`・`FullCollapse`・`AlphaSort`・`GetNodeAt` は TCustomTreeView の public。
  `ReadOnly`・`ShowLines`・`ShowRoot`・`ShowButtons`・`AutoExpand`・`HideSelection`・`RowSelect` と各イベントは
  TCustomTreeView の protected を TTreeView が published にしている。

## 検討した選択肢

ノードの C++ での表し方(利用者と相談して選択肢A に決定):

- 選択肢A: VCL と同じく `TTreeNode*` で扱う。ラッパーは初めて取得したときに作り、ノードの削除を通知で受けて delete する。
  同じノードには常に同じポインタが返るため、ポインタの比較・保持が VCL と同じようにできる。
- 選択肢B: TCanvas のように、取得するたびに作り直す非所有の軽いハンドルにする。
  寿命管理は不要だが、`TTreeNode*` の比較や保持が VCL と同じにならない。

削除の通知の受け方:

- 選択肢A-1: ツリービューの OnDeletion を内部で使い、利用者の OnDeletion を連鎖して呼ぶ。
  利用者の OnDeletion の登録・解除のたびに連鎖を管理する必要がある。
- 選択肢A-2: `TCustomTreeView.Delete`(protected virtual)を上書きした内部クラス `TNoVclTreeView = class(TTreeView)` を
  `TTreeView_Create` で生成し、`inherited`(利用者の OnDeletion)の後に削除を通知する。OnDeletion は利用者のものがそのまま使える。

## 決定

選択肢A と A-2 を採る。

- **Pascal 側**: `TTreeView_Create` は `TNoVclTreeView` を生成する(C/C++ からは TTreeView として扱い、公開するクラス階層は変わらない)。
  `TNoVclTreeView.Delete` は `inherited Delete`(OnDeletion)の後に、`TreeNodeFree_SetCallback` で登録したコールバックを呼ぶ
  (`FreeNotify_SetCallback` のノード版)。利用者の OnDeletion の中ではノードはまだ有効で、その後にラッパーが delete される。
- **C++ 側**: `TTreeNode`(`TPersistent` の派生)は独自のレジストリを持ち、`TTreeNode::Wrap(handle)` で初回の取得時にラッパーを作る。
  削除通知でレジストリから外して delete する。コンストラクタ・デストラクタは private(利用者は new/delete/Free しない)。
- **TTreeNodes**: ツリービューが所有する実体への非所有のビューで、TCanvas と同じく `TCustomTreeView` の値メンバとして持つ。
  VCL と同じ `TreeView1->Items->Add(nullptr, "Root")` と書けるよう、`ReadOnlyProperty<TTreeNodes*> Items` で公開する。
- **イベント**: OnChange/OnExpanded/OnCollapsed/OnDeletion(Sender, Node)と OnChanging/OnExpanding/OnCollapsing(Sender, Node, var Allow)の
  2 種類のブリッジ(`TNodeCallbackBridge`・`TNodeAllowCallbackBridge`)を追加した。C のコールバック型は
  `no_vcl_node_callback_t`・`no_vcl_node_allow_callback_t`。イベントの型(TTVChangedEvent・TTVExpandedEvent 等)は構造が同じ別名の型のため、
  Pascal 側は `MethodData` の多重定義ではなく `TMethod(...).Data` を渡す。
- インデックス付きプロパティ(`Items[Index]`)は、これまでと同じく `GetItem(Index)`。
- 見送ったもの: 画像(Images/ImageIndex/StateIndex。Tier 3 待ち)、複数選択(MultiSelect・Selections)、ラベルの編集(OnEditing/OnEdited)、
  オーナードロー、ソートの比較イベント(OnCompare)、ドラッグ&ドロップ。

## 実装して分かったこと

- **ツリービューの破棄に伴うノードの削除も、Delete の上書きで漏れなく受け取れる。** C 版テストで、Delete した 2 ノードに加え、
  フォームの破棄時に残りの 4 ノードの削除通知が届くことを確認した。
- **関数内 static のレジストリは、atexit で登録した終了処理より先に破棄されることがある。** TTreeNode のレジストリを関数内 static の値にしたところ、
  初回の構築(フォームの生成中)が `TApplication::Shutdown` の atexit 登録より後になるため、C++ の規則でレジストリが先に破棄され、
  Shutdown でフォームを破棄するときのノードの削除通知が、破棄済みのレジストリに触れていた(未定義動作。プロセスの終了が約 4 秒遅れ、
  後続のデストラクタの出力も欠けていた)。レジストリを意図的に破棄しない(new したままの)オブジェクトにして解消した。
  TComponent のレジストリは Application の生成時(atexit 登録より前)に構築されるため、この問題は無い。
  **今後、ラッパーのレジストリを追加するときは同じ形(破棄しないオブジェクト)にすること。**
- プログラムから `Selected` を設定すると、OnChanging はその場で呼ばれるが、OnChange はメッセージループが回ったとき(遅延)に呼ばれる(LCL の挙動)。
- LCL の TTreeView は OS のツリーコントロールではなく自前で描画する TCustomControl のため、実行中の確認は Win32 の
  ツリービュー用メッセージではなく、マウスのメッセージ(WM_LBUTTONDOWN/UP)で行った。クリックで OnChanging → OnChange が呼ばれることを確認した。
- 表示後は `GetNodeAt` で座標からノードを引ける(表示前はノードの位置が決まっていない)。
- Linux/GTK2(WSL)でも同じ結果になった。

## 影響

- ツリー構造の UI を VCL と同じ書き方(`TTreeNode*`・`Items->AddChild(...)`・`Node->Parent` 等)で組めるようになった。
- 「TComponent ではない項目を、削除の通知で寿命管理する」という 2 つ目の寿命管理の型ができた。TListView の TListItem
  (LCL でも TPersistent。`TCustomListView.Delete` 相当の削除通知があるかを確認してから)にも同じ形を適用できる見込み。
- ノードのポインタは、削除後(ツリービューの破棄後を含む)に使ってはならない。
