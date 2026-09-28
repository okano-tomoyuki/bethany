# 0037. ライブラリの名前を no_vcl から Bethany(略称 beth)に変える

- 状態: 承認
- 日付: 2026-09-28

## 背景

このライブラリは、C++Builder のプロプライエタリなソフトウェアへの批判から、FPC と Lazarus の資産(LCL)を使って
C++Builder に匹敵する OSS を作るという動機で始めた。名前の no_vcl と README の標語「Goodbye VCL」は、その動機を表している。
デザイナー([ADR 0035](0035-designer-in-this-repository.md))を公開する前に、名前を見直す。

- **利用者として想定するのは、C++Builder をかつて使っていた人たち**である。VCL を否定する「No」は、その人たちを入口で身構えさせる。
  名前が「何ではないか」を言っていて、何のライブラリかが分からない。「no vcl」は検索でも埋もれる。
- **VCL は Embarcadero の製品の構成要素の名前**である。説明の中で「VCL に似た API」と書くのと違い、プロジェクトの名前に入れると、
  関係のある製品と誤解されやすい。
- **実際に使っているのは LCL** で、名前はその恩恵を示していない。LCL の成果にただ乗りするのではなく、LCL の発展にも寄与したい。
- 公開はしているが利用者はまだいない。デザイナーは未公開。PyPI と VS Code Marketplace に出す予定がある。
  名前を変えるなら、今がいちばん安い。

## 検討した選択肢

- 選択肢A: no_vcl のままにする。変更のコストは無いが、上の問題が残る。
- 選択肢B: 名前に LCL を入れる(例: lcl-cpp)。LCL との関係ははっきりするが、Lazarus の公式のプロジェクトと誤解されうる
  (入れるなら Lazarus のコミュニティの了解が要る)。
- 選択肢C: 独自の名前にし、LCL は副題で示す。誤解が無く、名前の由来で LCL への敬意を示せる。

名前は、Delphi(ギリシア神話のデルポイ)・Lazarus(聖書のラザロ)と同じく、神話や聖書の固有名詞から選ぶ。候補:

| 名前 | 由来 | 見送った理由 |
|---|---|---|
| Hiram | ソロモンの神殿の青銅細工を造った職人(列王記上 7 章)。「Builder」に対応する | 人名として多く、検索で埋もれる |
| Bezalel | 幕屋を造った工匠(出エジプト記 31 章) | 発音しにくい。PyPI の名前が使われている |
| Emmaus | 復活したイエスに弟子が出会った道(ルカ 24 章) | 「造る」「Lazarus」との結び付きが弱い |
| Castalia・Parnassus | デルポイの近くの泉・山 | かつて Delphi の IDE の拡張の名前として使われた |
| Pythia・Daedalus・Hermes・Janus 等 | デルポイの巫女・工匠・使者・双面の神 | 同名のソフトウェアが多い |

## 決定

**選択肢C。名前は Bethany、略称は beth。**

- **Bethany**(ベタニア)は Lazarus の村(ヨハネ 11 章の「ベタニアのラザロ」)。「Lazarus の地から来たもの」として、LCL への恩義を示す。
  **beth** はヘブライ語の文字 ב(ベート)の名前で「家」を意味し、Bethany の「Beth-」も同じ語。
- 副題は「C++/Python bindings for the Lazarus LCL」。README では FPC・Lazarus を目立つように紹介し、LCL の modified LGPL を守って配布する
  (DLL にライセンスの文面と入手先を添える)。作る中で見つけた LCL の不具合は、上流に報告する。
- **DSL のファイルの拡張子は `.bfm.json`**(Bethany Form)。Delphi の dfm・Lazarus の lfm の流れに合わせる。

名前の対応:

| 対象 | 旧 | 新 |
|---|---|---|
| ライブラリの名前 | no_vcl | Bethany |
| C++ | `no_vcl.hpp`・`no_vcl.cpp`・`namespace no_vcl`・`NO_VCL_*` | `beth.hpp`・`beth.cpp`・`namespace beth`・`BETH_*` |
| C API | `no_vcl_c.h`・`no_vcl_*` の関数・`no_vcl_*_t` の型 | `beth_c.h`・`beth_*`・`beth_*_t` |
| Pascal・共有ライブラリ | `no_vcl.pas`・`no_vcl.dll`・`libno_vcl.so`・`TNoVcl*`・`ENoVcl*` | `beth.pas`・`beth.dll`・`libbeth.so`・`TBeth*`・`EBeth*` |
| Python | `no_vcl`・`no_vcl_core`・`no_vcl_internal`・`NoVclError` | `beth`・`beth_core`・`beth_internal`・`BethError`(PyPI の配布名は `bethany-lcl`) |
| DSL | `*.nvform.json` | `*.bfm.json` |
| CLI | `nvd` | `beth` |
| 生成コードで予約する名前 | `nvd_` で始まる名前 | `beth_` で始まる名前 |
| VS Code 拡張 | `no-vcl-designer`(no_vcl Designer)・`noVclDesigner.*` | `bethany-designer`(Bethany Designer)・`bethanyDesigner.*` |
| デザイナーのパッケージ | `@no-vcl-designer/*` | `@bethany-designer/*` |
| リポジトリ | `no_vcl` | `bethany` |

- PyPI の `beth` は別のもの(更新の止まったチェスの道具)が使っているため、配布名を `bethany-lcl` にし、import の名前は `beth` にする
  (Pillow の `import PIL` と同じ形)。同じ環境にその道具を入れたときだけ import の名前がぶつかる。
- 過去の ADR・文書の表記も新しい名前に置き換えた。旧名との対応はこの ADR で辿る。

## 影響

- ライブラリ・デザイナー・文書のすべてを置き換える(1 度だけの大きな変更。利用者がいないため互換の層は作らない)。
- GitHub のリポジトリの名前を変える(旧 URL からは GitHub が転送する)。
- 公開の前に、商標の登録(第 9 類・第 42 類)に同じ名前が無いかを確かめる。
