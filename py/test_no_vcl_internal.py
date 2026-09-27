import ctypes
import no_vcl_internal

lib = no_vcl_internal.lib

# ---- コールバック定義 ----

def cb_component_freed(obj, data):
    p = ctypes.cast(data, ctypes.POINTER(ctypes.c_int))
    p.contents.value += 1

class EMyError(Exception):
    pass

def cb_failing_menu_click(sender, data):
    raise EMyError("failed in the callback")

def cb_button_click(sender, data):
    p = ctypes.cast(data, ctypes.POINTER(ctypes.c_int))
    p.contents.value += 1
    print(f"Button clicked! count={p.contents.value}")

def cb_button_mousedown(sender, button, shift, x, y, data):
    print(f"Button mouse down: button={button} shift={shift} pos=({x},{y})")

def cb_edit_keydown(sender, key_ptr, shift, data):
    key = key_ptr.contents.value
    print(f"Edit key down: key={key} shift={shift}")

def cb_scrollbar_change(sender, data):
    pos = lib.TCustomScrollBar_GetPosition(sender)
    print(f"ScrollBar changed: Position={pos}")

def cb_trackbar_change(sender, data):
    pos = lib.TCustomTrackBar_GetPosition(sender)
    print(f"TrackBar changed: Position={pos}")

def cb_radiogroup_click(sender, data):
    idx = lib.TCustomRadioGroup_GetItemIndex(sender)
    print(f"RadioGroup clicked: ItemIndex={idx}")

def cb_checklistbox_clickcheck(sender, data):
    c0 = lib.TCustomCheckListBox_GetChecked(sender, 0)
    c1 = lib.TCustomCheckListBox_GetChecked(sender, 1)
    c2 = lib.TCustomCheckListBox_GetChecked(sender, 2)
    print(f"CheckListBox clicked: {c0}/{c1}/{c2}")

def cb_speedbutton_click(sender, data):
    down = lib.TCustomSpeedButton_GetDown(sender)
    print(f"SpeedButton clicked: Down={down}")

def cb_tabcontrol_change(sender, data):
    idx = lib.TTabControl_GetTabIndex(sender)
    print(f"TabControl changed: TabIndex={idx}")

def cb_checkbox_click(sender, data):
    chk = lib.TButtonControl_GetChecked(sender)
    print(f"CheckBox clicked: Checked={chk}")

def cb_radiobutton_click(sender, data):
    cap = lib.TControl_GetCaption(sender)
    chk = lib.TButtonControl_GetChecked(sender)
    print(f"RadioButton clicked: {cap} Checked={chk}")

def cb_edit_change(sender, data):
    txt = lib.TControl_GetText(sender)
    print(f"Edit changed: Text={txt}")

def cb_combobox_change(sender, data):
    idx = lib.TCustomComboBox_GetItemIndex(sender)
    txt = lib.TControl_GetText(sender)
    print(f"ComboBox changed: ItemIndex={idx} Text={txt}")

def cb_listbox_click(sender, data):
    idx = lib.TCustomListBox_GetItemIndex(sender)
    print(f"ListBox clicked: ItemIndex={idx}")

def cb_memo_change(sender, data):
    lines = lib.TCustomMemo_GetLines(sender)
    count = lib.TStrings_GetCount(lines)
    print(f"Memo changed: LineCount={count}")

def cb_form_close(sender, action_ptr, data):
    attempts = ctypes.cast(data, ctypes.POINTER(ctypes.c_int))
    attempts.contents.value += 1
    print(f"Form closing: attempt={attempts.contents.value}, default action={action_ptr.contents.value}")
    if attempts.contents.value == 1:
        action_ptr.contents.value = 0  # caNone
        print("Form closing blocked")

def cb_form_closequery(sender, canclose_ptr, data):
    print(f"Form close query: canClose={canclose_ptr.contents.value}")

def cb_form_destroy(sender, data):
    cap = lib.TControl_GetCaption(data)
    print(f"Form destroying: button caption={cap}")

def cb_form_show(sender, data):
    print("Form shown")

def cb_paintbox_paint(sender, data):
    canvas = lib.TPaintBox_GetCanvas(sender)
    pen = lib.TCanvas_GetPen(canvas)
    brush = lib.TCanvas_GetBrush(canvas)
    font = lib.TCanvas_GetFont(canvas)

    lib.TPen_SetColor(pen, 0x0000FF)
    lib.TPen_SetWidth(pen, 2)
    lib.TBrush_SetColor(brush, 0x00FFFF)
    lib.TCanvas_Rectangle(canvas, 10, 10, 110, 70)

    lib.TPen_SetColor(pen, 0xFF0000)
    lib.TBrush_SetColor(brush, 0xFFFFFF)
    lib.TCanvas_Ellipse(canvas, 120, 10, 200, 70)

    lib.TPen_SetColor(pen, 0x000000)
    lib.TCanvas_MoveTo(canvas, 10, 90)
    lib.TCanvas_LineTo(canvas, 200, 90)

    lib.TFont_SetColor(font, 0x008000)
    lib.TFont_SetSize(font, 14)
    lib.TCanvas_TextOut(canvas, 10, 100, b"Canvas drawing test")

def cb_timer_tick(sender, data):
    label = data
    cb_timer_tick.counter += 1
    txt = f"Tick: {cb_timer_tick.counter}".encode()
    lib.TControl_SetCaption(label, txt)
    print(f"Timer tick: {cb_timer_tick.counter}")

cb_timer_tick.counter = 0

# ---- ユーティリティ ----

def place(control, parent, left, top):
    lib.TControl_SetParent(control, parent)
    lib.TControl_SetLeft(control, left)
    lib.TControl_SetTop(control, top)
    return control

# ---- メイン ----

def main():
    freedCount = ctypes.c_int(0)
    clickCount = ctypes.c_int(0)
    closeAttempts = ctypes.c_int(0)

    lib.FreeNotify_SetCallback(cb_component_freed, ctypes.byref(freedCount))

    app = lib.GetApplication()
    lib.TApplication_SetTitle(app, b"Python no_vcl internal test")

    form = lib.TApplication_CreateForm(app)
    lib.TCustomForm_SetOnClose(form, cb_form_close, ctypes.byref(closeAttempts))
    lib.TCustomForm_SetOnShow(form, cb_form_show, None)

    lib.TControl_SetCaption(form, b"Hello from Python")
    lib.TControl_SetWidth(form, 640)
    lib.TControl_SetHeight(form, 930)

    # ---- Button ----
    button = place(lib.TButton_Create(form), form, 20, 20)
    lib.TControl_SetCaption(button, b"Click me")
    lib.TControl_SetWidth(button, 100)
    lib.TControl_SetHeight(button, 30)
    lib.TControl_SetOnClick(button, cb_button_click, ctypes.byref(clickCount))
    lib.TControl_SetOnMouseDown(button, cb_button_mousedown, None)
    lib.TCustomForm_SetOnDestroy(form, cb_form_destroy, button)

    # ---- Label ----
    label = place(lib.TLabel_Create(form), form, 20, 60)
    lib.TControl_SetCaption(label, b"Label text")

    # ---- Edit ----
    edit = place(lib.TEdit_Create(form), form, 20, 90)
    lib.TControl_SetText(edit, b"Edit me")
    lib.TControl_SetWidth(edit, 150)
    lib.TCustomEdit_SetOnChange(edit, cb_edit_change, None)
    lib.TWinControl_SetOnKeyDown(edit, cb_edit_keydown, None)

    # ---- CheckBox ----
    checkBox = place(lib.TCheckBox_Create(form), form, 20, 130)
    lib.TControl_SetCaption(checkBox, b"Check me")
    lib.TControl_SetOnClick(checkBox, cb_checkbox_click, None)

    # ---- RadioButtons ----
    radio1 = place(lib.TRadioButton_Create(form), form, 20, 160)
    lib.TControl_SetCaption(radio1, b"Option A")
    lib.TButtonControl_SetChecked(radio1, 1)
    lib.TControl_SetOnClick(radio1, cb_radiobutton_click, None)

    radio2 = place(lib.TRadioButton_Create(form), form, 20, 190)
    lib.TControl_SetCaption(radio2, b"Option B")
    lib.TControl_SetOnClick(radio2, cb_radiobutton_click, None)

    # ---- ComboBox ----
    combo = place(lib.TComboBox_Create(form), form, 220, 160)
    items = lib.TCustomComboBox_GetItems(combo)
    lib.TStrings_Add(items, b"Combo A")
    lib.TStrings_Add(items, b"Combo B")
    lib.TStrings_Add(items, b"Combo C")
    lib.TCustomComboBox_SetItemIndex(combo, 0)
    lib.TControl_SetWidth(combo, 150)
    lib.TComboBox_SetOnChange(combo, cb_combobox_change, None)

    # ---- ListBox ----
    listBox = place(lib.TListBox_Create(form), form, 220, 190)
    items = lib.TCustomListBox_GetItems(listBox)
    lib.TStrings_Add(items, b"List 1")
    lib.TStrings_Add(items, b"List 2")
    lib.TStrings_Add(items, b"List 3")
    lib.TControl_SetWidth(listBox, 150)
    lib.TControl_SetHeight(listBox, 80)
    lib.TControl_SetOnClick(listBox, cb_listbox_click, None)

    # ---- Memo ----
    memo = place(lib.TMemo_Create(form), form, 220, 280)
    lines = lib.TCustomMemo_GetLines(memo)
    lib.TStrings_Add(lines, b"Memo line 1")
    lib.TStrings_Add(lines, b"Memo line 2")
    lib.TControl_SetWidth(memo, 150)
    lib.TControl_SetHeight(memo, 80)
    lib.TCustomEdit_SetOnChange(memo, cb_memo_change, None)

    # ---- Timer ----
    tickLabel = place(lib.TLabel_Create(form), form, 20, 230)
    lib.TControl_SetCaption(tickLabel, b"Tick: 0")

    timer = lib.TTimer_Create(form)
    lib.TCustomTimer_SetInterval(timer, 500)
    lib.TCustomTimer_SetOnTimer(timer, cb_timer_tick, tickLabel)
    lib.TCustomTimer_SetEnabled(timer, 1)

    # ---- PaintBox ----
    paintBox = place(lib.TPaintBox_Create(form), form, 400, 20)
    lib.TControl_SetWidth(paintBox, 220)
    lib.TControl_SetHeight(paintBox, 130)
    lib.TPaintBox_SetOnPaint(paintBox, cb_paintbox_paint, None)

    # ---- 例外(docs/adr/0031) ----
    # DLL の中で LCL が例外を送出すると、関数が NoVclError を送出する。
    strList = lib.TStringList_Create()
    lib.TStrings_Add(strList, b"only")
    try:
        lib.TStrings_GetStrings(strList, 5)
        print("must not be reached")
    except no_vcl_internal.NoVclError as e:
        print(f"GetStrings(5) on 1 item raised: class_name={e.class_name} (expected EStringListError), message={e.message}")
    print(f"after the exception: Count={lib.TStrings_GetCount(strList)} (expected 1)")
    lib.TStringList_Destroy(strList)

    # コールバックの中で起きた Python の例外は、そのイベントを起こした関数(ここでは TMenuItem_Click)の NoVclError になる。
    failingItem = lib.TMenuItem_Create(form)
    lib.TMenuItem_SetOnClick(failingItem, cb_failing_menu_click, None)
    try:
        lib.TMenuItem_Click(failingItem)
        print("must not be reached")
    except no_vcl_internal.NoVclError as e:
        print(f"TMenuItem_Click with a failing callback raised: class_name={e.class_name} (expected EMyError), "
              f"message={e.message}, cause={type(e.__cause__).__name__}")

    # ---- ダイアログ(docs/adr/0033) ----
    # モーダルのダイアログの Execute は閉じるまで戻らないため、ここでは呼ばない。プロパティだけ確かめる。
    openDialog = lib.TOpenDialog_Create(form)
    lib.TFileDialog_SetFilter(openDialog, b"Text|*.txt|All|*.*")
    lib.TFileDialog_SetFilterIndex(openDialog, 2)
    lib.TOpenDialog_SetOptions(openDialog, lib.TOpenDialog_GetOptions(openDialog) | (1 << 6))  # ofAllowMultiSelect
    print(f"TOpenDialog Filter={lib.TFileDialog_GetFilter(openDialog).decode()} FilterIndex={lib.TFileDialog_GetFilterIndex(openDialog)} "
          f"(expected 2), Options=0x{lib.TOpenDialog_GetOptions(openDialog):x} (expected 0x900040)")
    colorDialog = lib.TColorDialog_Create(form)
    lib.TColorDialog_SetColor(colorDialog, 0x00FF00)
    print(f"TColorDialog Color={lib.TColorDialog_GetColor(colorDialog):06X} (expected 00FF00), "
          f"CustomColors Count={lib.TStrings_GetCount(lib.TColorDialog_GetCustomColors(colorDialog))} (expected 20)")
    fontDialog = lib.TFontDialog_Create(form)
    dialogFont = lib.TFontDialog_GetFont(fontDialog)
    lib.TFont_SetName(dialogFont, b"Arial")
    lib.TFont_SetStyle(dialogFont, 1 | 8)  # fsBold | fsStrikeOut
    lib.TControl_SetFont(memo, dialogFont)
    print(f"memo Font after TControl_SetFont: {lib.TFont_GetName(lib.TControl_GetFont(memo)).decode()} "
          f"Style=0x{lib.TFont_GetStyle(lib.TControl_GetFont(memo)):x} (expected Arial 0x9), "
          f"memo Color is clDefault: {lib.TControl_GetColor(memo) == 0x20000000}")
    replaceDialog = lib.TReplaceDialog_Create(form)
    lib.TFindDialog_SetFindText(replaceDialog, "検索".encode())
    lib.TFindDialog_SetReplaceText(replaceDialog, "置換".encode())
    print(f"TReplaceDialog FindText={lib.TFindDialog_GetFindText(replaceDialog).decode()} "
          f"ReplaceText={lib.TFindDialog_GetReplaceText(replaceDialog).decode()}")

    # ---- Show Form ----
    lib.TCustomForm_Show(form)
    lib.TApplication_Run(app)


if __name__ == "__main__":
    main()
