# 0036. デザイナーのキャンバスは Windows の LCL の見た目を HTML で再現し、Align・Anchors の配置は core で計算して LCL の実測と照合する

- 状態: 承認
- 日付: 2026-09-28

## 背景

[ADR 0035](0035-designer-in-this-repository.md) の順序の 4 番目(VS Code 拡張とデザイナーの画面)に着手する。
画面の中心になるキャンバス(フォームを編集する領域)の作り方を決める必要がある。

- DSL にはすべてのコントロールの位置と大きさ(Left・Top・Width・Height)を書く。Align で寄せたコントロールにも、デザイナーが
  計算した配置を書き込む(dsl-spec.md §5)。このため、デザイナーは LCL の Align・Anchors・BorderSpacing・Constraints の
  配置を自前で計算できなければならない。
- LCL のフォームの Width・Height はクライアント領域の大きさで、コンテナ(TPanel・TGroupBox・TTabSheet 等)の子の座標も、
  親のクライアント領域が基準になる。クライアント領域の余白(枠・見出し・タブ)は、クラスと widgetset とフォントで変わる。
- 見た目と大きさは widgetset(Windows は win32、Linux は GTK2)で違う。no_vcl の主な対象は Windows。
- tk-designer は Tk の pack・grid を TS に移植し、Windows の Tk で記録した値と照合している(tk-designer ADR 0008・0013)。
  この方式(アルゴリズムを移植し、実物で記録した値とテストで照合する)は、LCL にも当てはまる。

## 検討した選択肢

- 選択肢A: Webview の HTML/CSS でコントロールの見た目を描き、配置は core の TS で計算する。
  拡張だけで動き、ドラッグ中の表示も即座に更新できる。見た目は近似で、配置の計算は LCL と食い違う可能性がある
  (実測との照合で抑える)。
- 選択肢B: 実物の LCL でフォームを描かせ、画像を Webview に表示する(no_vcl の DLL を拡張から動かす)。
  見た目は正確だが、操作のたびに別のプロセスとの往復が要り、ドラッグ中の表示が遅れる。DLL のビルドと、拡張から
  ネイティブのプロセスを動かす仕組みが利用者の環境に要る。Linux・Windows の両方で同じように動かすのも難しい。
- 選択肢C: 見た目は A、配置は B(実物の LCL に配置だけを計算させる)。
  配置は正確になるが、B と同じく環境と往復の問題がある。

## 決定

**選択肢A。** 実物での確認は、生成したコードの実行(codegen-design.md §7)と、次の照合のテストで行う。

1. **再現する環境**: Windows の win32 widgetset(テーマ有効)、96dpi とする。GTK2 の見た目は再現しない
   (配置の計算は widgetset に依存しない部分を共通にし、クライアント領域の余白だけを環境ごとの値にする)。
2. **配置の計算は core に置く**(`core/src/layout/`)。LCL の `TWinControl.AlignControls` の Align(alTop・alBottom・alLeft・
   alRight・alClient)・BorderSpacing・Constraints と、親の大きさが変わったときの Anchors の追従を移植する。
   DOM に依存しない純粋な関数とし、拡張(書き込む配置の計算)と Webview(表示)で同じ関数を使う。
3. **クライアント領域の余白**は、Python のバインディングで実物のコントロールに alClient の子を置いて測り、
   記録した値をコミットする(`designer/tools/layout/`)。フォントに依存する余白(TGroupBox の見出し・TPageControl のタブ)は、
   記録したフォントでの値とし、ずれは許す。
4. **照合**: いくつかの配置の見本を実物の LCL で表示して配置を記録し(`layout:record`)、core の計算結果と一致することを
   テストで確かめる(tk-designer ADR 0008 と同じ方式)。記録は Windows で行う。
5. **見た目**はクラスごとの React の部品で、Windows 11 のテーマの見た目に近づける(完全な一致は求めない)。
   文字列の幅はブラウザで測る。AutoSize のコントロールの大きさは見積もりとする(実行時に LCL が決める。dsl-spec.md §5)。

## 影響

- キャンバスの見た目は近似になる。細部の確認は、生成したコードを実行して行う(将来、プレビューの実行を拡張のコマンドに
  することは妨げない)。
- LCL の配置の規則(同じ Align の兄弟の並べ方など)を移植する必要がある。LCL の版で規則が変わると、記録し直しと
  core の修正が要る。
- クライアント領域の余白と AutoSize の見積もりは、記録した環境(日本語版の Windows 11)のフォントに依存する。
  フォントの計測を OS に合わせる仕組み(tk-designer ADR 0013 の GDI での計測)は、ずれが問題になった時点で検討する。
- Linux(GTK2)の見た目・余白は対象外。Linux で開いても、Windows の見た目で表示する。
