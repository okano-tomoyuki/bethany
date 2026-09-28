# Bethany Designer

A visual form designer for [Bethany](https://github.com/okano-tomoyuki/bethany), a library that brings the
[Lazarus](https://www.lazarus-ide.org/) LCL to C++ and Python with an API similar to C++Builder.

Design a form on a canvas that looks like the Windows LCL, then generate the C++ (`beth.hpp`) or Python (`beth`) code for it.
The form is stored in a JSON file (`*.bfm.json`) that you can also edit as text.

> **Preview.** Bethany and this extension are at version 0.x. The format of `*.bfm.json` and the generated code may change.

## Features

- **Canvas**: place controls from the palette, move and resize them with the mouse, and see `Align`, `Anchors`, `BorderSpacing`
  and `Constraints` laid out as the LCL does. Main menus and non-visual components are shown on the form.
- **Structure tree** and **Object Inspector**: edit properties (including nested objects, sets, references and `TStrings`),
  menu items and event handler names.
- **Code generation**: the _Generate Code_ command writes a form class (`class TMainForm : public TForm`) to your source files.
  Only the regions between the designer's markers are replaced, so your own code, including event handlers, is kept.
- **Forms view**: the Bethany Designer icon in the Activity Bar lists the forms in your workspace. Open a form with a click, create one with the **+** button, or generate code from the list.
- **Projects**: a project file (`*.bfproj.json`) lists the forms of an application and its main form. With projects, the Forms view groups the forms by project and marks the main form with a star. Renaming, moving or deleting a form in VS Code updates the projects that contain it.
- **Validation**: problems in `*.bfm.json` are shown in the Problems panel, and a JSON Schema is provided for text editing.
- English and Japanese.

## Getting started

1. Open the Bethany Designer view from the Activity Bar and choose **Create New Form**
   (or right-click a folder in the Explorer, or use _File > New File..._).
2. Design the form, then run **Generate Code** from the editor title bar.
3. Build the generated code with Bethany. For C++, see the C++ section of the
   [Bethany README](https://github.com/okano-tomoyuki/bethany#readme).

## Settings

| Setting                             | Default     | Description                                                      |
| ----------------------------------- | ----------- | ---------------------------------------------------------------- |
| `bethanyDesigner.canvas.fontFamily` | (automatic) | Typeface used on the canvas as the LCL default font              |
| `bethanyDesigner.canvas.fontSize`   | `0` (9 pt)  | Size of that font                                                |
| `bethanyDesigner.canvas.gridSize`   | `8`         | Spacing of the grid that controls snap to. `0` disables snapping |
| `bethanyDesigner.canvas.showGrid`   | `true`      | Draw the grid dots on the form                                   |

## Known limitations

- The canvas approximates the appearance of the Windows LCL in HTML. Sizes that depend on fonts (for example `AutoSize`) can differ
  from the running application.
- There is no preview with the real LCL yet, and no alignment tools, tab-order editor or copy and paste.

## 日本語

Bethany(Lazarus の LCL を C++Builder に似た API で C++・Python から使うライブラリ)のフォームデザイナーです。
キャンバスでフォームを設計し、C++ または Python のコードを生成します。フォームは `*.bfm.json` に保存され、テキストとしても編集できます。
生成するのはマーカーで囲まれた区間だけで、イベントハンドラなど利用者が書いたコードは残ります。
設計の資料は [docs/designer](https://github.com/okano-tomoyuki/bethany/tree/master/docs/designer) にあります。

## License

[MIT](LICENSE)
