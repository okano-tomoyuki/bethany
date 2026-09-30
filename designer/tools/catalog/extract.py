"""beth.hpp からデザイナーのコンポーネントカタログ(catalog.json)を作る(docs/designer/catalog.md)。

    python extract.py            静的な抽出 + 実測(beth.dll が要る) + 補足(overlay.json)を重ねて catalog.json を書く
    python extract.py --no-runtime
                                 実測をせず、既定値は今の catalog.json のものを引き継ぐ(DLL が無くてよい)
    python extract.py --check    --no-runtime と同じ内容を作り、catalog.json と食い違っていればエラー(書き込まない)

- 静的な抽出: py/gen_api.py の beth.hpp の解析を使い、クラス・プロパティ・イベント・列挙型・定数を取り出す。
- 実測: py/beth(Python のバインディング)で、利用者が生成できる各クラスを生成して、プロパティの既定値を読む。
  値は DSL の書き方(docs/designer/dsl-spec.md §5)で記録する。Windows(Win32)で実行したものを基準にする。
- 補足: overlay.json(手書き)。デザイン時に設定できないプロパティ・パレットの分類・子を置けるか・親子の制約など。
"""
import argparse
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PY_DIR = ROOT / "py"
OVERLAY_FILE = HERE / "overlay.json"
OUTPUT_FILE = ROOT / "designer" / "packages" / "core" / "src" / "catalog" / "catalog.json"

sys.path.insert(0, str(PY_DIR))
import gen_api  # noqa: E402

# 所有者が持つオブジェクトのビューで、中のプロパティを入れ子で書けるもの(DSL の「入れ子のオブジェクト」)。
OBJECT_CLASSES = ["TFont", "TSizeConstraints", "TControlBorderSpacing", "TControlScrollBar", "TGridColumnTitle", "TPen", "TBrush"]
# 項目の一覧(TCollection)のビューで、項目を DSL の配列で書けるもの(DSL の「コレクション」。docs/adr/0044)。
# 一覧のクラス → 項目のクラス。項目のプロパティは objects に載せる(Index は配列の並びで決まるので載せない)。
COLLECTION_CLASSES = {"TStatusPanels": "TStatusPanel", "TGridColumns": "TGridColumn"}
COLLECTION_ITEM_SKIP = {"Index"}
# 列挙型の別名で、選べる要素が一部だけのもの(LCL の部分範囲の型。docs/adr/0048)。
ENUM_ALIAS_VALUES = {"TBorderStyle": ["bsNone", "bsSingle"]}

PROPERTY_RE = re.compile(r"(ReadOnly)?(Indexed)?Property(2)?<(.+)>\s+(\w+)")
USING_RE = re.compile(r"using (\w+)::(\w+)")


# ---------------- 静的な抽出 ----------------

class Extractor:
    def __init__(self):
        self.m = gen_api.parse_hpp()
        self.base = {name: b for name, (b, _, _) in self.m.classes.items()}
        self.event_types = set(self.m.events) | set(self.m.event_aliases)

    def derives(self, name, root):
        while name:
            if name == root:
                return True
            name = self.base.get(name)
        return False

    def chain(self, name):
        """基底から順のクラスの並び(TObject … name)。"""
        out = []
        while name:
            out.append(name)
            name = self.base.get(name)
        return list(reversed(out))

    def type_of(self, t):
        """プロパティの型を、カタログの型(DSL の値の書き方)に分類する。"""
        t = t.strip()
        if t in ("int", "unsigned int", "unsigned short", "std::intptr_t"):
            return {"kind": "int"}
        if t == "double":
            return {"kind": "float"}
        if t == "bool":
            return {"kind": "bool"}
        if t == "std::string":
            return {"kind": "string"}
        if t == "char":
            return {"kind": "char"}
        if t in self.m.int_aliases:
            return {"kind": "alias", "alias": t}
        if t in self.m.enums:
            return {"kind": "enum", "enum": t}
        if t in self.m.enum_aliases:
            target = self.m.enum_aliases[t]
            values = ENUM_ALIAS_VALUES.get(t)
            return {"kind": "enum", "enum": target, **({"values": values} if values else {})}
        if t in self.m.flags:
            return {"kind": "flags", "flags": t}
        if t in self.m.sets:
            return {"kind": "set", "enum": self.m.sets[t]}
        m = re.fullmatch(r"(?:const )?(\w+)\*", t)
        if m and m.group(1) in self.m.classes:
            c = m.group(1)
            if c in OBJECT_CLASSES:
                return {"kind": "object", "class": c}
            if c == "TStrings":
                return {"kind": "strings"}
            if c in COLLECTION_CLASSES:
                return {"kind": "collection", "item": COLLECTION_CLASSES[c]}
            if self.derives(c, "TComponent"):
                return {"kind": "ref", "class": c}
            return {"kind": "other", "type": t}
        return {"kind": "other", "type": t}

    def members(self, name):
        """クラス name の公開されたプロパティ・イベント(継承したものを含む)。後で宣言したものが前のものを隠す。"""
        props, events = {}, {}
        protected = {}  # 名前 → (種類, 情報)。派生の using で公開されるもの
        for cls in self.chain(name):
            if cls not in self.m.classes:
                continue
            _, body, _ = self.m.classes[cls]
            section = "private"
            for st in gen_api.split_statements(body):
                t = st.text
                sm = re.fullmatch(r"(public|protected|private)\s*:", t)
                if sm:
                    section = sm.group(1)
                    continue
                if section == "private" or st.body is not None:
                    continue
                um = USING_RE.fullmatch(t)
                if um:
                    if section == "public" and um.group(2) in protected:
                        kind, info = protected[um.group(2)]
                        (props if kind == "prop" else events)[um.group(2)] = info
                    continue
                pm = PROPERTY_RE.fullmatch(t)
                if not pm:
                    continue
                readonly, indexed, two, targs, pname = pm.groups()
                # 区切りのコメント(// ---- docs/adr/0049 ----)は説明に含めない
                doc = " ".join(c for c in st.comments if c and not re.fullmatch(r"-+ .* -+", c))
                if targs in self.event_types:
                    ev = self.m.event_aliases.get(targs, targs)
                    info = {"type": ev, "declaredIn": cls}
                    if doc:
                        info["doc"] = doc
                    kind = "event"
                else:
                    if indexed or two:
                        continue  # 添字つきのプロパティはデザイン時に設定しない
                    info = {"type": self.type_of(targs), "declaredIn": cls, "readOnly": bool(readonly)}
                    if doc:
                        info["doc"] = doc
                    kind = "prop"
                if section == "protected":
                    protected[pname] = (kind, info)
                    continue
                (props if kind == "prop" else events)[pname] = info
                protected.pop(pname, None)
        return props, events

    def creatable(self, name):
        _, body, _ = self.m.classes[name]
        section = "private"
        for st in gen_api.split_statements(body):
            sm = re.fullmatch(r"(public|protected|private)\s*:", st.text)
            if sm:
                section = sm.group(1)
                continue
            if section == "public" and re.fullmatch(rf"(?:explicit )?{name}\(TComponent\* AOwner\)", st.text):
                return True
        return False

    def public_names(self, name):
        """クラス name(継承したものを含む)の public なメンバの名前。生成するフォームのクラスで、
        コンポーネント・ハンドラの名前と衝突してはならないもの。"""
        names = set()
        for cls in self.chain(name):
            if cls not in self.m.classes:
                continue
            _, body, _ = self.m.classes[cls]
            section = "private"
            for st in gen_api.split_statements(body):
                t = st.text
                sm = re.fullmatch(r"(public|protected|private)\s*:", t)
                if sm:
                    section = sm.group(1)
                    continue
                if section == "private":
                    continue
                um = USING_RE.fullmatch(t)
                pm = PROPERTY_RE.fullmatch(t)
                fm = re.search(r"(~?\w+)\s*\(", t)
                vm = re.fullmatch(r"[\w:<>*&, ]+?[\s*&](\w+)(?:\s*=.*)?", t)
                if um:
                    names.add(um.group(2))
                elif pm:
                    names.add(pm.group(5))
                elif fm and not fm.group(1).startswith("~") and fm.group(1) not in ("operator", "explicit"):
                    names.add(fm.group(1))
                elif vm and not t.startswith(("using", "friend", "struct", "class", "enum")):
                    names.add(vm.group(1))
        # 基底クラスのコンストラクタ(protected の TComponent(ObjectHandle) 等)と演算子は名前ではない
        return sorted(names - set(self.chain(name)) - {"operator"})

    def class_kind(self, name):
        if name == "TForm" or self.derives(name, "TCustomForm"):
            return "form"
        if self.derives(name, "TControl"):
            return "control"
        if name == "TMenuItem":
            return "menuItem"
        if self.derives(name, "TBasicAction"):
            return "action"  # ActionList の子(DSL の actions。docs/adr/0046)
        return "component"

    def extract(self):
        classes = {}
        for name in self.m.class_order:
            if not self.derives(name, "TComponent") or name == "TComponent" or not self.creatable(name):
                continue
            props, events = self.members(name)
            classes[name] = {
                "ancestors": list(reversed(self.chain(name)[:-1])),
                "kind": self.class_kind(name),
                "properties": props,
                "events": events,
            }
        objects = {}
        for name in OBJECT_CLASSES:
            props, _ = self.members(name)
            objects[name] = {"properties": props}
        for name in COLLECTION_CLASSES.values():
            props, _ = self.members(name)
            objects[name] = {"properties": {n: i for n, i in props.items() if n not in COLLECTION_ITEM_SKIP}}
        enums = {e: [i for i, _, _ in items] for e, items in self.m.enums.items()}
        flags = {f: [i for i, _, _ in items] for f, items in self.m.flags.items()}
        constants = {}
        for alias, items in self.m.int_aliases.items():
            constants[alias] = {i: int(eval(v)) for i, v, _ in items}  # 値は 0x.. や -21 のような整数の式
        events = {}
        for ev, params in self.m.events.items():
            events[ev] = [{"name": "Sender", "type": "TObject*"}] + [
                {"name": n, "type": t} for t, n, _ in (gen_api.param_parts(p) for p in params)]
        return {"formMembers": self.public_names("TForm"), "classes": classes, "objects": objects, "enums": enums, "flags": flags,
                "sets": dict(self.m.sets), "constants": constants, "eventTypes": events}


# ---------------- 補足の適用 ----------------

def designable(info):
    k = info["type"]["kind"]
    if k in ("strings", "collection"):
        return True  # TStrings・コレクションは読み取り専用のプロパティだが、中身(Add)を設定する
    if k == "object" and info["readOnly"]:
        return True  # 入れ子のオブジェクト(グリッドの列の Title。docs/adr/0054)は、読み取り専用でも中のプロパティを設定する
    if info["readOnly"]:
        return False
    return k in ("int", "float", "bool", "string", "char", "alias", "enum", "flags", "set", "ref", "object")


def apply_overlay(cat, overlay, chain):
    """chain(name) は基底から順のクラスの並び(カタログに載らない中間のクラスも含む)。"""
    not_designable = overlay.get("notDesignable", {})
    re_designable = overlay.get("designable", {})
    for name, cls in cat["classes"].items():
        # notDesignable はそのクラスと派生で除外し、designable はそのクラスと派生で除外を取り消す。
        excluded = set()
        for owner, names in not_designable.items():
            if owner in chain(name):
                excluded.update(names)
        for owner, names in re_designable.items():
            if owner in chain(name):
                excluded.difference_update(names)
        for pname, info in cls["properties"].items():
            info["designable"] = designable(info) and pname not in excluded
        spec = overlay["classes"].get(name, {})
        cls["palette"] = spec.get("palette")
        cls["acceptsControls"] = spec.get("acceptsControls", False)
        for key in ("childClasses", "parentClasses"):
            if key in spec:
                cls[key] = spec[key]
    for name, obj in cat["objects"].items():
        for pname, info in obj["properties"].items():
            info["designable"] = designable(info)
    cat["palette"] = overlay["palette"]
    missing = [n for n in cat["classes"] if n not in overlay["classes"]]
    unknown = [n for n in overlay["classes"] if n not in cat["classes"]]
    return missing, unknown


# ---------------- 実測 ----------------

def measure(cat):
    """各クラスを生成して、デザイン時に設定できるプロパティの既定値を読む(DSL の書き方で)。"""
    import beth

    colors = {v: k for k, v in cat["constants"]["TColor"].items()}
    cursors = {}
    for k, v in cat["constants"]["TCursor"].items():
        cursors.setdefault(v, k)  # crSizeAll と crSize は同じ値。先に宣言したほうを使う

    def to_dsl(t, value):
        k = t["kind"]
        if k in ("int", "float", "bool", "string", "char"):
            return value
        if k == "enum":
            return value.name if hasattr(value, "name") else int(value)
        if k == "flags":
            return [n for n in cat["flags"][t["flags"]] if int(value) & int(getattr(beth, n))]
        if k == "set":
            return [n for n in cat["enums"][t["enum"]] if getattr(beth, n) in value]
        if k == "alias":
            v = int(value)
            if t["alias"] == "TColor":
                if v in colors:
                    return colors[v]
                return "#%02X%02X%02X" % (v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF)
            if t["alias"] == "TCursor":
                return cursors.get(v, v)
            if t["alias"] == "TShortCut":
                return beth.ShortCutToText(v) if v else ""  # 0 は割り当てなし(LCL は "Unknown" を返す)
            # 定数を持つ別名(TModalResult 等)は定数の名前で書く
            names = {c: n for n, c in reversed(list(cat["constants"].get(t["alias"], {}).items()))}
            return names.get(v, v)
        if k == "ref":
            return None if value is None else "?"
        if k == "strings":
            return [value.Strings[i] for i in range(value.Count)]
        if k == "object":
            return object_dsl(t["class"], value)
        if k == "collection":
            return [object_dsl(t["item"], value.Items[i]) for i in range(value.Count)]
        raise ValueError(k)

    def object_dsl(class_name, value):
        obj = {}
        for pname, info in cat["objects"][class_name]["properties"].items():
            if info["designable"]:
                obj[pname] = to_dsl(info["type"], getattr(value, pname))
        return obj

    def measure_items(obj, cls):
        """コレクションの項目の既定値: 空の項目を 1 つ加えて読む(項目のクラスごとに 1 度)。"""
        for pname, info in cls["properties"].items():
            t = info["type"]
            if t["kind"] != "collection" or not info["designable"] or t["item"] in measured_items:
                continue
            measured_items.add(t["item"])
            collection = getattr(obj, pname)
            item = collection.Add()
            for iname, iinfo in cat["objects"][t["item"]]["properties"].items():
                if iinfo["designable"]:
                    iinfo["default"] = to_dsl(iinfo["type"], getattr(item, iname))
            collection.Clear()

    owner = beth.TForm(beth.Application)
    failures = []
    measured_items = set()
    for name, cls in cat["classes"].items():
        try:
            obj = getattr(beth, name)(beth.Application if cls["kind"] == "form" else owner)
        except Exception as e:  # noqa: BLE001
            failures.append(f"{name}: 生成できない ({e})")
            continue
        for pname, info in cls["properties"].items():
            if not info["designable"]:
                continue
            try:
                info["default"] = to_dsl(info["type"], getattr(obj, pname))
            except Exception as e:  # noqa: BLE001
                failures.append(f"{name}.{pname}: 読めない ({e})")
        if cls["kind"] in ("control", "form"):
            cls["defaultSize"] = {"width": obj.Width, "height": obj.Height}
        try:
            measure_items(obj, cls)
        except Exception as e:  # noqa: BLE001
            failures.append(f"{name}: コレクションの項目の既定値を読めない ({e})")
        obj.Free()
    owner.Free()
    return failures


def keep_defaults(cat, old):
    """実測しないときは、今のカタログの既定値を引き継ぐ。"""
    for name, cls in cat["classes"].items():
        oc = old.get("classes", {}).get(name)
        if not oc:
            continue
        if "defaultSize" in oc:
            cls["defaultSize"] = oc["defaultSize"]
        for pname, info in cls["properties"].items():
            op = oc["properties"].get(pname)
            if op and "default" in op and info["designable"]:
                info["default"] = op["default"]
    for name, obj in cat["objects"].items():
        oo = old.get("objects", {}).get(name)
        if not oo:
            continue
        for pname, info in obj["properties"].items():
            op = oo["properties"].get(pname)
            if op and "default" in op and info["designable"]:
                info["default"] = op["default"]


# ---------------- 書き出し ----------------

def order(cat):
    """決まった順に並べ直す(クラスは beth.hpp の順、クラスの中のキーは固定の順)。
    デザイン時に設定できないプロパティは書き出さない(デザイナーは使わないため)。"""
    key_order = ["ancestors", "kind", "palette", "acceptsControls", "childClasses", "parentClasses", "defaultSize", "properties", "events"]
    prop_order = ["type", "declaredIn", "default", "doc"]
    first = ["Left", "Top", "Width", "Height"]
    out = dict(cat)
    classes = {}
    for name, cls in cat["classes"].items():
        c = {k: cls[k] for k in key_order if k in cls and cls[k] is not None}
        props = {n: i for n, i in cls["properties"].items() if i["designable"]}
        names = [n for n in first if n in props] + [n for n in props if n not in first]
        # 値は範囲の後に設定する(コード生成はカタログの順)。2026-09-28 の実測(Win32)では順で結果は変わらなかったが、
        # LCL の範囲に収める処理(GetLimitedValue)がいつ働くかに依存しないよう、範囲を先に決める
        if "Value" in names and "MaxValue" in names:
            names.remove("Value")
            names.insert(names.index("MaxValue") + 1, "Value")
        c["properties"] = {n: {k: props[n][k] for k in prop_order if k in props[n]} for n in names}
        classes[name] = c
    out["classes"] = classes
    out["objects"] = {name: {"properties": {n: {k: i[k] for k in prop_order if k in i}
                                            for n, i in obj["properties"].items() if i["designable"]}}
                      for name, obj in cat["objects"].items()}
    return out


def dump(cat):
    return json.dumps(cat, ensure_ascii=False, indent=2) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--no-runtime", action="store_true", help="実測しない(既定値は今のカタログから引き継ぐ)")
    ap.add_argument("--check", action="store_true", help="catalog.json が beth.hpp・overlay.json と食い違っていないか調べる")
    args = ap.parse_args()

    ex = Extractor()
    cat = ex.extract()
    overlay = json.loads(OVERLAY_FILE.read_text(encoding="utf-8"))
    missing, unknown = apply_overlay(cat, overlay, ex.chain)
    problems = [f"overlay.json に無いクラス: {n}" for n in missing] + [f"overlay.json のクラスがカタログに無い: {n}" for n in unknown]

    old = json.loads(OUTPUT_FILE.read_text(encoding="utf-8")) if OUTPUT_FILE.exists() else {}
    if args.check or args.no_runtime:
        keep_defaults(cat, old)
    else:
        problems += measure(cat)
    text = dump(order(cat))

    if args.check:
        current = OUTPUT_FILE.read_text(encoding="utf-8") if OUTPUT_FILE.exists() else ""
        if current != text:
            print("catalog.json が beth.hpp・overlay.json と食い違っている。python designer/tools/catalog/extract.py で作り直す。")
            sys.exit(1)
        if problems:
            print("\n".join(problems))
            sys.exit(1)
        print("catalog.json は最新")
        return
    if problems:
        print("\n".join(problems))
    OUTPUT_FILE.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_FILE.write_text(text, encoding="utf-8", newline="\n")
    n = len(cat["classes"])
    print(f"書き出した: {OUTPUT_FILE}(クラス {n})")
    if problems:
        sys.exit(1)


if __name__ == "__main__":
    main()
