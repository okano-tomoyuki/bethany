"""internal/api.h と internal/funcs.h から Python の ctypes バインディング(beth/_internal.py)を生成する。

- 型の対応(obj_t → c_void_p 等)とコールバック型は internal/api.h の using から作る。
- 関数の一覧は internal/funcs.h の BETH_FUNCS から作る。引数は「型 名前」なので型の部分だけを使う。
"""
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
API_HEADER = HERE.parent / "internal" / "api.h"
FUNCS_HEADER = HERE.parent / "internal" / "funcs.h"
OUTPUT_FILE = HERE / "beth" / "_internal.py"

# internal/api.h の基本型 → ctypes の型
BASE_TYPES = {
    "void": "None",
    "obj_t": "obj_t",
    "str_t": "str_t",
    "int_t": "int_t",
    "uint_t": "uint_t",
    "bool_t": "bool_t",
    "real_t": "real_t",
    "iptr_t": "iptr_t",
    "char": "c_char",
    "void*": "c_void_p",
}

# using xxx_callback_t = void (BETH_CALL *)(obj_t sender, void* data);
CALLBACK_RE = re.compile(r"using\s+(\w*callback_t)\s*=\s*void\s*\(\s*BETH_CALL\s*\*\s*\)\s*\(([^)]*)\)\s*;")

# X(ret, name, (params), (args))
FUNC_RE = re.compile(r"X\(\s*([^,]+?)\s*,\s*(\w+)\s*,\s*\(([^)]*)\)\s*,\s*\(([^)]*)\)\s*\)")


def param_types(params):
    """「obj_t o, int_t* l」から型だけを取り出す(["obj_t", "int_t*"])。"""
    params = params.strip()
    if params in ("", "void"):
        return []
    types = []
    for p in params.split(","):
        p = p.strip()
        # 最後の識別子(引数名)を落とす。ポインタの * は型の側に残す。
        m = re.fullmatch(r"(.+?)\s*\b\w+", p)
        types.append(m.group(1).replace(" ", "") if m else p)
    return types


def ctype_of(t, callbacks):
    if t in BASE_TYPES:
        return BASE_TYPES[t]
    if t in callbacks:
        return t
    if t.endswith("*") and t[:-1] in BASE_TYPES:
        return f"POINTER({BASE_TYPES[t[:-1]]})"
    raise ValueError(f"未対応の型: {t}")


def parse_callbacks(text):
    callbacks = {}
    for m in CALLBACK_RE.finditer(text):
        callbacks[m.group(1)] = param_types(m.group(2))
    return callbacks


def parse_funcs(text):
    return [(m.group(1).strip(), m.group(2), param_types(m.group(3))) for m in FUNC_RE.finditer(text)]


HEADER = '''\
# このファイルは gen.py が internal/api.h と internal/funcs.h から生成する。直接編集しない。
"""beth.dll / libbeth.so の内部層(beth::internal)の Python バインディング。

- lib.<DLL の関数名>(...) で呼ぶ。DLL の中で例外が起きると BethError を送出する(docs/adr/0031)。
- コールバックの引数には Python の関数をそのまま渡せる。渡したコールバックは、同じ登録先
  (関数名と対象のオブジェクト)に次のコールバックを登録するまで保持する(GC で解放されないように)。
- コールバックの中で起きた Python の例外は SetCallbackError で DLL へ知らせる。公開関数の中
  (Show・Click 等)で起きたものは、その関数の呼び出し側で BethError(__cause__ が元の例外)になり、
  メッセージループの中で起きたものは LCL の Application.HandleException が処理する。
- コールバックは DLL を呼んだスレッド(GUI の処理は TApplication_Run を呼んだスレッド)の上で呼ばれる。
"""
import os
import platform
import threading
import ctypes
from ctypes import POINTER, c_void_p, c_char_p, c_char, c_int, c_uint, c_ssize_t, c_double

# 基本型(internal/api.h)
obj_t = c_void_p
str_t = c_char_p   # UTF-8 の bytes
int_t = c_int
uint_t = c_uint
bool_t = c_int     # Pascal の LongBool。0 以外は真(DLL が返す真は -1)
real_t = c_double
iptr_t = c_ssize_t  # ポインタと同じ幅の符号付き整数(Pascal の PtrInt。TComponent の Tag)

# DLL の呼び出し規約は Windows では __stdcall(Win64 では cdecl と同じ)
if platform.system() == "Windows":
    _FUNCTYPE = ctypes.WINFUNCTYPE
    _dll = ctypes.WinDLL(os.path.join(os.path.dirname(os.path.abspath(__file__)), "beth.dll"))
else:
    _FUNCTYPE = ctypes.CFUNCTYPE
    _dll = ctypes.CDLL(os.path.join(os.path.dirname(os.path.abspath(__file__)), "libbeth.so"))


class BethError(Exception):
    """DLL の中で起きた例外。class_name は Pascal の例外クラス名(コールバック由来なら元の例外のクラス名)。
    C++ の beth::Exception と同じ名前でも読める(E.Message・E.ClassName())。"""

    def __init__(self, class_name, message):
        super().__init__(f"{class_name}: {message}")
        self.class_name = class_name
        self.message = message

    @property
    def Message(self):
        return self.message

    def ClassName(self):
        return self.class_name


# 直前のエラー(docs/adr/0031)。DLL は失敗した呼び出しと同じスレッドで、その呼び出しから戻る前に知らせる。
_state = threading.local()

_error_callback_t = _FUNCTYPE(None, c_char_p, c_char_p)


def _on_dll_error(class_name, message):
    _state.error = ((class_name or b"").decode("utf-8", "replace"), (message or b"").decode("utf-8", "replace"))


_on_dll_error_ref = _error_callback_t(_on_dll_error)
_dll.Error_SetCallback.argtypes = [_error_callback_t]
_dll.Error_SetCallback.restype = None
_dll.Error_SetCallback(_on_dll_error_ref)


def _errcheck(result, func, args):
    error = getattr(_state, "error", None)
    if error is None:
        return result
    _state.error = None
    cause = getattr(_state, "callback_exception", None)
    _state.callback_exception = None
    raise BethError(*error) from cause


def _guard(fn):
    """コールバックの本体を包み、Python の例外を SetCallbackError で DLL へ知らせる。"""

    def guarded(*args):
        try:
            fn(*args)
        except BaseException as e:
            _state.callback_exception = e
            if isinstance(e, BethError):
                # DLL の例外・利用者が送出した BethError は、元のクラス名とメッセージのまま伝える
                class_name, message = e.class_name, e.message
            else:
                class_name, message = type(e).__name__, str(e)
            _dll.SetCallbackError(class_name.encode("utf-8"), message.encode("utf-8", "replace"))

    return guarded


# 登録中のコールバック。キーは (関数名, 対象のオブジェクト)
_callbacks = {}


def _key_of(value):
    if isinstance(value, c_void_p):
        return value.value
    return value


def _to_callback(proto, fn):
    if fn is None:
        return proto()  # NULL(コールバックの解除)
    if isinstance(fn, proto):
        return fn
    return proto(_guard(fn))


class _Lib:
    pass


lib = _Lib()


def _bind(name, restype, argtypes, callback_index=None):
    f = getattr(_dll, name)
    f.restype = restype
    f.argtypes = argtypes
    f.errcheck = _errcheck
    if callback_index is None:
        setattr(lib, name, f)
        return
    proto = argtypes[callback_index]
    has_owner = callback_index > 0

    def call(*args):
        args = list(args)
        cb = _to_callback(proto, args[callback_index])
        args[callback_index] = cb
        _callbacks[(name, _key_of(args[0]) if has_owner else None)] = cb
        return f(*args)

    call.__name__ = name
    setattr(lib, name, call)


# コールバック型(internal/api.h)
'''


def generate(callbacks, funcs):
    lines = [HEADER]
    for name, params in callbacks.items():
        lines.append(f"{name} = _FUNCTYPE(None, {', '.join(ctype_of(t, callbacks) for t in params)})")
    lines.append("")
    lines.append("# 関数(internal/funcs.h)")
    for ret, name, params in funcs:
        argtypes = [ctype_of(t, callbacks) for t in params]
        cb_index = next((i for i, t in enumerate(params) if t in callbacks), None)
        extra = f", {cb_index}" if cb_index is not None else ""
        lines.append(f"_bind({name!r}, {ctype_of(ret, callbacks)}, [{', '.join(argtypes)}]{extra})")
    lines.append("")
    return "\n".join(lines)


def main():
    callbacks = parse_callbacks(API_HEADER.read_text(encoding="utf-8"))
    funcs = parse_funcs(FUNCS_HEADER.read_text(encoding="utf-8"))
    OUTPUT_FILE.write_text(generate(callbacks, funcs), encoding="utf-8", newline="\n")
    print(f"生成完了: {OUTPUT_FILE}(コールバック型 {len(callbacks)}・関数 {len(funcs)})")

    # 公開 API(beth/__init__.py)も続けて生成する
    import gen_api
    gen_api.main()


if __name__ == "__main__":
    main()
