# デザイナーの開発

デザイナー([designer/](../../designer/README.md))の開発の手順。必要なもの(Node.js 24・pnpm・Python)とコマンドの一覧は designer/README.md。

## 1. 拡張を動かす

`designer/` を VS Code のフォルダとして開き、実行とデバッグの **Run Extension**(F5)を使う。

- 監視ビルド(タスク `watch`: 拡張は esbuild、Webview は Vite)が起動し、開発用ウィンドウで `designer/.cache/playground/` が開く。
  これは見本(`samples/`)の写しで、無ければ作る(tools/dev/playground.mjs)。見本はテストと検証が読むので、開発用ウィンドウでの
  編集が混ざらないようにしている。写しを見本に戻したいときは `.cache/playground` を消す。
- ファイルを保存すると自動でビルドされる。
  - Webview の変更: デザイナーを開き直すと反映される。
  - 拡張の変更: デバッグツールバーの再起動(Ctrl+Shift+F5)で反映される。
- デバッグを停止すると開発用ウィンドウも閉じる。
- 開発用ウィンドウでは他の拡張機能を読み込まない(`--disable-extensions`)。
- 監視ビルドは型検査をしない。型の誤りは `pnpm check`(またはエディタ上の表示)で確認する。
- Webview の中身は、開発用ウィンドウで「開発者: Webview 開発者ツールを開く」から調べられる。

## 2. Windows でデバッガが接続できない問題(js-debug の不具合)と対処

tk-designer の開発で調べたものと同じ(tk-designer の docs/development.md)。

**症状**: F5 で開発用ウィンドウは開くが、デザイナーが読み込み中のまま止まり、しばらくしてデバッグ実行が終了する。成功したり失敗したりする。

**原因**(2026-09 時点、VS Code 1.139 / js-debug 1.117.0 で確認):

1. デバッガ(js-debug)は拡張機能ホストに `http://localhost:<port>` で接続する。`localhost` の場合は `127.0.0.1` と `[::1]` の両方に
   並行してリクエストを送り、最初に成功したほうを採用する。
2. 拡張機能ホストのデバッグポートは `127.0.0.1` でしか待ち受けないため、`[::1]` へのリクエストは即座に拒否される。
3. 本来この拒否は「失敗」として扱われるべきだが、js-debug 内部の HTTP ライブラリの取り消し処理の不具合
   (`The onCancel handler was attached after the promise settled`)で例外になり、進行中だった `127.0.0.1` へのリクエストまで取り消される。
4. `127.0.0.1` の応答が `[::1]` の拒否より先に届いた場合だけ成功する。

**対処**: `[::1]` へのリクエストも成功するようにして、不具合の起きる経路を通らないようにする。

1. [launch.json](../../designer/.vscode/launch.json) の Run Extension で、デバッグポートを `"port": 9339` に固定している。
2. **この問題が起きる環境では、一度だけ次の設定を行う**(管理者権限の PowerShell)。Windows 標準のポート転送で、
   `[::1]:9339` への接続を `127.0.0.1:9339` に転送する。設定は再起動後も残る(tk-designer と同じポートなので、設定済みなら不要)。

   ```powershell
   netsh interface portproxy add v6tov4 listenaddress=::1 listenport=9339 connectaddress=127.0.0.1 connectport=9339
   # 確認
   netsh interface portproxy show all
   # 不要になったら削除
   netsh interface portproxy delete v6tov4 listenaddress=::1 listenport=9339
   ```

   ポート転送には「IP Helper」サービス(iphlpsvc)が動いている必要がある(既定で自動起動)。

- ポートを固定しているため、開発用ウィンドウは同時に 1 つだけ開く(tk-designer の開発用ウィンドウとも同時には開けない)。
- この問題の起きない環境では、ポート転送の設定は不要(`"port": 9339` があっても害はない)。

**予備の構成**: どうしても接続できない場合は **Attach to Extension Host (127.0.0.1)** を使う。タスク `launch-dev-host` が
デバッグポートを 9339 に固定して開発用ウィンドウを起動し、Node の attach 構成で `127.0.0.1:9339` に直接接続する(不具合の経路を通らない)。
デバッグを停止しても開発用ウィンドウは閉じないので手で閉じる、拡張の変更は開発用ウィンドウで「開発者: ウィンドウの再読み込み」を
実行して反映する、という点が Run Extension と異なる。
