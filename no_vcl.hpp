#ifndef NO_VCL_HPP
#define NO_VCL_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "no_vcl_c.h"

// クラス階層は LCL の継承関係の「部分列」になっている(途中の階層を省くことはあっても、
// LCL に無い継承関係は作らない)。これにより、C++ 上で基底クラスとして扱えるオブジェクトは
// Pascal 側でもその基底クラスとして扱えることが保証される。
// メンバの公開範囲も LCL に合わせ、LCL で protected のメンバを派生クラスが公開している箇所は
// using で公開している。詳細は docs/class-hierarchy.md を参照。

namespace no_vcl
{

// DelphiのTColorに合わせ $00BBGGRR 順のパック整数として表す。
using TColor = std::int32_t;

const TColor clBlack  = 0x000000;
const TColor clWhite  = 0xFFFFFF;
const TColor clRed    = 0x0000FF;
const TColor clGreen  = 0x008000;
const TColor clBlue   = 0xFF0000;
const TColor clYellow = 0x00FFFF;

class TObject
{
public:
    TObject(const TObject&) = delete;
    TObject& operator=(const TObject&) = delete;
    TObject(TObject&&) = delete;
    TObject& operator=(TObject&&) = delete;

    no_vcl_obj_t Handle() const { return handle_; }

protected:
    // 各クラスは、生成した(または取得した)ハンドルを基底クラスの初期化の時点で渡す。
    // これにより、派生クラスのメンバ(Property や TPaintBox::Canvas)を構築する時点で
    // handle_ が有効であることが保証される。
    explicit TObject(no_vcl_obj_t handle) : handle_(handle) {}

    // TObject* 経由の delete でコンポーネントの寿命管理(TComponent 参照)を迂回できないよう protected にする。
    virtual ~TObject() = default;

    no_vcl_obj_t handle_;
};

// 所有者(TObject派生インスタンス)への生ポインタ + 固定のGetter/Setter関数ポインタを持つ
// 軽量プロキシ。std::functionを使わないためヒープ確保がなく、所有者のコピー/ムーブは
// TObjectで禁止しているためownerが指す実体が入れ替わることもない。
template<typename T>
class Property
{
public:
    using Getter = T    (*)(TObject*);
    using Setter = void (*)(TObject*, const T&);

    Property(TObject* owner, Getter getter, Setter setter)
        : owner_(owner)
        , getter_(getter)
        , setter_(setter)
    {}

    // コピー構築は所有者を取り違えるので禁止する。一方、プロパティ同士の代入
    // (Label1->Caption = Edit1->Text; / RadioButton2->OnClick = RadioButton1->OnClick;)は
    // 値のコピーとして扱う(所有者は付け替えない)。
    Property(const Property&) = delete;
    Property& operator=(const Property& other)
    {
        setter_(owner_, other.getter_(other.owner_));
        return *this;
    }

    Property& operator=(const T& value)
    {
        setter_(owner_, value);
        return *this;
    }

    operator T() const
    {
        return getter_(owner_);
    }

    // ポインタ型のプロパティ(Parent 等)で、Button1->Parent->Caption のようにメンバへ直接たどれるようにする。
    // ポインタ以外の型では使うとコンパイルエラーになる。
    T operator->() const
    {
        return getter_(owner_);
    }

private:
    TObject* owner_;
    Getter   getter_;
    Setter   setter_;
};

// 読み取り専用のプロパティ(Application->MainForm / Terminated 等)。代入はコンパイルエラーになる。
template<typename T>
class ReadOnlyProperty
{
public:
    using Getter = T (*)(TObject*);

    ReadOnlyProperty(TObject* owner, Getter getter)
        : owner_(owner)
        , getter_(getter)
    {}

    ReadOnlyProperty(const ReadOnlyProperty&) = delete;
    ReadOnlyProperty& operator=(const ReadOnlyProperty&) = delete;

    operator T() const
    {
        return getter_(owner_);
    }

    T operator->() const
    {
        return getter_(owner_);
    }

private:
    TObject* owner_;
    Getter   getter_;
};

// インデックス付きのプロパティ(VCL の ColWidths[ACol] 等)。Grid->ColWidths[0] = 80; のように添字で読み書きできる。
// 添字は所有者・添字・Getter/Setter を持つ要素のプロキシ(Reference)を返し、読み書きはその時点で所有者に委ねる。
template<typename T>
class IndexedProperty
{
public:
    using Getter = T    (*)(TObject*, int);
    using Setter = void (*)(TObject*, int, const T&);

    // 1 つの要素へのプロキシ。代入は値の書き込み、T への変換は値の読み出しになる
    // (auto で受けるとプロキシのままになるので、値が必要なら型を明示する)。
    class Reference
    {
    public:
        Reference(TObject* owner, int index, Getter getter, Setter setter)
            : owner_(owner), index_(index), getter_(getter), setter_(setter)
        {}

        // 値で返すためにコピー構築はできる(同じ要素を指す)。
        Reference(const Reference&) = default;

        // 要素同士の代入(Grid->ColWidths[1] = Grid->ColWidths[0];)は値のコピーとして扱う。
        Reference& operator=(const Reference& other)
        {
            setter_(owner_, index_, other.getter_(other.owner_, other.index_));
            return *this;
        }

        Reference& operator=(const T& value)
        {
            setter_(owner_, index_, value);
            return *this;
        }

        operator T() const
        {
            return getter_(owner_, index_);
        }

        T operator->() const
        {
            return getter_(owner_, index_);
        }

    private:
        TObject* owner_;
        int      index_;
        Getter   getter_;
        Setter   setter_;
    };

    IndexedProperty(TObject* owner, Getter getter, Setter setter)
        : owner_(owner)
        , getter_(getter)
        , setter_(setter)
    {}

    IndexedProperty(const IndexedProperty&) = delete;
    IndexedProperty& operator=(const IndexedProperty&) = delete;

    Reference operator[](int index) const
    {
        return Reference(owner_, index, getter_, setter_);
    }

private:
    TObject* owner_;
    Getter   getter_;
    Setter   setter_;
};

// 添字が 2 つのインデックス付きのプロパティ(VCL の TStringGrid::Cells[ACol][ARow])。
// Delphi の Cells[ACol, ARow] は、C++Builder と同じく Cells[ACol][ARow] と書く(1 つ目が列、2 つ目が行)。
template<typename T>
class IndexedProperty2
{
public:
    using Getter = T    (*)(TObject*, int, int);
    using Setter = void (*)(TObject*, int, int, const T&);

    // 1 つの要素へのプロキシ(IndexedProperty::Reference と同じ振る舞い)。
    class Reference
    {
    public:
        Reference(TObject* owner, int index1, int index2, Getter getter, Setter setter)
            : owner_(owner), index1_(index1), index2_(index2), getter_(getter), setter_(setter)
        {}

        Reference(const Reference&) = default;

        Reference& operator=(const Reference& other)
        {
            setter_(owner_, index1_, index2_, other.getter_(other.owner_, other.index1_, other.index2_));
            return *this;
        }

        Reference& operator=(const T& value)
        {
            setter_(owner_, index1_, index2_, value);
            return *this;
        }

        operator T() const
        {
            return getter_(owner_, index1_, index2_);
        }

        T operator->() const
        {
            return getter_(owner_, index1_, index2_);
        }

    private:
        TObject* owner_;
        int      index1_;
        int      index2_;
        Getter   getter_;
        Setter   setter_;
    };

    // 1 つ目の添字だけを指定した途中の段階。2 つ目の添字で Reference を返す。
    class Slice
    {
    public:
        Slice(TObject* owner, int index1, Getter getter, Setter setter)
            : owner_(owner), index1_(index1), getter_(getter), setter_(setter)
        {}

        Reference operator[](int index2) const
        {
            return Reference(owner_, index1_, index2, getter_, setter_);
        }

    private:
        TObject* owner_;
        int      index1_;
        Getter   getter_;
        Setter   setter_;
    };

    IndexedProperty2(TObject* owner, Getter getter, Setter setter)
        : owner_(owner)
        , getter_(getter)
        , setter_(setter)
    {}

    IndexedProperty2(const IndexedProperty2&) = delete;
    IndexedProperty2& operator=(const IndexedProperty2&) = delete;

    Slice operator[](int index1) const
    {
        return Slice(owner_, index1, getter_, setter_);
    }

private:
    TObject* owner_;
    Getter   getter_;
    Setter   setter_;
};

// イベントハンドラの型。C++Builder の TNotifyEvent に合わせ、イベントを発生させたオブジェクトを
// Sender として受け取る(static_cast / dynamic_cast で具体的な型に戻して使う)。
// 本家の __closure は標準 C++ に無いため、メンバ関数は [this](TObject* Sender) { Button1Click(Sender); }
// のようにラムダで割り当てる。nullptr を代入するとハンドラを解除できる。
using TNotifyEvent = std::function<void(TObject* Sender)>;

// フォームを閉じるときの動作(C++Builder / LCL の TCloseAction と同じ値)。
enum TCloseAction
{
    caNone,      // 閉じない
    caHide,      // 隠す(MainForm 以外の既定値)
    caFree,      // 破棄する(MainForm の既定値。MainForm ならアプリケーションを終了する)
    caMinimize   // 最小化する
};

// OnClose の型。Action には既定の動作が入っており、書き換えると Close の動作が変わる。
using TCloseEvent = std::function<void(TObject* Sender, TCloseAction& Action)>;

// OnCloseQuery の型。CanClose には true が入っており、false にすると閉じるのを取りやめる(OnClose より前に呼ばれる)。
using TCloseQueryEvent = std::function<void(TObject* Sender, bool& CanClose)>;

// 修飾キー・マウスボタンの状態を表すビット集合(LCL の TShiftState に対応)。複数のビットを OR して使う。
using TShiftState = unsigned int;
const TShiftState ssShift  = 0x0001;
const TShiftState ssAlt    = 0x0002;
const TShiftState ssCtrl   = 0x0004;
const TShiftState ssLeft   = 0x0008;  // マウスの左ボタンが押されている
const TShiftState ssRight  = 0x0010;
const TShiftState ssMiddle = 0x0020;
const TShiftState ssDouble = 0x0040;  // ダブルクリックの一部として発生した
const TShiftState ssMeta   = 0x0080;
const TShiftState ssSuper  = 0x0100;
const TShiftState ssHyper  = 0x0200;
const TShiftState ssAltGr  = 0x0400;
const TShiftState ssCaps   = 0x0800;
const TShiftState ssNum    = 0x1000;
const TShiftState ssScroll = 0x2000;
const TShiftState ssTriple = 0x4000;
const TShiftState ssQuad   = 0x8000;
const TShiftState ssExtra1 = 0x10000;
const TShiftState ssExtra2 = 0x20000;

// マウスボタン(OnMouseDown / OnMouseUp の Button)。
enum TMouseButton
{
    mbLeft,
    mbRight,
    mbMiddle,
    mbExtra1,
    mbExtra2
};

// キー入力・マウス操作のイベント。Key・Handled は書き換え可能で、書き換えると LCL に渡る値・以降の既定の処理が変わる
// (OnKeyDown/OnKeyUp で Key を 0 にする、OnKeyPress で Key を 0 にする、いずれもその入力を LCL に渡さない)。
using TKeyEvent = std::function<void(TObject* Sender, int& Key, TShiftState Shift)>;
using TKeyPressEvent = std::function<void(TObject* Sender, char& Key)>;
using TMouseEvent = std::function<void(TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y)>;
using TMouseMoveEvent = std::function<void(TObject* Sender, TShiftState Shift, int X, int Y)>;
// Handled に true を書き込むと、ホイール操作をこのハンドラで処理済みとして扱う(既定のスクロール等が起きなくなる)。
using TMouseWheelEvent = std::function<void(TObject* Sender, TShiftState Shift, int WheelDelta, int X, int Y, bool& Handled)>;

class TPersistent : public TObject
{
protected:
    explicit TPersistent(no_vcl_obj_t handle) : TObject(handle) {}
    ~TPersistent() override = default;

private:
    friend class ItemRegistry;  // 項目のラッパーを TPersistent* 経由で delete するため
};

// TComponent ではない項目(TTreeNode・TListItem・TListColumn)のラッパーの共通の管理(利用者は直接使わない)。
// 項目は FreeNotification を持たないため、TComponent のレジストリとは別に、DLL の「項目の破棄通知」
// (no_vcl_ItemFree_SetCallback)でラッパーを delete する。同じ項目には常に同じラッパーが返る。
// 各項目のクラスは、ハンドルを受け取るコンストラクタを private にし、friend class ItemRegistry とする。
class ItemRegistry
{
public:
    // ハンドルからラッパーを得る(無ければ作る)。nullptr には nullptr を返す。
    template<typename T>
    static T* Wrap(no_vcl_obj_t handle)
    {
        if (!handle)
            return nullptr;
        InstallCallback();
        std::unordered_map<no_vcl_obj_t, TPersistent*>& registry = Registry();
        auto it = registry.find(handle);
        if (it != registry.end())
            return static_cast<T*>(it->second);
        T* item = new T(handle);
        registry[handle] = item;
        return item;
    }

private:
    static void InstallCallback();
    static std::unordered_map<no_vcl_obj_t, TPersistent*>& Registry();
    static void NO_VCL_CALL FreeTrampoline(no_vcl_obj_t handle, void* data);
};

// LCL のコンポーネント(Create/Destroy を持つオブジェクト)。
// ハンドルと C++ ラッパーの対応を共通のレジストリで管理し、
// コールバックのトランポリンや Parent の Getter から C++ ラッパーを引けるようにする。
//
// C++ ラッパーの寿命は LCL オブジェクトの寿命と一致する。LCL オブジェクトが破棄されると
// (Free() でも、Owner による連鎖破棄でも)破棄通知を受けてラッパーも delete される。
// このため、コンポーネントは必ず new で生成し、delete ではなく Free() で破棄する
// (Owner を持つものは Owner に任せてよい)。スタック上に置いたり delete したりできないよう、
// コンポーネント系の全クラスでデストラクタを protected にしている
// (派生クラスを作る場合も、デストラクタを protected で宣言すること)。
class TComponent : public TPersistent
{
public:
    // LCL オブジェクトを破棄する。破棄通知によってこのラッパーも delete されるため、
    // 呼び出し後にこのオブジェクトへ触れてはならない。
    void Free();

protected:
    explicit TComponent(no_vcl_obj_t handle);
    ~TComponent() override;

    static no_vcl_obj_t HandleOf(const TObject* obj) { return obj ? obj->Handle() : nullptr; }
    static TComponent*  FromHandle(no_vcl_obj_t handle);

    // LCL が内部で生成したコンポーネント(TMenu::Items のルート項目等、*_Create を経由しないもの)のハンドルから
    // ラッパーを得る。まだラッパーが無ければ、その場で作ってレジストリに登録する(以降は同じラッパーを返す)。
    // ハンドルを返す C API の側で破棄通知の対象に登録しておくこと(登録されていないとラッパーが delete されない)。
    // T は、ハンドルを受け取るコンストラクタを TComponent から呼べるようにしておく(friend class TComponent)。
    template<typename T>
    static T* WrapExisting(no_vcl_obj_t handle)
    {
        if (!handle)
            return nullptr;
        if (TComponent* existing = FromHandle(handle))
            return static_cast<T*>(existing);
        return new T(handle);
    }

private:
    // LCL オブジェクトの破棄通知によって delete されるときに true にする。
    // それ以外でデストラクタが走るのは、派生クラスのコンストラクタが例外を投げた場合(基底部分の巻き戻し)だけで、
    // そのときは LCL オブジェクトが取り残されないよう、ここで破棄する。
    bool freedByLcl_ = false;

    static void NO_VCL_CALL FreeNotifyTrampoline(no_vcl_obj_t handle, void* data);
    static std::unordered_map<no_vcl_obj_t, TComponent*>& Registry();
};

// ショートカットキー(VCL の TShortCut と同じく、仮想キーコードに修飾キーのビットを OR した値)。
using TShortCut = unsigned short;
const TShortCut scShift = 0x2000;
const TShortCut scCtrl  = 0x4000;
const TShortCut scAlt   = 0x8000;

// VCL の Menus ユニットの同名の関数に対応する。Shift のうち ssShift/ssCtrl/ssAlt 以外は無視される。
TShortCut   ShortCut(unsigned short Key, TShiftState Shift);
// "Ctrl+S" のような文字列との変換。解釈できない文字列は 0 になる。
TShortCut   TextToShortCut(const std::string& Text);
std::string ShortCutToText(TShortCut ShortCut);

class TMenu;

// メニューの項目。TControl ではない(Parent/Left 等は無く、画面上の親子関係は Add/Insert で組む)。
// 子の項目は LCL の Items[Index] / Count に合わせ、GetItem(Index) / Count で参照する。
// 親の項目が破棄されると、子の項目も(Owner が別でも)一緒に破棄される(LCL の仕様。ラッパーも delete される)。
class TMenuItem : public TComponent
{
public:
    explicit TMenuItem(TComponent* AOwner);

    // "-" を設定すると区切り線になる。
    Property<std::string>  Caption;
    Property<bool>         Checked;
    Property<bool>         Enabled;
    Property<bool>         Visible;
    // true にすると、選ばれるたびに Checked が反転する(RadioItem なら同じ GroupIndex の他の項目が外れる)。
    Property<bool>         AutoCheck;
    Property<bool>         RadioItem;
    Property<int>          GroupIndex;
    Property<bool>         Default;
    Property<TShortCut>    ShortCut;
    Property<std::string>  Hint;
    Property<TNotifyEvent> OnClick;

    ReadOnlyProperty<int>        Count;
    // 親の項目。メニューの直下の項目なら、そのメニューの Items(ルート)。どこにも追加されていなければ nullptr。
    ReadOnlyProperty<TMenuItem*> Parent;

    TMenuItem* GetItem(int Index) const;
    void Add(TMenuItem* Item);
    void Insert(int Index, TMenuItem* Item);
    // Delete/Remove は子から外すだけで破棄しない(VCL と同じ)。Clear はすべての子を破棄する。
    void Delete(int Index);
    void Remove(TMenuItem* Item);
    void Clear();
    int  IndexOf(TMenuItem* Item) const;
    // 区切り線を末尾に追加する(項目は LCL が内部で生成する。GetItem で取得できる)。
    void AddSeparator();
    bool IsLine() const;
    // 利用者が項目を選んだときと同じ処理(AutoCheck の反映と OnClick)を行う。
    void Click();

protected:
    ~TMenuItem() override = default;

private:
    friend class TComponent;  // WrapExisting から、下のハンドルを受け取るコンストラクタを呼ぶため
    explicit TMenuItem(no_vcl_obj_t handle);

    TNotifyEvent onClick_;
    bool         onClickHooked_ = false;
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender, void* data);

    static std::string  GetCaptionImpl(TObject* owner);
    static void         SetCaptionImpl(TObject* owner, const std::string& value);
    static bool         GetCheckedImpl(TObject* owner);
    static void         SetCheckedImpl(TObject* owner, const bool& value);
    static bool         GetEnabledImpl(TObject* owner);
    static void         SetEnabledImpl(TObject* owner, const bool& value);
    static bool         GetVisibleImpl(TObject* owner);
    static void         SetVisibleImpl(TObject* owner, const bool& value);
    static bool         GetAutoCheckImpl(TObject* owner);
    static void         SetAutoCheckImpl(TObject* owner, const bool& value);
    static bool         GetRadioItemImpl(TObject* owner);
    static void         SetRadioItemImpl(TObject* owner, const bool& value);
    static int          GetGroupIndexImpl(TObject* owner);
    static void         SetGroupIndexImpl(TObject* owner, const int& value);
    static bool         GetDefaultImpl(TObject* owner);
    static void         SetDefaultImpl(TObject* owner, const bool& value);
    static TShortCut    GetShortCutImpl(TObject* owner);
    static void         SetShortCutImpl(TObject* owner, const TShortCut& value);
    static std::string  GetHintImpl(TObject* owner);
    static void         SetHintImpl(TObject* owner, const std::string& value);
    static TNotifyEvent GetOnClickImpl(TObject* owner);
    static void         SetOnClickImpl(TObject* owner, const TNotifyEvent& value);
    static int          GetCountImpl(TObject* owner);
    static TMenuItem*   GetParentImpl(TObject* owner);
};

// TMainMenu・TPopupMenu の共通の基底。Items はメニューのルートの項目で、メニュー自身が(LCL の内部で)生成・所有する。
// メニューに表示する項目は Items->Add(...) で追加する。
class TMenu : public TComponent
{
public:
    ReadOnlyProperty<TMenuItem*> Items;

protected:
    explicit TMenu(no_vcl_obj_t handle);
    ~TMenu() override = default;

private:
    static TMenuItem* GetItemsImpl(TObject* owner);
};

// フォームのメニューバー。TForm::Menu に割り当てると表示される。
class TMainMenu : public TMenu
{
public:
    explicit TMainMenu(TComponent* AOwner);

protected:
    ~TMainMenu() override = default;
};

// 右クリック等で開くメニュー。TControl::PopupMenu に割り当てると、そのコントロールの右クリックで開く(AutoPopup が true のとき)。
class TPopupMenu : public TMenu
{
public:
    explicit TPopupMenu(TComponent* AOwner);

    Property<bool>         AutoPopup;
    // 右クリックでメニューを開いたコンポーネント(OnPopup の中で、どこから開かれたかを知るのに使う)。
    // C++ ラッパーを介さずに作られたコンポーネントの場合は nullptr になる。
    Property<TComponent*>  PopupComponent;
    // 開く直前に呼ばれる。
    Property<TNotifyEvent> OnPopup;
    // 閉じた後に呼ばれる。
    Property<TNotifyEvent> OnClose;

    // X, Y はスクリーン座標。Win32 ではメニューが閉じるまで戻らない。
    void Popup(int X, int Y);

protected:
    ~TPopupMenu() override = default;

private:
    TNotifyEvent onPopup_;
    TNotifyEvent onClose_;
    bool         onPopupHooked_ = false;
    bool         onCloseHooked_ = false;
    static void NO_VCL_CALL PopupTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL CloseTrampoline(no_vcl_obj_t sender, void* data);

    static bool         GetAutoPopupImpl(TObject* owner);
    static void         SetAutoPopupImpl(TObject* owner, const bool& value);
    static TComponent*  GetPopupComponentImpl(TObject* owner);
    static void         SetPopupComponentImpl(TObject* owner, TComponent* const& value);
    static TNotifyEvent GetOnPopupImpl(TObject* owner);
    static void         SetOnPopupImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnCloseImpl(TObject* owner);
    static void         SetOnCloseImpl(TObject* owner, const TNotifyEvent& value);
};

class TWinControl;

// 親のクライアント領域への寄せ方(LCL / VCL の TAlign と同じ値)。寄せた方向の位置・大きさは LCL が決める
// (alTop なら Left/Top/Width が親に合わせられ、Height だけが保たれる。alClient は残りの領域をすべて埋める)。
enum TAlign { alNone, alTop, alBottom, alLeft, alRight, alClient, alCustom };

class TControl : public TComponent
{
public:
    Property<TWinControl*> Parent;
    Property<int>          Left;
    Property<int>          Top;
    Property<int>          Width;
    Property<int>          Height;
    Property<bool>         Visible;
    Property<bool>         Enabled;
    Property<std::string>  Caption;
    // 既定値はクラスごとに異なる(多くは alNone、TStatusBar は alBottom、TSplitter は alLeft)。
    Property<TAlign>       Align;
    // 右クリックで開くメニュー。C++ ラッパーを介さずに作られたメニューの場合は nullptr になる。
    Property<TPopupMenu*>  PopupMenu;
    Property<TNotifyEvent> OnClick;
    Property<TNotifyEvent> OnDblClick;
    // LCL では他のウィンドウメッセージへの応答等で発生し、必ずしもユーザー操作直後とは限らない。
    Property<TNotifyEvent> OnResize;
    Property<TMouseEvent>      OnMouseDown;
    Property<TMouseEvent>      OnMouseUp;
    Property<TMouseMoveEvent>  OnMouseMove;
    Property<TNotifyEvent>     OnMouseEnter;
    Property<TNotifyEvent>     OnMouseLeave;
    Property<TMouseWheelEvent> OnMouseWheel;

    void Show();
    void Hide();

protected:
    explicit TControl(no_vcl_obj_t handle);
    ~TControl() override = default;

    // LCL では TControl の protected。TCustomEdit / TCustomComboBox が公開する。
    Property<std::string>  Text;

private:
    TNotifyEvent onClick_;
    bool         onClickHooked_ = false;
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnClickImpl(TObject* owner);
    static void         SetOnClickImpl(TObject* owner, const TNotifyEvent& value);

    TNotifyEvent     onDblClick_;
    TNotifyEvent     onResize_;
    TMouseEvent      onMouseDown_;
    TMouseEvent      onMouseUp_;
    TMouseMoveEvent  onMouseMove_;
    TNotifyEvent     onMouseEnter_;
    TNotifyEvent     onMouseLeave_;
    TMouseWheelEvent onMouseWheel_;
    bool onDblClickHooked_   = false;
    bool onResizeHooked_     = false;
    bool onMouseDownHooked_  = false;
    bool onMouseUpHooked_    = false;
    bool onMouseMoveHooked_  = false;
    bool onMouseEnterHooked_ = false;
    bool onMouseLeaveHooked_ = false;
    bool onMouseWheelHooked_ = false;

    static void NO_VCL_CALL DblClickTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL ResizeTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL MouseEnterTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL MouseLeaveTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL MouseDownTrampoline(no_vcl_obj_t sender, no_vcl_int_t button, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void* data);
    static void NO_VCL_CALL MouseUpTrampoline(no_vcl_obj_t sender, no_vcl_int_t button, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void* data);
    static void NO_VCL_CALL MouseMoveTrampoline(no_vcl_obj_t sender, no_vcl_int_t shift, no_vcl_int_t x, no_vcl_int_t y, void* data);
    static void NO_VCL_CALL MouseWheelTrampoline(no_vcl_obj_t sender, no_vcl_int_t shift, no_vcl_int_t wheelDelta, no_vcl_int_t x, no_vcl_int_t y, no_vcl_bool_t* handled, void* data);

    static TNotifyEvent GetOnDblClickImpl(TObject* owner);
    static void         SetOnDblClickImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnResizeImpl(TObject* owner);
    static void         SetOnResizeImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnMouseEnterImpl(TObject* owner);
    static void         SetOnMouseEnterImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnMouseLeaveImpl(TObject* owner);
    static void         SetOnMouseLeaveImpl(TObject* owner, const TNotifyEvent& value);
    static TMouseEvent  GetOnMouseDownImpl(TObject* owner);
    static void         SetOnMouseDownImpl(TObject* owner, const TMouseEvent& value);
    static TMouseEvent  GetOnMouseUpImpl(TObject* owner);
    static void         SetOnMouseUpImpl(TObject* owner, const TMouseEvent& value);
    static TMouseMoveEvent GetOnMouseMoveImpl(TObject* owner);
    static void            SetOnMouseMoveImpl(TObject* owner, const TMouseMoveEvent& value);
    static TMouseWheelEvent GetOnMouseWheelImpl(TObject* owner);
    static void             SetOnMouseWheelImpl(TObject* owner, const TMouseWheelEvent& value);

    static TWinControl* GetParentImpl(TObject* owner);
    static void         SetParentImpl(TObject* owner, TWinControl* const& value);
    static int          GetLeftImpl(TObject* owner);
    static void         SetLeftImpl(TObject* owner, const int& value);
    static int          GetTopImpl(TObject* owner);
    static void         SetTopImpl(TObject* owner, const int& value);
    static int          GetWidthImpl(TObject* owner);
    static void         SetWidthImpl(TObject* owner, const int& value);
    static int          GetHeightImpl(TObject* owner);
    static void         SetHeightImpl(TObject* owner, const int& value);
    static bool         GetVisibleImpl(TObject* owner);
    static void         SetVisibleImpl(TObject* owner, const bool& value);
    static bool         GetEnabledImpl(TObject* owner);
    static void         SetEnabledImpl(TObject* owner, const bool& value);
    static std::string  GetCaptionImpl(TObject* owner);
    static void         SetCaptionImpl(TObject* owner, const std::string& value);
    static TAlign       GetAlignImpl(TObject* owner);
    static void         SetAlignImpl(TObject* owner, const TAlign& value);
    static TPopupMenu*  GetPopupMenuImpl(TObject* owner);
    static void         SetPopupMenuImpl(TObject* owner, TPopupMenu* const& value);
    static std::string  GetTextImpl(TObject* owner);
    static void         SetTextImpl(TObject* owner, const std::string& value);
};

class TWinControl : public TControl
{
public:
    Property<TKeyEvent>      OnKeyDown;
    Property<TKeyEvent>      OnKeyUp;
    Property<TKeyPressEvent> OnKeyPress;

protected:
    explicit TWinControl(no_vcl_obj_t handle);
    ~TWinControl() override = default;

private:
    TKeyEvent      onKeyDown_;
    TKeyEvent      onKeyUp_;
    TKeyPressEvent onKeyPress_;
    bool onKeyDownHooked_  = false;
    bool onKeyUpHooked_    = false;
    bool onKeyPressHooked_ = false;

    static void NO_VCL_CALL KeyDownTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, no_vcl_int_t shift, void* data);
    static void NO_VCL_CALL KeyUpTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, no_vcl_int_t shift, void* data);
    static void NO_VCL_CALL KeyPressTrampoline(no_vcl_obj_t sender, no_vcl_int_t* key, void* data);

    static TKeyEvent GetOnKeyDownImpl(TObject* owner);
    static void      SetOnKeyDownImpl(TObject* owner, const TKeyEvent& value);
    static TKeyEvent GetOnKeyUpImpl(TObject* owner);
    static void      SetOnKeyUpImpl(TObject* owner, const TKeyEvent& value);
    static TKeyPressEvent GetOnKeyPressImpl(TObject* owner);
    static void           SetOnKeyPressImpl(TObject* owner, const TKeyPressEvent& value);
};

// つまみを左右または上下にドラッグして値を選ぶスクロールバー。
enum TScrollBarKind { sbHorizontal, sbVertical };

class TCustomScrollBar : public TWinControl
{
public:
    Property<TScrollBarKind> Kind;
    Property<int>            Min;
    Property<int>            Max;
    Property<int>            Position;
    Property<int>            PageSize;
    Property<TNotifyEvent>   OnChange;

protected:
    explicit TCustomScrollBar(no_vcl_obj_t handle);
    ~TCustomScrollBar() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, void* data);

    static TScrollBarKind GetKindImpl(TObject* owner);
    static void           SetKindImpl(TObject* owner, const TScrollBarKind& value);
    static int            GetMinImpl(TObject* owner);
    static void           SetMinImpl(TObject* owner, const int& value);
    static int            GetMaxImpl(TObject* owner);
    static void           SetMaxImpl(TObject* owner, const int& value);
    static int            GetPositionImpl(TObject* owner);
    static void           SetPositionImpl(TObject* owner, const int& value);
    static int            GetPageSizeImpl(TObject* owner);
    static void           SetPageSizeImpl(TObject* owner, const int& value);
    static TNotifyEvent   GetOnChangeImpl(TObject* owner);
    static void           SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

class TScrollBar : public TCustomScrollBar
{
public:
    explicit TScrollBar(TComponent* AOwner);

protected:
    ~TScrollBar() override = default;
};

// つまみをドラッグして値を選ぶスライダー。
class TCustomTrackBar : public TWinControl
{
public:
    Property<int>          Min;
    Property<int>          Max;
    Property<int>          Position;
    Property<TNotifyEvent> OnChange;

protected:
    explicit TCustomTrackBar(no_vcl_obj_t handle);
    ~TCustomTrackBar() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, void* data);

    static int          GetMinImpl(TObject* owner);
    static void         SetMinImpl(TObject* owner, const int& value);
    static int          GetMaxImpl(TObject* owner);
    static void         SetMaxImpl(TObject* owner, const int& value);
    static int          GetPositionImpl(TObject* owner);
    static void         SetPositionImpl(TObject* owner, const int& value);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

class TTrackBar : public TCustomTrackBar
{
public:
    explicit TTrackBar(TComponent* AOwner);

protected:
    ~TTrackBar() override = default;
};

// 進捗を表示する表示専用コントロール(イベントは無い)。
class TCustomProgressBar : public TWinControl
{
public:
    Property<int> Min;
    Property<int> Max;
    Property<int> Position;

protected:
    explicit TCustomProgressBar(no_vcl_obj_t handle);
    ~TCustomProgressBar() override = default;

private:
    static int  GetMinImpl(TObject* owner);
    static void SetMinImpl(TObject* owner, const int& value);
    static int  GetMaxImpl(TObject* owner);
    static void SetMaxImpl(TObject* owner, const int& value);
    static int  GetPositionImpl(TObject* owner);
    static void SetPositionImpl(TObject* owner, const int& value);
};

class TProgressBar : public TCustomProgressBar
{
public:
    explicit TProgressBar(TComponent* AOwner);

protected:
    ~TProgressBar() override = default;
};

class TGraphicControl : public TControl
{
protected:
    explicit TGraphicControl(no_vcl_obj_t handle) : TControl(handle) {}
    ~TGraphicControl() override = default;
};

class TCustomControl : public TWinControl
{
protected:
    explicit TCustomControl(no_vcl_obj_t handle) : TWinControl(handle) {}
    ~TCustomControl() override = default;
};

// Edit 等に付属する上下矢印。Min/Max/Position/Increment/Associate は LCL では TCustomUpDown の
// protected だが、唯一の具象クラス TUpDown が published にしているため、TUpDown に直接置く
// (TCheckBox の Checked と同じ形)。OnClick/OnChanging は独自のシグネチャのため今回は未対応。
class TUpDown : public TCustomControl
{
public:
    explicit TUpDown(TComponent* AOwner);

    Property<int>          Min;
    Property<int>          Max;
    Property<int>          Position;
    Property<int>          Increment;
    // 値を増減させる対象のコントロール(TEdit 等)。
    Property<TWinControl*> Associate;

protected:
    ~TUpDown() override = default;

private:
    static int          GetMinImpl(TObject* owner);
    static void         SetMinImpl(TObject* owner, const int& value);
    static int          GetMaxImpl(TObject* owner);
    static void         SetMaxImpl(TObject* owner, const int& value);
    static int          GetPositionImpl(TObject* owner);
    static void         SetPositionImpl(TObject* owner, const int& value);
    static int          GetIncrementImpl(TObject* owner);
    static void         SetIncrementImpl(TObject* owner, const int& value);
    static TWinControl* GetAssociateImpl(TObject* owner);
    static void         SetAssociateImpl(TObject* owner, TWinControl* const& value);
};

/* ---------------- Form ---------------- */

class TScrollingWinControl : public TCustomControl
{
protected:
    explicit TScrollingWinControl(no_vcl_obj_t handle) : TCustomControl(handle) {}
    ~TScrollingWinControl() override = default;
};

// スクロール可能な汎用コンテナ。TScrollingWinControl の直接の派生で、追加のメンバは無い。
class TScrollBox : public TScrollingWinControl
{
public:
    explicit TScrollBox(TComponent* AOwner);

protected:
    ~TScrollBox() override = default;
};

class TCustomForm : public TScrollingWinControl
{
public:
    // LCL の TCustomForm は Show/Hide を独自に宣言している(TControl のものを隠す)。
    void Show();
    void Hide();
    int  ShowModal();
    void Close();

    // 保留中のメッセージを処理し終えてから破棄する(破棄後はラッパーも delete される)。
    // Free() と違い、フォーム自身やその子のイベントハンドラの中からでも安全に呼べる。
    void Release();

    // フォームの生成が完了したときに 1 度だけ呼ばれる。
    // Application->CreateForm で生成した場合は、派生クラスのコンストラクタの完了直後(C++Builder と同じ時点)。
    // new で直接生成した場合は、C++ ではコンストラクタの完了を検知できないため、最初に表示される直前になる。
    // どちらの場合もコンストラクタの中で設定すればよい。
    Property<TNotifyEvent>     OnCreate;
    Property<TNotifyEvent>     OnShow;
    Property<TNotifyEvent>     OnHide;
    Property<TNotifyEvent>     OnActivate;
    Property<TNotifyEvent>     OnDeactivate;
    Property<TCloseQueryEvent> OnCloseQuery;
    Property<TCloseEvent>      OnClose;
    // 破棄の最初に呼ばれる(子コントロールはまだ有効)。このあとラッパーも delete される。
    Property<TNotifyEvent>     OnDestroy;
    // フォームのメニューバー。nullptr を代入すると外す(メニュー自体は破棄されない)。
    Property<TMainMenu*>       Menu;

protected:
    explicit TCustomForm(no_vcl_obj_t handle);
    ~TCustomForm() override = default;

private:
    friend class TApplication;

    TNotifyEvent     onCreate_;
    TNotifyEvent     onShow_;
    TNotifyEvent     onHide_;
    TNotifyEvent     onActivate_;
    TNotifyEvent     onDeactivate_;
    TCloseQueryEvent onCloseQuery_;
    TCloseEvent      onClose_;
    TNotifyEvent     onDestroy_;
    bool             created_ = false;
    bool             onHideHooked_ = false;
    bool             onActivateHooked_ = false;
    bool             onDeactivateHooked_ = false;
    bool             onCloseQueryHooked_ = false;
    bool             onCloseHooked_ = false;
    bool             onDestroyHooked_ = false;

    // OnCreate がまだ呼ばれていなければ呼ぶ。
    void DoCreate();

    static void NO_VCL_CALL ShowTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL HideTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL ActivateTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL DeactivateTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL CloseQueryTrampoline(no_vcl_obj_t sender, no_vcl_bool_t* canClose, void* data);
    static void NO_VCL_CALL CloseTrampoline(no_vcl_obj_t sender, no_vcl_int_t* action, void* data);
    static void NO_VCL_CALL DestroyTrampoline(no_vcl_obj_t sender, void* data);

    static TNotifyEvent     GetOnCreateImpl(TObject* owner);
    static void             SetOnCreateImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent     GetOnShowImpl(TObject* owner);
    static void             SetOnShowImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent     GetOnHideImpl(TObject* owner);
    static void             SetOnHideImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent     GetOnActivateImpl(TObject* owner);
    static void             SetOnActivateImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent     GetOnDeactivateImpl(TObject* owner);
    static void             SetOnDeactivateImpl(TObject* owner, const TNotifyEvent& value);
    static TCloseQueryEvent GetOnCloseQueryImpl(TObject* owner);
    static void             SetOnCloseQueryImpl(TObject* owner, const TCloseQueryEvent& value);
    static TCloseEvent      GetOnCloseImpl(TObject* owner);
    static void             SetOnCloseImpl(TObject* owner, const TCloseEvent& value);
    static TNotifyEvent     GetOnDestroyImpl(TObject* owner);
    static void             SetOnDestroyImpl(TObject* owner, const TNotifyEvent& value);
    static TMainMenu*       GetMenuImpl(TObject* owner);
    static void             SetMenuImpl(TObject* owner, TMainMenu* const& value);
};

class TForm : public TCustomForm
{
public:
    explicit TForm(TComponent* AOwner);

protected:
    ~TForm() override = default;

private:
    friend class TApplication;

    // TApplication::CreateForm が LCL の CreateForm で生成済みの、コンストラクタに引き取られるのを待つハンドル。
    // Owner が Application のときだけ、新たに生成せずこれを使う。
    static no_vcl_obj_t pendingHandle_;
    static no_vcl_obj_t CreateHandle(TComponent* AOwner);
};

// C++Builder の TApplication。LCL の TApplication は FCL の TCustomApplication の派生だが、
// C++Builder に合わせて TComponent 直下に置く。インスタンスはグローバル変数 Application の 1 つだけ。
//
// Application が所有するフォーム(CreateForm や new TForm(Application) で生成したもの)は、
// プログラムの終了時(main から戻った後)にまとめて破棄され、ラッパーのデストラクタも呼ばれる。
class TApplication : public TComponent
{
public:
    // CreateForm で最初に生成したフォーム。Run はこれを表示し、これが閉じられると戻る。
    ReadOnlyProperty<TForm*> MainForm;
    ReadOnlyProperty<bool>   Terminated;
    Property<std::string>    Title;
    Property<bool>           ShowMainForm;

    // LCL 側は DLL の読み込み時に初期化済みのため何もしない(C++Builder のコードとの互換のために置く)。
    void Initialize() {}

    // C++Builder の Application->CreateForm(__classid(TForm1), &Form1) に相当する。
    // __classid は標準 C++ に無いため、型は引数から推論する: Application->CreateForm(&Form1);
    // T は TForm の派生で、(TComponent* AOwner) を受け取って TForm(AOwner) に渡すコンストラクタを持つこと。
    // 本家と違い、Reference への代入はコンストラクタの完了後になる。
    template<typename T>
    void CreateForm(T** Reference);

    void Run();
    void ProcessMessages();
    void Terminate();

protected:
    ~TApplication() override = default;

private:
    friend TApplication* NewApplication();
    explicit TApplication(no_vcl_obj_t handle);

    void BeginCreateForm();
    void EndCreateForm();
    static void Shutdown();

    static TForm*      GetMainFormImpl(TObject* owner);
    static bool        GetTerminatedImpl(TObject* owner);
    static std::string GetTitleImpl(TObject* owner);
    static void        SetTitleImpl(TObject* owner, const std::string& value);
    static bool        GetShowMainFormImpl(TObject* owner);
    static void        SetShowMainFormImpl(TObject* owner, const bool& value);
};

// C++Builder と同じく、アプリケーションに 1 つのグローバル変数として公開する。
// 静的初期化の順序は規定されないため、他の翻訳単位のグローバル変数の初期化子からは使わないこと。
extern TApplication* Application;

template<typename T>
void TApplication::CreateForm(T** Reference)
{
    static_assert(std::is_base_of<TForm, T>::value, "CreateForm can only create classes derived from TForm");

    BeginCreateForm();
    try
    {
        *Reference = new T(this);
    }
    catch (...)
    {
        EndCreateForm();
        throw;
    }
    EndCreateForm();

    // C++Builder では OnCreate は最派生クラスのコンストラクタの完了後(AfterConstruction)に呼ばれる。
    static_cast<TCustomForm*>(*Reference)->DoCreate();
}

/* ---------------- Panel / GroupBox / Label ---------------- */

class TCustomPanel : public TCustomControl
{
protected:
    explicit TCustomPanel(no_vcl_obj_t handle) : TCustomControl(handle) {}
    ~TCustomPanel() override = default;
};

class TPanel : public TCustomPanel
{
public:
    explicit TPanel(TComponent* AOwner);

protected:
    ~TPanel() override = default;
};

class TCustomGroupBox : public TWinControl
{
protected:
    explicit TCustomGroupBox(no_vcl_obj_t handle) : TWinControl(handle) {}
    ~TCustomGroupBox() override = default;
};

class TGroupBox : public TCustomGroupBox
{
public:
    explicit TGroupBox(TComponent* AOwner);

protected:
    ~TGroupBox() override = default;
};

// ラジオボタンの一覧を項目文字列から自動生成するグループ。OnClick は TControl のものとは別の、
// このクラス自身のイベント(いずれかのボタンが押されたときに呼ばれる)。
class TCustomRadioGroup : public TCustomGroupBox
{
public:
    Property<int>          ItemIndex;
    Property<TNotifyEvent> OnClick;

    void        ItemsAdd(const std::string& text);
    void        ItemsClear();
    int         ItemsCount() const;
    std::string ItemsGetText(int index) const;

protected:
    explicit TCustomRadioGroup(no_vcl_obj_t handle);
    ~TCustomRadioGroup() override = default;

private:
    TNotifyEvent onClick_;
    bool         onClickHooked_ = false;
    static void NO_VCL_CALL ClickTrampoline(no_vcl_obj_t sender, void* data);

    static int           GetItemIndexImpl(TObject* owner);
    static void          SetItemIndexImpl(TObject* owner, const int& value);
    static TNotifyEvent  GetOnClickImpl(TObject* owner);
    static void          SetOnClickImpl(TObject* owner, const TNotifyEvent& value);
};

class TRadioGroup : public TCustomRadioGroup
{
public:
    explicit TRadioGroup(TComponent* AOwner);

protected:
    ~TRadioGroup() override = default;
};

// チェックボックスの一覧を項目文字列から自動生成するグループ。
class TCustomCheckGroup : public TCustomGroupBox
{
public:
    void        ItemsAdd(const std::string& text);
    void        ItemsClear();
    int         ItemsCount() const;
    std::string ItemsGetText(int index) const;
    bool        GetChecked(int index) const;
    void        SetChecked(int index, bool value);

protected:
    explicit TCustomCheckGroup(no_vcl_obj_t handle) : TCustomGroupBox(handle) {}
    ~TCustomCheckGroup() override = default;
};

class TCheckGroup : public TCustomCheckGroup
{
public:
    explicit TCheckGroup(TComponent* AOwner);

protected:
    ~TCheckGroup() override = default;
};

class TCustomLabel : public TGraphicControl
{
protected:
    explicit TCustomLabel(no_vcl_obj_t handle) : TGraphicControl(handle) {}
    ~TCustomLabel() override = default;
};

class TLabel : public TCustomLabel
{
public:
    explicit TLabel(TComponent* AOwner);

protected:
    ~TLabel() override = default;
};

// 枠線や凹凸の表現に使う、単純な表示専用コントロール(TGraphicControl の直接の派生)。
enum TBevelShape { bsBox, bsFrame, bsTopLine, bsBottomLine, bsLeftLine, bsRightLine, bsSpacer };
enum TBevelStyle { bsLowered, bsRaised };

class TBevel : public TGraphicControl
{
public:
    explicit TBevel(TComponent* AOwner);

    Property<TBevelShape> Shape;
    Property<TBevelStyle> Style;

protected:
    ~TBevel() override = default;

private:
    static TBevelShape GetShapeImpl(TObject* owner);
    static void        SetShapeImpl(TObject* owner, const TBevelShape& value);
    static TBevelStyle GetStyleImpl(TObject* owner);
    static void        SetStyleImpl(TObject* owner, const TBevelStyle& value);
};

/* ---------------- Button / CheckBox / RadioButton ---------------- */

class TButtonControl : public TWinControl
{
protected:
    explicit TButtonControl(no_vcl_obj_t handle);
    ~TButtonControl() override = default;

    // LCL では TButtonControl の protected。TCheckBox / TRadioButton が公開する。
    Property<bool> Checked;

private:
    static bool GetCheckedImpl(TObject* owner);
    static void SetCheckedImpl(TObject* owner, const bool& value);
};

class TCustomButton : public TButtonControl
{
protected:
    explicit TCustomButton(no_vcl_obj_t handle) : TButtonControl(handle) {}
    ~TCustomButton() override = default;
};

class TButton : public TCustomButton
{
public:
    explicit TButton(TComponent* AOwner);

protected:
    ~TButton() override = default;
};

// bkOK/bkCancel 等の定型ボタン(既定の Caption を LCL が設定する)。Glyph(ビットマップ)は未対応。
enum TBitBtnKind
{
    bkCustom, bkOK, bkCancel, bkHelp, bkYes, bkNo,
    bkClose, bkAbort, bkRetry, bkIgnore, bkAll,
    bkNoToAll, bkYesToAll
};

class TCustomBitBtn : public TCustomButton
{
public:
    Property<TBitBtnKind> Kind;

protected:
    explicit TCustomBitBtn(no_vcl_obj_t handle);
    ~TCustomBitBtn() override = default;

private:
    static TBitBtnKind GetKindImpl(TObject* owner);
    static void        SetKindImpl(TObject* owner, const TBitBtnKind& value);
};

class TBitBtn : public TCustomBitBtn
{
public:
    explicit TBitBtn(TComponent* AOwner);

protected:
    ~TBitBtn() override = default;
};

class TCustomCheckBox : public TButtonControl
{
protected:
    explicit TCustomCheckBox(no_vcl_obj_t handle) : TButtonControl(handle) {}
    ~TCustomCheckBox() override = default;
};

class TCheckBox : public TCustomCheckBox
{
public:
    using TButtonControl::Checked;

    explicit TCheckBox(TComponent* AOwner);

protected:
    ~TCheckBox() override = default;
};

class TRadioButton : public TCustomCheckBox
{
public:
    using TButtonControl::Checked;

    explicit TRadioButton(TComponent* AOwner);

protected:
    ~TRadioButton() override = default;
};

// オン/オフの状態をボタン風の見た目で表す。TCustomCheckBox の直接の派生で、Checked を共有する。
class TToggleBox : public TCustomCheckBox
{
public:
    using TButtonControl::Checked;

    explicit TToggleBox(TComponent* AOwner);

protected:
    ~TToggleBox() override = default;
};

/* ---------------- Edit / Memo ---------------- */

class TCustomEdit : public TWinControl
{
public:
    using TControl::Text;
    Property<int>          MaxLength;
    Property<bool>         ReadOnly;
    Property<TNotifyEvent> OnChange;

protected:
    explicit TCustomEdit(no_vcl_obj_t handle);
    ~TCustomEdit() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);

    static int  GetMaxLengthImpl(TObject* owner);
    static void SetMaxLengthImpl(TObject* owner, const int& value);
    static bool GetReadOnlyImpl(TObject* owner);
    static void SetReadOnlyImpl(TObject* owner, const bool& value);
};

class TEdit : public TCustomEdit
{
public:
    explicit TEdit(TComponent* AOwner);

protected:
    ~TEdit() override = default;
};

// 実数のスピンエディット。OnChange は基底 TCustomEdit のものをそのまま使う。
class TCustomFloatSpinEdit : public TCustomEdit
{
public:
    Property<double> Value;
    Property<double> MinValue;
    Property<double> MaxValue;
    Property<double> Increment;
    Property<int>    DecimalPlaces;

protected:
    explicit TCustomFloatSpinEdit(no_vcl_obj_t handle);
    ~TCustomFloatSpinEdit() override = default;

private:
    static double GetValueImpl(TObject* owner);
    static void   SetValueImpl(TObject* owner, const double& value);
    static double GetMinValueImpl(TObject* owner);
    static void   SetMinValueImpl(TObject* owner, const double& value);
    static double GetMaxValueImpl(TObject* owner);
    static void   SetMaxValueImpl(TObject* owner, const double& value);
    static double GetIncrementImpl(TObject* owner);
    static void   SetIncrementImpl(TObject* owner, const double& value);
    static int    GetDecimalPlacesImpl(TObject* owner);
    static void   SetDecimalPlacesImpl(TObject* owner, const int& value);
};

class TFloatSpinEdit : public TCustomFloatSpinEdit
{
public:
    explicit TFloatSpinEdit(TComponent* AOwner);

protected:
    ~TFloatSpinEdit() override = default;
};

// 整数のスピンエディット。LCL では TCustomFloatSpinEdit の派生で、Value/MinValue/MaxValue/Increment を
// Integer で再宣言して Double 版を隠す。C++ でも同じ名前の Property<int> で基底の Property<double> を隠す
// (C++ の名前隠蔽により、TCustomSpinEdit* 経由では int 版だけが見える)。
class TCustomSpinEdit : public TCustomFloatSpinEdit
{
public:
    Property<int> Value;
    Property<int> MinValue;
    Property<int> MaxValue;
    Property<int> Increment;

protected:
    explicit TCustomSpinEdit(no_vcl_obj_t handle);
    ~TCustomSpinEdit() override = default;

private:
    static int  GetValueImpl(TObject* owner);
    static void SetValueImpl(TObject* owner, const int& value);
    static int  GetMinValueImpl(TObject* owner);
    static void SetMinValueImpl(TObject* owner, const int& value);
    static int  GetMaxValueImpl(TObject* owner);
    static void SetMaxValueImpl(TObject* owner, const int& value);
    static int  GetIncrementImpl(TObject* owner);
    static void SetIncrementImpl(TObject* owner, const int& value);
};

class TSpinEdit : public TCustomSpinEdit
{
public:
    explicit TSpinEdit(TComponent* AOwner);

protected:
    ~TSpinEdit() override = default;
};

// 書式付き入力(郵便番号・電話番号等)。EditMask は TCustomMaskEdit では protected だが、
// 唯一の具象クラス TMaskEdit が published にしているため、TMaskEdit に直接置く(TUpDown と同じ形)。
class TMaskEdit : public TCustomEdit
{
public:
    explicit TMaskEdit(TComponent* AOwner);

    Property<std::string> EditMask;

protected:
    ~TMaskEdit() override = default;

private:
    static std::string GetEditMaskImpl(TObject* owner);
    static void         SetEditMaskImpl(TObject* owner, const std::string& value);
};

// タブの位置(LCL の TTabPosition と同じ値)。
enum TTabPosition { tpTop, tpBottom, tpLeft, tpRight };

// OnChanging の型。AllowChange には true が入っており、false にするとページの切り替えを取りやめる。
using TTabChangingEvent = std::function<void(TObject* Sender, bool& AllowChange)>;

// TTabControl と TPageControl の共通の基底。以下のメンバは LCL の TCustomTabControl の public。
class TCustomTabControl : public TWinControl
{
public:
    // ページ(TPageControl なら TTabSheet)の数。TTabControl では Tabs の数と同じ。
    ReadOnlyProperty<int>       PageCount;
    Property<bool>              MultiLine;
    Property<bool>              ShowTabs;
    Property<TTabPosition>      TabPosition;
    // 利用者の操作でページが切り替わる前に呼ばれる。
    Property<TTabChangingEvent> OnChanging;

protected:
    explicit TCustomTabControl(no_vcl_obj_t handle);
    ~TCustomTabControl() override = default;

private:
    TTabChangingEvent onChanging_;
    bool              onChangingHooked_ = false;
    static void NO_VCL_CALL ChangingTrampoline(no_vcl_obj_t sender, no_vcl_bool_t* allowChange, void* data);

    static int               GetPageCountImpl(TObject* owner);
    static bool              GetMultiLineImpl(TObject* owner);
    static void              SetMultiLineImpl(TObject* owner, const bool& value);
    static bool              GetShowTabsImpl(TObject* owner);
    static void              SetShowTabsImpl(TObject* owner, const bool& value);
    static TTabPosition      GetTabPositionImpl(TObject* owner);
    static void              SetTabPositionImpl(TObject* owner, const TTabPosition& value);
    static TTabChangingEvent GetOnChangingImpl(TObject* owner);
    static void              SetOnChangingImpl(TObject* owner, const TTabChangingEvent& value);
};

// 単純なタブの切り替え UI(ページはコントロール自身では管理しない)。Tabs/TabIndex/OnChange は
// LCL では TCustomTabControl の protected だが、TTabControl が独自のフィールドで再宣言して published に
// しているため、すべて TTabControl に直接置く(TUpDown と同じ形)。ページ付きのタブは TPageControl。
class TTabControl : public TCustomTabControl
{
public:
    explicit TTabControl(TComponent* AOwner);

    Property<int>          TabIndex;
    Property<TNotifyEvent> OnChange;

    void        TabsAdd(const std::string& text);
    void        TabsClear();
    int         TabsCount() const;
    std::string TabsGetText(int index) const;

protected:
    ~TTabControl() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, void* data);

    static int           GetTabIndexImpl(TObject* owner);
    static void          SetTabIndexImpl(TObject* owner, const int& value);
    static TNotifyEvent  GetOnChangeImpl(TObject* owner);
    static void          SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

class TTabSheet;

// ページ付きのタブ。ページ(TTabSheet)は VCL と同じく、TTabSheet を生成して PageControl を設定するか、
// AddTabSheet で追加する。ページの上のコントロールは、ページを Parent にして置く。
// Pages[Index] は GetPage(Index)(インデックス付きプロパティは Get メソッドで表す、TMenuItem::GetItem と同じ形)。
class TPageControl : public TCustomTabControl
{
public:
    explicit TPageControl(TComponent* AOwner);

    // ページが 1 つも無ければ nullptr。
    Property<TTabSheet*>   ActivePage;
    Property<int>          ActivePageIndex;
    // 表示されているタブの中での位置(TabVisible が false のページは数えない)。
    Property<int>          TabIndex;
    // ページが切り替わった後に呼ばれる。プログラムからの ActivePage・ActivePageIndex の変更では呼ばれないが、
    // TCustomPage::PageIndex でページを並べ替えたときは(表示中のページの位置が変わるため)呼ばれる。
    Property<TNotifyEvent> OnChange;

    TTabSheet* GetPage(int Index) const;
    // ページを末尾に追加する。ページは LCL が内部で生成し、Owner はこのページコントロールになる。
    TTabSheet* AddTabSheet();
    // すべてのページを外して破棄する。破棄は LCL の遅延破棄(Application.ReleaseComponent)で、次にメッセージを
    // 処理したとき(または Owner の破棄時)に行われ、そのときにページのラッパーも delete される。
    void       Clear();
    void       SelectNextPage(bool GoForward);

protected:
    ~TPageControl() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, void* data);

    static TTabSheet*   GetActivePageImpl(TObject* owner);
    static void         SetActivePageImpl(TObject* owner, TTabSheet* const& value);
    static int          GetActivePageIndexImpl(TObject* owner);
    static void         SetActivePageIndexImpl(TObject* owner, const int& value);
    static int          GetTabIndexImpl(TObject* owner);
    static void         SetTabIndexImpl(TObject* owner, const int& value);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

// ページの共通の基底(LCL の TCustomPage。TWinControl の直接の派生)。
class TCustomPage : public TWinControl
{
public:
    // ページの並び順。書き換えるとタブの位置が移動する。
    Property<int>          PageIndex;
    Property<bool>         TabVisible;
    // ページが表示された/隠されたときに呼ばれる。
    Property<TNotifyEvent> OnShow;
    Property<TNotifyEvent> OnHide;

protected:
    explicit TCustomPage(no_vcl_obj_t handle);
    ~TCustomPage() override = default;

private:
    TNotifyEvent onShow_;
    TNotifyEvent onHide_;
    bool         onShowHooked_ = false;
    bool         onHideHooked_ = false;
    static void NO_VCL_CALL ShowTrampoline(no_vcl_obj_t sender, void* data);
    static void NO_VCL_CALL HideTrampoline(no_vcl_obj_t sender, void* data);

    static int          GetPageIndexImpl(TObject* owner);
    static void         SetPageIndexImpl(TObject* owner, const int& value);
    static bool         GetTabVisibleImpl(TObject* owner);
    static void         SetTabVisibleImpl(TObject* owner, const bool& value);
    static TNotifyEvent GetOnShowImpl(TObject* owner);
    static void         SetOnShowImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnHideImpl(TObject* owner);
    static void         SetOnHideImpl(TObject* owner, const TNotifyEvent& value);
};

// TPageControl のページ。タブの文字列は Caption。
class TTabSheet : public TCustomPage
{
public:
    explicit TTabSheet(TComponent* AOwner);

    // 設定するとそのページコントロールの末尾に追加される(nullptr で外す)。
    Property<TPageControl*> PageControl;
    // 表示されているタブの中での位置(TabVisible が false なら -1)。
    ReadOnlyProperty<int>   TabIndex;

protected:
    ~TTabSheet() override = default;

private:
    friend class TComponent;  // WrapExisting から(AddTabSheet 等で LCL が生成したページのラップ)
    explicit TTabSheet(no_vcl_obj_t handle);

    static TPageControl* GetPageControlImpl(TObject* owner);
    static void          SetPageControlImpl(TObject* owner, TPageControl* const& value);
    static int           GetTabIndexImpl(TObject* owner);
};

class TCustomTreeView;
class TTreeView;
class TTreeNodes;

// MoveTo の移動先の指定(LCL / VCL の TNodeAttachMode と同じ値)。
enum TNodeAttachMode { naAdd, naAddFirst, naAddChild, naAddChildFirst, naInsert, naInsertBehind };

// ツリービューのノード。TComponent ではない(LCL でも TPersistent)ため、new/Free() はせず、
// TTreeNodes::Add 等で追加し、Delete() 等で削除する。
// C++ のラッパーは初めて取得したときに作られ、同じノードには常に同じポインタが返る(ポインタ同士を比較してよい)。
// ノードが削除されると(ツリービューの破棄に伴う削除も含め)、OnDeletion の後にラッパーも delete される。
// 削除後にそのポインタへ触れてはならない。
// Items[Index] は GetItem(Index)(インデックス付きプロパティは Get メソッドで表す、TMenuItem::GetItem と同じ形)。
class TTreeNode : public TPersistent
{
public:
    Property<std::string> Text;
    Property<bool>        Expanded;
    Property<bool>        Selected;
    // 子が無くても展開ボタンを表示するとき(子を遅延で追加するとき等)に true にする。
    Property<bool>        HasChildren;
    // 利用者データ(LCL は解釈しない)。
    Property<void*>       Data;

    // 直下の子の数・兄弟の中での位置・深さ(最上位が 0)・上から順に数えた位置。
    ReadOnlyProperty<int>              Count;
    ReadOnlyProperty<int>              Index;
    ReadOnlyProperty<int>              Level;
    ReadOnlyProperty<int>              AbsoluteIndex;
    // 最上位のノードなら nullptr。
    ReadOnlyProperty<TTreeNode*>       Parent;
    ReadOnlyProperty<TCustomTreeView*> TreeView;

    // 以下のノードを返すメンバは、該当するノードが無ければ nullptr を返す。
    TTreeNode* GetItem(int Index) const;
    TTreeNode* GetFirstChild() const;
    TTreeNode* GetLastChild() const;
    TTreeNode* GetNextSibling() const;
    TTreeNode* GetPrevSibling() const;
    // 上から順(子孫を含む)の次/前のノード。
    TTreeNode* GetNext() const;
    TTreeNode* GetPrev() const;
    int        IndexOf(TTreeNode* Node) const;

    void Expand(bool Recurse);
    void Collapse(bool Recurse);
    // このノード(と子孫)を削除する。このラッパーも delete されるため、呼び出し後に触れてはならない。
    void Delete();
    void DeleteChildren();
    // 祖先を展開し、ノードが見えるようにスクロールする。
    void MakeVisible();
    void MoveTo(TTreeNode* Destination, TNodeAttachMode Mode);

private:
    friend class ItemRegistry;
    friend class TTreeNodes;
    friend class TCustomTreeView;
    friend class TTreeView;

    explicit TTreeNode(no_vcl_obj_t handle);
    ~TTreeNode() override = default;

    // ハンドルからラッパーを得る(無ければ作る)。nullptr には nullptr を返す。
    static TTreeNode* Wrap(no_vcl_obj_t handle) { return ItemRegistry::Wrap<TTreeNode>(handle); }

    static std::string      GetTextImpl(TObject* owner);
    static void             SetTextImpl(TObject* owner, const std::string& value);
    static bool             GetExpandedImpl(TObject* owner);
    static void             SetExpandedImpl(TObject* owner, const bool& value);
    static bool             GetSelectedImpl(TObject* owner);
    static void             SetSelectedImpl(TObject* owner, const bool& value);
    static bool             GetHasChildrenImpl(TObject* owner);
    static void             SetHasChildrenImpl(TObject* owner, const bool& value);
    static void*            GetDataImpl(TObject* owner);
    static void             SetDataImpl(TObject* owner, void* const& value);
    static int              GetCountImpl(TObject* owner);
    static int              GetIndexImpl(TObject* owner);
    static int              GetLevelImpl(TObject* owner);
    static int              GetAbsoluteIndexImpl(TObject* owner);
    static TTreeNode*       GetParentImpl(TObject* owner);
    static TCustomTreeView* GetTreeViewImpl(TObject* owner);
};

// ツリービューのノードの一覧(LCL の TTreeNodes)。ツリービューが所有する実体への非所有のビューで、
// TCanvas と同じくツリービューのメンバとして持ち、ツリービューと寿命が一致する(TCustomTreeView::Items で参照する)。
// Sibling/Parent に nullptr を渡すと最上位のノードになる(VCL と同じ)。
class TTreeNodes : public TPersistent
{
public:
    explicit TTreeNodes(no_vcl_obj_t handle);
    ~TTreeNodes() override = default;

    // すべてのノード(子孫を含む)の数。GetItem の Index は、上から順に数えた位置(AbsoluteIndex)。
    ReadOnlyProperty<int> Count;

    TTreeNode* Add(TTreeNode* Sibling, const std::string& S);
    TTreeNode* AddFirst(TTreeNode* Sibling, const std::string& S);
    TTreeNode* AddChild(TTreeNode* Parent, const std::string& S);
    TTreeNode* AddChildFirst(TTreeNode* Parent, const std::string& S);
    // NextNode の前に挿入する。
    TTreeNode* Insert(TTreeNode* NextNode, const std::string& S);
    void       Clear();
    void       Delete(TTreeNode* Node);
    TTreeNode* GetItem(int Index) const;
    TTreeNode* GetFirstNode() const;
    TTreeNode* FindNodeWithText(const std::string& S) const;
    void       BeginUpdate();
    void       EndUpdate();

private:
    static int GetCountImpl(TObject* owner);
};

// ノードを対象とするイベント。
using TTVChangedEvent    = std::function<void(TObject* Sender, TTreeNode* Node)>;
using TTVExpandedEvent   = TTVChangedEvent;
// AllowChange 等には true が入っており、false にすると選択の変更・展開・折りたたみを取りやめる。
using TTVChangingEvent   = std::function<void(TObject* Sender, TTreeNode* Node, bool& AllowChange)>;
using TTVExpandingEvent  = std::function<void(TObject* Sender, TTreeNode* Node, bool& AllowExpansion)>;
using TTVCollapsingEvent = std::function<void(TObject* Sender, TTreeNode* Node, bool& AllowCollapse)>;

// 以下のメンバは LCL の TCustomTreeView の public。
class TCustomTreeView : public TCustomControl
{
public:
    ReadOnlyProperty<TTreeNodes*> Items;
    // 選択されているノード(無ければ nullptr)。
    Property<TTreeNode*>          Selected;

    void       FullExpand();
    void       FullCollapse();
    // ノードを文字列の順に並べ替える。
    bool       AlphaSort();
    // X, Y はクライアント座標。そこにノードが無ければ nullptr。
    TTreeNode* GetNodeAt(int X, int Y) const;

protected:
    explicit TCustomTreeView(no_vcl_obj_t handle);
    ~TCustomTreeView() override = default;

private:
    friend class TTreeNode;  // TTreeNode::TreeView の Getter から FromHandle を使うため

    TTreeNodes items_;

    static TTreeNodes* GetItemsImpl(TObject* owner);
    static TTreeNode*  GetSelectedImpl(TObject* owner);
    static void        SetSelectedImpl(TObject* owner, TTreeNode* const& value);
};

// 以下のメンバは LCL では TCustomTreeView の protected で、TTreeView が published にしている。
class TTreeView : public TCustomTreeView
{
public:
    explicit TTreeView(TComponent* AOwner);

    Property<bool> ReadOnly;
    Property<bool> ShowLines;
    Property<bool> ShowRoot;
    Property<bool> ShowButtons;
    Property<bool> AutoExpand;
    Property<bool> HideSelection;
    Property<bool> RowSelect;

    // 選択が変わった後(Node は選択されたノードで、nullptr もありうる)。
    Property<TTVChangedEvent>    OnChange;
    // 選択が変わる前(Node は新しく選択されるノード)。
    Property<TTVChangingEvent>   OnChanging;
    Property<TTVExpandingEvent>  OnExpanding;
    Property<TTVExpandedEvent>   OnExpanded;
    Property<TTVCollapsingEvent> OnCollapsing;
    Property<TTVExpandedEvent>   OnCollapsed;
    // ノードが削除される直前(Node はまだ有効。ハンドラから戻った後にラッパーが delete される)。
    Property<TTVExpandedEvent>   OnDeletion;

protected:
    ~TTreeView() override = default;

private:
    TTVChangedEvent    onChange_;
    TTVChangingEvent   onChanging_;
    TTVExpandingEvent  onExpanding_;
    TTVExpandedEvent   onExpanded_;
    TTVCollapsingEvent onCollapsing_;
    TTVExpandedEvent   onCollapsed_;
    TTVExpandedEvent   onDeletion_;
    bool onChangeHooked_    = false;
    bool onChangingHooked_  = false;
    bool onExpandingHooked_ = false;
    bool onExpandedHooked_  = false;
    bool onCollapsingHooked_ = false;
    bool onCollapsedHooked_ = false;
    bool onDeletionHooked_  = false;

    template<typename Event>
    static void DispatchNode(no_vcl_obj_t sender, no_vcl_obj_t node, Event TTreeView::*slot);
    template<typename Event>
    static void DispatchNodeAllow(no_vcl_obj_t sender, no_vcl_obj_t node, no_vcl_bool_t* allow, Event TTreeView::*slot);

    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, no_vcl_obj_t node, void* data);
    static void NO_VCL_CALL ChangingTrampoline(no_vcl_obj_t sender, no_vcl_obj_t node, no_vcl_bool_t* allow, void* data);
    static void NO_VCL_CALL ExpandingTrampoline(no_vcl_obj_t sender, no_vcl_obj_t node, no_vcl_bool_t* allow, void* data);
    static void NO_VCL_CALL ExpandedTrampoline(no_vcl_obj_t sender, no_vcl_obj_t node, void* data);
    static void NO_VCL_CALL CollapsingTrampoline(no_vcl_obj_t sender, no_vcl_obj_t node, no_vcl_bool_t* allow, void* data);
    static void NO_VCL_CALL CollapsedTrampoline(no_vcl_obj_t sender, no_vcl_obj_t node, void* data);
    static void NO_VCL_CALL DeletionTrampoline(no_vcl_obj_t sender, no_vcl_obj_t node, void* data);

    static bool GetReadOnlyImpl(TObject* owner);
    static void SetReadOnlyImpl(TObject* owner, const bool& value);
    static bool GetShowLinesImpl(TObject* owner);
    static void SetShowLinesImpl(TObject* owner, const bool& value);
    static bool GetShowRootImpl(TObject* owner);
    static void SetShowRootImpl(TObject* owner, const bool& value);
    static bool GetShowButtonsImpl(TObject* owner);
    static void SetShowButtonsImpl(TObject* owner, const bool& value);
    static bool GetAutoExpandImpl(TObject* owner);
    static void SetAutoExpandImpl(TObject* owner, const bool& value);
    static bool GetHideSelectionImpl(TObject* owner);
    static void SetHideSelectionImpl(TObject* owner, const bool& value);
    static bool GetRowSelectImpl(TObject* owner);
    static void SetRowSelectImpl(TObject* owner, const bool& value);

    static TTVChangedEvent    GetOnChangeImpl(TObject* owner);
    static void               SetOnChangeImpl(TObject* owner, const TTVChangedEvent& value);
    static TTVChangingEvent   GetOnChangingImpl(TObject* owner);
    static void               SetOnChangingImpl(TObject* owner, const TTVChangingEvent& value);
    static TTVExpandingEvent  GetOnExpandingImpl(TObject* owner);
    static void               SetOnExpandingImpl(TObject* owner, const TTVExpandingEvent& value);
    static TTVExpandedEvent   GetOnExpandedImpl(TObject* owner);
    static void               SetOnExpandedImpl(TObject* owner, const TTVExpandedEvent& value);
    static TTVCollapsingEvent GetOnCollapsingImpl(TObject* owner);
    static void               SetOnCollapsingImpl(TObject* owner, const TTVCollapsingEvent& value);
    static TTVExpandedEvent   GetOnCollapsedImpl(TObject* owner);
    static void               SetOnCollapsedImpl(TObject* owner, const TTVExpandedEvent& value);
    static TTVExpandedEvent   GetOnDeletionImpl(TObject* owner);
    static void               SetOnDeletionImpl(TObject* owner, const TTVExpandedEvent& value);
};

/* ---------------- ListView ---------------- */

// 表示形式・並べ替え・列の文字の寄せ方・OnChange の変更の種類(LCL / VCL と同じ値)。
enum TViewStyle     { vsIcon, vsSmallIcon, vsList, vsReport };
enum TSortType      { stNone, stData, stText, stBoth };
enum TSortDirection { sdAscending, sdDescending };
enum TAlignment     { taLeftJustify, taRightJustify, taCenter };
enum TItemChange    { ctText, ctImage, ctState };

class TCustomListView;
class TListView;

// リストビューの項目。TTreeNode と同じく TComponent ではないため、new/Free() はせず TListItems::Add 等で追加し、
// Delete() 等で削除する。同じ項目には常に同じポインタが返り、項目が削除されると(リストビューの破棄に伴う削除も含め)
// OnDeletion の後にラッパーも delete される。
// Caption は 1 列目、SubItems は 2 列目以降の文字列(ViewStyle が vsReport のときに表示される)。
// SubItems(TStrings)は、TComboBox の Items と同じく SubItemsAdd 等のメンバ関数で操作する。
class TListItem : public TPersistent
{
public:
    Property<std::string> Caption;
    Property<bool>        Checked;
    Property<bool>        Selected;
    Property<bool>        Focused;
    // 利用者データ(LCL は解釈しない)。
    Property<void*>       Data;

    ReadOnlyProperty<int>              Index;
    ReadOnlyProperty<TCustomListView*> ListView;

    void        SubItemsAdd(const std::string& text);
    void        SubItemsClear();
    int         SubItemsCount() const;
    std::string SubItemsGetText(int index) const;
    void        SubItemsSetText(int index, const std::string& text);

    // この項目を削除する。このラッパーも delete されるため、呼び出し後に触れてはならない。
    void Delete();
    void MakeVisible(bool PartialOK);

private:
    friend class ItemRegistry;
    friend class TListItems;
    friend class TCustomListView;
    friend class TListView;

    explicit TListItem(no_vcl_obj_t handle);
    ~TListItem() override = default;
    static TListItem* Wrap(no_vcl_obj_t handle) { return ItemRegistry::Wrap<TListItem>(handle); }

    static std::string      GetCaptionImpl(TObject* owner);
    static void             SetCaptionImpl(TObject* owner, const std::string& value);
    static bool             GetCheckedImpl(TObject* owner);
    static void             SetCheckedImpl(TObject* owner, const bool& value);
    static bool             GetSelectedImpl(TObject* owner);
    static void             SetSelectedImpl(TObject* owner, const bool& value);
    static bool             GetFocusedImpl(TObject* owner);
    static void             SetFocusedImpl(TObject* owner, const bool& value);
    static void*            GetDataImpl(TObject* owner);
    static void             SetDataImpl(TObject* owner, void* const& value);
    static int              GetIndexImpl(TObject* owner);
    static TCustomListView* GetListViewImpl(TObject* owner);
};

// リストビューの項目の一覧(LCL の TListItems)。TTreeNodes と同じく、リストビューの値メンバとして持つ非所有のビュー。
// Items[Index] は GetItem(Index)。
class TListItems : public TPersistent
{
public:
    explicit TListItems(no_vcl_obj_t handle);
    ~TListItems() override = default;

    ReadOnlyProperty<int> Count;

    // 末尾に(Insert は Index の位置に)空の項目を追加して返す(Caption 等はその後で設定する。VCL と同じ)。
    TListItem* Add();
    TListItem* Insert(int Index);
    void       Delete(int Index);
    void       Clear();
    TListItem* GetItem(int Index) const;
    int        IndexOf(TListItem* Item) const;
    // StartIndex の次(Inclusive なら StartIndex から)から Caption を探す。Partial なら前方一致、Wrap なら末尾から先頭へ続けて探す。
    TListItem* FindCaption(int StartIndex, const std::string& Value, bool Partial, bool Inclusive, bool Wrap) const;
    void       Exchange(int Index1, int Index2);
    void       Move(int FromIndex, int ToIndex);
    void       BeginUpdate();
    void       EndUpdate();

private:
    static int GetCountImpl(TObject* owner);
};

// リストビューの列(LCL の TListColumn。TCollectionItem)。項目と同じく同じ列には常に同じポインタが返る。
// 列のラッパーは、TListColumns::Delete・Clear で削除したときと、リストビューの破棄のときに delete される。
class TListColumn : public TPersistent
{
public:
    Property<std::string> Caption;
    Property<int>         Width;
    Property<TAlignment>  Alignment;
    Property<bool>        AutoSize;
    Property<bool>        Visible;
    // 列の並び順。書き換えると列が移動する。
    Property<int>         Index;

private:
    friend class ItemRegistry;
    friend class TListColumns;
    friend class TListView;

    explicit TListColumn(no_vcl_obj_t handle);
    ~TListColumn() override = default;
    static TListColumn* Wrap(no_vcl_obj_t handle) { return ItemRegistry::Wrap<TListColumn>(handle); }

    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static TAlignment  GetAlignmentImpl(TObject* owner);
    static void        SetAlignmentImpl(TObject* owner, const TAlignment& value);
    static bool        GetAutoSizeImpl(TObject* owner);
    static void        SetAutoSizeImpl(TObject* owner, const bool& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static int         GetIndexImpl(TObject* owner);
    static void        SetIndexImpl(TObject* owner, const int& value);
};

// リストビューの列の一覧(LCL の TListColumns)。リストビューの値メンバとして持つ非所有のビュー。Items[Index] は GetItem(Index)。
class TListColumns : public TPersistent
{
public:
    explicit TListColumns(no_vcl_obj_t handle);
    ~TListColumns() override = default;

    ReadOnlyProperty<int> Count;

    TListColumn* Add();
    TListColumn* GetItem(int Index) const;
    // 列を削除する(列のラッパーも delete される)。
    void         Delete(int Index);
    void         Clear();

private:
    static int GetCountImpl(TObject* owner);
};

// 項目・列を対象とするイベント。
using TLVDeletedEvent     = std::function<void(TObject* Sender, TListItem* Item)>;
using TLVCheckedItemEvent = TLVDeletedEvent;
using TLVSelectItemEvent  = std::function<void(TObject* Sender, TListItem* Item, bool Selected)>;
using TLVChangeEvent      = std::function<void(TObject* Sender, TListItem* Item, TItemChange Change)>;
using TLVColumnClickEvent = std::function<void(TObject* Sender, TListColumn* Column)>;

// 以下のメンバは LCL の TCustomListView の public。
class TCustomListView : public TWinControl
{
public:
    ReadOnlyProperty<TListItems*> Items;
    // 選択されている項目(MultiSelect なら最初の 1 つ。無ければ nullptr)と、その位置(無ければ -1)。
    // 表示前(フォームのコンストラクタ等)に設定しても選択される(LCL 単体では選択されないため DLL 側で補っている)。
    Property<TListItem*>          Selected;
    Property<int>                 ItemIndex;
    ReadOnlyProperty<int>         SelCount;
    Property<bool>                Checkboxes;
    Property<bool>                GridLines;
    Property<bool>                MultiSelect;
    Property<bool>                ReadOnly;
    Property<bool>                RowSelect;

    // すべての項目を削除する(列は残る)。
    void       Clear();
    void       BeginUpdate();
    void       EndUpdate();
    // X, Y はクライアント座標。そこに項目が無ければ nullptr。
    TListItem* GetItemAt(int X, int Y) const;
    void       ClearSelection();
    void       SelectAll();

protected:
    explicit TCustomListView(no_vcl_obj_t handle);
    ~TCustomListView() override = default;

private:
    friend class TListItem;  // TListItem::ListView の Getter から FromHandle を使うため

    TListItems items_;

    static TListItems* GetItemsImpl(TObject* owner);
    static TListItem*  GetSelectedImpl(TObject* owner);
    static void        SetSelectedImpl(TObject* owner, TListItem* const& value);
    static int         GetItemIndexImpl(TObject* owner);
    static void        SetItemIndexImpl(TObject* owner, const int& value);
    static int         GetSelCountImpl(TObject* owner);
    static bool        GetCheckboxesImpl(TObject* owner);
    static void        SetCheckboxesImpl(TObject* owner, const bool& value);
    static bool        GetGridLinesImpl(TObject* owner);
    static void        SetGridLinesImpl(TObject* owner, const bool& value);
    static bool        GetMultiSelectImpl(TObject* owner);
    static void        SetMultiSelectImpl(TObject* owner, const bool& value);
    static bool        GetReadOnlyImpl(TObject* owner);
    static void        SetReadOnlyImpl(TObject* owner, const bool& value);
    static bool        GetRowSelectImpl(TObject* owner);
    static void        SetRowSelectImpl(TObject* owner, const bool& value);
};

// 以下のメンバは LCL では TCustomListView の protected で、TListView が published にしている。
class TListView : public TCustomListView
{
public:
    explicit TListView(TComponent* AOwner);

    ReadOnlyProperty<TListColumns*> Columns;
    // 列見出しと SubItems が表示されるのは vsReport のとき。
    Property<TViewStyle>     ViewStyle;
    Property<bool>           HideSelection;
    // SortType が stText のとき、SortColumn の列(0 が Caption の列)の文字列の順に並ぶ。
    // SortColumn が既定の -1 のままでは並べ替えない(LCL の仕様。先に SortColumn を設定する)。
    Property<TSortType>      SortType;
    Property<int>            SortColumn;
    Property<TSortDirection> SortDirection;

    // 項目の選択状態が変わったとき。
    Property<TLVSelectItemEvent>  OnSelectItem;
    // 項目が変わったとき(Change は変更の種類)。
    Property<TLVChangeEvent>      OnChange;
    // 項目が削除される直前(Item はまだ有効。ハンドラから戻った後にラッパーが delete される)。
    Property<TLVDeletedEvent>     OnDeletion;
    // チェックボックス(Checkboxes)が切り替わったとき。
    Property<TLVCheckedItemEvent> OnItemChecked;
    // 列見出しがクリックされたとき。
    Property<TLVColumnClickEvent> OnColumnClick;

protected:
    ~TListView() override = default;

private:
    TListColumns columns_;

    TLVSelectItemEvent  onSelectItem_;
    TLVChangeEvent      onChange_;
    TLVDeletedEvent     onDeletion_;
    TLVCheckedItemEvent onItemChecked_;
    TLVColumnClickEvent onColumnClick_;
    bool onSelectItemHooked_  = false;
    bool onChangeHooked_      = false;
    bool onDeletionHooked_    = false;
    bool onItemCheckedHooked_ = false;
    bool onColumnClickHooked_ = false;

    static void NO_VCL_CALL SelectItemTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, no_vcl_int_t selected, void* data);
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, no_vcl_int_t change, void* data);
    static void NO_VCL_CALL DeletionTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, void* data);
    static void NO_VCL_CALL ItemCheckedTrampoline(no_vcl_obj_t sender, no_vcl_obj_t item, void* data);
    static void NO_VCL_CALL ColumnClickTrampoline(no_vcl_obj_t sender, no_vcl_obj_t column, void* data);

    static TListColumns*  GetColumnsImpl(TObject* owner);
    static TViewStyle     GetViewStyleImpl(TObject* owner);
    static void           SetViewStyleImpl(TObject* owner, const TViewStyle& value);
    static bool           GetHideSelectionImpl(TObject* owner);
    static void           SetHideSelectionImpl(TObject* owner, const bool& value);
    static TSortType      GetSortTypeImpl(TObject* owner);
    static void           SetSortTypeImpl(TObject* owner, const TSortType& value);
    static int            GetSortColumnImpl(TObject* owner);
    static void           SetSortColumnImpl(TObject* owner, const int& value);
    static TSortDirection GetSortDirectionImpl(TObject* owner);
    static void           SetSortDirectionImpl(TObject* owner, const TSortDirection& value);

    static TLVSelectItemEvent  GetOnSelectItemImpl(TObject* owner);
    static void                SetOnSelectItemImpl(TObject* owner, const TLVSelectItemEvent& value);
    static TLVChangeEvent      GetOnChangeImpl(TObject* owner);
    static void                SetOnChangeImpl(TObject* owner, const TLVChangeEvent& value);
    static TLVDeletedEvent     GetOnDeletionImpl(TObject* owner);
    static void                SetOnDeletionImpl(TObject* owner, const TLVDeletedEvent& value);
    static TLVCheckedItemEvent GetOnItemCheckedImpl(TObject* owner);
    static void                SetOnItemCheckedImpl(TObject* owner, const TLVCheckedItemEvent& value);
    static TLVColumnClickEvent GetOnColumnClickImpl(TObject* owner);
    static void                SetOnColumnClickImpl(TObject* owner, const TLVColumnClickEvent& value);
};

// Splitter が寄せる辺(LCL の TAnchorKind と同じ値。VCL には無い)と、ドラッグ中の表示のしかた。
enum TAnchorKind  { akTop, akLeft, akRight, akBottom };
enum TResizeStyle { rsLine, rsNone, rsPattern, rsUpdate };

// 同じ Align を持つ直前のコントロール(alLeft なら、自分より左にある alLeft のコントロール)の幅・高さを
// ドラッグで変える区切りバー。Align が alLeft/alRight なら縦、alTop/alBottom なら横のバーになる(既定は alLeft)。
// メンバはすべて LCL の TCustomSplitter の public。OnCanResize/OnCanOffset(var 引数 2 つの独自のイベント形)は
// 今回は未対応。SplitterPosition は LCL ではプロパティではなくメソッドの組のため、そのまま Get/Set メソッドにする。
class TCustomSplitter : public TCustomControl
{
public:
    Property<bool>         AutoSnap;
    Property<bool>         Beveled;
    Property<int>          MinSize;
    Property<TAnchorKind>  ResizeAnchor;
    Property<TResizeStyle> ResizeStyle;
    // マウスでのドラッグが終わったときに呼ばれる(SetSplitterPosition では呼ばれない)。
    Property<TNotifyEvent> OnMoved;

    // 縦のバーなら Left、横のバーなら Top にあたる(親のクライアント座標)。
    int  GetSplitterPosition() const;
    void SetSplitterPosition(int NewPosition);

protected:
    explicit TCustomSplitter(no_vcl_obj_t handle);
    ~TCustomSplitter() override = default;

private:
    TNotifyEvent onMoved_;
    bool         onMovedHooked_ = false;
    static void NO_VCL_CALL MovedTrampoline(no_vcl_obj_t sender, void* data);

    static bool         GetAutoSnapImpl(TObject* owner);
    static void         SetAutoSnapImpl(TObject* owner, const bool& value);
    static bool         GetBeveledImpl(TObject* owner);
    static void         SetBeveledImpl(TObject* owner, const bool& value);
    static int          GetMinSizeImpl(TObject* owner);
    static void         SetMinSizeImpl(TObject* owner, const int& value);
    static TAnchorKind  GetResizeAnchorImpl(TObject* owner);
    static void         SetResizeAnchorImpl(TObject* owner, const TAnchorKind& value);
    static TResizeStyle GetResizeStyleImpl(TObject* owner);
    static void         SetResizeStyleImpl(TObject* owner, const TResizeStyle& value);
    static TNotifyEvent GetOnMovedImpl(TObject* owner);
    static void         SetOnMovedImpl(TObject* owner, const TNotifyEvent& value);
};

class TSplitter : public TCustomSplitter
{
public:
    explicit TSplitter(TComponent* AOwner);

protected:
    ~TSplitter() override = default;
};

class TCustomMemo : public TCustomEdit
{
public:
    Property<int> ScrollBars;

    void        LinesAdd(const std::string& text);
    void        LinesClear();
    int         LinesCount() const;
    std::string LinesGetText(int index) const;

protected:
    explicit TCustomMemo(no_vcl_obj_t handle);
    ~TCustomMemo() override = default;

private:
    static int  GetScrollBarsImpl(TObject* owner);
    static void SetScrollBarsImpl(TObject* owner, const int& value);
};

class TMemo : public TCustomMemo
{
public:
    explicit TMemo(TComponent* AOwner);

protected:
    ~TMemo() override = default;
};

/* ---------------- ComboBox / ListBox ---------------- */

class TCustomComboBox : public TWinControl
{
public:
    using TControl::Text;
    Property<int> ItemIndex;

    void        ItemsAdd(const std::string& text);
    void        ItemsClear();
    int         ItemsCount() const;
    std::string ItemsGetText(int index) const;

protected:
    explicit TCustomComboBox(no_vcl_obj_t handle);
    ~TCustomComboBox() override = default;

private:
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);
};

class TComboBox : public TCustomComboBox
{
public:
    // LCL では TCustomComboBox の protected で、公開しているのは TComboBox だけ。
    Property<TNotifyEvent> OnChange;

    explicit TComboBox(TComponent* AOwner);

protected:
    ~TComboBox() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void NO_VCL_CALL ChangeTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

// 利用者による選択の変更(マウス・キー操作とも)では OnClick が呼ばれる(VCL と同じ)。
// プログラムからの ItemIndex の変更では呼ばれない。
class TCustomListBox : public TWinControl
{
public:
    Property<int> ItemIndex;

    void        ItemsAdd(const std::string& text);
    void        ItemsClear();
    int         ItemsCount() const;
    std::string ItemsGetText(int index) const;

protected:
    explicit TCustomListBox(no_vcl_obj_t handle);
    ~TCustomListBox() override = default;

private:
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);
};

class TListBox : public TCustomListBox
{
public:
    explicit TListBox(TComponent* AOwner);

protected:
    ~TListBox() override = default;
};

// 各項目にチェックボックスを持つリストボックス。Items は基底 TCustomListBox のものをそのまま使う。
class TCustomCheckListBox : public TCustomListBox
{
public:
    Property<TNotifyEvent> OnClickCheck;

    bool GetChecked(int index) const;
    void SetChecked(int index, bool value);

protected:
    explicit TCustomCheckListBox(no_vcl_obj_t handle);
    ~TCustomCheckListBox() override = default;

private:
    TNotifyEvent onClickCheck_;
    bool         onClickCheckHooked_ = false;
    static void NO_VCL_CALL ClickCheckTrampoline(no_vcl_obj_t sender, void* data);

    static TNotifyEvent GetOnClickCheckImpl(TObject* owner);
    static void         SetOnClickCheckImpl(TObject* owner, const TNotifyEvent& value);
};

class TCheckListBox : public TCustomCheckListBox
{
public:
    explicit TCheckListBox(TComponent* AOwner);

protected:
    ~TCheckListBox() override = default;
};

// 枠線付きの表示専用テキスト(TLabel と異なりウィンドウを持つ)。
enum TStaticBorderStyle { sbsNone, sbsSingle, sbsSunken };

class TCustomStaticText : public TWinControl
{
public:
    Property<TStaticBorderStyle> BorderStyle;

protected:
    explicit TCustomStaticText(no_vcl_obj_t handle);
    ~TCustomStaticText() override = default;

private:
    static TStaticBorderStyle GetBorderStyleImpl(TObject* owner);
    static void                SetBorderStyleImpl(TObject* owner, const TStaticBorderStyle& value);
};

class TStaticText : public TCustomStaticText
{
public:
    explicit TStaticText(TComponent* AOwner);

protected:
    ~TStaticText() override = default;
};

// ステータス行。LCL では中間の TCustomStatusBar が無く、TWinControl の直接の派生。
// Panels(複数区画のコレクション)は今回未対応で、SimpleText/SimplePanel のみ。
// 他のコントロールと同じく、フォームのコンストラクタの中で生成・配置してよい
// (LCL の Win32 実装が DLL で失敗する問題は DLL 側で回避済み。docs/adr/0015-... を参照)。
class TStatusBar : public TWinControl
{
public:
    explicit TStatusBar(TComponent* AOwner);

    Property<std::string> SimpleText;
    Property<bool>         SimplePanel;

protected:
    ~TStatusBar() override = default;

private:
    static std::string GetSimpleTextImpl(TObject* owner);
    static void         SetSimpleTextImpl(TObject* owner, const std::string& value);
    static bool         GetSimplePanelImpl(TObject* owner);
    static void          SetSimplePanelImpl(TObject* owner, const bool& value);
};

/* ---------------- Canvas ---------------- */

// TPen/TBrush/TFont/TCanvas は LCL でも TComponent ではなく TPersistent の派生であり、
// Canvas を持つコントロールが内部で保持するオブジェクトへの非所有(non-owning)ラッパー。
// 自前で Create/Destroy は行わない(取得元のコントロールが破棄されれば一緒に破棄される)。
// TPaintBox::Canvas のように値メンバとして持つため、これらのデストラクタは public にしている。

class TPen : public TPersistent
{
public:
    Property<TColor> Color;
    Property<int>    Width;

    explicit TPen(no_vcl_obj_t handle);
    ~TPen() override = default;

private:
    static TColor GetColorImpl(TObject* owner);
    static void   SetColorImpl(TObject* owner, const TColor& value);
    static int    GetWidthImpl(TObject* owner);
    static void   SetWidthImpl(TObject* owner, const int& value);
};

class TBrush : public TPersistent
{
public:
    Property<TColor> Color;

    explicit TBrush(no_vcl_obj_t handle);
    ~TBrush() override = default;

private:
    static TColor GetColorImpl(TObject* owner);
    static void   SetColorImpl(TObject* owner, const TColor& value);
};

class TFont : public TPersistent
{
public:
    Property<std::string> Name;
    Property<int>         Size;
    Property<TColor>      Color;

    explicit TFont(no_vcl_obj_t handle);
    ~TFont() override = default;

private:
    static std::string GetNameImpl(TObject* owner);
    static void        SetNameImpl(TObject* owner, const std::string& value);
    static int         GetSizeImpl(TObject* owner);
    static void        SetSizeImpl(TObject* owner, const int& value);
    static TColor      GetColorImpl(TObject* owner);
    static void        SetColorImpl(TObject* owner, const TColor& value);
};

class TCanvas : public TPersistent
{
public:
    TPen   Pen;
    TBrush Brush;
    TFont  Font;

    explicit TCanvas(no_vcl_obj_t handle);
    ~TCanvas() override = default;

    void MoveTo(int x, int y);
    void LineTo(int x, int y);
    void Rectangle(int x1, int y1, int x2, int y2);
    void Ellipse(int x1, int y1, int x2, int y2);
    void TextOut(int x, int y, const std::string& text);
};

// 矩形・楕円等の図形を描画する表示専用コントロール。Pen/Brush は TCanvas と同じく、
// コントロールが所有する実体への非所有のビュー(コントロールと寿命が一致する)。
enum TShapeType
{
    stRectangle, stSquare, stRoundRect, stRoundSquare,
    stEllipse, stCircle, stSquaredDiamond, stDiamond,
    stTriangle, stTriangleLeft, stTriangleRight, stTriangleDown,
    stStar, stStarDown, stPolygon
};

class TCustomShape : public TGraphicControl
{
public:
    TPen   Pen;
    TBrush Brush;
    Property<TShapeType> Shape;

protected:
    explicit TCustomShape(no_vcl_obj_t handle);
    ~TCustomShape() override = default;

private:
    static TShapeType GetShapeImpl(TObject* owner);
    static void        SetShapeImpl(TObject* owner, const TShapeType& value);
};

class TShape : public TCustomShape
{
public:
    explicit TShape(TComponent* AOwner);

protected:
    ~TShape() override = default;
};

// クリックで押し込まれた状態を保つ(GroupIndex でラジオボタン風のグループも作れる)グラフィックボタン。
// Down/GroupIndex/Flat/AllowAllUp はいずれも LCL では public。Caption/OnClick は TControl から共有する。
// Glyph(ビットマップ)は未対応。
class TCustomSpeedButton : public TGraphicControl
{
public:
    Property<bool> Down;
    Property<int>  GroupIndex;
    Property<bool> Flat;
    Property<bool> AllowAllUp;

protected:
    explicit TCustomSpeedButton(no_vcl_obj_t handle);
    ~TCustomSpeedButton() override = default;

private:
    static bool GetDownImpl(TObject* owner);
    static void SetDownImpl(TObject* owner, const bool& value);
    static int  GetGroupIndexImpl(TObject* owner);
    static void SetGroupIndexImpl(TObject* owner, const int& value);
    static bool GetFlatImpl(TObject* owner);
    static void SetFlatImpl(TObject* owner, const bool& value);
    static bool GetAllowAllUpImpl(TObject* owner);
    static void SetAllowAllUpImpl(TObject* owner, const bool& value);
};

class TSpeedButton : public TCustomSpeedButton
{
public:
    explicit TSpeedButton(TComponent* AOwner);

protected:
    ~TSpeedButton() override = default;
};

class TPaintBox : public TGraphicControl
{
public:
    TCanvas                Canvas;
    Property<TNotifyEvent> OnPaint;

    explicit TPaintBox(TComponent* AOwner);

protected:
    ~TPaintBox() override = default;

private:
    TNotifyEvent onPaint_;
    bool         onPaintHooked_ = false;
    static void NO_VCL_CALL PaintTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnPaintImpl(TObject* owner);
    static void         SetOnPaintImpl(TObject* owner, const TNotifyEvent& value);
};

/* ---------------- Grid ---------------- */

// 矩形(VCL の TRect と同じく Left/Top/Right/Bottom)。グリッドの選択範囲(TGridRect)にも使う(Left/Right が列、Top/Bottom が行)。
struct TRect
{
    int Left;
    int Top;
    int Right;
    int Bottom;
};
using TGridRect = TRect;

// グリッドの Options(LCL の TGridOptions)。TShiftState と同じく、ビットを OR した集合として扱う。
using TGridOptions = unsigned int;
const TGridOptions goFixedVertLine               = 1u << 0;
const TGridOptions goFixedHorzLine               = 1u << 1;
const TGridOptions goVertLine                    = 1u << 2;
const TGridOptions goHorzLine                    = 1u << 3;
const TGridOptions goRangeSelect                 = 1u << 4;
const TGridOptions goDrawFocusSelected           = 1u << 5;
const TGridOptions goRowSizing                   = 1u << 6;
const TGridOptions goColSizing                   = 1u << 7;
const TGridOptions goRowMoving                   = 1u << 8;
const TGridOptions goColMoving                   = 1u << 9;
const TGridOptions goEditing                     = 1u << 10;
const TGridOptions goAutoAddRows                 = 1u << 11;
const TGridOptions goTabs                        = 1u << 12;
const TGridOptions goRowSelect                   = 1u << 13;
const TGridOptions goAlwaysShowEditor            = 1u << 14;
const TGridOptions goThumbTracking               = 1u << 15;
const TGridOptions goColSpanning                 = 1u << 16;
const TGridOptions goRelaxedRowSelect            = 1u << 17;
const TGridOptions goDblClickAutoSize            = 1u << 18;
const TGridOptions goSmoothScroll                = 1u << 19;
const TGridOptions goFixedRowNumbering           = 1u << 20;
const TGridOptions goScrollKeepVisible           = 1u << 21;
const TGridOptions goHeaderHotTracking           = 1u << 22;
const TGridOptions goHeaderPushedLook            = 1u << 23;
const TGridOptions goSelectionActive             = 1u << 24;
const TGridOptions goFixedColSizing              = 1u << 25;
const TGridOptions goDontScrollPartCell          = 1u << 26;
const TGridOptions goCellHints                   = 1u << 27;
const TGridOptions goTruncCellHints              = 1u << 28;
const TGridOptions goCellEllipsis                = 1u << 29;
const TGridOptions goAutoAddRowsSkipContentCheck = 1u << 30;
const TGridOptions goRowHighlight                = 1u << 31;

// OnDrawCell の AState(LCL の TGridDrawState)。
using TGridDrawState = unsigned int;
const TGridDrawState gdSelected     = 0x01;
const TGridDrawState gdFocused      = 0x02;
const TGridDrawState gdFixed        = 0x04;
const TGridDrawState gdHot          = 0x08;
const TGridDrawState gdPushed       = 0x10;
const TGridDrawState gdRowHighlight = 0x20;

// セルを描画するとき(ARect はセルのクライアント座標での矩形。描画は TCustomDrawGrid::Canvas に行う)。
using TOnDrawCell        = std::function<void(TObject* Sender, int ACol, int ARow, TRect ARect, TGridDrawState AState)>;
// セルが選択される前(CanSelect を false にすると選択させない)。
using TOnSelectCellEvent = std::function<void(TObject* Sender, int ACol, int ARow, bool& CanSelect)>;
// セルが選択された後。
using TOnSelectEvent     = std::function<void(TObject* Sender, int ACol, int ARow)>;
// 見出し(固定行・固定列)がクリックされたとき(IsColumn は列見出しなら true)。
using THdrEvent          = std::function<void(TObject* Sender, bool IsColumn, int Index)>;

// グリッドの共通の基底。以下のメンバは LCL の TCustomGrid の public。
// セルは(列, 行)の位置で指定する(0 始まり。固定行・固定列を含む)。
class TCustomGrid : public TCustomControl
{
public:
    void  BeginUpdate();
    void  EndUpdate();
    // すべての行・列を削除する(ColCount・RowCount が 0 になる)。セルの文字列だけを消すのは TCustomStringGrid::Clean。
    void  Clear();
    // セルのクライアント座標での矩形。
    TRect CellRect(int ACol, int ARow) const;
    // クライアント座標 X, Y にあるセル。セルの外なら -1。
    void  MouseToCell(int X, int Y, int& ACol, int& ARow) const;

protected:
    explicit TCustomGrid(no_vcl_obj_t handle) : TCustomControl(handle) {}
    ~TCustomGrid() override = default;
};

// 以下のメンバは LCL では TCustomGrid の protected で、TCustomDrawGrid が public にしている。
class TCustomDrawGrid : public TCustomGrid
{
public:
    // OnDrawCell の中で描画する先。グリッドが所有する実体への非所有のビュー(TPaintBox::Canvas と同じ)。
    TCanvas Canvas;

    Property<int>          ColCount;
    Property<int>          RowCount;
    // 固定列・固定行(見出し)の数。既定は 1。
    Property<int>          FixedCols;
    Property<int>          FixedRows;
    // 現在のセル(フォーカスのあるセル)の列・行。
    Property<int>          Col;
    Property<int>          Row;
    Property<int>          DefaultColWidth;
    Property<int>          DefaultRowHeight;
    Property<TGridOptions> Options;
    // 選択範囲(単一のセルなら Left = Right、Top = Bottom)。
    Property<TGridRect>    Selection;
    // スクロール位置(表示されている最初の列・行)。
    Property<int>          LeftCol;
    Property<int>          TopRow;
    // false にすると、OnDrawCell の前にセルの既定の描画(背景・文字列)を行わない。
    Property<bool>         DefaultDrawing;
    Property<TColor>       FixedColor;
    // セルの編集中か(goEditing のとき)。true を設定すると現在のセルの編集を始める。
    Property<bool>         EditorMode;

    Property<TOnDrawCell>        OnDrawCell;
    Property<TOnSelectCellEvent> OnSelectCell;
    Property<TOnSelectEvent>     OnSelection;
    Property<THdrEvent>          OnHeaderClick;

    // 列ごとの幅・行ごとの高さ(Grid->ColWidths[0] = 80;)。
    IndexedProperty<int> ColWidths;
    IndexedProperty<int> RowHeights;

    void InsertColRow(bool IsColumn, int Index);
    void DeleteColRow(bool IsColumn, int Index);
    void MoveColRow(bool IsColumn, int FromIndex, int ToIndex);
    // IsColumn が true なら、列 Index の値で行を並べ替える(固定行は除く)。false なら行 Index の値で列を並べ替える。
    void SortColRow(bool IsColumn, int Index);

protected:
    explicit TCustomDrawGrid(no_vcl_obj_t handle);
    ~TCustomDrawGrid() override = default;

private:
    TOnDrawCell        onDrawCell_;
    TOnSelectCellEvent onSelectCell_;
    TOnSelectEvent     onSelection_;
    THdrEvent          onHeaderClick_;
    bool onDrawCellHooked_    = false;
    bool onSelectCellHooked_  = false;
    bool onSelectionHooked_   = false;
    bool onHeaderClickHooked_ = false;

    static void NO_VCL_CALL DrawCellTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row,
                                               no_vcl_int_t left, no_vcl_int_t top, no_vcl_int_t right, no_vcl_int_t bottom,
                                               no_vcl_uint_t state, void* data);
    static void NO_VCL_CALL SelectCellTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row, no_vcl_bool_t* canSelect, void* data);
    static void NO_VCL_CALL SelectionTrampoline(no_vcl_obj_t sender, no_vcl_int_t col, no_vcl_int_t row, void* data);
    static void NO_VCL_CALL HeaderClickTrampoline(no_vcl_obj_t sender, no_vcl_int_t isColumn, no_vcl_int_t index, void* data);

    static int          GetColCountImpl(TObject* owner);
    static void         SetColCountImpl(TObject* owner, const int& value);
    static int          GetRowCountImpl(TObject* owner);
    static void         SetRowCountImpl(TObject* owner, const int& value);
    static int          GetFixedColsImpl(TObject* owner);
    static void         SetFixedColsImpl(TObject* owner, const int& value);
    static int          GetFixedRowsImpl(TObject* owner);
    static void         SetFixedRowsImpl(TObject* owner, const int& value);
    static int          GetColImpl(TObject* owner);
    static void         SetColImpl(TObject* owner, const int& value);
    static int          GetRowImpl(TObject* owner);
    static void         SetRowImpl(TObject* owner, const int& value);
    static int          GetDefaultColWidthImpl(TObject* owner);
    static void         SetDefaultColWidthImpl(TObject* owner, const int& value);
    static int          GetDefaultRowHeightImpl(TObject* owner);
    static void         SetDefaultRowHeightImpl(TObject* owner, const int& value);
    static TGridOptions GetOptionsImpl(TObject* owner);
    static void         SetOptionsImpl(TObject* owner, const TGridOptions& value);
    static TGridRect    GetSelectionImpl(TObject* owner);
    static void         SetSelectionImpl(TObject* owner, const TGridRect& value);
    static int          GetLeftColImpl(TObject* owner);
    static void         SetLeftColImpl(TObject* owner, const int& value);
    static int          GetTopRowImpl(TObject* owner);
    static void         SetTopRowImpl(TObject* owner, const int& value);
    static bool         GetDefaultDrawingImpl(TObject* owner);
    static void         SetDefaultDrawingImpl(TObject* owner, const bool& value);
    static TColor       GetFixedColorImpl(TObject* owner);
    static void         SetFixedColorImpl(TObject* owner, const TColor& value);
    static bool         GetEditorModeImpl(TObject* owner);
    static void         SetEditorModeImpl(TObject* owner, const bool& value);
    static int          GetColWidthsImpl(TObject* owner, int ACol);
    static void         SetColWidthsImpl(TObject* owner, int ACol, const int& value);
    static int          GetRowHeightsImpl(TObject* owner, int ARow);
    static void         SetRowHeightsImpl(TObject* owner, int ARow, const int& value);

    static TOnDrawCell        GetOnDrawCellImpl(TObject* owner);
    static void               SetOnDrawCellImpl(TObject* owner, const TOnDrawCell& value);
    static TOnSelectCellEvent GetOnSelectCellImpl(TObject* owner);
    static void               SetOnSelectCellImpl(TObject* owner, const TOnSelectCellEvent& value);
    static TOnSelectEvent     GetOnSelectionImpl(TObject* owner);
    static void               SetOnSelectionImpl(TObject* owner, const TOnSelectEvent& value);
    static THdrEvent          GetOnHeaderClickImpl(TObject* owner);
    static void               SetOnHeaderClickImpl(TObject* owner, const THdrEvent& value);
};

// セルの内容を OnDrawCell で利用者が描画するグリッド(セルの文字列は持たない)。
class TDrawGrid : public TCustomDrawGrid
{
public:
    explicit TDrawGrid(TComponent* AOwner);

protected:
    ~TDrawGrid() override = default;
};

// 以下のメンバは LCL の TCustomStringGrid の public。
class TCustomStringGrid : public TCustomDrawGrid
{
public:
    // セルの文字列。C++Builder と同じく StringGrid1->Cells[ACol][ARow] = "x"; と書く(1 つ目が列、2 つ目が行)。
    IndexedProperty2<std::string> Cells;

    // すべてのセルの文字列を消す(行・列の数は変わらない)。
    void        Clean();
    // 列の幅を文字列に合わせる。
    void        AutoSizeColumns();
    void        AutoSizeColumn(int ACol);

protected:
    explicit TCustomStringGrid(no_vcl_obj_t handle);
    ~TCustomStringGrid() override = default;

private:
    static std::string GetCellsImpl(TObject* owner, int ACol, int ARow);
    static void        SetCellsImpl(TObject* owner, int ACol, int ARow, const std::string& value);
};

// セルごとに文字列を持つグリッド。
class TStringGrid : public TCustomStringGrid
{
public:
    explicit TStringGrid(TComponent* AOwner);

protected:
    ~TStringGrid() override = default;
};

/* ---------------- Timer ---------------- */

class TCustomTimer : public TComponent
{
public:
    Property<int>          Interval;
    Property<bool>         Enabled;
    Property<TNotifyEvent> OnTimer;

protected:
    explicit TCustomTimer(no_vcl_obj_t handle);
    ~TCustomTimer() override = default;

private:
    TNotifyEvent onTimer_;
    bool         onTimerHooked_ = false;
    static void NO_VCL_CALL TimerTrampoline(no_vcl_obj_t sender, void* data);
    static TNotifyEvent GetOnTimerImpl(TObject* owner);
    static void         SetOnTimerImpl(TObject* owner, const TNotifyEvent& value);

    static int  GetIntervalImpl(TObject* owner);
    static void SetIntervalImpl(TObject* owner, const int& value);
    static bool GetEnabledImpl(TObject* owner);
    static void SetEnabledImpl(TObject* owner, const bool& value);
};

class TTimer : public TCustomTimer
{
public:
    explicit TTimer(TComponent* AOwner);

protected:
    ~TTimer() override = default;
};

} // namespace no_vcl

#endif // NO_VCL_HPP
