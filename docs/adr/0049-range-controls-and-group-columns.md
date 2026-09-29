# 0049. 範囲のコントロールの細部とグループの列(Tier B の B10・B11)

- 状態: 承認
- 日付: 2026-09-30

## 背景

[member-coverage.md](../member-coverage.md) の Tier B の B10・B11。TTrackBar・TProgressBar・TScrollBar・TUpDown は Min・Max・Position だけ、
TRadioGroup・TCheckGroup は項目とチェックだけで、向き・目盛り・列の数などが設定できなかった。

## 決定

### ライブラリ

- 列挙型を加える(値の順は LCL と同じ): `TTrackBarOrientation`・`TTickMark`・`TTickStyle`・`TProgressBarOrientation`・`TProgressBarStyle`・
  `TUDOrientation`・`TUDAlignButton`・`TColumnLayout`・`TScrollCode`。
- **TTrackBar**: `Orientation`・`Frequency`・`TickMarks`・`TickStyle`・`LineSize`・`PageSize`・`SelStart`・`SelEnd`・`ShowSelRange`・`Reversed`。
- **TProgressBar**: `Orientation`・`Smooth`・`Step`・`Style`(`pbstMarquee`)・`BarShowText`、メソッド `StepIt()`・`StepBy(Delta)`。
- **TScrollBar**: `LargeChange`・`SmallChange`・`OnScroll`。
  - `using TScrollEvent = std::function<void(TObject* Sender, TScrollCode ScrollCode, int& ScrollPos)>;`(VCL と同じ形)。
  - ハンドラで `ScrollPos` を変えると、つまみの位置がその値になる。
- **TUpDown**: `Orientation`・`AlignButton`・`Wrap`・`ArrowKeys`・`Thousands`。
  - TUpDown の独自の `OnClick`(`TUDClickEvent`)・`OnChanging` は、今回も対象外にする(TControl の OnClick と型が違い、使う場面も少ない)。
- **TRadioGroup・TCheckGroup**: `Columns`・`ColumnLayout`・`AutoFill`。
  - TRadioGroup には `OnSelectionChanged`(TNotifyEvent)を加える。
  - TCheckGroup には `CheckEnabled[i]` と `OnItemClick` を加える(`using TCheckGroupClicked = std::function<void(TObject* Sender, int Index)>;`)。
- DLL のコールバックの型を 2 つ加える。
  - `scroll_callback_t(sender, scrollCode, int_t* scrollPos, data)`
  - `int_callback_t(sender, value, data)`

### デザイナー

- カタログを抽出し直す。上のプロパティ・イベントが Object Inspector に出る。
- キャンバスでは次を描く。
  - プログレスバー: `Orientation`(縦・右から・上から)と、`pbstMarquee`(棒の一部だけ)
  - トラックバー: 縦の向き
  - アップダウン: 横の向き
  - ラジオグループ・チェックグループ: 項目を `Columns` の列に `ColumnLayout` の順で並べる。`AutoFill` が false なら上から詰める。

## 実装で決めたこと

- LCL の TRadioGroup は、`ItemIndex` への代入でも `OnClick` と `OnSelectionChanged` を呼ぶ(この順)。
- TCheckGroup の `OnItemClick` と TScrollBar の `OnScroll` は、利用者の操作のときだけ呼ばれる(`Checked[i]`・`Position` への代入では呼ばれない)。
- 確認のプログラムでは、Windows のメッセージ(WM_HSCROLL・BM_CLICK)を送って、実際の操作と同じ経路で呼ばれることを確かめた。

## 影響

- C++ のイベントの引数に、列挙型と `int&` を持つもの(`TScrollEvent`)が加わる。Python では `ScrollPos` が `Ref`(`.value` を読み書きする)。
