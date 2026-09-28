# Bethany

C++/Python bindings for the Lazarus LCL

[Free Pascal](https://www.freepascal.org/) と [Lazarus](https://www.lazarus-ide.org/) の LCL(Lazarus Component Library)を、
薄いラッパー経由で C++ と Python から使えるようにする、C++Builder に似た API のライブラリ。
フォームを画面で設計するデザイナー(VS Code 拡張)を [designer/](designer/README.md) で開発している。

設計は [docs/](docs/README.md) を、実装の進捗は [todo.md](todo.md) を参照。

## 名前

Bethany(ベタニア)は、聖書でラザロ(Lazarus)が暮らした村の名前。Delphi・Lazarus と同じく神話や聖書の固有名詞にちなみ、
このライブラリが Lazarus の成果の上に成り立っていることを表す。略称の beth はヘブライ語の文字 ב(ベート、「家」の意味)の名前で、
ファイル名・C++ の名前空間・Python のモジュール名に使う(`beth.hpp`・`namespace beth`・`import beth`)。
経緯は [ADR 0037](docs/adr/0037-rename-to-bethany.md)。

## 謝辞

画面の部品とその振る舞いは、すべて Lazarus の LCL が提供している。Free Pascal と Lazarus の開発者・貢献者に感謝する。
LCL は modified LGPL(静的リンクの例外付き)で配布されており、Bethany の共有ライブラリ(`beth.dll`・`libbeth.so`)は LCL を含む。
