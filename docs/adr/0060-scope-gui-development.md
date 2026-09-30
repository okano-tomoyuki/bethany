# 0060. 対象を GUI の開発に必要なものに限り、DB・印刷・設定ファイル等の非ビジュアルのライブラリは扱わない

- 状態: 承認
- 日付: 2026-09-30

## 背景

1.0 に向けて、ライブラリの範囲を決めておく必要がある。1.0 の後に範囲を広げること自体は互換を壊さないが、何を扱い何を扱わないかの
基準が無いと、要望のたびに判断がぶれ、保守の負担が際限なく増える。

C++Builder(VCL・RTL)は、GUI の部品のほかに多くの非ビジュアルのライブラリを持つ。

- データベース: TDataSet・TField・TDataSource・データ対応コントロール(TDBGrid 等)と、DB ごとのドライバ
- 印刷(TPrinter・TPrintDialog)
- 設定ファイル・レジストリ(TIniFile・TRegistry)
- ネットワーク(Indy 等)、JSON、正規表現、ファイル操作、汎用のコンテナ、スレッド(TThread)

Delphi の名前は「Oracle(神託)に伺いを立てるなら Delphi(神殿)へ」に由来し、発売当時の DB 開発(Oracle への接続)の強みを表す。
ただし、これは製品の出自の話である。Bethany が目指すのは、C++Builder と同じ書き方で GUI を作れること(オブジェクトモデルの忠実さ。
[ADR 0002](0002-object-model-fidelity.md))であり、RTL・DB の全体の再現ではない。

また、C++ と Python には、上の多くについて成熟した標準・定番のライブラリがある(SQLite・ODBC・Python の DB-API や
configparser 等)。

## 検討した選択肢

- 選択肢A: C++Builder の非ビジュアルのライブラリも広く扱う。移行は楽になるが、規模が GUI の部品の全体に匹敵するかそれ以上になる。
  DB はドライバ(Oracle の OCI 等)の依存も抱える。C++・Python の既存のライブラリと役割が重なる。
- 選択肢B: GUI の開発に必要なものに限る。非ビジュアルのものは、GUI(LCL のオブジェクト・メッセージループ)と関わるものだけを扱い、
  それ以外は C++・Python の標準・定番のライブラリを使ってもらう。

## 決定

**選択肢B を選ぶ。** 次の基準で扱うかどうかを決める。

- **扱う**: GUI の開発に必要なもの。
  - コントロール・フォーム・メニュー・ダイアログ等の画面の部品と、その描画(Canvas・グラフィック・画像の一覧)。
  - 非ビジュアルのものは、LCL のオブジェクトやメッセージループと関わり、GUI のアプリを作るのに要るものに限る。
    - 実装済みの例: Application・Screen・Clipboard・TTimer・TActionList。
    - 部品のプロパティの型として要るもの: TStrings・TStringList(ListBox の Items・Memo の Lines 等。[ADR 0028](0028-labelededit-and-stringlist.md))。
    - これから扱うもの: 別のスレッドからメインスレッドへ処理を渡す仕組み(`TThread::Synchronize`・`TThread::Queue`)。
      画面の部品はメインスレッドからしか触れないため、GUI のアプリで重い処理を別のスレッドに分けるのに欠かせない(別の ADR で設計する)。
- **扱わない**:
  - データベース(TDataSet・データ対応コントロール・各 DB のドライバ)
  - 印刷(TPrinter・TPrintDialog・TPrinterSetupDialog)
  - 設定ファイル・レジストリ(TIniFile・TRegistry)
  - ネットワーク・JSON・正規表現・ファイル操作・汎用のコンテナ
  - これらは、C++・Python の標準・定番のライブラリを使ってもらう(README に代わりの例を書く)。

扱わないものに要望があれば、この基準に当てはまるかを見直してから、別の ADR で判断する。DB のデータ対応コントロールのように
デザイナーとの連携に価値があるものでも、本体とは別のパッケージとして検討する。

## 影響

- 1.0 までに残る作業は、範囲の中の品質(C++Builder との違いの総点検・回帰テスト・文書)と、スレッドの受け渡しになる。
- [vision.md](../vision.md) のスコープの「やらないこと」に、上の非ビジュアルのライブラリを加える。
- [component-coverage.md](../component-coverage.md) の TPrintDialog・TPrinterSetupDialog は、見送りから対象外に変える。
- C++Builder のコードを移すときは、DB・印刷・設定ファイル等の部分を、C++・Python のライブラリで書き直すことになる。
