# Changelog

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
