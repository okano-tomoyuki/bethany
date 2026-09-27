# 0018. Tier 2 の 1 バッチ目として TPageControl と TTabSheet を追加する

- 状態: 承認
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の Tier 2 に着手した。Tier 2 は 1 クラスごとに専用のコレクション・
所有子コンポーネントの設計が必要なため、Tier 1 と同じくバッチに分けて進める。1 バッチ目は、Tier 1 から
「所有ページの生成・破棄の設計が要る」として再分類していた TPageControl + TTabSheet にした。
[ADR 0017](0017-menus-and-wrapping-lcl-created-components.md) で LCL が内部で生成したコンポーネントをラップする仕組み
(`WrapExisting`)が入ったため、所有ページの問題はこれで扱える見込みが立ったことによる。

LCL のソース(`comctrls.pp`・`include/pagecontrol.inc`・`include/customnotebook.inc`)で確認したこと:

- 継承関係は `TWinControl → TCustomTabControl → TPageControl`(実装済みの TTabControl と同じ基底)、
  `TWinControl → TCustomPage → TTabSheet`。
- ページの追加は VCL と同じく 2 通り。`TTabSheet.Create(Owner)` して `PageControl` を設定するか、
  `TPageControl.AddTabSheet` を呼ぶ。後者のページは LCL が `GetPageClass.Create(Self)` で生成する(Owner はページコントロール)。
- `TPageControl.Clear`(`Tabs.Clear` → `TNBPages.Delete`)は、ページを外したあと `Application.ReleaseComponent` で
  **遅延破棄** する。
- `TabIndex`/`OnChange` は TCustomTabControl の protected を TPageControl が published にしている。
  `PageCount`・`MultiLine`・`ShowTabs`・`TabPosition`・`OnChanging` は TCustomTabControl の public(TTabControl でも使える)。
- `OnChanging` の型 `TTabChangingEvent = procedure(Sender: TObject; var AllowChange: Boolean)` は、OnCloseQuery と同じ形。
- `SetPageIndex`(ActivePageIndex/ActivePage の変更)は、既定の Options では OnChange を呼ばない。

## 検討した選択肢

AddTabSheet・Pages[i]・ActivePage が返すページの扱い:

- 選択肢A: AddTabSheet を提供せず、ページは必ず利用者が TTabSheet を生成する形に限る。
  `WrapExisting` が不要になるが、VCL の `AddTabSheet` を使ったコードをそのまま移植できない。
- 選択肢B: ページを返す関数は Watch してから返し、C++ は `WrapExisting<TTabSheet>` で受ける(ADR 0017 と同じ形)。

## 決定

選択肢B を採る。

- C API: `no_vcl_TPageControl_*`(ActivePage・ActivePageIndex・GetPage・AddTabSheet・Clear・SelectNextPage・TabIndex・OnChange)、
  `no_vcl_TCustomTabControl_*`(PageCount・MultiLine・ShowTabs・TabPosition・OnChanging)、
  `no_vcl_TTabSheet_*`(PageControl・TabIndex)、`no_vcl_TCustomPage_*`(PageIndex・TabVisible・OnShow・OnHide)。
  ページを返す関数(GetActivePage・GetPage・AddTabSheet)は Watch してから返す。
- C++: 既存の空だった `TCustomTabControl` に public メンバを追加し、`TPageControl`・`TCustomPage`・`TTabSheet` を追加した。
  `Pages[Index]` は `GetPage(Index)`(TMenuItem::GetItem と同じ、インデックス付きプロパティを Get メソッドで表す形)。
  TTabSheet のハンドルを受け取るコンストラクタは private にし、`friend class TComponent` で WrapExisting からだけ呼べるようにした。
- OnChanging は、OnCloseQuery と同じブリッジ(`TVarCallbackBridge.DoCloseQuery`)と C のコールバック型
  (`no_vcl_close_query_callback_t`)をそのまま使う。C++ は `TTabChangingEvent = std::function<void(TObject*, bool& AllowChange)>`。
- 見送ったもの: Images/ImageIndex(Tier 3 待ち)、Style・HotTrack・TabHeight/TabWidth・Options、OnCloseTabClicked、ドッキング関連。

## 実装して分かったこと

- **Clear はページを遅延破棄する。** Clear の直後はページがまだ残っており(PageCount は 0)、次にメッセージを処理したとき
  か Owner の破棄時に破棄されて、そのときに破棄通知が届く(C 版テストで、Clear 直後の破棄数が 0、Owner の破棄後に 2 増えることを確認)。
- **`PageIndex` でページを並べ替えると OnChange が呼ばれる**(表示中のページの位置が変わるため)。
  ActivePage・ActivePageIndex をプログラムから変更した場合は呼ばれない。
- 実行中のタブコントロール(`SysTabControl32`)に `TCM_SETCURFOCUS` を送ってページを切り替えると、
  OnChanging → 切り替わったページの OnShow → OnChange の順に呼ばれることを確認した。TabVisible を false にしたページのタブは表示されない。
- Run() の前にページコントロールとページを生成・配置して問題は無かった。Linux/GTK2(WSL)でも同じ結果になった。

## 影響

- 設定画面のようなページ切り替え UI を VCL と同じ書き方で組めるようになった。
- TTabControl にも PageCount・MultiLine・ShowTabs・TabPosition・OnChanging が使えるようになった(LCL と同じ)。
- Tier 2 の残り(TTreeView・TListView・TStringGrid/TDrawGrid・THeaderControl・TToolBar/TToolButton・TCoolBar)は、
  次のバッチ以降で進める。TTreeView の TTreeNode・TListView の TListItem は TComponent ではない(TPersistent)ため、
  破棄通知に頼れず、`WrapExisting` とは別の寿命管理を設計する必要がある。
