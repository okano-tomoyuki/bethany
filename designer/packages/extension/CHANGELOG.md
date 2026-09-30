# Changelog

## [Unreleased]

## [0.6.0] - 2026-09-30

This version requires the Bethany C++ library and Python package 0.4.0 or later (the generated C++ code sets `Pen` and `Brush` of
shapes through pointers, `Shape1->Pen->Color`).

- `Pen` and `Brush` of shapes (`TShape`) in the Object Inspector: the color, width and style of the line, and the color and style of the
  fill. The canvas draws each `Shape` (rectangles, rounded rectangles, ellipses, diamonds, triangles and stars) with them

## [0.5.2] - 2026-09-30

- **Fixed**: the canvas drew controls without a window (`TLabel`, `TImage`, `TShape`, ...) over overlapping controls with a window
  (`TMemo`, `TPanel`, ...) when they came later in the form. As in the LCL and the VCL, controls with a window are now always drawn
  on top, so a label under a memo is hidden on the canvas as it is when the application runs

## [0.5.1] - 2026-09-30

The `Columns` of grids require the Bethany C++ library and Python package 0.3.1 or later.

- Grid columns from the Bethany library 0.3.1: the `Columns` property of grids lists its columns in the Object Inspector, where you can
  add, delete and reorder columns and set their `Title` (caption, font, ...), `Width`, `ButtonStyle`, `PickList`, `ValueChecked` and
  more. Items of collections can now have nested objects and string lists. The canvas draws the column widths and titles, and the bold
  and italic styles of `TitleFont`. Also new: the `OnSelectEditor`, `OnButtonClick`, `OnPickListSelect`, `OnGetCheckboxState`,
  `OnSetCheckboxState` and `OnCheckboxToggled` events

## [0.5.0] - 2026-09-30

This version requires the Bethany C++ library and Python package 0.3.0 or later (the generated code uses the new properties,
for example `ScrollBars` of memos is now a `TScrollStyle`).

- New properties from the Bethany library 0.3.0: `BorderStyle`, `Position`, `WindowState`, `BorderIcons`, `FormStyle`, `KeyPreview`
  and `ActiveControl` of forms, and `ModalResult`, `Default` and `Cancel` of buttons. The canvas draws the default button with an accent border
- New properties from the Bethany library 0.3.0: `PasswordChar`, `EchoMode`, `CharCase`, `Alignment`, `TextHint`, `NumbersOnly`,
  `AutoSelect` and `HideSelection` of edits, `WordWrap`, `WantReturns` and `WantTabs` of memos, `Alignment`, `Layout`, `WordWrap`,
  `Transparent`, `FocusControl` and `ShowAccelChar` of labels, and the `OnEnter` and `OnExit` events. The canvas shows the alignment and
  word wrap of labels and memos, and the text hint and password characters of edits
- New properties from the Bethany library 0.3.0: `MultiSelect`, `ExtendedSelect` and `Sorted` of list boxes, `Style`, `DropDownCount`,
  `Sorted`, `ReadOnly` and `AutoComplete` of combo boxes, `State` and `AllowGrayed` of check boxes, and the `OnSelectionChange`,
  `OnSelect`, `OnDropDown`, `OnCloseUp` and `OnChange` (check boxes) events. The canvas draws the grayed state of check boxes
- Integer properties with named constants (`ModalResult`, `Cursor`) are chosen from a list in the Object Inspector
- Status bar panels: the `Panels` property of status bars lists its items in the Object Inspector, where you can add, delete and
  reorder panels and set their `Text`, `Width`, `Alignment`, `Bevel` and `Style`. The canvas draws the panels when `SimplePanel` is off.
  Also new: `SizeGrip` and `AutoHint`, and the `OnDrawPanel` and `OnHint` events
- The C++ header declares handler parameters of const reference types with the namespace (`const beth::TRect& Rect`)
- New from the Bethany library 0.3.0: the `OnPaint` event of forms, panels and scroll boxes, and `Height`, `Orientation` and `Quality`
  of fonts. The canvas uses the font `Height` for the text size
- Actions: put an action list (Standard palette) and add actions to it with "Add Action" in the structure tree. Actions appear
  under the action list, and the `Action` property of controls and menu items chooses one. The Object Inspector and the canvas
  show the values that come from the action, and a warning tells when a property or `OnClick` set on the control is overridden
- New from the Bethany library 0.3.0: the `AllowDropFiles` property and the `OnDropFiles` event of forms (files dropped from Explorer)
- New from the Bethany library 0.3.0: the bevels (`BevelOuter`, `BevelInner`, `BevelWidth`, `BevelColor`) and caption alignment
  (`Alignment`, `VerticalAlignment`, `WordWrap`) of panels, `BorderStyle` and `BorderWidth`, `ScrollBars` of grids, tree views and
  list views, and `AutoScroll`, `HorzScrollBar` and `VertScrollBar` of forms and scroll boxes. The canvas draws the bevels, borders
  and caption alignment, and the layout accounts for bevels, borders and `BorderWidth`
- `ScrollBars` of memos is now a named value (`ssBoth`). Older form files with a number are read as the name
- New from the Bethany library 0.3.0: orientation, ticks and selection of track bars, orientation, style and step of progress bars,
  `LargeChange`, `SmallChange` and `OnScroll` of scroll bars, orientation and options of up-down buttons, and `Columns`,
  `ColumnLayout` and `AutoFill` of radio groups and check groups with the `OnSelectionChanged` and `OnItemClick` events. The canvas
  draws the orientation of progress bars, track bars and up-down buttons, and the columns of radio groups and check groups
- Owner draw from the Bethany library 0.3.0: `Style` and `ItemHeight` of list boxes, `ItemHeight` of combo boxes, `OwnerDraw` of
  menus, and the `OnDrawItem` and `OnMeasureItem` events. More color constants, including system colors such as `clBtnFace` and
  `clHighlight`, which the canvas draws with the Windows 11 default colors
- The default `Color` of tree views and grids and `FixedColor` of grids were wrong (almost black) in the catalog; they are now
  `clWindow` and `clBtnFace`
- New from the Bethany library 0.3.0: `MultiSelect`, `MultiSelectStyle`, `SortType`, `Indent`, `HotTrack`, `RightClickSelect` and
  `ToolTips` of tree views, and the `OnCompare`, `OnEditing`, `OnEdited` and `OnCustomDrawItem` events
- New from the Bethany library 0.3.0: `ShowColumnHeaders`, `ColumnClick`, `ToolTips`, `OwnerDraw`, `AutoSort`, `OwnerData` and
  `HotTrack` of list views, and the `OnCompare`, `OnData`, `OnEditing`, `OnEdited`, `OnCustomDrawItem`, `OnCustomDrawSubItem` and
  `OnDrawItem` events
- New from the Bethany library 0.3.0: `AutoEdit`, `AlternateColor`, `FocusColor`, `GridLineColor`, `GridLineWidth`, `TitleFont`,
  `AutoFillColumns`, `ColumnClickSorts` and `SortOrder` of grids, and the `OnGetEditText`, `OnSetEditText`, `OnValidateEntry`,
  `OnPrepareCanvas`, `OnCompareCells`, `OnTopLeftChanged`, `OnHeaderSized` and `OnColRow…` events. The canvas draws the grid with
  `Color`, `FixedColor`, `AlternateColor`, `GridLineColor` and `GridLineWidth`, and stretches the columns with `AutoFillColumns`
- The include guard macro does not repeat a namespace that is also the folder of the header
  (`APP_DIALOGS_ABOUT_HPP` instead of `APP_DIALOGS_DIALOGS_ABOUT_HPP` for `app::dialogs` and `dialogs/About.hpp`)

## [0.4.0] - 2026-09-29

This version requires the Bethany C++ library 0.2.0 or later for C++ (the headers moved to `include/bethany/`).

- Project settings editor: a project file (`*.bfproj.json`) opens in a settings screen, like the project options of C++Builder.
  Set the main form, the auto-create forms and their order, the languages, the C++ and Python settings and the settings
  for some forms, and see where the code of each form is generated. _Open as Text_ switches to the text editor.
  Clicking a project in the Forms view opens its settings
- _Generate Code for the Whole Project_ generates the code of all the forms of a project and its startup code at once
  (Forms view, the project settings, the editor title of a project file and the Command Palette)
- C++ settings in `codegen.cpp` of the project file: `namespace` (`app` or `app::ui`), `includeGuard` (`macro` or `pragma`),
  `includeGuardPrefix`, `headerExtension` / `sourceExtension`, and `headerDir` / `sourceDir` to put headers and sources in separate folders
- `overrides` in `codegen.cpp` change these settings for some forms only, matched by paths or patterns
  (`{ "forms": ["dialogs"], "namespace": "app::dialogs" }`). The last matching entry wins
- `moduleDir` in `codegen.python` puts the Python modules of the forms in a folder
  (for example, `cpp/` and `py/` folders next to the project file)
- **Changed**: the generated C++ code includes Bethany as a system header, `#include <bethany/beth.hpp>`
- **Changed**: the header uses an include guard macro (`#ifndef MAINFORM_HPP`) instead of `#pragma once` by default.
  The beginning and the end of the generated C++ files are marker regions, so changing the settings updates existing files.
  Files generated by earlier versions get the new regions when they are generated again
- **Changed**: code is generated only for forms in a project, because the output folders, the file extensions and the languages
  are set in the project. _Generate Code_ on a form that is not in a project offers to add it to a project (or to create one)
  and then generates the code. The CLI reports an error

## [0.3.0] - 2026-09-29

- Go to an event handler as in C++Builder: double-clicking a control on the canvas, or the **+** / **→** button on the Events tab,
  sets the handler name if needed, generates the code (adding the handler stub) and opens the handler in the editor.
  When both C++ and Python are generated, the language is asked the first time (setting `bethanyDesigner.goToHandler.language`)
- Icons for the classes in the palette, the structure tree and the non-visual components on the canvas.
  The palette can show icons only (the button next to its title)
- **Fixed**: double-clicking a control on the canvas set the event of the form instead of the control

## [0.2.0] - 2026-09-28

- Forms are created at startup as in C++Builder: the startup code creates the main form first and then the other forms of the project
  (`autoCreate` of the project file; all forms by default). _Create at Startup_ / _Don't Create at Startup_ switch it per form
- Python: a form module has a variable for the form (`Form2: "TForm2" = None`, like the global variable of a form in C++Builder),
  which the startup code assigns. Other forms use it as `import Form2` and `Form2.Form2` (see _Using other forms_ in the README)
- **Changed**: code generation settings moved from the form file to the project file. A form file (`*.bfm.json`) now holds only the design.
  The languages and the comment language are set in `codegen` of the project file; a form that is not in a project generates C++ and Python.
  The output files and the class name are always derived from the file name and the form name. `codegen` left in a form file is ignored with a warning.
  The _Code Generation Settings_ screen of the designer was removed
- _Generate Code_ on a project generates the application startup code (`Project1.cpp` / `Project1.py`): initialize the application,
  create the main form and run it
- Project files (`*.bfproj.json`) that list the forms of an application and its main form. The Forms view groups the forms by project
  and marks the main form. _Set as Main Form_, _Add to Project..._, _Remove from Project_ and _New Project_ commands
- Renaming, moving or deleting a form in VS Code updates the project files that contain it
- Bethany Designer view in the Activity Bar: a list of the forms in the workspace, a **+** button to create a form,
  and a button to create the first form when there is none
- _Create New Form_ creates the form in the first workspace folder by default, and the folder can be changed from the name box.
  The default name is MainForm, or Form2, Form3, ... when MainForm already exists

## [0.1.0] - 2026-09-28

First preview release.

- Custom editor for `*.bfm.json` with a palette, a canvas, a structure tree and an Object Inspector
- Layout of `Align`, `Anchors`, `BorderSpacing` and `Constraints` on the canvas
- Main menus, pop-up menus and non-visual components
- _Create New Form_ and _Generate Code_ commands (C++ and Python)
- Diagnostics in the Problems panel and a JSON Schema for `*.bfm.json`
- English and Japanese
