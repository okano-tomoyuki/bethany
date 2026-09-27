# 0026. Tier 2 の 7 バッチ目として TCoolBar を追加し、項目の破棄の通知を TPersistent の観察者にそろえる

- 状態: 承認
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の Tier 2、最後の 7 バッチ目として TCoolBar に着手した。
LCL のソース(`comctrls.pp`・`include/coolbar.inc`)で確認したこと:

- 継承関係:
  - クールバーは `TCustomControl → TToolWindow → TCustomCoolBar → TCoolBar`。
  - バンドは `TCollectionItem → TCoolBand`、一覧は `TCollection → TCoolBands`(`Items[Index]` が default)。
- バンドの生成と削除は、LCL の内部で起きる:
  - ウィンドウを持つコントロールの Parent をクールバーにすると、`TCustomCoolBar.InsertControl` がそのコントロールのバンドを自動で追加する。
  - コントロールを外すと(Parent の変更・破棄)、`RemoveControl` がそのバンドを削除する。
  - いずれも、この DLL の関数を通らない。
- 生成するクラスを差し替える仮想関数が無い:
  - THeaderControl の `CreateSection`([ADR 0024](0024-headercontrol.md))に当たるものが無い。
  - TCoolBands の生成を差し替える方法も無い。
- Align の Setter:
  - `TCustomCoolBar` は Align の Setter を **reintroduce で差し替え**(非仮想)、alLeft/alRight なら Vertical も切り替える。
  - TControl の Setter を経由すると、Vertical が切り替わらない。

そのため、バンドの破棄を知る別の方法が要った。FPC の RTL(`persist.inc`・`collect.inc`)で確認したこと:

- `TPersistent.Destroy`:
  - 付いている観察者(`FPOAttachObserver`)に **`ooFree` を必ず送る**。
  - 送るのは破棄の最後で、派生クラスのデストラクタの処理がすべて終わった後。
  - コレクションの Clear・破棄の途中でも送られる。
- `TCollection.Notify` の `ooDeleteItem`:
  - 更新中(Clear・破棄・BeginUpdate の間)は送られないので、破棄の通知には使えない。

またこの調査の途中で、**TreeView のバッチ([ADR 0019](0019-treeview-and-non-component-items.md))からある不具合**が見つかった。

- これまでの方式:
  - ノードの破棄の通知を、`TCustomTreeView.Delete`(TTreeNode.Destroy の最初に呼ばれる)を上書きして送っていた。
- 起きること:
  1. LCL は `Delete` の後に、同じノードの破棄の処理(`HasChildren := False` による折りたたみ等)を続ける。
  2. そこで呼ばれたイベント(OnCollapsing 等)で、C++ 側がノードを**再びラップ**する。
  3. その登録はもう削除されない。
  4. 同じアドレスに別の項目(ここではクールバーのバンド)が作られると、`ItemRegistry::Wrap` が**違う型のラッパー**(TTreeNode)を返し、そのメンバを呼んで落ちる。
- 同じ危険のある箇所:
  - ListView の項目(`DoDeletion` の上書き)
  - 列(削除の前の通知)
  - ヘッダーのセクション(派生セクションのデストラクタ。基底のデストラクタの処理より前)

## 検討した選択肢

- 選択肢A: 観察者の方式にそろえる。
  - 項目のハンドルを C 側へ渡すとき(関数の戻り値・イベントの引数)に観察者を付け、`ooFree` で破棄を通知する。
  - TCoolBand だけでなく、TTreeNode・TListItem・TListColumn・THeaderSection もこの方式にする。
- 選択肢B: ツリーだけを直す。
  - 削除中のノード(LCL の Deleting が True)ではイベントを C 側へ送らない。
  - 変更は小さいが、次の問題が残る:
    - 子の OnDeletion から削除中の親をたどる場合などは防げない。
    - ほかの項目の型には手を入れない。
    - バンドには別の方式が要る。

## 決定

選択肢A を採る(ユーザーの判断)。

### 項目の破棄の通知

- Pascal 側に `WatchItem(Item: TPersistent)` を追加した。
  - コンポーネントの `Watch`(FreeNotification の登録)と同じ考え方で、C 側へハンドルを渡すすべての箇所で呼ぶ。
  - 観察者は 1 つ(`TItemFreeObserver`)。参照カウントをしない TComponent の IInterface の実装を使う。
  - 観察者を付けた項目は `TAVLTree`(ポインタの比較)で覚え、同じ項目に二重に付けない。
  - `ooFree` を受け取ると、木から外して `NotifyItemFreed` を呼ぶ。
- ハンドルを渡す箇所は、関数の戻り値とイベントのブリッジの引数で、ツリー・リスト・ヘッダー・クールバーのすべて。
- 不要になったものを取り除いた:
  - `TNoVclTreeView`(Delete の上書き)
  - `TNoVclListView`(DoDeletion とデストラクタの上書き)
  - `TListColumns_Delete`・`Clear` の削除前の通知
  - `TNoVclHeaderSection`・`TNoVclHeaderControl`(CreateSection の上書き)
  - TTreeView・TListView・THeaderControl は、LCL のクラスをそのまま生成する。
- 通知の対象と時点:
  - 対象は、一度でも C 側へ返された項目だけ。返されたことの無い項目には C 側のラッパーが無いので、通知も要らない。
  - 通知は、削除の処理とイベントがすべて終わった後に届く。途中のイベントで再びラップされても、最後の通知で必ず消える。
- C API・C++ API の形は変えていない(`no_vcl_ItemFree_SetCallback` と `ItemRegistry` はそのまま)。

### TCoolBar

- C API:
  - 宣言しているクラスの名前で、`no_vcl_TCustomCoolBar_*`・`no_vcl_TCoolBands_*`・`no_vcl_TCoolBand_*`。
  - 生成は `no_vcl_TCoolBar_Create`。
  - EdgeBorders 等は `no_vcl_TToolWindow_*`([ADR 0025](0025-toolbar-and-toolbutton.md))。
- C++ の各クラス:
  - `TCoolBand`(TPersistent): Text・Width・MinWidth・MinHeight・Break・Visible・FixedSize・FixedBackground・HorizontalOnly・
    Color・ParentColor・Index・Control と、読み取り専用の Left・Top・Right・Height、AutosizeWidth。
  - `TCoolBands`: クールバーの値メンバとして持つ非所有のビュー。Count・`Items[i]`・Add・Delete・Clear・BeginUpdate・EndUpdate・
    FindBand・FindBandIndex。
  - `TCustomCoolBar`(TToolWindow の派生): Bands・FixedSize・FixedOrder・GrabStyle・GrabWidth・HorizontalSpacing・VerticalSpacing・
    ShowText・Themed・Vertical・OnChange・AutosizeBands・MouseToBandPos。
  - `TCoolBar`: コンストラクタだけ。
- Align:
  - `no_vcl_TControl_SetAlign` は、対象が TCustomCoolBar なら、その Setter を呼ぶようにした。
  - C++ の `Align` も同じ関数を通るので、alLeft/alRight で Vertical が true になる。
- 見送ったもの:
  - Bitmap・ParentBitmap・Images・ImagesWidth・ImageIndex(TBitmap/TImageList は Tier 3 待ち)
  - BandBorderStyle・BorderStyle(TBorderStyle をまだ C++ に定義していない)
  - BandMaximize(LCL でも Delphi でも使われていない)

## 実装して分かったこと

- バンドの配置:
  - `Break` の既定は True なので、何もしなければバンドは 1 行に 1 つずつ並ぶ。
  - 同じ行に並べるには、後ろのバンドの Break を False にする。
- OnChange は、ドラッグでバンドを動かす・幅を変えて、マウスを離したときに呼ばれる。
  マウスのメッセージで、2 行目のバンドのつまみを 1 行目のバンドの上へドラッグすると、並びが入れ替わって OnChange が呼ばれた。
  スクリーンショットでも確認した。
- バンドの削除:
  - コントロールを破棄すると、そのバンドは LCL が内部で削除する。
  - C のテストで、この API を通らないこの削除でも、一度返したバンドには破棄の通知が届くことを確認した。
- 以前のバッチの確認:
  - 観察者の方式にそろえた後も、TreeView・ListView・THeaderControl のテストの結果は変わらなかった(破棄の通知の数も同じ)。
  - THeaderControl・TToolBar のマウスでの確認も、以前と同じ結果になった。
- Linux/GTK2(WSL)でも同じ結果になった。

## 影響

- Tier 2 のコントロールはすべて揃った。
- 置き換えた方式:
  - [ADR 0019](0019-treeview-and-non-component-items.md)・[0020](0020-listview-and-shared-item-registry.md)・[0024](0024-headercontrol.md)
    の破棄の通知の方式は、この ADR で置き換えた。
  - これらの内部の派生クラスも不要になった。
- 今後、TComponent ではない項目(TPersistent)を追加するとき:
  - 項目のハンドルを C 側へ渡す箇所で `WatchItem` を呼べばよい。
  - 生成・破棄を差し替える仮想関数を探す必要は無い。
- `OnCreateSectionClass`(THeaderControl)は、セクションのクラスを差し替えても通知されるようになったため、公開の妨げは無くなった(まだ公開していない)。
