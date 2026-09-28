# Changelog

## [Unreleased]

- Project files (`*.bfproj.json`) that list the forms of an application and its main form. The Forms view groups the forms by project
  and marks the main form. _Set as Main Form_, _Add to Project..._, _Remove from Project_ and _New Project_ commands
- Renaming, moving or deleting a form in VS Code updates the project files that contain it
- Bethany Designer view in the Activity Bar: a list of the forms in the workspace, a **+** button to create a form,
  and a button to create the first form when there is none
- _Create New Form_ creates the form in the first workspace folder by default, and the folder can be changed from the name box.
  The default name is MainForm, or Form2, Form3, ... when MainForm already exists

## [0.1.0] - Unreleased

First preview release.

- Custom editor for `*.bfm.json` with a palette, a canvas, a structure tree and an Object Inspector
- Layout of `Align`, `Anchors`, `BorderSpacing` and `Constraints` on the canvas
- Main menus, pop-up menus and non-visual components
- _Create New Form_ and _Generate Code_ commands (C++ and Python)
- Diagnostics in the Problems panel and a JSON Schema for `*.bfm.json`
- English and Japanese
