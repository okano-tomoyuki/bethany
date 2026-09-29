"""実装済みのクラスについて、LCL にあって beth.hpp に無いメンバを洗い出す(docs/member-coverage.md)。

    python py/member_gap.py [--lazarus C:\\tool\\lazarus]

- LCL のソースの interface 部から、クラスの published / public のプロパティと public のメソッドを取り出し、継承をたどって集める。
- beth.hpp のクラスのメンバ(gen_api.py の解析)も、継承をたどって集める。
- published の差分は、そのメンバを最初に宣言した LCL のクラスごとにまとめる(TControl のものを派生のクラスごとに重ねて出さない)。
  ドッキング・ドラッグ・右から左・ヘルプなど、意図して扱わないものは「対象外」に分ける。
- public は LCL の内部向けのものが多いため、VCL のアプリでよく使うもの(COMMON)だけを照合する。

解析は正規表現による簡易なもので、条件付きコンパイル(IFDEF)や、published を明示せずに書かれたメンバは正確でないことがある。
Tier 表(docs/member-coverage.md)は、この出力を元に手で分類したもの。
"""
import argparse
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import gen_api  # noqa: E402

FILES = ["controls.pp", "forms.pp", "stdctrls.pp", "extctrls.pp", "comctrls.pp", "buttons.pp", "menus.pp",
         "dialogs.pp", "spin.pp", "maskedit.pp", "grids.pas", "graphics.pp", "imglist.pp", "toolwin.pp",
         "clipbrd.pp", "checklst.pas", "lclclasses.pp"]

# 意図して扱わないもの(ドッキング・ドラッグ・右から左・ヘルプ・高 DPI・アクセシビリティ・設計時だけのもの)
OUT_OF_SCOPE = re.compile(
    r"^(AnchorSide\w*|Dock\w*|OnDock\w*|OnUnDock|OnStartDock|OnEndDock|OnGetSiteInfo|OnGetDockCaption|UseDockManager|"
    r"Drag\w*|OnDrag\w*|OnStartDrag|OnEndDrag|BiDiMode|BidiMode|ParentBiDiMode|ParentBidiMode|Help\w*|OnHelp\w*|"
    r"DoubleBuffered|ParentDoubleBuffered|Accessible\w*|DesignTimePPI|LCLVersion|SessionProperties|PixelsPerInch|"
    r"Scaled|OnUTF8KeyPress|ChildSizing|ScreenSnap|SnapBuffer|SnapOptions|DefaultMonitor|\w*ImagesWidth|ImageWidth|"
    r"OnGetWidthForPPI|OnCreate\w*Class)$"
)

# VCL のアプリでよく使う public のメンバ(クラス → 名前)
COMMON = {
    "TControl": ["Show", "Hide", "Refresh", "Repaint", "Invalidate", "Update", "BringToFront", "SendToBack",
                 "SetBounds", "ClientToScreen", "ScreenToClient", "ClientWidth", "ClientHeight", "BoundsRect"],
    "TWinControl": ["SetFocus", "CanFocus", "Focused", "Controls", "ControlCount", "OnEnter", "OnExit"],
    "TCustomEdit": ["SelStart", "SelLength", "SelText", "SelectAll", "ClearSelection", "CopyToClipboard",
                    "CutToClipboard", "PasteFromClipboard", "Undo", "CanUndo", "Clear", "Modified", "CaretPos"],
    "TCustomMemo": ["Append", "WordWrap", "WantReturns", "WantTabs"],
    "TCustomForm": ["ModalResult", "ActiveControl", "WindowState", "BorderStyle", "BorderIcons", "Position",
                    "FormStyle", "KeyPreview", "Icon", "CloseQuery", "Close", "Release", "ShowModal"],
    "TCustomButton": ["ModalResult", "Default", "Cancel", "Click"],
    "TCustomListBox": ["ItemIndex", "Selected", "Sorted", "TopIndex", "ItemAtPos", "MultiSelect", "SelCount",
                       "ClearSelection", "SelectAll", "Style", "OnDrawItem", "ItemHeight"],
    "TCustomComboBox": ["ItemIndex", "Style", "DropDownCount", "Sorted", "SelectAll", "SelStart", "SelLength",
                        "SelText", "AutoComplete", "DroppedDown", "OnSelect", "OnDropDown", "OnCloseUp"],
    "TApplication": ["MessageBox", "ProcessMessages", "Terminate", "Title", "Icon", "ExeName", "OnException",
                     "OnIdle", "Minimize", "Restore", "BringToFront", "HintPause", "HintHidePause", "ShowHint"],
    "TCanvas": ["TextOut", "TextWidth", "TextHeight", "TextRect", "Rectangle", "Ellipse", "LineTo", "MoveTo",
                "Polygon", "Polyline", "RoundRect", "Arc", "Pie", "FillRect", "FrameRect", "Draw", "StretchDraw",
                "CopyRect", "Brush", "Pen", "Font", "Pixels"],
}


def strip_comments(src: str) -> str:
    src = re.sub(r"\{[^}]*\}", " ", src, flags=re.S)
    src = re.sub(r"\(\*.*?\*\)", " ", src, flags=re.S)
    return re.sub(r"//[^\n]*", " ", src)


def parse_lcl(lcl: Path) -> dict:
    """クラス → (基底, {published, public, public_methods})"""
    classes: dict = {}
    for name in FILES:
        path = lcl / name
        if not path.exists():
            continue
        src = strip_comments(path.read_text(encoding="latin-1"))
        imp = re.search(r"\bimplementation\b", src, re.I)
        if imp:
            src = src[: imp.start()]
        for m in re.finditer(r"\b(T\w+)\s*=\s*class\s*\(\s*(T\w+)[^)]*\)(.*?)\bend\s*;", src, re.S | re.I):
            cls, base, body = m.group(1), m.group(2), m.group(3)
            section = "published"  # 可視性の指定の無い先頭は published(TPersistent の派生の既定)
            members = {"published": set(), "public": set(), "public_methods": set()}
            for stmt in body.split(";"):
                s = stmt.strip()
                for vis in ("strict private", "strict protected", "private", "protected", "public", "published"):
                    if re.match(rf"{vis}\b", s, re.I):
                        section = vis
                        s = s[len(vis):].strip()
                        break
                if section not in ("public", "published"):
                    continue
                pm = re.match(r"property\s+(\w+)", s, re.I)
                if pm:
                    members[section].add(pm.group(1))
                    continue
                fm = re.match(r"(?:class\s+)?(procedure|function)\s+(\w+)", s, re.I)
                if fm and section == "public":
                    members["public_methods"].add(fm.group(2))
            classes.setdefault(cls, (base, members))
    return classes


def lcl_chain(classes: dict, cls: str) -> list:
    chain = []
    while cls in classes and cls not in chain:
        chain.append(cls)
        cls = classes[cls][0]
    return chain


def beth_members() -> dict:
    """クラス → (基底, そのクラスで宣言したメンバの名前)"""
    model = gen_api.parse_hpp()
    own = {}
    for cls, (base, body, _comments) in model.classes.items():
        names = set()
        patterns = [
            r"\b(?:Property|ReadOnlyProperty|IndexedProperty|ReadOnlyIndexedProperty|IndexedProperty2)<[^;]*?>\s+(\w+)\s*;",
            r"(?:^|\n)\s*T\w+\*?\s+([A-Z]\w*)\s*;",  # 値のメンバ(TCanvas の TPen Pen; 等)
            r"\busing\s+\w+::(\w+)\s*;",
            r"(?:^|\n)\s*(?:virtual\s+|static\s+)?[\w:<>*&\s]+?\b([A-Z]\w*)\s*\(",  # メソッド
        ]
        for pattern in patterns:
            names.update(m.group(1) for m in re.finditer(pattern, body))
        own[cls] = (base, names)
    return own


def beth_chain_members(own: dict, cls: str) -> set:
    names: set = set()
    while cls in own:
        base, n = own[cls]
        names |= n
        cls = base
    return names


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--lazarus", default=os.environ.get("LAZARUS_HOME", r"C:\tool\lazarus"))
    args = ap.parse_args()
    # 日本語の Windows でも、リダイレクト先を UTF-8 で書く
    sys.stdout.reconfigure(encoding="utf-8")
    lcl = parse_lcl(Path(args.lazarus) / "lcl")
    own = beth_members()

    def declaring(cls: str, member: str) -> str:
        found = cls
        for c in lcl_chain(lcl, cls):
            m = lcl[c][1]
            if member in m["published"] or member in m["public"] or member in m["public_methods"]:
                found = c
        return found

    missing: dict = defaultdict(set)
    out_of_scope: dict = defaultdict(set)
    for cls in own:
        if cls not in lcl:
            continue
        have = beth_chain_members(own, cls)
        published = set().union(*(lcl[c][1]["published"] for c in lcl_chain(lcl, cls)))
        for p in published - have:
            (out_of_scope if OUT_OF_SCOPE.match(p) else missing)[declaring(cls, p)].add(p)

    print("# 未実装の published のメンバ(宣言した LCL のクラスごと)\n")
    for cls, names in sorted(missing.items()):
        print(f"- {cls}: {', '.join(sorted(names))}")
    print("\n# 対象外(ドッキング・ドラッグ・右から左・ヘルプ・高 DPI 等)\n")
    for cls, names in sorted(out_of_scope.items()):
        print(f"- {cls}: {', '.join(sorted(names))}")
    print("\n# VCL でよく使う public のメンバのうち未実装のもの\n")
    for cls, names in COMMON.items():
        if cls not in own:
            print(f"- {cls}: (クラスが無い)")
            continue
        have = beth_chain_members(own, cls)
        lacking = [n for n in names if n not in have]
        if lacking:
            print(f"- {cls}: {', '.join(lacking)}")


if __name__ == "__main__":
    main()
