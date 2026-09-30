#ifndef BETH_INTERNAL_API_H
#define BETH_INTERNAL_API_H

// DLL の呼び出し層(docs/adr/0032)。beth.hpp の実装が使うもので、利用者は直接使わない。
// 各関数は DLL の同名の公開関数を呼び、DLL の中で例外が起きていれば beth::Exception を送出する(docs/adr/0031)。

#if defined(_WIN32) || defined(_WIN64)
    #define BETH_CALL __stdcall
#else
    // x86_64 Linux では呼び出し規約は 1 種類しかなく、__cdecl はキーワードとして存在しない。
    #define BETH_CALL
#endif

#include <cstdint>

#include "funcs.h"

namespace beth
{
namespace internal
{

// DLL との受け渡しの型(docs/dll-abi.md)。
using obj_t  = void*;
using str_t  = const char*;   // UTF-8。DLL が返す文字列は、同じスレッドで次に文字列を返す関数を呼ぶまで有効。
using int_t  = int;
using uint_t = unsigned int;  // ビット集合(グリッド・ダイアログの Options、TFont の Style。グリッドの Options は 32 ビットすべてを使う)。
using bool_t = int;           // Pascal の LongBool。0 以外は真(DLL が返す真は -1)。
using real_t = double;
using iptr_t = std::intptr_t; // ポインタと同じ幅の符号付き整数(Pascal の PtrInt。TComponent の Tag)。

// DLL が呼ぶコールバック(イベント・破棄通知)。data は登録時に渡した値。
using callback_t = void (BETH_CALL *)(obj_t sender, void* data);
using close_callback_t = void (BETH_CALL *)(obj_t sender, int_t* action, void* data);
using close_query_callback_t = void (BETH_CALL *)(obj_t sender, bool_t* canClose, void* data);
using bool_callback_t = void (BETH_CALL *)(obj_t sender, bool_t value, void* data);
using exception_callback_t = void (BETH_CALL *)(obj_t sender, str_t className, str_t message, void* data);
using key_callback_t = void (BETH_CALL *)(obj_t sender, int_t* key, int_t shift, void* data);
// OnKeyPress(docs/adr/0063)。key は入力された 1 文字(UTF-8)。*result に文字列を返すと、その先頭の 1 文字を入力にする(空なら入力を捨てる)。
using key_press_callback_t = void (BETH_CALL *)(obj_t sender, str_t key, str_t* result, void* data);
using mouse_callback_t = void (BETH_CALL *)(obj_t sender, int_t button, int_t shift, int_t x, int_t y, void* data);
using mouse_move_callback_t = void (BETH_CALL *)(obj_t sender, int_t shift, int_t x, int_t y, void* data);
using mouse_wheel_callback_t = void (BETH_CALL *)(obj_t sender, int_t shift, int_t wheelDelta, int_t x, int_t y, bool_t* handled, void* data);
using item_callback_t = void (BETH_CALL *)(obj_t sender, obj_t item, void* data);
using item_allow_callback_t = void (BETH_CALL *)(obj_t sender, obj_t item, bool_t* allow, void* data);
using item_int_callback_t = void (BETH_CALL *)(obj_t sender, obj_t item, int_t value, void* data);
using cell_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, void* data);
using header_callback_t = void (BETH_CALL *)(obj_t sender, int_t isColumn, int_t index, void* data);
using cell_allow_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, bool_t* canSelect, void* data);
using draw_cell_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, int_t left, int_t top, int_t right, int_t bottom, uint_t state, void* data);
using section_track_callback_t = void (BETH_CALL *)(obj_t sender, obj_t section, int_t width, int_t state, void* data);
using section_drag_callback_t = void (BETH_CALL *)(obj_t sender, obj_t fromSection, obj_t toSection, bool_t* allow, void* data);
using item_rect_callback_t = void (BETH_CALL *)(obj_t sender, obj_t item, int_t left, int_t top, int_t right, int_t bottom, void* data);
// ファイル名(UTF-8)の数と配列。呼び出しの間だけ有効(docs/adr/0047)。
using drop_files_callback_t = void (BETH_CALL *)(obj_t sender, int_t count, str_t* fileNames, void* data);
// TScrollBar の OnScroll(操作の種類と、つまみの位置)・TCheckGroup の OnItemClick(docs/adr/0049)。
using scroll_callback_t = void (BETH_CALL *)(obj_t sender, int_t scrollCode, int_t* scrollPos, void* data);
using int_callback_t = void (BETH_CALL *)(obj_t sender, int_t value, void* data);
// オーナードロー(docs/adr/0050)。state は TOwnerDrawState のビット。
using draw_item_callback_t = void (BETH_CALL *)(obj_t sender, int_t index, int_t left, int_t top, int_t right, int_t bottom, uint_t state, void* data);
using measure_item_callback_t = void (BETH_CALL *)(obj_t sender, int_t index, int_t* height, void* data);
using menu_draw_callback_t = void (BETH_CALL *)(obj_t sender, obj_t canvas, int_t left, int_t top, int_t right, int_t bottom, uint_t state, void* data);
using menu_measure_callback_t = void (BETH_CALL *)(obj_t sender, obj_t canvas, int_t* width, int_t* height, void* data);
// TTreeView の OnCompare・OnEdited・OnCustomDrawItem(docs/adr/0051)。OnEdited は *result に文字列を返すとそれを使う
// (nil なら s のまま。返した文字列は DLL が写すまで有効であること)。state は TCustomDrawState のビット。
using tv_compare_callback_t = void (BETH_CALL *)(obj_t sender, obj_t node1, obj_t node2, int_t* compare, void* data);
using tv_edited_callback_t = void (BETH_CALL *)(obj_t sender, obj_t node, str_t s, str_t* result, void* data);
using tv_custom_draw_callback_t = void (BETH_CALL *)(obj_t sender, obj_t node, uint_t state, bool_t* defaultDraw, void* data);
// TListView の OnCompare・OnCustomDrawSubItem・OnDrawItem(docs/adr/0052)。OnEdited・OnCustomDrawItem は TTreeView と同じ形を使う。
// OnDrawItem の state は TOwnerDrawState のビット。
using lv_compare_callback_t = void (BETH_CALL *)(obj_t sender, obj_t item1, obj_t item2, int_t data, int_t* compare, void* cbData);
using lv_custom_draw_sub_item_callback_t = void (BETH_CALL *)(obj_t sender, obj_t item, int_t subItem, uint_t state, bool_t* defaultDraw, void* data);
using lv_draw_item_callback_t = void (BETH_CALL *)(obj_t sender, obj_t item, int_t left, int_t top, int_t right, int_t bottom, uint_t state, void* data);
// グリッドの OnGetEditText・OnSetEditText・OnValidateEntry・OnPrepareCanvas・OnCompareCells・OnColRow…(docs/adr/0053)。
// *result に文字列を返すとそれを使う(nil ならそのまま。返した文字列は DLL が写すまで有効であること)。state は TGridDrawState のビット。
using grid_get_edit_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, str_t value, str_t* result, void* data);
using grid_set_edit_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, str_t value, void* data);
using grid_validate_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, str_t oldValue, str_t newValue, str_t* result, void* data);
using grid_prepare_canvas_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, uint_t state, void* data);
using grid_compare_cells_callback_t = void (BETH_CALL *)(obj_t sender, int_t acol, int_t arow, int_t bcol, int_t brow, int_t* result, void* data);
using grid_operation_callback_t = void (BETH_CALL *)(obj_t sender, int_t isColumn, int_t sIndex, int_t tIndex, void* data);
// グリッドの OnSelectEditor・OnGetCheckboxState・OnSetCheckboxState・OnCheckboxToggled(docs/adr/0054)。state は TCheckBoxState の序数。
using grid_select_editor_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, obj_t* editor, void* data);
using grid_get_checkbox_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, int_t* state, void* data);
using grid_set_checkbox_callback_t = void (BETH_CALL *)(obj_t sender, int_t col, int_t row, int_t state, void* data);

#define BETH_DECLARE_FUNC(ret, name, params, args) ret name params;
BETH_FUNCS(BETH_DECLARE_FUNC)
#undef BETH_DECLARE_FUNC

} // namespace internal
} // namespace beth

#endif
