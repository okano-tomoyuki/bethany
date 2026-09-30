# 0059. デザイナーのリリースにもタグを付け、GitHub の Release に vsix を添える

- 状態: 承認
- 日付: 2026-09-30

## 背景

C++ のライブラリと Python のパッケージは、タグ `v<バージョン>` で GitHub Actions がリリースする([ADR 0056](0056-release-by-github-actions.md))。

デザイナー(VS Code 拡張)は、vsix を手で作って Marketplace に出していて、タグを付けていなかった。

- どの版がどのコミットから作られたかが、git から分からない。
- 0.5.1・0.5.2 のように、ライブラリと別の時期に出す版もある。そのため、ライブラリのタグからは辿れない。

## 決定

- デザイナーのリリースには、タグ `designer-v<バージョン>` を付ける。
  - ライブラリのタグ(`v*`)と接頭辞を分け、`release.yml` が反応しないようにする。
- タグの push で、`.github/workflows/designer-release.yml` が次を行う。
  1. タグと拡張の `package.json` のバージョン、拡張の `CHANGELOG.md` の日付つきの節が揃っているかを確かめる。
  2. `pnpm check` を実行し、vsix を作る。
  3. GitHub の Release を作り、vsix を添える。
     - 本文は、拡張の CHANGELOG.md の該当する節から作る。
     - `--latest=false` で作る。Releases の「Latest」は、README から案内するライブラリの zip のままにする。
- Marketplace への公開は、これまでどおり手で行う(公開の設定が整うまで)。
- 過去の版(0.5.2 まで)にはタグを付けない。

## リリースの手順

1. 拡張のバージョンを上げる(`designer/packages/extension/package.json`)。
2. 拡張の `CHANGELOG.md` の `[Unreleased]` を `[<バージョン>] - <日付>` にする。
3. コミットして master に push する(ci.yml で検証される)。
4. タグ `designer-v<バージョン>` を付けて push する。designer-release.yml が vsix を作り、Release に添える。
5. Marketplace に vsix を手で出す(Release に添えた vsix か、手元で作ったもの)。

## 影響

- デザイナーの各版のソースを、タグから辿れる。
- Marketplace を使わない利用者も、Release から vsix を入手できる。
- タグで動くワークフローは、タグが指すコミットの定義を使う。そのため、designer-release.yml を加える前のコミットには、タグを付けても動かない。
