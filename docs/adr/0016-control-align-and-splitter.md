# 0016. TControl.Align を追加し、それを前提とする TSplitter を追加する

- 状態: 承認
- 日付: 2026-09-27

## 背景

[docs/component-coverage.md](../component-coverage.md) の「cross-cutting な既知の課題」に挙げていたとおり、
Bethany にはレイアウトの基本である `TControl.Align` が無かった。ツールバーを alTop、メインの領域を alClient、
ステータス行を alBottom のように置く VCL アプリの典型的なレイアウトを再現できず、
Align を前提とする TSplitter も Tier 1 から見送っていた。

LCL では Align は `TControl` の **public** プロパティ(`controls.pp`)で、型は
`TAlign = (alNone, alTop, alBottom, alLeft, alRight, alClient, alCustom)`。既定値は TControl では alNone だが、
派生クラスが上書きしている(TStatusBar は alBottom、TCustomSplitter は `property Align default alLeft`)。

## 検討した選択肢

- 選択肢A: Align を TControl に置く(LCL と同じ)。
  全コントロール(TForm を含む)で使える。LCL の宣言と一致する。
- 選択肢B: Align を公開する具象クラスごとに置く。
  LCL では TControl の時点で public のため、[ADR 0007](0007-lcl-faithful-hierarchy.md) の「公開される階層に置く」
  規則に反する。

TSplitter のイベントのうち OnCanResize/OnCanOffset は `(Sender; var NewSize: Integer; var Accept: Boolean)` という
var 引数を 2 つ持つ独自の形で、既存のブリッジ(var 引数 1 つの TVarCallbackBridge)では表せない。

## 決定

選択肢A を採る。

- **TControl.Align**: C API は `beth_TControl_GetAlign` / `SetAlign`(TAlign の序数と、`beth_al*` 定数)。
  C++ は `enum TAlign { alNone, alTop, ... }` と `TControl::Align`(`Property<TAlign>`)。
  他の列挙型プロパティと同じく `Ord`/型キャストで整数として受け渡す。
- **TSplitter**(`extctrls.pp`): C++ は `TCustomControl → TCustomSplitter → TSplitter`。メンバはすべて
  TCustomSplitter の public なので、C API は `beth_TCustomSplitter_*`、C++ も TCustomSplitter に置く。
  - AutoSnap・Beveled・MinSize・ResizeAnchor(`TAnchorKind`。VCL には無い LCL のもの)・ResizeStyle(`TResizeStyle`)。
  - SplitterPosition は LCL ではプロパティではなく `GetSplitterPosition` / `SetSplitterPosition` メソッドの組のため、
    C++ でもメソッドのままにする。
  - OnMoved(TNotifyEvent)は既存の BridgeFor パターンで登録する。
  - OnCanResize/OnCanOffset は今回見送る(var 引数 2 つのブリッジが必要になった時点で追加する)。
  - ResizeControl(TControl を返す)も見送る。

## 実装して分かったこと

- **Align による配置は、LCL ではフォームが表示されるまで行われない。** `TControl.AutoSizeDelayed` が
  「表示されていないコントロールは配置を遅らせる」(`not IsControlVisible`)ため、フォームのコンストラクタの中や
  `Application->Run()` の前に Left/Top/Width/Height を読むと、Align を設定する前の値のまま。
  OnShow の時点では配置が済んでいる。VCL は表示前でも即座に配置するため、ここは VCL と挙動が異なる。
  LCL の標準の挙動であり、Bethany 側では手を入れない(テストも OnShow で配置後の値を確認している)。
- **同じ Align のコントロール同士は Left(alTop/alBottom なら Top)の小さい順に並ぶ。** TSplitter は既定で
  alLeft かつ Left=0 のため、そのまま Parent を設定すると、先に置いた alLeft のパネルより左に並んでしまう。
  VCL と同じ注意点で、Parent を設定する前に Left をパネルより右にしておく。
- **OnMoved はマウスでのドラッグが終わったときだけ呼ばれる**(`StopSplitterMove`)。
  `SetSplitterPosition` では呼ばれない。
- Win32 では、TPanel のクライアント領域は枠(BevelOuter)の 1px 分だけ内側になる。Linux/GTK2 では
  右と下がさらに 4px 狭い(ウィジェットセットの差)。Align の配置自体は両方で正しく動くことを確認した。
- TStatusBar は LCL の既定値が alBottom のため、Align を追加する前からフォームの下端に付いていた。
  今回 `Align` で既定値 alBottom を読み出せることを確認した。

## 影響

- ツールバー・サイドバー・ステータス行を持つ、実用的なレイアウトを Align で組めるようになった。
- TSplitter が Tier 1 に加わり、Tier 1 で見送っているのは TLabeledEdit(内部生成コンポーネントのラップ待ち)と、
  Tier 2 へ再分類した TPageControl+TTabSheet だけになった。
- Align と組み合わせて使うことの多い Anchors・AlignWithMargins(LCL では BorderSpacing)・Constraints・AutoSize は
  未実装。必要になった時点で同じ方式(TControl の public プロパティ)で追加する。
- 表示前に配置後の大きさを前提にした計算(フォームのコンストラクタで alClient のコントロールの Width を読む等)は
  LCL ではできない。OnShow か OnResize で行う必要がある。
