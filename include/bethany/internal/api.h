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
using key_press_callback_t = void (BETH_CALL *)(obj_t sender, int_t* key, void* data);
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

#define BETH_DECLARE_FUNC(ret, name, params, args) ret name params;
BETH_FUNCS(BETH_DECLARE_FUNC)
#undef BETH_DECLARE_FUNC

} // namespace internal
} // namespace beth

#endif
