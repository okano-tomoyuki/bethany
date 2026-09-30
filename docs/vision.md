# ビジョン

## 目的

FreePascal Compiler(FPC) / Lazarus の LCL という既に成熟した GUI コンポーネント資産を、
薄い C++ ラッパーを経由して C++ から利用できるようにし、
Delphi / C++Builder が持っていた VCL のオブジェクトモデル
(プロパティ構文、`TObject`/`TForm`/`TButton` 等の継承関係、`Canvas.Pen.Color` のような合成構造)
にできる限り忠実な形で **OSS として再現する**。

RAD Studio 本体が持つビジュアルデザイナー(フォームデザイナー)は対象外とする。
その代わりに、専用のデザイナーアプリケーションを別途セットで提供することで、
デザイナー無しでは失われがちな生産性を補う([ADR 0001](adr/0001-designer-app-bundling.md))。

## 背景・市場調査

2026-09 に実施した市場調査の要点([todo.md](../todo.md) にも記録):

- [liblcl](https://github.com/ying32/liblcl) が「LCL を DLL 化してフラットな C API として C/Go/Rust/Nim 等に公開する」という、
  本ライブラリの DLL の層に相当する取り組みの先行事例として存在する。
- 一方で liblcl は手続き型のフラット API に留まっており、
  C++Builder のようなオブジェクト指向ラッパー(プロパティ構文、クラス階層)を提供する例は見当たらなかった。
  ここが本ライブラリの独自性になり得る。
- 本家 C++Builder には無償の Community Edition があるが、OSS ではなく利用条件(収益上限等)がある。
  本ライブラリはデザイナーを含まない代わりに、完全 OSS・手元の CMake/MinGW ツールチェインで完結する点で異なる層を狙う。
- FPC/LCL の Modified LGPL は静的リンクを明示的に許可しており、
  本ライブラリを使うアプリケーション側のライセンスを制約しない。

## 想定ユーザーと利用シーン

- かつて Delphi / C++Builder を使っていた世代で、OSS かつ手元の CMake/MinGW ツールチェインで完結させたい開発者
- LCL の成熟したウィジェット資産を、モダンな C++ から RAD 的な書き味で使いたい人
- Embarcadero のライセンス・利用条件に縛られたくない、社内ツール・プロトタイプ・学習用途の開発者

## 提供価値

1. **C++Builder ライクなプロパティ API**(`form.Caption = "...";` / `Canvas->Pen->Color = clRed;` 等)。
   `Property<T>` による軽量プロキシで実現し、`std::function` ベースのイベントハンドラ登録も備える。
2. **VCL の代替としての C++ ラッパー(`beth.hpp`)**。
   当初は低レベル C API(`beth_c.h`)との二層公開構成だったが、C API の公開は終了し、DLL の呼び出し層は
   内部層(`beth::internal`)とした([ADR 0032](adr/0032-internalize-c-api.md))。別の言語から使う場合は、DLL の関数を直接呼ぶ層を作る
   (決まりは [dll-abi.md](dll-abi.md))。
3. **専用デザイナーアプリケーションによるビジュアル編集**(tk-designer と同様の戦略。ADR 0001)。
   本リポジトリの `designer/` に Bethany 専用として作る([ADR 0035](adr/0035-designer-in-this-repository.md))。
   DSL からのコード生成を前提に、C++ 側のオブジェクトモデルを見直す(ADR 0003)。

## スコープ

### やること

- LCL 主要コントロールの C++Builder 風ラッパー(Phase 1〜5 で実施済み。[todo.md](../todo.md) 参照)
- DSL からのコード生成を見据えた初期化モデルの整理([ADR 0003](adr/0003-two-phase-initialization.md))
- 専用デザイナーアプリケーションとの連携を見据えた API 設計
- 専用デザイナーアプリケーション本体の設計・実装(本リポジトリの `designer/`。ADR 0035。設計は [designer/](designer/README.md))

### 検討中(後で判断)

- Windows 以外のプラットフォーム対応(現状は LCL の win32 ウィジェットセットに限定)

### やらないこと

- RAD Studio のようなフルの統合 IDE 化
- FPC 自体の代替・独自コンパイラの実装
- Embarcadero 製品との互換性維持(VCL のソースコードレベル互換は目指さない。オブジェクトモデルの「見た目」の忠実さを優先する)
- GUI の開発に関わらない非ビジュアルのライブラリ(データベース・印刷・設定ファイル・レジストリ・ネットワーク等)。C++・Python のライブラリを使ってもらう([ADR 0060](adr/0060-scope-gui-development.md))
