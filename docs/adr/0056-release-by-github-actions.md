# 0056. C++ のライブラリと Python のパッケージを GitHub Actions でリリースする

- 状態: 承認
- 日付: 2026-09-30

## 背景

これまでのリリースは手元で行っていた。
- ビルド(`build-windows.sh`)と検証(デザイナーの検査・生成したコードの照合)を実行する。
- 配布物(`package-windows.sh`・`package-python.sh`)を作る。
- GitHub の Release と PyPI に手で上げる。

手順が多く、上げ忘れや、手元の状態(コミットしていない変更・古い `beth.dll`)が配布物に入る恐れがある。
また、MSYS2 の Python 向けの sdist([ADR 0055](0055-python-sdist-for-msys2.md))が加わり、確かめることが増えた。

## 決定

GitHub Actions で、ビルド・検証・配布物の作成と、C++ のライブラリと Python のパッケージの公開を行う。

- **`build.yml`**(再利用のワークフロー)は、Windows のランナーで手元と同じ手順を実行する。
  - Lazarus(4.8。FPC 3.2.2)を公式のインストーラで入れ、キャッシュする。gcc・cmake・ninja は MSYS2(MINGW64)のものを使う。
  - `build-windows.sh` を実行する。
  - デザイナーの `pnpm check`・`codegen:verify-cpp`・`codegen:verify-python` を実行する。
  - `package-windows.sh`・`package-python.sh` を実行する。
  - `check-python-install.sh` で、python.org の Python に wheel を、MSYS2(UCRT64)の Python に sdist を入れて動かす。
  - zip・wheel・sdist を成果物として残す。
- **`ci.yml`**: master への push と PR で `build.yml` を実行する。
- **`release.yml`**:
  - タグ `v<バージョン>` の push で実行する。
    1. タグ・`CMakeLists.txt`・`py/pyproject.toml` のバージョンと、`CHANGELOG.md` の日付つきの節が揃っているかを確かめる。
    2. `build.yml` を実行する。
    3. PyPI に wheel・sdist を出す。
    4. GitHub の Release を作る。本文は CHANGELOG.md の該当する節で、zip・wheel・sdist を添える。
  - 手で実行(workflow_dispatch)すると、TestPyPI に出す。本番の前の確かめで、Release は作らない。
  - PyPI・TestPyPI へは Trusted Publishing で出す。API トークンをリポジトリに置かない。
  - GitHub の環境 `pypi`・`testpypi` に承認者を設定し、公開の前で止める。
- **デザイナーの vsix は含めない**。これまでどおり手で作って Marketplace に出す。

## 初めに一度だけ行う設定

- PyPI の `bethany-lcl` の Publishing に、Trusted Publisher(GitHub)を加える。
  - Owner: `okano-tomoyuki`
  - Repository: `bethany`
  - Workflow: `release.yml`
  - Environment: `pypi`
  - TestPyPI にも、Environment を `testpypi` にして同じものを加える。
- GitHub のリポジトリの Settings → Environments に `pypi`・`testpypi` を作り、Required reviewers に自分を加える。

## リリースの手順

1. バージョンを上げる(`CMakeLists.txt`・`py/pyproject.toml`)。`CHANGELOG.md` の `[Unreleased]` を `[<バージョン>] - <日付>` にする。README・example の URL も上げる。
2. コミットして master に push する(ci.yml で検証される)。
3. タグ `v<バージョン>` を付けて push する。release.yml が走り、環境 `pypi` の承認を待つ。
4. 承認すると、PyPI への公開と GitHub の Release の作成が行われる。
5. デザイナーの vsix は手で作って出す。

## 実装で決めたこと

- **ランナーは英語の Windows**である。手元(日本語の Windows)と次の点が違うため、合わせた。
  - Python の既定の文字コードが cp1252 で、日本語を出力するスクリプトが失敗する。ワークフローで UTF-8 モード(`PYTHONUTF8=1`)にする。
  - システムのフォントが Segoe UI(日本語の Windows では Yu Gothic UI)で、Windows が決める大きさが変わる。
    - TStatusBar の高さ(23 と 24)。生成したコードの照合では、TStatusBar は Left・Width だけを比べる。
    - グループボックス等の内側の余白(`metrics.json`)。配置の記録([ADR 0036](0036-designer-canvas.md))は日本語の Windows で取ったものなので、
      その照合(`layout:check`)は CI では行わず、手元で行う。
- 手順が失敗したら、出力の末尾を GitHub の注釈にする(`.github/run-and-annotate.sh`)。ログを開かずに失敗の理由が分かる。

## 影響

- 配布物はランナーでビルドした `beth.dll` を含む。手元の状態に左右されない。
- リリースの前に、手元で行っていた検証が(配置の記録の照合を除いて)自動で行われる。GUI を表示する検証(生成したコードの照合)も、Windows のランナーで実行する。
