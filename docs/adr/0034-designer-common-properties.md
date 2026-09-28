# 0034. デザイナーで設定する共通のプロパティ(Anchors・BorderSpacing・Constraints・TabOrder 等)を追加し、集合型を Set<E> で表す

- 状態: 承認
- 日付: 2026-09-28

## 背景

Tier 1〜5 が揃い([ADR 0033](0033-dialogs.md))、デザイナーアプリ([ADR 0001](0001-designer-app-bundling.md))の設計に着手する段階になった。
デザイナーのプロパティ編集でまず必要になる、どのコントロールにもある共通のプロパティのうち、次のものが無かった。

- 配置: Anchors・BorderSpacing・Constraints(これまでの配置は Left/Top/Width/Height と Align だけで、大きさの変わるフォームを作れなかった)。
- 操作: TabOrder・TabStop・Hint・ShowHint・ParentShowHint・Cursor。
- 見た目: ParentColor・ParentFont(Color・Font は ADR 0033 で追加済み)。
- その他: TComponent の Tag。

特に Anchors は、デザイナーの DSL の配置モデル(座標 + Align + Anchors)の前提になるので、DSL を決める前に入れておく。

LCL のソース(`controls.pp`・`include/control.inc`・`include/customedit.inc`)で確認したこと:

- **公開範囲**:
  - TControl の public: Anchors・BorderSpacing・Constraints・ShowHint・Cursor・Hint。
  - TControl の protected(ほとんどの具象クラスが published にしている): ParentColor・ParentFont・ParentShowHint。
  - TWinControl の public: TabOrder・TabStop。
  - TComponent の published: Tag(`PtrInt`。ポインタと同じ幅)。
- **型**:
  - Anchors は `set of TAnchorKind`。LCL の TAnchorKind は `(akTop, akLeft, akRight, akBottom)` で、VCL の `(akLeft, akTop, akRight, akBottom)` と並びが違う。
  - BorderSpacing(TControlBorderSpacing)・Constraints(TSizeConstraints)は TPersistent で、コントロールが生成時に作り、差し替えない。
    SetBorderSpacing・SetConstraints は Assign(内容のコピー)。
  - BorderSpacing は LCL 固有で、VCL の Margins・AlignWithMargins に近い。
- **既定値**: Anchors は `[akLeft, akTop]`、TabOrder は同じ Parent の中で追加した順、ParentColor・ParentFont・ParentShowHint は True
  (ただし TCustomEdit は生成時に ParentColor を False にする)。

## 検討した選択肢

集合型(Anchors)の C++ での表し方:

- 選択肢A: これまでの TShiftState・TGridOptions と同じく、ビットの定数を OR した整数にする。
  定数の名前が akLeft 等の列挙型の要素と衝突するため、`akLeftBit` のような VCL に無い名前が要る。
  また、要素(列挙型)をそのまま代入すると序数が整数として渡り、誤ったビットになる(`Anchors = akLeft` が akTop の意味になる)。
- 選択肢B: C++Builder の `Set<T, minEl, maxEl>` に倣った値型 `Set<E>` を入れ、`using TAnchors = Set<TAnchorKind>` とする。
  C++Builder と同じく `Anchors = TAnchors() << akLeft << akTop;`・`Contains(akRight)` と書ける。要素の型が違うと代入できない。

ParentColor・ParentFont・ParentShowHint の置き場所:

- 選択肢A: ADR 0007 の原則どおり TControl の protected にし、公開している具象クラスで `using` する(対象のクラスが非常に多い)。
- 選択肢B: OnDblClick・OnMouseDown 等と同じく、TControl の public に置き、DLL 側は protected hack でアクセスする。

## 決定

- **追加したメンバ**:
  - TComponent: Tag(`Property<std::intptr_t>`。DLL との受け渡しに内部層の型 `iptr_t`(Pascal の PtrInt)を足した)。
  - TControl: Anchors・BorderSpacing・Constraints・Hint・ShowHint・ParentShowHint・Cursor・ParentColor・ParentFont。
  - TWinControl: TabOrder・TabStop。
  - 新しいクラス TSizeConstraints(MinWidth・MinHeight・MaxWidth・MaxHeight)・TControlBorderSpacing(Left・Top・Right・Bottom・Around・InnerBorder)。
    TFont と同じく、コントロールが所有するものへの非所有のラッパーで、コントロールの値メンバとして持つ。
    `Property<T*>` への代入は内容のコピー(`Button2->Constraints = Button1->Constraints;`)。
  - TCursor(`std::int32_t`)と crDefault 等の定数(LCL と同じ値)。
- **集合型**: 選択肢B。`template<typename E> class Set` を入れ、TAnchors をその別名にした。
  - 演算は `<<`(加える)・`>>`(除く)・`Contains`・`Empty`・`+`(和)・`-`(差)・`*`(積)・`==`。DLL とは要素の序数をビットの位置とした整数で受け渡す。
  - `Property<Set<E>>` から `Button1->Anchors->Contains(akRight)` と読めるよう、Set に `operator->` を持たせた。
  - 既存の整数のビット集合(TShiftState・TGridOptions・ダイアログの Options 等)は変えない。
    これらは要素の列挙型を C++ に持っておらず、ビットの定数(ssShift 等)が要素の名前を兼ねているため、Set にしても得るものが少ない。
    今後、要素の列挙型が既にある集合型を足すときは Set<E> を使う。
  - TAnchorKind の定義は、TControl より前に移した(TSplitter の ResizeAnchor と共有する)。
- **ParentColor・ParentFont・ParentShowHint**: 選択肢B(TControl の public。class-hierarchy.md の OnDblClick 等と同じ扱い)。
- **Python**: gen_api.py が `using X = Set<E>` を読み、Anchors を要素の frozenset として読み書きする(`Button1.Anchors = {akLeft, akTop}`・
  `akRight in Button1.Anchors`)。`std::intptr_t` は整数。TComponent は手書き(beth_core.py)なので、Tag はそこに足した。
- 見送ったもの: TControl.Name(LCL の Name はストリーミング・FindComponent 用で、生成コードはメンバ変数で参照するため当面は不要)、
  SetBounds、Constraints の OnChange・Options、BorderSpacing の CellAlign 系・Space[]、TabOrder の変更の通知。

## 実装して分かったこと

- **Anchors・BorderSpacing は、Align と同じくフォームが表示されるまで配置に反映されない**(値の読み書きはすぐできる)。
  右の辺との距離は、Anchors・Parent を設定した時点の Parent の大きさで決まる(表示前に Parent の幅を変えると、その差だけ幅が変わる)。
- 表示後は、左右の辺に付けたボタンの幅が、Parent(alClient のパネル)の幅の変化と同じだけ変わった(Splitter を 40px 動かして -40)。
- BorderSpacing.Around = 4 で alBottom に寄せたボタンは、パネルのクライアント領域(枠の 1px の内側)から 4px 離れた
  (Win32 で (5,41,183,22)。GTK2 はクライアント領域が 4px 狭いので (5,37,179,22))。
- Constraints は設定した時点で効く(MaxWidth = 60 の後に Width = 200 とすると 60 になる。表示を待たない)。
- TabOrder を 0 にすると、それまで 0 だったコントロールが 1 になる(同じ Parent の中で並べ直される)。
- ShowHint・Font・Color を設定すると、ParentShowHint・ParentFont・ParentColor は False になる。ただし、今と同じ色の代入では変わらない
  (LCL の SetColor は値が同じなら何もしない)。
- Tag は Win64 で 64 ビットの値(0x123456789A)をそのまま往復できた(Python)。
- Linux(GTK2、WSLg)でも、上の値は Windows と同じだった(BorderSpacing の座標だけ、上記のクライアント領域の差の分だけ違う)。

## 影響

- デザイナーで扱う配置モデル(座標・Align・Anchors・BorderSpacing・Constraints)が、C++・Python の両方で揃った。
- Anchors の要素の並びは LCL に合わせたため、VCL の TAnchorKind の序数に依存するコードは書き換えが要る(名前で書いていれば影響しない)。
- 集合型の表し方が 2 通り(整数のビット集合と Set<E>)になった。新しいものは Set<E> を使い、既存のものは必要が出たときに見直す。
