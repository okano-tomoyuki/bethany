# 0024. Tier 2 の 5 バッチ目として THeaderControl を追加し、セクションの破棄を派生クラスのデストラクタで通知する

- 状態: 承認(一部置換→0026。セクションの破棄の通知は派生セクションのデストラクタではなく TPersistent の観察者から送る)
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の Tier 2、5 バッチ目として THeaderControl に着手した。
LCL のソース(`comctrls.pp`・`include/headercontrol.inc`)で確認したこと:

- 継承関係は `TCustomControl → TCustomHeaderControl → THeaderControl`。
  - セクションの描画も LCL が行う(OS のヘッダーコントロールは使わない)。
- セクションまわりのクラス:
  - `THeaderSection` は `TCollectionItem`、一覧の `THeaderSections` は `TCollection`。
  - `Items[Index]` が default のプロパティ。
- メンバの宣言場所:
  - Sections・DragReorder・OnSection* は、TCustomHeaderControl の published。
    THeaderControl は OnSectionClick/Resize/Track などを再度 published にしているだけ。
  - GetSectionAt(P: TPoint)・SectionFromOriginalIndex[] は、TCustomHeaderControl の public。
- セクションの生成:
  - `THeaderSections.Add/Insert` は、`TCustomHeaderControl.CreateSection`(**protected virtual**)でセクションを生成する。
  - 生成するクラスは、既定では `OnCreateSectionClass` で差し替えられる。
- イベントの引数:
  - OnSectionClick などの 1 つ目の引数は `Sender: TObject` ではなく、`HeaderControl: TCustomHeaderControl`。
  - OnSectionDrag だけは `Sender: TObject`。

## 検討した選択肢

セクションは TComponent ではないため、TListColumn と同じく C++ のラッパーの寿命を破棄の通知で管理する必要がある
([ADR 0020](0020-listview-and-shared-item-registry.md))。通知の出し方:

- 選択肢A: TListColumns と同じ形。
  - この DLL の `THeaderSections_Delete`・`Clear` で、削除の前に通知する。
  - ヘッダーコントロールの破棄の最初にも、全セクションを通知する。
- 選択肢B: `CreateSection` を上書きした内部の派生クラス `TBethHeaderControl` に、
  デストラクタで通知する派生セクション `TBethHeaderSection` を生成させる。

## 決定

選択肢B を採る。

- 通知の漏れ:
  - セクションの破棄は必ずデストラクタを通るので、Delete・Clear・ヘッダーコントロールの破棄のどの経路でも漏れなく通知される。
  - この DLL の関数を通らない経路(LCL 内部の Clear 等)でも漏れない。
- リストビューの列にこの形を使えなかったのは、TListColumns の生成を差し替える仮想関数が LCL に無いため。
- そのかわり `OnCreateSectionClass`(セクションのクラスの差し替え)は公開しない(差し替えると通知されなくなる)。

API:

- C API:
  - ヘッダーコントロールのメンバは、宣言しているクラスの名前で `beth_TCustomHeaderControl_*`。
  - 生成は `beth_THeaderControl_Create`。
  - 一覧は `beth_THeaderSections_*`、セクションは `beth_THeaderSection_*`。
- C++ の各クラス:
  - `THeaderSection`(TPersistent): Text・Width・MinWidth・MaxWidth・Alignment・Visible・Index と、読み取り専用の Left・Right・OriginalIndex。
  - `THeaderSections`: ヘッダーコントロールの値メンバとして持つ非所有のビュー。
    Count・`Items[i]`・Add・Insert・Delete・Clear・BeginUpdate・EndUpdate。
  - `TCustomHeaderControl`: Sections・DragReorder・`SectionFromOriginalIndex[i]`・GetSectionAt と 6 つのイベント。
  - `THeaderControl`: コンストラクタだけ。
- イベントの型は LCL の名前と引数に合わせた:
  - `TCustomSectionNotifyEvent(TCustomHeaderControl* HeaderControl, THeaderSection* Section)`
    を OnSectionClick・OnSectionResize・OnSectionSeparatorDblClick に使う。
  - `TCustomSectionTrackEvent(HeaderControl, Section, int Width, TSectionTrackState State)`
  - `TSectionDragEvent(TObject* Sender, FromSection, ToSection, bool& AllowDrag)`
  - OnSectionEndDrag は TNotifyEvent。
- ブリッジ(Pascal 側):
  - OnSectionClick・OnSectionResize・OnSectionSeparatorDblClick は、既存の項目用のブリッジに `DoSection` を追加して使う。
  - OnSectionTrack(幅・状態)と OnSectionDrag(2 つのセクションと var AllowDrag)には、専用のブリッジを追加した。
- C++ に VCL と同じ `struct TPoint { X, Y }` を追加した(GetSectionAt の引数)。
- 見送ったもの:
  - Images・ImagesWidth・ImageIndex(TImageList は Tier 3 待ち)
  - State(描画用の内部状態)
  - OnCreateSectionClass

## 実装して分かったこと

- Win32 で Pascal から `Point(...)` を呼ぶと、Windows ユニットの型名 `Point`(TPoint の別名)に隠される
  (ADR 0021 の `Rect` と同じ)。TPoint はフィールドで組み立てる。
- `THeaderSection.Width` は、Visible が False なら 0 を返す。Left・Right は、前にあるセクションの Width の合計から求める。
- `OriginalIndex` は Index を書き換えて並べ替えても変わらない。Insert・Delete では前後のセクションの値が詰められる。
- クリックされたセクションは、LCL がメッセージの座標ではなく `Mouse.CursorPos`(実際のカーソル位置)から求める。
  そのため、メッセージを送るテストでは SetCursorPos でカーソルも動かす必要がある。
- マウスのメッセージで確認したこと:
  - クリック → OnSectionClick。
  - 境界のドラッグ → OnSectionTrack(tsTrackBegin → tsTrackEnd)と OnSectionResize。
  - セクションのドラッグ → OnSectionDrag と OnSectionEndDrag。
    - OnSectionDrag はドラッグ中に何度も呼ばれ、同じセクションの上(FromSection = ToSection)でも呼ばれる。
    - AllowDrag を false にすると並べ替えられない。そのときは、マウスを離した位置のセクションで OnSectionClick が呼ばれる。
  - スクリーンショットでも並びが入れ替わったことを確認した。
- C のテストで、Delete したセクションはその場で、残りはフォームの破棄で、破棄の通知が届くことを確認した。
- Linux/GTK2(WSL)でも同じ結果になった。

## 影響

- 表の見出しを独自に描画する UI(ヘッダーコントロール + TPaintBox 等)を組めるようになった。
- 内部の派生クラスで生成を差し替えられる項目は、デストラクタで破棄を通知する形が最も漏れが無い。
  今後の TCollectionItem 系(TCoolBand 等)でも、まず生成を差し替える仮想関数があるかを確認する。
