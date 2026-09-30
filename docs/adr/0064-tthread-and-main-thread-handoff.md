# 0064. TThread と、メインスレッドへの受け渡し(Synchronize・Queue)を加える

- 状態: 承認
- 日付: 2026-09-30

## 背景

画面の部品(コントロール・フォーム等)はメインスレッドからしか触れない(VCL・LCL と同じ)。重い処理を別のスレッドに分け、結果を画面に
出すには、別のスレッドからメインスレッドへ処理を渡す仕組みが要る。C++Builder では `TThread` の派生クラスで `Execute` を書き、
`Synchronize`(終わるまで待つ)・`Queue`(待たない)でメインスレッドに渡す。GUI の開発に必要なものとして、[ADR 0060](0060-scope-gui-development.md)
で扱うと決めていた。

調べて分かったこと:

- **DLL は別のスレッドから呼ばれることを想定していなかった**。FPC は、自分で `BeginThread` したときにしか `IsMultiThread` を立てない。
  Bethany の DLL は自分ではスレッドを作らないため、C++ の `std::thread` や Python の `threading` から DLL の関数を呼ぶと、メモリ管理等が
  スレッドに対して安全でなかった。なお、FPC の DLL は `DLL_THREAD_ATTACH` でスレッドごとの変数を用意するので、FPC が作っていない
  スレッドから呼ぶこと自体はできる。
- **DLL ではメインスレッドを起こせなかった**。LCL の Win32 の `WakeMainThread` は、アプリケーションのウィンドウ(`AppHandle`)に
  WM_NULL を送る。DLL(`IsLibrary`)ではこのウィンドウが作られない(`AppHandle = 0`)ため、何も送られない。メッセージを待っている
  メインスレッド(`Application->Run` の中)が起きず、FPC の `TThread.Queue`・`Synchronize`、LCL の `Application.QueueAsyncCall` が、
  別のメッセージ(マウスの移動等)が来るまで実行されなかった。
- C++ には Delphi の `AfterConstruction`(構築の後に呼ばれる仕組み)が無い。VCL の `TThread(false)` は構築の直後に始まるが、
  基底のコンストラクタでスレッドを始めると、派生クラスの構築前に `Execute`(仮想関数)が呼ばれうる。

## 決定

### DLL

- DLL の初期化で `IsMultiThread := True` にする。
- **メインスレッドを起こす仕組みを替える**(Windows)。VCL と同じく、メッセージを受けるだけの隠れたウィンドウ(`HWND_MESSAGE`)を作る。
  - `WakeMainThread` を、そのウィンドウにメッセージを送るものに替える(`Application.Initialize` の後)。
  - ウィンドウの手続きがそのメッセージを受けたら、`CheckSynchronize` する。例外は `Application.HandleException`(OnException 等)に渡す。
  - スレッドへのメッセージ(`PostThreadMessage`)にしないのは、Windows のモーダルのループ(MessageBox・メニュー等)の間に捨てられるため。
  - これで `Application.QueueAsyncCall` も DLL で働くようになる。
- `TThread_Queue(cb, data)`: `cb` をメインスレッドで呼ぶ(FPC の `TThread.Queue`)。どのスレッドからでも呼べる。
- `TThread_IsMainThread()`: 呼んだのがメインスレッドか。

### C++

- `TThreadMethod`(`std::function<void()>`。メンバ関数もラムダも渡せる)。
- `TThread`(`TObject` の派生。LCL のオブジェクトは持たない)。
  - 静的な `Synchronize(TThread* AThread, TThreadMethod)`・`Queue(TThread*, TThreadMethod)`・`RemoveQueuedEvents(TThread*)`。
    `TThread::Synchronize(nullptr, [&] { ... })` のように、どのスレッドからでも呼べる。
  - protected の `Synchronize(TThreadMethod)`・`Queue(TThreadMethod)`(`Execute` の中から、自分を AThread にして呼ぶ)。
  - `Execute`(純粋仮想)・`Start`・`Terminate`・`Terminated`(protected)・`WaitFor`・`ReturnValue`(protected)・`FreeOnTerminate`・
    `OnTerminate`・`Finished`・`FatalException`(`std::exception_ptr`)。
- 振る舞いは VCL に合わせる。
  - `Synchronize` はメインスレッドで実行し終わるまで待ち、送出された例外は呼んだスレッドで送出し直す。`Queue` は待たない。
    どちらも、メインスレッドから呼ぶとその場で実行する。
  - `OnTerminate` は `Execute` の後にメインスレッドで呼ぶ。`FreeOnTerminate` なら、その後に自分を delete する(そのスレッドの上で)。
  - デストラクタは、動いていれば `Terminate` して終わるまで待ち、その TThread が Queue した未実行の処理を取り除く。
  - メインスレッドの `WaitFor` は、待つ間も渡された処理を実行する(待っている相手の `Synchronize`・`OnTerminate` で止まらないように)。
  - `FreeOnTerminate` のスレッドの `WaitFor`、2 度目の `Start` は例外(`EThread`)。
- **`TThread(false)` は構築が終わった後に始める**: 開始をメインスレッドの列に入れ、メッセージループ(か `WaitFor`)で始める。
  すぐに始めたいときは、`TThread(true)` にして構築の後で `Start()` を呼ぶ(ヘッダのコメントに書いた)。
- **仕組み**: 渡す処理の列(`std::mutex` と `std::function` の列)は C++ の側に持つ。別のスレッドから DLL を呼ぶのは、列が空でなくなったときに
  メインスレッドを起こす `TThread_Queue` の 1 回だけにする。メインスレッドで列を順に実行する。`Synchronize` の待ち合わせ・例外の受け渡し・
  `RemoveQueuedEvents` は C++ の側で行う。
- **ヘッダは `<thread>`・`<mutex>` を含めない**: `TThread` の実装は `beth.cpp` に置く(`std::unique_ptr<Impl>`)。スレッドの作成には
  C++11 の `std::thread` を使うので、ライブラリをビルドするコンパイラは `std::thread` に対応している必要がある(MSYS2・WinLibs 等の
  MinGW-w64 の g++ は対応している)。

### Python

- `TThread.Synchronize(AThread, func)`・`TThread.Queue(AThread, func)`・`TThread.RemoveQueuedEvents(AThread)`(静的なメソッド)。
  スレッドそのものは `threading.Thread` を使い、C++ のような派生クラスは用意しない(`TThread()` の生成は TypeError)。
- 仕組みは C++ と同じ(Python の側の列と、DLL の `TThread_Queue`)。DLL に渡すコールバックは 1 つを使い続ける(実行を待っている間に解放されないように)。
- 生成器は、ヘッダの `TThread` クラスを Python に出さない(手書きの `_core.TThread` を使う)。`TThreadMethod` はイベントの型として扱わない。

## 影響

- 別のスレッドから、画面の部品を安全に更新できる。DLL の関数を別のスレッドから呼んでもメモリ管理が壊れなくなる
  (ただし、画面の部品を別のスレッドから直接触らないという決まりは変わらない)。
- `Application.QueueAsyncCall` 等、LCL の中でメインスレッドを起こす処理も DLL で働く。
- 確認のプログラム(C++・Python)で、次を確かめた。
  - `std::thread`・`threading.Thread` からの `Synchronize`・`Queue` がメインスレッドで実行されること(`Synchronize` は終わるまで待つ)。
  - メインスレッドからはその場で実行されること、`Synchronize` の例外が呼んだスレッドで送出し直されること。
  - `TThread(false)` が構築の後に始まること、`Execute` の中の `Synchronize`、`Terminate`・`WaitFor`(ReturnValue)・メインスレッドでの `OnTerminate`。
  - `TThread(true)` と `Start`、2 度目の `Start` の例外、動いている間の delete、`FatalException`、`FreeOnTerminate`、`RemoveQueuedEvents`。
  - 8 つのスレッドから 500 回ずつの `Queue`(C++)・6 つのスレッドから 300 回ずつ(Python)。
  - `Application->Run` のメッセージループの中での `Synchronize`・`Queue`(直す前は、メインスレッドが起きず止まっていた)。
- デモ(`test/main.cpp`・`py/test_beth.py`)の "Thread" のページで、別のスレッドで数えながら画面を更新し、Stop で止められる。
