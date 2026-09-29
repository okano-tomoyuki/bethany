"""beth.hpp(C++ の公開 API)と internal/funcs.h・internal/api.h から、Python の公開 API(beth/__init__.py)を生成する。

beth.hpp のクラス・プロパティ・イベント・メソッド・列挙型・定数を、命名規則(docs/adr/0007。TControl::Left なら
TControl_GetLeft・TControl_SetLeft)で DLL の関数に対応付ける。寿命の管理や C++ で独自の実装を持つメンバは
beth/_core.py に手で書き(_mixins)、ここでは生成しない。
対応付けられないメンバがあれば、生成を止めて一覧を表示する(黙って落とさない)。
"""
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
HPP = ROOT / "include" / "bethany" / "beth.hpp"
CPP = ROOT / "src" / "beth.cpp"
FUNCS_HEADER = ROOT / "include" / "bethany" / "internal" / "funcs.h"
API_HEADER = ROOT / "include" / "bethany" / "internal" / "api.h"
CORE_FILE = HERE / "beth" / "_core.py"
OUTPUT_FILE = HERE / "beth" / "__init__.py"

# Application.MessageBox の Flags と戻り値(Windows の MB_…・ID…。C++ は <windows.h> の定数を使う。beth/_core.py で定義する)。
MESSAGE_BOX_CONSTANTS = [
    "MB_OK", "MB_OKCANCEL", "MB_ABORTRETRYIGNORE", "MB_YESNOCANCEL", "MB_YESNO", "MB_RETRYCANCEL",
    "MB_ICONERROR", "MB_ICONQUESTION", "MB_ICONWARNING", "MB_ICONINFORMATION",
    "MB_DEFBUTTON1", "MB_DEFBUTTON2", "MB_DEFBUTTON3",
    "IDOK", "IDCANCEL", "IDABORT", "IDRETRY", "IDIGNORE", "IDYES", "IDNO",
]

# 手書き(beth/_core.py)のクラスと、C++ の実装のための補助クラス(生成しない)。
CORE_CLASSES = {"TObject", "TPersistent", "TComponent"}
HELPER_CLASSES = {"Property", "ReadOnlyProperty", "IndexedProperty", "ReadOnlyIndexedProperty", "IndexedProperty2",
                  "ItemRegistry", "CanvasHolder", "Exception"}


# ---------------- C++ の文の分割 ----------------

class Stmt:
    def __init__(self, text, body, comments):
        self.text = " ".join(text.split())
        self.body = body
        self.comments = comments


def split_statements(src):
    """深さ 0 の文に分ける。文は text(本体の {} より前)・body({} の中身。無ければ None)・comments(直前と行末のコメント)。"""
    out = []
    i, n = 0, len(src)
    text, comments = [], []
    while i < n:
        c = src[i]
        if src.startswith("//", i):
            j = src.find("\n", i)
            j = n if j < 0 else j
            comments.append(src[i + 2:j].strip())
            i = j
            continue
        if src.startswith("/*", i):
            j = src.find("*/", i)
            body = src[i + 2:j]
            comments.extend(line.strip().lstrip("*").strip() for line in body.splitlines())
            i = j + 2
            continue
        if c == "\n" and not "".join(text).strip():
            # 空行を挟んだコメントは、次の文のものではない
            if i + 1 < n and src[i + 1] == "\n":
                comments = []
            i += 1
            continue
        if c == "{":
            j = match_brace(src, i)
            body = src[i + 1:j]
            i = j + 1
            k = i
            while k < n and src[k] in " \t":
                k += 1
            trailing = src[i:k]
            if k < n and src[k] == ";":
                i = k + 1
            elif re.match(r"\s*\w", src[i:i + 40]) and re.match(r"\s*\w+\s*;", src[i:i + 80]):
                # enum { ... } name; の形は無いが念のため
                pass
            out.append(Stmt("".join(text), body, comments))
            text, comments = [], []
            continue
        if c == ";":
            # 行末のコメントもこの文のもの
            j = i + 1
            while j < n and src[j] in " \t":
                j += 1
            if src.startswith("//", j):
                k = src.find("\n", j)
                k = n if k < 0 else k
                comments.append(src[j + 2:k].strip())
                j = k
            else:
                j = i + 1
            out.append(Stmt("".join(text), None, comments))
            text, comments = [], []
            i = j
            continue
        if c == ":" and re.fullmatch(r"\s*(public|protected|private)\s*", "".join(text)):
            out.append(Stmt("".join(text) + ":", None, []))
            text = []
            i += 1
            continue
        text.append(c)
        i += 1
    return out


def match_brace(src, i):
    depth = 0
    n = len(src)
    while i < n:
        c = src[i]
        if src.startswith("//", i):
            i = src.find("\n", i)
            continue
        if src.startswith("/*", i):
            i = src.find("*/", i) + 2
            continue
        if c in "\"'":
            j = i + 1
            while src[j] != c:
                j += 2 if src[j] == "\\" else 1
            i = j + 1
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("括弧が閉じていない")


# ---------------- 読み込み ----------------

def read_funcs():
    text = FUNCS_HEADER.read_text(encoding="utf-8")
    funcs = {}
    for m in re.finditer(r"X\(\s*([^,]+?)\s*,\s*(\w+)\s*,\s*\(([^)]*)\)", text):
        params = m.group(3).strip()
        types = []
        if params and params != "void":
            for p in params.split(","):
                pm = re.fullmatch(r"(.+?)\s*\b\w+", p.strip())
                types.append(pm.group(1).replace(" ", ""))
        funcs[m.group(2)] = (m.group(1).strip(), types)
    return funcs


def read_callbacks():
    text = API_HEADER.read_text(encoding="utf-8")
    cbs = {}
    for m in re.finditer(r"using\s+(\w*callback_t)\s*=\s*void\s*\(\s*BETH_CALL\s*\*\s*\)\s*\(([^)]*)\)\s*;", text):
        cbs[m.group(1)] = len([p for p in m.group(2).split(",") if p.strip()])
    return cbs


class Model:
    def __init__(self):
        self.enums = {}        # 名前 → [(要素, 値)]
        self.enum_comments = {}
        self.flags = {}        # 名前 → [(要素, 値の式, コメント)]
        self.sets = {}         # Set<E> の別名(TAnchors 等) → 要素の列挙型
        self.int_aliases = {}  # TColor・TShortCut 等 → 定数 [(名前, 値の式, コメント)]
        self.events = {}       # 名前 → [引数の型](Sender を除く)
        self.event_aliases = {}
        self.classes = {}      # 名前 → (基底, 本体, コメント)
        self.set_constants = []  # 集合の定数 [(名前, 集合の型, [要素])](mbYesNo 等。beth.cpp の定義から)
        self.class_order = []


def cxx_value(v):
    v = v.strip()
    v = re.sub(r"(\d)[uU]\b", r"\1", v)
    return v


def parse_hpp():
    src = HPP.read_text(encoding="utf-8")
    ns = re.search(r"namespace beth\s*\{", src)
    body = src[ns.end():match_brace(src, ns.end() - 1)]
    model = Model()
    type_of_const = {}
    for st in split_statements(body):
        t = st.text
        m = re.fullmatch(r"enum (\w+)", t)
        if m and st.body is not None:
            items, value = [], 0
            for part in split_statements(st.body.replace(",", ";") + ";"):
                pt = part.text.strip()
                if not pt:
                    continue
                em = re.fullmatch(r"(\w+)(?:\s*=\s*(.+))?", pt)
                if em.group(2) is not None:
                    value = int(cxx_value(em.group(2)), 0)
                items.append((em.group(1), value, part.comments))
                value += 1
            model.enums[m.group(1)] = items
            model.enum_comments[m.group(1)] = st.comments
            continue
        m = re.fullmatch(r"(class|struct) (\w+)(?:\s*:\s*public (\w+))?", t)
        if m and st.body is not None:
            if m.group(1) == "class":
                model.classes[m.group(2)] = (m.group(3), st.body, st.comments)
                model.class_order.append(m.group(2))
            continue
        m = re.fullmatch(r"using (\w+) = std::function<void\((.*)\)>", t)
        if m:
            model.events[m.group(1)] = split_params(m.group(2))[1:]
            continue
        m = re.fullmatch(r"using (\w+) = (\w+)", t)
        if m and (m.group(2) in model.events):
            model.event_aliases[m.group(1)] = m.group(2)
            continue
        m = re.fullmatch(r"using (\w+) = Set<(\w+)>", t)
        if m:
            model.sets[m.group(1)] = m.group(2)
            continue
        m = re.fullmatch(r"using (\w+) = unsigned int", t)
        if m:
            model.flags[m.group(1)] = []
            continue
        m = re.fullmatch(r"using (\w+) = (?:std::int32_t|unsigned short)", t)
        if m:
            model.int_aliases[m.group(1)] = []
            continue
        m = re.fullmatch(r"const (\w+) (\w+) = (.+)", t)
        if m:
            target = model.flags.get(m.group(1), model.int_aliases.get(m.group(1)))
            target.append((m.group(2), cxx_value(m.group(3)), st.comments))
            continue
    # 集合の定数(beth.hpp では extern const で宣言し、beth.cpp で const TMsgDlgButtons mbYesNo = TMsgDlgButtons() << mbYes << mbNo; と定義する)
    cpp = CPP.read_text(encoding="utf-8")
    for m in re.finditer(r"^const (\w+) (\w+)\s*=\s*\1\(\)((?:\s*<<\s*\w+)+);", cpp, re.M):
        if m.group(1) in model.sets:
            model.set_constants.append((m.group(2), m.group(1), re.findall(r"\w+", m.group(3))))
    return model


def split_params(params):
    params = params.strip()
    if not params or params == "void":
        return []
    out, depth, cur = [], 0, ""
    for c in params:
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        if c == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += c
    out.append(cur.strip())
    return out


def param_parts(p):
    """「const std::string& S」「bool Enabled = true」→ (型, 名前, 既定値)。名前の無い型だけのもの(イベントの引数)は名前が None。"""
    default = None
    if "=" in p:
        p, default = p.split("=", 1)
        default = default.strip()
    p = p.strip()
    m = re.fullmatch(r"(.*?[\w*&>])\s+(\w+)", p)
    if m and m.group(1) not in ("const", "unsigned"):
        return m.group(1).replace(" *", "*").replace(" &", "&"), m.group(2), default
    return p, None, default


# ---------------- 型の分類 ----------------

class Gen:
    def __init__(self, model, funcs, callbacks, mixins):
        self.m = model
        self.funcs = funcs
        self.callbacks = callbacks
        self.mixins = mixins
        self.errors = []
        self.base = {name: b for name, (b, _, _) in model.classes.items()}
        self.item_classes = {n for n, (_, body, _) in model.classes.items() if "ItemRegistry::Wrap<" in body}
        self.existing_classes = {n for n, (_, body, _) in model.classes.items() if "friend class TComponent;" in body}

    def derives(self, name, root):
        while name:
            if name == root:
                return True
            name = self.base.get(name)
        return False

    def is_class(self, name):
        return name in self.m.classes

    def scalar(self, t):
        """スカラーの型 → 'int'・'bool'・'float'・'char'・'str'・'ptr'・('enum', 名前)。クラス等なら None。"""
        t = t.replace("const ", "").strip()
        if t in ("int", "unsigned short", "unsigned int", "std::intptr_t") or t in self.m.int_aliases:
            return "int"
        if t == "bool":
            return "bool"
        if t == "double":
            return "float"
        if t == "char":
            return "char"
        if t == "std::string":
            return "str"
        if t == "void*":
            return "ptr"
        if t in self.m.enums or t in self.m.flags:
            return ("enum", t)
        return None

    def class_kind(self, name):
        if name in self.item_classes:
            return "item"
        if name in self.existing_classes:
            return "existing"
        if self.derives(name, "TComponent") or name == "TComponent":
            return "comp"
        if self.derives(name, "TStrings") or self.derives(name, "TGraphic") or name in ("TStrings", "TGraphic"):
            return "view"
        return "obj"

    def conv(self, t):
        """プロパティ・添字の値の変換(_core の _Conv)の式。"""
        if t in self.m.sets:
            return f'_set("{self.m.sets[t]}")'
        s = self.scalar(t)
        if s is not None:
            if isinstance(s, tuple):
                return f'_enum("{s[1]}")'
            return {"int": "_int", "bool": "_bool", "float": "_float", "char": "_char", "str": "_str", "ptr": "_ptr"}[s]
        if t in ("TRect", "TGridRect"):
            return "_rect_conv"
        if t == "TPoint":
            return "_point_conv"
        m = re.fullmatch(r"(?:const )?(\w+)\*", t)
        if m and self.is_class(m.group(1)) or (m and m.group(1) in CORE_CLASSES):
            kind = self.class_kind(m.group(1))
            return {"item": "_item", "existing": "_existing", "comp": "_comp", "view": "_view", "obj": "_obj"}[kind] + f'("{m.group(1)}")'
        return None

    def from_raw(self, t, raw):
        """メソッドの戻り値の変換の式。"""
        s = self.scalar(t)
        if s == "int" or s == "ptr":
            return raw
        if s == "bool":
            return f"{raw} != 0"
        if s == "str":
            return f"_dec({raw})"
        if isinstance(s, tuple):
            return f'_to_enum("{s[1]}", {raw})'
        m = re.fullmatch(r"(?:const )?(\w+)\*", t)
        if m:
            kind = self.class_kind(m.group(1))
            fn = {"item": "_to_item", "existing": "_to_existing", "comp": "_to_comp", "obj": "_to_obj"}.get(kind)
            if fn:
                return f'{fn}("{m.group(1)}", {raw})'
        return None

    def event_arg(self, t):
        ref = t.endswith("&")
        base = t[:-1].strip() if ref else t
        s = self.scalar(base)
        if ref:
            if s == "int":
                return "_a_ref_int"
            if s == "bool":
                return "_a_ref_bool"
            if s == "char":
                return "_a_ref_char"
            if isinstance(s, tuple):
                return f'_a_ref_enum("{s[1]}")'
            return None
        if s == "int":
            return "_a_int"
        if s == "bool":
            return "_a_bool"
        if isinstance(s, tuple):
            return f'_a_enum("{s[1]}")'
        if base in ("TRect", "TGridRect"):
            return "_a_rect"
        m = re.fullmatch(r"(\w+)\*", base)
        if m:
            kind = self.class_kind(m.group(1))
            if kind == "item":
                return f'_a_item("{m.group(1)}")'
            return f'_a_comp("{m.group(1)}")'
        return None

    # ---------------- 生成 ----------------

    def err(self, msg):
        self.errors.append(msg)

    def generate(self):
        out = [HEADER]
        out += self.gen_enums()
        out += self.gen_flags()
        for name in self.m.class_order:
            if name in CORE_CLASSES or name in HELPER_CLASSES:
                continue
            out += self.gen_class(name)
        out += self.gen_events()
        out.append(FOOTER.format(names=self.public_names()))
        return "\n".join(out)

    def public_names(self):
        names = ["BethError", "Ref", "TRect", "TPoint", "TObject", "TPersistent", "TComponent",
                 "ShortCut", "TextToShortCut", "ShortCutToText", "Application",
                 "ShowMessage", "MessageDlg", "InputBox", "PasswordBox", "InputQuery"]
        names += MESSAGE_BOX_CONSTANTS
        for e, items in self.m.enums.items():
            names.append(e)
            names += [i for i, _, _ in items]
        for f, items in self.m.flags.items():
            names.append(f)
            names += [i for i, _, _ in items]
        for a, items in self.m.int_aliases.items():
            names.append(a)
            names += [i for i, _, _ in items]
        names += [n for n, _, _ in self.m.set_constants]
        names += [c for c in self.m.class_order if c not in CORE_CLASSES and c not in HELPER_CLASSES]
        lines, cur = [], "    "
        for n in names:
            item = f'"{n}", '
            if len(cur) + len(item) > 116:
                lines.append(cur.rstrip())
                cur = "    "
            cur += item
        lines.append(cur.rstrip())
        return "\n".join(lines)

    def gen_enums(self):
        out = ["", "# ---------------- 列挙型 ----------------", ""]
        for name, items in self.m.enums.items():
            out += [f"# {c}" for c in self.m.enum_comments[name] if c]
            out.append(f"class {name}(enum.IntEnum):")
            for item, value, comments in items:
                trailing = f"  # {' '.join(comments)}" if comments else ""
                out.append(f"    {item} = {value}{trailing}")
            out.append("")
            out.append(", ".join(i for i, _, _ in items) + " = " + ", ".join(f"{name}.{i}" for i, _, _ in items))
            out.append("")
            out.append("")
        return out

    def gen_flags(self):
        out = ["# ---------------- 集合・定数 ----------------", ""]
        for name, items in self.m.flags.items():
            out.append(f"class {name}(enum.IntFlag):")
            for item, value, comments in items:
                trailing = f"  # {' '.join(comments)}" if comments else ""
                out.append(f"    {item} = {value}{trailing}")
            out.append("")
            out.append("")
            for item, _, _ in items:
                out.append(f"{item} = {name}.{item}")
            out.append("")
            out.append("")
        for name, items in self.m.int_aliases.items():
            out.append(f"{name} = int")
            for item, value, comments in items:
                trailing = f"  # {' '.join(comments)}" if comments else ""
                out.append(f"{item} = {value}{trailing}")
            out.append("")
        for name, set_type, items in self.m.set_constants:
            out.append(f"{name} = frozenset({{{', '.join(items)}}})  # {set_type}")
        if self.m.set_constants:
            out.append("")
        out.append("")
        return out

    def gen_class(self, name):
        base, body, comments = self.m.classes[name]
        mixin = self.mixins.get(name)
        bases = []
        if mixin is not None:
            bases.append(f'_mixins["{name}"]')
        bases.append(base or "TObject")
        if name in self.item_classes:
            bases.append("_ItemMixin")
        out = [f"class {name}({', '.join(bases)}):"]
        doc = [c for c in comments if c]
        if doc:
            out.append('    """' + "\n    ".join(doc) + '"""')
        members = []
        section = "private"
        for st in split_statements(body):
            t = st.text
            sm = re.fullmatch(r"(public|protected|private)\s*:", t)
            if sm:
                section = sm.group(1)
                continue
            if section == "private" or not t:
                continue
            lines = self.gen_member(name, section, st, mixin)
            if lines:
                if st.comments and any(st.comments):
                    members += [f"    # {c}" if c else "    #" for c in st.comments]
                members += lines
        if not members and not doc:
            members.append("    pass")
        out += members
        out += ["", ""]
        return out

    def owns_member(self, mixin, name):
        return mixin is not None and name in mixin.__dict__

    def gen_member(self, cls, section, st, mixin):
        t = st.text
        if st.body is not None:
            # 本体を持つ(C++ の中だけの実装・テンプレート)。Python では手書き(_mixins)にする。
            m = re.search(r"(\w+)\s*\(", t)
            name = m.group(1) if m else t
            if section == "public" and not self.owns_member(mixin, name) and name not in ("Current", "Handle"):
                self.err(f"{cls}: 本体を持つメンバ {t!r} は _mixins に書く")
            return None
        if t.startswith(("friend ", "static ", "virtual ", "template")) or "~" in t or "= default" in t or "= delete" in t:
            return None
        m = re.fullmatch(r"using (\w+)::(\w+)", t)
        if m:
            return [f"    {m.group(2)} = {m.group(1)}._{m.group(2)}"] if section == "public" else None
        if re.fullmatch(r"using \w+ = .*", t):
            return None
        m = re.fullmatch(r"(ReadOnly)?(Indexed)?Property(2)?<(.+)>\s+(\w+)", t)
        if m:
            name = m.group(5)
            pyname = name if section == "public" else "_" + name
            if self.owns_member(mixin, name):
                return None
            return self.gen_property(cls, pyname, name, m.group(4), bool(m.group(1)), bool(m.group(2)), bool(m.group(3)))
        m = re.fullmatch(r"(?:explicit )?(\w+)\((.*)\)", t)
        if m and m.group(1) == cls:
            if section != "public" or self.owns_member(mixin, "__init__"):
                return None
            return self.gen_ctor(cls, split_params(m.group(2)))
        m = re.fullmatch(r"(\w+) (\w+)", t)
        if m and self.is_class(m.group(1)):
            # 値メンバ(TCanvas::Pen・TPaintBox::Canvas 等)。所有者の中身への非所有のラッパー。
            if self.owns_member(mixin, m.group(2)):
                return None
            getter = f"{cls}_Get{m.group(2)}"
            if getter not in self.funcs:
                self.err(f"{cls}.{m.group(2)}: {getter} が無い")
                return None
            return [f'    {m.group(2)} = _Prop("{getter}", None, {self.conv(m.group(1) + "*")})']
        m = re.fullmatch(r"(.+?)\s*\b(\w+)\((.*)\)\s*(const)?", t)
        if m:
            if section != "public":
                self.err(f"{cls}: protected のメソッド {t!r}")
                return None
            if self.owns_member(mixin, m.group(2)):
                return None
            return self.gen_method(cls, m.group(1).strip(), m.group(2), split_params(m.group(3)))
        self.err(f"{cls}: 解釈できないメンバ {t!r}")
        return None

    def gen_property(self, cls, pyname, name, targs, readonly, indexed, two):
        if targs.startswith("T") and (targs in self.m.events or targs in self.m.event_aliases):
            setter = f"{cls}_Set{name}"
            if setter not in self.funcs:
                self.err(f"{cls}.{name}: {setter} が無い")
                return None
            ev = self.m.event_aliases.get(targs, targs)
            self.check_event(cls, name, ev, setter)
            return [f'    {pyname} = _Event("{setter}", "{ev}")']
        index_key = "int"
        if indexed and "," in targs:
            targs, index_key = [x.strip() for x in targs.split(",", 1)]
        conv = self.conv(targs)
        if conv is None:
            self.err(f"{cls}.{name}: 型 {targs} を変換できない")
            return None
        getter = f"{cls}_Get{name}"
        if getter not in self.funcs and (indexed or two):
            # Items → GetItem(LCL の添字付きのプロパティの読み出し関数は単数形)
            for singular in (name[:-1], name[:-2] if name.endswith("es") else None):
                if singular and f"{cls}_Get{singular}" in self.funcs:
                    getter = f"{cls}_Get{singular}"
                    break
        if getter not in self.funcs:
            self.err(f"{cls}.{name}: {getter} が無い")
            return None
        setter = None
        if not readonly:
            setter = f"{cls}_Set{name}"
            if setter not in self.funcs:
                self.err(f"{cls}.{name}: {setter} が無い(読み書きできるプロパティ)")
                return None
        setter_s = f'"{setter}"' if setter else "None"
        if two:
            return [f'    {pyname} = _Indexed("{getter}", {setter_s}, {conv}, dims=2)']
        if indexed:
            key = ", key=_str_key" if index_key == "std::string" else ""
            return [f'    {pyname} = _Indexed("{getter}", {setter_s}, {conv}{key})']
        return [f'    {pyname} = _Prop("{getter}", {setter_s}, {conv})']

    def check_event(self, cls, name, ev, setter):
        cb = self.funcs[setter][1][1]
        raw = self.callbacks[cb] - 2
        n = 0
        for t in self.m.events[ev]:
            a = self.event_arg(param_parts(t)[0])
            if a is None:
                self.err(f"{cls}.{name}: イベントの引数 {t} を変換できない")
                return
            n += 4 if a == "_a_rect" else 1
        if n != raw:
            self.err(f"{cls}.{name}: {ev} の引数({n})と {cb} の引数({raw})が合わない")

    def gen_ctor(self, cls, params):
        create = f"{cls}_Create"
        if any(p.startswith(("ObjectHandle", "TObject* owner")) for p in params):
            return None
        if create not in self.funcs:
            self.err(f"{cls}: {create} が無い")
            return None
        if params == ["TComponent* AOwner"]:
            return ["    def __init__(self, AOwner):", f"        self._attach(lib.{create}(_h(AOwner)))"]
        if not params:
            return ["    def __init__(self):", f"        self._init_owned(lib.{create}())"]
        if any(p.startswith(("ObjectHandle", "TObject* owner")) for p in params):
            # ハンドル・所有者の中身から作るラッパー(C++ の内部用)。Python では _wrap_handle・_wrap_view で作る。
            return None
        self.err(f"{cls}: コンストラクタ {params}")
        return None

    def gen_method(self, cls, ret, name, params):
        fname = f"{cls}_{name}"
        if fname not in self.funcs:
            self.err(f"{cls}.{name}: {fname} が無い")
            return None
        fret, ftypes = self.funcs[fname]
        ftypes = ftypes[1:]  # self
        sig, args, pre, post, expect = ["self"], [], [], [], []
        for p in params:
            t, pname, default = param_parts(p)
            if default is not None:
                default = {"true": "True", "false": "False", "nullptr": "None"}.get(default, default)
                sig.append(f"{pname}={default}")
            else:
                sig.append(pname)
            s = self.scalar(t.rstrip("&").strip()) if t.endswith("&") and not t.startswith("const") else None
            if t.endswith("&") and not t.startswith("const"):
                # 出力引数(int& Index 等)。Ref を受け取り、DLL が書き込んだ値を入れる。
                tmp = f"_out_{pname}"
                pre.append(f"        {tmp} = ctypes.c_int()")
                args.append(f"ctypes.byref({tmp})")
                post.append(f"        {pname}.value = {tmp}.value" + (" != 0" if s == "bool" else ""))
                expect.append("*")
                continue
            base = t.replace("const ", "").rstrip("&").strip()
            if base == "TRect":
                args.append(f"*_rect({pname})")
                expect += ["int_t"] * 4
                continue
            if base == "TPoint":
                args.append(f"*_point({pname})")
                expect += ["int_t"] * 2
                continue
            sc = self.scalar(base)
            if sc == "str":
                args.append(f"_enc({pname})")
                expect.append("str_t")
            elif sc == "bool":
                args.append(f"_b({pname})")
                expect.append("bool_t")
            elif sc == "int" or isinstance(sc, tuple):
                args.append(f"int({pname})")
                expect.append("int_t")
            elif sc == "ptr":
                args.append(pname)
                expect.append("void*")
            elif re.fullmatch(r"\w+\*", base):
                args.append(f"_h({pname})")
                expect.append("obj_t")
            else:
                self.err(f"{cls}.{name}: 引数の型 {t} を変換できない")
                return None
        rret = None
        if ret in ("TRect", "TGridRect"):
            pre.append("        _r = [ctypes.c_int() for _ in range(4)]")
            args.append("*(ctypes.byref(x) for x in _r)")
            expect += ["*"] * 4
            rret = "TRect(*(x.value for x in _r))"
        elif ret != "void":
            rret = self.from_raw(ret, "_r")
            if rret is None:
                self.err(f"{cls}.{name}: 戻り値の型 {ret} を変換できない")
                return None
        if len(expect) != len(ftypes) or any(e != "*" and e != f and not (e == "int_t" and f == "uint_t") and not (e == "obj_t" and f == "void*")
                                             for e, f in zip(expect, ftypes)) or any(e == "*" and not f.endswith("*") for e, f in zip(expect, ftypes)):
            self.err(f"{cls}.{name}: 引数 {params} が {fname}{ftypes} と合わない")
            return None
        call = f"lib.{fname}(self._current(){''.join(', ' + a for a in args)})"
        lines = [f"    def {name}({', '.join(sig)}):"] + pre
        if rret is None:
            lines.append(f"        {call}")
            lines += post
        elif rret.startswith("TRect("):
            lines.append(f"        {call}")
            lines += post
            lines.append(f"        return {rret}")
        elif post:
            lines.append(f"        _r = {call}")
            lines += post
            lines.append(f"        return {rret}")
        else:
            lines.append(f"        _r = {call}")
            lines.append(f"        return {rret}")
        return lines

    def gen_events(self):
        out = ["# ---------------- イベントの型(Sender 以外の引数) ----------------", "", "_event_types.update({"]
        for name, params in self.m.events.items():
            args = [self.event_arg(param_parts(p)[0]) for p in params]
            names = [param_parts(p)[1] or "" for p in params]
            out.append(f'    "{name}": ({"".join(a + ", " for a in args)}),  # (Sender{"".join(", " + n for n in names)})')
        out.append("})")
        out.append("")
        return out


HEADER = '''\
# このファイルは gen_api.py が beth.hpp から生成する。直接編集しない。
"""Bethany の Python の公開 API。C++ の公開 API(beth.hpp)と同じクラス・メンバを持つ。

    from beth import *

    class TForm1(TForm):
        def __init__(self, AOwner):
            super().__init__(AOwner)
            self.Caption = "Hello"
            self.Button1 = TButton(self)
            self.Button1.Parent = self
            self.Button1.Caption = "OK"
            self.Button1.OnClick = self.Button1Click

        def Button1Click(self, Sender):
            self.Caption = "Clicked"

    Application.Initialize()
    Form1 = Application.CreateForm(TForm1)
    Application.Run()

C++ との違い(詳しくは beth/_core.py):
- 参照渡しの引数(int& Key・bool& CanClose・TCloseAction& Action 等)・メソッドの出力引数は Ref(.value を読み書きする)。
- Application->CreateForm(&Form1) は Form1 = Application.CreateForm(TForm1)。
- 利用者が生成するもの(TStringList・TBitmap・TPicture 等)の delete は Free()(参照が無くなったときにも破棄される)。
- LCL オブジェクトが破棄された後にラッパーへ触ると ReferenceError(C++ では未定義動作)。
- C++ の Exception は BethError(E.Message・E.ClassName())。
"""
import ctypes
import enum

from ._core import (BethError, Ref, TRect, TPoint, TObject, TPersistent, TComponent,
                   ShortCut, TextToShortCut, ShortCutToText,
                   ShowMessage, MessageDlg, InputBox, PasswordBox, InputQuery,
                   MB_OK, MB_OKCANCEL, MB_ABORTRETRYIGNORE, MB_YESNOCANCEL, MB_YESNO, MB_RETRYCANCEL,
                   MB_ICONERROR, MB_ICONQUESTION, MB_ICONWARNING, MB_ICONINFORMATION,
                   MB_DEFBUTTON1, MB_DEFBUTTON2, MB_DEFBUTTON3,
                   IDOK, IDCANCEL, IDABORT, IDRETRY, IDIGNORE, IDYES, IDNO)
from ._core import (lib, _mixins, _register, _event_types, _ItemMixin, _Prop, _Indexed, _Event,
                   _int, _float, _bool, _str, _char, _ptr, _rect_conv, _point_conv, _enum, _set, _comp, _existing, _item, _obj, _view,
                   _str_key, _enc, _dec, _h, _b, _rect, _point, _to_enum, _to_comp, _to_existing, _to_item, _to_obj,
                   _a_int, _a_bool, _a_rect, _a_enum, _a_comp, _a_item, _a_ref_int, _a_ref_bool, _a_ref_char, _a_ref_enum)
'''

FOOTER = '''\
_register(globals())

# C++Builder と同じく、アプリケーションに 1 つのグローバル変数として公開する。
Application = TApplication._global()

__all__ = [
{names}
]
'''


class MixinInfo:
    """beth/_core.py の手書きのメンバ(クラスの __dict__ の代わりに、定義されている名前の集合を持つ)。"""

    def __init__(self, names):
        self.__dict__ = {name: True for name in names}


def read_mixins():
    """beth/_core.py の _mixins(クラス名 → 手書きのクラス)を、import せずに(DLL を読み込まずに)ソースから読む。"""
    import ast
    tree = ast.parse(CORE_FILE.read_text(encoding="utf-8"))
    classes = {n.name: n for n in tree.body if isinstance(n, ast.ClassDef)}
    keys = []
    for n in tree.body:
        if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "_mixins" for t in n.targets):
            keys = [k.value for k in n.value.keys]
    mixins = {}
    for key in keys:
        names = set()
        for m in classes[key].body:
            if isinstance(m, (ast.FunctionDef, ast.AsyncFunctionDef)):
                names.add(m.name)
            elif isinstance(m, ast.Assign):
                names.update(t.id for t in m.targets if isinstance(t, ast.Name))
        mixins[key] = MixinInfo(names)
    return mixins


def main():
    model = parse_hpp()
    gen = Gen(model, read_funcs(), read_callbacks(), read_mixins())
    code = gen.generate()
    if gen.errors:
        print("対応付けられないメンバがある(beth/_core.py の _mixins に書くか、生成の規則を直す):")
        for e in gen.errors:
            print("  " + e)
        sys.exit(1)
    OUTPUT_FILE.write_text(code, encoding="utf-8", newline="\n")
    n_classes = len([c for c in model.class_order if c not in CORE_CLASSES and c not in HELPER_CLASSES])
    print(f"生成完了: {OUTPUT_FILE}(クラス {n_classes}・列挙型 {len(model.enums)}・イベントの型 {len(model.events)})")


if __name__ == "__main__":
    main()
