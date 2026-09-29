# 0046. Action(Tier B の B4)

- 状態: 承認
- 日付: 2026-09-29

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B4。C++Builder のアプリでは、メニュー項目・ツールボタン・ボタンが同じ操作
(保存・切り取り等)を持つとき、`TActionList` の `TAction` に操作(`OnExecute`)と状態(`Caption`・`Enabled`・`Checked`・`ShortCut`・`ImageIndex`)を
まとめ、各コントロールの `Action` に割り当てる。状態は `OnUpdate`(アイドルのときに呼ばれる)でまとめて切り替える。

LCL の作り(ActnList):

- `TBasicAction`(TComponent): `Execute()`・`Update()`・`OnExecute`・`OnUpdate`
- `TContainedAction`: `ActionList`(属する一覧)・`Category`・`Index`
- `TCustomAction` / `TAction`: `Caption`・`Checked`・`AutoCheck`・`GroupIndex`・`Enabled`・`Visible`・`Hint`・`ImageIndex`・`ShortCut`・
  `SecondaryShortCuts`・`DisableIfNoHandler`・`OnHint`
- `TCustomActionList` / `TActionList`(TComponent): `Actions[i]`・`ActionCount`・`Images`・`State`・`OnExecute`・`OnUpdate`・`OnChange`
- `TControl.Action`・`TMenuItem.Action`(TBasicAction)。割り当てると、Action の Caption 等がコントロールに写り、その後も連動する。
- 標準の Action(StdActns: `TEditCut`・`TEditCopy`・`TEditPaste`・`TEditSelectAll`・`TFileExit`・`TFileOpen` 等)

Action は TComponent で、フォームが所有する(フォームのメンバになる)。一覧に入れるのは `Action1->ActionList = ActionList1`。

## 決定

### ライブラリ

- `TBasicAction`・`TContainedAction`・`TCustomAction`・`TAction`・`TCustomActionList`・`TActionList` を、上のメンバで加える
  (`HelpContext` 等のヘルプ関係は対象外。[member-coverage.md](../member-coverage.md) §4)。
  - `TActionList` の `OnExecute`・`OnUpdate` は `TActionEvent(TBasicAction* Action, bool& Handled)`。
  - `SecondaryShortCuts`(TShortCutList)は今回は対象外にする(1 つの Action に複数のショートカットを割り当てるもの。使う場面が少ない)。
- `TControl::Action`・`TMenuItem::Action`(`TBasicAction*`)を加える。
- 標準の Action は、この ADR では扱わない(後の Tier C。下の「デザイナー」の `class` で入れられる形にしておく)。

### デザイナー: Action をどこに置くか

**案 A(推奨): ActionList の子のノード。** メニューの項目(`items`)と同じく、`components` の ActionList のノードに `actions` を持たせる。

```json
{
  "name": "ActionList1",
  "class": "TActionList",
  "properties": { "Images": "ImageList1" },
  "actions": [
    { "name": "FileSave", "properties": { "Caption": "&Save", "ShortCut": "Ctrl+S", "Category": "File" },
      "events": { "OnExecute": "FileSaveExecute", "OnUpdate": "FileSaveUpdate" } },
    { "name": "EditCut", "properties": { "Caption": "Cu&t", "ShortCut": "Ctrl+X" } }
  ]
}
```

- キャンバスには Action のアイコンを出さない(ActionList のアイコンだけ)。構造のツリーでは ActionList の下に Action が並ぶ。
- 右クリックのメニュー(ActionList・Action を選んだとき)に「Action を追加」を加える。並べ替え・削除・名前の変更は、メニュー項目と同じ操作で行う。
- Action を選ぶと、Object Inspector に Action のプロパティ・イベントが出る。
- ノードの `class` は省略でき、省略は `TAction`(メニュー項目の `TMenuItem` と同じ)。後で標準の Action(`"class": "TEditCopy"`)を入れる余地を残す。
- コード生成: Action はフォームのメンバとして生成し(`TAction* FileSave;`)、`FileSave->ActionList = ActionList1;` で一覧に入れる。

**案 B: 通常の非ビジュアルコンポーネント。** `TAction` をパレットに置き、`ActionList` プロパティ(参照)で一覧を選ぶ。
実装は最も少ない(今の参照のプロパティの仕組みでそのまま書ける)が、Action の数だけキャンバスにアイコンが並び、一覧の中の順も
ファイルの中の順に依存する。C++Builder(Action は ActionList のエディタの中で作り、フォームにアイコンを出さない)とも違う。

→ 案 A にする。メニュー項目の仕組み(ノードの種類・ツリー・追加・並べ替え・名前・コード生成)に、Action の種類を加える形で実装する。

### デザイナー: コントロールの `Action`

- ボタン・メニュー項目等の `Action` プロパティでは、同じフォームの Action(ActionList の子)を選ぶ。
- **コード生成の順**: `Action` への代入は、Action のプロパティを設定した後に行う(割り当てた時点で Action の Caption 等がコントロールに写るため)。
  Action のノードは ActionList のノードの中にあるので、コントロールの `Action` の代入は、ほかのコントロールへの参照と同じく最後にまとめる。
- **Action と重なるプロパティ**: `Action` を設定したコントロールで、`Caption`・`Enabled`・`Hint`・`ImageIndex`・`ShortCut`・`Checked`・`Visible` を
  フォームのファイルに書いたときは、Action の値で上書きされる(VCL の .dfm でも、Action から来る値は保存しない)。
  警告を出す(「Action の値で上書きされます」)。Object Inspector には Action の値を薄く表示する。
- キャンバスは、`Action` を設定したコントロールの Caption に Action の Caption を使う。

## 実装で決めたこと

- `TActionList` の `OnExecute`・`OnUpdate`(`TActionEvent`)は、VCL と違い先頭に Sender(ActionList)を置く
  (`(TObject* Sender, TBasicAction* Action, bool& Handled)`)。Bethany の C++・Python の生成とデザイナーのハンドラは、どのイベントも
  最初の引数を Sender とするため。
- LCL のコントロールは、Action を割り当てても自分の `OnClick` を上書きしない。クリックすると `OnClick` の後に Action の `OnExecute` が呼ばれる
  (VCL は `OnClick` が Action と違えば `OnClick` だけを呼ぶ)。デザイナーは `Action` と `OnClick` の両方を書いたときも警告する。
- Action が写すプロパティは LCL の ActionChange に合わせる: TControl は Caption・Enabled・Hint・Visible、TMenuItem は加えて AutoCheck・
  Checked・GroupIndex・ImageIndex・ShortCut、TCustomBitBtn・TCustomSpeedButton は Images・ImageIndex(TCustomSpeedButton は GroupIndex も)、
  TToolButton は Down(Action の Checked)・ImageIndex。
- `OnUpdate` がアイドルのときに呼ばれるのは、表示中のフォームのコントロールとメインメニューの(最上位の)項目に割り当てた Action だけ
  (LCL・VCL の TCustomForm.UpdateActions)。
- デザイナー: カタログのクラスの種類に `action` を加える(TBasicAction の派生。パレットに出さない)。フォームの `Action` と、
  Action の `ActionList`・`Index` はデザイン時に設定しない(ActionList はノードの位置で、Index は並びで決まる)。
  Object Inspector とキャンバスは、Action を割り当てたものに Action から写る値を表示する。

## 影響

- デザイナーのノードの種類が 1 つ増える(form・control・component・menuItem・**action**)。検証・名前の衝突・構造のツリー・選択・
  並べ替え・削除・改名(参照の書き換え)は、メニュー項目と同じ扱いにする。コピーと貼り付けは、デザイナーにまだ無い(加えるときは、
  Action を貼り付けると ActionList の中に入るようにする)。
- 標準の Action を後で加えるときは、ノードの `class` に書けるクラスを増やし、ライブラリにクラスを加えればよい。
