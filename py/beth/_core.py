"""Bethany の公開 API(beth/__init__.py)の土台。__init__.py は gen_api.py が beth.hpp から生成し、このモジュールの
クラス・デスクリプタを使う。利用者は beth を import し、このモジュールを直接使わない。

C++ ラッパー(beth.hpp)との対応:
- プロパティは Python のプロパティ(Button1.Caption = "OK")、添字付きのプロパティは添字(Grid.Cells[c, r]・Grid.Cells[c][r])。
- イベントは Python の呼び出し可能オブジェクトを代入する(Button1.OnClick = self.Button1Click)。ハンドラは
  C++ と同じく Sender を最初の引数に受け取る。C++ で参照渡し(int& Key・bool& CanClose 等)の引数は Ref で渡され、
  .value を書き換えると LCL に戻る。メソッドの出力引数(int& Index 等)にも Ref を渡す。
- 文字列は str(DLL との間は UTF-8)。集合(TShiftState 等)は enum.IntFlag、列挙型は enum.IntEnum。
- コンポーネントのラッパーの寿命は LCL オブジェクトに従う(docs/adr/0008)。破棄された後に触ると ReferenceError。
- DLL の中で起きた例外は BethError(C++ の Exception。Message・ClassName())になる(docs/adr/0031)。
"""
import atexit
import ctypes
import enum

from . import _internal
from ._internal import BethError, lib

__all__ = ["BethError", "Ref", "TRect", "TPoint", "TObject", "TPersistent", "TComponent"]


# ---------------- 値 ----------------

class Ref:
    """参照渡しの引数(C++ の int& Key・bool& CanClose・TCloseAction& Action 等)。.value を読み書きする。"""

    __slots__ = ("value",)

    def __init__(self, value=None):
        self.value = value

    def __repr__(self):
        return f"Ref({self.value!r})"


class TRect:
    """矩形(VCL の TRect)。TRect(Left, Top, Right, Bottom)。引数には (Left, Top, Right, Bottom) のタプルも渡せる。"""

    __slots__ = ("Left", "Top", "Right", "Bottom")

    def __init__(self, Left=0, Top=0, Right=0, Bottom=0):
        self.Left, self.Top, self.Right, self.Bottom = Left, Top, Right, Bottom

    def __iter__(self):
        return iter((self.Left, self.Top, self.Right, self.Bottom))

    def __eq__(self, other):
        return tuple(self) == tuple(other) if isinstance(other, (TRect, tuple)) else NotImplemented

    def __repr__(self):
        return f"TRect({self.Left}, {self.Top}, {self.Right}, {self.Bottom})"


class TPoint:
    """点(VCL の TPoint)。TPoint(X, Y)。引数には (X, Y) のタプルも渡せる。"""

    __slots__ = ("X", "Y")

    def __init__(self, X=0, Y=0):
        self.X, self.Y = X, Y

    def __iter__(self):
        return iter((self.X, self.Y))

    def __eq__(self, other):
        return tuple(self) == tuple(other) if isinstance(other, (TPoint, tuple)) else NotImplemented

    def __repr__(self):
        return f"TPoint({self.X}, {self.Y})"


# ---------------- 変換 ----------------

# beth/__init__.py が定義したクラス・列挙型(名前 → 型)。変換は型を名前で受け取り、使うときに引く
# (クラスの定義順に関係なく、後で定義されるクラスを型として使えるように)。
_types = {}


def _register(namespace):
    _types.update({k: v for k, v in namespace.items() if isinstance(v, type)})


def _enc(s):
    if isinstance(s, bytes):
        return s
    return str(s).encode("utf-8")


def _dec(b):
    return b.decode("utf-8", "replace") if b is not None else ""


def _h(obj):
    """ラッパーから DLL に渡すハンドル(None は NULL)。"""
    return None if obj is None else obj._current()


def _b(v):
    return 1 if v else 0


def _rect(r):
    left, top, right, bottom = r
    return int(left), int(top), int(right), int(bottom)


def _point(p):
    x, y = p
    return int(x), int(y)


def _key(v):
    """OnKeyPress の Key(1 文字の str。0 も受け付ける)を DLL の値にする。"""
    if isinstance(v, str):
        return ord(v) & 0xFF if v else 0
    return int(v) & 0xFF


def _to_enum(name, raw):
    try:
        return _types[name](raw)
    except ValueError:
        return raw


def _to_comp(name, raw):
    """TComponent のラッパー(Bethany のラッパーを介さずに作られたものなら None)。"""
    return _components.get(raw) if raw else None


def _to_existing(name, raw):
    """LCL が内部で生成したコンポーネント(メニューのルート項目・AddTabSheet のページ等)。無ければラッパーを作る。"""
    return _types[name]._wrap_existing(raw)


def _to_item(name, raw):
    """TComponent ではない項目(TTreeNode 等)。同じ項目には同じラッパーを返す。"""
    return _types[name]._wrap_item(raw)


def _to_obj(name, raw):
    """所有者の中身への非所有のラッパー(TCanvas・TTreeNodes 等)。"""
    return _types[name]._wrap_handle(raw)


class _Conv:
    """プロパティ・添字の値の変換。read/write は所有者と DLL の関数を受け取る(ビュー・矩形は呼び方が違うため)。"""

    def __init__(self, from_raw, to_raw):
        self.from_raw = from_raw
        self.to_raw = to_raw

    def read(self, obj, fn, *index):
        return self.from_raw(fn(obj._current(), *index))

    def write(self, obj, fn, value, *index):
        fn(obj._current(), *index, self.to_raw(value))


class _ViewConv(_Conv):
    """TStrings・TGraphic のビュー。中身のハンドルを覚えず、操作のたびに所有者から取得する(docs/adr/0027・0029)。"""

    def __init__(self, name):
        super().__init__(None, _h)
        self.name = name

    def read(self, obj, fn, *index):
        return _types[self.name]._wrap_view(obj, fn)


class _RectConv(_Conv):
    """TRect(DLL は 4 つの出力引数で返し、4 つの引数で受け取る)。"""

    def __init__(self):
        super().__init__(None, None)

    def read(self, obj, fn, *index):
        v = [ctypes.c_int() for _ in range(4)]
        fn(obj._current(), *index, *(ctypes.byref(x) for x in v))
        return TRect(*(x.value for x in v))

    def write(self, obj, fn, value, *index):
        fn(obj._current(), *index, *_rect(value))


_int = _Conv(lambda raw: raw, int)
_float = _Conv(lambda raw: raw, float)
_bool = _Conv(lambda raw: raw != 0, _b)
_str = _Conv(_dec, _enc)
_char = _Conv(lambda raw: raw.decode("latin-1"), lambda v: _enc(v)[:1])
_ptr = _Conv(lambda raw: raw, lambda v: v)
_rect_conv = _RectConv()


class _PointConv(_Conv):
    """TPoint(DLL は 2 つの出力引数で返し、2 つの引数で受け取る)。"""

    def __init__(self):
        super().__init__(None, None)

    def read(self, obj, fn, *index):
        x, y = ctypes.c_int(), ctypes.c_int()
        fn(obj._current(), *index, ctypes.byref(x), ctypes.byref(y))
        return TPoint(x.value, y.value)

    def write(self, obj, fn, value, *index):
        fn(obj._current(), *index, *_point(value))


_point_conv = _PointConv()


def _enum(name):
    return _Conv(lambda raw: _to_enum(name, raw), int)


def _set(name):
    """C++ の Set<E>(Pascal の集合型。TAnchors 等)。読むと要素(列挙型)の frozenset、書くときは要素の集まり
    ({akLeft, akTop} のような set・list・tuple)を渡す。"""
    def from_raw(raw):
        return frozenset(k for k in _types[name] if raw & (1 << int(k)))

    def to_raw(value):
        return sum(1 << int(k) for k in set(value))
    return _Conv(from_raw, to_raw)


def _comp(name):
    return _Conv(lambda raw: _to_comp(name, raw), _h)


def _existing(name):
    return _Conv(lambda raw: _to_existing(name, raw), _h)


def _item(name):
    return _Conv(lambda raw: _to_item(name, raw), _h)


def _obj(name):
    return _Conv(lambda raw: _to_obj(name, raw), _h)


def _view(name):
    return _ViewConv(name)


# ---------------- プロパティ ----------------

class _Prop:
    """Property / ReadOnlyProperty。"""

    def __init__(self, getter, setter, conv):
        self.getter = getter
        self.setter = setter
        self.conv = conv

    def __set_name__(self, owner, name):
        self.name = name

    def __get__(self, obj, objtype=None):
        if obj is None:
            return self
        return self.conv.read(obj, getattr(lib, self.getter))

    def __set__(self, obj, value):
        if self.setter is None:
            raise AttributeError(f"{type(obj).__name__}.{self.name} は読み取り専用")
        self.conv.write(obj, getattr(lib, self.setter), value)


class _IndexedProxy:
    __slots__ = ("_obj", "_prop")

    def __init__(self, obj, prop):
        self._obj = obj
        self._prop = prop

    def __getitem__(self, index):
        p = self._prop
        if p.dims == 2:
            if isinstance(index, tuple):
                return p.conv.read(self._obj, getattr(lib, p.getter), *map(int, index))
            return _IndexedSlice(self, int(index))
        return p.conv.read(self._obj, getattr(lib, p.getter), p.key(index))

    def __setitem__(self, index, value):
        p = self._prop
        if p.setter is None:
            raise TypeError(f"{p.name} は読み取り専用")
        keys = tuple(map(int, index)) if p.dims == 2 else (p.key(index),)
        p.conv.write(self._obj, getattr(lib, p.setter), value, *keys)


class _IndexedSlice:
    """添字が 2 つのプロパティの 1 つ目だけを指定した段階(C++ の Cells[ACol][ARow] の書き方のため)。"""

    __slots__ = ("_proxy", "_index1")

    def __init__(self, proxy, index1):
        self._proxy = proxy
        self._index1 = index1

    def __getitem__(self, index2):
        return self._proxy[self._index1, index2]

    def __setitem__(self, index2, value):
        self._proxy[self._index1, index2] = value


class _Indexed:
    """IndexedProperty / ReadOnlyIndexedProperty / IndexedProperty2。"""

    def __init__(self, getter, setter, conv, key=int, dims=1):
        self.getter = getter
        self.setter = setter
        self.conv = conv
        self.key = key
        self.dims = dims

    def __set_name__(self, owner, name):
        self.name = name

    def __get__(self, obj, objtype=None):
        if obj is None:
            return self
        return _IndexedProxy(obj, self)

    def __set__(self, obj, value):
        raise AttributeError(f"{self.name} には添字を付けて代入する")


def _str_key(v):
    return _enc(v)


# ---------------- イベント ----------------

class _Arg:
    """イベントの引数の変換。n は DLL のコールバックの引数をいくつ使うか。ref はポインタで受け、Ref で渡して書き戻す。"""

    def __init__(self, get, n=1, put=None):
        self.get = get
        self.n = n
        self.put = put


_a_int = _Arg(lambda raws: raws[0])
_a_bool = _Arg(lambda raws: raws[0] != 0)
_a_rect = _Arg(lambda raws: TRect(*raws), n=4)
# 例外(Application.OnException の E)。DLL はクラス名とメッセージを渡す。C++ の beth::Exception と同じく E.Message・E.ClassName() で読む
_a_exception = _Arg(lambda raws: BethError(_dec(raws[0]), _dec(raws[1])), n=2)
# 文字列の配列(OnDropFiles の FileNames)。DLL は数と UTF-8 の文字列の配列を渡す(docs/adr/0047)
_a_strings = _Arg(lambda raws: [_dec(raws[1][i]) for i in range(raws[0])], n=2)


def _a_enum(name):
    return _Arg(lambda raws: _to_enum(name, raws[0]))


def _a_comp(name):
    return _Arg(lambda raws: _to_comp(name, raws[0]))


def _a_item(name):
    return _Arg(lambda raws: _to_item(name, raws[0]))


def _a_obj(name):
    """呼び出しの間だけ有効な非所有のラッパー(TMenuItem の OnDrawItem の ACanvas。docs/adr/0050)。"""
    return _Arg(lambda raws: _to_obj(name, raws[0]))


def _a_ref(from_raw, to_raw):
    return _Arg(lambda raws: Ref(from_raw(raws[0][0])), put=lambda raws, ref: raws[0].__setitem__(0, to_raw(ref.value)))


_a_ref_int = _a_ref(lambda raw: raw, int)
_a_ref_bool = _a_ref(lambda raw: raw != 0, _b)
_a_ref_char = _a_ref(lambda raw: chr(raw & 0xFF), _key)


def _a_ref_enum(name):
    return _a_ref(lambda raw: _to_enum(name, raw), int)


# イベントの型(TKeyEvent 等)→ Sender 以外の引数の変換。beth/__init__.py が設定する。
_event_types = {}


class _Event:
    """Property<T...Event>。最初に None 以外のハンドラが代入されたときだけ DLL にブリッジを登録する(docs/adr/0009)。"""

    def __init__(self, setter, event_type):
        self.setter = setter
        self.event_type = event_type

    def __set_name__(self, owner, name):
        self.name = name
        self.slot = "_on_" + name
        self.hooked = "_hooked_" + name

    def __get__(self, obj, objtype=None):
        if obj is None:
            return self
        return obj.__dict__.get(self.slot)

    def __set__(self, obj, handler):
        if handler is not None and not callable(handler):
            raise TypeError(f"{self.name} には呼び出し可能なもの(または None)を代入する")
        obj.__dict__[self.slot] = handler
        if handler is not None and not obj.__dict__.get(self.hooked):
            getattr(lib, self.setter)(obj._current(), self._trampoline, None)
            obj.__dict__[self.hooked] = True

    def _trampoline(self, sender, *raws):
        # 例外は _internal が SetCallbackError で DLL へ知らせる(docs/adr/0031)。
        self_ = _components.get(sender)
        if self_ is None and self.setter.startswith("TApplication_"):
            # Application のイベント(OnException 等)の Sender は nil のことがある(LCL のタイマーの例外等)
            self_ = _types["TApplication"]._instance
        if self_ is None:
            return
        self._dispatch(self_, raws[:-1])

    def _dispatch(self, obj, raws):
        handler = obj.__dict__.get(self.slot)
        if handler is None:
            return
        args, puts, i = [], [], 0
        for a in _event_types[self.event_type]:
            part = raws[i:i + a.n]
            value = a.get(part)
            args.append(value)
            if a.put is not None:
                puts.append((a, part, value))
            i += a.n
        handler(obj, *args)
        for a, part, value in puts:
            a.put(part, value)


# ---------------- オブジェクト ----------------

class TObject:
    """LCL のオブジェクトのラッパー。Handle は DLL の関数に渡すハンドル。

    ラッパーには 3 つの持ち方がある:
    - 自分のハンドルを持つもの(コンポーネント・項目・利用者が生成したもの・TCanvas 等の非所有のラッパー)
    - 所有者の中身のビュー(ListBox1.Items・Image1.Picture.Bitmap 等)。操作のたびに所有者から中身のハンドルを取得する。
    """

    _handle = None
    _owner = None
    _accessor = None
    _freed = False
    _owns = False
    _destroy = None   # 利用者が生成したものを破棄する DLL の関数名(TStringList・TGraphic・TPicture)

    def __init__(self, *args, **kwargs):
        raise TypeError(f"{type(self).__name__} は直接生成できない")

    def _current(self):
        if self._accessor is not None:
            return self._accessor(self._owner._current())
        if self._freed:
            raise ReferenceError(f"{type(self).__name__} の LCL オブジェクトは破棄済み")
        return self._handle

    @property
    def Handle(self):
        return self._current()

    @classmethod
    def _wrap_handle(cls, handle):
        if not handle:
            return None
        obj = cls.__new__(cls)
        obj._handle = handle
        return obj

    @classmethod
    def _wrap_view(cls, owner, accessor):
        obj = cls.__new__(cls)
        obj._owner = owner
        obj._accessor = accessor
        return obj

    # 利用者が生成するもの(TStringList・TBitmap・TPicture 等)。C++ の delete に当たるのが Free()。
    # 参照が無くなったときにも破棄する。with 文でも使える。
    def _init_owned(self, handle):
        self._handle = handle
        self._owns = True

    def Free(self):
        if self._owns and not self._freed:
            self._freed = True
            getattr(lib, self._destroy)(self._handle)

    def __del__(self):
        if self._owns and not self._freed:
            try:
                self.Free()
            except Exception:
                pass

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.Free()

    def __repr__(self):
        if self._freed:
            state = "freed"
        elif self._accessor is not None:
            state = "view"
        else:
            state = f"0x{self._handle or 0:x}"
        return f"<{type(self).__name__} {state}>"


class TPersistent(TObject):
    pass


# ---------------- コンポーネント(docs/adr/0008) ----------------

# ハンドル → ラッパー。LCL オブジェクトが破棄されると(Free・Owner による連鎖破棄)破棄通知で取り除く。
_components = {}
_free_notify_installed = False


def _on_component_freed(handle, data):
    obj = _components.pop(handle, None)
    if obj is not None:
        obj._freed = True


def _install_free_notify():
    global _free_notify_installed
    if not _free_notify_installed:
        lib.FreeNotify_SetCallback(_on_component_freed, None)
        _free_notify_installed = True


class TComponent(TPersistent):
    """LCL のコンポーネント。ラッパーの寿命は LCL オブジェクトに従い、Free()・Owner による連鎖破棄で LCL オブジェクトが
    破棄されると、そのラッパーは破棄済みになる(触ると ReferenceError)。"""

    # 利用者が自由に使う整数(LCL は解釈しない。ポインタと同じ幅。docs/adr/0034)。
    Tag = _Prop("TComponent_GetTag", "TComponent_SetTag", _int)

    def _attach(self, handle):
        _install_free_notify()
        self._handle = handle
        _components[handle] = self

    @classmethod
    def _wrap_existing(cls, handle):
        if not handle:
            return None
        obj = _components.get(handle)
        if obj is None:
            obj = cls.__new__(cls)
            obj._attach(handle)
        return obj

    def Free(self):
        """LCL オブジェクトを破棄する(このラッパーも破棄済みになる)。"""
        lib.TComponent_Destroy(self._current())


# ---------------- 項目(docs/adr/0019・0020・0026) ----------------

# TComponent ではない項目(TTreeNode・TListItem・TListColumn・THeaderSection・TCoolBand)。
# FreeNotification を持たないため、DLL の項目の破棄通知(ItemFree_SetCallback)で取り除く。
_items = {}
_item_free_installed = False


def _on_item_freed(handle, data):
    obj = _items.pop(handle, None)
    if obj is not None:
        obj._freed = True


class _ItemMixin:
    @classmethod
    def _wrap_item(cls, handle):
        global _item_free_installed
        if not handle:
            return None
        if not _item_free_installed:
            lib.ItemFree_SetCallback(_on_item_freed, None)
            _item_free_installed = True
        obj = _items.get(handle)
        if obj is None:
            obj = cls.__new__(cls)
            obj._handle = handle
            _items[handle] = obj
        return obj


# ---------------- 手書きのメンバ(beth/__init__.py の生成したクラスに混ぜる) ----------------
# gen_api.py は、ここで定義したメンバを生成しない。


class TStrings:
    def AddStrings(self, Source):
        """Source の内容(文字列と Objects)を末尾に加える。None なら何もしない。"""
        if Source is not None:
            lib.TStrings_AddStrings(self._current(), Source._current())

    def __len__(self):
        return self.Count

    def __iter__(self):
        for i in range(self.Count):
            yield self.Strings[i]


class TStringList:
    _destroy = "TStringList_Destroy"


class TGraphic:
    _destroy = "TGraphic_Destroy"

    def Assign(self, Source):
        """Source(グラフィック・Clipboard())の内容で置き換える(別のクラスのグラフィックからは画素を写して変換する)。
        None なら空にする。クリップボードに画像が無ければ何もしない。"""
        lib.TGraphic_Assign(self._current(), _h(Source))


class TPicture:
    _destroy = "TPicture_Destroy"

    # 中身のグラフィック(空なら None)。クラスを問わない TGraphic のビュー。代入は内容のコピー(None なら空にする)。
    @property
    def Graphic(self):
        if not lib.TPicture_GetGraphic(self._current()):
            return None
        return _types["TGraphic"]._wrap_view(self, lib.TPicture_GetGraphic)

    @Graphic.setter
    def Graphic(self, value):
        lib.TPicture_SetGraphic(self._current(), _h(value))

    # 中身をそのクラスとして扱うビュー。操作したとき、中身が別のクラスなら LCL が変換し、空なら空のものを作る。
    # 代入は Graphic と同じく内容のコピー。
    @property
    def Bitmap(self):
        return _types["TBitmap"]._wrap_view(self, lib.TPicture_GetBitmap)

    @Bitmap.setter
    def Bitmap(self, value):
        lib.TPicture_SetGraphic(self._current(), _h(value))

    @property
    def PNG(self):
        return _types["TPortableNetworkGraphic"]._wrap_view(self, lib.TPicture_GetPNG)

    @PNG.setter
    def PNG(self, value):
        lib.TPicture_SetGraphic(self._current(), _h(value))

    @property
    def Jpeg(self):
        return _types["TJPEGImage"]._wrap_view(self, lib.TPicture_GetJpeg)

    @Jpeg.setter
    def Jpeg(self, value):
        lib.TPicture_SetGraphic(self._current(), _h(value))

    @property
    def Icon(self):
        return _types["TIcon"]._wrap_view(self, lib.TPicture_GetIcon)

    @Icon.setter
    def Icon(self, value):
        lib.TPicture_SetIcon(self._current(), _h(value))

    def Assign(self, Source):
        """Source(TPicture・Clipboard())の内容で置き換える。None なら空にする。クリップボードに画像が無ければ何もしない。"""
        lib.TPicture_Assign(self._current(), _h(Source))


class TClipboard:
    def Assign(self, Source):
        """画像(TPicture・グラフィック)をクリップボードに置く(内容を置き換える)。None なら何もしない。"""
        if Source is not None:
            lib.TClipboard_Assign(self._current(), Source._current())


class TCanvas:
    def Draw(self, X, Y, Graphic):
        """グラフィックを描く。Graphic が None なら何もしない。"""
        if Graphic is not None:
            lib.TCanvas_Draw(self._current(), int(X), int(Y), Graphic._current())

    def StretchDraw(self, Rect, Graphic):
        """Rect に合わせて伸縮して描く。Graphic が None なら何もしない。"""
        if Graphic is not None:
            lib.TCanvas_StretchDraw(self._current(), *_rect(Rect), Graphic._current())

    def Polygon(self, Points):
        """点を結んだ多角形(Pen で縁を、Brush で中を描く)。Points は TPoint か (x, y) の並び。"""
        flat, count = _points(Points)
        if count > 0:
            lib.TCanvas_Polygon(self._current(), flat, count)

    def Polyline(self, Points):
        """点を結んだ折れ線(Pen だけで描く)。Points は TPoint か (x, y) の並び。"""
        flat, count = _points(Points)
        if count > 0:
            lib.TCanvas_Polyline(self._current(), flat, count)

    def CopyRect(self, Dest, Canvas, Source):
        """Canvas の Source の範囲を、この Canvas の Dest に写す(大きさが違えば伸縮する)。Canvas が None なら何もしない。"""
        if Canvas is not None:
            lib.TCanvas_CopyRect(self._current(), *_rect(Dest), Canvas._current(), *_rect(Source))


def _points(points):
    """点の並びを DLL に渡す形((X, Y) を並べた整数の配列と点の数)にする。"""
    coords = []
    for p in points:
        if isinstance(p, TPoint):
            coords += [int(p.X), int(p.Y)]
        else:
            x, y = p
            coords += [int(x), int(y)]
    count = len(coords) // 2
    return (ctypes.c_int * len(coords))(*coords), count


class _FormCreateEvent:
    """TCustomForm.OnCreate。DLL のイベントではなく、DoCreate で呼ぶ。"""

    def __set_name__(self, owner, name):
        self.name = name

    def __get__(self, obj, objtype=None):
        if obj is None:
            return self
        return obj.__dict__.get("_on_OnCreate")

    def __set__(self, obj, handler):
        obj.__dict__["_on_OnCreate"] = handler


class _FormShowEvent(_Event):
    """TCustomForm.OnShow。フォームの生成時に常にブリッジを登録し、最初の表示の直前に OnCreate を呼ぶ。"""

    def __set__(self, obj, handler):
        if handler is not None and not callable(handler):
            raise TypeError("OnShow には呼び出し可能なもの(または None)を代入する")
        obj.__dict__[self.slot] = handler

    def _dispatch(self, obj, raws):
        obj._DoCreate()
        super()._dispatch(obj, raws)


_form_show = _FormShowEvent("TCustomForm_SetOnShow", "TNotifyEvent")


class TCustomForm:
    # フォームの生成が完了したときに 1 度だけ呼ばれる。
    # Application.CreateForm で生成した場合は、派生クラスの __init__ の完了直後。
    # 直接生成した場合は、完了を検知できないため、最初に表示される直前になる。どちらの場合も __init__ の中で設定すればよい。
    OnCreate = _FormCreateEvent()
    OnShow = _form_show

    def _attach(self, handle):
        TComponent._attach(self, handle)
        # OnShow のブリッジは常に登録する。直接生成したフォームの OnCreate を、最初の表示の直前に呼ぶため。
        lib.TCustomForm_SetOnShow(handle, _form_show._trampoline, None)

    def _DoCreate(self):
        if self.__dict__.get("_created"):
            return
        self.__dict__["_created"] = True
        handler = self.OnCreate
        if handler is not None:
            handler(self)


# Application.CreateForm が LCL の CreateForm で生成済みの、TForm の __init__ に引き取られるのを待つハンドル。
_pending_form = None


class TForm:
    def __init__(self, AOwner):
        global _pending_form
        if _pending_form and AOwner is not None and AOwner is _types["TApplication"]._instance:
            handle, _pending_form = _pending_form, None
        else:
            handle = lib.TForm_Create(_h(AOwner))
        self._attach(handle)


class TApplication:
    """C++Builder の TApplication。インスタンスは beth.Application の 1 つだけ。
    Application が所有するフォームは、プログラムの終了時(atexit)にまとめて破棄される。"""

    _instance = None

    @classmethod
    def _global(cls):
        app = cls._wrap_existing(lib.GetApplication())
        cls._instance = app
        atexit.register(_shutdown)
        return app

    def Initialize(self):
        """LCL 側は DLL の読み込み時に初期化済みのため何もしない(C++Builder のコードとの互換のために置く)。"""

    def MessageBox(self, Text, Caption, Flags=0):
        """Windows のメッセージボックス(LCL の TApplication.MessageBox)。Flags は MB_…(MB_YESNO | MB_ICONQUESTION 等)、
        戻り値は ID…(IDYES 等)。docs/adr/0041。"""
        return lib.TApplication_MessageBox(self._handle, _enc(Text), _enc(Caption), int(Flags))

    def CreateForm(self, FormClass):
        """C++Builder の Application->CreateForm(__classid(TForm1), &Form1) に相当する: Form1 = Application.CreateForm(TForm1)。
        FormClass は TForm の派生で、(AOwner) を受け取って TForm.__init__(AOwner) を呼ぶこと。生成したフォームを返す。"""
        global _pending_form
        if not (isinstance(FormClass, type) and issubclass(FormClass, _types["TForm"])):
            raise TypeError("CreateForm can only create classes derived from TForm")
        if _pending_form:
            lib.TComponent_Destroy(_pending_form)
        _pending_form = lib.TApplication_CreateForm(self._current())
        try:
            form = FormClass(self)
        finally:
            if _pending_form:
                lib.TComponent_Destroy(_pending_form)
                _pending_form = None
        # C++Builder では OnCreate は最派生クラスのコンストラクタの完了後(AfterConstruction)に呼ばれる。
        form._DoCreate()
        return form


def _shutdown():
    # Python の終了時(atexit)に、Application が所有するフォームを破棄する(破棄通知でラッパーも破棄済みになる)。
    app = _types["TApplication"]._instance
    if app is not None:
        lib.TComponent_DestroyComponents(app._handle)
    lib.FreeNotify_SetCallback(None, None)
    lib.ItemFree_SetCallback(None, None)


# ---------------- メッセージのダイアログ(docs/adr/0041。LCL の Dialogs ユニットの関数) ----------------

# Application.MessageBox の Flags と戻り値(Windows の値)。
MB_OK = 0x00
MB_OKCANCEL = 0x01
MB_ABORTRETRYIGNORE = 0x02
MB_YESNOCANCEL = 0x03
MB_YESNO = 0x04
MB_RETRYCANCEL = 0x05
MB_ICONERROR = 0x10
MB_ICONQUESTION = 0x20
MB_ICONWARNING = 0x30
MB_ICONINFORMATION = 0x40
MB_DEFBUTTON1 = 0x000
MB_DEFBUTTON2 = 0x100
MB_DEFBUTTON3 = 0x200
IDOK = 1
IDCANCEL = 2
IDABORT = 3
IDRETRY = 4
IDIGNORE = 5
IDYES = 6
IDNO = 7


def ShowMessage(Msg):
    """メッセージと OK のボタンだけのダイアログ。"""
    lib.Dialogs_ShowMessage(_enc(Msg))


def MessageDlg(*args):
    """ボタンを選ぶダイアログ。押したボタンの ModalResult(mbYes なら mrYes)を返す。
    MessageDlg(Msg, DlgType, Buttons, HelpCtx=0) か、題名を付ける MessageDlg(Caption, Msg, DlgType, Buttons, HelpCtx=0)。
    Buttons は TMsgDlgBtn の集まり({mbYes, mbNo} や定数の mbYesNo)。"""
    if len(args) >= 2 and isinstance(args[1], str):
        caption, msg, dlg_type, buttons, *rest = args
    else:
        caption = ""
        msg, dlg_type, buttons, *rest = args
    help_ctx = rest[0] if rest else 0
    bits = sum(1 << int(b) for b in set(buttons))
    return lib.Dialogs_MessageDlg(_enc(caption), _enc(msg), int(dlg_type), bits, int(help_ctx))


def InputBox(ACaption, APrompt, ADefault):
    """1 行の文字列を入力するダイアログ。取りやめたら ADefault を返す。"""
    return _dec(lib.Dialogs_InputBox(_enc(ACaption), _enc(APrompt), _enc(ADefault)))


def PasswordBox(ACaption, APrompt):
    """入力した文字を隠すダイアログ。取りやめたら空文字列を返す。"""
    return _dec(lib.Dialogs_PasswordBox(_enc(ACaption), _enc(APrompt)))


def InputQuery(ACaption, APrompt, Value):
    """1 行の文字列を入力するダイアログ。Value は Ref(C++ の std::string&)で、OK なら Value.value を入力した文字列にして True、
    取りやめたら Value.value のままで False を返す。"""
    ok = ctypes.c_int(0)
    result = _dec(lib.Dialogs_InputQuery(_enc(ACaption), _enc(APrompt), _enc(Value.value or ""), ctypes.byref(ok)))
    if not ok.value:
        return False
    Value.value = result
    return True


def ShortCut(Key, Shift):
    """VCL の Menus ユニットの ShortCut。Shift のうち ssShift/ssCtrl/ssAlt 以外は無視される。"""
    return lib.ShortCut_Make(int(Key), int(Shift))


def TextToShortCut(Text):
    """"Ctrl+S" のような文字列をショートカットキーにする。解釈できない文字列は 0 になる。"""
    return lib.ShortCut_FromText(_enc(Text))


def ShortCutToText(ShortCut):
    return _dec(lib.ShortCut_ToText(int(ShortCut)))


_clipboard = None


def Clipboard():
    """クリップボード(C++Builder と同じく関数。docs/adr/0047)。LCL が持つ 1 つだけで、破棄しない。"""
    global _clipboard
    if _clipboard is None:
        _clipboard = _types["TClipboard"]._wrap_handle(lib.GetClipboard())
    return _clipboard


def CF_Text():
    """クリップボードの文字列の形式(Windows では CF_UNICODETEXT)。"""
    return lib.Clipboard_CF_Text()


def CF_Bitmap():
    """クリップボードのビットマップの形式(Windows の CF_BITMAP)。"""
    return lib.Clipboard_CF_Bitmap()


def CF_Picture():
    """LCL が読み込める画像の形式のどれか(HasFormat(CF_Picture()) は HasPictureFormat() と同じ)。"""
    return lib.Clipboard_CF_Picture()


# 生成したクラスに混ぜる手書きのメンバ(クラス名 → クラス)。
_mixins = {
    "TStrings": TStrings,
    "TStringList": TStringList,
    "TGraphic": TGraphic,
    "TPicture": TPicture,
    "TClipboard": TClipboard,
    "TCanvas": TCanvas,
    "TCustomForm": TCustomForm,
    "TForm": TForm,
    "TApplication": TApplication,
}
del TStrings, TStringList, TGraphic, TPicture, TClipboard, TCanvas, TCustomForm, TForm, TApplication
