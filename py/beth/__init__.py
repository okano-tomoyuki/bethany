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
                   ShortCut, TextToShortCut, ShortCutToText, Clipboard, CF_Text, CF_Bitmap, CF_Picture,
                   ShowMessage, MessageDlg, InputBox, PasswordBox, InputQuery,
                   MB_OK, MB_OKCANCEL, MB_ABORTRETRYIGNORE, MB_YESNOCANCEL, MB_YESNO, MB_RETRYCANCEL,
                   MB_ICONERROR, MB_ICONQUESTION, MB_ICONWARNING, MB_ICONINFORMATION,
                   MB_DEFBUTTON1, MB_DEFBUTTON2, MB_DEFBUTTON3,
                   IDOK, IDCANCEL, IDABORT, IDRETRY, IDIGNORE, IDYES, IDNO)
from ._core import (lib, _mixins, _register, _event_types, _ItemMixin, _Prop, _Indexed, _Event,
                   _int, _float, _bool, _str, _char, _ptr, _rect_conv, _point_conv, _enum, _set, _comp, _existing, _item, _obj, _view,
                   _str_key, _enc, _dec, _h, _b, _rect, _point, _to_enum, _to_comp, _to_existing, _to_item, _to_obj,
                   _a_int, _a_bool, _a_rect, _a_exception, _a_strings, _a_enum, _a_comp, _a_item, _a_obj, _a_ref_int, _a_ref_bool, _a_ref_char, _a_ref_enum)


# ---------------- 列挙型 ----------------

# フォームを閉じるときの動作(C++Builder / LCL の TCloseAction と同じ値)。
class TCloseAction(enum.IntEnum):
    caNone = 0  # 閉じない
    caHide = 1  # 隠す(MainForm 以外の既定値)
    caFree = 2  # 破棄する(MainForm の既定値。MainForm ならアプリケーションを終了する)
    caMinimize = 3  # 最小化する

caNone, caHide, caFree, caMinimize = TCloseAction.caNone, TCloseAction.caHide, TCloseAction.caFree, TCloseAction.caMinimize


# マウスボタン(OnMouseDown / OnMouseUp の Button)。
class TMouseButton(enum.IntEnum):
    mbLeft = 0
    mbRight = 1
    mbMiddle = 2
    mbExtra1 = 3
    mbExtra2 = 4

mbLeft, mbRight, mbMiddle, mbExtra1, mbExtra2 = TMouseButton.mbLeft, TMouseButton.mbRight, TMouseButton.mbMiddle, TMouseButton.mbExtra1, TMouseButton.mbExtra2


# Sorted のときの重複の扱い(LCL の TDuplicates と同じ値)。
class TDuplicates(enum.IntEnum):
    dupIgnore = 0
    dupAccept = 1
    dupError = 2

dupIgnore, dupAccept, dupError = TDuplicates.dupIgnore, TDuplicates.dupAccept, TDuplicates.dupError


# 線の種類(LCL の TPenStyle と同じ値。docs/adr/0045)。psClear は線を描かない。
class TPenStyle(enum.IntEnum):
    psSolid = 0
    psDash = 1
    psDot = 2
    psDashDot = 3
    psDashDotDot = 4
    psInsideFrame = 5
    psPattern = 6
    psClear = 7

psSolid, psDash, psDot, psDashDot, psDashDotDot, psInsideFrame, psPattern, psClear = TPenStyle.psSolid, TPenStyle.psDash, TPenStyle.psDot, TPenStyle.psDashDot, TPenStyle.psDashDotDot, TPenStyle.psInsideFrame, TPenStyle.psPattern, TPenStyle.psClear


# 線の描き方(ラスタ演算。LCL の TPenMode と同じ値)。pmXor で 2 度描くと元に戻る(ラバーバンド等)。
class TPenMode(enum.IntEnum):
    pmBlack = 0
    pmWhite = 1
    pmNop = 2
    pmNot = 3
    pmCopy = 4
    pmNotCopy = 5
    pmMergePenNot = 6
    pmMaskPenNot = 7
    pmMergeNotPen = 8
    pmMaskNotPen = 9
    pmMerge = 10
    pmNotMerge = 11
    pmMask = 12
    pmNotMask = 13
    pmXor = 14
    pmNotXor = 15

pmBlack, pmWhite, pmNop, pmNot, pmCopy, pmNotCopy, pmMergePenNot, pmMaskPenNot, pmMergeNotPen, pmMaskNotPen, pmMerge, pmNotMerge, pmMask, pmNotMask, pmXor, pmNotXor = TPenMode.pmBlack, TPenMode.pmWhite, TPenMode.pmNop, TPenMode.pmNot, TPenMode.pmCopy, TPenMode.pmNotCopy, TPenMode.pmMergePenNot, TPenMode.pmMaskPenNot, TPenMode.pmMergeNotPen, TPenMode.pmMaskNotPen, TPenMode.pmMerge, TPenMode.pmNotMerge, TPenMode.pmMask, TPenMode.pmNotMask, TPenMode.pmXor, TPenMode.pmNotXor


# 塗りつぶしの種類(LCL の TBrushStyle と同じ値)。bsClear は塗りつぶさない(TextOut の背景も透明になる)。
class TBrushStyle(enum.IntEnum):
    bsSolid = 0
    bsClear = 1
    bsHorizontal = 2
    bsVertical = 3
    bsFDiagonal = 4
    bsBDiagonal = 5
    bsCross = 6
    bsDiagCross = 7
    bsImage = 8
    bsPattern = 9

bsSolid, bsClear, bsHorizontal, bsVertical, bsFDiagonal, bsBDiagonal, bsCross, bsDiagCross, bsImage, bsPattern = TBrushStyle.bsSolid, TBrushStyle.bsClear, TBrushStyle.bsHorizontal, TBrushStyle.bsVertical, TBrushStyle.bsFDiagonal, TBrushStyle.bsBDiagonal, TBrushStyle.bsCross, TBrushStyle.bsDiagCross, TBrushStyle.bsImage, TBrushStyle.bsPattern


# 文字の描き方(アンチエイリアス等。LCL の TFontQuality と同じ値)。
class TFontQuality(enum.IntEnum):
    fqDefault = 0
    fqDraft = 1
    fqProof = 2
    fqNonAntialiased = 3
    fqAntialiased = 4
    fqCleartype = 5
    fqCleartypeNatural = 6

fqDefault, fqDraft, fqProof, fqNonAntialiased, fqAntialiased, fqCleartype, fqCleartypeNatural = TFontQuality.fqDefault, TFontQuality.fqDraft, TFontQuality.fqProof, TFontQuality.fqNonAntialiased, TFontQuality.fqAntialiased, TFontQuality.fqCleartype, TFontQuality.fqCleartypeNatural


# 画素の形式(LCL の TPixelFormat と同じ値)。
class TPixelFormat(enum.IntEnum):
    pfDevice = 0
    pf1bit = 1
    pf4bit = 2
    pf8bit = 3
    pf15bit = 4
    pf16bit = 5
    pf24bit = 6
    pf32bit = 7
    pfCustom = 8

pfDevice, pf1bit, pf4bit, pf8bit, pf15bit, pf16bit, pf24bit, pf32bit, pfCustom = TPixelFormat.pfDevice, TPixelFormat.pf1bit, TPixelFormat.pf4bit, TPixelFormat.pf8bit, TPixelFormat.pf15bit, TPixelFormat.pf16bit, TPixelFormat.pf24bit, TPixelFormat.pf32bit, TPixelFormat.pfCustom


# Transparent のときに透過する色の決め方。tmAuto は左下の画素の色、tmFixed は TransparentColor。
class TTransparentMode(enum.IntEnum):
    tmAuto = 0
    tmFixed = 1

tmAuto, tmFixed = TTransparentMode.tmAuto, TTransparentMode.tmFixed


# 画像リストの描き方(LCL の TDrawingStyle と同じ値)。
class TDrawingStyle(enum.IntEnum):
    dsFocus = 0
    dsSelected = 1
    dsNormal = 2
    dsTransparent = 3

dsFocus, dsSelected, dsNormal, dsTransparent = TDrawingStyle.dsFocus, TDrawingStyle.dsSelected, TDrawingStyle.dsNormal, TDrawingStyle.dsTransparent


# リストボックスの描き方(lbOwnerDrawFixed・lbOwnerDrawVariable なら OnDrawItem で描く。lbVirtual は対象外)。
class TListBoxStyle(enum.IntEnum):
    lbStandard = 0
    lbOwnerDrawFixed = 1
    lbOwnerDrawVariable = 2
    lbVirtual = 3

lbStandard, lbOwnerDrawFixed, lbOwnerDrawVariable, lbVirtual = TListBoxStyle.lbStandard, TListBoxStyle.lbOwnerDrawFixed, TListBoxStyle.lbOwnerDrawVariable, TListBoxStyle.lbVirtual


# 親のクライアント領域への寄せ方(LCL / VCL の TAlign と同じ値)。寄せた方向の位置・大きさは LCL が決める
# (alTop なら Left/Top/Width が親に合わせられ、Height だけが保たれる。alClient は残りの領域をすべて埋める)。
class TAlign(enum.IntEnum):
    alNone = 0
    alTop = 1
    alBottom = 2
    alLeft = 3
    alRight = 4
    alClient = 5
    alCustom = 6

alNone, alTop, alBottom, alLeft, alRight, alClient, alCustom = TAlign.alNone, TAlign.alTop, TAlign.alBottom, TAlign.alLeft, TAlign.alRight, TAlign.alClient, TAlign.alCustom


# コントロールの辺(LCL の TAnchorKind と同じ値。VCL の TAnchorKind とは並びが違う)。
class TAnchorKind(enum.IntEnum):
    akTop = 0
    akLeft = 1
    akRight = 2
    akBottom = 3

akTop, akLeft, akRight, akBottom = TAnchorKind.akTop, TAnchorKind.akLeft, TAnchorKind.akRight, TAnchorKind.akBottom


# フォームの枠(bsDialog は大きさを変えられず、最小化・最大化のボタンが無い。docs/adr/0041)。
class TFormBorderStyle(enum.IntEnum):
    bsNone = 0
    bsSingle = 1
    bsSizeable = 2
    bsDialog = 3
    bsToolWindow = 4
    bsSizeToolWin = 5

bsNone, bsSingle, bsSizeable, bsDialog, bsToolWindow, bsSizeToolWin = TFormBorderStyle.bsNone, TFormBorderStyle.bsSingle, TFormBorderStyle.bsSizeable, TFormBorderStyle.bsDialog, TFormBorderStyle.bsToolWindow, TFormBorderStyle.bsSizeToolWin


# パネルの縁の凹凸(bvSpace は凹凸の無い余白)。
class TPanelBevel(enum.IntEnum):
    bvNone = 0
    bvLowered = 1
    bvRaised = 2
    bvSpace = 3

bvNone, bvLowered, bvRaised, bvSpace = TPanelBevel.bvNone, TPanelBevel.bvLowered, TPanelBevel.bvRaised, TPanelBevel.bvSpace


# 文字の縦の揃え(TPanel の VerticalAlignment)。
class TVerticalAlignment(enum.IntEnum):
    taAlignTop = 0
    taAlignBottom = 1
    taVerticalCenter = 2

taAlignTop, taAlignBottom, taVerticalCenter = TVerticalAlignment.taAlignTop, TVerticalAlignment.taAlignBottom, TVerticalAlignment.taVerticalCenter


# スクロールバーの出し方。ssAuto… は、内容がはみ出したときだけ出す。
class TScrollStyle(enum.IntEnum):
    ssNone = 0
    ssHorizontal = 1
    ssVertical = 2
    ssBoth = 3
    ssAutoHorizontal = 4
    ssAutoVertical = 5
    ssAutoBoth = 6

ssNone, ssHorizontal, ssVertical, ssBoth, ssAutoHorizontal, ssAutoVertical, ssAutoBoth = TScrollStyle.ssNone, TScrollStyle.ssHorizontal, TScrollStyle.ssVertical, TScrollStyle.ssBoth, TScrollStyle.ssAutoHorizontal, TScrollStyle.ssAutoVertical, TScrollStyle.ssAutoBoth


class TScrollBarKind(enum.IntEnum):
    sbHorizontal = 0
    sbVertical = 1

sbHorizontal, sbVertical = TScrollBarKind.sbHorizontal, TScrollBarKind.sbVertical


# コンボボックスの見た目と入力(csDropDownList は一覧から選ぶだけ。csOwnerDraw… は Tier B のオーナードロー)。
class TComboBoxStyle(enum.IntEnum):
    csDropDown = 0
    csSimple = 1
    csDropDownList = 2
    csOwnerDrawFixed = 3
    csOwnerDrawVariable = 4
    csOwnerDrawEditableFixed = 5
    csOwnerDrawEditableVariable = 6

csDropDown, csSimple, csDropDownList, csOwnerDrawFixed, csOwnerDrawVariable, csOwnerDrawEditableFixed, csOwnerDrawEditableVariable = TComboBoxStyle.csDropDown, TComboBoxStyle.csSimple, TComboBoxStyle.csDropDownList, TComboBoxStyle.csOwnerDrawFixed, TComboBoxStyle.csOwnerDrawVariable, TComboBoxStyle.csOwnerDrawEditableFixed, TComboBoxStyle.csOwnerDrawEditableVariable


# チェックボックスの状態。
class TCheckBoxState(enum.IntEnum):
    cbUnchecked = 0
    cbChecked = 1
    cbGrayed = 2

cbUnchecked, cbChecked, cbGrayed = TCheckBoxState.cbUnchecked, TCheckBoxState.cbChecked, TCheckBoxState.cbGrayed


# 文字の横の揃え(LCL の TAlignment)。
class TAlignment(enum.IntEnum):
    taLeftJustify = 0
    taRightJustify = 1
    taCenter = 2

taLeftJustify, taRightJustify, taCenter = TAlignment.taLeftJustify, TAlignment.taRightJustify, TAlignment.taCenter


# 文字の縦の揃え(TLabel の Layout)。
class TTextLayout(enum.IntEnum):
    tlTop = 0
    tlCenter = 1
    tlBottom = 2

tlTop, tlCenter, tlBottom = TTextLayout.tlTop, TTextLayout.tlCenter, TTextLayout.tlBottom


# 入力した文字の表示のしかた(TEdit の EchoMode)。emPassword は PasswordChar(既定は *)で伏せる。
class TEchoMode(enum.IntEnum):
    emNormal = 0
    emNone = 1
    emPassword = 2

emNormal, emNone, emPassword = TEchoMode.emNormal, TEchoMode.emNone, TEchoMode.emPassword


# 入力した英字のそろえ方(TEdit の CharCase。VCL と同じ綴り)。
class TEditCharCase(enum.IntEnum):
    ecNormal = 0
    ecUpperCase = 1
    ecLowerCase = 2

ecNormal, ecUpperCase, ecLowerCase = TEditCharCase.ecNormal, TEditCharCase.ecUpperCase, TEditCharCase.ecLowerCase


class TTrackBarOrientation(enum.IntEnum):
    trHorizontal = 0
    trVertical = 1

trHorizontal, trVertical = TTrackBarOrientation.trHorizontal, TTrackBarOrientation.trVertical


# 目盛りを付ける側(tmBottomRight は横なら下、縦なら右)。
class TTickMark(enum.IntEnum):
    tmBottomRight = 0
    tmTopLeft = 1
    tmBoth = 2

tmBottomRight, tmTopLeft, tmBoth = TTickMark.tmBottomRight, TTickMark.tmTopLeft, TTickMark.tmBoth


class TTickStyle(enum.IntEnum):
    tsNone = 0
    tsAuto = 1
    tsManual = 2

tsNone, tsAuto, tsManual = TTickStyle.tsNone, TTickStyle.tsAuto, TTickStyle.tsManual


class TProgressBarOrientation(enum.IntEnum):
    pbHorizontal = 0
    pbVertical = 1
    pbRightToLeft = 2
    pbTopDown = 3

pbHorizontal, pbVertical, pbRightToLeft, pbTopDown = TProgressBarOrientation.pbHorizontal, TProgressBarOrientation.pbVertical, TProgressBarOrientation.pbRightToLeft, TProgressBarOrientation.pbTopDown


class TProgressBarStyle(enum.IntEnum):
    pbstNormal = 0
    pbstMarquee = 1

pbstNormal, pbstMarquee = TProgressBarStyle.pbstNormal, TProgressBarStyle.pbstMarquee


class TUDOrientation(enum.IntEnum):
    udHorizontal = 0
    udVertical = 1

udHorizontal, udVertical = TUDOrientation.udHorizontal, TUDOrientation.udVertical


class TUDAlignButton(enum.IntEnum):
    udLeft = 0
    udRight = 1
    udTop = 2
    udBottom = 3

udLeft, udRight, udTop, udBottom = TUDAlignButton.udLeft, TUDAlignButton.udRight, TUDAlignButton.udTop, TUDAlignButton.udBottom


class TColumnLayout(enum.IntEnum):
    clHorizontalThenVertical = 0
    clVerticalThenHorizontal = 1

clHorizontalThenVertical, clVerticalThenHorizontal = TColumnLayout.clHorizontalThenVertical, TColumnLayout.clVerticalThenHorizontal


# スクロールバーの操作(TScrollBar の OnScroll。Windows の SB_… と同じ値)。
class TScrollCode(enum.IntEnum):
    scLineUp = 0
    scLineDown = 1
    scPageUp = 2
    scPageDown = 3
    scPosition = 4
    scTrack = 5
    scTop = 6
    scBottom = 7
    scEndScroll = 8

scLineUp, scLineDown, scPageUp, scPageDown, scPosition, scTrack, scTop, scBottom, scEndScroll = TScrollCode.scLineUp, TScrollCode.scLineDown, TScrollCode.scPageUp, TScrollCode.scPageDown, TScrollCode.scPosition, TScrollCode.scTrack, TScrollCode.scTop, TScrollCode.scBottom, TScrollCode.scEndScroll


# 最初に表示する位置(poDesigned は Left・Top のまま。poMainFormCenter はメインフォームの中央)。
class TPosition(enum.IntEnum):
    poDesigned = 0
    poDefault = 1
    poDefaultPosOnly = 2
    poDefaultSizeOnly = 3
    poScreenCenter = 4
    poDesktopCenter = 5
    poMainFormCenter = 6
    poOwnerFormCenter = 7
    poWorkAreaCenter = 8

poDesigned, poDefault, poDefaultPosOnly, poDefaultSizeOnly, poScreenCenter, poDesktopCenter, poMainFormCenter, poOwnerFormCenter, poWorkAreaCenter = TPosition.poDesigned, TPosition.poDefault, TPosition.poDefaultPosOnly, TPosition.poDefaultSizeOnly, TPosition.poScreenCenter, TPosition.poDesktopCenter, TPosition.poMainFormCenter, TPosition.poOwnerFormCenter, TPosition.poWorkAreaCenter


class TWindowState(enum.IntEnum):
    wsNormal = 0
    wsMinimized = 1
    wsMaximized = 2
    wsFullScreen = 3

wsNormal, wsMinimized, wsMaximized, wsFullScreen = TWindowState.wsNormal, TWindowState.wsMinimized, TWindowState.wsMaximized, TWindowState.wsFullScreen


# タイトルバーのボタン。
class TBorderIcon(enum.IntEnum):
    biSystemMenu = 0
    biMinimize = 1
    biMaximize = 2
    biHelp = 3

biSystemMenu, biMinimize, biMaximize, biHelp = TBorderIcon.biSystemMenu, TBorderIcon.biMinimize, TBorderIcon.biMaximize, TBorderIcon.biHelp


# fsStayOnTop は常に手前に表示する。MDI(fsMDIChild・fsMDIForm)は LCL の Win32 でも対応が限られる。
class TFormStyle(enum.IntEnum):
    fsNormal = 0
    fsMDIChild = 1
    fsMDIForm = 2
    fsStayOnTop = 3
    fsSplash = 4
    fsSystemStayOnTop = 5

fsNormal, fsMDIChild, fsMDIForm, fsStayOnTop, fsSplash, fsSystemStayOnTop = TFormStyle.fsNormal, TFormStyle.fsMDIChild, TFormStyle.fsMDIForm, TFormStyle.fsStayOnTop, TFormStyle.fsSplash, TFormStyle.fsSystemStayOnTop


class TMsgDlgType(enum.IntEnum):
    mtWarning = 0
    mtError = 1
    mtInformation = 2
    mtConfirmation = 3
    mtCustom = 4

mtWarning, mtError, mtInformation, mtConfirmation, mtCustom = TMsgDlgType.mtWarning, TMsgDlgType.mtError, TMsgDlgType.mtInformation, TMsgDlgType.mtConfirmation, TMsgDlgType.mtCustom


class TMsgDlgBtn(enum.IntEnum):
    mbYes = 0
    mbNo = 1
    mbOK = 2
    mbCancel = 3
    mbAbort = 4
    mbRetry = 5
    mbIgnore = 6
    mbAll = 7
    mbNoToAll = 8
    mbYesToAll = 9
    mbHelp = 10
    mbClose = 11

mbYes, mbNo, mbOK, mbCancel, mbAbort, mbRetry, mbIgnore, mbAll, mbNoToAll, mbYesToAll, mbHelp, mbClose = TMsgDlgBtn.mbYes, TMsgDlgBtn.mbNo, TMsgDlgBtn.mbOK, TMsgDlgBtn.mbCancel, TMsgDlgBtn.mbAbort, TMsgDlgBtn.mbRetry, TMsgDlgBtn.mbIgnore, TMsgDlgBtn.mbAll, TMsgDlgBtn.mbNoToAll, TMsgDlgBtn.mbYesToAll, TMsgDlgBtn.mbHelp, TMsgDlgBtn.mbClose


# 枠線や凹凸の表現に使う、単純な表示専用コントロール(TGraphicControl の直接の派生)。
class TBevelShape(enum.IntEnum):
    bsBox = 0
    bsFrame = 1
    bsTopLine = 2
    bsBottomLine = 3
    bsLeftLine = 4
    bsRightLine = 5
    bsSpacer = 6

bsBox, bsFrame, bsTopLine, bsBottomLine, bsLeftLine, bsRightLine, bsSpacer = TBevelShape.bsBox, TBevelShape.bsFrame, TBevelShape.bsTopLine, TBevelShape.bsBottomLine, TBevelShape.bsLeftLine, TBevelShape.bsRightLine, TBevelShape.bsSpacer


class TBevelStyle(enum.IntEnum):
    bsLowered = 0
    bsRaised = 1

bsLowered, bsRaised = TBevelStyle.bsLowered, TBevelStyle.bsRaised


# bkOK/bkCancel 等の定型ボタン(既定の Caption を LCL が設定する)。
class TBitBtnKind(enum.IntEnum):
    bkCustom = 0
    bkOK = 1
    bkCancel = 2
    bkHelp = 3
    bkYes = 4
    bkNo = 5
    bkClose = 6
    bkAbort = 7
    bkRetry = 8
    bkIgnore = 9
    bkAll = 10
    bkNoToAll = 11
    bkYesToAll = 12

bkCustom, bkOK, bkCancel, bkHelp, bkYes, bkNo, bkClose, bkAbort, bkRetry, bkIgnore, bkAll, bkNoToAll, bkYesToAll = TBitBtnKind.bkCustom, TBitBtnKind.bkOK, TBitBtnKind.bkCancel, TBitBtnKind.bkHelp, TBitBtnKind.bkYes, TBitBtnKind.bkNo, TBitBtnKind.bkClose, TBitBtnKind.bkAbort, TBitBtnKind.bkRetry, TBitBtnKind.bkIgnore, TBitBtnKind.bkAll, TBitBtnKind.bkNoToAll, TBitBtnKind.bkYesToAll


# ボタンの画像(Glyph)の位置。
class TButtonLayout(enum.IntEnum):
    blGlyphLeft = 0
    blGlyphRight = 1
    blGlyphTop = 2
    blGlyphBottom = 3

blGlyphLeft, blGlyphRight, blGlyphTop, blGlyphBottom = TButtonLayout.blGlyphLeft, TButtonLayout.blGlyphRight, TButtonLayout.blGlyphTop, TButtonLayout.blGlyphBottom


# ラベルの位置(LCL の TLabelPosition と同じ値)。
class TLabelPosition(enum.IntEnum):
    lpAbove = 0
    lpBelow = 1
    lpLeft = 2
    lpRight = 3

lpAbove, lpBelow, lpLeft, lpRight = TLabelPosition.lpAbove, TLabelPosition.lpBelow, TLabelPosition.lpLeft, TLabelPosition.lpRight


# タブの位置(LCL の TTabPosition と同じ値)。
class TTabPosition(enum.IntEnum):
    tpTop = 0
    tpBottom = 1
    tpLeft = 2
    tpRight = 3

tpTop, tpBottom, tpLeft, tpRight = TTabPosition.tpTop, TTabPosition.tpBottom, TTabPosition.tpLeft, TTabPosition.tpRight


# MoveTo の移動先の指定(LCL / VCL の TNodeAttachMode と同じ値)。
class TNodeAttachMode(enum.IntEnum):
    naAdd = 0
    naAddFirst = 1
    naAddChild = 2
    naAddChildFirst = 3
    naInsert = 4
    naInsertBehind = 5

naAdd, naAddFirst, naAddChild, naAddChildFirst, naInsert, naInsertBehind = TNodeAttachMode.naAdd, TNodeAttachMode.naAddFirst, TNodeAttachMode.naAddChild, TNodeAttachMode.naAddChildFirst, TNodeAttachMode.naInsert, TNodeAttachMode.naInsertBehind


# 表示形式・並べ替え・列の文字の寄せ方・OnChange の変更の種類(LCL / VCL と同じ値)。
class TViewStyle(enum.IntEnum):
    vsIcon = 0
    vsSmallIcon = 1
    vsList = 2
    vsReport = 3

vsIcon, vsSmallIcon, vsList, vsReport = TViewStyle.vsIcon, TViewStyle.vsSmallIcon, TViewStyle.vsList, TViewStyle.vsReport


class TSortType(enum.IntEnum):
    stNone = 0
    stData = 1
    stText = 2
    stBoth = 3

stNone, stData, stText, stBoth = TSortType.stNone, TSortType.stData, TSortType.stText, TSortType.stBoth


class TSortDirection(enum.IntEnum):
    sdAscending = 0
    sdDescending = 1

sdAscending, sdDescending = TSortDirection.sdAscending, TSortDirection.sdDescending


class TItemChange(enum.IntEnum):
    ctText = 0
    ctImage = 1
    ctState = 2

ctText, ctImage, ctState = TItemChange.ctText, TItemChange.ctImage, TItemChange.ctState


# Splitter のドラッグ中の表示のしかた(寄せる辺の ResizeAnchor は TAnchorKind)。
class TResizeStyle(enum.IntEnum):
    rsLine = 0
    rsNone = 1
    rsPattern = 2
    rsUpdate = 3

rsLine, rsNone, rsPattern, rsUpdate = TResizeStyle.rsLine, TResizeStyle.rsNone, TResizeStyle.rsPattern, TResizeStyle.rsUpdate


# 枠線付きの表示専用テキスト(TLabel と異なりウィンドウを持つ)。
class TStaticBorderStyle(enum.IntEnum):
    sbsNone = 0
    sbsSingle = 1
    sbsSunken = 2

sbsNone, sbsSingle, sbsSunken = TStaticBorderStyle.sbsNone, TStaticBorderStyle.sbsSingle, TStaticBorderStyle.sbsSunken


# パネルの描き方(LCL の TStatusPanelStyle と同じ値)。psOwnerDraw なら TStatusBar::OnDrawPanel で描く。
class TStatusPanelStyle(enum.IntEnum):
    psText = 0
    psOwnerDraw = 1

psText, psOwnerDraw = TStatusPanelStyle.psText, TStatusPanelStyle.psOwnerDraw


# パネルの縁(LCL の TStatusPanelBevel と同じ値)。
class TStatusPanelBevel(enum.IntEnum):
    pbNone = 0
    pbLowered = 1
    pbRaised = 2

pbNone, pbLowered, pbRaised = TStatusPanelBevel.pbNone, TStatusPanelBevel.pbLowered, TStatusPanelBevel.pbRaised


# 矩形・楕円等の図形を描画する表示専用コントロール。Pen/Brush は TCanvas と同じく、
# コントロールが所有する実体への非所有のビュー(コントロールと寿命が一致する)。
class TShapeType(enum.IntEnum):
    stRectangle = 0
    stSquare = 1
    stRoundRect = 2
    stRoundSquare = 3
    stEllipse = 4
    stCircle = 5
    stSquaredDiamond = 6
    stDiamond = 7
    stTriangle = 8
    stTriangleLeft = 9
    stTriangleRight = 10
    stTriangleDown = 11
    stStar = 12
    stStarDown = 13
    stPolygon = 14

stRectangle, stSquare, stRoundRect, stRoundSquare, stEllipse, stCircle, stSquaredDiamond, stDiamond, stTriangle, stTriangleLeft, stTriangleRight, stTriangleDown, stStar, stStarDown, stPolygon = TShapeType.stRectangle, TShapeType.stSquare, TShapeType.stRoundRect, TShapeType.stRoundSquare, TShapeType.stEllipse, TShapeType.stCircle, TShapeType.stSquaredDiamond, TShapeType.stDiamond, TShapeType.stTriangle, TShapeType.stTriangleLeft, TShapeType.stTriangleRight, TShapeType.stTriangleDown, TShapeType.stStar, TShapeType.stStarDown, TShapeType.stPolygon


# ドラッグで幅を変えている間の段階(LCL の TSectionTrackState と同じ値)。
class TSectionTrackState(enum.IntEnum):
    tsTrackBegin = 0
    tsTrackMove = 1
    tsTrackEnd = 2

tsTrackBegin, tsTrackMove, tsTrackEnd = TSectionTrackState.tsTrackBegin, TSectionTrackState.tsTrackMove, TSectionTrackState.tsTrackEnd


# 縁の描き方(LCL の TEdgeStyle と同じ値)。
class TEdgeStyle(enum.IntEnum):
    esNone = 0
    esRaised = 1
    esLowered = 2

esNone, esRaised, esLowered = TEdgeStyle.esNone, TEdgeStyle.esRaised, TEdgeStyle.esLowered


# ツールボタンの種類(LCL の TToolButtonStyle と同じ値)。
class TToolButtonStyle(enum.IntEnum):
    tbsButton = 0  # 普通のボタン
    tbsCheck = 1  # クリックで Down が切り替わる(Grouped なら隣り合うボタンのどれか 1 つだけが Down)
    tbsDropDown = 2  # 右に矢印が付き、矢印で DropdownMenu を表示する
    tbsSeparator = 3  # 空白
    tbsDivider = 4  # 線の入った区切り
    tbsButtonDrop = 5  # ボタンと一体の矢印(どこを押しても DropdownMenu を表示する)

tbsButton, tbsCheck, tbsDropDown, tbsSeparator, tbsDivider, tbsButtonDrop = TToolButtonStyle.tbsButton, TToolButtonStyle.tbsCheck, TToolButtonStyle.tbsDropDown, TToolButtonStyle.tbsSeparator, TToolButtonStyle.tbsDivider, TToolButtonStyle.tbsButtonDrop


# バンドの左端のつまみの描き方(LCL の TGrabStyle と同じ値)。
class TGrabStyle(enum.IntEnum):
    gsSimple = 0
    gsDouble = 1
    gsHorLines = 2
    gsVerLines = 3
    gsGripper = 4
    gsButton = 5

gsSimple, gsDouble, gsHorLines, gsVerLines, gsGripper, gsButton = TGrabStyle.gsSimple, TGrabStyle.gsDouble, TGrabStyle.gsHorLines, TGrabStyle.gsVerLines, TGrabStyle.gsGripper, TGrabStyle.gsButton


# ActionList の状態(LCL の TActionListState と同じ値)。asSuspended は Action を実行・更新しない。
class TActionListState(enum.IntEnum):
    asNormal = 0
    asSuspended = 1
    asSuspendedEnabled = 2

asNormal, asSuspended, asSuspendedEnabled = TActionListState.asNormal, TActionListState.asSuspended, TActionListState.asSuspendedEnabled


TBorderStyle = TFormBorderStyle


# ---------------- 集合・定数 ----------------

class TShiftState(enum.IntFlag):
    ssShift = 0x0001
    ssAlt = 0x0002
    ssCtrl = 0x0004
    ssLeft = 0x0008  # マウスの左ボタンが押されている
    ssRight = 0x0010
    ssMiddle = 0x0020
    ssDouble = 0x0040  # ダブルクリックの一部として発生した
    ssMeta = 0x0080
    ssSuper = 0x0100
    ssHyper = 0x0200
    ssAltGr = 0x0400
    ssCaps = 0x0800
    ssNum = 0x1000
    ssScroll = 0x2000
    ssTriple = 0x4000
    ssQuad = 0x8000
    ssExtra1 = 0x10000
    ssExtra2 = 0x20000


ssShift = TShiftState.ssShift
ssAlt = TShiftState.ssAlt
ssCtrl = TShiftState.ssCtrl
ssLeft = TShiftState.ssLeft
ssRight = TShiftState.ssRight
ssMiddle = TShiftState.ssMiddle
ssDouble = TShiftState.ssDouble
ssMeta = TShiftState.ssMeta
ssSuper = TShiftState.ssSuper
ssHyper = TShiftState.ssHyper
ssAltGr = TShiftState.ssAltGr
ssCaps = TShiftState.ssCaps
ssNum = TShiftState.ssNum
ssScroll = TShiftState.ssScroll
ssTriple = TShiftState.ssTriple
ssQuad = TShiftState.ssQuad
ssExtra1 = TShiftState.ssExtra1
ssExtra2 = TShiftState.ssExtra2


class TFontStyles(enum.IntFlag):
    fsBold = 1 << 0
    fsItalic = 1 << 1
    fsUnderline = 1 << 2
    fsStrikeOut = 1 << 3


fsBold = TFontStyles.fsBold
fsItalic = TFontStyles.fsItalic
fsUnderline = TFontStyles.fsUnderline
fsStrikeOut = TFontStyles.fsStrikeOut


class TOwnerDrawState(enum.IntFlag):
    odSelected = 1 << 0
    odGrayed = 1 << 1
    odDisabled = 1 << 2
    odChecked = 1 << 3
    odFocused = 1 << 4
    odDefault = 1 << 5
    odHotLight = 1 << 6
    odInactive = 1 << 7
    odNoAccel = 1 << 8
    odNoFocusRect = 1 << 9
    odReserved1 = 1 << 10
    odReserved2 = 1 << 11
    odComboBoxEdit = 1 << 12
    odBackgroundPainted = 1 << 13


odSelected = TOwnerDrawState.odSelected
odGrayed = TOwnerDrawState.odGrayed
odDisabled = TOwnerDrawState.odDisabled
odChecked = TOwnerDrawState.odChecked
odFocused = TOwnerDrawState.odFocused
odDefault = TOwnerDrawState.odDefault
odHotLight = TOwnerDrawState.odHotLight
odInactive = TOwnerDrawState.odInactive
odNoAccel = TOwnerDrawState.odNoAccel
odNoFocusRect = TOwnerDrawState.odNoFocusRect
odReserved1 = TOwnerDrawState.odReserved1
odReserved2 = TOwnerDrawState.odReserved2
odComboBoxEdit = TOwnerDrawState.odComboBoxEdit
odBackgroundPainted = TOwnerDrawState.odBackgroundPainted


class TGridOptions(enum.IntFlag):
    goFixedVertLine = 1 << 0
    goFixedHorzLine = 1 << 1
    goVertLine = 1 << 2
    goHorzLine = 1 << 3
    goRangeSelect = 1 << 4
    goDrawFocusSelected = 1 << 5
    goRowSizing = 1 << 6
    goColSizing = 1 << 7
    goRowMoving = 1 << 8
    goColMoving = 1 << 9
    goEditing = 1 << 10
    goAutoAddRows = 1 << 11
    goTabs = 1 << 12
    goRowSelect = 1 << 13
    goAlwaysShowEditor = 1 << 14
    goThumbTracking = 1 << 15
    goColSpanning = 1 << 16
    goRelaxedRowSelect = 1 << 17
    goDblClickAutoSize = 1 << 18
    goSmoothScroll = 1 << 19
    goFixedRowNumbering = 1 << 20
    goScrollKeepVisible = 1 << 21
    goHeaderHotTracking = 1 << 22
    goHeaderPushedLook = 1 << 23
    goSelectionActive = 1 << 24
    goFixedColSizing = 1 << 25
    goDontScrollPartCell = 1 << 26
    goCellHints = 1 << 27
    goTruncCellHints = 1 << 28
    goCellEllipsis = 1 << 29
    goAutoAddRowsSkipContentCheck = 1 << 30
    goRowHighlight = 1 << 31


goFixedVertLine = TGridOptions.goFixedVertLine
goFixedHorzLine = TGridOptions.goFixedHorzLine
goVertLine = TGridOptions.goVertLine
goHorzLine = TGridOptions.goHorzLine
goRangeSelect = TGridOptions.goRangeSelect
goDrawFocusSelected = TGridOptions.goDrawFocusSelected
goRowSizing = TGridOptions.goRowSizing
goColSizing = TGridOptions.goColSizing
goRowMoving = TGridOptions.goRowMoving
goColMoving = TGridOptions.goColMoving
goEditing = TGridOptions.goEditing
goAutoAddRows = TGridOptions.goAutoAddRows
goTabs = TGridOptions.goTabs
goRowSelect = TGridOptions.goRowSelect
goAlwaysShowEditor = TGridOptions.goAlwaysShowEditor
goThumbTracking = TGridOptions.goThumbTracking
goColSpanning = TGridOptions.goColSpanning
goRelaxedRowSelect = TGridOptions.goRelaxedRowSelect
goDblClickAutoSize = TGridOptions.goDblClickAutoSize
goSmoothScroll = TGridOptions.goSmoothScroll
goFixedRowNumbering = TGridOptions.goFixedRowNumbering
goScrollKeepVisible = TGridOptions.goScrollKeepVisible
goHeaderHotTracking = TGridOptions.goHeaderHotTracking
goHeaderPushedLook = TGridOptions.goHeaderPushedLook
goSelectionActive = TGridOptions.goSelectionActive
goFixedColSizing = TGridOptions.goFixedColSizing
goDontScrollPartCell = TGridOptions.goDontScrollPartCell
goCellHints = TGridOptions.goCellHints
goTruncCellHints = TGridOptions.goTruncCellHints
goCellEllipsis = TGridOptions.goCellEllipsis
goAutoAddRowsSkipContentCheck = TGridOptions.goAutoAddRowsSkipContentCheck
goRowHighlight = TGridOptions.goRowHighlight


class TGridDrawState(enum.IntFlag):
    gdSelected = 0x01
    gdFocused = 0x02
    gdFixed = 0x04
    gdHot = 0x08
    gdPushed = 0x10
    gdRowHighlight = 0x20


gdSelected = TGridDrawState.gdSelected
gdFocused = TGridDrawState.gdFocused
gdFixed = TGridDrawState.gdFixed
gdHot = TGridDrawState.gdHot
gdPushed = TGridDrawState.gdPushed
gdRowHighlight = TGridDrawState.gdRowHighlight


class TEdgeBorders(enum.IntFlag):
    ebLeft = 0x01
    ebTop = 0x02
    ebRight = 0x04
    ebBottom = 0x08


ebLeft = TEdgeBorders.ebLeft
ebTop = TEdgeBorders.ebTop
ebRight = TEdgeBorders.ebRight
ebBottom = TEdgeBorders.ebBottom


class TOpenOptions(enum.IntFlag):
    ofReadOnly = 1 << 0
    ofOverwritePrompt = 1 << 1  # TSaveDialog: 既存のファイルなら上書きを確かめる
    ofHideReadOnly = 1 << 2
    ofNoChangeDir = 1 << 3
    ofShowHelp = 1 << 4
    ofNoValidate = 1 << 5
    ofAllowMultiSelect = 1 << 6  # 複数のファイルを選べる(Files で受け取る)
    ofExtensionDifferent = 1 << 7
    ofPathMustExist = 1 << 8
    ofFileMustExist = 1 << 9
    ofCreatePrompt = 1 << 10
    ofShareAware = 1 << 11
    ofNoReadOnlyReturn = 1 << 12
    ofNoTestFileCreate = 1 << 13
    ofNoNetworkButton = 1 << 14
    ofNoLongNames = 1 << 15
    ofOldStyleDialog = 1 << 16
    ofNoDereferenceLinks = 1 << 17
    ofNoResolveLinks = 1 << 18
    ofEnableIncludeNotify = 1 << 19
    ofEnableSizing = 1 << 20
    ofDontAddToRecent = 1 << 21
    ofForceShowHidden = 1 << 22
    ofViewDetail = 1 << 23
    ofAutoPreview = 1 << 24


ofReadOnly = TOpenOptions.ofReadOnly
ofOverwritePrompt = TOpenOptions.ofOverwritePrompt
ofHideReadOnly = TOpenOptions.ofHideReadOnly
ofNoChangeDir = TOpenOptions.ofNoChangeDir
ofShowHelp = TOpenOptions.ofShowHelp
ofNoValidate = TOpenOptions.ofNoValidate
ofAllowMultiSelect = TOpenOptions.ofAllowMultiSelect
ofExtensionDifferent = TOpenOptions.ofExtensionDifferent
ofPathMustExist = TOpenOptions.ofPathMustExist
ofFileMustExist = TOpenOptions.ofFileMustExist
ofCreatePrompt = TOpenOptions.ofCreatePrompt
ofShareAware = TOpenOptions.ofShareAware
ofNoReadOnlyReturn = TOpenOptions.ofNoReadOnlyReturn
ofNoTestFileCreate = TOpenOptions.ofNoTestFileCreate
ofNoNetworkButton = TOpenOptions.ofNoNetworkButton
ofNoLongNames = TOpenOptions.ofNoLongNames
ofOldStyleDialog = TOpenOptions.ofOldStyleDialog
ofNoDereferenceLinks = TOpenOptions.ofNoDereferenceLinks
ofNoResolveLinks = TOpenOptions.ofNoResolveLinks
ofEnableIncludeNotify = TOpenOptions.ofEnableIncludeNotify
ofEnableSizing = TOpenOptions.ofEnableSizing
ofDontAddToRecent = TOpenOptions.ofDontAddToRecent
ofForceShowHidden = TOpenOptions.ofForceShowHidden
ofViewDetail = TOpenOptions.ofViewDetail
ofAutoPreview = TOpenOptions.ofAutoPreview


class TColorDialogOptions(enum.IntFlag):
    cdFullOpen = 1 << 0  # 色の作成の部分を最初から開く
    cdPreventFullOpen = 1 << 1  # 色の作成のボタンを無効にする
    cdShowHelp = 1 << 2
    cdSolidColor = 1 << 3
    cdAnyColor = 1 << 4


cdFullOpen = TColorDialogOptions.cdFullOpen
cdPreventFullOpen = TColorDialogOptions.cdPreventFullOpen
cdShowHelp = TColorDialogOptions.cdShowHelp
cdSolidColor = TColorDialogOptions.cdSolidColor
cdAnyColor = TColorDialogOptions.cdAnyColor


class TFontDialogOptions(enum.IntFlag):
    fdAnsiOnly = 1 << 0
    fdTrueTypeOnly = 1 << 1
    fdEffects = 1 << 2
    fdFixedPitchOnly = 1 << 3
    fdForceFontExist = 1 << 4
    fdNoFaceSel = 1 << 5
    fdNoOEMFonts = 1 << 6
    fdNoSimulations = 1 << 7
    fdNoSizeSel = 1 << 8
    fdNoStyleSel = 1 << 9
    fdNoVectorFonts = 1 << 10
    fdShowHelp = 1 << 11
    fdWysiwyg = 1 << 12
    fdLimitSize = 1 << 13  # MinFontSize・MaxFontSize で大きさを制限する
    fdScalableOnly = 1 << 14
    fdApplyButton = 1 << 15


fdAnsiOnly = TFontDialogOptions.fdAnsiOnly
fdTrueTypeOnly = TFontDialogOptions.fdTrueTypeOnly
fdEffects = TFontDialogOptions.fdEffects
fdFixedPitchOnly = TFontDialogOptions.fdFixedPitchOnly
fdForceFontExist = TFontDialogOptions.fdForceFontExist
fdNoFaceSel = TFontDialogOptions.fdNoFaceSel
fdNoOEMFonts = TFontDialogOptions.fdNoOEMFonts
fdNoSimulations = TFontDialogOptions.fdNoSimulations
fdNoSizeSel = TFontDialogOptions.fdNoSizeSel
fdNoStyleSel = TFontDialogOptions.fdNoStyleSel
fdNoVectorFonts = TFontDialogOptions.fdNoVectorFonts
fdShowHelp = TFontDialogOptions.fdShowHelp
fdWysiwyg = TFontDialogOptions.fdWysiwyg
fdLimitSize = TFontDialogOptions.fdLimitSize
fdScalableOnly = TFontDialogOptions.fdScalableOnly
fdApplyButton = TFontDialogOptions.fdApplyButton


class TFindOptions(enum.IntFlag):
    frDown = 1 << 0  # 下へ検索する
    frFindNext = 1 << 1
    frHideMatchCase = 1 << 2
    frHideWholeWord = 1 << 3
    frHideUpDown = 1 << 4
    frMatchCase = 1 << 5  # 大文字と小文字を区別する
    frDisableMatchCase = 1 << 6
    frDisableUpDown = 1 << 7
    frDisableWholeWord = 1 << 8
    frReplace = 1 << 9
    frReplaceAll = 1 << 10
    frWholeWord = 1 << 11  # 単語単位で探す
    frShowHelp = 1 << 12
    frEntireScope = 1 << 13
    frHideEntireScope = 1 << 14
    frPromptOnReplace = 1 << 15
    frHidePromptOnReplace = 1 << 16
    frButtonsAtBottom = 1 << 17


frDown = TFindOptions.frDown
frFindNext = TFindOptions.frFindNext
frHideMatchCase = TFindOptions.frHideMatchCase
frHideWholeWord = TFindOptions.frHideWholeWord
frHideUpDown = TFindOptions.frHideUpDown
frMatchCase = TFindOptions.frMatchCase
frDisableMatchCase = TFindOptions.frDisableMatchCase
frDisableUpDown = TFindOptions.frDisableUpDown
frDisableWholeWord = TFindOptions.frDisableWholeWord
frReplace = TFindOptions.frReplace
frReplaceAll = TFindOptions.frReplaceAll
frWholeWord = TFindOptions.frWholeWord
frShowHelp = TFindOptions.frShowHelp
frEntireScope = TFindOptions.frEntireScope
frHideEntireScope = TFindOptions.frHideEntireScope
frPromptOnReplace = TFindOptions.frPromptOnReplace
frHidePromptOnReplace = TFindOptions.frHidePromptOnReplace
frButtonsAtBottom = TFindOptions.frButtonsAtBottom


TColor = int
clBlack = 0x000000  # 色の定数(LCL・VCL と同じ値)。標準の 16 色と、LCL の追加の 4 色(docs/adr/0050 で加えた)。
clMaroon = 0x000080
clGreen = 0x008000
clOlive = 0x008080
clNavy = 0x800000
clPurple = 0x800080
clTeal = 0x808000
clGray = 0x808080
clSilver = 0xC0C0C0
clRed = 0x0000FF
clLime = 0x00FF00
clYellow = 0x00FFFF
clBlue = 0xFF0000
clFuchsia = 0xFF00FF
clAqua = 0xFFFF00
clWhite = 0xFFFFFF
clMoneyGreen = 0xC0DCC0
clSkyBlue = 0xF0CAA6
clCream = 0xF0FBFF
clMedGray = 0xA4A0A0
clScrollBar = -0x80000000  # システムの色(docs/adr/0050)。Windows の設定の色(0x80000000 | Windows の COLOR_… の番号)。描くときに実際の色になる。 選択された項目の地の色は clHighlight、文字は clHighlightText 等。TColor は符号付きのため、値は負の数で書く。
clBackground = -0x7FFFFFFF
clActiveCaption = -0x7FFFFFFE
clInactiveCaption = -0x7FFFFFFD
clMenu = -0x7FFFFFFC
clWindow = -0x7FFFFFFB
clWindowFrame = -0x7FFFFFFA
clMenuText = -0x7FFFFFF9
clWindowText = -0x7FFFFFF8
clCaptionText = -0x7FFFFFF7
clActiveBorder = -0x7FFFFFF6
clInactiveBorder = -0x7FFFFFF5
clAppWorkspace = -0x7FFFFFF4
clHighlight = -0x7FFFFFF3
clHighlightText = -0x7FFFFFF2
clBtnFace = -0x7FFFFFF1
clBtnShadow = -0x7FFFFFF0
clGrayText = -0x7FFFFFEF
clBtnText = -0x7FFFFFEE
clInactiveCaptionText = -0x7FFFFFED
clBtnHighlight = -0x7FFFFFEC
cl3DDkShadow = -0x7FFFFFEB
cl3DLight = -0x7FFFFFEA
clInfoText = -0x7FFFFFE9
clInfoBk = -0x7FFFFFE8
clHotLight = -0x7FFFFFE6
clGradientActiveCaption = -0x7FFFFFE5
clGradientInactiveCaption = -0x7FFFFFE4
clMenuHighlight = -0x7FFFFFE3
clMenuBar = -0x7FFFFFE2
clForm = -0x7FFFFFE1
clNone = 0x1FFFFFFF  # 色を持たない(LCL の clNone)。
clDefault = 0x20000000  # 既定の色(LCL の clDefault)。コントロールの Color の既定値で、実際の色はウィジェットセットが決める。

TShortCut = int
scShift = 0x2000
scCtrl = 0x4000
scAlt = 0x8000

TCursor = int
crDefault = 0
crNone = -1
crArrow = -2
crCross = -3
crIBeam = -4
crSizeNESW = -6
crSizeNS = -7
crSizeNWSE = -8
crSizeWE = -9
crUpArrow = -10
crHourGlass = -11
crDrag = -12
crNoDrop = -13
crHSplit = -14
crVSplit = -15
crMultiDrag = -16
crSQLWait = -17
crNo = -18
crAppStart = -19
crHelp = -20
crHandPoint = -21
crSizeAll = -22
crSize = -22
crSizeNW = -23
crSizeN = -24
crSizeNE = -25
crSizeW = -26
crSizeE = -27
crSizeSW = -28
crSizeS = -29
crSizeSE = -30

TModalResult = int
mrNone = 0
mrOk = 1
mrCancel = 2
mrAbort = 3
mrRetry = 4
mrIgnore = 5
mrYes = 6
mrNo = 7
mrAll = 8
mrNoToAll = 9
mrYesToAll = 10
mrClose = 11

TClipboardFormat = int

mbYesNo = frozenset({mbYes, mbNo})  # TMsgDlgButtons
mbYesNoCancel = frozenset({mbYes, mbNo, mbCancel})  # TMsgDlgButtons
mbOKCancel = frozenset({mbOK, mbCancel})  # TMsgDlgButtons
mbAbortRetryIgnore = frozenset({mbAbort, mbRetry, mbIgnore})  # TMsgDlgButtons


class TStrings(_mixins["TStrings"], TPersistent):
    """文字列の一覧(LCL の TStrings)。コントロールの Items・Lines・Tabs 等として、所有者の値メンバで持つ非所有のビュー
    (ListBox1->Items->Add("x"); ListBox1->Items->Strings[0]; Memo1->Lines->Text = "..."; のように VCL と同じく使う)。
    LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがある(TListBox・TComboBox・TMemo)ため、
    このビューは中身のハンドルを覚えず、操作のたびに所有者から取得する。そのため Handle() は nullptr を返す
    (DLL の関数に渡すハンドルは Current() で得る。保存しないこと)。
    利用者が生成する文字列の一覧は、派生の TStringList を使う(docs/adr/0028)。"""
    Count = _Prop("TStrings_GetCount", None, _int)
    Strings = _Indexed("TStrings_GetStrings", "TStrings_SetStrings", _str)
    # 利用者データ(ポインタ)。LCL は解釈も解放もしない。
    Objects = _Indexed("TStrings_GetObjects", "TStrings_SetObjects", _ptr)
    # すべての行を改行でつないだ文字列。設定すると改行で分けて置き換える。
    Text = _Prop("TStrings_GetText", "TStrings_SetText", _str)
    # カンマ区切りの文字列(空白・カンマを含む要素は二重引用符で囲まれる)。
    CommaText = _Prop("TStrings_GetCommaText", "TStrings_SetCommaText", _str)
    # 名前=値 の形の行。Names[i] は '=' より前、ValueFromIndex[i] は後ろ。
    # Values["name"] は name の行の値で、無ければ空文字列。無い名前に代入すると末尾に追加する。
    # 空文字列を代入したとき、ValueFromIndex[i] はその行を削除するが、Values["name"] は値を空にするだけで行は残る
    # (LCL(FPC)の仕様。VCL の Values は行を削除する)。
    Names = _Indexed("TStrings_GetNames", None, _str)
    Values = _Indexed("TStrings_GetValues", "TStrings_SetValues", _str, key=_str_key)
    ValueFromIndex = _Indexed("TStrings_GetValueFromIndex", "TStrings_SetValueFromIndex", _str)
    # Delimiter で区切った文字列(既定は ',')。StrictDelimiter が false(既定)なら、空白も区切りとして扱い、
    # 空白・区切り文字を含む要素は二重引用符で囲まれる。
    Delimiter = _Prop("TStrings_GetDelimiter", "TStrings_SetDelimiter", _char)
    StrictDelimiter = _Prop("TStrings_GetStrictDelimiter", "TStrings_SetStrictDelimiter", _bool)
    DelimitedText = _Prop("TStrings_GetDelimitedText", "TStrings_SetDelimitedText", _str)
    # 末尾に追加し、追加した位置を返す(ソートされた一覧では挿入された位置)。
    def Add(self, S):
        _r = lib.TStrings_Add(self._current(), _enc(S))
        return _r
    def AddObject(self, S, AObject):
        _r = lib.TStrings_AddObject(self._current(), _enc(S), AObject)
        return _r
    def Insert(self, Index, S):
        lib.TStrings_Insert(self._current(), int(Index), _enc(S))
    def Delete(self, Index):
        lib.TStrings_Delete(self._current(), int(Index))
    def Clear(self):
        lib.TStrings_Clear(self._current())
    # 見つからなければ -1。
    def IndexOf(self, S):
        _r = lib.TStrings_IndexOf(self._current(), _enc(S))
        return _r
    def Exchange(self, Index1, Index2):
        lib.TStrings_Exchange(self._current(), int(Index1), int(Index2))
    def Move(self, CurIndex, NewIndex):
        lib.TStrings_Move(self._current(), int(CurIndex), int(NewIndex))
    def BeginUpdate(self):
        lib.TStrings_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TStrings_EndUpdate(self._current())
    # Source の内容(文字列と Objects)で置き換える / 末尾に加える。
    def Assign(self, Source):
        lib.TStrings_Assign(self._current(), _h(Source))
    # 名前=値 の行のうち、名前が Name の行の位置。見つからなければ -1。
    def IndexOfName(self, Name):
        _r = lib.TStrings_IndexOfName(self._current(), _enc(Name))
        return _r
    # ファイル名・内容とも UTF-8 のまま扱う(文字コードの変換はしない)。
    def LoadFromFile(self, FileName):
        lib.TStrings_LoadFromFile(self._current(), _enc(FileName))
    def SaveToFile(self, FileName):
        lib.TStrings_SaveToFile(self._current(), _enc(FileName))


class TStringList(_mixins["TStringList"], TStrings):
    """利用者が生成する文字列の一覧(LCL の TStringList)。TComponent ではないため、Owner も破棄通知も無く、
    生成した側が破棄する(VCL と同じく new して delete する。スタックや値メンバに置いてもよい)。
    TStrings* を受け取るもの(ListBox1->Items->Assign(List) 等)にそのまま渡せる。"""
    def __init__(self):
        self._init_owned(lib.TStringList_Create())
    # true にすると並べ替え、以降の Add はソート順の位置に入る(Insert と Strings への代入は Exception を送出する)。
    Sorted = _Prop("TStringList_GetSorted", "TStringList_SetSorted", _bool)
    # Sorted のときの重複の扱い(既定は dupIgnore で、重複は加えない。dupError で重複を加えると Exception を送出する)。
    Duplicates = _Prop("TStringList_GetDuplicates", "TStringList_SetDuplicates", _enum("TDuplicates"))
    # 並べ替え・IndexOf・Find で大文字と小文字を区別するか(既定は false)。
    CaseSensitive = _Prop("TStringList_GetCaseSensitive", "TStringList_SetCaseSensitive", _bool)
    def Sort(self):
        lib.TStringList_Sort(self._current())
    # ソートされた一覧から S を二分探索する。見つからなければ、S を挿入すべき位置を Index に入れて false を返す。
    # Sorted が false の一覧には使えない(Exception を送出する)。
    def Find(self, S, Index):
        _out_Index = ctypes.c_int()
        _r = lib.TStringList_Find(self._current(), _enc(S), ctypes.byref(_out_Index))
        Index.value = _out_Index.value
        return _r != 0


class TPen(TPersistent):
    Color = _Prop("TPen_GetColor", "TPen_SetColor", _int)
    Width = _Prop("TPen_GetWidth", "TPen_SetWidth", _int)
    Style = _Prop("TPen_GetStyle", "TPen_SetStyle", _enum("TPenStyle"))
    Mode = _Prop("TPen_GetMode", "TPen_SetMode", _enum("TPenMode"))


class TBrush(TPersistent):
    Color = _Prop("TBrush_GetColor", "TBrush_SetColor", _int)
    Style = _Prop("TBrush_GetStyle", "TBrush_SetStyle", _enum("TBrushStyle"))


class TFont(TPersistent):
    """Canvas・コントロール・TFontDialog が持つフォントへの非所有のラッパー(Style・Assign は docs/adr/0033)。"""
    Name = _Prop("TFont_GetName", "TFont_SetName", _str)
    Size = _Prop("TFont_GetSize", "TFont_SetSize", _int)
    Color = _Prop("TFont_GetColor", "TFont_SetColor", _int)
    Style = _Prop("TFont_GetStyle", "TFont_SetStyle", _enum("TFontStyles"))
    # 文字の高さ(ピクセル)。負の値は文字の高さ、正の値はセルの高さ(内部の余白を含む)。0 は既定。Size と連動する。
    Height = _Prop("TFont_GetHeight", "TFont_SetHeight", _int)
    # 文字の傾き(0.1 度単位。反時計回り。900 で縦書きの向き)。
    Orientation = _Prop("TFont_GetOrientation", "TFont_SetOrientation", _int)
    Quality = _Prop("TFont_GetQuality", "TFont_SetQuality", _enum("TFontQuality"))
    # Source の内容(Name・Size・Color・Style 等)を写す(VCL の Font->Assign)。nullptr なら何もしない。
    def Assign(self, Source):
        lib.TFont_Assign(self._current(), _h(Source))


class TCanvas(_mixins["TCanvas"], TPersistent):
    Pen = _Prop("TCanvas_GetPen", None, _obj("TPen"))
    Brush = _Prop("TCanvas_GetBrush", None, _obj("TBrush"))
    Font = _Prop("TCanvas_GetFont", None, _obj("TFont"))
    # 1 画素の色(Canvas->Pixels[X][Y]。VCL の Pixels[X, Y])。
    Pixels = _Indexed("TCanvas_GetPixels", "TCanvas_SetPixels", _int, dims=2)
    def MoveTo(self, x, y):
        lib.TCanvas_MoveTo(self._current(), int(x), int(y))
    def LineTo(self, x, y):
        lib.TCanvas_LineTo(self._current(), int(x), int(y))
    def Rectangle(self, x1, y1, x2, y2):
        lib.TCanvas_Rectangle(self._current(), int(x1), int(y1), int(x2), int(y2))
    def Ellipse(self, x1, y1, x2, y2):
        lib.TCanvas_Ellipse(self._current(), int(x1), int(y1), int(x2), int(y2))
    def TextOut(self, x, y, text):
        lib.TCanvas_TextOut(self._current(), int(x), int(y), _enc(text))
    # Brush で塗りつぶす(枠は描かない)。
    def FillRect(self, Rect):
        lib.TCanvas_FillRect(self._current(), *_rect(Rect))
    # ---- docs/adr/0045 ----
    # Text を今の Font で描いたときの幅・高さ(ピクセル)。
    def TextWidth(self, Text):
        _r = lib.TCanvas_TextWidth(self._current(), _enc(Text))
        return _r
    def TextHeight(self, Text):
        _r = lib.TCanvas_TextHeight(self._current(), _enc(Text))
        return _r
    # Rect の中だけに、(X, Y) から Text を描く(はみ出した部分は切り取る)。
    def TextRect(self, Rect, X, Y, Text):
        lib.TCanvas_TextRect(self._current(), *_rect(Rect), int(X), int(Y), _enc(Text))
    # 角の丸い矩形(RX・RY は角の楕円の幅・高さ)。
    def RoundRect(self, X1, Y1, X2, Y2, RX, RY):
        lib.TCanvas_RoundRect(self._current(), int(X1), int(Y1), int(X2), int(Y2), int(RX), int(RY))
    # (X1, Y1)-(X2, Y2) に内接する楕円の、中心から (X3, Y3) の方向から (X4, Y4) の方向まで(反時計回り)の弧・扇形・弓形。
    def Arc(self, X1, Y1, X2, Y2, X3, Y3, X4, Y4):
        lib.TCanvas_Arc(self._current(), int(X1), int(Y1), int(X2), int(Y2), int(X3), int(Y3), int(X4), int(Y4))
    def Pie(self, X1, Y1, X2, Y2, X3, Y3, X4, Y4):
        lib.TCanvas_Pie(self._current(), int(X1), int(Y1), int(X2), int(Y2), int(X3), int(Y3), int(X4), int(Y4))
    def Chord(self, X1, Y1, X2, Y2, X3, Y3, X4, Y4):
        lib.TCanvas_Chord(self._current(), int(X1), int(Y1), int(X2), int(Y2), int(X3), int(Y3), int(X4), int(Y4))
    # Rect の縁を Brush の色で 1 ピクセルの幅で描く(中は描かない)。
    def FrameRect(self, Rect):
        lib.TCanvas_FrameRect(self._current(), *_rect(Rect))


class TGraphic(_mixins["TGraphic"], TPersistent):
    """グラフィック(LCL の TGraphic。TPersistent で、TComponent ではない)。2 通りの持ち方がある。
    - 利用者が生成するもの(new TBitmap 等): VCL と同じく delete で破棄する(LCL のオブジェクトも破棄される)。
    スタックや値メンバに置いてもよい。
    - 所有者の中身のビュー(Image1->Picture->Bitmap・BitBtn1->Glyph 等): TStrings と同じく中身のハンドルを覚えず、
    操作のたびに所有者から取得する(TPicture は LoadFromFile 等のたびに中身を作り直すため)。Handle() は nullptr を返す
    (DLL の関数に渡すハンドルは Current() で得る。保存しないこと)。
    Picture->Graphic・Glyph 等への代入は、LCL と同じく内容のコピーになる(代入したものは代入した側の持ち物のまま)。
    読み込めないファイル・形式の違うファイルでは Exception(EFOpenError 等)が送出される。"""
    Width = _Prop("TGraphic_GetWidth", "TGraphic_SetWidth", _int)
    Height = _Prop("TGraphic_GetHeight", "TGraphic_SetHeight", _int)
    Empty = _Prop("TGraphic_GetEmpty", None, _bool)
    Transparent = _Prop("TGraphic_GetTransparent", "TGraphic_SetTransparent", _bool)
    # 形式はクラスで決まる(TBitmap に PNG のファイルは読めない)。拡張子で形式を選ぶのは TPicture::LoadFromFile。
    # ファイル名は UTF-8。
    def LoadFromFile(self, FileName):
        lib.TGraphic_LoadFromFile(self._current(), _enc(FileName))
    def SaveToFile(self, FileName):
        lib.TGraphic_SaveToFile(self._current(), _enc(FileName))
    def Clear(self):
        lib.TGraphic_Clear(self._current())


class TRasterImage(TGraphic):
    """TBitmap・TPortableNetworkGraphic・TJPEGImage の共通の基底(LCL の TRasterImage)。"""
    # グラフィックに描く先。グラフィックが所有し、中身が作り直されると別のものになる(ポインタを保存しないこと)。
    Canvas = _Prop("TRasterImage_GetCanvas", None, _obj("TCanvas"))
    PixelFormat = _Prop("TRasterImage_GetPixelFormat", "TRasterImage_SetPixelFormat", _enum("TPixelFormat"))
    TransparentColor = _Prop("TRasterImage_GetTransparentColor", "TRasterImage_SetTransparentColor", _int)
    TransparentMode = _Prop("TRasterImage_GetTransparentMode", "TRasterImage_SetTransparentMode", _enum("TTransparentMode"))


class TCustomBitmap(TRasterImage):
    def SetSize(self, AWidth, AHeight):
        lib.TCustomBitmap_SetSize(self._current(), int(AWidth), int(AHeight))


class TBitmap(TCustomBitmap):
    """ビットマップ(.bmp)。new TBitmap で生成して delete で破棄する(VCL と同じ)。"""
    def __init__(self):
        self._init_owned(lib.TBitmap_Create())


class TPortableNetworkGraphic(TCustomBitmap):
    """PNG 画像(.png)。LCL の TPortableNetworkGraphic(VCL の TPngImage に当たる)。"""
    def __init__(self):
        self._init_owned(lib.TPortableNetworkGraphic_Create())


class TJPEGImage(TCustomBitmap):
    """JPEG 画像(.jpg)。"""
    def __init__(self):
        self._init_owned(lib.TJPEGImage_Create())
    # 保存するときの品質(1〜100。既定は 75)。
    CompressionQuality = _Prop("TJPEGImage_GetCompressionQuality", "TJPEGImage_SetCompressionQuality", _int)


class TIcon(TRasterImage):
    """アイコン(.ico。docs/adr/0047)。LCL の TIcon(TCustomIcon の派生)。new TIcon で生成して delete で破棄する(TBitmap と同じ)。
    Form1->Icon・Application->Icon・Picture->Icon は所有者の中身のビュー。1 つのファイルに大きさの違う画像を複数持てる。"""
    def __init__(self):
        self._init_owned(lib.TIcon_Create())


class TPicture(_mixins["TPicture"], TPersistent):
    """形式を問わない画像の入れ物(LCL の TPicture)。Image1->Picture のように画像コントロールが持つもの(コントロールと寿命が一致する)と、
    利用者が new TPicture で生成して delete で破棄するものがある。"""
    def __init__(self):
        self._init_owned(lib.TPicture_Create())
    Width = _Prop("TPicture_GetWidth", None, _int)
    Height = _Prop("TPicture_GetHeight", None, _int)
    # 拡張子から形式(クラス)を選んで読み込む(.bmp・.png・.jpg 等)。ファイル名は UTF-8。
    def LoadFromFile(self, FileName):
        lib.TPicture_LoadFromFile(self._current(), _enc(FileName))
    def SaveToFile(self, FileName):
        lib.TPicture_SaveToFile(self._current(), _enc(FileName))
    def Clear(self):
        lib.TPicture_Clear(self._current())


class TCustomImageList(TComponent):
    """同じ大きさの画像の一覧(LCL の TCustomImageList)。TComponent なので、他のコンポーネントと同じく new で生成し、
    Owner に任せるか Free() で破棄する。ツリービュー・ツールバー等の Images に設定し、項目の ImageIndex で画像を選ぶ。
    画像を受け取るメソッドは、画像を写して加える(渡したグラフィックは呼び出し側の持ち物のまま)。
    Add・Insert 等は、画像を Width・Height の大きさに伸縮して 1 つとして加える(VCL と違い、幅が Width の倍数でも分けない)。
    横に並んだ複数の画像を分けて加えるのは AddSliced。
    画像リストを破棄すると(Free()・Owner の破棄)、それを Images 等に設定していたコントロールの Images は LCL が nullptr に戻す。"""
    # 画像の大きさ(既定は 16x16)。
    Width = _Prop("TCustomImageList_GetWidth", "TCustomImageList_SetWidth", _int)
    Height = _Prop("TCustomImageList_GetHeight", "TCustomImageList_SetHeight", _int)
    Count = _Prop("TCustomImageList_GetCount", None, _int)
    Masked = _Prop("TCustomImageList_GetMasked", "TCustomImageList_SetMasked", _bool)
    BkColor = _Prop("TCustomImageList_GetBkColor", "TCustomImageList_SetBkColor", _int)
    DrawingStyle = _Prop("TCustomImageList_GetDrawingStyle", "TCustomImageList_SetDrawingStyle", _enum("TDrawingStyle"))
    # Clear・Delete・Move・BkColor の変更で呼ばれる(LCL の仕様で、Add・Insert 等では呼ばれない。
    # BeginUpdate の間は EndUpdate まで遅れる)。
    OnChange = _Event("TCustomImageList_SetOnChange", "TNotifyEvent")
    # Mask は nullptr でよい。
    def Add(self, Image, Mask):
        _r = lib.TCustomImageList_Add(self._current(), _h(Image), _h(Mask))
        return _r
    # Image を横 AHorizontalCount・縦 AVerticalCount に分けて、それぞれを画像として加える。加えた最初の画像の位置を返す。
    def AddSliced(self, Image, AHorizontalCount, AVerticalCount):
        _r = lib.TCustomImageList_AddSliced(self._current(), _h(Image), int(AHorizontalCount), int(AVerticalCount))
        return _r
    # MaskColor の画素を透明として加える。
    def AddMasked(self, Image, MaskColor):
        _r = lib.TCustomImageList_AddMasked(self._current(), _h(Image), int(MaskColor))
        return _r
    def Insert(self, Index, Image, Mask):
        lib.TCustomImageList_Insert(self._current(), int(Index), _h(Image), _h(Mask))
    def Replace(self, Index, Image, Mask):
        lib.TCustomImageList_Replace(self._current(), int(Index), _h(Image), _h(Mask))
    def Delete(self, Index):
        lib.TCustomImageList_Delete(self._current(), int(Index))
    def Clear(self):
        lib.TCustomImageList_Clear(self._current())
    def Move(self, CurIndex, NewIndex):
        lib.TCustomImageList_Move(self._current(), int(CurIndex), int(NewIndex))
    # Index 番目の画像を Image に写す。
    def GetBitmap(self, Index, Image):
        lib.TCustomImageList_GetBitmap(self._current(), int(Index), _h(Image))
    # Canvas の (X, Y) に Index 番目の画像を描く。Enabled が false なら無効の見た目で描く。
    def Draw(self, Canvas, X, Y, Index, Enabled=True):
        lib.TCustomImageList_Draw(self._current(), _h(Canvas), int(X), int(Y), int(Index), _b(Enabled))
    def BeginUpdate(self):
        lib.TCustomImageList_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TCustomImageList_EndUpdate(self._current())


class TImageList(TCustomImageList):
    """LCL の TImageList は TDragImageList(ドラッグ中の画像の表示)の派生だが、その機能は公開していないため省いた。"""
    def __init__(self, AOwner):
        self._attach(lib.TImageList_Create(_h(AOwner)))


class TMenuItem(TComponent):
    """メニューの項目。TControl ではない(Parent/Left 等は無く、画面上の親子関係は Add/Insert で組む)。
    子の項目は LCL の Items[Index] / Count に合わせ、Items[Index] / Count で参照する。
    親の項目が破棄されると、子の項目も(Owner が別でも)一緒に破棄される(LCL の仕様。ラッパーも delete される)。"""
    def __init__(self, AOwner):
        self._attach(lib.TMenuItem_Create(_h(AOwner)))
    # "-" を設定すると区切り線になる。
    Caption = _Prop("TMenuItem_GetCaption", "TMenuItem_SetCaption", _str)
    Checked = _Prop("TMenuItem_GetChecked", "TMenuItem_SetChecked", _bool)
    Enabled = _Prop("TMenuItem_GetEnabled", "TMenuItem_SetEnabled", _bool)
    Visible = _Prop("TMenuItem_GetVisible", "TMenuItem_SetVisible", _bool)
    # true にすると、選ばれるたびに Checked が反転する(RadioItem なら同じ GroupIndex の他の項目が外れる)。
    AutoCheck = _Prop("TMenuItem_GetAutoCheck", "TMenuItem_SetAutoCheck", _bool)
    RadioItem = _Prop("TMenuItem_GetRadioItem", "TMenuItem_SetRadioItem", _bool)
    # 0〜255。
    GroupIndex = _Prop("TMenuItem_GetGroupIndex", "TMenuItem_SetGroupIndex", _int)
    Default = _Prop("TMenuItem_GetDefault", "TMenuItem_SetDefault", _bool)
    ShortCut = _Prop("TMenuItem_GetShortCut", "TMenuItem_SetShortCut", _int)
    Hint = _Prop("TMenuItem_GetHint", "TMenuItem_SetHint", _str)
    OnClick = _Event("TMenuItem_SetOnClick", "TNotifyEvent")
    Count = _Prop("TMenuItem_GetCount", None, _int)
    # 親の項目。メニューの直下の項目なら、そのメニューの Items(ルート)。どこにも追加されていなければ nullptr。
    Parent = _Prop("TMenuItem_GetParent", None, _existing("TMenuItem"))
    # 子の項目(MenuItem->Items[i]->Caption のように使う)。
    Items = _Indexed("TMenuItem_GetItem", None, _existing("TMenuItem"))
    # 画像の、メニューの Images(または親の項目の SubMenuImages)での位置(-1 なら無し。docs/adr/0030)。
    ImageIndex = _Prop("TMenuItem_GetImageIndex", "TMenuItem_SetImageIndex", _int)
    # 子の項目の画像リスト(設定すると、子の項目は TMenu::Images の代わりにこれを使う)。
    SubMenuImages = _Prop("TMenuItem_GetSubMenuImages", "TMenuItem_SetSubMenuImages", _comp("TCustomImageList"))
    # 項目の画像(ImageIndex を使わない場合)。項目が所有する TBitmap のビューで、初めて参照したときに作られる。代入は内容のコピー。
    Bitmap = _Prop("TMenuItem_GetBitmap", "TMenuItem_SetBitmap", _view("TBitmap"))
    # 割り当てた Action(docs/adr/0046)。Action の Caption・Checked・Enabled・ShortCut・ImageIndex 等が項目に写り、以後も連動する。
    # 選ぶと、OnClick(設定していれば)の後に Action の OnExecute が呼ばれる。
    Action = _Prop("TMenuItem_GetAction", "TMenuItem_SetAction", _comp("TBasicAction"))
    def Add(self, Item):
        lib.TMenuItem_Add(self._current(), _h(Item))
    def Insert(self, Index, Item):
        lib.TMenuItem_Insert(self._current(), int(Index), _h(Item))
    # Delete/Remove は子から外すだけで破棄しない(VCL と同じ)。Clear はすべての子を破棄する。
    def Delete(self, Index):
        lib.TMenuItem_Delete(self._current(), int(Index))
    def Remove(self, Item):
        lib.TMenuItem_Remove(self._current(), _h(Item))
    def Clear(self):
        lib.TMenuItem_Clear(self._current())
    def IndexOf(self, Item):
        _r = lib.TMenuItem_IndexOf(self._current(), _h(Item))
        return _r
    # 区切り線を末尾に追加する(項目は LCL が内部で生成する。GetItem で取得できる)。
    def AddSeparator(self):
        lib.TMenuItem_AddSeparator(self._current())
    def IsLine(self):
        _r = lib.TMenuItem_IsLine(self._current())
        return _r != 0
    # 利用者が項目を選んだときと同じ処理(AutoCheck の反映と OnClick)を行う。
    def Click(self):
        lib.TMenuItem_Click(self._current())
    # ---- docs/adr/0050 ----
    # メニューの OwnerDraw が true のとき、この項目を描く・大きさを決める。
    OnDrawItem = _Event("TMenuItem_SetOnDrawItem", "TMenuDrawItemEvent")
    OnMeasureItem = _Event("TMenuItem_SetOnMeasureItem", "TMenuMeasureItemEvent")


class TMenu(TComponent):
    """TMainMenu・TPopupMenu の共通の基底。Items はメニューのルートの項目で、メニュー自身が(LCL の内部で)生成・所有する。
    メニューに表示する項目は Items->Add(...) で追加する。"""
    Items = _Prop("TMenu_GetItems", None, _existing("TMenuItem"))
    # 項目の画像リスト(docs/adr/0030)。各項目の画像は TMenuItem::ImageIndex。
    Images = _Prop("TMenu_GetImages", "TMenu_SetImages", _comp("TCustomImageList"))
    # ---- docs/adr/0050 ----
    # true なら、項目を各項目の OnDrawItem で描く(OnMeasureItem で大きさを決める)。
    OwnerDraw = _Prop("TMenu_GetOwnerDraw", "TMenu_SetOwnerDraw", _bool)


class TMainMenu(TMenu):
    """フォームのメニューバー。TForm::Menu に割り当てると表示される。"""
    def __init__(self, AOwner):
        self._attach(lib.TMainMenu_Create(_h(AOwner)))


class TPopupMenu(TMenu):
    """右クリック等で開くメニュー。TControl::PopupMenu に割り当てると、そのコントロールの右クリックで開く(AutoPopup が true のとき)。"""
    def __init__(self, AOwner):
        self._attach(lib.TPopupMenu_Create(_h(AOwner)))
    AutoPopup = _Prop("TPopupMenu_GetAutoPopup", "TPopupMenu_SetAutoPopup", _bool)
    # 右クリックでメニューを開いたコンポーネント(OnPopup の中で、どこから開かれたかを知るのに使う)。
    # C++ ラッパーを介さずに作られたコンポーネントの場合は nullptr になる。
    PopupComponent = _Prop("TPopupMenu_GetPopupComponent", "TPopupMenu_SetPopupComponent", _comp("TComponent"))
    # 開く直前に呼ばれる。
    OnPopup = _Event("TPopupMenu_SetOnPopup", "TNotifyEvent")
    # 閉じた後に呼ばれる。
    OnClose = _Event("TPopupMenu_SetOnClose", "TNotifyEvent")
    # X, Y はスクリーン座標。Win32 ではメニューが閉じるまで戻らない。
    def Popup(self, X, Y):
        lib.TPopupMenu_Popup(self._current(), int(X), int(Y))


class TSizeConstraints(TPersistent):
    """コントロールの大きさの制限(LCL の TSizeConstraints)。コントロールが所有するものへの非所有のラッパー(docs/adr/0034)。
    0 は制限なし。Button1->Constraints->MinWidth = 80; のように使う。"""
    MinWidth = _Prop("TSizeConstraints_GetMinWidth", "TSizeConstraints_SetMinWidth", _int)
    MinHeight = _Prop("TSizeConstraints_GetMinHeight", "TSizeConstraints_SetMinHeight", _int)
    MaxWidth = _Prop("TSizeConstraints_GetMaxWidth", "TSizeConstraints_SetMaxWidth", _int)
    MaxHeight = _Prop("TSizeConstraints_GetMaxHeight", "TSizeConstraints_SetMaxHeight", _int)


class TControlBorderSpacing(TPersistent):
    """コントロールの周りの余白(LCL の TControlBorderSpacing。VCL には無く、VCL の Margins に近い)。
    コントロールが所有するものへの非所有のラッパー(docs/adr/0034)。Align・Anchors で配置するときに、親・隣との間を空ける。
    各辺の余白は Around + その辺の値。InnerBorder はコントロールの内側の余白(AutoSize のときに使われる)。"""
    Left = _Prop("TControlBorderSpacing_GetLeft", "TControlBorderSpacing_SetLeft", _int)
    Top = _Prop("TControlBorderSpacing_GetTop", "TControlBorderSpacing_SetTop", _int)
    Right = _Prop("TControlBorderSpacing_GetRight", "TControlBorderSpacing_SetRight", _int)
    Bottom = _Prop("TControlBorderSpacing_GetBottom", "TControlBorderSpacing_SetBottom", _int)
    Around = _Prop("TControlBorderSpacing_GetAround", "TControlBorderSpacing_SetAround", _int)
    InnerBorder = _Prop("TControlBorderSpacing_GetInnerBorder", "TControlBorderSpacing_SetInnerBorder", _int)


class TControlScrollBar(TPersistent):
    """TForm・TScrollBox の横・縦のスクロールバー(LCL の TControlScrollBar)。コントロールが所有するものへの非所有のラッパー。
    Range を領域の幅・高さより大きくすると、スクロールバーが出る(AutoScroll が true なら、Range は子の配置から LCL が決める)。"""
    # sbHorizontal か sbVertical。
    Kind = _Prop("TControlScrollBar_GetKind", None, _enum("TScrollBarKind"))
    # スクロールバーの太さ(ピクセル)。
    Size = _Prop("TControlScrollBar_GetSize", None, _int)
    # 矢印を押したときに動く量と、つまみの外を押したときに動く量(ピクセル)。
    Increment = _Prop("TControlScrollBar_GetIncrement", "TControlScrollBar_SetIncrement", _int)
    Page = _Prop("TControlScrollBar_GetPage", "TControlScrollBar_SetPage", _int)
    # 位置(0 から Range - 領域の大きさ まで)。
    Position = _Prop("TControlScrollBar_GetPosition", "TControlScrollBar_SetPosition", _int)
    # スクロールする範囲の大きさ(ピクセル)。
    Range = _Prop("TControlScrollBar_GetRange", "TControlScrollBar_SetRange", _int)
    # true なら、Increment を領域の大きさから決める。
    Smooth = _Prop("TControlScrollBar_GetSmooth", "TControlScrollBar_SetSmooth", _bool)
    # true なら、つまみをドラッグしている間も表示を動かす。
    Tracking = _Prop("TControlScrollBar_GetTracking", "TControlScrollBar_SetTracking", _bool)
    # false なら、Range が大きくてもスクロールバーを出さない。
    Visible = _Prop("TControlScrollBar_GetVisible", "TControlScrollBar_SetVisible", _bool)
    # 今スクロールバーが表示されているか。
    def IsScrollBarVisible(self):
        _r = lib.TControlScrollBar_IsScrollBarVisible(self._current())
        return _r != 0


class TControl(TComponent):
    # クライアント領域(枠・タイトルバー・メニューの内側)の幅。設定するとそれに合わせて Width が変わる。
    ClientWidth = _Prop("TControl_GetClientWidth", "TControl_SetClientWidth", _int)
    # クライアント領域の高さ。
    ClientHeight = _Prop("TControl_GetClientHeight", "TControl_SetClientHeight", _int)
    # 再描画を依頼する(描画はメッセージの処理のときに行われる)。
    def Invalidate(self):
        lib.TControl_Invalidate(self._current())
    # すぐに再描画する。
    def Repaint(self):
        lib.TControl_Repaint(self._current())
    # すぐに再描画する(Repaint と同じ)。
    def Refresh(self):
        lib.TControl_Refresh(self._current())
    # 再描画を依頼済みの部分を、すぐに描画する。
    def Update(self):
        lib.TControl_Update(self._current())
    # 兄弟の中で一番手前にする。
    def BringToFront(self):
        lib.TControl_BringToFront(self._current())
    # 兄弟の中で一番奥にする。
    def SendToBack(self):
        lib.TControl_SendToBack(self._current())
    # 位置と大きさをまとめて設定する(Left・Top・Width・Height を 1 つずつ設定するより、配置の計算が 1 回で済む)。
    def SetBounds(self, ALeft, ATop, AWidth, AHeight):
        lib.TControl_SetBounds(self._current(), int(ALeft), int(ATop), int(AWidth), int(AHeight))
    # 割り当てた Action(docs/adr/0046)。割り当てると Action の Caption・Enabled・Hint・Visible 等がコントロールに写り、以後も連動する。
    # クリックすると、OnClick(設定していれば)の後に Action の OnExecute が呼ばれる(LCL の仕様。VCL は OnClick だけを呼ぶ)。
    Action = _Prop("TControl_GetAction", "TControl_SetAction", _comp("TBasicAction"))
    Parent = _Prop("TControl_GetParent", "TControl_SetParent", _comp("TWinControl"))
    Left = _Prop("TControl_GetLeft", "TControl_SetLeft", _int)
    Top = _Prop("TControl_GetTop", "TControl_SetTop", _int)
    Width = _Prop("TControl_GetWidth", "TControl_SetWidth", _int)
    Height = _Prop("TControl_GetHeight", "TControl_SetHeight", _int)
    Visible = _Prop("TControl_GetVisible", "TControl_SetVisible", _bool)
    Enabled = _Prop("TControl_GetEnabled", "TControl_SetEnabled", _bool)
    Caption = _Prop("TControl_GetCaption", "TControl_SetCaption", _str)
    # 既定値はクラスごとに異なる(多くは alNone、TStatusBar は alBottom、TSplitter は alLeft)。
    Align = _Prop("TControl_GetAlign", "TControl_SetAlign", _enum("TAlign"))
    # true にすると、内容(TImage なら画像)に合わせて大きさを LCL が決める(docs/adr/0029)。
    AutoSize = _Prop("TControl_GetAutoSize", "TControl_SetAutoSize", _bool)
    # 右クリックで開くメニュー。C++ ラッパーを介さずに作られたメニューの場合は nullptr になる。
    PopupMenu = _Prop("TControl_GetPopupMenu", "TControl_SetPopupMenu", _comp("TPopupMenu"))
    # 背景色(既定は clDefault。docs/adr/0033)。
    Color = _Prop("TControl_GetColor", "TControl_SetColor", _int)
    # 文字のフォント。コントロールが所有する TFont のビューで、コントロールと寿命が一致する(docs/adr/0033)。
    # 代入は内容のコピー(nullptr なら何もしない)。Font->Assign(FontDialog1->Font) と同じ。
    Font = _Prop("TControl_GetFont", "TControl_SetFont", _obj("TFont"))
    # true(既定)なら、Parent の Color・Font を使う(Color・Font を設定すると false になる)。
    # LCL では TControl の protected で、ほとんどの具象クラスが published にしている(docs/adr/0034)。
    ParentColor = _Prop("TControl_GetParentColor", "TControl_SetParentColor", _bool)
    ParentFont = _Prop("TControl_GetParentFont", "TControl_SetParentFont", _bool)
    # 親の辺との距離を保つ辺(既定は akLeft・akTop。docs/adr/0034)。親の大きさが変わると、それに合わせて位置・大きさが変わる。
    Anchors = _Prop("TControl_GetAnchors", "TControl_SetAnchors", _set("TAnchorKind"))
    # 周りの余白・大きさの制限。コントロールが所有するもののビューで、代入は内容のコピー(nullptr なら何もしない)。
    BorderSpacing = _Prop("TControl_GetBorderSpacing", "TControl_SetBorderSpacing", _obj("TControlBorderSpacing"))
    Constraints = _Prop("TControl_GetConstraints", "TControl_SetConstraints", _obj("TSizeConstraints"))
    # マウスを重ねたときに表示する文字列。表示するのは ShowHint が true のとき(ParentShowHint が true なら Parent に従う)。
    Hint = _Prop("TControl_GetHint", "TControl_SetHint", _str)
    ShowHint = _Prop("TControl_GetShowHint", "TControl_SetShowHint", _bool)
    ParentShowHint = _Prop("TControl_GetParentShowHint", "TControl_SetParentShowHint", _bool)
    Cursor = _Prop("TControl_GetCursor", "TControl_SetCursor", _int)
    OnClick = _Event("TControl_SetOnClick", "TNotifyEvent")
    OnDblClick = _Event("TControl_SetOnDblClick", "TNotifyEvent")
    # LCL では他のウィンドウメッセージへの応答等で発生し、必ずしもユーザー操作直後とは限らない。
    OnResize = _Event("TControl_SetOnResize", "TNotifyEvent")
    OnMouseDown = _Event("TControl_SetOnMouseDown", "TMouseEvent")
    OnMouseUp = _Event("TControl_SetOnMouseUp", "TMouseEvent")
    OnMouseMove = _Event("TControl_SetOnMouseMove", "TMouseMoveEvent")
    OnMouseEnter = _Event("TControl_SetOnMouseEnter", "TNotifyEvent")
    OnMouseLeave = _Event("TControl_SetOnMouseLeave", "TNotifyEvent")
    OnMouseWheel = _Event("TControl_SetOnMouseWheel", "TMouseWheelEvent")
    def Show(self):
        lib.TControl_Show(self._current())
    def Hide(self):
        lib.TControl_Hide(self._current())
    # LCL では TControl の protected。TCustomEdit / TCustomComboBox が公開する。
    _Text = _Prop("TControl_GetText", "TControl_SetText", _str)


class TWinControl(TControl):
    # フォーカスを移す(表示されていない・無効なコントロールには移せず、例外になる。CanFocus で確かめる)。
    def SetFocus(self):
        lib.TWinControl_SetFocus(self._current())
    # フォーカスを移せるか(自分と親がすべて表示され、有効か)。
    def CanFocus(self):
        _r = lib.TWinControl_CanFocus(self._current())
        return _r != 0
    # フォーカスを持っているか。
    def Focused(self):
        _r = lib.TWinControl_Focused(self._current())
        return _r != 0
    # フォーカスを受けたとき・失ったとき。
    OnEnter = _Event("TWinControl_SetOnEnter", "TNotifyEvent")
    OnExit = _Event("TWinControl_SetOnExit", "TNotifyEvent")
    OnKeyDown = _Event("TWinControl_SetOnKeyDown", "TKeyEvent")
    OnKeyUp = _Event("TWinControl_SetOnKeyUp", "TKeyEvent")
    OnKeyPress = _Event("TWinControl_SetOnKeyPress", "TKeyPressEvent")
    # Tab キーでのフォーカスの移動の順(同じ Parent の中での位置。-1 は末尾)と、移動の対象にするか(docs/adr/0034)。
    TabOrder = _Prop("TWinControl_GetTabOrder", "TWinControl_SetTabOrder", _int)
    TabStop = _Prop("TWinControl_GetTabStop", "TWinControl_SetTabStop", _bool)
    # 内側の余白(ピクセル)。子を置ける範囲(Align で寄せる範囲)が、四辺ともこの幅だけ狭くなる(docs/adr/0048)。
    # LCL と同じく、TCustomPanel・TCustomForm・TCustomListView・TCustomTreeView が using で公開する(TTabSheet は、LCL が公開しているが
    # Windows では効かないため公開しない)。
    _BorderWidth = _Prop("TWinControl_GetBorderWidth", "TWinControl_SetBorderWidth", _int)
    # 枠(bsNone・bsSingle)。LCL と同じく、枠を持てるクラス(TCustomEdit・TCustomListBox・TCustomComboBox・TCustomListView・
    # TCustomControl)が using で公開する(docs/adr/0048)。
    _BorderStyle = _Prop("TWinControl_GetBorderStyle", "TWinControl_SetBorderStyle", _enum("TFormBorderStyle"))


class TCustomScrollBar(TWinControl):
    Kind = _Prop("TCustomScrollBar_GetKind", "TCustomScrollBar_SetKind", _enum("TScrollBarKind"))
    Min = _Prop("TCustomScrollBar_GetMin", "TCustomScrollBar_SetMin", _int)
    Max = _Prop("TCustomScrollBar_GetMax", "TCustomScrollBar_SetMax", _int)
    Position = _Prop("TCustomScrollBar_GetPosition", "TCustomScrollBar_SetPosition", _int)
    PageSize = _Prop("TCustomScrollBar_GetPageSize", "TCustomScrollBar_SetPageSize", _int)
    OnChange = _Event("TCustomScrollBar_SetOnChange", "TNotifyEvent")
    # ---- docs/adr/0049 ----
    # つまみの外を押したとき・矢印を押したときに動く量。
    LargeChange = _Prop("TCustomScrollBar_GetLargeChange", "TCustomScrollBar_SetLargeChange", _int)
    SmallChange = _Prop("TCustomScrollBar_GetSmallChange", "TCustomScrollBar_SetSmallChange", _int)
    # つまみ・矢印を操作したとき(OnChange より先に呼ばれる)。
    OnScroll = _Event("TCustomScrollBar_SetOnScroll", "TScrollEvent")


class TScrollBar(TCustomScrollBar):
    def __init__(self, AOwner):
        self._attach(lib.TScrollBar_Create(_h(AOwner)))


class TCustomTrackBar(TWinControl):
    """つまみをドラッグして値を選ぶスライダー。"""
    Min = _Prop("TCustomTrackBar_GetMin", "TCustomTrackBar_SetMin", _int)
    Max = _Prop("TCustomTrackBar_GetMax", "TCustomTrackBar_SetMax", _int)
    Position = _Prop("TCustomTrackBar_GetPosition", "TCustomTrackBar_SetPosition", _int)
    OnChange = _Event("TCustomTrackBar_SetOnChange", "TNotifyEvent")
    # ---- docs/adr/0049 ----
    # 縦(trVertical)か横か。
    Orientation = _Prop("TCustomTrackBar_GetOrientation", "TCustomTrackBar_SetOrientation", _enum("TTrackBarOrientation"))
    # 目盛りの間隔(TickStyle が tsAuto のとき)。
    Frequency = _Prop("TCustomTrackBar_GetFrequency", "TCustomTrackBar_SetFrequency", _int)
    # 目盛りを付ける側。
    TickMarks = _Prop("TCustomTrackBar_GetTickMarks", "TCustomTrackBar_SetTickMarks", _enum("TTickMark"))
    # 目盛りの付け方(tsNone は付けない)。
    TickStyle = _Prop("TCustomTrackBar_GetTickStyle", "TCustomTrackBar_SetTickStyle", _enum("TTickStyle"))
    # 矢印キーで動く量と、PageUp・PageDown で動く量。
    LineSize = _Prop("TCustomTrackBar_GetLineSize", "TCustomTrackBar_SetLineSize", _int)
    PageSize = _Prop("TCustomTrackBar_GetPageSize", "TCustomTrackBar_SetPageSize", _int)
    # 選択の範囲として強調する区間(ShowSelRange が true のとき)。
    SelStart = _Prop("TCustomTrackBar_GetSelStart", "TCustomTrackBar_SetSelStart", _int)
    SelEnd = _Prop("TCustomTrackBar_GetSelEnd", "TCustomTrackBar_SetSelEnd", _int)
    ShowSelRange = _Prop("TCustomTrackBar_GetShowSelRange", "TCustomTrackBar_SetShowSelRange", _bool)
    # true なら、Min と Max の側を入れ替える。
    Reversed = _Prop("TCustomTrackBar_GetReversed", "TCustomTrackBar_SetReversed", _bool)


class TTrackBar(TCustomTrackBar):
    def __init__(self, AOwner):
        self._attach(lib.TTrackBar_Create(_h(AOwner)))


class TCustomProgressBar(TWinControl):
    """進捗を表示する表示専用コントロール(イベントは無い)。"""
    Min = _Prop("TCustomProgressBar_GetMin", "TCustomProgressBar_SetMin", _int)
    Max = _Prop("TCustomProgressBar_GetMax", "TCustomProgressBar_SetMax", _int)
    Position = _Prop("TCustomProgressBar_GetPosition", "TCustomProgressBar_SetPosition", _int)
    # ---- docs/adr/0049 ----
    # 伸びる向き(pbVertical は下から上)。
    Orientation = _Prop("TCustomProgressBar_GetOrientation", "TCustomProgressBar_SetOrientation", _enum("TProgressBarOrientation"))
    # true なら、区切りの無い棒で描く。
    Smooth = _Prop("TCustomProgressBar_GetSmooth", "TCustomProgressBar_SetSmooth", _bool)
    # StepIt で進める量。
    Step = _Prop("TCustomProgressBar_GetStep", "TCustomProgressBar_SetStep", _int)
    # pbstMarquee は、進み具合の分からない処理の間に動き続ける表示。
    Style = _Prop("TCustomProgressBar_GetStyle", "TCustomProgressBar_SetStyle", _enum("TProgressBarStyle"))
    # true なら、進み具合を文字でも表示する(Windows では表示されないことがある)。
    BarShowText = _Prop("TCustomProgressBar_GetBarShowText", "TCustomProgressBar_SetBarShowText", _bool)
    # Position を Step だけ進める・Delta だけ進める(Max を超えると Min に戻る)。
    def StepIt(self):
        lib.TCustomProgressBar_StepIt(self._current())
    def StepBy(self, Delta):
        lib.TCustomProgressBar_StepBy(self._current(), int(Delta))


class TProgressBar(TCustomProgressBar):
    def __init__(self, AOwner):
        self._attach(lib.TProgressBar_Create(_h(AOwner)))


class TGraphicControl(TControl):
    pass


class TCustomControl(TWinControl):
    """自分で描くことのできるウィンドウのコントロール(TForm・TPanel・TScrollBox・グリッド等の基底)。"""
    BorderStyle = TWinControl._BorderStyle
    # 描く先(docs/adr/0045)。OnPaint(グリッドは OnDrawCell)の中で描く。コントロールが所有する実体への非所有のビュー
    # (TPaintBox::Canvas と同じ)。OnPaint の外で描いたものは、次の再描画で消える。
    Canvas = _Prop("TCustomControl_GetCanvas", None, _obj("TCanvas"))
    # 描き直すとき(LCL では protected。TForm・TPanel・TScrollBox が公開する)。描くのは Canvas に。
    # 描き直させるには Invalidate() を呼ぶ。
    _OnPaint = _Event("TCustomControl_SetOnPaint", "TNotifyEvent")


class TUpDown(TCustomControl):
    """Edit 等に付属する上下矢印。Min/Max/Position/Increment/Associate は LCL では TCustomUpDown の
    protected だが、唯一の具象クラス TUpDown が published にしているため、TUpDown に直接置く
    (TCheckBox の Checked と同じ形)。OnClick/OnChanging は独自のシグネチャのため今回は未対応。"""
    def __init__(self, AOwner):
        self._attach(lib.TUpDown_Create(_h(AOwner)))
    Min = _Prop("TUpDown_GetMin", "TUpDown_SetMin", _int)
    Max = _Prop("TUpDown_GetMax", "TUpDown_SetMax", _int)
    Position = _Prop("TUpDown_GetPosition", "TUpDown_SetPosition", _int)
    Increment = _Prop("TUpDown_GetIncrement", "TUpDown_SetIncrement", _int)
    # 値を増減させる対象のコントロール(TEdit 等)。
    Associate = _Prop("TUpDown_GetAssociate", "TUpDown_SetAssociate", _comp("TWinControl"))
    # ---- docs/adr/0049 ----
    # 矢印の向き(udVertical は上下、udHorizontal は左右)。
    Orientation = _Prop("TUpDown_GetOrientation", "TUpDown_SetOrientation", _enum("TUDOrientation"))
    # Associate のどちら側に付けるか。
    AlignButton = _Prop("TUpDown_GetAlignButton", "TUpDown_SetAlignButton", _enum("TUDAlignButton"))
    # true なら、Max を超えると Min に戻る(逆も)。
    Wrap = _Prop("TUpDown_GetWrap", "TUpDown_SetWrap", _bool)
    # true なら、Associate の上で矢印キーを押すと値が変わる。
    ArrowKeys = _Prop("TUpDown_GetArrowKeys", "TUpDown_SetArrowKeys", _bool)
    # true なら、Associate に表示する値に 3 桁ごとの区切りを入れる。
    Thousands = _Prop("TUpDown_GetThousands", "TUpDown_SetThousands", _bool)


class TScrollingWinControl(TCustomControl):
    # true なら、子がはみ出したときにスクロールバーを出す(Range を子の配置から決める。docs/adr/0048)。
    AutoScroll = _Prop("TScrollingWinControl_GetAutoScroll", "TScrollingWinControl_SetAutoScroll", _bool)
    # 横・縦のスクロールバー。代入は内容のコピー。
    HorzScrollBar = _Prop("TScrollingWinControl_GetHorzScrollBar", "TScrollingWinControl_SetHorzScrollBar", _obj("TControlScrollBar"))
    VertScrollBar = _Prop("TScrollingWinControl_GetVertScrollBar", "TScrollingWinControl_SetVertScrollBar", _obj("TControlScrollBar"))


class TScrollBox(TScrollingWinControl):
    """スクロール可能な汎用コンテナ。TScrollingWinControl の直接の派生で、追加のメンバは無い。"""
    OnPaint = TCustomControl._OnPaint
    def __init__(self, AOwner):
        self._attach(lib.TScrollBox_Create(_h(AOwner)))


class TCustomForm(_mixins["TCustomForm"], TScrollingWinControl):
    BorderWidth = TWinControl._BorderWidth
    OnPaint = TCustomControl._OnPaint
    # LCL の TCustomForm は Show/Hide を独自に宣言している(TControl のものを隠す)。
    def Show(self):
        lib.TCustomForm_Show(self._current())
    def Hide(self):
        lib.TCustomForm_Hide(self._current())
    # モーダルで表示し、閉じられたときの ModalResult を返す(× で閉じたときは mrCancel)。
    def ShowModal(self):
        _r = lib.TCustomForm_ShowModal(self._current())
        return _r
    def Close(self):
        lib.TCustomForm_Close(self._current())
    # 保留中のメッセージを処理し終えてから破棄する(破棄後はラッパーも delete される)。
    # Free() と違い、フォーム自身やその子のイベントハンドラの中からでも安全に呼べる。
    def Release(self):
        lib.TCustomForm_Release(self._current())
    OnHide = _Event("TCustomForm_SetOnHide", "TNotifyEvent")
    OnActivate = _Event("TCustomForm_SetOnActivate", "TNotifyEvent")
    OnDeactivate = _Event("TCustomForm_SetOnDeactivate", "TNotifyEvent")
    OnCloseQuery = _Event("TCustomForm_SetOnCloseQuery", "TCloseQueryEvent")
    OnClose = _Event("TCustomForm_SetOnClose", "TCloseEvent")
    # 破棄の最初に呼ばれる(子コントロールはまだ有効)。このあとラッパーも delete される。
    OnDestroy = _Event("TCustomForm_SetOnDestroy", "TNotifyEvent")
    # フォームのメニューバー。nullptr を代入すると外す(メニュー自体は破棄されない)。
    Menu = _Prop("TCustomForm_GetMenu", "TCustomForm_SetMenu", _comp("TMainMenu"))
    # モーダルの結果。モーダルで表示中に mrNone 以外を設定すると、フォームが閉じて ShowModal() がその値を返す(docs/adr/0041)。
    ModalResult = _Prop("TCustomForm_GetModalResult", "TCustomForm_SetModalResult", _int)
    BorderStyle = _Prop("TCustomForm_GetBorderStyle", "TCustomForm_SetBorderStyle", _enum("TFormBorderStyle"))
    Position = _Prop("TCustomForm_GetPosition", "TCustomForm_SetPosition", _enum("TPosition"))
    WindowState = _Prop("TCustomForm_GetWindowState", "TCustomForm_SetWindowState", _enum("TWindowState"))
    BorderIcons = _Prop("TCustomForm_GetBorderIcons", "TCustomForm_SetBorderIcons", _set("TBorderIcon"))
    FormStyle = _Prop("TCustomForm_GetFormStyle", "TCustomForm_SetFormStyle", _enum("TFormStyle"))
    # true なら、キーの入力を子のコントロールより先にフォームの OnKeyDown・OnKeyPress・OnKeyUp が受ける。
    KeyPreview = _Prop("TCustomForm_GetKeyPreview", "TCustomForm_SetKeyPreview", _bool)
    # フォーカスを持つ(表示したときに持たせる)コントロール。
    ActiveControl = _Prop("TCustomForm_GetActiveControl", "TCustomForm_SetActiveControl", _comp("TWinControl"))
    # ---- docs/adr/0047 ----
    # タイトルバー・タスクバーのアイコン。空なら Application->Icon を使う。代入は内容のコピー(nullptr なら空にする)。
    Icon = _Prop("TCustomForm_GetIcon", "TCustomForm_SetIcon", _view("TIcon"))
    # true なら、エクスプローラー等からファイルをドロップできる(ドロップすると OnDropFiles が呼ばれる)。
    AllowDropFiles = _Prop("TCustomForm_GetAllowDropFiles", "TCustomForm_SetAllowDropFiles", _bool)
    # ファイルをドロップしたとき。FileNames はフルパス(UTF-8)。
    OnDropFiles = _Event("TCustomForm_SetOnDropFiles", "TDropFilesEvent")


class TForm(_mixins["TForm"], TCustomForm):
    pass


class TApplication(_mixins["TApplication"], TComponent):
    """C++Builder の TApplication。LCL の TApplication は FCL の TCustomApplication の派生だが、
    C++Builder に合わせて TComponent 直下に置く。インスタンスはグローバル変数 Application の 1 つだけ。
    Application が所有するフォーム(CreateForm や new TForm(Application) で生成したもの)は、
    プログラムの終了時(main から戻った後)にまとめて破棄され、ラッパーのデストラクタも呼ばれる。"""
    # 実行ファイルのフルパス。
    ExeName = _Prop("TApplication_GetExeName", None, _str)
    # 表示中(マウスの下のコントロール)のヒント。TStatusBar::OnHint の中で使う(docs/adr/0044)。
    Hint = _Prop("TApplication_GetHint", "TApplication_SetHint", _str)
    # false なら、アプリケーションのすべてのヒントを表示しない。
    ShowHint = _Prop("TApplication_GetShowHint", "TApplication_SetShowHint", _bool)
    # マウスを止めてからヒントを表示するまでの時間(ミリ秒)。
    HintPause = _Prop("TApplication_GetHintPause", "TApplication_SetHintPause", _int)
    # ヒントを表示してから消すまでの時間(ミリ秒)。
    HintHidePause = _Prop("TApplication_GetHintHidePause", "TApplication_SetHintHidePause", _int)
    # アプリケーション(のすべてのフォーム)を最小化する。
    def Minimize(self):
        lib.TApplication_Minimize(self._current())
    # 最小化したアプリケーションを元に戻す。
    def Restore(self):
        lib.TApplication_Restore(self._current())
    # アプリケーションを手前に出す。
    def BringToFront(self):
        lib.TApplication_BringToFront(self._current())
    # メッセージの処理が終わり、待ちに入るとき。Done を false にすると、すぐにもう一度呼ばれる(既定は true)。
    OnIdle = _Event("TApplication_SetOnIdle", "TIdleEvent")
    # イベントのハンドラから送出された例外を、既定のエラーのダイアログの代わりに受ける(docs/adr/0031)。
    OnException = _Event("TApplication_SetOnException", "TExceptionEvent")
    # アプリケーションのアイコン(docs/adr/0047)。Icon が空のフォームは、これを使う。代入は内容のコピー。
    Icon = _Prop("TApplication_GetIcon", "TApplication_SetIcon", _view("TIcon"))
    # CreateForm で最初に生成したフォーム。Run はこれを表示し、これが閉じられると戻る。
    MainForm = _Prop("TApplication_GetMainForm", None, _comp("TForm"))
    Terminated = _Prop("TApplication_GetTerminated", None, _bool)
    Title = _Prop("TApplication_GetTitle", "TApplication_SetTitle", _str)
    ShowMainForm = _Prop("TApplication_GetShowMainForm", "TApplication_SetShowMainForm", _bool)
    def Run(self):
        lib.TApplication_Run(self._current())
    def ProcessMessages(self):
        lib.TApplication_ProcessMessages(self._current())
    def Terminate(self):
        lib.TApplication_Terminate(self._current())


class TScreen(TComponent):
    """画面(LCL の TScreen)。インスタンスはグローバル変数 Screen の 1 つだけ(LCL が持つもので、破棄しない)。"""
    # crDefault 以外にすると、すべてのコントロールの上でそのカーソルになる(処理の間の crHourGlass 等。crDefault で戻す)。
    Cursor = _Prop("TScreen_GetCursor", "TScreen_SetCursor", _int)
    # 主モニタの大きさ。
    Width = _Prop("TScreen_GetWidth", None, _int)
    Height = _Prop("TScreen_GetHeight", None, _int)
    # すべてのモニタを合わせた範囲。
    DesktopLeft = _Prop("TScreen_GetDesktopLeft", None, _int)
    DesktopTop = _Prop("TScreen_GetDesktopTop", None, _int)
    DesktopWidth = _Prop("TScreen_GetDesktopWidth", None, _int)
    DesktopHeight = _Prop("TScreen_GetDesktopHeight", None, _int)
    # 主モニタの、タスクバーを除いた範囲。
    WorkAreaLeft = _Prop("TScreen_GetWorkAreaLeft", None, _int)
    WorkAreaTop = _Prop("TScreen_GetWorkAreaTop", None, _int)
    WorkAreaWidth = _Prop("TScreen_GetWorkAreaWidth", None, _int)
    WorkAreaHeight = _Prop("TScreen_GetWorkAreaHeight", None, _int)
    WorkAreaRect = _Prop("TScreen_GetWorkAreaRect", None, _rect_conv)
    # 画面の解像度(96 が 100%)。
    PixelsPerInch = _Prop("TScreen_GetPixelsPerInch", None, _int)
    MonitorCount = _Prop("TScreen_GetMonitorCount", None, _int)
    # 開いている(生成済みの)フォーム(TForm の派生だけ)。プログラムが作ったものでないフォーム(MessageDlg のダイアログ等)は nullptr。
    FormCount = _Prop("TScreen_GetFormCount", None, _int)
    Forms = _Indexed("TScreen_GetForms", None, _comp("TForm"))
    # アクティブなフォーム・フォーカスを持つコントロール(無いとき・プログラムが作ったものでないときは nullptr)。
    ActiveForm = _Prop("TScreen_GetActiveForm", None, _comp("TForm"))
    ActiveControl = _Prop("TScreen_GetActiveControl", None, _comp("TWinControl"))
    # インストールされているフォントの名前。
    Fonts = _Prop("TScreen_GetFonts", None, _view("TStrings"))
    # アクティブなフォーム・フォーカスを持つコントロールが変わったとき(Sender は Screen)。
    OnActiveFormChange = _Event("TScreen_SetOnActiveFormChange", "TNotifyEvent")
    OnActiveControlChange = _Event("TScreen_SetOnActiveControlChange", "TNotifyEvent")


class TClipboard(_mixins["TClipboard"], TPersistent):
    """クリップボード(LCL の TClipboard)。Clipboard() で得る 1 つだけで、利用者は破棄しない。
    画像を置くのは Clipboard()->Assign(Image1->Picture)、読むのは Image1->Picture->Assign(Clipboard())。"""
    # クリップボードの文字列(UTF-8)。文字列が無ければ空文字列。代入するとクリップボードの内容を置き換える。
    AsText = _Prop("TClipboard_GetAsText", "TClipboard_SetAsText", _str)
    # 今の内容が持つ形式の数と番号。
    FormatCount = _Prop("TClipboard_GetFormatCount", None, _int)
    Formats = _Indexed("TClipboard_GetFormats", None, _int)
    # その形式の内容があるか(Clipboard()->HasFormat(CF_Text()))。
    def HasFormat(self, Format):
        _r = lib.TClipboard_HasFormat(self._current(), int(Format))
        return _r != 0
    # 画像(読み込める形式のどれか)があるか。
    def HasPictureFormat(self):
        _r = lib.TClipboard_HasPictureFormat(self._current())
        return _r != 0
    # 内容を消す。
    def Clear(self):
        lib.TClipboard_Clear(self._current())
    # Open から Close までの間に置いた内容(AsText と画像等)を、1 度にまとめて置く。
    def Open(self):
        lib.TClipboard_Open(self._current())
    def Close(self):
        lib.TClipboard_Close(self._current())


class TCustomPanel(TCustomControl):
    BorderWidth = TWinControl._BorderWidth
    # ---- docs/adr/0048 ----
    # Caption の横・縦の揃えと折り返し(既定は中央)。
    Alignment = _Prop("TCustomPanel_GetAlignment", "TCustomPanel_SetAlignment", _enum("TAlignment"))
    VerticalAlignment = _Prop("TCustomPanel_GetVerticalAlignment", "TCustomPanel_SetVerticalAlignment", _enum("TVerticalAlignment"))
    WordWrap = _Prop("TCustomPanel_GetWordWrap", "TCustomPanel_SetWordWrap", _bool)
    # 縁の外側・内側の凹凸(既定は外側が bvRaised、内側が bvNone)と、その幅・色(clDefault は凹凸の既定の色)。
    # 縁があると、子を置ける範囲(Align で寄せる範囲)がその幅だけ狭くなる。
    BevelOuter = _Prop("TCustomPanel_GetBevelOuter", "TCustomPanel_SetBevelOuter", _enum("TPanelBevel"))
    BevelInner = _Prop("TCustomPanel_GetBevelInner", "TCustomPanel_SetBevelInner", _enum("TPanelBevel"))
    BevelWidth = _Prop("TCustomPanel_GetBevelWidth", "TCustomPanel_SetBevelWidth", _int)
    BevelColor = _Prop("TCustomPanel_GetBevelColor", "TCustomPanel_SetBevelColor", _int)


class TPanel(TCustomPanel):
    OnPaint = TCustomControl._OnPaint
    def __init__(self, AOwner):
        self._attach(lib.TPanel_Create(_h(AOwner)))


class TCustomGroupBox(TWinControl):
    pass


class TGroupBox(TCustomGroupBox):
    def __init__(self, AOwner):
        self._attach(lib.TGroupBox_Create(_h(AOwner)))


class TCustomRadioGroup(TCustomGroupBox):
    """ラジオボタンの一覧を項目文字列から自動生成するグループ。OnClick は TControl のものとは別の、
    このクラス自身のイベント(いずれかのボタンが押されたときに呼ばれる)。"""
    ItemIndex = _Prop("TCustomRadioGroup_GetItemIndex", "TCustomRadioGroup_SetItemIndex", _int)
    OnClick = _Event("TCustomRadioGroup_SetOnClick", "TNotifyEvent")
    # 文字列の一覧(TStrings。RadioGroup1->Items->Add("x") のように使う)。
    Items = _Prop("TCustomRadioGroup_GetItems", None, _view("TStrings"))
    # ---- docs/adr/0049 ----
    # 項目を並べる列の数。
    Columns = _Prop("TCustomRadioGroup_GetColumns", "TCustomRadioGroup_SetColumns", _int)
    # 項目を並べる順(clHorizontalThenVertical は横に並べてから次の行)。
    ColumnLayout = _Prop("TCustomRadioGroup_GetColumnLayout", "TCustomRadioGroup_SetColumnLayout", _enum("TColumnLayout"))
    # true なら、項目をグループの高さいっぱいに広げて並べる。
    AutoFill = _Prop("TCustomRadioGroup_GetAutoFill", "TCustomRadioGroup_SetAutoFill", _bool)
    # ItemIndex が変わったとき(利用者の操作でも、プログラムからの代入でも)。
    OnSelectionChanged = _Event("TCustomRadioGroup_SetOnSelectionChanged", "TNotifyEvent")


class TRadioGroup(TCustomRadioGroup):
    def __init__(self, AOwner):
        self._attach(lib.TRadioGroup_Create(_h(AOwner)))


class TCustomCheckGroup(TCustomGroupBox):
    """チェックボックスの一覧を項目文字列から自動生成するグループ。"""
    # 文字列の一覧(TStrings。CheckGroup1->Items->Add("x") のように使う)。
    Items = _Prop("TCustomCheckGroup_GetItems", None, _view("TStrings"))
    # 項目ごとのチェックの状態(CheckGroup1->Checked[i] = true;)。
    Checked = _Indexed("TCustomCheckGroup_GetChecked", "TCustomCheckGroup_SetChecked", _bool)
    # ---- docs/adr/0049 ----
    # 項目を並べる列の数。
    Columns = _Prop("TCustomCheckGroup_GetColumns", "TCustomCheckGroup_SetColumns", _int)
    # 項目を並べる順(clHorizontalThenVertical は横に並べてから次の行)。
    ColumnLayout = _Prop("TCustomCheckGroup_GetColumnLayout", "TCustomCheckGroup_SetColumnLayout", _enum("TColumnLayout"))
    # true なら、項目をグループの高さいっぱいに広げて並べる。
    AutoFill = _Prop("TCustomCheckGroup_GetAutoFill", "TCustomCheckGroup_SetAutoFill", _bool)
    # 項目ごとに、利用者がチェックを切り替えられるか。
    CheckEnabled = _Indexed("TCustomCheckGroup_GetCheckEnabled", "TCustomCheckGroup_SetCheckEnabled", _bool)
    # 利用者が項目のチェックを切り替えたとき。
    OnItemClick = _Event("TCustomCheckGroup_SetOnItemClick", "TCheckGroupClicked")


class TCheckGroup(TCustomCheckGroup):
    def __init__(self, AOwner):
        self._attach(lib.TCheckGroup_Create(_h(AOwner)))


class TCustomLabel(TGraphicControl):
    # 文字の横の揃え(AutoSize が false のときに効く)。
    Alignment = _Prop("TCustomLabel_GetAlignment", "TCustomLabel_SetAlignment", _enum("TAlignment"))
    # 文字の縦の揃え(AutoSize が false のときに効く)。
    Layout = _Prop("TCustomLabel_GetLayout", "TCustomLabel_SetLayout", _enum("TTextLayout"))
    # 幅に合わせて折り返す(AutoSize が true なら、折り返した行の数に合わせて高さが変わる)。
    WordWrap = _Prop("TCustomLabel_GetWordWrap", "TCustomLabel_SetWordWrap", _bool)
    # 背景を塗らない(親の背景が見える)。
    Transparent = _Prop("TCustomLabel_GetTransparent", "TCustomLabel_SetTransparent", _bool)
    # Caption のアクセスキー(&N)を押したときにフォーカスを移すコントロール。
    FocusControl = _Prop("TCustomLabel_GetFocusControl", "TCustomLabel_SetFocusControl", _comp("TWinControl"))
    # Caption の & をアクセスキーの印(下線)として表示する(false なら & をそのまま表示する)。
    ShowAccelChar = _Prop("TCustomLabel_GetShowAccelChar", "TCustomLabel_SetShowAccelChar", _bool)


class TLabel(TCustomLabel):
    def __init__(self, AOwner):
        self._attach(lib.TLabel_Create(_h(AOwner)))


class TBoundLabel(TCustomLabel):
    """TLabeledEdit の EditLabel。LCL が LabeledEdit の生成時に内部で作るラベルで、利用者は生成しない
    (LabeledEdit と一緒に破棄される)。Caption 等は TControl のものを使う。"""


class TBevel(TGraphicControl):
    def __init__(self, AOwner):
        self._attach(lib.TBevel_Create(_h(AOwner)))
    Shape = _Prop("TBevel_GetShape", "TBevel_SetShape", _enum("TBevelShape"))
    Style = _Prop("TBevel_GetStyle", "TBevel_SetStyle", _enum("TBevelStyle"))


class TButtonControl(TWinControl):
    # LCL では TButtonControl の protected。TCheckBox / TRadioButton が公開する。
    _Checked = _Prop("TButtonControl_GetChecked", "TButtonControl_SetChecked", _bool)


class TCustomButton(TButtonControl):
    # mrNone 以外なら、押したときにフォームの ModalResult をこの値にする(モーダルのフォームが閉じる。docs/adr/0041)。
    ModalResult = _Prop("TCustomButton_GetModalResult", "TCustomButton_SetModalResult", _int)
    # true なら、フォームで Enter を押したときに押される(既定のボタン)。
    Default = _Prop("TCustomButton_GetDefault", "TCustomButton_SetDefault", _bool)
    # true なら、フォームで Esc を押したときに押される(取り消しのボタン)。
    Cancel = _Prop("TCustomButton_GetCancel", "TCustomButton_SetCancel", _bool)


class TButton(TCustomButton):
    def __init__(self, AOwner):
        self._attach(lib.TButton_Create(_h(AOwner)))


class TCustomBitBtn(TCustomButton):
    Kind = _Prop("TCustomBitBtn_GetKind", "TCustomBitBtn_SetKind", _enum("TBitBtnKind"))
    # ボタンの画像。ボタンが所有する TBitmap のビューで、ボタンと寿命が一致する(docs/adr/0029)。
    # 代入は内容のコピー(nullptr なら画像を無くす)。代入すると NumGlyphs は画像の幅と高さの比から LCL が決め直す。
    Glyph = _Prop("TCustomBitBtn_GetGlyph", "TCustomBitBtn_SetGlyph", _view("TBitmap"))
    # 横に並べた状態別(通常・無効・押下・下がったまま)の画像の数(1〜4)。
    NumGlyphs = _Prop("TCustomBitBtn_GetNumGlyphs", "TCustomBitBtn_SetNumGlyphs", _int)
    Layout = _Prop("TCustomBitBtn_GetLayout", "TCustomBitBtn_SetLayout", _enum("TButtonLayout"))
    # 端から画像までの距離(-1(既定)なら画像と文字列をまとめて中央に置く)。
    Margin = _Prop("TCustomBitBtn_GetMargin", "TCustomBitBtn_SetMargin", _int)
    # 画像と文字列の間隔。
    Spacing = _Prop("TCustomBitBtn_GetSpacing", "TCustomBitBtn_SetSpacing", _int)
    # 画像リスト(docs/adr/0030)。設定すると、Glyph の代わりに Images の ImageIndex 番目の画像を表示する。
    Images = _Prop("TCustomBitBtn_GetImages", "TCustomBitBtn_SetImages", _comp("TCustomImageList"))
    ImageIndex = _Prop("TCustomBitBtn_GetImageIndex", "TCustomBitBtn_SetImageIndex", _int)


class TBitBtn(TCustomBitBtn):
    def __init__(self, AOwner):
        self._attach(lib.TBitBtn_Create(_h(AOwner)))


class TCustomCheckBox(TButtonControl):
    # チェックの状態(cbGrayed は AllowGrayed のときだけ、利用者の操作でもなる)。
    State = _Prop("TCustomCheckBox_GetState", "TCustomCheckBox_SetState", _enum("TCheckBoxState"))
    # クリックで cbUnchecked → cbChecked → cbGrayed と 3 つの状態を切り替える。
    AllowGrayed = _Prop("TCustomCheckBox_GetAllowGrayed", "TCustomCheckBox_SetAllowGrayed", _bool)
    # State(Checked)が変わったとき(プログラムからの変更でも呼ばれる)。
    OnChange = _Event("TCustomCheckBox_SetOnChange", "TNotifyEvent")


class TCheckBox(TCustomCheckBox):
    Checked = TButtonControl._Checked
    def __init__(self, AOwner):
        self._attach(lib.TCheckBox_Create(_h(AOwner)))


class TRadioButton(TCustomCheckBox):
    Checked = TButtonControl._Checked
    def __init__(self, AOwner):
        self._attach(lib.TRadioButton_Create(_h(AOwner)))


class TToggleBox(TCustomCheckBox):
    """オン/オフの状態をボタン風の見た目で表す。TCustomCheckBox の直接の派生で、Checked を共有する。"""
    Checked = TButtonControl._Checked
    def __init__(self, AOwner):
        self._attach(lib.TToggleBox_Create(_h(AOwner)))


class TCustomEdit(TWinControl):
    BorderStyle = TWinControl._BorderStyle
    # 選択の開始位置(文字の数。0 から)。選択が無ければキャレットの位置。
    SelStart = _Prop("TCustomEdit_GetSelStart", "TCustomEdit_SetSelStart", _int)
    # 選択の長さ(文字の数)。
    SelLength = _Prop("TCustomEdit_GetSelLength", "TCustomEdit_SetSelLength", _int)
    # 選択している文字列。設定すると選択を置き換える(選択が無ければキャレットの位置に挿入する)。
    SelText = _Prop("TCustomEdit_GetSelText", "TCustomEdit_SetSelText", _str)
    # 利用者が内容を変えたか。Text を設定すると false に戻る。
    Modified = _Prop("TCustomEdit_GetModified", "TCustomEdit_SetModified", _bool)
    # 元に戻せる編集があるか。
    CanUndo = _Prop("TCustomEdit_GetCanUndo", None, _bool)
    # 入力した文字の代わりに表示する文字(#0 なら隠さない)。
    PasswordChar = _Prop("TCustomEdit_GetPasswordChar", "TCustomEdit_SetPasswordChar", _char)
    # 入力した文字の表示のしかた(emPassword は伏せ字、emNone は表示しない)。
    EchoMode = _Prop("TCustomEdit_GetEchoMode", "TCustomEdit_SetEchoMode", _enum("TEchoMode"))
    # 入力した英字を大文字・小文字にそろえる。
    CharCase = _Prop("TCustomEdit_GetCharCase", "TCustomEdit_SetCharCase", _enum("TEditCharCase"))
    # 文字の横の揃え。
    Alignment = _Prop("TCustomEdit_GetAlignment", "TCustomEdit_SetAlignment", _enum("TAlignment"))
    # 空のときに薄く表示する説明。
    TextHint = _Prop("TCustomEdit_GetTextHint", "TCustomEdit_SetTextHint", _str)
    # 数字だけを入力できるようにする。
    NumbersOnly = _Prop("TCustomEdit_GetNumbersOnly", "TCustomEdit_SetNumbersOnly", _bool)
    # フォーカスを受けたときに全体を選択する。
    AutoSelect = _Prop("TCustomEdit_GetAutoSelect", "TCustomEdit_SetAutoSelect", _bool)
    # フォーカスが無いときに選択の表示を隠す。
    HideSelection = _Prop("TCustomEdit_GetHideSelection", "TCustomEdit_SetHideSelection", _bool)
    # キャレットの位置(X は行の中の文字の位置、Y は行。どちらも 0 から)。
    CaretPos = _Prop("TCustomEdit_GetCaretPos", "TCustomEdit_SetCaretPos", _point_conv)
    # 全体を選択する。
    def SelectAll(self):
        lib.TCustomEdit_SelectAll(self._current())
    # 選択している文字列を消す。
    def ClearSelection(self):
        lib.TCustomEdit_ClearSelection(self._current())
    # 内容を空にする。
    def Clear(self):
        lib.TCustomEdit_Clear(self._current())
    # 選択している文字列をクリップボードに写す。
    def CopyToClipboard(self):
        lib.TCustomEdit_CopyToClipboard(self._current())
    # 選択している文字列をクリップボードに移す。
    def CutToClipboard(self):
        lib.TCustomEdit_CutToClipboard(self._current())
    # クリップボードの文字列を、選択を置き換えて貼り付ける。
    def PasteFromClipboard(self):
        lib.TCustomEdit_PasteFromClipboard(self._current())
    # 直前の編集を元に戻す。
    def Undo(self):
        lib.TCustomEdit_Undo(self._current())
    Text = TControl._Text
    MaxLength = _Prop("TCustomEdit_GetMaxLength", "TCustomEdit_SetMaxLength", _int)
    ReadOnly = _Prop("TCustomEdit_GetReadOnly", "TCustomEdit_SetReadOnly", _bool)
    OnChange = _Event("TCustomEdit_SetOnChange", "TNotifyEvent")


class TEdit(TCustomEdit):
    def __init__(self, AOwner):
        self._attach(lib.TEdit_Create(_h(AOwner)))


class TCustomFloatSpinEdit(TCustomEdit):
    """実数のスピンエディット。OnChange は基底 TCustomEdit のものをそのまま使う。"""
    Value = _Prop("TCustomFloatSpinEdit_GetValue", "TCustomFloatSpinEdit_SetValue", _float)
    MinValue = _Prop("TCustomFloatSpinEdit_GetMinValue", "TCustomFloatSpinEdit_SetMinValue", _float)
    MaxValue = _Prop("TCustomFloatSpinEdit_GetMaxValue", "TCustomFloatSpinEdit_SetMaxValue", _float)
    Increment = _Prop("TCustomFloatSpinEdit_GetIncrement", "TCustomFloatSpinEdit_SetIncrement", _float)
    DecimalPlaces = _Prop("TCustomFloatSpinEdit_GetDecimalPlaces", "TCustomFloatSpinEdit_SetDecimalPlaces", _int)


class TFloatSpinEdit(TCustomFloatSpinEdit):
    def __init__(self, AOwner):
        self._attach(lib.TFloatSpinEdit_Create(_h(AOwner)))


class TCustomSpinEdit(TCustomFloatSpinEdit):
    """整数のスピンエディット。LCL では TCustomFloatSpinEdit の派生で、Value/MinValue/MaxValue/Increment を
    Integer で再宣言して Double 版を隠す。C++ でも同じ名前の Property<int> で基底の Property<double> を隠す
    (C++ の名前隠蔽により、TCustomSpinEdit* 経由では int 版だけが見える)。"""
    Value = _Prop("TCustomSpinEdit_GetValue", "TCustomSpinEdit_SetValue", _int)
    MinValue = _Prop("TCustomSpinEdit_GetMinValue", "TCustomSpinEdit_SetMinValue", _int)
    MaxValue = _Prop("TCustomSpinEdit_GetMaxValue", "TCustomSpinEdit_SetMaxValue", _int)
    Increment = _Prop("TCustomSpinEdit_GetIncrement", "TCustomSpinEdit_SetIncrement", _int)


class TSpinEdit(TCustomSpinEdit):
    def __init__(self, AOwner):
        self._attach(lib.TSpinEdit_Create(_h(AOwner)))


class TMaskEdit(TCustomEdit):
    """書式付き入力(郵便番号・電話番号等)。EditMask は TCustomMaskEdit では protected だが、
    唯一の具象クラス TMaskEdit が published にしているため、TMaskEdit に直接置く(TUpDown と同じ形)。"""
    def __init__(self, AOwner):
        self._attach(lib.TMaskEdit_Create(_h(AOwner)))
    EditMask = _Prop("TMaskEdit_GetEditMask", "TMaskEdit_SetEditMask", _str)


class TCustomLabeledEdit(TCustomEdit):
    """ラベル付きのエディット。EditLabel は LCL が内部で生成したラベルで、初めて取得したときにラッパーが作られる(docs/adr/0028)。
    ラベルの Parent と位置は、エディットの Parent・LabelPosition・LabelSpacing に合わせて LCL が決める
    (位置の反映はフォームの配置が行われるとき。Align と同じく、表示までは行われないことがある)。"""
    EditLabel = _Prop("TCustomLabeledEdit_GetEditLabel", None, _existing("TBoundLabel"))
    LabelPosition = _Prop("TCustomLabeledEdit_GetLabelPosition", "TCustomLabeledEdit_SetLabelPosition", _enum("TLabelPosition"))
    # ラベルとエディットの間隔(既定は 3)。
    LabelSpacing = _Prop("TCustomLabeledEdit_GetLabelSpacing", "TCustomLabeledEdit_SetLabelSpacing", _int)


class TLabeledEdit(TCustomLabeledEdit):
    def __init__(self, AOwner):
        self._attach(lib.TLabeledEdit_Create(_h(AOwner)))


class TCustomTabControl(TWinControl):
    """TTabControl と TPageControl の共通の基底。以下のメンバは LCL の TCustomTabControl の public。"""
    # ページ(TPageControl なら TTabSheet)の数。TTabControl では Tabs の数と同じ。
    PageCount = _Prop("TCustomTabControl_GetPageCount", None, _int)
    MultiLine = _Prop("TCustomTabControl_GetMultiLine", "TCustomTabControl_SetMultiLine", _bool)
    ShowTabs = _Prop("TCustomTabControl_GetShowTabs", "TCustomTabControl_SetShowTabs", _bool)
    TabPosition = _Prop("TCustomTabControl_GetTabPosition", "TCustomTabControl_SetTabPosition", _enum("TTabPosition"))
    # 利用者の操作でページが切り替わる前に呼ばれる。
    OnChanging = _Event("TCustomTabControl_SetOnChanging", "TTabChangingEvent")
    # タブの画像リスト(docs/adr/0030)。各ページの画像は TCustomPage::ImageIndex。
    Images = _Prop("TCustomTabControl_GetImages", "TCustomTabControl_SetImages", _comp("TCustomImageList"))


class TTabControl(TCustomTabControl):
    """単純なタブの切り替え UI(ページはコントロール自身では管理しない)。Tabs/TabIndex/OnChange は
    LCL では TCustomTabControl の protected だが、TTabControl が独自のフィールドで再宣言して published に
    しているため、すべて TTabControl に直接置く(TUpDown と同じ形)。ページ付きのタブは TPageControl。"""
    def __init__(self, AOwner):
        self._attach(lib.TTabControl_Create(_h(AOwner)))
    TabIndex = _Prop("TTabControl_GetTabIndex", "TTabControl_SetTabIndex", _int)
    OnChange = _Event("TTabControl_SetOnChange", "TNotifyEvent")
    # 文字列の一覧(TStrings。TabControl1->Tabs->Add("x") のように使う)。
    Tabs = _Prop("TTabControl_GetTabs", None, _view("TStrings"))


class TPageControl(TCustomTabControl):
    """ページ付きのタブ。ページ(TTabSheet)は VCL と同じく、TTabSheet を生成して PageControl を設定するか、
    AddTabSheet で追加する。ページの上のコントロールは、ページを Parent にして置く。
    Pages[Index] は読み取り専用のインデックス付きプロパティ(ReadOnlyIndexedProperty)。"""
    def __init__(self, AOwner):
        self._attach(lib.TPageControl_Create(_h(AOwner)))
    # ページが 1 つも無ければ nullptr。
    ActivePage = _Prop("TPageControl_GetActivePage", "TPageControl_SetActivePage", _existing("TTabSheet"))
    ActivePageIndex = _Prop("TPageControl_GetActivePageIndex", "TPageControl_SetActivePageIndex", _int)
    # 表示されているタブの中での位置(TabVisible が false のページは数えない)。
    TabIndex = _Prop("TPageControl_GetTabIndex", "TPageControl_SetTabIndex", _int)
    # ページが切り替わった後に呼ばれる。プログラムからの ActivePage・ActivePageIndex の変更では呼ばれないが、
    # TCustomPage::PageIndex でページを並べ替えたときは(表示中のページの位置が変わるため)呼ばれる。
    OnChange = _Event("TPageControl_SetOnChange", "TNotifyEvent")
    Pages = _Indexed("TPageControl_GetPage", None, _existing("TTabSheet"))
    # ページを末尾に追加する。ページは LCL が内部で生成し、Owner はこのページコントロールになる。
    def AddTabSheet(self):
        _r = lib.TPageControl_AddTabSheet(self._current())
        return _to_existing("TTabSheet", _r)
    # すべてのページを外して破棄する。破棄は LCL の遅延破棄(Application.ReleaseComponent)で、次にメッセージを
    # 処理したとき(または Owner の破棄時)に行われ、そのときにページのラッパーも delete される。
    def Clear(self):
        lib.TPageControl_Clear(self._current())
    def SelectNextPage(self, GoForward):
        lib.TPageControl_SelectNextPage(self._current(), _b(GoForward))


class TCustomPage(TWinControl):
    """ページの共通の基底(LCL の TCustomPage。TWinControl の直接の派生)。"""
    # ページの並び順。書き換えるとタブの位置が移動する。
    PageIndex = _Prop("TCustomPage_GetPageIndex", "TCustomPage_SetPageIndex", _int)
    TabVisible = _Prop("TCustomPage_GetTabVisible", "TCustomPage_SetTabVisible", _bool)
    # ページが表示された/隠されたときに呼ばれる。
    OnShow = _Event("TCustomPage_SetOnShow", "TNotifyEvent")
    OnHide = _Event("TCustomPage_SetOnHide", "TNotifyEvent")
    # タブに表示する画像の、PageControl の Images での位置(-1 なら無し。docs/adr/0030)。
    ImageIndex = _Prop("TCustomPage_GetImageIndex", "TCustomPage_SetImageIndex", _int)


class TTabSheet(TCustomPage):
    """TPageControl のページ。タブの文字列は Caption。"""
    def __init__(self, AOwner):
        self._attach(lib.TTabSheet_Create(_h(AOwner)))
    # 設定するとそのページコントロールの末尾に追加される(nullptr で外す)。
    PageControl = _Prop("TTabSheet_GetPageControl", "TTabSheet_SetPageControl", _comp("TPageControl"))
    # 表示されているタブの中での位置(TabVisible が false なら -1)。
    TabIndex = _Prop("TTabSheet_GetTabIndex", None, _int)


class TTreeNode(TPersistent, _ItemMixin):
    """ツリービューのノード。TComponent ではない(LCL でも TPersistent)ため、new/Free() はせず、
    TTreeNodes::Add 等で追加し、Delete() 等で削除する。
    C++ のラッパーは初めて取得したときに作られ、同じノードには常に同じポインタが返る(ポインタ同士を比較してよい)。
    ノードが削除されると(ツリービューの破棄に伴う削除も含め)、OnDeletion などの削除の処理がすべて終わった後にラッパーも delete される。
    削除後にそのポインタへ触れてはならない。
    Items[Index] は直下の子(読み取り専用のインデックス付きプロパティ)。"""
    Text = _Prop("TTreeNode_GetText", "TTreeNode_SetText", _str)
    Expanded = _Prop("TTreeNode_GetExpanded", "TTreeNode_SetExpanded", _bool)
    Selected = _Prop("TTreeNode_GetSelected", "TTreeNode_SetSelected", _bool)
    # 子が無くても展開ボタンを表示するとき(子を遅延で追加するとき等)に true にする。
    HasChildren = _Prop("TTreeNode_GetHasChildren", "TTreeNode_SetHasChildren", _bool)
    # 利用者データ(LCL は解釈しない)。
    Data = _Prop("TTreeNode_GetData", "TTreeNode_SetData", _ptr)
    # 直下の子の数・兄弟の中での位置・深さ(最上位が 0)・上から順に数えた位置。
    Count = _Prop("TTreeNode_GetCount", None, _int)
    Index = _Prop("TTreeNode_GetIndex", None, _int)
    Level = _Prop("TTreeNode_GetLevel", None, _int)
    AbsoluteIndex = _Prop("TTreeNode_GetAbsoluteIndex", None, _int)
    # 最上位のノードなら nullptr。
    Parent = _Prop("TTreeNode_GetParent", None, _item("TTreeNode"))
    TreeView = _Prop("TTreeNode_GetTreeView", None, _comp("TCustomTreeView"))
    # 直下の子(Node->Items[i])。
    Items = _Indexed("TTreeNode_GetItem", None, _item("TTreeNode"))
    # 画像の、ツリービューの Images での位置(-1 なら無し。docs/adr/0030)。SelectedIndex は選択中の画像(-1 なら ImageIndex と同じ)。
    ImageIndex = _Prop("TTreeNode_GetImageIndex", "TTreeNode_SetImageIndex", _int)
    SelectedIndex = _Prop("TTreeNode_GetSelectedIndex", "TTreeNode_SetSelectedIndex", _int)
    # StateImages での位置。OverlayIndex は重ねて描く画像の、Images での位置。
    StateIndex = _Prop("TTreeNode_GetStateIndex", "TTreeNode_SetStateIndex", _int)
    OverlayIndex = _Prop("TTreeNode_GetOverlayIndex", "TTreeNode_SetOverlayIndex", _int)
    # 以下のノードを返すメンバは、該当するノードが無ければ nullptr を返す。
    def GetFirstChild(self):
        _r = lib.TTreeNode_GetFirstChild(self._current())
        return _to_item("TTreeNode", _r)
    def GetLastChild(self):
        _r = lib.TTreeNode_GetLastChild(self._current())
        return _to_item("TTreeNode", _r)
    def GetNextSibling(self):
        _r = lib.TTreeNode_GetNextSibling(self._current())
        return _to_item("TTreeNode", _r)
    def GetPrevSibling(self):
        _r = lib.TTreeNode_GetPrevSibling(self._current())
        return _to_item("TTreeNode", _r)
    # 上から順(子孫を含む)の次/前のノード。
    def GetNext(self):
        _r = lib.TTreeNode_GetNext(self._current())
        return _to_item("TTreeNode", _r)
    def GetPrev(self):
        _r = lib.TTreeNode_GetPrev(self._current())
        return _to_item("TTreeNode", _r)
    def IndexOf(self, Node):
        _r = lib.TTreeNode_IndexOf(self._current(), _h(Node))
        return _r
    def Expand(self, Recurse):
        lib.TTreeNode_Expand(self._current(), _b(Recurse))
    def Collapse(self, Recurse):
        lib.TTreeNode_Collapse(self._current(), _b(Recurse))
    # このノード(と子孫)を削除する。このラッパーも delete されるため、呼び出し後に触れてはならない。
    def Delete(self):
        lib.TTreeNode_Delete(self._current())
    def DeleteChildren(self):
        lib.TTreeNode_DeleteChildren(self._current())
    # 祖先を展開し、ノードが見えるようにスクロールする。
    def MakeVisible(self):
        lib.TTreeNode_MakeVisible(self._current())
    def MoveTo(self, Destination, Mode):
        lib.TTreeNode_MoveTo(self._current(), _h(Destination), int(Mode))


class TTreeNodes(TPersistent):
    """ツリービューのノードの一覧(LCL の TTreeNodes)。ツリービューが所有する実体への非所有のビューで、
    TCanvas と同じくツリービューのメンバとして持ち、ツリービューと寿命が一致する(TCustomTreeView::Items で参照する)。
    Sibling/Parent に nullptr を渡すと最上位のノードになる(VCL と同じ)。"""
    # すべてのノード(子孫を含む)の数。GetItem の Index は、上から順に数えた位置(AbsoluteIndex)。
    Count = _Prop("TTreeNodes_GetCount", None, _int)
    # 上から順(子孫を含む)に数えた位置のノード(TreeView1->Items->Item[i])。
    Item = _Indexed("TTreeNodes_GetItem", None, _item("TTreeNode"))
    def Add(self, Sibling, S):
        _r = lib.TTreeNodes_Add(self._current(), _h(Sibling), _enc(S))
        return _to_item("TTreeNode", _r)
    def AddFirst(self, Sibling, S):
        _r = lib.TTreeNodes_AddFirst(self._current(), _h(Sibling), _enc(S))
        return _to_item("TTreeNode", _r)
    def AddChild(self, Parent, S):
        _r = lib.TTreeNodes_AddChild(self._current(), _h(Parent), _enc(S))
        return _to_item("TTreeNode", _r)
    def AddChildFirst(self, Parent, S):
        _r = lib.TTreeNodes_AddChildFirst(self._current(), _h(Parent), _enc(S))
        return _to_item("TTreeNode", _r)
    # NextNode の前に挿入する。
    def Insert(self, NextNode, S):
        _r = lib.TTreeNodes_Insert(self._current(), _h(NextNode), _enc(S))
        return _to_item("TTreeNode", _r)
    def Clear(self):
        lib.TTreeNodes_Clear(self._current())
    def Delete(self, Node):
        lib.TTreeNodes_Delete(self._current(), _h(Node))
    def GetFirstNode(self):
        _r = lib.TTreeNodes_GetFirstNode(self._current())
        return _to_item("TTreeNode", _r)
    def FindNodeWithText(self, S):
        _r = lib.TTreeNodes_FindNodeWithText(self._current(), _enc(S))
        return _to_item("TTreeNode", _r)
    def BeginUpdate(self):
        lib.TTreeNodes_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TTreeNodes_EndUpdate(self._current())


class TCustomTreeView(TCustomControl):
    """以下のメンバは LCL の TCustomTreeView の public。"""
    BorderWidth = TWinControl._BorderWidth
    # スクロールバーの出し方(docs/adr/0048)。
    ScrollBars = _Prop("TCustomTreeView_GetScrollBars", "TCustomTreeView_SetScrollBars", _enum("TScrollStyle"))
    Items = _Prop("TCustomTreeView_GetItems", None, _obj("TTreeNodes"))
    # 選択されているノード(無ければ nullptr)。
    Selected = _Prop("TCustomTreeView_GetSelected", "TCustomTreeView_SetSelected", _item("TTreeNode"))
    # ノードの画像リスト(docs/adr/0030)。各ノードの画像は TTreeNode::ImageIndex・SelectedIndex。
    Images = _Prop("TCustomTreeView_GetImages", "TCustomTreeView_SetImages", _comp("TCustomImageList"))
    # 状態(チェック等)の画像リスト。各ノードの画像は TTreeNode::StateIndex。
    StateImages = _Prop("TCustomTreeView_GetStateImages", "TCustomTreeView_SetStateImages", _comp("TCustomImageList"))
    def FullExpand(self):
        lib.TCustomTreeView_FullExpand(self._current())
    def FullCollapse(self):
        lib.TCustomTreeView_FullCollapse(self._current())
    # ノードを文字列の順に並べ替える。
    def AlphaSort(self):
        _r = lib.TCustomTreeView_AlphaSort(self._current())
        return _r != 0
    # X, Y はクライアント座標。そこにノードが無ければ nullptr。
    def GetNodeAt(self, X, Y):
        _r = lib.TCustomTreeView_GetNodeAt(self._current(), int(X), int(Y))
        return _to_item("TTreeNode", _r)


class TTreeView(TCustomTreeView):
    """以下のメンバは LCL では TCustomTreeView の protected で、TTreeView が published にしている。"""
    def __init__(self, AOwner):
        self._attach(lib.TTreeView_Create(_h(AOwner)))
    ReadOnly = _Prop("TTreeView_GetReadOnly", "TTreeView_SetReadOnly", _bool)
    ShowLines = _Prop("TTreeView_GetShowLines", "TTreeView_SetShowLines", _bool)
    ShowRoot = _Prop("TTreeView_GetShowRoot", "TTreeView_SetShowRoot", _bool)
    ShowButtons = _Prop("TTreeView_GetShowButtons", "TTreeView_SetShowButtons", _bool)
    AutoExpand = _Prop("TTreeView_GetAutoExpand", "TTreeView_SetAutoExpand", _bool)
    HideSelection = _Prop("TTreeView_GetHideSelection", "TTreeView_SetHideSelection", _bool)
    RowSelect = _Prop("TTreeView_GetRowSelect", "TTreeView_SetRowSelect", _bool)
    # 選択が変わった後(Node は選択されたノードで、nullptr もありうる)。
    OnChange = _Event("TTreeView_SetOnChange", "TTVChangedEvent")
    # 選択が変わる前(Node は新しく選択されるノード)。
    OnChanging = _Event("TTreeView_SetOnChanging", "TTVChangingEvent")
    OnExpanding = _Event("TTreeView_SetOnExpanding", "TTVExpandingEvent")
    OnExpanded = _Event("TTreeView_SetOnExpanded", "TTVChangedEvent")
    OnCollapsing = _Event("TTreeView_SetOnCollapsing", "TTVCollapsingEvent")
    OnCollapsed = _Event("TTreeView_SetOnCollapsed", "TTVChangedEvent")
    # ノードが削除される直前(Node はまだ有効。ハンドラから戻った後にラッパーが delete される)。
    OnDeletion = _Event("TTreeView_SetOnDeletion", "TTVChangedEvent")


class TListItem(TPersistent, _ItemMixin):
    """リストビューの項目。TTreeNode と同じく TComponent ではないため、new/Free() はせず TListItems::Add 等で追加し、
    Delete() 等で削除する。同じ項目には常に同じポインタが返り、項目が削除されると(リストビューの破棄に伴う削除も含め)
    OnDeletion などの削除の処理がすべて終わった後にラッパーも delete される。
    Caption は 1 列目、SubItems は 2 列目以降の文字列(ViewStyle が vsReport のときに表示される)。
    SubItems(TStrings)は Item->SubItems->Add("x"); のように操作する。"""
    Caption = _Prop("TListItem_GetCaption", "TListItem_SetCaption", _str)
    Checked = _Prop("TListItem_GetChecked", "TListItem_SetChecked", _bool)
    Selected = _Prop("TListItem_GetSelected", "TListItem_SetSelected", _bool)
    Focused = _Prop("TListItem_GetFocused", "TListItem_SetFocused", _bool)
    # 利用者データ(LCL は解釈しない)。
    Data = _Prop("TListItem_GetData", "TListItem_SetData", _ptr)
    Index = _Prop("TListItem_GetIndex", None, _int)
    ListView = _Prop("TListItem_GetListView", None, _comp("TCustomListView"))
    # 文字列の一覧(TStrings。Item->SubItems->Add("x") のように使う)。
    SubItems = _Prop("TListItem_GetSubItems", None, _view("TStrings"))
    # 画像の、リストビューの SmallImages・LargeImages での位置(-1 なら無し。docs/adr/0030)。StateIndex は StateImages での位置。
    ImageIndex = _Prop("TListItem_GetImageIndex", "TListItem_SetImageIndex", _int)
    StateIndex = _Prop("TListItem_GetStateIndex", "TListItem_SetStateIndex", _int)
    # この項目を削除する。このラッパーも delete されるため、呼び出し後に触れてはならない。
    def Delete(self):
        lib.TListItem_Delete(self._current())
    def MakeVisible(self, PartialOK):
        lib.TListItem_MakeVisible(self._current(), _b(PartialOK))


class TListItems(TPersistent):
    """リストビューの項目の一覧(LCL の TListItems)。TTreeNodes と同じく、リストビューの値メンバとして持つ非所有のビュー。
    Item[Index] は LCL と同じ名前(Items ではない)。"""
    Count = _Prop("TListItems_GetCount", None, _int)
    # ListView1->Items->Item[i]。
    Item = _Indexed("TListItems_GetItem", None, _item("TListItem"))
    # 末尾に(Insert は Index の位置に)空の項目を追加して返す(Caption 等はその後で設定する。VCL と同じ)。
    def Add(self):
        _r = lib.TListItems_Add(self._current())
        return _to_item("TListItem", _r)
    def Insert(self, Index):
        _r = lib.TListItems_Insert(self._current(), int(Index))
        return _to_item("TListItem", _r)
    def Delete(self, Index):
        lib.TListItems_Delete(self._current(), int(Index))
    def Clear(self):
        lib.TListItems_Clear(self._current())
    def IndexOf(self, Item):
        _r = lib.TListItems_IndexOf(self._current(), _h(Item))
        return _r
    # StartIndex の次(Inclusive なら StartIndex から)から Caption を探す。Partial なら前方一致、Wrap なら末尾から先頭へ続けて探す。
    def FindCaption(self, StartIndex, Value, Partial, Inclusive, Wrap):
        _r = lib.TListItems_FindCaption(self._current(), int(StartIndex), _enc(Value), _b(Partial), _b(Inclusive), _b(Wrap))
        return _to_item("TListItem", _r)
    def Exchange(self, Index1, Index2):
        lib.TListItems_Exchange(self._current(), int(Index1), int(Index2))
    def Move(self, FromIndex, ToIndex):
        lib.TListItems_Move(self._current(), int(FromIndex), int(ToIndex))
    def BeginUpdate(self):
        lib.TListItems_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TListItems_EndUpdate(self._current())


class TListColumn(TPersistent, _ItemMixin):
    """リストビューの列(LCL の TListColumn。TCollectionItem)。項目と同じく同じ列には常に同じポインタが返る。
    列のラッパーは、列が破棄されたとき(TListColumns::Delete・Clear、リストビューの破棄)に delete される。"""
    Caption = _Prop("TListColumn_GetCaption", "TListColumn_SetCaption", _str)
    Width = _Prop("TListColumn_GetWidth", "TListColumn_SetWidth", _int)
    Alignment = _Prop("TListColumn_GetAlignment", "TListColumn_SetAlignment", _enum("TAlignment"))
    AutoSize = _Prop("TListColumn_GetAutoSize", "TListColumn_SetAutoSize", _bool)
    Visible = _Prop("TListColumn_GetVisible", "TListColumn_SetVisible", _bool)
    # 列の並び順。書き換えると列が移動する。
    Index = _Prop("TListColumn_GetIndex", "TListColumn_SetIndex", _int)
    # 見出しの画像の、SmallImages での位置(-1 なら無し。docs/adr/0030)。
    ImageIndex = _Prop("TListColumn_GetImageIndex", "TListColumn_SetImageIndex", _int)


class TListColumns(TPersistent):
    """リストビューの列の一覧(LCL の TListColumns)。リストビューの値メンバとして持つ非所有のビュー。Items[Index] で列を参照する。"""
    Count = _Prop("TListColumns_GetCount", None, _int)
    # ListView1->Columns->Items[i]。
    Items = _Indexed("TListColumns_GetItem", None, _item("TListColumn"))
    def Add(self):
        _r = lib.TListColumns_Add(self._current())
        return _to_item("TListColumn", _r)
    # 列を削除する(列のラッパーも delete される)。
    def Delete(self, Index):
        lib.TListColumns_Delete(self._current(), int(Index))
    def Clear(self):
        lib.TListColumns_Clear(self._current())


class TCustomListView(TWinControl):
    """以下のメンバは LCL の TCustomListView の public。"""
    BorderWidth = TWinControl._BorderWidth
    # スクロールバーの出し方(docs/adr/0048)。
    ScrollBars = _Prop("TCustomListView_GetScrollBars", "TCustomListView_SetScrollBars", _enum("TScrollStyle"))
    BorderStyle = TWinControl._BorderStyle
    Items = _Prop("TCustomListView_GetItems", None, _obj("TListItems"))
    # 選択されている項目(MultiSelect なら最初の 1 つ。無ければ nullptr)と、その位置(無ければ -1)。
    # 表示前(フォームのコンストラクタ等)に設定しても選択される(LCL 単体では選択されないため DLL 側で補っている)。
    Selected = _Prop("TCustomListView_GetSelected", "TCustomListView_SetSelected", _item("TListItem"))
    ItemIndex = _Prop("TCustomListView_GetItemIndex", "TCustomListView_SetItemIndex", _int)
    SelCount = _Prop("TCustomListView_GetSelCount", None, _int)
    Checkboxes = _Prop("TCustomListView_GetCheckboxes", "TCustomListView_SetCheckboxes", _bool)
    GridLines = _Prop("TCustomListView_GetGridLines", "TCustomListView_SetGridLines", _bool)
    MultiSelect = _Prop("TCustomListView_GetMultiSelect", "TCustomListView_SetMultiSelect", _bool)
    ReadOnly = _Prop("TCustomListView_GetReadOnly", "TCustomListView_SetReadOnly", _bool)
    RowSelect = _Prop("TCustomListView_GetRowSelect", "TCustomListView_SetRowSelect", _bool)
    # すべての項目を削除する(列は残る)。
    def Clear(self):
        lib.TCustomListView_Clear(self._current())
    def BeginUpdate(self):
        lib.TCustomListView_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TCustomListView_EndUpdate(self._current())
    # X, Y はクライアント座標。そこに項目が無ければ nullptr。
    def GetItemAt(self, X, Y):
        _r = lib.TCustomListView_GetItemAt(self._current(), int(X), int(Y))
        return _to_item("TListItem", _r)
    def ClearSelection(self):
        lib.TCustomListView_ClearSelection(self._current())
    def SelectAll(self):
        lib.TCustomListView_SelectAll(self._current())


class TListView(TCustomListView):
    """以下のメンバは LCL では TCustomListView の protected で、TListView が published にしている。"""
    def __init__(self, AOwner):
        self._attach(lib.TListView_Create(_h(AOwner)))
    Columns = _Prop("TListView_GetColumns", None, _obj("TListColumns"))
    # 列見出しと SubItems が表示されるのは vsReport のとき。
    ViewStyle = _Prop("TListView_GetViewStyle", "TListView_SetViewStyle", _enum("TViewStyle"))
    HideSelection = _Prop("TListView_GetHideSelection", "TListView_SetHideSelection", _bool)
    # SortType が stText のとき、SortColumn の列(0 が Caption の列)の文字列の順に並ぶ。
    # SortColumn が既定の -1 のままでは並べ替えない(LCL の仕様。先に SortColumn を設定する)。
    SortType = _Prop("TListView_GetSortType", "TListView_SetSortType", _enum("TSortType"))
    SortColumn = _Prop("TListView_GetSortColumn", "TListView_SetSortColumn", _int)
    SortDirection = _Prop("TListView_GetSortDirection", "TListView_SetSortDirection", _enum("TSortDirection"))
    # 項目の選択状態が変わったとき。
    OnSelectItem = _Event("TListView_SetOnSelectItem", "TLVSelectItemEvent")
    # 項目が変わったとき(Change は変更の種類)。
    OnChange = _Event("TListView_SetOnChange", "TLVChangeEvent")
    # 項目が削除される直前(Item はまだ有効。ハンドラから戻った後にラッパーが delete される)。
    # リストビュー自身の破棄に伴う削除では呼ばれない(LCL はリストビューの破棄通知の後に項目を削除し、
    # そのときにはリストビューのラッパーが delete されているため)。
    OnDeletion = _Event("TListView_SetOnDeletion", "TLVDeletedEvent")
    # チェックボックス(Checkboxes)が切り替わったとき。
    OnItemChecked = _Event("TListView_SetOnItemChecked", "TLVDeletedEvent")
    # 列見出しがクリックされたとき。
    OnColumnClick = _Event("TListView_SetOnColumnClick", "TLVColumnClickEvent")
    # 画像リスト(docs/adr/0030)。LCL では TCustomListView の protected で、TListView が公開する。LargeImages は vsIcon、SmallImages はそれ以外の表示形式で使う。
    LargeImages = _Prop("TListView_GetLargeImages", "TListView_SetLargeImages", _comp("TCustomImageList"))
    SmallImages = _Prop("TListView_GetSmallImages", "TListView_SetSmallImages", _comp("TCustomImageList"))
    StateImages = _Prop("TListView_GetStateImages", "TListView_SetStateImages", _comp("TCustomImageList"))


class TCustomSplitter(TCustomControl):
    """同じ Align を持つ直前のコントロール(alLeft なら、自分より左にある alLeft のコントロール)の幅・高さを
    ドラッグで変える区切りバー。Align が alLeft/alRight なら縦、alTop/alBottom なら横のバーになる(既定は alLeft)。
    メンバはすべて LCL の TCustomSplitter の public。OnCanResize/OnCanOffset(var 引数 2 つの独自のイベント形)は
    今回は未対応。SplitterPosition は LCL ではプロパティではなくメソッドの組のため、そのまま Get/Set メソッドにする。"""
    AutoSnap = _Prop("TCustomSplitter_GetAutoSnap", "TCustomSplitter_SetAutoSnap", _bool)
    Beveled = _Prop("TCustomSplitter_GetBeveled", "TCustomSplitter_SetBeveled", _bool)
    MinSize = _Prop("TCustomSplitter_GetMinSize", "TCustomSplitter_SetMinSize", _int)
    ResizeAnchor = _Prop("TCustomSplitter_GetResizeAnchor", "TCustomSplitter_SetResizeAnchor", _enum("TAnchorKind"))
    ResizeStyle = _Prop("TCustomSplitter_GetResizeStyle", "TCustomSplitter_SetResizeStyle", _enum("TResizeStyle"))
    # マウスでのドラッグが終わったときに呼ばれる(SetSplitterPosition では呼ばれない)。
    OnMoved = _Event("TCustomSplitter_SetOnMoved", "TNotifyEvent")
    # 縦のバーなら Left、横のバーなら Top にあたる(親のクライアント座標)。
    def GetSplitterPosition(self):
        _r = lib.TCustomSplitter_GetSplitterPosition(self._current())
        return _r
    def SetSplitterPosition(self, NewPosition):
        lib.TCustomSplitter_SetSplitterPosition(self._current(), int(NewPosition))


class TSplitter(TCustomSplitter):
    def __init__(self, AOwner):
        self._attach(lib.TSplitter_Create(_h(AOwner)))


class TCustomMemo(TCustomEdit):
    # 右端で折り返す(折り返すと横のスクロールバーは出ない)。
    WordWrap = _Prop("TCustomMemo_GetWordWrap", "TCustomMemo_SetWordWrap", _bool)
    # Enter で改行を入れる(false なら、フォームの既定のボタンが押される)。
    WantReturns = _Prop("TCustomMemo_GetWantReturns", "TCustomMemo_SetWantReturns", _bool)
    # Tab でタブ文字を入れる(false なら、次のコントロールにフォーカスが移る)。
    WantTabs = _Prop("TCustomMemo_GetWantTabs", "TCustomMemo_SetWantTabs", _bool)
    # 末尾に 1 行加える(Lines->Add と違い、表示を最後の行までスクロールする)。
    def Append(self, S):
        lib.TCustomMemo_Append(self._current(), _enc(S))
    # スクロールバーの出し方(docs/adr/0048 で int から TScrollStyle にした)。
    ScrollBars = _Prop("TCustomMemo_GetScrollBars", "TCustomMemo_SetScrollBars", _enum("TScrollStyle"))
    # 文字列の一覧(TStrings。Memo1->Lines->Add("x") のように使う)。
    Lines = _Prop("TCustomMemo_GetLines", None, _view("TStrings"))


class TMemo(TCustomMemo):
    def __init__(self, AOwner):
        self._attach(lib.TMemo_Create(_h(AOwner)))


class TCustomComboBox(TWinControl):
    BorderStyle = TWinControl._BorderStyle
    # 見た目と入力(csDropDownList は一覧から選ぶだけで、文字を入力できない)。
    Style = _Prop("TCustomComboBox_GetStyle", "TCustomComboBox_SetStyle", _enum("TComboBoxStyle"))
    # 一覧を開いたときに表示する項目の数。
    DropDownCount = _Prop("TCustomComboBox_GetDropDownCount", "TCustomComboBox_SetDropDownCount", _int)
    # 項目を並べ替えて表示する。
    Sorted = _Prop("TCustomComboBox_GetSorted", "TCustomComboBox_SetSorted", _bool)
    # 文字を入力できない(一覧から選ぶことはできる)。
    ReadOnly = _Prop("TCustomComboBox_GetReadOnly", "TCustomComboBox_SetReadOnly", _bool)
    # 一覧が開いているか。設定すると開く・閉じる。
    DroppedDown = _Prop("TCustomComboBox_GetDroppedDown", "TCustomComboBox_SetDroppedDown", _bool)
    # 入力した文字で始まる項目を補う。
    AutoComplete = _Prop("TCustomComboBox_GetAutoComplete", "TCustomComboBox_SetAutoComplete", _bool)
    # 一覧から項目を選んだとき(文字の入力では呼ばれない)。
    OnSelect = _Event("TCustomComboBox_SetOnSelect", "TNotifyEvent")
    # 一覧を開く直前。
    OnDropDown = _Event("TCustomComboBox_SetOnDropDown", "TNotifyEvent")
    # 一覧を閉じたとき。
    OnCloseUp = _Event("TCustomComboBox_SetOnCloseUp", "TNotifyEvent")
    Text = TControl._Text
    ItemIndex = _Prop("TCustomComboBox_GetItemIndex", "TCustomComboBox_SetItemIndex", _int)
    # 文字列の一覧(TStrings。ComboBox1->Items->Add("x") のように使う)。
    Items = _Prop("TCustomComboBox_GetItems", None, _view("TStrings"))
    # ---- docs/adr/0050 ----
    # Style が csOwnerDrawFixed・csOwnerDrawVariable 等なら、一覧の項目を OnDrawItem で描く。
    # 項目の高さ(lbOwnerDrawFixed 等で使う)。
    ItemHeight = _Prop("TCustomComboBox_GetItemHeight", "TCustomComboBox_SetItemHeight", _int)
    # OnDrawItem の中で描く先。コントロールが所有する実体への非所有のビュー。
    Canvas = _Prop("TCustomComboBox_GetCanvas", None, _obj("TCanvas"))
    OnDrawItem = _Event("TCustomComboBox_SetOnDrawItem", "TDrawItemEvent")
    OnMeasureItem = _Event("TCustomComboBox_SetOnMeasureItem", "TMeasureItemEvent")


class TComboBox(TCustomComboBox):
    # LCL では TCustomComboBox の protected で、公開しているのは TComboBox だけ。
    OnChange = _Event("TComboBox_SetOnChange", "TNotifyEvent")
    def __init__(self, AOwner):
        self._attach(lib.TComboBox_Create(_h(AOwner)))


class TCustomListBox(TWinControl):
    """利用者による選択の変更(マウス・キー操作とも)では OnClick が呼ばれる(VCL と同じ)。
    プログラムからの ItemIndex の変更では呼ばれない。"""
    BorderStyle = TWinControl._BorderStyle
    # 複数の項目を選べるようにする(選んだ項目は Selected[i])。
    MultiSelect = _Prop("TCustomListBox_GetMultiSelect", "TCustomListBox_SetMultiSelect", _bool)
    # MultiSelect のとき、Shift・Ctrl で範囲・追加の選択をする(false なら、クリックのたびに選択を切り替える)。
    ExtendedSelect = _Prop("TCustomListBox_GetExtendedSelect", "TCustomListBox_SetExtendedSelect", _bool)
    # 項目を並べ替えて表示する(Items への追加も並べ替えた位置に入る)。
    Sorted = _Prop("TCustomListBox_GetSorted", "TCustomListBox_SetSorted", _bool)
    # 一番上に表示している項目。
    TopIndex = _Prop("TCustomListBox_GetTopIndex", "TCustomListBox_SetTopIndex", _int)
    # 選んでいる項目の数(MultiSelect のとき)。
    SelCount = _Prop("TCustomListBox_GetSelCount", None, _int)
    # 項目が選ばれているか(ListBox1->Selected[i])。
    Selected = _Indexed("TCustomListBox_GetSelected", "TCustomListBox_SetSelected", _bool)
    # 選択をすべて外す。
    def ClearSelection(self):
        lib.TCustomListBox_ClearSelection(self._current())
    # すべての項目を選ぶ(MultiSelect のとき)。
    def SelectAll(self):
        lib.TCustomListBox_SelectAll(self._current())
    # クライアント領域の座標にある項目。無ければ -1(LCL では Existing によらない。VCL との互換のために受け取る)。
    def ItemAtPos(self, Pos, Existing):
        _r = lib.TCustomListBox_ItemAtPos(self._current(), *_point(Pos), _b(Existing))
        return _r
    # 選択が変わったとき。User は利用者の操作によるものか(プログラムからの変更なら false)。
    OnSelectionChange = _Event("TCustomListBox_SetOnSelectionChange", "TSelectionChangeEvent")
    ItemIndex = _Prop("TCustomListBox_GetItemIndex", "TCustomListBox_SetItemIndex", _int)
    # 文字列の一覧(TStrings。ListBox1->Items->Add("x") のように使う)。
    Items = _Prop("TCustomListBox_GetItems", None, _view("TStrings"))
    # ---- docs/adr/0050 ----
    # 描き方(lbOwnerDrawFixed・lbOwnerDrawVariable なら OnDrawItem で描く)。
    Style = _Prop("TCustomListBox_GetStyle", "TCustomListBox_SetStyle", _enum("TListBoxStyle"))
    # 項目の高さ(lbOwnerDrawFixed 等で使う)。
    ItemHeight = _Prop("TCustomListBox_GetItemHeight", "TCustomListBox_SetItemHeight", _int)
    # OnDrawItem の中で描く先。コントロールが所有する実体への非所有のビュー。
    Canvas = _Prop("TCustomListBox_GetCanvas", None, _obj("TCanvas"))
    OnDrawItem = _Event("TCustomListBox_SetOnDrawItem", "TDrawItemEvent")
    OnMeasureItem = _Event("TCustomListBox_SetOnMeasureItem", "TMeasureItemEvent")


class TListBox(TCustomListBox):
    def __init__(self, AOwner):
        self._attach(lib.TListBox_Create(_h(AOwner)))


class TCustomCheckListBox(TCustomListBox):
    """各項目にチェックボックスを持つリストボックス。Items は基底 TCustomListBox のものをそのまま使う。"""
    OnClickCheck = _Event("TCustomCheckListBox_SetOnClickCheck", "TNotifyEvent")
    # 項目ごとのチェックの状態(CheckListBox1->Checked[i] = true;)。
    Checked = _Indexed("TCustomCheckListBox_GetChecked", "TCustomCheckListBox_SetChecked", _bool)


class TCheckListBox(TCustomCheckListBox):
    def __init__(self, AOwner):
        self._attach(lib.TCheckListBox_Create(_h(AOwner)))


class TCustomStaticText(TWinControl):
    BorderStyle = _Prop("TCustomStaticText_GetBorderStyle", "TCustomStaticText_SetBorderStyle", _enum("TStaticBorderStyle"))


class TStaticText(TCustomStaticText):
    def __init__(self, AOwner):
        self._attach(lib.TStaticText_Create(_h(AOwner)))


class TStatusPanel(TPersistent, _ItemMixin):
    """ステータスバーのパネル(LCL の TStatusPanel。TCollectionItem。docs/adr/0044)。同じパネルには常に同じポインタが返る。
    ラッパーは、パネルが破棄されたとき(TStatusPanels::Delete・Clear、ステータスバーの破棄)に delete される。"""
    Text = _Prop("TStatusPanel_GetText", "TStatusPanel_SetText", _str)
    # 幅(最後のパネルは残りの幅いっぱいに広がる)。
    Width = _Prop("TStatusPanel_GetWidth", "TStatusPanel_SetWidth", _int)
    Alignment = _Prop("TStatusPanel_GetAlignment", "TStatusPanel_SetAlignment", _enum("TAlignment"))
    Bevel = _Prop("TStatusPanel_GetBevel", "TStatusPanel_SetBevel", _enum("TStatusPanelBevel"))
    Style = _Prop("TStatusPanel_GetStyle", "TStatusPanel_SetStyle", _enum("TStatusPanelStyle"))
    # 並び順。書き換えるとパネルが移動する。
    Index = _Prop("TStatusPanel_GetIndex", "TStatusPanel_SetIndex", _int)


class TStatusPanels(TPersistent):
    """パネルの一覧(LCL の TStatusPanels。TCollection)。ステータスバーの値メンバとして持つ非所有のビュー。"""
    Count = _Prop("TStatusPanels_GetCount", None, _int)
    # StatusBar1->Panels->Items[i]。
    Items = _Indexed("TStatusPanels_GetItem", None, _item("TStatusPanel"))
    # 末尾に(Insert は Index の位置に)空のパネルを追加して返す(Text・Width はその後で設定する。VCL と同じ)。
    def Add(self):
        _r = lib.TStatusPanels_Add(self._current())
        return _to_item("TStatusPanel", _r)
    def Insert(self, Index):
        _r = lib.TStatusPanels_Insert(self._current(), int(Index))
        return _to_item("TStatusPanel", _r)
    # パネルを削除する(パネルのラッパーも delete される)。
    def Delete(self, Index):
        lib.TStatusPanels_Delete(self._current(), int(Index))
    def Clear(self):
        lib.TStatusPanels_Clear(self._current())
    def BeginUpdate(self):
        lib.TStatusPanels_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TStatusPanels_EndUpdate(self._current())


class TStatusBar(TWinControl):
    """ステータス行。LCL では中間の TCustomStatusBar が無く、TWinControl の直接の派生。
    パネルを表示するには SimplePanel を false にする(LCL の既定は true で、SimpleText だけを表示する。VCL の既定は false)。
    他のコントロールと同じく、フォームのコンストラクタの中で生成・配置してよい
    (LCL の Win32 実装が DLL で失敗する問題は DLL 側で回避済み。docs/adr/0015-... を参照)。"""
    def __init__(self, AOwner):
        self._attach(lib.TStatusBar_Create(_h(AOwner)))
    SimpleText = _Prop("TStatusBar_GetSimpleText", "TStatusBar_SetSimpleText", _str)
    SimplePanel = _Prop("TStatusBar_GetSimplePanel", "TStatusBar_SetSimplePanel", _bool)
    Panels = _Prop("TStatusBar_GetPanels", None, _obj("TStatusPanels"))
    # 右下のサイズ変更のつまみを出すか(フォームの右下にあり、フォームの大きさを変えられるときだけ出る)。
    SizeGrip = _Prop("TStatusBar_GetSizeGrip", "TStatusBar_SetSizeGrip", _bool)
    # true なら、Application のヒント(コントロールの Hint)をステータスバーに表示する
    # (SimplePanel なら SimpleText、そうでなければ最初のパネルに。OnHint を設定すると、代わりに OnHint を呼ぶ)。
    # LCL は ShowHint が true のコントロール(か親)にだけ Application のヒントを設定する(VCL は ShowHint によらない)。
    AutoHint = _Prop("TStatusBar_GetAutoHint", "TStatusBar_SetAutoHint", _bool)
    # OnDrawPanel の中で描画する先。ステータスバーが所有する実体への非所有のビュー(TPaintBox::Canvas と同じ)。
    Canvas = _Prop("TStatusBar_GetCanvas", None, _obj("TCanvas"))
    OnDrawPanel = _Event("TStatusBar_SetOnDrawPanel", "TDrawPanelEvent")
    # AutoHint のとき、ヒントを表示する代わりに呼ばれる(ヒントは Application->Hint)。
    OnHint = _Event("TStatusBar_SetOnHint", "TNotifyEvent")
    # クライアント座標 (X, Y) にあるパネルの位置。無ければ -1。
    def GetPanelIndexAt(self, X, Y):
        _r = lib.TStatusBar_GetPanelIndexAt(self._current(), int(X), int(Y))
        return _r
    # パネルをまとめて変えるとき、EndUpdate まで再描画を止める。
    def BeginUpdate(self):
        lib.TStatusBar_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TStatusBar_EndUpdate(self._current())


class TCustomShape(TGraphicControl):
    Pen = _Prop("TCustomShape_GetPen", None, _obj("TPen"))
    Brush = _Prop("TCustomShape_GetBrush", None, _obj("TBrush"))
    Shape = _Prop("TCustomShape_GetShape", "TCustomShape_SetShape", _enum("TShapeType"))


class TShape(TCustomShape):
    def __init__(self, AOwner):
        self._attach(lib.TShape_Create(_h(AOwner)))


class TCustomSpeedButton(TGraphicControl):
    """クリックで押し込まれた状態を保つ(GroupIndex でラジオボタン風のグループも作れる)グラフィックボタン。
    Down/GroupIndex/Flat/AllowAllUp はいずれも LCL では public。Caption/OnClick は TControl から共有する。
    Glyph 等の意味は TCustomBitBtn と同じ。"""
    Down = _Prop("TCustomSpeedButton_GetDown", "TCustomSpeedButton_SetDown", _bool)
    GroupIndex = _Prop("TCustomSpeedButton_GetGroupIndex", "TCustomSpeedButton_SetGroupIndex", _int)
    Flat = _Prop("TCustomSpeedButton_GetFlat", "TCustomSpeedButton_SetFlat", _bool)
    AllowAllUp = _Prop("TCustomSpeedButton_GetAllowAllUp", "TCustomSpeedButton_SetAllowAllUp", _bool)
    Glyph = _Prop("TCustomSpeedButton_GetGlyph", "TCustomSpeedButton_SetGlyph", _view("TBitmap"))
    NumGlyphs = _Prop("TCustomSpeedButton_GetNumGlyphs", "TCustomSpeedButton_SetNumGlyphs", _int)
    Layout = _Prop("TCustomSpeedButton_GetLayout", "TCustomSpeedButton_SetLayout", _enum("TButtonLayout"))
    Margin = _Prop("TCustomSpeedButton_GetMargin", "TCustomSpeedButton_SetMargin", _int)
    Spacing = _Prop("TCustomSpeedButton_GetSpacing", "TCustomSpeedButton_SetSpacing", _int)
    # 画像リスト(docs/adr/0030)。意味は TCustomBitBtn と同じ。
    Images = _Prop("TCustomSpeedButton_GetImages", "TCustomSpeedButton_SetImages", _comp("TCustomImageList"))
    ImageIndex = _Prop("TCustomSpeedButton_GetImageIndex", "TCustomSpeedButton_SetImageIndex", _int)


class TSpeedButton(TCustomSpeedButton):
    def __init__(self, AOwner):
        self._attach(lib.TSpeedButton_Create(_h(AOwner)))


class TPaintBox(TGraphicControl):
    Canvas = _Prop("TPaintBox_GetCanvas", None, _obj("TCanvas"))
    OnPaint = _Event("TPaintBox_SetOnPaint", "TNotifyEvent")
    def __init__(self, AOwner):
        self._attach(lib.TPaintBox_Create(_h(AOwner)))


class TCustomImage(TGraphicControl):
    """画像を表示するコントロール(LCL の TCustomImage。docs/adr/0029)。AutoSize は TControl のもの。"""
    # 表示する画像。コントロールが所有し、コントロールと寿命が一致する。代入は内容のコピー。
    Picture = _Prop("TCustomImage_GetPicture", "TCustomImage_SetPicture", _obj("TPicture"))
    # 画像に描く先。Picture が空なら、コントロールの大きさの TBitmap を作ってからその Canvas を返す(描いた内容は Picture に残る)。
    # Picture の中身が作り直されると別のものになる(ポインタを保存しないこと)。
    Canvas = _Prop("TCustomImage_GetCanvas", None, _obj("TCanvas"))
    HasGraphic = _Prop("TCustomImage_GetHasGraphic", None, _bool)
    Center = _Prop("TCustomImage_GetCenter", "TCustomImage_SetCenter", _bool)
    # コントロールの大きさに伸縮する。StretchOutEnabled・StretchInEnabled を false にすると、拡大・縮小を個別に禁止できる。
    Stretch = _Prop("TCustomImage_GetStretch", "TCustomImage_SetStretch", _bool)
    StretchOutEnabled = _Prop("TCustomImage_GetStretchOutEnabled", "TCustomImage_SetStretchOutEnabled", _bool)
    StretchInEnabled = _Prop("TCustomImage_GetStretchInEnabled", "TCustomImage_SetStretchInEnabled", _bool)
    # 縦横比を保ってコントロールに収める。
    Proportional = _Prop("TCustomImage_GetProportional", "TCustomImage_SetProportional", _bool)
    Transparent = _Prop("TCustomImage_GetTransparent", "TCustomImage_SetTransparent", _bool)
    # Picture(またはその中身)が変わったときに呼ばれる。
    OnPictureChanged = _Event("TCustomImage_SetOnPictureChanged", "TNotifyEvent")
    # 画像リスト(docs/adr/0030)。設定すると、Picture が空のとき Images の ImageIndex 番目の画像を表示する。
    Images = _Prop("TCustomImage_GetImages", "TCustomImage_SetImages", _comp("TCustomImageList"))
    ImageIndex = _Prop("TCustomImage_GetImageIndex", "TCustomImage_SetImageIndex", _int)


class TImage(TCustomImage):
    def __init__(self, AOwner):
        self._attach(lib.TImage_Create(_h(AOwner)))


class TCustomGrid(TCustomControl):
    """グリッドの共通の基底。以下のメンバは LCL の TCustomGrid の public。
    セルは(列, 行)の位置で指定する(0 始まり。固定行・固定列を含む)。"""
    def BeginUpdate(self):
        lib.TCustomGrid_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TCustomGrid_EndUpdate(self._current())
    # すべての行・列を削除する(ColCount・RowCount が 0 になる)。セルの文字列だけを消すのは TCustomStringGrid::Clean。
    def Clear(self):
        lib.TCustomGrid_Clear(self._current())
    # セルのクライアント座標での矩形。
    def CellRect(self, ACol, ARow):
        _r = [ctypes.c_int() for _ in range(4)]
        lib.TCustomGrid_CellRect(self._current(), int(ACol), int(ARow), *(ctypes.byref(x) for x in _r))
        return TRect(*(x.value for x in _r))
    # クライアント座標 X, Y にあるセル。セルの外なら -1。
    def MouseToCell(self, X, Y, ACol, ARow):
        _out_ACol = ctypes.c_int()
        _out_ARow = ctypes.c_int()
        lib.TCustomGrid_MouseToCell(self._current(), int(X), int(Y), ctypes.byref(_out_ACol), ctypes.byref(_out_ARow))
        ACol.value = _out_ACol.value
        ARow.value = _out_ARow.value


class TCustomDrawGrid(TCustomGrid):
    """以下のメンバは LCL では TCustomGrid の protected で、TCustomDrawGrid が public にしている。"""
    # スクロールバーの出し方(docs/adr/0048)。
    ScrollBars = _Prop("TCustomDrawGrid_GetScrollBars", "TCustomDrawGrid_SetScrollBars", _enum("TScrollStyle"))
    ColCount = _Prop("TCustomDrawGrid_GetColCount", "TCustomDrawGrid_SetColCount", _int)
    RowCount = _Prop("TCustomDrawGrid_GetRowCount", "TCustomDrawGrid_SetRowCount", _int)
    # 固定列・固定行(見出し)の数。既定は 1。
    FixedCols = _Prop("TCustomDrawGrid_GetFixedCols", "TCustomDrawGrid_SetFixedCols", _int)
    FixedRows = _Prop("TCustomDrawGrid_GetFixedRows", "TCustomDrawGrid_SetFixedRows", _int)
    # 現在のセル(フォーカスのあるセル)の列・行。
    Col = _Prop("TCustomDrawGrid_GetCol", "TCustomDrawGrid_SetCol", _int)
    Row = _Prop("TCustomDrawGrid_GetRow", "TCustomDrawGrid_SetRow", _int)
    DefaultColWidth = _Prop("TCustomDrawGrid_GetDefaultColWidth", "TCustomDrawGrid_SetDefaultColWidth", _int)
    DefaultRowHeight = _Prop("TCustomDrawGrid_GetDefaultRowHeight", "TCustomDrawGrid_SetDefaultRowHeight", _int)
    Options = _Prop("TCustomDrawGrid_GetOptions", "TCustomDrawGrid_SetOptions", _enum("TGridOptions"))
    # 選択範囲(単一のセルなら Left = Right、Top = Bottom)。
    Selection = _Prop("TCustomDrawGrid_GetSelection", "TCustomDrawGrid_SetSelection", _rect_conv)
    # スクロール位置(表示されている最初の列・行)。
    LeftCol = _Prop("TCustomDrawGrid_GetLeftCol", "TCustomDrawGrid_SetLeftCol", _int)
    TopRow = _Prop("TCustomDrawGrid_GetTopRow", "TCustomDrawGrid_SetTopRow", _int)
    # false にすると、OnDrawCell の前にセルの既定の描画(背景・文字列)を行わない。
    DefaultDrawing = _Prop("TCustomDrawGrid_GetDefaultDrawing", "TCustomDrawGrid_SetDefaultDrawing", _bool)
    FixedColor = _Prop("TCustomDrawGrid_GetFixedColor", "TCustomDrawGrid_SetFixedColor", _int)
    # セルの編集中か(goEditing のとき)。true を設定すると現在のセルの編集を始める。
    EditorMode = _Prop("TCustomDrawGrid_GetEditorMode", "TCustomDrawGrid_SetEditorMode", _bool)
    OnDrawCell = _Event("TCustomDrawGrid_SetOnDrawCell", "TOnDrawCell")
    OnSelectCell = _Event("TCustomDrawGrid_SetOnSelectCell", "TOnSelectCellEvent")
    OnSelection = _Event("TCustomDrawGrid_SetOnSelection", "TOnSelectEvent")
    OnHeaderClick = _Event("TCustomDrawGrid_SetOnHeaderClick", "THdrEvent")
    # 列ごとの幅・行ごとの高さ(Grid->ColWidths[0] = 80;)。
    ColWidths = _Indexed("TCustomDrawGrid_GetColWidths", "TCustomDrawGrid_SetColWidths", _int)
    RowHeights = _Indexed("TCustomDrawGrid_GetRowHeights", "TCustomDrawGrid_SetRowHeights", _int)
    def InsertColRow(self, IsColumn, Index):
        lib.TCustomDrawGrid_InsertColRow(self._current(), _b(IsColumn), int(Index))
    def DeleteColRow(self, IsColumn, Index):
        lib.TCustomDrawGrid_DeleteColRow(self._current(), _b(IsColumn), int(Index))
    def MoveColRow(self, IsColumn, FromIndex, ToIndex):
        lib.TCustomDrawGrid_MoveColRow(self._current(), _b(IsColumn), int(FromIndex), int(ToIndex))
    # IsColumn が true なら、列 Index の値で行を並べ替える(固定行は除く)。false なら行 Index の値で列を並べ替える。
    def SortColRow(self, IsColumn, Index):
        lib.TCustomDrawGrid_SortColRow(self._current(), _b(IsColumn), int(Index))


class TDrawGrid(TCustomDrawGrid):
    """セルの内容を OnDrawCell で利用者が描画するグリッド(セルの文字列は持たない)。"""
    def __init__(self, AOwner):
        self._attach(lib.TDrawGrid_Create(_h(AOwner)))


class TCustomStringGrid(TCustomDrawGrid):
    """以下のメンバは LCL の TCustomStringGrid の public。"""
    # セルの文字列。C++Builder と同じく StringGrid1->Cells[ACol][ARow] = "x"; と書く(1 つ目が列、2 つ目が行)。
    Cells = _Indexed("TCustomStringGrid_GetCells", "TCustomStringGrid_SetCells", _str, dims=2)
    # すべてのセルの文字列を消す(行・列の数は変わらない)。
    def Clean(self):
        lib.TCustomStringGrid_Clean(self._current())
    # 列の幅を文字列に合わせる。
    def AutoSizeColumns(self):
        lib.TCustomStringGrid_AutoSizeColumns(self._current())
    def AutoSizeColumn(self, ACol):
        lib.TCustomStringGrid_AutoSizeColumn(self._current(), int(ACol))


class TStringGrid(TCustomStringGrid):
    """セルごとに文字列を持つグリッド。"""
    def __init__(self, AOwner):
        self._attach(lib.TStringGrid_Create(_h(AOwner)))


class THeaderSection(TPersistent, _ItemMixin):
    """ヘッダーコントロールのセクション(LCL の THeaderSection。TCollectionItem)。同じセクションには常に同じポインタが返る。
    ラッパーは、セクションが破棄されたとき(THeaderSections::Delete・Clear、ヘッダーコントロールの破棄)に delete される。"""
    Text = _Prop("THeaderSection_GetText", "THeaderSection_SetText", _str)
    # Visible が false なら 0 を返す。
    Width = _Prop("THeaderSection_GetWidth", "THeaderSection_SetWidth", _int)
    MinWidth = _Prop("THeaderSection_GetMinWidth", "THeaderSection_SetMinWidth", _int)
    MaxWidth = _Prop("THeaderSection_GetMaxWidth", "THeaderSection_SetMaxWidth", _int)
    Alignment = _Prop("THeaderSection_GetAlignment", "THeaderSection_SetAlignment", _enum("TAlignment"))
    Visible = _Prop("THeaderSection_GetVisible", "THeaderSection_SetVisible", _bool)
    # 並び順。書き換えるとセクションが移動する。
    Index = _Prop("THeaderSection_GetIndex", "THeaderSection_SetIndex", _int)
    # クライアント座標での左端・右端。
    Left = _Prop("THeaderSection_GetLeft", None, _int)
    Right = _Prop("THeaderSection_GetRight", None, _int)
    # 並べ替えても変わらない位置。
    OriginalIndex = _Prop("THeaderSection_GetOriginalIndex", None, _int)
    # 画像の、ヘッダーの Images での位置(-1 なら無し。docs/adr/0030)。
    ImageIndex = _Prop("THeaderSection_GetImageIndex", "THeaderSection_SetImageIndex", _int)


class THeaderSections(TPersistent):
    """セクションの一覧(LCL の THeaderSections。TCollection)。ヘッダーコントロールの値メンバとして持つ非所有のビュー。"""
    Count = _Prop("THeaderSections_GetCount", None, _int)
    # HeaderControl1->Sections->Items[i]。
    Items = _Indexed("THeaderSections_GetItem", None, _item("THeaderSection"))
    # 末尾に(Insert は Index の位置に)空のセクションを追加して返す(Text 等はその後で設定する。VCL と同じ)。
    def Add(self):
        _r = lib.THeaderSections_Add(self._current())
        return _to_item("THeaderSection", _r)
    def Insert(self, Index):
        _r = lib.THeaderSections_Insert(self._current(), int(Index))
        return _to_item("THeaderSection", _r)
    # セクションを削除する(セクションのラッパーも delete される)。
    def Delete(self, Index):
        lib.THeaderSections_Delete(self._current(), int(Index))
    def Clear(self):
        lib.THeaderSections_Clear(self._current())
    def BeginUpdate(self):
        lib.THeaderSections_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.THeaderSections_EndUpdate(self._current())


class TCustomHeaderControl(TCustomControl):
    """以下のメンバは LCL の TCustomHeaderControl の public/published。セクションは OS のコントロールではなく LCL が描画する。"""
    Sections = _Prop("TCustomHeaderControl_GetSections", None, _obj("THeaderSections"))
    # true にすると、セクションをドラッグで並べ替えられる。
    DragReorder = _Prop("TCustomHeaderControl_GetDragReorder", "TCustomHeaderControl_SetDragReorder", _bool)
    # 並べ替えても変わらない位置(OriginalIndex)のセクション。無ければ nullptr。
    SectionFromOriginalIndex = _Indexed("TCustomHeaderControl_GetSectionFromOriginalIndex", None, _item("THeaderSection"))
    # セクションがクリックされたとき。
    OnSectionClick = _Event("TCustomHeaderControl_SetOnSectionClick", "TCustomSectionNotifyEvent")
    # ドラッグで幅を変え終えたとき。
    OnSectionResize = _Event("TCustomHeaderControl_SetOnSectionResize", "TCustomSectionNotifyEvent")
    # セクションの境界がダブルクリックされたとき。
    OnSectionSeparatorDblClick = _Event("TCustomHeaderControl_SetOnSectionSeparatorDblClick", "TCustomSectionNotifyEvent")
    # ドラッグで幅を変えている間(開始・移動・終了)。
    OnSectionTrack = _Event("TCustomHeaderControl_SetOnSectionTrack", "TCustomSectionTrackEvent")
    OnSectionDrag = _Event("TCustomHeaderControl_SetOnSectionDrag", "TSectionDragEvent")
    # ドラッグでの並べ替えが終わったとき。
    OnSectionEndDrag = _Event("TCustomHeaderControl_SetOnSectionEndDrag", "TNotifyEvent")
    # セクションの画像リスト(docs/adr/0030)。各セクションの画像は THeaderSection::ImageIndex。
    Images = _Prop("TCustomHeaderControl_GetImages", "TCustomHeaderControl_SetImages", _comp("TCustomImageList"))
    # クライアント座標 P にあるセクションの位置。無ければ -1。
    def GetSectionAt(self, P):
        _r = lib.TCustomHeaderControl_GetSectionAt(self._current(), *_point(P))
        return _r


class THeaderControl(TCustomHeaderControl):
    """列の見出しを並べたコントロール。"""
    def __init__(self, AOwner):
        self._attach(lib.THeaderControl_Create(_h(AOwner)))


class TToolWindow(TCustomControl):
    """以下のメンバは LCL の TToolWindow の public(TToolBar が published にしている)。"""
    EdgeBorders = _Prop("TToolWindow_GetEdgeBorders", "TToolWindow_SetEdgeBorders", _enum("TEdgeBorders"))
    EdgeInner = _Prop("TToolWindow_GetEdgeInner", "TToolWindow_SetEdgeInner", _enum("TEdgeStyle"))
    EdgeOuter = _Prop("TToolWindow_GetEdgeOuter", "TToolWindow_SetEdgeOuter", _enum("TEdgeStyle"))
    def BeginUpdate(self):
        lib.TToolWindow_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TToolWindow_EndUpdate(self._current())


class TToolBar(TToolWindow):
    """ツールバー。ボタン(TToolButton)は、Parent をツールバーにすると追加される(VCL と同じ)。既定の Align は alTop。"""
    def __init__(self, AOwner):
        self._attach(lib.TToolBar_Create(_h(AOwner)))
    ButtonCount = _Prop("TToolBar_GetButtonCount", None, _int)
    # 並び順のボタン(ToolBar1->Buttons[i])。
    Buttons = _Indexed("TToolBar_GetButton", None, _comp("TToolButton"))
    # LCL では行数ではなく、Wrapable が false のときに Wrap のボタンで折り返した回数(折り返しが無ければ 0)。
    RowCount = _Prop("TToolBar_GetRowCount", None, _int)
    ButtonHeight = _Prop("TToolBar_GetButtonHeight", "TToolBar_SetButtonHeight", _int)
    ButtonWidth = _Prop("TToolBar_GetButtonWidth", "TToolBar_SetButtonWidth", _int)
    # tbsDropDown のボタンの矢印部分の幅。
    DropDownWidth = _Prop("TToolBar_GetDropDownWidth", "TToolBar_SetDropDownWidth", _int)
    # 最初のボタンの左の余白。
    Indent = _Prop("TToolBar_GetIndent", "TToolBar_SetIndent", _int)
    Flat = _Prop("TToolBar_GetFlat", "TToolBar_SetFlat", _bool)
    # true なら、ボタンの文字をアイコンの右に置く。
    List = _Prop("TToolBar_GetList", "TToolBar_SetList", _bool)
    # true なら、ボタンに Caption を表示する(既定は false)。
    ShowCaptions = _Prop("TToolBar_GetShowCaptions", "TToolBar_SetShowCaptions", _bool)
    Transparent = _Prop("TToolBar_GetTransparent", "TToolBar_SetTransparent", _bool)
    # true なら、幅に収まらないボタンを次の行へ折り返す(既定は true)。
    Wrapable = _Prop("TToolBar_GetWrapable", "TToolBar_SetWrapable", _bool)
    # ボタンの画像リスト(docs/adr/0030)。HotImages はマウスが上にあるとき、DisabledImages は無効のときに使う(設定しなければ Images から LCL が作る)。各ボタンの画像は TToolButton::ImageIndex。
    Images = _Prop("TToolBar_GetImages", "TToolBar_SetImages", _comp("TCustomImageList"))
    HotImages = _Prop("TToolBar_GetHotImages", "TToolBar_SetHotImages", _comp("TCustomImageList"))
    DisabledImages = _Prop("TToolBar_GetDisabledImages", "TToolBar_SetDisabledImages", _comp("TCustomImageList"))
    def SetButtonSize(self, NewButtonWidth, NewButtonHeight):
        lib.TToolBar_SetButtonSize(self._current(), int(NewButtonWidth), int(NewButtonHeight))


class TToolButton(TGraphicControl):
    """ツールバーのボタン。Caption・OnClick は TControl のものを使う。"""
    def __init__(self, AOwner):
        self._attach(lib.TToolButton_Create(_h(AOwner)))
    # tbsCheck で Grouped のとき、すべてのボタンを上げた状態にできるか。
    AllowAllUp = _Prop("TToolButton_GetAllowAllUp", "TToolButton_SetAllowAllUp", _bool)
    # 押された状態(tbsCheck はクリックで切り替わる)。
    Down = _Prop("TToolButton_GetDown", "TToolButton_SetDown", _bool)
    Grouped = _Prop("TToolButton_GetGrouped", "TToolButton_SetGrouped", _bool)
    Indeterminate = _Prop("TToolButton_GetIndeterminate", "TToolButton_SetIndeterminate", _bool)
    Marked = _Prop("TToolButton_GetMarked", "TToolButton_SetMarked", _bool)
    ShowCaption = _Prop("TToolButton_GetShowCaption", "TToolButton_SetShowCaption", _bool)
    # true なら、このボタンの後で行を折り返す。
    Wrap = _Prop("TToolButton_GetWrap", "TToolButton_SetWrap", _bool)
    Style = _Prop("TToolButton_GetStyle", "TToolButton_SetStyle", _enum("TToolButtonStyle"))
    # tbsDropDown・tbsButtonDrop の矢印で表示するポップアップメニュー。
    DropdownMenu = _Prop("TToolButton_GetDropdownMenu", "TToolButton_SetDropdownMenu", _comp("TPopupMenu"))
    # 設定すると、そのメニュー項目の Caption・Enabled 等を写す。マウスで押すと、その項目の OnClick を呼んでから
    # 子の項目をポップアップメニューとして表示する(DropdownMenu と同じく、メニューを閉じるまで戻らない)。
    MenuItem = _Prop("TToolButton_GetMenuItem", "TToolButton_SetMenuItem", _existing("TMenuItem"))
    # tbsDropDown・tbsButtonDrop の矢印がクリックされたとき(DropdownMenu を表示する前)。
    OnArrowClick = _Event("TToolButton_SetOnArrowClick", "TNotifyEvent")
    # ツールバーの中での位置(ツールバーに置かれていなければ -1)。
    Index = _Prop("TToolButton_GetIndex", None, _int)
    # 画像の、ツールバーの Images での位置(-1 なら無し。docs/adr/0030)。
    ImageIndex = _Prop("TToolButton_GetImageIndex", "TToolButton_SetImageIndex", _int)
    # OnClick を呼ぶ(tbsCheck の Down は変えない。Down の切り替えはマウスを離したときに LCL が行う)。
    def Click(self):
        lib.TToolButton_Click(self._current())
    # OnArrowClick を呼ぶ(DropdownMenu は表示しない)。
    def ArrowClick(self):
        lib.TToolButton_ArrowClick(self._current())
    # ボタンのクライアント座標 X, Y が矢印の部分にあるか。
    def PointInArrow(self, X, Y):
        _r = lib.TToolButton_PointInArrow(self._current(), int(X), int(Y))
        return _r != 0


class TCoolBand(TPersistent, _ItemMixin):
    """クールバーのバンド(LCL の TCoolBand。TCollectionItem)。同じバンドには常に同じポインタが返る。
    ウィンドウを持つコントロールの Parent をクールバーにすると LCL がバンドを自動で追加し、コントロールを外すと削除する。
    ラッパーは、バンドが破棄されたとき(どの経路でも)に delete される。"""
    Text = _Prop("TCoolBand_GetText", "TCoolBand_SetText", _str)
    Width = _Prop("TCoolBand_GetWidth", "TCoolBand_SetWidth", _int)
    MinWidth = _Prop("TCoolBand_GetMinWidth", "TCoolBand_SetMinWidth", _int)
    MinHeight = _Prop("TCoolBand_GetMinHeight", "TCoolBand_SetMinHeight", _int)
    # true なら、このバンドから新しい行を始める(既定は true)。
    Break = _Prop("TCoolBand_GetBreak", "TCoolBand_SetBreak", _bool)
    Visible = _Prop("TCoolBand_GetVisible", "TCoolBand_SetVisible", _bool)
    FixedSize = _Prop("TCoolBand_GetFixedSize", "TCoolBand_SetFixedSize", _bool)
    FixedBackground = _Prop("TCoolBand_GetFixedBackground", "TCoolBand_SetFixedBackground", _bool)
    HorizontalOnly = _Prop("TCoolBand_GetHorizontalOnly", "TCoolBand_SetHorizontalOnly", _bool)
    Color = _Prop("TCoolBand_GetColor", "TCoolBand_SetColor", _int)
    ParentColor = _Prop("TCoolBand_GetParentColor", "TCoolBand_SetParentColor", _bool)
    # 並び順。書き換えるとバンドが移動する。
    Index = _Prop("TCoolBand_GetIndex", "TCoolBand_SetIndex", _int)
    # バンドに置くコントロール。設定するとそのコントロールの Parent がクールバーになり、Align は alNone になる。
    Control = _Prop("TCoolBand_GetControl", "TCoolBand_SetControl", _comp("TControl"))
    # クールバーのクライアント座標での位置(配置の計算の後で決まる)。
    Left = _Prop("TCoolBand_GetLeft", None, _int)
    Top = _Prop("TCoolBand_GetTop", None, _int)
    Right = _Prop("TCoolBand_GetRight", None, _int)
    Height = _Prop("TCoolBand_GetHeight", None, _int)
    # 画像の、クールバーの Images での位置(-1 なら無し。docs/adr/0030)。
    ImageIndex = _Prop("TCoolBand_GetImageIndex", "TCoolBand_SetImageIndex", _int)
    # 背景の画像。バンドが所有する TBitmap のビューで、代入は内容のコピー。
    Bitmap = _Prop("TCoolBand_GetBitmap", "TCoolBand_SetBitmap", _view("TBitmap"))
    # 幅を、置いているコントロールに合わせる。
    def AutosizeWidth(self):
        lib.TCoolBand_AutosizeWidth(self._current())


class TCoolBands(TPersistent):
    """バンドの一覧(LCL の TCoolBands。TCollection)。クールバーの値メンバとして持つ非所有のビュー。"""
    Count = _Prop("TCoolBands_GetCount", None, _int)
    # CoolBar1->Bands->Items[i]。
    Items = _Indexed("TCoolBands_GetItem", None, _item("TCoolBand"))
    # 空のバンドを末尾に追加して返す(Text・Control 等はその後で設定する)。
    def Add(self):
        _r = lib.TCoolBands_Add(self._current())
        return _to_item("TCoolBand", _r)
    # バンドを削除する(バンドのラッパーも delete される。置いていたコントロールは破棄されない)。
    def Delete(self, Index):
        lib.TCoolBands_Delete(self._current(), int(Index))
    def Clear(self):
        lib.TCoolBands_Clear(self._current())
    def BeginUpdate(self):
        lib.TCoolBands_BeginUpdate(self._current())
    def EndUpdate(self):
        lib.TCoolBands_EndUpdate(self._current())
    # Control がそのコントロールのバンド(無ければ nullptr・-1)。
    def FindBand(self, AControl):
        _r = lib.TCoolBands_FindBand(self._current(), _h(AControl))
        return _to_item("TCoolBand", _r)
    def FindBandIndex(self, AControl):
        _r = lib.TCoolBands_FindBandIndex(self._current(), _h(AControl))
        return _r


class TCustomCoolBar(TToolWindow):
    """以下のメンバは LCL の TCustomCoolBar の public(TCoolBar が published にしている)。
    Align は TControl のものを使う(alLeft/alRight にすると Vertical も true になる)。既定は alTop。"""
    Bands = _Prop("TCustomCoolBar_GetBands", None, _obj("TCoolBands"))
    # true なら、ドラッグでバンドの幅を変えられない。
    FixedSize = _Prop("TCustomCoolBar_GetFixedSize", "TCustomCoolBar_SetFixedSize", _bool)
    # true なら、ドラッグでバンドを並べ替えられない。
    FixedOrder = _Prop("TCustomCoolBar_GetFixedOrder", "TCustomCoolBar_SetFixedOrder", _bool)
    GrabStyle = _Prop("TCustomCoolBar_GetGrabStyle", "TCustomCoolBar_SetGrabStyle", _enum("TGrabStyle"))
    GrabWidth = _Prop("TCustomCoolBar_GetGrabWidth", "TCustomCoolBar_SetGrabWidth", _int)
    HorizontalSpacing = _Prop("TCustomCoolBar_GetHorizontalSpacing", "TCustomCoolBar_SetHorizontalSpacing", _int)
    VerticalSpacing = _Prop("TCustomCoolBar_GetVerticalSpacing", "TCustomCoolBar_SetVerticalSpacing", _int)
    # true なら、バンドの Text を表示する(既定は true)。
    ShowText = _Prop("TCustomCoolBar_GetShowText", "TCustomCoolBar_SetShowText", _bool)
    Themed = _Prop("TCustomCoolBar_GetThemed", "TCustomCoolBar_SetThemed", _bool)
    Vertical = _Prop("TCustomCoolBar_GetVertical", "TCustomCoolBar_SetVertical", _bool)
    # ドラッグでバンドを動かす・幅を変えて、マウスを離したとき。
    OnChange = _Event("TCustomCoolBar_SetOnChange", "TNotifyEvent")
    # バンドの画像リスト(docs/adr/0030)。各バンドの画像は TCoolBand::ImageIndex。
    Images = _Prop("TCustomCoolBar_GetImages", "TCustomCoolBar_SetImages", _comp("TCustomImageList"))
    # 背景の画像。クールバーが所有する TBitmap のビューで、代入は内容のコピー。
    Bitmap = _Prop("TCustomCoolBar_GetBitmap", "TCustomCoolBar_SetBitmap", _view("TBitmap"))
    # すべてのバンドの幅を、置いているコントロールに合わせる。
    def AutosizeBands(self):
        lib.TCustomCoolBar_AutosizeBands(self._current())
    # クライアント座標 X, Y にあるバンドの表示上の位置(ABand。無ければ負)と、つまみの上か(AGrabber)。
    def MouseToBandPos(self, X, Y, ABand, AGrabber):
        _out_ABand = ctypes.c_int()
        _out_AGrabber = ctypes.c_int()
        lib.TCustomCoolBar_MouseToBandPos(self._current(), int(X), int(Y), ctypes.byref(_out_ABand), ctypes.byref(_out_AGrabber))
        ABand.value = _out_ABand.value
        AGrabber.value = _out_AGrabber.value != 0


class TCoolBar(TCustomCoolBar):
    """並べ替え・幅の変更ができるバンドに、コントロールを置くバー。"""
    def __init__(self, AOwner):
        self._attach(lib.TCoolBar_Create(_h(AOwner)))


class TCustomTimer(TComponent):
    Interval = _Prop("TCustomTimer_GetInterval", "TCustomTimer_SetInterval", _int)
    Enabled = _Prop("TCustomTimer_GetEnabled", "TCustomTimer_SetEnabled", _bool)
    OnTimer = _Event("TCustomTimer_SetOnTimer", "TNotifyEvent")


class TTimer(TCustomTimer):
    def __init__(self, AOwner):
        self._attach(lib.TTimer_Create(_h(AOwner)))


class TBasicAction(TComponent):
    """操作(LCL の TBasicAction)。コントロール・メニュー項目の Action に割り当てると、選んだときに OnExecute が呼ばれる。"""
    # OnExecute を呼ぶ(ActionList の OnExecute で Handled にされたときは呼ばない)。呼んだら true。
    def Execute(self):
        _r = lib.TBasicAction_Execute(self._current())
        return _r != 0
    # OnUpdate を呼ぶ(アイドルのときに LCL が呼ぶものを、すぐに呼ぶ)。
    def Update(self):
        _r = lib.TBasicAction_Update(self._current())
        return _r != 0
    # Execute を起こしたコントロール・メニュー項目(プログラムから Execute したときは nullptr)。
    ActionComponent = _Prop("TBasicAction_GetActionComponent", None, _comp("TComponent"))
    # 実行するとき。Sender は Action(起こしたものは ActionComponent)。
    OnExecute = _Event("TBasicAction_SetOnExecute", "TNotifyEvent")
    # アイドルのときに LCL が呼ぶ。Enabled・Checked 等をここで今の状態に合わせる(割り当てたコントロールにも写る)。
    # 呼ばれるのは、表示中のフォームのコントロール・メインメニューの項目に割り当てた Action だけ(VCL と同じ)。
    OnUpdate = _Event("TBasicAction_SetOnUpdate", "TNotifyEvent")


class TContainedAction(TBasicAction):
    """ActionList に入る Action(LCL の TContainedAction)。"""
    # 属する ActionList。代入すると一覧の末尾に入る(nullptr なら一覧から外す)。
    ActionList = _Prop("TContainedAction_GetActionList", "TContainedAction_SetActionList", _comp("TCustomActionList"))
    # 分類(ActionList の中でまとめて扱うための名前。動作には影響しない)。
    Category = _Prop("TContainedAction_GetCategory", "TContainedAction_SetCategory", _str)
    # ActionList の中の位置。書き換えると移動する。
    Index = _Prop("TContainedAction_GetIndex", "TContainedAction_SetIndex", _int)


class TCustomAction(TContainedAction):
    """表示の状態を持つ Action(LCL の TCustomAction)。値を変えると、割り当てたコントロール・メニュー項目にも写る。"""
    Caption = _Prop("TCustomAction_GetCaption", "TCustomAction_SetCaption", _str)
    Hint = _Prop("TCustomAction_GetHint", "TCustomAction_SetHint", _str)
    Checked = _Prop("TCustomAction_GetChecked", "TCustomAction_SetChecked", _bool)
    # true にすると、実行するたびに Checked が反転する。
    AutoCheck = _Prop("TCustomAction_GetAutoCheck", "TCustomAction_SetAutoCheck", _bool)
    # 0 でなければ、同じ GroupIndex の Action のうち 1 つだけが Checked になる。
    GroupIndex = _Prop("TCustomAction_GetGroupIndex", "TCustomAction_SetGroupIndex", _int)
    Enabled = _Prop("TCustomAction_GetEnabled", "TCustomAction_SetEnabled", _bool)
    Visible = _Prop("TCustomAction_GetVisible", "TCustomAction_SetVisible", _bool)
    # 割り当てたメニュー項目・ボタンの画像の、ActionList の Images での位置(-1 なら無し)。
    ImageIndex = _Prop("TCustomAction_GetImageIndex", "TCustomAction_SetImageIndex", _int)
    # フォームにフォーカスがあるときにこのキーを押すと実行する(割り当てたメニュー項目にも表示される)。
    ShortCut = _Prop("TCustomAction_GetShortCut", "TCustomAction_SetShortCut", _int)
    # true(既定)なら、OnExecute が無いとき Enabled を false にする。
    DisableIfNoHandler = _Prop("TCustomAction_GetDisableIfNoHandler", "TCustomAction_SetDisableIfNoHandler", _bool)


class TAction(TCustomAction):
    def __init__(self, AOwner):
        self._attach(lib.TAction_Create(_h(AOwner)))


class TCustomActionList(TComponent):
    """Action の一覧(LCL の TCustomActionList)。"""
    Actions = _Indexed("TCustomActionList_GetActions", None, _comp("TContainedAction"))
    ActionCount = _Prop("TCustomActionList_GetActionCount", None, _int)
    # Action の ImageIndex が指す画像リスト。割り当てたメニュー項目・ボタンにも使われる。
    Images = _Prop("TCustomActionList_GetImages", "TCustomActionList_SetImages", _comp("TCustomImageList"))
    State = _Prop("TCustomActionList_GetState", "TCustomActionList_SetState", _enum("TActionListState"))
    # どの Action を実行するときにも、Action の OnExecute の前に呼ばれる。
    OnExecute = _Event("TCustomActionList_SetOnExecute", "TActionEvent")
    # どの Action を更新するときにも、Action の OnUpdate の前に呼ばれる。
    OnUpdate = _Event("TCustomActionList_SetOnUpdate", "TActionEvent")


class TActionList(TCustomActionList):
    def __init__(self, AOwner):
        self._attach(lib.TActionList_Create(_h(AOwner)))


class TCommonDialog(TComponent):
    """ダイアログの共通の基底(LCL の TCommonDialog)。VCL と同じく、プロパティを設定して Execute() を呼び、結果を bool で受け取る
    (if (OpenDialog1->Execute()) Memo1->Lines->LoadFromFile(OpenDialog1->FileName);)。
    TComponent なので、他のコンポーネントと同じく new で生成し、Owner に任せるか Free() で破棄する。1 つを何度でも Execute できる。"""
    # ダイアログのタイトル(空なら OS・LCL の既定)。Win32 の TFontDialog では使われない。
    Title = _Prop("TCommonDialog_GetTitle", "TCommonDialog_SetTitle", _str)
    # ダイアログが表示されたとき・閉じたとき。
    OnShow = _Event("TCommonDialog_SetOnShow", "TNotifyEvent")
    OnClose = _Event("TCommonDialog_SetOnClose", "TNotifyEvent")
    # OK で閉じようとしたとき(CanClose を false にすると閉じない)。Win32 ではファイルのダイアログでだけ呼ばれる。
    OnCanClose = _Event("TCommonDialog_SetOnCanClose", "TCloseQueryEvent")
    # ダイアログを表示する。閉じるまで戻らず、OK で閉じたら true、キャンセルなら false を返す
    # (TFindDialog・TReplaceDialog はモードレスで、表示してすぐ true を返す)。
    def Execute(self):
        _r = lib.TCommonDialog_Execute(self._current())
        return _r != 0


class TFileDialog(TCommonDialog):
    """ファイルを選ぶダイアログの共通の基底(LCL の TFileDialog)。"""
    # 選択したファイルのフルパス(Execute の前に設定すると、初期のファイル名になる)。
    FileName = _Prop("TFileDialog_GetFileName", "TFileDialog_SetFileName", _str)
    # "テキスト|*.txt|すべて|*.*" のように、表示名とマスクを | で区切って並べる(1 つのマスクに複数のパターンは ; で区切る)。
    Filter = _Prop("TFileDialog_GetFilter", "TFileDialog_SetFilter", _str)
    # 選択されているフィルターの位置(1 始まり)。
    FilterIndex = _Prop("TFileDialog_GetFilterIndex", "TFileDialog_SetFilterIndex", _int)
    InitialDir = _Prop("TFileDialog_GetInitialDir", "TFileDialog_SetInitialDir", _str)
    # ファイル名に拡張子が無いときに補う拡張子。LCL は先頭に . を補う("txt" を設定すると ".txt" が返る。VCL は補わない)。
    DefaultExt = _Prop("TFileDialog_GetDefaultExt", "TFileDialog_SetDefaultExt", _str)
    # 選択したファイルの一覧(TStrings。ofAllowMultiSelect のとき複数)。
    Files = _Prop("TFileDialog_GetFiles", None, _view("TStrings"))


class TOpenDialog(TFileDialog):
    """ファイルを開くダイアログ。"""
    Options = _Prop("TOpenDialog_GetOptions", "TOpenDialog_SetOptions", _enum("TOpenOptions"))
    def __init__(self, AOwner):
        self._attach(lib.TOpenDialog_Create(_h(AOwner)))


class TSaveDialog(TOpenDialog):
    """ファイルを保存するダイアログ。"""
    def __init__(self, AOwner):
        self._attach(lib.TSaveDialog_Create(_h(AOwner)))


class TSelectDirectoryDialog(TOpenDialog):
    """ディレクトリを選ぶダイアログ(VCL には無く、LCL にある)。選んだディレクトリは FileName で受け取る。"""
    def __init__(self, AOwner):
        self._attach(lib.TSelectDirectoryDialog_Create(_h(AOwner)))


class TColorDialog(TCommonDialog):
    """色を選ぶダイアログ。"""
    # 選択した色(Execute の前に設定すると、初期の色になる)。
    Color = _Prop("TColorDialog_GetColor", "TColorDialog_SetColor", _int)
    # 作成した色("ColorA=FFFFFF" のような 名前=値 の行。値は $BBGGRR の 16 進。TStrings)。LCL の既定は ColorA〜ColorT の 20 色。
    CustomColors = _Prop("TColorDialog_GetCustomColors", None, _view("TStrings"))
    Options = _Prop("TColorDialog_GetOptions", "TColorDialog_SetOptions", _enum("TColorDialogOptions"))
    def __init__(self, AOwner):
        self._attach(lib.TColorDialog_Create(_h(AOwner)))


class TFontDialog(TCommonDialog):
    """フォントを選ぶダイアログ。"""
    # 選択したフォント。ダイアログが所有する TFont のビューで、ダイアログと寿命が一致する。
    # 代入は内容のコピー(nullptr なら何もしない)。FontDialog1->Font = Memo1->Font; で初期のフォントにする。
    Font = _Prop("TFontDialog_GetFont", "TFontDialog_SetFont", _obj("TFont"))
    # 選べる大きさの範囲(Options に fdLimitSize があるときだけ使われる)。
    MinFontSize = _Prop("TFontDialog_GetMinFontSize", "TFontDialog_SetMinFontSize", _int)
    MaxFontSize = _Prop("TFontDialog_GetMaxFontSize", "TFontDialog_SetMaxFontSize", _int)
    Options = _Prop("TFontDialog_GetOptions", "TFontDialog_SetOptions", _enum("TFontDialogOptions"))
    def __init__(self, AOwner):
        self._attach(lib.TFontDialog_Create(_h(AOwner)))


class TFindDialog(TCommonDialog):
    """検索のダイアログ。VCL と同じくモードレスで、Execute() は表示してすぐ戻り、利用者が「次を検索」を押すたびに OnFind が呼ばれる
    (検索そのものは OnFind で FindText・Options を見て行う)。閉じるのは利用者か CloseDialog()。"""
    FindText = _Prop("TFindDialog_GetFindText", "TFindDialog_SetFindText", _str)
    Options = _Prop("TFindDialog_GetOptions", "TFindDialog_SetOptions", _enum("TFindOptions"))
    # ダイアログの位置(画面の座標)。
    Left = _Prop("TFindDialog_GetLeft", "TFindDialog_SetLeft", _int)
    Top = _Prop("TFindDialog_GetTop", "TFindDialog_SetTop", _int)
    OnFind = _Event("TFindDialog_SetOnFind", "TNotifyEvent")
    def __init__(self, AOwner):
        self._attach(lib.TFindDialog_Create(_h(AOwner)))
    # 表示中のダイアログを閉じる。
    def CloseDialog(self):
        lib.TFindDialog_CloseDialog(self._current())
    # LCL では TFindDialog の protected。TReplaceDialog が公開する。
    _ReplaceText = _Prop("TFindDialog_GetReplaceText", "TFindDialog_SetReplaceText", _str)
    _OnReplace = _Event("TFindDialog_SetOnReplace", "TNotifyEvent")


class TReplaceDialog(TFindDialog):
    """置換のダイアログ。「置換」「すべて置換」が押されると OnReplace が呼ばれる(どちらかは Options の frReplace・frReplaceAll で分かる)。"""
    ReplaceText = TFindDialog._ReplaceText
    OnReplace = TFindDialog._OnReplace
    def __init__(self, AOwner):
        self._attach(lib.TReplaceDialog_Create(_h(AOwner)))


# ---------------- イベントの型(Sender 以外の引数) ----------------

_event_types.update({
    "TNotifyEvent": (),  # (Sender)
    "TCloseEvent": (_a_ref_enum("TCloseAction"), ),  # (Sender, Action)
    "TCloseQueryEvent": (_a_ref_bool, ),  # (Sender, CanClose)
    "TKeyEvent": (_a_ref_int, _a_enum("TShiftState"), ),  # (Sender, Key, Shift)
    "TKeyPressEvent": (_a_ref_char, ),  # (Sender, Key)
    "TMouseEvent": (_a_enum("TMouseButton"), _a_enum("TShiftState"), _a_int, _a_int, ),  # (Sender, Button, Shift, X, Y)
    "TMouseMoveEvent": (_a_enum("TShiftState"), _a_int, _a_int, ),  # (Sender, Shift, X, Y)
    "TMouseWheelEvent": (_a_enum("TShiftState"), _a_int, _a_int, _a_int, _a_ref_bool, ),  # (Sender, Shift, WheelDelta, X, Y, Handled)
    "TDrawItemEvent": (_a_int, _a_rect, _a_enum("TOwnerDrawState"), ),  # (Sender, Index, ARect, State)
    "TMeasureItemEvent": (_a_int, _a_ref_int, ),  # (Sender, Index, AHeight)
    "TMenuDrawItemEvent": (_a_obj("TCanvas"), _a_rect, _a_enum("TOwnerDrawState"), ),  # (Sender, ACanvas, ARect, AState)
    "TMenuMeasureItemEvent": (_a_obj("TCanvas"), _a_ref_int, _a_ref_int, ),  # (Sender, ACanvas, AWidth, AHeight)
    "TSelectionChangeEvent": (_a_bool, ),  # (Sender, User)
    "TIdleEvent": (_a_ref_bool, ),  # (Sender, Done)
    "TExceptionEvent": (_a_exception, ),  # (Sender, E)
    "TDropFilesEvent": (_a_strings, ),  # (Sender, FileNames)
    "TScrollEvent": (_a_enum("TScrollCode"), _a_ref_int, ),  # (Sender, ScrollCode, ScrollPos)
    "TCheckGroupClicked": (_a_int, ),  # (Sender, Index)
    "TTabChangingEvent": (_a_ref_bool, ),  # (Sender, AllowChange)
    "TTVChangedEvent": (_a_item("TTreeNode"), ),  # (Sender, Node)
    "TTVChangingEvent": (_a_item("TTreeNode"), _a_ref_bool, ),  # (Sender, Node, AllowChange)
    "TTVExpandingEvent": (_a_item("TTreeNode"), _a_ref_bool, ),  # (Sender, Node, AllowExpansion)
    "TTVCollapsingEvent": (_a_item("TTreeNode"), _a_ref_bool, ),  # (Sender, Node, AllowCollapse)
    "TLVDeletedEvent": (_a_item("TListItem"), ),  # (Sender, Item)
    "TLVSelectItemEvent": (_a_item("TListItem"), _a_bool, ),  # (Sender, Item, Selected)
    "TLVChangeEvent": (_a_item("TListItem"), _a_enum("TItemChange"), ),  # (Sender, Item, Change)
    "TLVColumnClickEvent": (_a_item("TListColumn"), ),  # (Sender, Column)
    "TDrawPanelEvent": (_a_item("TStatusPanel"), _a_rect, ),  # (Sender, Panel, Rect)
    "TOnDrawCell": (_a_int, _a_int, _a_rect, _a_enum("TGridDrawState"), ),  # (Sender, ACol, ARow, ARect, AState)
    "TOnSelectCellEvent": (_a_int, _a_int, _a_ref_bool, ),  # (Sender, ACol, ARow, CanSelect)
    "TOnSelectEvent": (_a_int, _a_int, ),  # (Sender, ACol, ARow)
    "THdrEvent": (_a_bool, _a_int, ),  # (Sender, IsColumn, Index)
    "TCustomSectionNotifyEvent": (_a_item("THeaderSection"), ),  # (Sender, Section)
    "TCustomSectionTrackEvent": (_a_item("THeaderSection"), _a_int, _a_enum("TSectionTrackState"), ),  # (Sender, Section, Width, State)
    "TSectionDragEvent": (_a_item("THeaderSection"), _a_item("THeaderSection"), _a_ref_bool, ),  # (Sender, FromSection, ToSection, AllowDrag)
    "TActionEvent": (_a_comp("TBasicAction"), _a_ref_bool, ),  # (Sender, Action, Handled)
})

_register(globals())

# C++Builder と同じく、アプリケーションに 1 つのグローバル変数として公開する。
Application = TApplication._global()
# 画面(LCL の Screen)。Application と同じく 1 つだけ(docs/adr/0047)。
Screen = TScreen._wrap_existing(lib.GetScreen())

__all__ = [
    "BethError", "Ref", "TRect", "TPoint", "TObject", "TPersistent", "TComponent", "ShortCut", "TextToShortCut",
    "ShortCutToText", "Application", "Screen", "Clipboard", "CF_Text", "CF_Bitmap", "CF_Picture", "ShowMessage",
    "MessageDlg", "InputBox", "PasswordBox", "InputQuery", "MB_OK", "MB_OKCANCEL", "MB_ABORTRETRYIGNORE",
    "MB_YESNOCANCEL", "MB_YESNO", "MB_RETRYCANCEL", "MB_ICONERROR", "MB_ICONQUESTION", "MB_ICONWARNING",
    "MB_ICONINFORMATION", "MB_DEFBUTTON1", "MB_DEFBUTTON2", "MB_DEFBUTTON3", "IDOK", "IDCANCEL", "IDABORT",
    "IDRETRY", "IDIGNORE", "IDYES", "IDNO", "TCloseAction", "caNone", "caHide", "caFree", "caMinimize",
    "TMouseButton", "mbLeft", "mbRight", "mbMiddle", "mbExtra1", "mbExtra2", "TDuplicates", "dupIgnore",
    "dupAccept", "dupError", "TPenStyle", "psSolid", "psDash", "psDot", "psDashDot", "psDashDotDot",
    "psInsideFrame", "psPattern", "psClear", "TPenMode", "pmBlack", "pmWhite", "pmNop", "pmNot", "pmCopy",
    "pmNotCopy", "pmMergePenNot", "pmMaskPenNot", "pmMergeNotPen", "pmMaskNotPen", "pmMerge", "pmNotMerge",
    "pmMask", "pmNotMask", "pmXor", "pmNotXor", "TBrushStyle", "bsSolid", "bsClear", "bsHorizontal", "bsVertical",
    "bsFDiagonal", "bsBDiagonal", "bsCross", "bsDiagCross", "bsImage", "bsPattern", "TFontQuality", "fqDefault",
    "fqDraft", "fqProof", "fqNonAntialiased", "fqAntialiased", "fqCleartype", "fqCleartypeNatural", "TPixelFormat",
    "pfDevice", "pf1bit", "pf4bit", "pf8bit", "pf15bit", "pf16bit", "pf24bit", "pf32bit", "pfCustom",
    "TTransparentMode", "tmAuto", "tmFixed", "TDrawingStyle", "dsFocus", "dsSelected", "dsNormal", "dsTransparent",
    "TListBoxStyle", "lbStandard", "lbOwnerDrawFixed", "lbOwnerDrawVariable", "lbVirtual", "TAlign", "alNone",
    "alTop", "alBottom", "alLeft", "alRight", "alClient", "alCustom", "TAnchorKind", "akTop", "akLeft", "akRight",
    "akBottom", "TFormBorderStyle", "bsNone", "bsSingle", "bsSizeable", "bsDialog", "bsToolWindow",
    "bsSizeToolWin", "TPanelBevel", "bvNone", "bvLowered", "bvRaised", "bvSpace", "TVerticalAlignment",
    "taAlignTop", "taAlignBottom", "taVerticalCenter", "TScrollStyle", "ssNone", "ssHorizontal", "ssVertical",
    "ssBoth", "ssAutoHorizontal", "ssAutoVertical", "ssAutoBoth", "TScrollBarKind", "sbHorizontal", "sbVertical",
    "TComboBoxStyle", "csDropDown", "csSimple", "csDropDownList", "csOwnerDrawFixed", "csOwnerDrawVariable",
    "csOwnerDrawEditableFixed", "csOwnerDrawEditableVariable", "TCheckBoxState", "cbUnchecked", "cbChecked",
    "cbGrayed", "TAlignment", "taLeftJustify", "taRightJustify", "taCenter", "TTextLayout", "tlTop", "tlCenter",
    "tlBottom", "TEchoMode", "emNormal", "emNone", "emPassword", "TEditCharCase", "ecNormal", "ecUpperCase",
    "ecLowerCase", "TTrackBarOrientation", "trHorizontal", "trVertical", "TTickMark", "tmBottomRight", "tmTopLeft",
    "tmBoth", "TTickStyle", "tsNone", "tsAuto", "tsManual", "TProgressBarOrientation", "pbHorizontal",
    "pbVertical", "pbRightToLeft", "pbTopDown", "TProgressBarStyle", "pbstNormal", "pbstMarquee", "TUDOrientation",
    "udHorizontal", "udVertical", "TUDAlignButton", "udLeft", "udRight", "udTop", "udBottom", "TColumnLayout",
    "clHorizontalThenVertical", "clVerticalThenHorizontal", "TScrollCode", "scLineUp", "scLineDown", "scPageUp",
    "scPageDown", "scPosition", "scTrack", "scTop", "scBottom", "scEndScroll", "TPosition", "poDesigned",
    "poDefault", "poDefaultPosOnly", "poDefaultSizeOnly", "poScreenCenter", "poDesktopCenter", "poMainFormCenter",
    "poOwnerFormCenter", "poWorkAreaCenter", "TWindowState", "wsNormal", "wsMinimized", "wsMaximized",
    "wsFullScreen", "TBorderIcon", "biSystemMenu", "biMinimize", "biMaximize", "biHelp", "TFormStyle", "fsNormal",
    "fsMDIChild", "fsMDIForm", "fsStayOnTop", "fsSplash", "fsSystemStayOnTop", "TMsgDlgType", "mtWarning",
    "mtError", "mtInformation", "mtConfirmation", "mtCustom", "TMsgDlgBtn", "mbYes", "mbNo", "mbOK", "mbCancel",
    "mbAbort", "mbRetry", "mbIgnore", "mbAll", "mbNoToAll", "mbYesToAll", "mbHelp", "mbClose", "TBevelShape",
    "bsBox", "bsFrame", "bsTopLine", "bsBottomLine", "bsLeftLine", "bsRightLine", "bsSpacer", "TBevelStyle",
    "bsLowered", "bsRaised", "TBitBtnKind", "bkCustom", "bkOK", "bkCancel", "bkHelp", "bkYes", "bkNo", "bkClose",
    "bkAbort", "bkRetry", "bkIgnore", "bkAll", "bkNoToAll", "bkYesToAll", "TButtonLayout", "blGlyphLeft",
    "blGlyphRight", "blGlyphTop", "blGlyphBottom", "TLabelPosition", "lpAbove", "lpBelow", "lpLeft", "lpRight",
    "TTabPosition", "tpTop", "tpBottom", "tpLeft", "tpRight", "TNodeAttachMode", "naAdd", "naAddFirst",
    "naAddChild", "naAddChildFirst", "naInsert", "naInsertBehind", "TViewStyle", "vsIcon", "vsSmallIcon", "vsList",
    "vsReport", "TSortType", "stNone", "stData", "stText", "stBoth", "TSortDirection", "sdAscending",
    "sdDescending", "TItemChange", "ctText", "ctImage", "ctState", "TResizeStyle", "rsLine", "rsNone", "rsPattern",
    "rsUpdate", "TStaticBorderStyle", "sbsNone", "sbsSingle", "sbsSunken", "TStatusPanelStyle", "psText",
    "psOwnerDraw", "TStatusPanelBevel", "pbNone", "pbLowered", "pbRaised", "TShapeType", "stRectangle", "stSquare",
    "stRoundRect", "stRoundSquare", "stEllipse", "stCircle", "stSquaredDiamond", "stDiamond", "stTriangle",
    "stTriangleLeft", "stTriangleRight", "stTriangleDown", "stStar", "stStarDown", "stPolygon",
    "TSectionTrackState", "tsTrackBegin", "tsTrackMove", "tsTrackEnd", "TEdgeStyle", "esNone", "esRaised",
    "esLowered", "TToolButtonStyle", "tbsButton", "tbsCheck", "tbsDropDown", "tbsSeparator", "tbsDivider",
    "tbsButtonDrop", "TGrabStyle", "gsSimple", "gsDouble", "gsHorLines", "gsVerLines", "gsGripper", "gsButton",
    "TActionListState", "asNormal", "asSuspended", "asSuspendedEnabled", "TBorderStyle", "TShiftState", "ssShift",
    "ssAlt", "ssCtrl", "ssLeft", "ssRight", "ssMiddle", "ssDouble", "ssMeta", "ssSuper", "ssHyper", "ssAltGr",
    "ssCaps", "ssNum", "ssScroll", "ssTriple", "ssQuad", "ssExtra1", "ssExtra2", "TFontStyles", "fsBold",
    "fsItalic", "fsUnderline", "fsStrikeOut", "TOwnerDrawState", "odSelected", "odGrayed", "odDisabled",
    "odChecked", "odFocused", "odDefault", "odHotLight", "odInactive", "odNoAccel", "odNoFocusRect", "odReserved1",
    "odReserved2", "odComboBoxEdit", "odBackgroundPainted", "TGridOptions", "goFixedVertLine", "goFixedHorzLine",
    "goVertLine", "goHorzLine", "goRangeSelect", "goDrawFocusSelected", "goRowSizing", "goColSizing",
    "goRowMoving", "goColMoving", "goEditing", "goAutoAddRows", "goTabs", "goRowSelect", "goAlwaysShowEditor",
    "goThumbTracking", "goColSpanning", "goRelaxedRowSelect", "goDblClickAutoSize", "goSmoothScroll",
    "goFixedRowNumbering", "goScrollKeepVisible", "goHeaderHotTracking", "goHeaderPushedLook", "goSelectionActive",
    "goFixedColSizing", "goDontScrollPartCell", "goCellHints", "goTruncCellHints", "goCellEllipsis",
    "goAutoAddRowsSkipContentCheck", "goRowHighlight", "TGridDrawState", "gdSelected", "gdFocused", "gdFixed",
    "gdHot", "gdPushed", "gdRowHighlight", "TEdgeBorders", "ebLeft", "ebTop", "ebRight", "ebBottom",
    "TOpenOptions", "ofReadOnly", "ofOverwritePrompt", "ofHideReadOnly", "ofNoChangeDir", "ofShowHelp",
    "ofNoValidate", "ofAllowMultiSelect", "ofExtensionDifferent", "ofPathMustExist", "ofFileMustExist",
    "ofCreatePrompt", "ofShareAware", "ofNoReadOnlyReturn", "ofNoTestFileCreate", "ofNoNetworkButton",
    "ofNoLongNames", "ofOldStyleDialog", "ofNoDereferenceLinks", "ofNoResolveLinks", "ofEnableIncludeNotify",
    "ofEnableSizing", "ofDontAddToRecent", "ofForceShowHidden", "ofViewDetail", "ofAutoPreview",
    "TColorDialogOptions", "cdFullOpen", "cdPreventFullOpen", "cdShowHelp", "cdSolidColor", "cdAnyColor",
    "TFontDialogOptions", "fdAnsiOnly", "fdTrueTypeOnly", "fdEffects", "fdFixedPitchOnly", "fdForceFontExist",
    "fdNoFaceSel", "fdNoOEMFonts", "fdNoSimulations", "fdNoSizeSel", "fdNoStyleSel", "fdNoVectorFonts",
    "fdShowHelp", "fdWysiwyg", "fdLimitSize", "fdScalableOnly", "fdApplyButton", "TFindOptions", "frDown",
    "frFindNext", "frHideMatchCase", "frHideWholeWord", "frHideUpDown", "frMatchCase", "frDisableMatchCase",
    "frDisableUpDown", "frDisableWholeWord", "frReplace", "frReplaceAll", "frWholeWord", "frShowHelp",
    "frEntireScope", "frHideEntireScope", "frPromptOnReplace", "frHidePromptOnReplace", "frButtonsAtBottom",
    "TColor", "clBlack", "clMaroon", "clGreen", "clOlive", "clNavy", "clPurple", "clTeal", "clGray", "clSilver",
    "clRed", "clLime", "clYellow", "clBlue", "clFuchsia", "clAqua", "clWhite", "clMoneyGreen", "clSkyBlue",
    "clCream", "clMedGray", "clScrollBar", "clBackground", "clActiveCaption", "clInactiveCaption", "clMenu",
    "clWindow", "clWindowFrame", "clMenuText", "clWindowText", "clCaptionText", "clActiveBorder",
    "clInactiveBorder", "clAppWorkspace", "clHighlight", "clHighlightText", "clBtnFace", "clBtnShadow",
    "clGrayText", "clBtnText", "clInactiveCaptionText", "clBtnHighlight", "cl3DDkShadow", "cl3DLight",
    "clInfoText", "clInfoBk", "clHotLight", "clGradientActiveCaption", "clGradientInactiveCaption",
    "clMenuHighlight", "clMenuBar", "clForm", "clNone", "clDefault", "TShortCut", "scShift", "scCtrl", "scAlt",
    "TCursor", "crDefault", "crNone", "crArrow", "crCross", "crIBeam", "crSizeNESW", "crSizeNS", "crSizeNWSE",
    "crSizeWE", "crUpArrow", "crHourGlass", "crDrag", "crNoDrop", "crHSplit", "crVSplit", "crMultiDrag",
    "crSQLWait", "crNo", "crAppStart", "crHelp", "crHandPoint", "crSizeAll", "crSize", "crSizeNW", "crSizeN",
    "crSizeNE", "crSizeW", "crSizeE", "crSizeSW", "crSizeS", "crSizeSE", "TModalResult", "mrNone", "mrOk",
    "mrCancel", "mrAbort", "mrRetry", "mrIgnore", "mrYes", "mrNo", "mrAll", "mrNoToAll", "mrYesToAll", "mrClose",
    "TClipboardFormat", "mbYesNo", "mbYesNoCancel", "mbOKCancel", "mbAbortRetryIgnore", "TStrings", "TStringList",
    "TPen", "TBrush", "TFont", "TCanvas", "TGraphic", "TRasterImage", "TCustomBitmap", "TBitmap",
    "TPortableNetworkGraphic", "TJPEGImage", "TIcon", "TPicture", "TCustomImageList", "TImageList", "TMenuItem",
    "TMenu", "TMainMenu", "TPopupMenu", "TSizeConstraints", "TControlBorderSpacing", "TControlScrollBar",
    "TControl", "TWinControl", "TCustomScrollBar", "TScrollBar", "TCustomTrackBar", "TTrackBar",
    "TCustomProgressBar", "TProgressBar", "TGraphicControl", "TCustomControl", "TUpDown", "TScrollingWinControl",
    "TScrollBox", "TCustomForm", "TForm", "TApplication", "TScreen", "TClipboard", "TCustomPanel", "TPanel",
    "TCustomGroupBox", "TGroupBox", "TCustomRadioGroup", "TRadioGroup", "TCustomCheckGroup", "TCheckGroup",
    "TCustomLabel", "TLabel", "TBoundLabel", "TBevel", "TButtonControl", "TCustomButton", "TButton",
    "TCustomBitBtn", "TBitBtn", "TCustomCheckBox", "TCheckBox", "TRadioButton", "TToggleBox", "TCustomEdit",
    "TEdit", "TCustomFloatSpinEdit", "TFloatSpinEdit", "TCustomSpinEdit", "TSpinEdit", "TMaskEdit",
    "TCustomLabeledEdit", "TLabeledEdit", "TCustomTabControl", "TTabControl", "TPageControl", "TCustomPage",
    "TTabSheet", "TTreeNode", "TTreeNodes", "TCustomTreeView", "TTreeView", "TListItem", "TListItems",
    "TListColumn", "TListColumns", "TCustomListView", "TListView", "TCustomSplitter", "TSplitter", "TCustomMemo",
    "TMemo", "TCustomComboBox", "TComboBox", "TCustomListBox", "TListBox", "TCustomCheckListBox", "TCheckListBox",
    "TCustomStaticText", "TStaticText", "TStatusPanel", "TStatusPanels", "TStatusBar", "TCustomShape", "TShape",
    "TCustomSpeedButton", "TSpeedButton", "TPaintBox", "TCustomImage", "TImage", "TCustomGrid", "TCustomDrawGrid",
    "TDrawGrid", "TCustomStringGrid", "TStringGrid", "THeaderSection", "THeaderSections", "TCustomHeaderControl",
    "THeaderControl", "TToolWindow", "TToolBar", "TToolButton", "TCoolBand", "TCoolBands", "TCustomCoolBar",
    "TCoolBar", "TCustomTimer", "TTimer", "TBasicAction", "TContainedAction", "TCustomAction", "TAction",
    "TCustomActionList", "TActionList", "TCommonDialog", "TFileDialog", "TOpenDialog", "TSaveDialog",
    "TSelectDirectoryDialog", "TColorDialog", "TFontDialog", "TFindDialog", "TReplaceDialog",
]
