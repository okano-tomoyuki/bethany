#ifndef BETH_HPP
#define BETH_HPP

#include <cstdint>
#include <exception>
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "internal/api.h"

// クラス階層は LCL の継承関係の「部分列」になっている(途中の階層を省くことはあっても、
// LCL に無い継承関係は作らない)。これにより、C++ 上で基底クラスとして扱えるオブジェクトは
// Pascal 側でもその基底クラスとして扱えることが保証される。
// メンバの公開範囲も LCL に合わせ、LCL で protected のメンバを派生クラスが公開している箇所は
// using で公開している。詳細は docs/class-hierarchy.md を参照。

namespace beth
{

// LCL のオブジェクトを指すハンドル(TObject::Handle())。DLL の関数に渡す値で、利用者が中身を解釈することはない。
using ObjectHandle = internal::obj_t;

// DelphiのTColorに合わせ $00BBGGRR 順のパック整数として表す。
using TColor = std::int32_t;

// 色の定数(LCL・VCL と同じ値)。標準の 16 色と、LCL の追加の 4 色(docs/adr/0050 で加えた)。
const TColor clBlack                   = 0x000000;
const TColor clMaroon                  = 0x000080;
const TColor clGreen                   = 0x008000;
const TColor clOlive                   = 0x008080;
const TColor clNavy                    = 0x800000;
const TColor clPurple                  = 0x800080;
const TColor clTeal                    = 0x808000;
const TColor clGray                    = 0x808080;
const TColor clSilver                  = 0xC0C0C0;
const TColor clRed                     = 0x0000FF;
const TColor clLime                    = 0x00FF00;
const TColor clYellow                  = 0x00FFFF;
const TColor clBlue                    = 0xFF0000;
const TColor clFuchsia                 = 0xFF00FF;
const TColor clAqua                    = 0xFFFF00;
const TColor clWhite                   = 0xFFFFFF;
const TColor clMoneyGreen              = 0xC0DCC0;
const TColor clSkyBlue                 = 0xF0CAA6;
const TColor clCream                   = 0xF0FBFF;
const TColor clMedGray                 = 0xA4A0A0;
// システムの色(docs/adr/0050)。Windows の設定の色(0x80000000 | Windows の COLOR_… の番号)。描くときに実際の色になる。
// 選択された項目の地の色は clHighlight、文字は clHighlightText 等。TColor は符号付きのため、値は負の数で書く。
const TColor clScrollBar               = -0x80000000;
const TColor clBackground              = -0x7FFFFFFF;
const TColor clActiveCaption           = -0x7FFFFFFE;
const TColor clInactiveCaption         = -0x7FFFFFFD;
const TColor clMenu                    = -0x7FFFFFFC;
const TColor clWindow                  = -0x7FFFFFFB;
const TColor clWindowFrame             = -0x7FFFFFFA;
const TColor clMenuText                = -0x7FFFFFF9;
const TColor clWindowText              = -0x7FFFFFF8;
const TColor clCaptionText             = -0x7FFFFFF7;
const TColor clActiveBorder            = -0x7FFFFFF6;
const TColor clInactiveBorder          = -0x7FFFFFF5;
const TColor clAppWorkspace            = -0x7FFFFFF4;
const TColor clHighlight               = -0x7FFFFFF3;
const TColor clHighlightText           = -0x7FFFFFF2;
const TColor clBtnFace                 = -0x7FFFFFF1;
const TColor clBtnShadow               = -0x7FFFFFF0;
const TColor clGrayText                = -0x7FFFFFEF;
const TColor clBtnText                 = -0x7FFFFFEE;
const TColor clInactiveCaptionText     = -0x7FFFFFED;
const TColor clBtnHighlight            = -0x7FFFFFEC;
const TColor cl3DDkShadow              = -0x7FFFFFEB;
const TColor cl3DLight                 = -0x7FFFFFEA;
const TColor clInfoText                = -0x7FFFFFE9;
const TColor clInfoBk                  = -0x7FFFFFE8;
const TColor clHotLight                = -0x7FFFFFE6;
const TColor clGradientActiveCaption   = -0x7FFFFFE5;
const TColor clGradientInactiveCaption = -0x7FFFFFE4;
const TColor clMenuHighlight           = -0x7FFFFFE3;
const TColor clMenuBar                 = -0x7FFFFFE2;
const TColor clForm                    = -0x7FFFFFE1;
// 色を持たない(LCL の clNone)。
const TColor clNone    = 0x1FFFFFFF;
// 既定の色(LCL の clDefault)。コントロールの Color の既定値で、実際の色はウィジェットセットが決める。
const TColor clDefault = 0x20000000;

// LCL が送出した例外(docs/adr/0031)。VCL の Exception と同じく、catch (Exception& E) で受けて E.Message を使う。
// - Bethany の関数・プロパティの中で LCL が例外を送出すると(範囲外の添字・読み込めないファイル等)、その操作は中断し、
//   この例外が送出される。ClassName() は LCL の例外のクラス名(EStringListError・EFOpenError 等)。
// - イベントのハンドラから送出された例外(この例外に限らない)は、ハンドラを呼んだ DLL 側で送出し直される。
//   メッセージループの中なら LCL が処理し(VCL と同じく、メッセージを表示して処理を続ける)、
//   MenuItem1->Click() のように Bethany の関数の中で起きたイベントなら、その関数からこの例外として送出される
//   (ClassName() は元の例外のクラス名。std::exception の派生なら "std::exception"、それ以外は空文字列)。
class Exception : public std::exception
{
public:
    explicit Exception(const std::string& Msg) : Message(Msg), className_("Exception") {}
    Exception(const std::string& AClassName, const std::string& Msg) : Message(Msg), className_(AClassName) {}

    std::string Message;

    const std::string& ClassName() const { return className_; }
    const char* what() const noexcept override { return Message.c_str(); }

private:
    std::string className_;
};

class TObject
{
public:
    TObject(const TObject&) = delete;
    TObject& operator=(const TObject&) = delete;
    TObject(TObject&&) = delete;
    TObject& operator=(TObject&&) = delete;

    ObjectHandle Handle() const { return handle_; }

protected:
    // 各クラスは、生成した(または取得した)ハンドルを基底クラスの初期化の時点で渡す。
    // これにより、派生クラスのメンバ(Property や TPaintBox::Canvas)を構築する時点で
    // handle_ が有効であることが保証される。
    explicit TObject(ObjectHandle handle) : handle_(handle) {}

    // TObject* 経由の delete でコンポーネントの寿命管理(TComponent 参照)を迂回できないよう protected にする。
    virtual ~TObject() = default;

    ObjectHandle handle_;
};

// 所有者(TObject派生インスタンス)への生ポインタ + 固定のGetter/Setter関数ポインタを持つ
// 軽量プロキシ。std::functionを使わないためヒープ確保がなく、所有者のコピー/ムーブは
// TObjectで禁止しているためownerが指す実体が入れ替わることもない。
template<typename T>
class Property
{
public:
    // プロパティの値の型(文字列・集合のプロパティの演算子の選択に使う。docs/adr/0061)。
    using PropertyValue = T;
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

    // 値を取り出す(docs/adr/0061)。暗黙の変換が効かない場面(auto で受ける・テンプレートや std::max に渡す・
    // 文字列のメソッドを呼ぶ Edit1->Text.Get().c_str() 等)で使う。
    T Get() const
    {
        return getter_(owner_);
    }

    // 複合代入(Button1->Left += 10; Label1->Caption += "!"; docs/adr/0061)。読み出した値に演算して書き戻す。
    // 演算できない型(列挙型等)で使うとコンパイルエラーになる。
    template<typename U> Property& operator+=(const U& value) { T v = Get(); v += value; setter_(owner_, v); return *this; }
    template<typename U> Property& operator-=(const U& value) { T v = Get(); v -= value; setter_(owner_, v); return *this; }
    template<typename U> Property& operator*=(const U& value) { T v = Get(); v *= value; setter_(owner_, v); return *this; }
    template<typename U> Property& operator/=(const U& value) { T v = Get(); v /= value; setter_(owner_, v); return *this; }
    // ++・--(Tag++;)。後置は変える前の値を返す。
    Property& operator++() { T v = Get(); ++v; setter_(owner_, v); return *this; }
    Property& operator--() { T v = Get(); --v; setter_(owner_, v); return *this; }
    T operator++(int) { T old = Get(); T v = old; ++v; setter_(owner_, v); return old; }
    T operator--(int) { T old = Get(); T v = old; --v; setter_(owner_, v); return old; }

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
    using PropertyValue = T;
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

    // 値を取り出す(Property::Get と同じ。docs/adr/0061)。
    T Get() const
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
// 添字の型 I は既定で int。TStrings::Values のように文字列を添字にするものは I = std::string とする。
template<typename T, typename I = int>
class IndexedProperty
{
public:
    using Getter = T    (*)(TObject*, I);
    using Setter = void (*)(TObject*, I, const T&);

    // 1 つの要素へのプロキシ。代入は値の書き込み、T への変換は値の読み出しになる
    // (auto で受けるとプロキシのままになるので、値が必要なら型を明示する)。
    class Reference
    {
    public:
        using PropertyValue = T;

        Reference(TObject* owner, I index, Getter getter, Setter setter)
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

        // 値を取り出す(docs/adr/0061)。暗黙の変換が効かない場面(auto で受ける・テンプレートや std::max に渡す・
        // 文字列のメソッドを呼ぶ Edit1->Text.Get().c_str() 等)で使う。
        T Get() const
        {
            return getter_(owner_, index_);
        }

        // 複合代入(Button1->Left += 10; Label1->Caption += "!"; docs/adr/0061)。読み出した値に演算して書き戻す。
        // 演算できない型(列挙型等)で使うとコンパイルエラーになる。
        template<typename U> Reference& operator+=(const U& value) { T v = Get(); v += value; setter_(owner_, index_, v); return *this; }
        template<typename U> Reference& operator-=(const U& value) { T v = Get(); v -= value; setter_(owner_, index_, v); return *this; }
        template<typename U> Reference& operator*=(const U& value) { T v = Get(); v *= value; setter_(owner_, index_, v); return *this; }
        template<typename U> Reference& operator/=(const U& value) { T v = Get(); v /= value; setter_(owner_, index_, v); return *this; }
        // ++・--(Tag++;)。後置は変える前の値を返す。
        Reference& operator++() { T v = Get(); ++v; setter_(owner_, index_, v); return *this; }
        Reference& operator--() { T v = Get(); --v; setter_(owner_, index_, v); return *this; }
        T operator++(int) { T old = Get(); T v = old; ++v; setter_(owner_, index_, v); return old; }
        T operator--(int) { T old = Get(); T v = old; --v; setter_(owner_, index_, v); return old; }

        T operator->() const
        {
            return getter_(owner_, index_);
        }

    private:
        TObject* owner_;
        I        index_;
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

    Reference operator[](I index) const
    {
        return Reference(owner_, index, getter_, setter_);
    }

private:
    TObject* owner_;
    Getter   getter_;
    Setter   setter_;
};

// 読み取り専用のインデックス付きのプロパティ(VCL の MenuItem->Items[i]・PageControl->Pages[i] 等)。
// 代入できないため要素のプロキシは使わず、添字で値そのものを返す(auto で受けても、printf に渡しても値になる)。
template<typename T>
class ReadOnlyIndexedProperty
{
public:
    using Getter = T (*)(TObject*, int);

    ReadOnlyIndexedProperty(TObject* owner, Getter getter)
        : owner_(owner)
        , getter_(getter)
    {}

    ReadOnlyIndexedProperty(const ReadOnlyIndexedProperty&) = delete;
    ReadOnlyIndexedProperty& operator=(const ReadOnlyIndexedProperty&) = delete;

    T operator[](int index) const
    {
        return getter_(owner_, index);
    }

private:
    TObject* owner_;
    Getter   getter_;
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
        using PropertyValue = T;

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

        // 値を取り出す(docs/adr/0061)。暗黙の変換が効かない場面(auto で受ける・テンプレートや std::max に渡す・
        // 文字列のメソッドを呼ぶ Edit1->Text.Get().c_str() 等)で使う。
        T Get() const
        {
            return getter_(owner_, index1_, index2_);
        }

        // 複合代入(Button1->Left += 10; Label1->Caption += "!"; docs/adr/0061)。読み出した値に演算して書き戻す。
        // 演算できない型(列挙型等)で使うとコンパイルエラーになる。
        template<typename U> Reference& operator+=(const U& value) { T v = Get(); v += value; setter_(owner_, index1_, index2_, v); return *this; }
        template<typename U> Reference& operator-=(const U& value) { T v = Get(); v -= value; setter_(owner_, index1_, index2_, v); return *this; }
        template<typename U> Reference& operator*=(const U& value) { T v = Get(); v *= value; setter_(owner_, index1_, index2_, v); return *this; }
        template<typename U> Reference& operator/=(const U& value) { T v = Get(); v /= value; setter_(owner_, index1_, index2_, v); return *this; }
        // ++・--(Tag++;)。後置は変える前の値を返す。
        Reference& operator++() { T v = Get(); ++v; setter_(owner_, index1_, index2_, v); return *this; }
        Reference& operator--() { T v = Get(); --v; setter_(owner_, index1_, index2_, v); return *this; }
        T operator++(int) { T old = Get(); T v = old; ++v; setter_(owner_, index1_, index2_, v); return old; }
        T operator--(int) { T old = Get(); T v = old; --v; setter_(owner_, index1_, index2_, v); return old; }

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

// ---- 文字列のプロパティの比較・連結(docs/adr/0061) ----
// Edit1->Text == "abc"・Label1->Caption = Edit1->Text + "!"・std::cout << Edit1->Text のように、std::string と同じく書けるようにする
// (std::string の演算子はテンプレートのため、プロパティから暗黙に変換されない)。対象は値の型が std::string のプロパティ・要素だけ。
namespace detail
{
template<typename P, typename = void>
struct IsStringProperty : std::false_type {};
template<typename P>
struct IsStringProperty<P, typename std::enable_if<std::is_same<typename P::PropertyValue, std::string>::value>::type>
    : std::true_type {};

template<typename P, typename R = void>
using IfStringProperty = typename std::enable_if<IsStringProperty<P>::value, R>::type;
template<typename P, typename Q, typename R = void>
using IfStringProperties = typename std::enable_if<IsStringProperty<P>::value && IsStringProperty<Q>::value, R>::type;
} // namespace detail

#define BETH_STRING_PROPERTY_OPS(OP, R)                                                                                        \
    template<typename P> detail::IfStringProperty<P, R> operator OP(const P& a, const std::string& b) { return a.Get() OP b; }  \
    template<typename P> detail::IfStringProperty<P, R> operator OP(const std::string& a, const P& b) { return a OP b.Get(); }  \
    template<typename P> detail::IfStringProperty<P, R> operator OP(const P& a, const char* b) { return a.Get() OP b; }         \
    template<typename P> detail::IfStringProperty<P, R> operator OP(const char* a, const P& b) { return a OP b.Get(); }         \
    template<typename P, typename Q> detail::IfStringProperties<P, Q, R> operator OP(const P& a, const Q& b)                   \
    {                                                                                                                           \
        return a.Get() OP b.Get();                                                                                              \
    }
BETH_STRING_PROPERTY_OPS(+, std::string)
BETH_STRING_PROPERTY_OPS(==, bool)
BETH_STRING_PROPERTY_OPS(!=, bool)
BETH_STRING_PROPERTY_OPS(<, bool)
BETH_STRING_PROPERTY_OPS(>, bool)
BETH_STRING_PROPERTY_OPS(<=, bool)
BETH_STRING_PROPERTY_OPS(>=, bool)
#undef BETH_STRING_PROPERTY_OPS

template<typename P> detail::IfStringProperty<P, std::string> operator+(const P& a, char b) { return a.Get() + b; }
template<typename P> detail::IfStringProperty<P, std::string> operator+(char a, const P& b) { return a + b.Get(); }

// std::cout << Edit1->Text;
template<typename P>
detail::IfStringProperty<P, std::ostream&> operator<<(std::ostream& os, const P& p)
{
    return os << p.Get();
}

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

// Pascal の集合型(set of 列挙型)に対応する値型(C++Builder の Set<T, minEl, maxEl> に倣う。docs/adr/0034)。
// 要素は << で加え、>> で除き、Contains で確かめる(Button1->Anchors = TAnchors() << akLeft << akRight;)。
// プロパティからは Button1->Anchors->Contains(akRight) のように -> で読める。
template<typename E>
class Set
{
public:
    using Element = E;

    Set() : bits_(0) {}

    Set& operator<<(E e) { bits_ |= Bit(e); return *this; }
    Set& operator>>(E e) { bits_ &= ~Bit(e); return *this; }
    bool Contains(E e) const { return (bits_ & Bit(e)) != 0; }
    bool Empty() const { return bits_ == 0; }
    Set& Clear() { bits_ = 0; return *this; }

    // 和・差・積(Pascal の +・-・*)。
    Set operator+(const Set& other) const { return FromInt(bits_ | other.bits_); }
    Set operator-(const Set& other) const { return FromInt(bits_ & ~other.bits_); }
    Set operator*(const Set& other) const { return FromInt(bits_ & other.bits_); }
    bool operator==(const Set& other) const { return bits_ == other.bits_; }
    bool operator!=(const Set& other) const { return bits_ != other.bits_; }

    // Property<Set<E>> の operator-> から、メンバをたどれるようにする。
    const Set* operator->() const { return this; }

    // 要素の序数をビットの位置とした整数(DLL との受け渡しに使う)。
    unsigned int ToInt() const { return bits_; }
    static Set FromInt(unsigned int bits) { Set s; s.bits_ = bits; return s; }

private:
    static unsigned int Bit(E e) { return 1u << static_cast<unsigned int>(e); }
    unsigned int bits_;
};

// ---- 集合のプロパティの演算子(docs/adr/0061) ----
// Grid1->Options = Grid1->Options << goEditing; のように、プロパティの値に要素を加えた(除いた)新しい集合を作る
// (C++Builder と同じ書き方。プロパティそのものは変わらないので、代入で書き戻す)。
namespace detail
{
template<typename P, typename = void>
struct SetOfProperty {};
template<typename P>
struct SetOfProperty<P, typename std::enable_if<std::is_same<typename P::PropertyValue,
                                                             Set<typename P::PropertyValue::Element>>::value>::type>
{
    using type = typename P::PropertyValue;
};
} // namespace detail

template<typename P, typename S = typename detail::SetOfProperty<P>::type>
S operator<<(const P& p, typename S::Element e) { S s = p.Get(); return s << e; }
template<typename P, typename S = typename detail::SetOfProperty<P>::type>
S operator>>(const P& p, typename S::Element e) { S s = p.Get(); return s >> e; }
template<typename P, typename S = typename detail::SetOfProperty<P>::type>
S operator+(const P& p, const typename detail::SetOfProperty<P>::type& other) { return p.Get() + other; }
template<typename P, typename S = typename detail::SetOfProperty<P>::type>
S operator-(const P& p, const typename detail::SetOfProperty<P>::type& other) { return p.Get() - other; }
template<typename P, typename S = typename detail::SetOfProperty<P>::type>
S operator*(const P& p, const typename detail::SetOfProperty<P>::type& other) { return p.Get() * other; }

// 修飾キー・マウスボタンの状態(LCL の TShiftState。C++Builder と同じく Set。Shift.Contains(ssShift) で調べる。docs/adr/0061)。
enum TShiftStateEnum
{
    ssShift,
    ssAlt,
    ssCtrl,
    ssLeft,    // マウスの左ボタンが押されている
    ssRight,
    ssMiddle,
    ssDouble,  // ダブルクリックの一部として発生した
    ssMeta,
    ssSuper,
    ssHyper,
    ssAltGr,
    ssCaps,
    ssNum,
    ssScroll,
    ssTriple,
    ssQuad,
    ssExtra1,
    ssExtra2
};
using TShiftState = Set<TShiftStateEnum>;

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
    explicit TPersistent(ObjectHandle handle) : TObject(handle) {}
    ~TPersistent() override = default;

private:
    friend class ItemRegistry;  // 項目のラッパーを TPersistent* 経由で delete するため
};

// TComponent ではない項目(TTreeNode・TListItem・TListColumn・THeaderSection)のラッパーの共通の管理(利用者は直接使わない)。
// 項目は FreeNotification を持たないため、TComponent のレジストリとは別に、DLL の「項目の破棄通知」
// (ItemFree_SetCallback)でラッパーを delete する。同じ項目には常に同じラッパーが返る。
// 各項目のクラスは、ハンドルを受け取るコンストラクタを private にし、friend class ItemRegistry とする。
class ItemRegistry
{
public:
    // ハンドルからラッパーを得る(無ければ作る)。nullptr には nullptr を返す。
    template<typename T>
    static T* Wrap(ObjectHandle handle)
    {
        if (!handle)
            return nullptr;
        InstallCallback();
        std::unordered_map<ObjectHandle, TPersistent*>& registry = Registry();
        auto it = registry.find(handle);
        if (it != registry.end())
            return static_cast<T*>(it->second);
        T* item = new T(handle);
        registry[handle] = item;
        return item;
    }

private:
    static void InstallCallback();
    static std::unordered_map<ObjectHandle, TPersistent*>& Registry();
    static void BETH_CALL FreeTrampoline(ObjectHandle handle, void* data);
};


// 文字列の一覧(LCL の TStrings)。コントロールの Items・Lines・Tabs 等として、所有者の値メンバで持つ非所有のビュー
// (ListBox1->Items->Add("x"); ListBox1->Items->Strings[0]; Memo1->Lines->Text = "..."; のように VCL と同じく使う)。
// LCL はウィンドウの生成・破棄のときに中身の TStrings を差し替えることがある(TListBox・TComboBox・TMemo)ため、
// このビューは中身のハンドルを覚えず、操作のたびに所有者から取得する。そのため Handle() は nullptr を返す
// (DLL の関数に渡すハンドルは Current() で得る。保存しないこと)。
// 利用者が生成する文字列の一覧は、派生の TStringList を使う(docs/adr/0028)。
class TStrings : public TPersistent
{
public:
    // 所有者のハンドルから中身の TStrings のハンドルを得る内部層の関数(internal::TCustomListBox_GetItems 等)。
    using Accessor = ObjectHandle (*)(ObjectHandle owner);

    TStrings(TObject* owner, Accessor accessor);
    ~TStrings() override = default;

    ReadOnlyProperty<int>        Count;
    IndexedProperty<std::string> Strings;
    // 利用者データ(ポインタ)。LCL は解釈も解放もしない。
    IndexedProperty<void*>       Objects;
    // すべての行を改行でつないだ文字列。設定すると改行で分けて置き換える。
    Property<std::string>        Text;
    // カンマ区切りの文字列(空白・カンマを含む要素は二重引用符で囲まれる)。
    Property<std::string>        CommaText;
    // 名前=値 の形の行。Names[i] は '=' より前、ValueFromIndex[i] は後ろ。
    // Values["name"] は name の行の値で、無ければ空文字列。無い名前に代入すると末尾に追加する。
    // 空文字列を代入したとき、ValueFromIndex[i] はその行を削除するが、Values["name"] は値を空にするだけで行は残る
    // (LCL(FPC)の仕様。VCL の Values は行を削除する)。
    ReadOnlyIndexedProperty<std::string>      Names;
    IndexedProperty<std::string, std::string> Values;
    IndexedProperty<std::string>              ValueFromIndex;
    // Delimiter で区切った文字列(既定は ',')。StrictDelimiter が false(既定)なら、空白も区切りとして扱い、
    // 空白・区切り文字を含む要素は二重引用符で囲まれる。
    Property<char>               Delimiter;
    Property<bool>               StrictDelimiter;
    Property<std::string>        DelimitedText;

    // 末尾に追加し、追加した位置を返す(ソートされた一覧では挿入された位置)。
    int  Add(const std::string& S);
    int  AddObject(const std::string& S, void* AObject);
    void Insert(int Index, const std::string& S);
    void Delete(int Index);
    void Clear();
    // 見つからなければ -1。
    int  IndexOf(const std::string& S) const;
    void Exchange(int Index1, int Index2);
    void Move(int CurIndex, int NewIndex);
    void BeginUpdate();
    void EndUpdate();
    // Source の内容(文字列と Objects)で置き換える / 末尾に加える。
    void Assign(const TStrings* Source);
    void AddStrings(const TStrings* Source);
    // 名前=値 の行のうち、名前が Name の行の位置。見つからなければ -1。
    int  IndexOfName(const std::string& Name) const;
    // ファイル名・内容とも UTF-8 のまま扱う(文字コードの変換はしない)。
    void LoadFromFile(const std::string& FileName);
    void SaveToFile(const std::string& FileName) const;

    // 現在の中身のハンドル。
    ObjectHandle Current() const { return accessor_(owner_->Handle()); }

protected:
    // 自分のハンドルそのものを中身とする(TStringList 用)。
    explicit TStrings(ObjectHandle handle);

private:
    TObject* owner_;
    Accessor accessor_;

    static ObjectHandle SelfAccessor(ObjectHandle handle) { return handle; }

    static int         GetCountImpl(TObject* owner);
    static std::string GetStringsImpl(TObject* owner, int Index);
    static void        SetStringsImpl(TObject* owner, int Index, const std::string& value);
    static void*       GetObjectsImpl(TObject* owner, int Index);
    static void        SetObjectsImpl(TObject* owner, int Index, void* const& value);
    static std::string GetTextImpl(TObject* owner);
    static void        SetTextImpl(TObject* owner, const std::string& value);
    static std::string GetCommaTextImpl(TObject* owner);
    static void        SetCommaTextImpl(TObject* owner, const std::string& value);
    static std::string GetNamesImpl(TObject* owner, int Index);
    static std::string GetValuesImpl(TObject* owner, std::string Name);
    static void        SetValuesImpl(TObject* owner, std::string Name, const std::string& value);
    static std::string GetValueFromIndexImpl(TObject* owner, int Index);
    static void        SetValueFromIndexImpl(TObject* owner, int Index, const std::string& value);
    static char        GetDelimiterImpl(TObject* owner);
    static void        SetDelimiterImpl(TObject* owner, const char& value);
    static bool        GetStrictDelimiterImpl(TObject* owner);
    static void        SetStrictDelimiterImpl(TObject* owner, const bool& value);
    static std::string GetDelimitedTextImpl(TObject* owner);
    static void        SetDelimitedTextImpl(TObject* owner, const std::string& value);
};

// Sorted のときの重複の扱い(LCL の TDuplicates と同じ値)。
enum TDuplicates { dupIgnore, dupAccept, dupError };

// 利用者が生成する文字列の一覧(LCL の TStringList)。TComponent ではないため、Owner も破棄通知も無く、
// 生成した側が破棄する(VCL と同じく new して delete する。スタックや値メンバに置いてもよい)。
// TStrings* を受け取るもの(ListBox1->Items->Assign(List) 等)にそのまま渡せる。
class TStringList : public TStrings
{
public:
    TStringList();
    ~TStringList() override;

    // true にすると並べ替え、以降の Add はソート順の位置に入る(Insert と Strings への代入は Exception を送出する)。
    Property<bool>        Sorted;
    // Sorted のときの重複の扱い(既定は dupIgnore で、重複は加えない。dupError で重複を加えると Exception を送出する)。
    Property<TDuplicates> Duplicates;
    // 並べ替え・IndexOf・Find で大文字と小文字を区別するか(既定は false)。
    Property<bool>        CaseSensitive;

    void Sort();
    // ソートされた一覧から S を二分探索する。見つからなければ、S を挿入すべき位置を Index に入れて false を返す。
    // Sorted が false の一覧には使えない(Exception を送出する)。
    bool Find(const std::string& S, int& Index) const;

private:
    static bool        GetSortedImpl(TObject* owner);
    static void        SetSortedImpl(TObject* owner, const bool& value);
    static TDuplicates GetDuplicatesImpl(TObject* owner);
    static void        SetDuplicatesImpl(TObject* owner, const TDuplicates& value);
    static bool        GetCaseSensitiveImpl(TObject* owner);
    static void        SetCaseSensitiveImpl(TObject* owner, const bool& value);
};

/* ---------------- Geometry ---------------- */

// 矩形(VCL の TRect と同じく Left/Top/Right/Bottom)。Canvas の描画や、グリッドの選択範囲(TGridRect)にも使う。
struct TRect
{
    int Left;
    int Top;
    int Right;
    int Bottom;
};

// 点(VCL の TPoint と同じく X/Y)。
struct TPoint
{
    int X;
    int Y;
};

/* ---------------- Canvas ---------------- */

// TPen/TBrush/TFont/TCanvas は LCL でも TComponent ではなく TPersistent の派生であり、
// Canvas を持つコントロールが内部で保持するオブジェクトへの非所有(non-owning)ラッパー。
// 自前で Create/Destroy は行わない(取得元のコントロールが破棄されれば一緒に破棄される)。
// 所有者のクラスの中に実体(値メンバ)を置き、C++Builder と同じくポインタのプロパティで返す(Canvas->Pen->Color。docs/adr/0057)。
// そのため、これらのデストラクタは public にしている。

// 線の種類(LCL の TPenStyle と同じ値。docs/adr/0045)。psClear は線を描かない。
enum TPenStyle { psSolid, psDash, psDot, psDashDot, psDashDotDot, psInsideFrame, psPattern, psClear };
// 線の描き方(ラスタ演算。LCL の TPenMode と同じ値)。pmXor で 2 度描くと元に戻る(ラバーバンド等)。
enum TPenMode
{
    pmBlack, pmWhite, pmNop, pmNot, pmCopy, pmNotCopy, pmMergePenNot, pmMaskPenNot,
    pmMergeNotPen, pmMaskNotPen, pmMerge, pmNotMerge, pmMask, pmNotMask, pmXor, pmNotXor
};
// 塗りつぶしの種類(LCL の TBrushStyle と同じ値)。bsClear は塗りつぶさない(TextOut の背景も透明になる)。
enum TBrushStyle
{
    bsSolid, bsClear, bsHorizontal, bsVertical, bsFDiagonal, bsBDiagonal, bsCross, bsDiagCross, bsImage, bsPattern
};
// 文字の描き方(アンチエイリアス等。LCL の TFontQuality と同じ値)。
enum TFontQuality { fqDefault, fqDraft, fqProof, fqNonAntialiased, fqAntialiased, fqCleartype, fqCleartypeNatural };

class TPen : public TPersistent
{
public:
    Property<TColor>    Color;
    Property<int>       Width;
    Property<TPenStyle> Style;
    Property<TPenMode>  Mode;

    explicit TPen(ObjectHandle handle);
    ~TPen() override = default;

    // Source の内容を写す(VCL の Pen->Assign。docs/adr/0057)。nullptr なら何もしない。
    void Assign(const TPen* Source);

private:
    static TColor    GetColorImpl(TObject* owner);
    static void      SetColorImpl(TObject* owner, const TColor& value);
    static int       GetWidthImpl(TObject* owner);
    static void      SetWidthImpl(TObject* owner, const int& value);
    static TPenStyle GetStyleImpl(TObject* owner);
    static void      SetStyleImpl(TObject* owner, const TPenStyle& value);
    static TPenMode  GetModeImpl(TObject* owner);
    static void      SetModeImpl(TObject* owner, const TPenMode& value);
};

class TBrush : public TPersistent
{
public:
    Property<TColor>      Color;
    Property<TBrushStyle> Style;

    explicit TBrush(ObjectHandle handle);
    ~TBrush() override = default;

    // Source の内容を写す(VCL の Brush->Assign。docs/adr/0057)。nullptr なら何もしない。
    void Assign(const TBrush* Source);

private:
    static TColor      GetColorImpl(TObject* owner);
    static void        SetColorImpl(TObject* owner, const TColor& value);
    static TBrushStyle GetStyleImpl(TObject* owner);
    static void        SetStyleImpl(TObject* owner, const TBrushStyle& value);
};

// フォントの修飾(LCL の TFontStyles)。Font->Style = TFontStyles() << fsBold << fsItalic; のように書く(docs/adr/0061)。
enum TFontStyle
{
    fsBold,
    fsItalic,
    fsUnderline,
    fsStrikeOut
};
using TFontStyles = Set<TFontStyle>;

// Canvas・コントロール・TFontDialog が持つフォントへの非所有のラッパー(Style・Assign は docs/adr/0033)。
class TFont : public TPersistent
{
public:
    Property<std::string> Name;
    Property<int>         Size;
    Property<TColor>      Color;
    Property<TFontStyles> Style;
    // 文字の高さ(ピクセル)。負の値は文字の高さ、正の値はセルの高さ(内部の余白を含む)。0 は既定。Size と連動する。
    Property<int>          Height;
    // 文字の傾き(0.1 度単位。反時計回り。900 で縦書きの向き)。
    Property<int>          Orientation;
    Property<TFontQuality> Quality;

    explicit TFont(ObjectHandle handle);
    ~TFont() override = default;

    // Source の内容(Name・Size・Color・Style 等)を写す(VCL の Font->Assign)。nullptr なら何もしない。
    void Assign(const TFont* Source);

private:
    static TFontStyles GetStyleImpl(TObject* owner);
    static void        SetStyleImpl(TObject* owner, const TFontStyles& value);
    static std::string GetNameImpl(TObject* owner);
    static void        SetNameImpl(TObject* owner, const std::string& value);
    static int         GetSizeImpl(TObject* owner);
    static void        SetSizeImpl(TObject* owner, const int& value);
    static TColor      GetColorImpl(TObject* owner);
    static void        SetColorImpl(TObject* owner, const TColor& value);
    static int          GetHeightImpl(TObject* owner);
    static void         SetHeightImpl(TObject* owner, const int& value);
    static int          GetOrientationImpl(TObject* owner);
    static void         SetOrientationImpl(TObject* owner, const int& value);
    static TFontQuality GetQualityImpl(TObject* owner);
    static void         SetQualityImpl(TObject* owner, const TFontQuality& value);
};

class TGraphic;
class TClipboard;

class TCanvas : public TPersistent
{
public:
    // 線・塗りつぶし・文字(C++Builder と同じくポインタ。Canvas->Pen->Color = clRed;。docs/adr/0057)。
    // Canvas が所有するものへの非所有のラッパーで、代入(Canvas->Pen = OtherPen;)は内容のコピー(VCL・LCL の Assign)。
    Property<TPen*>   Pen;
    Property<TBrush*> Brush;
    Property<TFont*>  Font;
    // 1 画素の色(Canvas->Pixels[X][Y]。VCL の Pixels[X, Y])。
    IndexedProperty2<TColor> Pixels;

    explicit TCanvas(ObjectHandle handle);
    ~TCanvas() override = default;

    void MoveTo(int x, int y);
    void LineTo(int x, int y);
    void Rectangle(int x1, int y1, int x2, int y2);
    void Ellipse(int x1, int y1, int x2, int y2);
    void TextOut(int x, int y, const std::string& text);
    // Brush で塗りつぶす(枠は描かない)。
    void FillRect(const TRect& Rect);
    // グラフィックを描く(docs/adr/0029)。Graphic が nullptr なら何もしない。StretchDraw は Rect に合わせて伸縮する。
    void Draw(int X, int Y, const TGraphic* Graphic);
    void StretchDraw(const TRect& Rect, const TGraphic* Graphic);

    // ---- docs/adr/0045 ----
    // Text を今の Font で描いたときの幅・高さ(ピクセル)。
    int  TextWidth(const std::string& Text) const;
    int  TextHeight(const std::string& Text) const;
    // Rect の中だけに、(X, Y) から Text を描く(はみ出した部分は切り取る)。
    void TextRect(const TRect& Rect, int X, int Y, const std::string& Text);
    // 点を結んだ多角形(Pen で縁を、Brush で中を描く)・折れ線(Pen だけ)。Points は {{0, 0}, {10, 0}, {5, 8}} のようにも書ける。
    void Polygon(const std::vector<TPoint>& Points);
    void Polygon(const TPoint* Points, int Count);
    void Polyline(const std::vector<TPoint>& Points);
    void Polyline(const TPoint* Points, int Count);
    // 角の丸い矩形(RX・RY は角の楕円の幅・高さ)。
    void RoundRect(int X1, int Y1, int X2, int Y2, int RX, int RY);
    // (X1, Y1)-(X2, Y2) に内接する楕円の、中心から (X3, Y3) の方向から (X4, Y4) の方向まで(反時計回り)の弧・扇形・弓形。
    void Arc(int X1, int Y1, int X2, int Y2, int X3, int Y3, int X4, int Y4);
    void Pie(int X1, int Y1, int X2, int Y2, int X3, int Y3, int X4, int Y4);
    void Chord(int X1, int Y1, int X2, int Y2, int X3, int Y3, int X4, int Y4);
    // Rect の縁を Brush の色で 1 ピクセルの幅で描く(中は描かない)。
    void FrameRect(const TRect& Rect);
    // Canvas の Source の範囲を、この Canvas の Dest に写す(大きさが違えば伸縮する)。Canvas が nullptr なら何もしない。
    void CopyRect(const TRect& Dest, const TCanvas* Canvas, const TRect& Source);

private:
    TPen   pen_;
    TBrush brush_;
    TFont  font_;
    static TPen*   GetPenImpl(TObject* owner);
    static void    SetPenImpl(TObject* owner, TPen* const& value);
    static TBrush* GetBrushImpl(TObject* owner);
    static void    SetBrushImpl(TObject* owner, TBrush* const& value);
    static TFont*  GetFontImpl(TObject* owner);
    static void    SetFontImpl(TObject* owner, TFont* const& value);
    static TColor GetPixelsImpl(TObject* owner, int X, int Y);
    static void   SetPixelsImpl(TObject* owner, int X, int Y, const TColor& value);
};

/* ---------------- Graphics(docs/adr/0029) ---------------- */

// グラフィック・TImage の Canvas を TCanvas のラッパーとして返すための保持者(利用者は直接使わない)。
// これらの Canvas は、所有者が中身のグラフィックを作り直すと別のオブジェクトになるため、取得のたびに現在のハンドル
// (と Pen・Brush・Font のハンドル)を確かめ、変わっていればラッパーを作り直す。
class CanvasHolder
{
public:
    CanvasHolder() = default;
    CanvasHolder(const CanvasHolder&) = delete;
    CanvasHolder& operator=(const CanvasHolder&) = delete;

    // canvas が nullptr なら nullptr を返す。
    TCanvas* Get(ObjectHandle canvas);

private:
    std::unique_ptr<TCanvas> canvas_;
};

// グラフィック(LCL の TGraphic。TPersistent で、TComponent ではない)。2 通りの持ち方がある。
//   - 利用者が生成するもの(new TBitmap 等): VCL と同じく delete で破棄する(LCL のオブジェクトも破棄される)。
//     スタックや値メンバに置いてもよい。
//   - 所有者の中身のビュー(Image1->Picture->Bitmap・BitBtn1->Glyph 等): TStrings と同じく中身のハンドルを覚えず、
//     操作のたびに所有者から取得する(TPicture は LoadFromFile 等のたびに中身を作り直すため)。Handle() は nullptr を返す
//     (DLL の関数に渡すハンドルは Current() で得る。保存しないこと)。
// Picture->Graphic・Glyph 等への代入は、LCL と同じく内容のコピーになる(代入したものは代入した側の持ち物のまま)。
// 読み込めないファイル・形式の違うファイルでは Exception(EFOpenError 等)が送出される。
class TGraphic : public TPersistent
{
public:
    // 所有者から中身のグラフィックのハンドルを得る内部層の関数(internal::TPicture_GetBitmap 等)。
    using Accessor = ObjectHandle (*)(ObjectHandle owner);

    // 利用者が生成したものなら、LCL のオブジェクトも破棄する。ビューなら何もしない。
    ~TGraphic() override;

    Property<int>          Width;
    Property<int>          Height;
    ReadOnlyProperty<bool> Empty;
    Property<bool>         Transparent;

    // 形式はクラスで決まる(TBitmap に PNG のファイルは読めない)。拡張子で形式を選ぶのは TPicture::LoadFromFile。
    // ファイル名は UTF-8。
    void LoadFromFile(const std::string& FileName);
    void SaveToFile(const std::string& FileName) const;
    // Source の内容で置き換える(別のクラスのグラフィックからは画素を写して変換する)。nullptr なら空にする。
    void Assign(const TGraphic* Source);
    // クリップボードの画像で置き換える(Bitmap1->Assign(Clipboard())。docs/adr/0047)。画像が無ければ何もしない。
    void Assign(const TClipboard* Source);
    void Clear();

    // 現在の中身のハンドル。
    ObjectHandle Current() const { return accessor_(owner_->Handle()); }

protected:
    // 利用者が生成したもの(自分のハンドルを持つ)。
    explicit TGraphic(ObjectHandle handle);
    // 所有者の中身のビュー。
    TGraphic(TObject* owner, Accessor accessor);

private:
    friend class TPicture;  // Graphic のビュー(クラスを問わない)を持つため

    TObject* owner_;
    Accessor accessor_;
    bool     owns_;

    static ObjectHandle SelfAccessor(ObjectHandle handle) { return handle; }

    static int  GetWidthImpl(TObject* owner);
    static void SetWidthImpl(TObject* owner, const int& value);
    static int  GetHeightImpl(TObject* owner);
    static void SetHeightImpl(TObject* owner, const int& value);
    static bool GetEmptyImpl(TObject* owner);
    static bool GetTransparentImpl(TObject* owner);
    static void SetTransparentImpl(TObject* owner, const bool& value);
};

// 画素の形式(LCL の TPixelFormat と同じ値)。
enum TPixelFormat { pfDevice, pf1bit, pf4bit, pf8bit, pf15bit, pf16bit, pf24bit, pf32bit, pfCustom };
// Transparent のときに透過する色の決め方。tmAuto は左下の画素の色、tmFixed は TransparentColor。
enum TTransparentMode { tmAuto, tmFixed };

// TBitmap・TPortableNetworkGraphic・TJPEGImage の共通の基底(LCL の TRasterImage)。
class TRasterImage : public TGraphic
{
public:
    // グラフィックに描く先。グラフィックが所有し、中身が作り直されると別のものになる(ポインタを保存しないこと)。
    ReadOnlyProperty<TCanvas*>  Canvas;
    Property<TPixelFormat>      PixelFormat;
    Property<TColor>            TransparentColor;
    Property<TTransparentMode>  TransparentMode;

protected:
    explicit TRasterImage(ObjectHandle handle);
    TRasterImage(TObject* owner, Accessor accessor);

private:
    CanvasHolder canvas_;

    static TCanvas*         GetCanvasImpl(TObject* owner);
    static TPixelFormat     GetPixelFormatImpl(TObject* owner);
    static void             SetPixelFormatImpl(TObject* owner, const TPixelFormat& value);
    static TColor           GetTransparentColorImpl(TObject* owner);
    static void             SetTransparentColorImpl(TObject* owner, const TColor& value);
    static TTransparentMode GetTransparentModeImpl(TObject* owner);
    static void             SetTransparentModeImpl(TObject* owner, const TTransparentMode& value);
};

class TCustomBitmap : public TRasterImage
{
public:
    void SetSize(int AWidth, int AHeight);

protected:
    explicit TCustomBitmap(ObjectHandle handle) : TRasterImage(handle) {}
    TCustomBitmap(TObject* owner, Accessor accessor) : TRasterImage(owner, accessor) {}
};

class TCustomBitBtn;
class TCustomSpeedButton;
class TMenuItem;
class TCoolBand;
class TCustomCoolBar;

// ビットマップ(.bmp)。new TBitmap で生成して delete で破棄する(VCL と同じ)。
class TBitmap : public TCustomBitmap
{
public:
    TBitmap();

private:
    friend class TPicture;
    friend class TCustomBitBtn;
    friend class TCustomSpeedButton;
    friend class TMenuItem;
    friend class TCoolBand;
    friend class TCustomCoolBar;
    TBitmap(TObject* owner, Accessor accessor) : TCustomBitmap(owner, accessor) {}
};

// PNG 画像(.png)。LCL の TPortableNetworkGraphic(VCL の TPngImage に当たる)。
class TPortableNetworkGraphic : public TCustomBitmap
{
public:
    TPortableNetworkGraphic();

private:
    friend class TPicture;
    TPortableNetworkGraphic(TObject* owner, Accessor accessor) : TCustomBitmap(owner, accessor) {}
};

// JPEG 画像(.jpg)。
class TJPEGImage : public TCustomBitmap
{
public:
    TJPEGImage();

    // 保存するときの品質(1〜100。既定は 75)。
    Property<int> CompressionQuality;

private:
    friend class TPicture;
    TJPEGImage(TObject* owner, Accessor accessor);

    static int  GetCompressionQualityImpl(TObject* owner);
    static void SetCompressionQualityImpl(TObject* owner, const int& value);
};

// アイコン(.ico。docs/adr/0047)。LCL の TIcon(TCustomIcon の派生)。new TIcon で生成して delete で破棄する(TBitmap と同じ)。
// Form1->Icon・Application->Icon・Picture->Icon は所有者の中身のビュー。1 つのファイルに大きさの違う画像を複数持てる。
class TIcon : public TRasterImage
{
public:
    TIcon();

private:
    friend class TPicture;
    friend class TCustomForm;
    friend class TApplication;
    TIcon(TObject* owner, Accessor accessor) : TRasterImage(owner, accessor) {}
};

class TCustomImage;

// 形式を問わない画像の入れ物(LCL の TPicture)。Image1->Picture のように画像コントロールが持つもの(コントロールと寿命が一致する)と、
// 利用者が new TPicture で生成して delete で破棄するものがある。
class TPicture : public TPersistent
{
public:
    TPicture();
    // 利用者が生成したものなら、LCL のオブジェクトも破棄する。
    ~TPicture() override;

    // 中身のグラフィック(空なら nullptr)。クラスを問わない TGraphic のビューで、TBitmap 等への dynamic_cast はできない
    // (クラスごとの操作は Bitmap・PNG・Jpeg を使う)。代入は内容のコピー(nullptr なら空にする)。
    Property<TGraphic*>                Graphic;
    // 中身をそのクラスとして扱うビュー。操作したとき、中身が別のクラスなら LCL が変換し、空なら空のものを作る。
    // 代入は Graphic と同じく内容のコピー。
    Property<TBitmap*>                 Bitmap;
    Property<TPortableNetworkGraphic*> PNG;
    Property<TJPEGImage*>              Jpeg;
    Property<TIcon*>                   Icon;
    ReadOnlyProperty<int>              Width;
    ReadOnlyProperty<int>              Height;

    // 拡張子から形式(クラス)を選んで読み込む(.bmp・.png・.jpg 等)。ファイル名は UTF-8。
    void LoadFromFile(const std::string& FileName);
    void SaveToFile(const std::string& FileName) const;
    // Source の内容で置き換える。nullptr なら空にする。
    void Assign(const TPicture* Source);
    // クリップボードの画像で置き換える(Image1->Picture->Assign(Clipboard())。docs/adr/0047)。画像が無ければ何もしない。
    void Assign(const TClipboard* Source);
    void Clear();

private:
    friend class TCustomImage;
    // owns が false なら画像コントロールが持つもの(破棄しない)。
    TPicture(ObjectHandle handle, bool owns);

    bool                    owns_;
    TGraphic                graphic_;
    TBitmap                 bitmap_;
    TPortableNetworkGraphic png_;
    TJPEGImage              jpeg_;
    TIcon                   icon_;

    static TGraphic*                GetGraphicImpl(TObject* owner);
    static void                     SetGraphicImpl(TObject* owner, TGraphic* const& value);
    static TBitmap*                 GetBitmapImpl(TObject* owner);
    static void                     SetBitmapImpl(TObject* owner, TBitmap* const& value);
    static TPortableNetworkGraphic* GetPNGImpl(TObject* owner);
    static void                     SetPNGImpl(TObject* owner, TPortableNetworkGraphic* const& value);
    static TJPEGImage*              GetJpegImpl(TObject* owner);
    static void                     SetJpegImpl(TObject* owner, TJPEGImage* const& value);
    static TIcon*                   GetIconImpl(TObject* owner);
    static void                     SetIconImpl(TObject* owner, TIcon* const& value);
    static int                      GetWidthImpl(TObject* owner);
    static int                      GetHeightImpl(TObject* owner);
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
    // 利用者が自由に使う整数(LCL は解釈しない。ポインタと同じ幅。docs/adr/0034)。
    Property<std::intptr_t> Tag;

    // LCL オブジェクトを破棄する。破棄通知によってこのラッパーも delete されるため、
    // 呼び出し後にこのオブジェクトへ触れてはならない。
    void Free();

protected:
    explicit TComponent(ObjectHandle handle);
    ~TComponent() override;

    static ObjectHandle HandleOf(const TObject* obj) { return obj ? obj->Handle() : nullptr; }
    static TComponent*  FromHandle(ObjectHandle handle);

    // LCL が内部で生成したコンポーネント(TMenu::Items のルート項目等、*_Create を経由しないもの)のハンドルから
    // ラッパーを得る。まだラッパーが無ければ、その場で作ってレジストリに登録する(以降は同じラッパーを返す)。
    // ハンドルを返す DLL の関数の側で破棄通知の対象に登録しておくこと(登録されていないとラッパーが delete されない)。
    // T は、ハンドルを受け取るコンストラクタを TComponent から呼べるようにしておく(friend class TComponent)。
    template<typename T>
    static T* WrapExisting(ObjectHandle handle)
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

    static void BETH_CALL FreeNotifyTrampoline(ObjectHandle handle, void* data);
    static std::unordered_map<ObjectHandle, TComponent*>& Registry();

    static std::intptr_t GetTagImpl(TObject* owner);
    static void          SetTagImpl(TObject* owner, const std::intptr_t& value);
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

/* ---------------- ImageList(docs/adr/0030) ---------------- */

// 画像リストの描き方(LCL の TDrawingStyle と同じ値)。
enum TDrawingStyle { dsFocus, dsSelected, dsNormal, dsTransparent };

// 同じ大きさの画像の一覧(LCL の TCustomImageList)。TComponent なので、他のコンポーネントと同じく new で生成し、
// Owner に任せるか Free() で破棄する。ツリービュー・ツールバー等の Images に設定し、項目の ImageIndex で画像を選ぶ。
// 画像を受け取るメソッドは、画像を写して加える(渡したグラフィックは呼び出し側の持ち物のまま)。
// Add・Insert 等は、画像を Width・Height の大きさに伸縮して 1 つとして加える(VCL と違い、幅が Width の倍数でも分けない)。
// 横に並んだ複数の画像を分けて加えるのは AddSliced。
// 画像リストを破棄すると(Free()・Owner の破棄)、それを Images 等に設定していたコントロールの Images は LCL が nullptr に戻す。
class TCustomImageList : public TComponent
{
public:
    // 画像の大きさ(既定は 16x16)。
    Property<int>           Width;
    Property<int>           Height;
    ReadOnlyProperty<int>   Count;
    Property<bool>          Masked;
    Property<TColor>        BkColor;
    Property<TDrawingStyle> DrawingStyle;
    // Clear・Delete・Move・BkColor の変更で呼ばれる(LCL の仕様で、Add・Insert 等では呼ばれない。
    // BeginUpdate の間は EndUpdate まで遅れる)。
    Property<TNotifyEvent>  OnChange;

    // Mask は nullptr でよい。
    int  Add(const TCustomBitmap* Image, const TCustomBitmap* Mask);
    // Image を横 AHorizontalCount・縦 AVerticalCount に分けて、それぞれを画像として加える。加えた最初の画像の位置を返す。
    int  AddSliced(const TCustomBitmap* Image, int AHorizontalCount, int AVerticalCount);
    // MaskColor の画素を透明として加える。
    int  AddMasked(const TBitmap* Image, TColor MaskColor);
    void Insert(int Index, const TCustomBitmap* Image, const TCustomBitmap* Mask);
    void Replace(int Index, const TCustomBitmap* Image, const TCustomBitmap* Mask);
    void Delete(int Index);
    void Clear();
    void Move(int CurIndex, int NewIndex);
    // Index 番目の画像を Image に写す。
    void GetBitmap(int Index, TCustomBitmap* Image) const;
    // Canvas の (X, Y) に Index 番目の画像を描く。Enabled が false なら無効の見た目で描く。
    void Draw(TCanvas* Canvas, int X, int Y, int Index, bool Enabled = true) const;
    void BeginUpdate();
    void EndUpdate();

protected:
    explicit TCustomImageList(ObjectHandle handle);
    ~TCustomImageList() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;

    static int           GetWidthImpl(TObject* owner);
    static void          SetWidthImpl(TObject* owner, const int& value);
    static int           GetHeightImpl(TObject* owner);
    static void          SetHeightImpl(TObject* owner, const int& value);
    static int           GetCountImpl(TObject* owner);
    static bool          GetMaskedImpl(TObject* owner);
    static void          SetMaskedImpl(TObject* owner, const bool& value);
    static TColor        GetBkColorImpl(TObject* owner);
    static void          SetBkColorImpl(TObject* owner, const TColor& value);
    static TDrawingStyle GetDrawingStyleImpl(TObject* owner);
    static void          SetDrawingStyleImpl(TObject* owner, const TDrawingStyle& value);
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent  GetOnChangeImpl(TObject* owner);
    static void          SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

// LCL の TImageList は TDragImageList(ドラッグ中の画像の表示)の派生だが、その機能は公開していないため省いた。
class TImageList : public TCustomImageList
{
public:
    explicit TImageList(TComponent* AOwner);

protected:
    ~TImageList() override = default;
};

class TMenu;
class TBasicAction;

// ---- オーナードロー(docs/adr/0050。値の順は LCL と同じ) ----

class TWinControl;

// リストボックスの描き方(lbOwnerDrawFixed・lbOwnerDrawVariable なら OnDrawItem で描く。lbVirtual は対象外)。
enum TListBoxStyle { lbStandard, lbOwnerDrawFixed, lbOwnerDrawVariable, lbVirtual };
// 描く項目の状態(LCL の TOwnerDrawState)。Set(State.Contains(odSelected) で調べる。docs/adr/0061)。
enum TOwnerDrawStateType
{
    odSelected,
    odGrayed,
    odDisabled,
    odChecked,
    odFocused,
    odDefault,
    odHotLight,
    odInactive,
    odNoAccel,
    odNoFocusRect,
    odReserved1,
    odReserved2,
    odComboBoxEdit,
    odBackgroundPainted
};
using TOwnerDrawState = Set<TOwnerDrawStateType>;

// リストボックス・コンボボックスの項目を描くとき。Control の Canvas の ARect に描く。
using TDrawItemEvent = std::function<void(TWinControl* Control, int Index, TRect ARect, TOwnerDrawState State)>;
// 項目の高さを決めるとき(lbOwnerDrawVariable・csOwnerDrawVariable)。AHeight を項目の高さにする。
using TMeasureItemEvent = std::function<void(TWinControl* Control, int Index, int& AHeight)>;
// メニュー項目を描くとき(メニューの OwnerDraw が true)。ACanvas の ARect に描く(ACanvas はこの呼び出しの間だけ有効)。
using TMenuDrawItemEvent = std::function<void(TObject* Sender, TCanvas* ACanvas, TRect ARect, TOwnerDrawState AState)>;
// メニュー項目の大きさを決めるとき。AWidth・AHeight を項目の幅・高さにする。
using TMenuMeasureItemEvent = std::function<void(TObject* Sender, TCanvas* ACanvas, int& AWidth, int& AHeight)>;

// メニューの項目。TControl ではない(Parent/Left 等は無く、画面上の親子関係は Add/Insert で組む)。
// 子の項目は LCL の Items[Index] / Count に合わせ、Items[Index] / Count で参照する。
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
    // 0〜255。
    Property<int>          GroupIndex;
    Property<bool>         Default;
    Property<TShortCut>    ShortCut;
    Property<std::string>  Hint;
    Property<TNotifyEvent> OnClick;

    ReadOnlyProperty<int>        Count;
    // 親の項目。メニューの直下の項目なら、そのメニューの Items(ルート)。どこにも追加されていなければ nullptr。
    ReadOnlyProperty<TMenuItem*> Parent;

    // 子の項目(MenuItem->Items[i]->Caption のように使う)。
    ReadOnlyIndexedProperty<TMenuItem*> Items;
    // 画像の、メニューの Images(または親の項目の SubMenuImages)での位置(-1 なら無し。docs/adr/0030)。
    Property<int> ImageIndex;
    // 子の項目の画像リスト(設定すると、子の項目は TMenu::Images の代わりにこれを使う)。
    Property<TCustomImageList*> SubMenuImages;
    // 項目の画像(ImageIndex を使わない場合)。項目が所有する TBitmap のビューで、初めて参照したときに作られる。代入は内容のコピー。
    Property<TBitmap*> Bitmap;
    // 割り当てた Action(docs/adr/0046)。Action の Caption・Checked・Enabled・ShortCut・ImageIndex 等が項目に写り、以後も連動する。
    // 選ぶと、OnClick(設定していれば)の後に Action の OnExecute が呼ばれる。
    Property<TBasicAction*> Action;

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

    // ---- docs/adr/0050 ----
    // メニューの OwnerDraw が true のとき、この項目を描く・大きさを決める。
    Property<TMenuDrawItemEvent> OnDrawItem;
    Property<TMenuMeasureItemEvent> OnMeasureItem;

protected:
    ~TMenuItem() override = default;

private:
    static TMenuItem* GetItemsImpl(TObject* owner, int Index);
    friend class TComponent;  // WrapExisting から、下のハンドルを受け取るコンストラクタを呼ぶため
    explicit TMenuItem(ObjectHandle handle);

    TNotifyEvent onClick_;
    bool         onClickHooked_ = false;
    static void BETH_CALL ClickTrampoline(ObjectHandle sender, void* data);

    static TBasicAction* GetActionImpl(TObject* owner);
    static void          SetActionImpl(TObject* owner, TBasicAction* const& value);

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

    TBitmap bitmap_;
    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
    static TCustomImageList* GetSubMenuImagesImpl(TObject* owner);
    static void SetSubMenuImagesImpl(TObject* owner, TCustomImageList* const& value);
    static TBitmap* GetBitmapImpl(TObject* owner);
    static void SetBitmapImpl(TObject* owner, TBitmap* const& value);

    // ---- docs/adr/0050 ----
    TMenuDrawItemEvent onDrawItem_;
    bool onDrawItemHooked_ = false;
    static TMenuDrawItemEvent GetOnDrawItemImpl(TObject* owner);
    static void SetOnDrawItemImpl(TObject* owner, const TMenuDrawItemEvent& value);
    static void BETH_CALL DrawItemTrampoline(ObjectHandle sender, ObjectHandle canvas, internal::int_t left, internal::int_t top, internal::int_t right, internal::int_t bottom, internal::uint_t state, void* data);
    TMenuMeasureItemEvent onMeasureItem_;
    bool onMeasureItemHooked_ = false;
    static TMenuMeasureItemEvent GetOnMeasureItemImpl(TObject* owner);
    static void SetOnMeasureItemImpl(TObject* owner, const TMenuMeasureItemEvent& value);
    static void BETH_CALL MeasureItemTrampoline(ObjectHandle sender, ObjectHandle canvas, internal::int_t* width, internal::int_t* height, void* data);
};

// TMainMenu・TPopupMenu の共通の基底。Items はメニューのルートの項目で、メニュー自身が(LCL の内部で)生成・所有する。
// メニューに表示する項目は Items->Add(...) で追加する。
class TMenu : public TComponent
{
public:
    ReadOnlyProperty<TMenuItem*> Items;
    // 項目の画像リスト(docs/adr/0030)。各項目の画像は TMenuItem::ImageIndex。
    Property<TCustomImageList*> Images;

    // ---- docs/adr/0050 ----
    // true なら、項目を各項目の OnDrawItem で描く(OnMeasureItem で大きさを決める)。
    Property<bool> OwnerDraw;

protected:
    explicit TMenu(ObjectHandle handle);
    ~TMenu() override = default;

private:
    static TMenuItem* GetItemsImpl(TObject* owner);

    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);

    // ---- docs/adr/0050 ----
    static bool GetOwnerDrawImpl(TObject* owner);
    static void SetOwnerDrawImpl(TObject* owner, const bool& value);
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
    static void BETH_CALL PopupTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL CloseTrampoline(ObjectHandle sender, void* data);

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

// コントロールの辺(LCL の TAnchorKind と同じ値。VCL の TAnchorKind とは並びが違う)。
enum TAnchorKind { akTop, akLeft, akRight, akBottom };
// 親の辺との距離を保つ辺の集合(既定は akLeft・akTop。akRight も加えると、親の幅に合わせて幅が変わる)。
using TAnchors = Set<TAnchorKind>;

// マウスカーソルの形(LCL の TCursor と同じ値)。
using TCursor = std::int32_t;
const TCursor crDefault   = 0;
const TCursor crNone      = -1;
const TCursor crArrow     = -2;
const TCursor crCross     = -3;
const TCursor crIBeam     = -4;
const TCursor crSizeNESW  = -6;
const TCursor crSizeNS    = -7;
const TCursor crSizeNWSE  = -8;
const TCursor crSizeWE    = -9;
const TCursor crUpArrow   = -10;
const TCursor crHourGlass = -11;
const TCursor crDrag      = -12;
const TCursor crNoDrop    = -13;
const TCursor crHSplit    = -14;
const TCursor crVSplit    = -15;
const TCursor crMultiDrag = -16;
const TCursor crSQLWait   = -17;
const TCursor crNo        = -18;
const TCursor crAppStart  = -19;
const TCursor crHelp      = -20;
const TCursor crHandPoint = -21;
const TCursor crSizeAll   = -22;
const TCursor crSize      = -22;
const TCursor crSizeNW    = -23;
const TCursor crSizeN     = -24;
const TCursor crSizeNE    = -25;
const TCursor crSizeW     = -26;
const TCursor crSizeE     = -27;
const TCursor crSizeSW    = -28;
const TCursor crSizeS     = -29;
const TCursor crSizeSE    = -30;

// コントロールの大きさの制限(LCL の TSizeConstraints)。コントロールが所有するものへの非所有のラッパー(docs/adr/0034)。
// 0 は制限なし。Button1->Constraints->MinWidth = 80; のように使う。
class TSizeConstraints : public TPersistent
{
public:
    Property<int> MinWidth;
    Property<int> MinHeight;
    Property<int> MaxWidth;
    Property<int> MaxHeight;

    explicit TSizeConstraints(ObjectHandle handle);
    ~TSizeConstraints() override = default;

private:
    static int  GetMinWidthImpl(TObject* owner);
    static void SetMinWidthImpl(TObject* owner, const int& value);
    static int  GetMinHeightImpl(TObject* owner);
    static void SetMinHeightImpl(TObject* owner, const int& value);
    static int  GetMaxWidthImpl(TObject* owner);
    static void SetMaxWidthImpl(TObject* owner, const int& value);
    static int  GetMaxHeightImpl(TObject* owner);
    static void SetMaxHeightImpl(TObject* owner, const int& value);
};

// コントロールの周りの余白(LCL の TControlBorderSpacing。VCL には無く、VCL の Margins に近い)。
// コントロールが所有するものへの非所有のラッパー(docs/adr/0034)。Align・Anchors で配置するときに、親・隣との間を空ける。
// 各辺の余白は Around + その辺の値。InnerBorder はコントロールの内側の余白(AutoSize のときに使われる)。
class TControlBorderSpacing : public TPersistent
{
public:
    Property<int> Left;
    Property<int> Top;
    Property<int> Right;
    Property<int> Bottom;
    Property<int> Around;
    Property<int> InnerBorder;

    explicit TControlBorderSpacing(ObjectHandle handle);
    ~TControlBorderSpacing() override = default;

private:
    static int  GetLeftImpl(TObject* owner);
    static void SetLeftImpl(TObject* owner, const int& value);
    static int  GetTopImpl(TObject* owner);
    static void SetTopImpl(TObject* owner, const int& value);
    static int  GetRightImpl(TObject* owner);
    static void SetRightImpl(TObject* owner, const int& value);
    static int  GetBottomImpl(TObject* owner);
    static void SetBottomImpl(TObject* owner, const int& value);
    static int  GetAroundImpl(TObject* owner);
    static void SetAroundImpl(TObject* owner, const int& value);
    static int  GetInnerBorderImpl(TObject* owner);
    static void SetInnerBorderImpl(TObject* owner, const int& value);
};

// ---- パネルの縁・スクロール・コントロールの枠(docs/adr/0048。値の順は LCL と同じ) ----

// フォームの枠(bsDialog は大きさを変えられず、最小化・最大化のボタンが無い。docs/adr/0041)。
enum TFormBorderStyle { bsNone, bsSingle, bsSizeable, bsDialog, bsToolWindow, bsSizeToolWin };
// コントロールの枠(bsNone か bsSingle だけを使う。LCL・VCL の TBorderStyle は TFormBorderStyle の bsNone..bsSingle の範囲)。
using TBorderStyle = TFormBorderStyle;
// パネルの縁の凹凸(bvSpace は凹凸の無い余白)。
enum TPanelBevel { bvNone, bvLowered, bvRaised, bvSpace };
// 文字の縦の揃え(TPanel の VerticalAlignment)。
enum TVerticalAlignment { taAlignTop, taAlignBottom, taVerticalCenter };
// スクロールバーの出し方。ssAuto… は、内容がはみ出したときだけ出す。
enum TScrollStyle { ssNone, ssHorizontal, ssVertical, ssBoth, ssAutoHorizontal, ssAutoVertical, ssAutoBoth };

enum TScrollBarKind { sbHorizontal, sbVertical };

// TForm・TScrollBox の横・縦のスクロールバー(LCL の TControlScrollBar)。コントロールが所有するものへの非所有のラッパー。
// Range を領域の幅・高さより大きくすると、スクロールバーが出る(AutoScroll が true なら、Range は子の配置から LCL が決める)。
class TControlScrollBar : public TPersistent
{
public:
    // sbHorizontal か sbVertical。
    ReadOnlyProperty<TScrollBarKind> Kind;
    // スクロールバーの太さ(ピクセル)。
    ReadOnlyProperty<int> Size;
    // 矢印を押したときに動く量と、つまみの外を押したときに動く量(ピクセル)。
    Property<int>  Increment;
    Property<int>  Page;
    // 位置(0 から Range - 領域の大きさ まで)。
    Property<int>  Position;
    // スクロールする範囲の大きさ(ピクセル)。
    Property<int>  Range;
    // true なら、Increment を領域の大きさから決める。
    Property<bool> Smooth;
    // true なら、つまみをドラッグしている間も表示を動かす。
    Property<bool> Tracking;
    // false なら、Range が大きくてもスクロールバーを出さない。
    Property<bool> Visible;

    // 今スクロールバーが表示されているか。
    bool IsScrollBarVisible() const;

    explicit TControlScrollBar(ObjectHandle handle);
    ~TControlScrollBar() override = default;

private:
    static TScrollBarKind GetKindImpl(TObject* owner);
    static int  GetSizeImpl(TObject* owner);
    static int  GetIncrementImpl(TObject* owner);
    static void SetIncrementImpl(TObject* owner, const int& value);
    static int  GetPageImpl(TObject* owner);
    static void SetPageImpl(TObject* owner, const int& value);
    static int  GetPositionImpl(TObject* owner);
    static void SetPositionImpl(TObject* owner, const int& value);
    static int  GetRangeImpl(TObject* owner);
    static void SetRangeImpl(TObject* owner, const int& value);
    static bool GetSmoothImpl(TObject* owner);
    static void SetSmoothImpl(TObject* owner, const bool& value);
    static bool GetTrackingImpl(TObject* owner);
    static void SetTrackingImpl(TObject* owner, const bool& value);
    static bool GetVisibleImpl(TObject* owner);
    static void SetVisibleImpl(TObject* owner, const bool& value);
};

// ---- リスト・コンボの選択・チェックの 3 状態・Application(docs/adr/0043。値の順は LCL と同じ) ----

// コンボボックスの見た目と入力(csDropDownList は一覧から選ぶだけ。csOwnerDraw… は Tier B のオーナードロー)。
enum TComboBoxStyle
{
    csDropDown, csSimple, csDropDownList, csOwnerDrawFixed, csOwnerDrawVariable,
    csOwnerDrawEditableFixed, csOwnerDrawEditableVariable
};
// チェックボックスの状態。
enum TCheckBoxState { cbUnchecked, cbChecked, cbGrayed };

// リストボックスの選択が変わったとき(User は利用者の操作によるものか)。
using TSelectionChangeEvent = std::function<void(TObject* Sender, bool User)>;
// Application->OnIdle。Done を false にすると、すぐにもう一度呼ばれる。
using TIdleEvent = std::function<void(TObject* Sender, bool& Done)>;
// Application->OnException。ハンドラから送出された例外(E.ClassName()・E.Message)。
using TExceptionEvent = std::function<void(TObject* Sender, const Exception& E)>;
// フォームにファイルをドロップしたとき(docs/adr/0047)。FileNames はフルパス(UTF-8)。
using TDropFilesEvent = std::function<void(TObject* Sender, const std::vector<std::string>& FileNames)>;

// ---- テキストの表示・入力(docs/adr/0042。値の順は LCL と同じ) ----

// 文字の横の揃え(LCL の TAlignment)。
enum TAlignment     { taLeftJustify, taRightJustify, taCenter };
// 文字の縦の揃え(TLabel の Layout)。
enum TTextLayout    { tlTop, tlCenter, tlBottom };
// 入力した文字の表示のしかた(TEdit の EchoMode)。emPassword は PasswordChar(既定は *)で伏せる。
enum TEchoMode      { emNormal, emNone, emPassword };
// 入力した英字のそろえ方(TEdit の CharCase。VCL と同じ綴り)。
enum TEditCharCase  { ecNormal, ecUpperCase, ecLowerCase };

class TControl : public TComponent
{
public:
    // クライアント領域(枠・タイトルバー・メニューの内側)の幅。設定するとそれに合わせて Width が変わる。
    Property<int> ClientWidth;
    // クライアント領域の高さ。
    Property<int> ClientHeight;
    // 再描画を依頼する(描画はメッセージの処理のときに行われる)。
    void Invalidate();
    // すぐに再描画する。
    void Repaint();
    // すぐに再描画する(Repaint と同じ)。
    void Refresh();
    // 再描画を依頼済みの部分を、すぐに描画する。
    void Update();
    // 兄弟の中で一番手前にする。
    void BringToFront();
    // 兄弟の中で一番奥にする。
    void SendToBack();
    // 位置と大きさをまとめて設定する(Left・Top・Width・Height を 1 つずつ設定するより、配置の計算が 1 回で済む)。
    void SetBounds(int ALeft, int ATop, int AWidth, int AHeight);
    // 割り当てた Action(docs/adr/0046)。割り当てると Action の Caption・Enabled・Hint・Visible 等がコントロールに写り、以後も連動する。
    // クリックすると、OnClick(設定していれば)の後に Action の OnExecute が呼ばれる(LCL の仕様。VCL は OnClick だけを呼ぶ)。
    Property<TBasicAction*> Action;

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
    // true にすると、内容(TImage なら画像)に合わせて大きさを LCL が決める(docs/adr/0029)。
    Property<bool>         AutoSize;
    // 右クリックで開くメニュー。C++ ラッパーを介さずに作られたメニューの場合は nullptr になる。
    Property<TPopupMenu*>  PopupMenu;
    // 背景色(既定は clDefault。docs/adr/0033)。
    Property<TColor>       Color;
    // 文字のフォント。コントロールが所有する TFont のビューで、コントロールと寿命が一致する(docs/adr/0033)。
    // 代入は内容のコピー(nullptr なら何もしない)。Font->Assign(FontDialog1->Font) と同じ。
    Property<TFont*>       Font;
    // true(既定)なら、Parent の Color・Font を使う(Color・Font を設定すると false になる)。
    // LCL では TControl の protected で、ほとんどの具象クラスが published にしている(docs/adr/0034)。
    Property<bool>         ParentColor;
    Property<bool>         ParentFont;
    // 親の辺との距離を保つ辺(既定は akLeft・akTop。docs/adr/0034)。親の大きさが変わると、それに合わせて位置・大きさが変わる。
    Property<TAnchors>     Anchors;
    // 周りの余白・大きさの制限。コントロールが所有するもののビューで、代入は内容のコピー(nullptr なら何もしない)。
    Property<TControlBorderSpacing*> BorderSpacing;
    Property<TSizeConstraints*>      Constraints;
    // マウスを重ねたときに表示する文字列。表示するのは ShowHint が true のとき(ParentShowHint が true なら Parent に従う)。
    Property<std::string>  Hint;
    Property<bool>         ShowHint;
    Property<bool>         ParentShowHint;
    Property<TCursor>      Cursor;
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
    explicit TControl(ObjectHandle handle);
    ~TControl() override = default;

    // LCL では TControl の protected。TCustomEdit / TCustomComboBox が公開する。
    Property<std::string>  Text;

private:
    friend class TCoolBand;  // TCoolBand::Control の Getter から FromHandle を使うため

    TFont                 font_;
    TControlBorderSpacing borderSpacing_;
    TSizeConstraints      constraints_;

    TNotifyEvent onClick_;
    bool         onClickHooked_ = false;
    static void BETH_CALL ClickTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnClickImpl(TObject* owner);
    static void         SetOnClickImpl(TObject* owner, const TNotifyEvent& value);

    static TBasicAction* GetActionImpl(TObject* owner);
    static void          SetActionImpl(TObject* owner, TBasicAction* const& value);

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

    static void BETH_CALL DblClickTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL ResizeTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL MouseEnterTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL MouseLeaveTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL MouseDownTrampoline(ObjectHandle sender, internal::int_t button, internal::int_t shift, internal::int_t x, internal::int_t y, void* data);
    static void BETH_CALL MouseUpTrampoline(ObjectHandle sender, internal::int_t button, internal::int_t shift, internal::int_t x, internal::int_t y, void* data);
    static void BETH_CALL MouseMoveTrampoline(ObjectHandle sender, internal::int_t shift, internal::int_t x, internal::int_t y, void* data);
    static void BETH_CALL MouseWheelTrampoline(ObjectHandle sender, internal::int_t shift, internal::int_t wheelDelta, internal::int_t x, internal::int_t y, internal::bool_t* handled, void* data);

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
    static bool         GetAutoSizeImpl(TObject* owner);
    static void         SetAutoSizeImpl(TObject* owner, const bool& value);
    static TPopupMenu*  GetPopupMenuImpl(TObject* owner);
    static void         SetPopupMenuImpl(TObject* owner, TPopupMenu* const& value);
    static TColor       GetColorImpl(TObject* owner);
    static void         SetColorImpl(TObject* owner, const TColor& value);
    static TFont*       GetFontImpl(TObject* owner);
    static void         SetFontImpl(TObject* owner, TFont* const& value);
    static bool         GetParentColorImpl(TObject* owner);
    static void         SetParentColorImpl(TObject* owner, const bool& value);
    static bool         GetParentFontImpl(TObject* owner);
    static void         SetParentFontImpl(TObject* owner, const bool& value);
    static TAnchors     GetAnchorsImpl(TObject* owner);
    static void         SetAnchorsImpl(TObject* owner, const TAnchors& value);
    static TControlBorderSpacing* GetBorderSpacingImpl(TObject* owner);
    static void                   SetBorderSpacingImpl(TObject* owner, TControlBorderSpacing* const& value);
    static TSizeConstraints*      GetConstraintsImpl(TObject* owner);
    static void                   SetConstraintsImpl(TObject* owner, TSizeConstraints* const& value);
    static std::string  GetHintImpl(TObject* owner);
    static void         SetHintImpl(TObject* owner, const std::string& value);
    static bool         GetShowHintImpl(TObject* owner);
    static void         SetShowHintImpl(TObject* owner, const bool& value);
    static bool         GetParentShowHintImpl(TObject* owner);
    static void         SetParentShowHintImpl(TObject* owner, const bool& value);
    static TCursor      GetCursorImpl(TObject* owner);
    static void         SetCursorImpl(TObject* owner, const TCursor& value);
    static std::string  GetTextImpl(TObject* owner);
    static void         SetTextImpl(TObject* owner, const std::string& value);

private:
    static int GetClientWidthImpl(TObject* owner);
    static void SetClientWidthImpl(TObject* owner, const int& value);
    static int GetClientHeightImpl(TObject* owner);
    static void SetClientHeightImpl(TObject* owner, const int& value);
};

class TWinControl : public TControl
{
public:
    // フォーカスを移す(表示されていない・無効なコントロールには移せず、例外になる。CanFocus で確かめる)。
    void SetFocus();
    // フォーカスを移せるか(自分と親がすべて表示され、有効か)。
    bool CanFocus() const;
    // フォーカスを持っているか。
    bool Focused() const;
    // フォーカスを受けたとき・失ったとき。
    Property<TNotifyEvent> OnEnter;
    Property<TNotifyEvent> OnExit;

    Property<TKeyEvent>      OnKeyDown;
    Property<TKeyEvent>      OnKeyUp;
    Property<TKeyPressEvent> OnKeyPress;
    // Tab キーでのフォーカスの移動の順(同じ Parent の中での位置。-1 は末尾)と、移動の対象にするか(docs/adr/0034)。
    Property<int>            TabOrder;
    Property<bool>           TabStop;

protected:
    explicit TWinControl(ObjectHandle handle);

    // 内側の余白(ピクセル)。子を置ける範囲(Align で寄せる範囲)が、四辺ともこの幅だけ狭くなる(docs/adr/0048)。
    // LCL と同じく、TCustomPanel・TCustomForm・TCustomListView・TCustomTreeView が using で公開する(TTabSheet は、LCL が公開しているが
    // Windows では効かないため公開しない)。
    Property<int> BorderWidth;
    // 枠(bsNone・bsSingle)。LCL と同じく、枠を持てるクラス(TCustomEdit・TCustomListBox・TCustomComboBox・TCustomListView・
    // TCustomControl)が using で公開する(docs/adr/0048)。
    Property<TBorderStyle> BorderStyle;
    ~TWinControl() override = default;

private:
    TKeyEvent      onKeyDown_;
    TKeyEvent      onKeyUp_;
    TKeyPressEvent onKeyPress_;
    bool onKeyDownHooked_  = false;
    bool onKeyUpHooked_    = false;
    bool onKeyPressHooked_ = false;

    static void BETH_CALL KeyDownTrampoline(ObjectHandle sender, internal::int_t* key, internal::int_t shift, void* data);
    static void BETH_CALL KeyUpTrampoline(ObjectHandle sender, internal::int_t* key, internal::int_t shift, void* data);
    static void BETH_CALL KeyPressTrampoline(ObjectHandle sender, internal::int_t* key, void* data);

    static TKeyEvent GetOnKeyDownImpl(TObject* owner);
    static void      SetOnKeyDownImpl(TObject* owner, const TKeyEvent& value);
    static TKeyEvent GetOnKeyUpImpl(TObject* owner);
    static void      SetOnKeyUpImpl(TObject* owner, const TKeyEvent& value);
    static TKeyPressEvent GetOnKeyPressImpl(TObject* owner);
    static void           SetOnKeyPressImpl(TObject* owner, const TKeyPressEvent& value);
    static int            GetTabOrderImpl(TObject* owner);
    static void           SetTabOrderImpl(TObject* owner, const int& value);
    static bool           GetTabStopImpl(TObject* owner);
    static void           SetTabStopImpl(TObject* owner, const bool& value);

private:
    TNotifyEvent onEnter_;
    TNotifyEvent onExit_;
    bool         onEnterHooked_ = false;
    bool         onExitHooked_ = false;
    static void BETH_CALL EnterTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL ExitTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnEnterImpl(TObject* owner);
    static void         SetOnEnterImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnExitImpl(TObject* owner);
    static void         SetOnExitImpl(TObject* owner, const TNotifyEvent& value);

    // ---- docs/adr/0048 ----
    static int          GetBorderWidthImpl(TObject* owner);
    static void         SetBorderWidthImpl(TObject* owner, const int& value);
    static TBorderStyle GetBorderStyleImpl(TObject* owner);
    static void         SetBorderStyleImpl(TObject* owner, const TBorderStyle& value);
};

// ---- 範囲のコントロール・グループの列(docs/adr/0049。値の順は LCL と同じ) ----

enum TTrackBarOrientation { trHorizontal, trVertical };
// 目盛りを付ける側(tmBottomRight は横なら下、縦なら右)。
enum TTickMark { tmBottomRight, tmTopLeft, tmBoth };
enum TTickStyle { tsNone, tsAuto, tsManual };
enum TProgressBarOrientation { pbHorizontal, pbVertical, pbRightToLeft, pbTopDown };
enum TProgressBarStyle { pbstNormal, pbstMarquee };
enum TUDOrientation { udHorizontal, udVertical };
enum TUDAlignButton { udLeft, udRight, udTop, udBottom };
enum TColumnLayout { clHorizontalThenVertical, clVerticalThenHorizontal };
// スクロールバーの操作(TScrollBar の OnScroll。Windows の SB_… と同じ値)。
enum TScrollCode { scLineUp, scLineDown, scPageUp, scPageDown, scPosition, scTrack, scTop, scBottom, scEndScroll };

// TScrollBar の OnScroll。ScrollPos を変えると、つまみの位置がその値になる。
using TScrollEvent = std::function<void(TObject* Sender, TScrollCode ScrollCode, int& ScrollPos)>;
// TCheckGroup の OnItemClick。Index はチェックを切り替えた項目。
using TCheckGroupClicked = std::function<void(TObject* Sender, int Index)>;

// つまみを左右または上下にドラッグして値を選ぶスクロールバー(TScrollBarKind は docs/adr/0048 の区間)。

class TCustomScrollBar : public TWinControl
{
public:
    Property<TScrollBarKind> Kind;
    Property<int>            Min;
    Property<int>            Max;
    Property<int>            Position;
    Property<int>            PageSize;
    Property<TNotifyEvent>   OnChange;

    // ---- docs/adr/0049 ----
    // つまみの外を押したとき・矢印を押したときに動く量。
    Property<int> LargeChange;
    Property<int> SmallChange;
    // つまみ・矢印を操作したとき(OnChange より先に呼ばれる)。
    Property<TScrollEvent> OnScroll;

protected:
    explicit TCustomScrollBar(ObjectHandle handle);
    ~TCustomScrollBar() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);

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

    // ---- docs/adr/0049 ----
    static int GetLargeChangeImpl(TObject* owner);
    static void SetLargeChangeImpl(TObject* owner, const int& value);
    static int GetSmallChangeImpl(TObject* owner);
    static void SetSmallChangeImpl(TObject* owner, const int& value);
    TScrollEvent onScroll_;
    bool onScrollHooked_ = false;
    static TScrollEvent GetOnScrollImpl(TObject* owner);
    static void SetOnScrollImpl(TObject* owner, const TScrollEvent& value);
    static void BETH_CALL ScrollTrampoline(ObjectHandle sender, internal::int_t code, internal::int_t* pos, void* data);
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

    // ---- docs/adr/0049 ----
    // 縦(trVertical)か横か。
    Property<TTrackBarOrientation> Orientation;
    // 目盛りの間隔(TickStyle が tsAuto のとき)。
    Property<int> Frequency;
    // 目盛りを付ける側。
    Property<TTickMark> TickMarks;
    // 目盛りの付け方(tsNone は付けない)。
    Property<TTickStyle> TickStyle;
    // 矢印キーで動く量と、PageUp・PageDown で動く量。
    Property<int> LineSize;
    Property<int> PageSize;
    // 選択の範囲として強調する区間(ShowSelRange が true のとき)。
    Property<int> SelStart;
    Property<int> SelEnd;
    Property<bool> ShowSelRange;
    // true なら、Min と Max の側を入れ替える。
    Property<bool> Reversed;

protected:
    explicit TCustomTrackBar(ObjectHandle handle);
    ~TCustomTrackBar() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);

    static int          GetMinImpl(TObject* owner);
    static void         SetMinImpl(TObject* owner, const int& value);
    static int          GetMaxImpl(TObject* owner);
    static void         SetMaxImpl(TObject* owner, const int& value);
    static int          GetPositionImpl(TObject* owner);
    static void         SetPositionImpl(TObject* owner, const int& value);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);

    // ---- docs/adr/0049 ----
    static TTrackBarOrientation GetOrientationImpl(TObject* owner);
    static void SetOrientationImpl(TObject* owner, const TTrackBarOrientation& value);
    static int GetFrequencyImpl(TObject* owner);
    static void SetFrequencyImpl(TObject* owner, const int& value);
    static TTickMark GetTickMarksImpl(TObject* owner);
    static void SetTickMarksImpl(TObject* owner, const TTickMark& value);
    static TTickStyle GetTickStyleImpl(TObject* owner);
    static void SetTickStyleImpl(TObject* owner, const TTickStyle& value);
    static int GetLineSizeImpl(TObject* owner);
    static void SetLineSizeImpl(TObject* owner, const int& value);
    static int GetPageSizeImpl(TObject* owner);
    static void SetPageSizeImpl(TObject* owner, const int& value);
    static int GetSelStartImpl(TObject* owner);
    static void SetSelStartImpl(TObject* owner, const int& value);
    static int GetSelEndImpl(TObject* owner);
    static void SetSelEndImpl(TObject* owner, const int& value);
    static bool GetShowSelRangeImpl(TObject* owner);
    static void SetShowSelRangeImpl(TObject* owner, const bool& value);
    static bool GetReversedImpl(TObject* owner);
    static void SetReversedImpl(TObject* owner, const bool& value);
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

    // ---- docs/adr/0049 ----
    // 伸びる向き(pbVertical は下から上)。
    Property<TProgressBarOrientation> Orientation;
    // true なら、区切りの無い棒で描く。
    Property<bool> Smooth;
    // StepIt で進める量。
    Property<int> Step;
    // pbstMarquee は、進み具合の分からない処理の間に動き続ける表示。
    Property<TProgressBarStyle> Style;
    // true なら、進み具合を文字でも表示する(Windows では表示されないことがある)。
    Property<bool> BarShowText;
    // Position を Step だけ進める・Delta だけ進める(Max を超えると Min に戻る)。
    void StepIt();
    void StepBy(int Delta);

protected:
    explicit TCustomProgressBar(ObjectHandle handle);
    ~TCustomProgressBar() override = default;

private:
    static int  GetMinImpl(TObject* owner);
    static void SetMinImpl(TObject* owner, const int& value);
    static int  GetMaxImpl(TObject* owner);
    static void SetMaxImpl(TObject* owner, const int& value);
    static int  GetPositionImpl(TObject* owner);
    static void SetPositionImpl(TObject* owner, const int& value);

    // ---- docs/adr/0049 ----
    static TProgressBarOrientation GetOrientationImpl(TObject* owner);
    static void SetOrientationImpl(TObject* owner, const TProgressBarOrientation& value);
    static bool GetSmoothImpl(TObject* owner);
    static void SetSmoothImpl(TObject* owner, const bool& value);
    static int GetStepImpl(TObject* owner);
    static void SetStepImpl(TObject* owner, const int& value);
    static TProgressBarStyle GetStyleImpl(TObject* owner);
    static void SetStyleImpl(TObject* owner, const TProgressBarStyle& value);
    static bool GetBarShowTextImpl(TObject* owner);
    static void SetBarShowTextImpl(TObject* owner, const bool& value);
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
    explicit TGraphicControl(ObjectHandle handle) : TControl(handle) {}
    ~TGraphicControl() override = default;
};

// 自分で描くことのできるウィンドウのコントロール(TForm・TPanel・TScrollBox・グリッド等の基底)。
class TCustomControl : public TWinControl
{
public:
    using TWinControl::BorderStyle;

    // 描く先(docs/adr/0045)。OnPaint(グリッドは OnDrawCell)の中で描く。コントロールが所有する実体への非所有のビュー
    // (TPaintBox::Canvas と同じ)。OnPaint の外で描いたものは、次の再描画で消える。
    ReadOnlyProperty<TCanvas*> Canvas;

protected:
    explicit TCustomControl(ObjectHandle handle);
    ~TCustomControl() override = default;

    // 描き直すとき(LCL では protected。TForm・TPanel・TScrollBox が公開する)。描くのは Canvas に。
    // 描き直させるには Invalidate() を呼ぶ。
    Property<TNotifyEvent> OnPaint;

private:
    // Canvas の実体への非所有のラッパー(最初に読んだときに作る。LCL の Canvas はコントロールの破棄まで同じもの)。
    std::unique_ptr<TCanvas> canvas_;
    static TCanvas* GetCanvasImpl(TObject* owner);
    TNotifyEvent onPaint_;
    bool         onPaintHooked_ = false;
    static void BETH_CALL PaintTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnPaintImpl(TObject* owner);
    static void         SetOnPaintImpl(TObject* owner, const TNotifyEvent& value);
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

    // ---- docs/adr/0049 ----
    // 矢印の向き(udVertical は上下、udHorizontal は左右)。
    Property<TUDOrientation> Orientation;
    // Associate のどちら側に付けるか。
    Property<TUDAlignButton> AlignButton;
    // true なら、Max を超えると Min に戻る(逆も)。
    Property<bool> Wrap;
    // true なら、Associate の上で矢印キーを押すと値が変わる。
    Property<bool> ArrowKeys;
    // true なら、Associate に表示する値に 3 桁ごとの区切りを入れる。
    Property<bool> Thousands;

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

    // ---- docs/adr/0049 ----
    static TUDOrientation GetOrientationImpl(TObject* owner);
    static void SetOrientationImpl(TObject* owner, const TUDOrientation& value);
    static TUDAlignButton GetAlignButtonImpl(TObject* owner);
    static void SetAlignButtonImpl(TObject* owner, const TUDAlignButton& value);
    static bool GetWrapImpl(TObject* owner);
    static void SetWrapImpl(TObject* owner, const bool& value);
    static bool GetArrowKeysImpl(TObject* owner);
    static void SetArrowKeysImpl(TObject* owner, const bool& value);
    static bool GetThousandsImpl(TObject* owner);
    static void SetThousandsImpl(TObject* owner, const bool& value);
};

/* ---------------- Form ---------------- */

class TScrollingWinControl : public TCustomControl
{
public:
    // true なら、子がはみ出したときにスクロールバーを出す(Range を子の配置から決める。docs/adr/0048)。
    Property<bool>               AutoScroll;
    // 横・縦のスクロールバー。代入は内容のコピー。
    Property<TControlScrollBar*> HorzScrollBar;
    Property<TControlScrollBar*> VertScrollBar;

protected:
    explicit TScrollingWinControl(ObjectHandle handle);
    ~TScrollingWinControl() override = default;

private:
    TControlScrollBar horzScrollBar_;
    TControlScrollBar vertScrollBar_;

    static bool               GetAutoScrollImpl(TObject* owner);
    static void               SetAutoScrollImpl(TObject* owner, const bool& value);
    static TControlScrollBar* GetHorzScrollBarImpl(TObject* owner);
    static void               SetHorzScrollBarImpl(TObject* owner, TControlScrollBar* const& value);
    static TControlScrollBar* GetVertScrollBarImpl(TObject* owner);
    static void               SetVertScrollBarImpl(TObject* owner, TControlScrollBar* const& value);
};

// スクロール可能な汎用コンテナ。TScrollingWinControl の直接の派生で、追加のメンバは無い。
class TScrollBox : public TScrollingWinControl
{
public:
    using TCustomControl::OnPaint;

    explicit TScrollBox(TComponent* AOwner);

protected:
    ~TScrollBox() override = default;
};

// ---- フォームの表示・ModalResult(docs/adr/0041。値の順は LCL と同じ) ----

// モーダルの結果(LCL・VCL と同じ値)。ボタンの ModalResult を設定すると、押したときにフォームが閉じて ShowModal() がその値を返す。
// VCL と同じく、利用者が独自の値(mrOk + 100 等)も使えるよう、列挙型ではなく整数にする。
using TModalResult = std::int32_t;
const TModalResult mrNone     = 0;
const TModalResult mrOk       = 1;
const TModalResult mrCancel   = 2;
const TModalResult mrAbort    = 3;
const TModalResult mrRetry    = 4;
const TModalResult mrIgnore   = 5;
const TModalResult mrYes      = 6;
const TModalResult mrNo       = 7;
const TModalResult mrAll      = 8;
const TModalResult mrNoToAll  = 9;
const TModalResult mrYesToAll = 10;
const TModalResult mrClose    = 11;

// 最初に表示する位置(poDesigned は Left・Top のまま。poMainFormCenter はメインフォームの中央)。
enum TPosition
{
    poDesigned, poDefault, poDefaultPosOnly, poDefaultSizeOnly, poScreenCenter,
    poDesktopCenter, poMainFormCenter, poOwnerFormCenter, poWorkAreaCenter
};
enum TWindowState { wsNormal, wsMinimized, wsMaximized, wsFullScreen };
// タイトルバーのボタン。
enum TBorderIcon { biSystemMenu, biMinimize, biMaximize, biHelp };
using TBorderIcons = Set<TBorderIcon>;
// fsStayOnTop は常に手前に表示する。MDI(fsMDIChild・fsMDIForm)は LCL の Win32 でも対応が限られる。
enum TFormStyle { fsNormal, fsMDIChild, fsMDIForm, fsStayOnTop, fsSplash, fsSystemStayOnTop };

class TCustomForm : public TScrollingWinControl
{
public:
    using TWinControl::BorderWidth;
    using TCustomControl::OnPaint;

    // LCL の TCustomForm は Show/Hide を独自に宣言している(TControl のものを隠す)。
    void Show();
    void Hide();
    // モーダルで表示し、閉じられたときの ModalResult を返す(× で閉じたときは mrCancel)。
    TModalResult ShowModal();
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

    // モーダルの結果。モーダルで表示中に mrNone 以外を設定すると、フォームが閉じて ShowModal() がその値を返す(docs/adr/0041)。
    Property<TModalResult>     ModalResult;
    Property<TFormBorderStyle> BorderStyle;
    Property<TPosition>        Position;
    Property<TWindowState>     WindowState;
    Property<TBorderIcons>     BorderIcons;
    Property<TFormStyle>       FormStyle;
    // true なら、キーの入力を子のコントロールより先にフォームの OnKeyDown・OnKeyPress・OnKeyUp が受ける。
    Property<bool>             KeyPreview;
    // フォーカスを持つ(表示したときに持たせる)コントロール。
    Property<TWinControl*>     ActiveControl;

    // ---- docs/adr/0047 ----
    // タイトルバー・タスクバーのアイコン。空なら Application->Icon を使う。代入は内容のコピー(nullptr なら空にする)。
    Property<TIcon*>           Icon;
    // true なら、エクスプローラー等からファイルをドロップできる(ドロップすると OnDropFiles が呼ばれる)。
    Property<bool>             AllowDropFiles;
    // ファイルをドロップしたとき。FileNames はフルパス(UTF-8)。
    Property<TDropFilesEvent>  OnDropFiles;

protected:
    explicit TCustomForm(ObjectHandle handle);
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

    static void BETH_CALL ShowTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL HideTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL ActivateTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL DeactivateTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL CloseQueryTrampoline(ObjectHandle sender, internal::bool_t* canClose, void* data);
    static void BETH_CALL CloseTrampoline(ObjectHandle sender, internal::int_t* action, void* data);
    static void BETH_CALL DestroyTrampoline(ObjectHandle sender, void* data);

    static TModalResult     GetModalResultImpl(TObject* owner);
    static void             SetModalResultImpl(TObject* owner, const TModalResult& value);
    static TFormBorderStyle GetBorderStyleImpl(TObject* owner);
    static void             SetBorderStyleImpl(TObject* owner, const TFormBorderStyle& value);
    static TPosition        GetPositionImpl(TObject* owner);
    static void             SetPositionImpl(TObject* owner, const TPosition& value);
    static TWindowState     GetWindowStateImpl(TObject* owner);
    static void             SetWindowStateImpl(TObject* owner, const TWindowState& value);
    static TBorderIcons     GetBorderIconsImpl(TObject* owner);
    static void             SetBorderIconsImpl(TObject* owner, const TBorderIcons& value);
    static TFormStyle       GetFormStyleImpl(TObject* owner);
    static void             SetFormStyleImpl(TObject* owner, const TFormStyle& value);
    static bool             GetKeyPreviewImpl(TObject* owner);
    static void             SetKeyPreviewImpl(TObject* owner, const bool& value);
    static TWinControl*     GetActiveControlImpl(TObject* owner);
    static void             SetActiveControlImpl(TObject* owner, TWinControl* const& value);

    TIcon                   icon_;
    TDropFilesEvent         onDropFiles_;
    bool                    onDropFilesHooked_ = false;
    static TIcon*           GetIconImpl(TObject* owner);
    static void             SetIconImpl(TObject* owner, TIcon* const& value);
    static bool             GetAllowDropFilesImpl(TObject* owner);
    static void             SetAllowDropFilesImpl(TObject* owner, const bool& value);
    static TDropFilesEvent  GetOnDropFilesImpl(TObject* owner);
    static void             SetOnDropFilesImpl(TObject* owner, const TDropFilesEvent& value);
    static void BETH_CALL   DropFilesTrampoline(ObjectHandle sender, internal::int_t count, internal::str_t* fileNames, void* data);

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
    static ObjectHandle pendingHandle_;
    static ObjectHandle CreateHandle(TComponent* AOwner);
};

// C++Builder の TApplication。LCL の TApplication は FCL の TCustomApplication の派生だが、
// C++Builder に合わせて TComponent 直下に置く。インスタンスはグローバル変数 Application の 1 つだけ。
//
// Application が所有するフォーム(CreateForm や new TForm(Application) で生成したもの)は、
// プログラムの終了時(main から戻った後)にまとめて破棄され、ラッパーのデストラクタも呼ばれる。
class TApplication : public TComponent
{
public:
    // 実行ファイルのフルパス。
    ReadOnlyProperty<std::string> ExeName;
    // 表示中(マウスの下のコントロール)のヒント。TStatusBar::OnHint の中で使う(docs/adr/0044)。
    Property<std::string> Hint;
    // false なら、アプリケーションのすべてのヒントを表示しない。
    Property<bool> ShowHint;
    // マウスを止めてからヒントを表示するまでの時間(ミリ秒)。
    Property<int> HintPause;
    // ヒントを表示してから消すまでの時間(ミリ秒)。
    Property<int> HintHidePause;
    // アプリケーション(のすべてのフォーム)を最小化する。
    void Minimize();
    // 最小化したアプリケーションを元に戻す。
    void Restore();
    // アプリケーションを手前に出す。
    void BringToFront();
    // メッセージの処理が終わり、待ちに入るとき。Done を false にすると、すぐにもう一度呼ばれる(既定は true)。
    Property<TIdleEvent> OnIdle;
    // イベントのハンドラから送出された例外を、既定のエラーのダイアログの代わりに受ける(docs/adr/0031)。
    Property<TExceptionEvent> OnException;
    // アプリケーションのアイコン(docs/adr/0047)。Icon が空のフォームは、これを使う。代入は内容のコピー。
    Property<TIcon*> Icon;

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

    // Windows のメッセージボックス(LCL の TApplication.MessageBox)。Flags と戻り値は Windows の MB_…・ID… の値
    // (C++ では <windows.h> の定数を使う。docs/adr/0041)。<windows.h> の MessageBox マクロ(MessageBoxA 等への置き換え)と
    // 翻訳単位ごとに名前がずれないよう、ヘッダの中で定義する。
    int MessageBox(const std::string& Text, const std::string& Caption, int Flags = 0)
    {
        return MessageBoxImpl(Text, Caption, Flags);
    }

protected:
    ~TApplication() override = default;

private:
    friend TApplication* NewApplication();
    explicit TApplication(ObjectHandle handle);

    int MessageBoxImpl(const std::string& Text, const std::string& Caption, int Flags);

    void BeginCreateForm();
    void EndCreateForm();
    static void Shutdown();

    static TForm*      GetMainFormImpl(TObject* owner);
    static bool        GetTerminatedImpl(TObject* owner);
    static std::string GetTitleImpl(TObject* owner);
    static void        SetTitleImpl(TObject* owner, const std::string& value);
    static bool        GetShowMainFormImpl(TObject* owner);
    static void        SetShowMainFormImpl(TObject* owner, const bool& value);

private:
    static std::string GetExeNameImpl(TObject* owner);
    static std::string GetHintImpl(TObject* owner);
    static void SetHintImpl(TObject* owner, const std::string& value);
    static bool GetShowHintImpl(TObject* owner);
    static void SetShowHintImpl(TObject* owner, const bool& value);
    static int GetHintPauseImpl(TObject* owner);
    static void SetHintPauseImpl(TObject* owner, const int& value);
    static int GetHintHidePauseImpl(TObject* owner);
    static void SetHintHidePauseImpl(TObject* owner, const int& value);
    TIdleEvent onIdle_;
    bool onIdleHooked_ = false;
    static void BETH_CALL IdleTrampoline(ObjectHandle sender, internal::bool_t* done, void* data);
    static TIdleEvent GetOnIdleImpl(TObject* owner);
    static void SetOnIdleImpl(TObject* owner, const TIdleEvent& value);
    TExceptionEvent onException_;
    bool onExceptionHooked_ = false;
    static void BETH_CALL ExceptionTrampoline(ObjectHandle sender, internal::str_t className, internal::str_t message, void* data);
    static TExceptionEvent GetOnExceptionImpl(TObject* owner);
    static void SetOnExceptionImpl(TObject* owner, const TExceptionEvent& value);
    TIcon icon_;
    static TIcon* GetIconImpl(TObject* owner);
    static void SetIconImpl(TObject* owner, TIcon* const& value);
};

// C++Builder と同じく、アプリケーションに 1 つのグローバル変数として公開する。
// 静的初期化の順序は規定されないため、他の翻訳単位のグローバル変数の初期化子からは使わないこと。
extern TApplication* Application;

// ---- Screen・Clipboard(docs/adr/0047) ----

// 画面(LCL の TScreen)。インスタンスはグローバル変数 Screen の 1 つだけ(LCL が持つもので、破棄しない)。
class TScreen : public TComponent
{
public:
    // crDefault 以外にすると、すべてのコントロールの上でそのカーソルになる(処理の間の crHourGlass 等。crDefault で戻す)。
    Property<TCursor> Cursor;
    // 主モニタの大きさ。
    ReadOnlyProperty<int> Width;
    ReadOnlyProperty<int> Height;
    // すべてのモニタを合わせた範囲。
    ReadOnlyProperty<int> DesktopLeft;
    ReadOnlyProperty<int> DesktopTop;
    ReadOnlyProperty<int> DesktopWidth;
    ReadOnlyProperty<int> DesktopHeight;
    // 主モニタの、タスクバーを除いた範囲。
    ReadOnlyProperty<int>   WorkAreaLeft;
    ReadOnlyProperty<int>   WorkAreaTop;
    ReadOnlyProperty<int>   WorkAreaWidth;
    ReadOnlyProperty<int>   WorkAreaHeight;
    ReadOnlyProperty<TRect> WorkAreaRect;
    // 画面の解像度(96 が 100%)。
    ReadOnlyProperty<int> PixelsPerInch;
    ReadOnlyProperty<int> MonitorCount;
    // 開いている(生成済みの)フォーム(TForm の派生だけ)。プログラムが作ったものでないフォーム(MessageDlg のダイアログ等)は nullptr。
    ReadOnlyProperty<int>                FormCount;
    ReadOnlyIndexedProperty<TForm*>      Forms;
    // アクティブなフォーム・フォーカスを持つコントロール(無いとき・プログラムが作ったものでないときは nullptr)。
    ReadOnlyProperty<TForm*>       ActiveForm;
    ReadOnlyProperty<TWinControl*> ActiveControl;
    // インストールされているフォントの名前。
    ReadOnlyProperty<TStrings*> Fonts;
    // アクティブなフォーム・フォーカスを持つコントロールが変わったとき(Sender は Screen)。
    Property<TNotifyEvent> OnActiveFormChange;
    Property<TNotifyEvent> OnActiveControlChange;

protected:
    ~TScreen() override = default;

private:
    friend TScreen* NewScreen();
    explicit TScreen(ObjectHandle handle);

    TStrings     fonts_;
    TNotifyEvent onActiveFormChange_;
    TNotifyEvent onActiveControlChange_;
    bool         onActiveFormChangeHooked_ = false;
    bool         onActiveControlChangeHooked_ = false;

    static TCursor      GetCursorImpl(TObject* owner);
    static void         SetCursorImpl(TObject* owner, const TCursor& value);
    static int          GetWidthImpl(TObject* owner);
    static int          GetHeightImpl(TObject* owner);
    static int          GetDesktopLeftImpl(TObject* owner);
    static int          GetDesktopTopImpl(TObject* owner);
    static int          GetDesktopWidthImpl(TObject* owner);
    static int          GetDesktopHeightImpl(TObject* owner);
    static int          GetWorkAreaLeftImpl(TObject* owner);
    static int          GetWorkAreaTopImpl(TObject* owner);
    static int          GetWorkAreaWidthImpl(TObject* owner);
    static int          GetWorkAreaHeightImpl(TObject* owner);
    static TRect        GetWorkAreaRectImpl(TObject* owner);
    static int          GetPixelsPerInchImpl(TObject* owner);
    static int          GetMonitorCountImpl(TObject* owner);
    static int          GetFormCountImpl(TObject* owner);
    static TForm*       GetFormsImpl(TObject* owner, int Index);
    static TForm*       GetActiveFormImpl(TObject* owner);
    static TWinControl* GetActiveControlImpl(TObject* owner);
    static TStrings*    GetFontsImpl(TObject* owner);
    static TNotifyEvent GetOnActiveFormChangeImpl(TObject* owner);
    static void         SetOnActiveFormChangeImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnActiveControlChangeImpl(TObject* owner);
    static void         SetOnActiveControlChangeImpl(TObject* owner, const TNotifyEvent& value);
    static void BETH_CALL ActiveFormChangeTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL ActiveControlChangeTrampoline(ObjectHandle sender, void* data);
};

// Application と同じく、グローバル変数として公開する。
extern TScreen* Screen;

// クリップボードの形式(Windows の形式の番号)。LCL の値は実行時に決まるため、CF_Text() 等の関数で得る。
using TClipboardFormat = std::uint32_t;
// 文字列(Windows では CF_UNICODETEXT)。
TClipboardFormat CF_Text();
// ビットマップ(Windows の CF_BITMAP)。
TClipboardFormat CF_Bitmap();
// LCL が読み込める画像の形式のどれか(HasFormat(CF_Picture()) は HasPictureFormat() と同じ)。
TClipboardFormat CF_Picture();

// クリップボード(LCL の TClipboard)。Clipboard() で得る 1 つだけで、利用者は破棄しない。
// 画像を置くのは Clipboard()->Assign(Image1->Picture)、読むのは Image1->Picture->Assign(Clipboard())。
class TClipboard : public TPersistent
{
public:
    // クリップボードの文字列(UTF-8)。文字列が無ければ空文字列。代入するとクリップボードの内容を置き換える。
    Property<std::string> AsText;
    // 今の内容が持つ形式の数と番号。
    ReadOnlyProperty<int>                        FormatCount;
    ReadOnlyIndexedProperty<TClipboardFormat>    Formats;

    // その形式の内容があるか(Clipboard()->HasFormat(CF_Text()))。
    bool HasFormat(TClipboardFormat Format) const;
    // 画像(読み込める形式のどれか)があるか。
    bool HasPictureFormat() const;
    // 内容を消す。
    void Clear();
    // Open から Close までの間に置いた内容(AsText と画像等)を、1 度にまとめて置く。
    void Open();
    void Close();
    // 画像をクリップボードに置く(内容を置き換える)。Source が nullptr なら何もしない。
    void Assign(const TPicture* Source);
    void Assign(const TGraphic* Source);

private:
    friend TClipboard* Clipboard();
    explicit TClipboard(ObjectHandle handle);

    static std::string      GetAsTextImpl(TObject* owner);
    static void             SetAsTextImpl(TObject* owner, const std::string& value);
    static int              GetFormatCountImpl(TObject* owner);
    static TClipboardFormat GetFormatsImpl(TObject* owner, int Index);
};

// クリップボード(C++Builder と同じく関数)。
TClipboard* Clipboard();

// ---- メッセージのダイアログ(docs/adr/0041。LCL の Dialogs ユニットの関数) ----

enum TMsgDlgType { mtWarning, mtError, mtInformation, mtConfirmation, mtCustom };
enum TMsgDlgBtn
{
    mbYes, mbNo, mbOK, mbCancel, mbAbort, mbRetry, mbIgnore,
    mbAll, mbNoToAll, mbYesToAll, mbHelp, mbClose
};
// 表示するボタン(TMsgDlgButtons() << mbYes << mbNo)。よく使う組み合わせは下の定数。
using TMsgDlgButtons = Set<TMsgDlgBtn>;
extern const TMsgDlgButtons mbYesNo;             // mbYes, mbNo
extern const TMsgDlgButtons mbYesNoCancel;       // mbYes, mbNo, mbCancel
extern const TMsgDlgButtons mbOKCancel;          // mbOK, mbCancel
extern const TMsgDlgButtons mbAbortRetryIgnore;  // mbAbort, mbRetry, mbIgnore

// メッセージと OK のボタンだけのダイアログ。
void ShowMessage(const std::string& Msg);
// ボタンを選ぶダイアログ。押したボタンの ModalResult(mbYes なら mrYes)を返す。題名を省くと LCL の既定の題名になる。
TModalResult MessageDlg(const std::string& Msg, TMsgDlgType DlgType, TMsgDlgButtons Buttons, int HelpCtx = 0);
TModalResult MessageDlg(const std::string& Caption, const std::string& Msg, TMsgDlgType DlgType,
                        TMsgDlgButtons Buttons, int HelpCtx = 0);
// 1 行の文字列を入力するダイアログ。取りやめたら Default を返す。
std::string InputBox(const std::string& ACaption, const std::string& APrompt, const std::string& ADefault);
// 入力した文字を隠すダイアログ。取りやめたら空文字列を返す。
std::string PasswordBox(const std::string& ACaption, const std::string& APrompt);
// 1 行の文字列を入力するダイアログ。OK なら Value を入力した文字列にして true、取りやめたら Value のままで false。
bool InputQuery(const std::string& ACaption, const std::string& APrompt, std::string& Value);

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
public:
    using TWinControl::BorderWidth;
    // ---- docs/adr/0048 ----
    // Caption の横・縦の揃えと折り返し(既定は中央)。
    Property<TAlignment>         Alignment;
    Property<TVerticalAlignment> VerticalAlignment;
    Property<bool>               WordWrap;
    // 縁の外側・内側の凹凸(既定は外側が bvRaised、内側が bvNone)と、その幅・色(clDefault は凹凸の既定の色)。
    // 縁があると、子を置ける範囲(Align で寄せる範囲)がその幅だけ狭くなる。
    Property<TPanelBevel>        BevelOuter;
    Property<TPanelBevel>        BevelInner;
    Property<int>                BevelWidth;
    Property<TColor>             BevelColor;

protected:
    explicit TCustomPanel(ObjectHandle handle);
    ~TCustomPanel() override = default;

private:
    static TAlignment         GetAlignmentImpl(TObject* owner);
    static void               SetAlignmentImpl(TObject* owner, const TAlignment& value);
    static TVerticalAlignment GetVerticalAlignmentImpl(TObject* owner);
    static void               SetVerticalAlignmentImpl(TObject* owner, const TVerticalAlignment& value);
    static bool               GetWordWrapImpl(TObject* owner);
    static void               SetWordWrapImpl(TObject* owner, const bool& value);
    static TPanelBevel        GetBevelOuterImpl(TObject* owner);
    static void               SetBevelOuterImpl(TObject* owner, const TPanelBevel& value);
    static TPanelBevel        GetBevelInnerImpl(TObject* owner);
    static void               SetBevelInnerImpl(TObject* owner, const TPanelBevel& value);
    static int                GetBevelWidthImpl(TObject* owner);
    static void               SetBevelWidthImpl(TObject* owner, const int& value);
    static TColor             GetBevelColorImpl(TObject* owner);
    static void               SetBevelColorImpl(TObject* owner, const TColor& value);
};

class TPanel : public TCustomPanel
{
public:
    using TCustomControl::OnPaint;

    explicit TPanel(TComponent* AOwner);

protected:
    ~TPanel() override = default;
};

class TCustomGroupBox : public TWinControl
{
protected:
    explicit TCustomGroupBox(ObjectHandle handle) : TWinControl(handle) {}
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

    // 文字列の一覧(TStrings。RadioGroup1->Items->Add("x") のように使う)。
    ReadOnlyProperty<TStrings*> Items;

    // ---- docs/adr/0049 ----
    // 項目を並べる列の数。
    Property<int> Columns;
    // 項目を並べる順(clHorizontalThenVertical は横に並べてから次の行)。
    Property<TColumnLayout> ColumnLayout;
    // true なら、項目をグループの高さいっぱいに広げて並べる。
    Property<bool> AutoFill;
    // ItemIndex が変わったとき(利用者の操作でも、プログラムからの代入でも)。
    Property<TNotifyEvent> OnSelectionChanged;

protected:
    explicit TCustomRadioGroup(ObjectHandle handle);
    ~TCustomRadioGroup() override = default;

private:
    TStrings items_;
    static TStrings* GetItemsImpl(TObject* owner);
    TNotifyEvent onClick_;
    bool         onClickHooked_ = false;
    static void BETH_CALL ClickTrampoline(ObjectHandle sender, void* data);

    static int           GetItemIndexImpl(TObject* owner);
    static void          SetItemIndexImpl(TObject* owner, const int& value);
    static TNotifyEvent  GetOnClickImpl(TObject* owner);
    static void          SetOnClickImpl(TObject* owner, const TNotifyEvent& value);

    // ---- docs/adr/0049 ----
    static int GetColumnsImpl(TObject* owner);
    static void SetColumnsImpl(TObject* owner, const int& value);
    static TColumnLayout GetColumnLayoutImpl(TObject* owner);
    static void SetColumnLayoutImpl(TObject* owner, const TColumnLayout& value);
    static bool GetAutoFillImpl(TObject* owner);
    static void SetAutoFillImpl(TObject* owner, const bool& value);
    TNotifyEvent onSelectionChanged_;
    bool onSelectionChangedHooked_ = false;
    static TNotifyEvent GetOnSelectionChangedImpl(TObject* owner);
    static void SetOnSelectionChangedImpl(TObject* owner, const TNotifyEvent& value);
    static void BETH_CALL SelectionChangedTrampoline(ObjectHandle sender, void* data);
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
    // 文字列の一覧(TStrings。CheckGroup1->Items->Add("x") のように使う)。
    ReadOnlyProperty<TStrings*> Items;
    // 項目ごとのチェックの状態(CheckGroup1->Checked[i] = true;)。
    IndexedProperty<bool> Checked;

    // ---- docs/adr/0049 ----
    // 項目を並べる列の数。
    Property<int> Columns;
    // 項目を並べる順(clHorizontalThenVertical は横に並べてから次の行)。
    Property<TColumnLayout> ColumnLayout;
    // true なら、項目をグループの高さいっぱいに広げて並べる。
    Property<bool> AutoFill;
    // 項目ごとに、利用者がチェックを切り替えられるか。
    IndexedProperty<bool> CheckEnabled;
    // 利用者が項目のチェックを切り替えたとき。
    Property<TCheckGroupClicked> OnItemClick;

protected:
    explicit TCustomCheckGroup(ObjectHandle handle);
    ~TCustomCheckGroup() override = default;

private:
    TStrings items_;
    static TStrings* GetItemsImpl(TObject* owner);
    static bool GetCheckedImpl(TObject* owner, int index);
    static void SetCheckedImpl(TObject* owner, int index, const bool& value);

    // ---- docs/adr/0049 ----
    static int GetColumnsImpl(TObject* owner);
    static void SetColumnsImpl(TObject* owner, const int& value);
    static TColumnLayout GetColumnLayoutImpl(TObject* owner);
    static void SetColumnLayoutImpl(TObject* owner, const TColumnLayout& value);
    static bool GetAutoFillImpl(TObject* owner);
    static void SetAutoFillImpl(TObject* owner, const bool& value);
    static bool GetCheckEnabledImpl(TObject* owner, int Index);
    static void SetCheckEnabledImpl(TObject* owner, int Index, const bool& value);
    TCheckGroupClicked onItemClick_;
    bool onItemClickHooked_ = false;
    static TCheckGroupClicked GetOnItemClickImpl(TObject* owner);
    static void SetOnItemClickImpl(TObject* owner, const TCheckGroupClicked& value);
    static void BETH_CALL ItemClickTrampoline(ObjectHandle sender, internal::int_t index, void* data);
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
public:
    // 文字の横の揃え(AutoSize が false のときに効く)。
    Property<TAlignment> Alignment;
    // 文字の縦の揃え(AutoSize が false のときに効く)。
    Property<TTextLayout> Layout;
    // 幅に合わせて折り返す(AutoSize が true なら、折り返した行の数に合わせて高さが変わる)。
    Property<bool> WordWrap;
    // 背景を塗らない(親の背景が見える)。
    Property<bool> Transparent;
    // Caption のアクセスキー(&N)を押したときにフォーカスを移すコントロール。
    Property<TWinControl*> FocusControl;
    // Caption の & をアクセスキーの印(下線)として表示する(false なら & をそのまま表示する)。
    Property<bool> ShowAccelChar;

protected:
    explicit TCustomLabel(ObjectHandle handle);
    ~TCustomLabel() override = default;

private:
    static TAlignment GetAlignmentImpl(TObject* owner);
    static void SetAlignmentImpl(TObject* owner, const TAlignment& value);
    static TTextLayout GetLayoutImpl(TObject* owner);
    static void SetLayoutImpl(TObject* owner, const TTextLayout& value);
    static bool GetWordWrapImpl(TObject* owner);
    static void SetWordWrapImpl(TObject* owner, const bool& value);
    static bool GetTransparentImpl(TObject* owner);
    static void SetTransparentImpl(TObject* owner, const bool& value);
    static TWinControl* GetFocusControlImpl(TObject* owner);
    static void SetFocusControlImpl(TObject* owner, TWinControl* const& value);
    static bool GetShowAccelCharImpl(TObject* owner);
    static void SetShowAccelCharImpl(TObject* owner, const bool& value);
};

class TLabel : public TCustomLabel
{
public:
    explicit TLabel(TComponent* AOwner);

protected:
    ~TLabel() override = default;
};

// TLabeledEdit の EditLabel。LCL が LabeledEdit の生成時に内部で作るラベルで、利用者は生成しない
// (LabeledEdit と一緒に破棄される)。Caption 等は TControl のものを使う。
class TBoundLabel : public TCustomLabel
{
protected:
    ~TBoundLabel() override = default;

private:
    friend class TComponent;  // WrapExisting から(TCustomLabeledEdit::EditLabel のラップ)
    explicit TBoundLabel(ObjectHandle handle) : TCustomLabel(handle) {}
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
    explicit TButtonControl(ObjectHandle handle);
    ~TButtonControl() override = default;

    // LCL では TButtonControl の protected。TCheckBox / TRadioButton が公開する。
    Property<bool> Checked;

private:
    static bool GetCheckedImpl(TObject* owner);
    static void SetCheckedImpl(TObject* owner, const bool& value);
};

class TCustomButton : public TButtonControl
{
public:
    // mrNone 以外なら、押したときにフォームの ModalResult をこの値にする(モーダルのフォームが閉じる。docs/adr/0041)。
    Property<TModalResult> ModalResult;
    // true なら、フォームで Enter を押したときに押される(既定のボタン)。
    Property<bool>         Default;
    // true なら、フォームで Esc を押したときに押される(取り消しのボタン)。
    Property<bool>         Cancel;

protected:
    explicit TCustomButton(ObjectHandle handle);
    ~TCustomButton() override = default;

private:
    static TModalResult GetModalResultImpl(TObject* owner);
    static void         SetModalResultImpl(TObject* owner, const TModalResult& value);
    static bool         GetDefaultImpl(TObject* owner);
    static void         SetDefaultImpl(TObject* owner, const bool& value);
    static bool         GetCancelImpl(TObject* owner);
    static void         SetCancelImpl(TObject* owner, const bool& value);
};

class TButton : public TCustomButton
{
public:
    explicit TButton(TComponent* AOwner);

protected:
    ~TButton() override = default;
};

// bkOK/bkCancel 等の定型ボタン(既定の Caption を LCL が設定する)。
enum TBitBtnKind
{
    bkCustom, bkOK, bkCancel, bkHelp, bkYes, bkNo,
    bkClose, bkAbort, bkRetry, bkIgnore, bkAll,
    bkNoToAll, bkYesToAll
};

// ボタンの画像(Glyph)の位置。
enum TButtonLayout { blGlyphLeft, blGlyphRight, blGlyphTop, blGlyphBottom };

class TCustomBitBtn : public TCustomButton
{
public:
    Property<TBitBtnKind>   Kind;
    // ボタンの画像。ボタンが所有する TBitmap のビューで、ボタンと寿命が一致する(docs/adr/0029)。
    // 代入は内容のコピー(nullptr なら画像を無くす)。代入すると NumGlyphs は画像の幅と高さの比から LCL が決め直す。
    Property<TBitmap*>      Glyph;
    // 横に並べた状態別(通常・無効・押下・下がったまま)の画像の数(1〜4)。
    Property<int>           NumGlyphs;
    Property<TButtonLayout> Layout;
    // 端から画像までの距離(-1(既定)なら画像と文字列をまとめて中央に置く)。
    Property<int>           Margin;
    // 画像と文字列の間隔。
    Property<int>           Spacing;
    // 画像リスト(docs/adr/0030)。設定すると、Glyph の代わりに Images の ImageIndex 番目の画像を表示する。
    Property<TCustomImageList*> Images;
    Property<int> ImageIndex;

protected:
    explicit TCustomBitBtn(ObjectHandle handle);
    ~TCustomBitBtn() override = default;

private:
    TBitmap glyph_;

    static TBitBtnKind   GetKindImpl(TObject* owner);
    static void          SetKindImpl(TObject* owner, const TBitBtnKind& value);
    static TBitmap*      GetGlyphImpl(TObject* owner);
    static void          SetGlyphImpl(TObject* owner, TBitmap* const& value);
    static int           GetNumGlyphsImpl(TObject* owner);
    static void          SetNumGlyphsImpl(TObject* owner, const int& value);
    static TButtonLayout GetLayoutImpl(TObject* owner);
    static void          SetLayoutImpl(TObject* owner, const TButtonLayout& value);
    static int           GetMarginImpl(TObject* owner);
    static void          SetMarginImpl(TObject* owner, const int& value);
    static int           GetSpacingImpl(TObject* owner);
    static void          SetSpacingImpl(TObject* owner, const int& value);

    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);
    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
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
public:
    // チェックの状態(cbGrayed は AllowGrayed のときだけ、利用者の操作でもなる)。
    Property<TCheckBoxState> State;
    // クリックで cbUnchecked → cbChecked → cbGrayed と 3 つの状態を切り替える。
    Property<bool> AllowGrayed;
    // State(Checked)が変わったとき(プログラムからの変更でも呼ばれる)。
    Property<TNotifyEvent> OnChange;

protected:
    explicit TCustomCheckBox(ObjectHandle handle);
    ~TCustomCheckBox() override = default;

private:
    static TCheckBoxState GetStateImpl(TObject* owner);
    static void SetStateImpl(TObject* owner, const TCheckBoxState& value);
    static bool GetAllowGrayedImpl(TObject* owner);
    static void SetAllowGrayedImpl(TObject* owner, const bool& value);
    TNotifyEvent onChange_;
    bool onChangeHooked_ = false;
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
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
    using TWinControl::BorderStyle;

    // 選択の開始位置(文字の数。0 から)。選択が無ければキャレットの位置。
    Property<int> SelStart;
    // 選択の長さ(文字の数)。
    Property<int> SelLength;
    // 選択している文字列。設定すると選択を置き換える(選択が無ければキャレットの位置に挿入する)。
    Property<std::string> SelText;
    // 利用者が内容を変えたか。Text を設定すると false に戻る。
    Property<bool> Modified;
    // 元に戻せる編集があるか。
    ReadOnlyProperty<bool> CanUndo;
    // 入力した文字の代わりに表示する文字(#0 なら隠さない)。
    Property<char> PasswordChar;
    // 入力した文字の表示のしかた(emPassword は伏せ字、emNone は表示しない)。
    Property<TEchoMode> EchoMode;
    // 入力した英字を大文字・小文字にそろえる。
    Property<TEditCharCase> CharCase;
    // 文字の横の揃え。
    Property<TAlignment> Alignment;
    // 空のときに薄く表示する説明。
    Property<std::string> TextHint;
    // 数字だけを入力できるようにする。
    Property<bool> NumbersOnly;
    // フォーカスを受けたときに全体を選択する。
    Property<bool> AutoSelect;
    // フォーカスが無いときに選択の表示を隠す。
    Property<bool> HideSelection;
    // キャレットの位置(X は行の中の文字の位置、Y は行。どちらも 0 から)。
    Property<TPoint> CaretPos;
    // 全体を選択する。
    void SelectAll();
    // 選択している文字列を消す。
    void ClearSelection();
    // 内容を空にする。
    void Clear();
    // 選択している文字列をクリップボードに写す。
    void CopyToClipboard();
    // 選択している文字列をクリップボードに移す。
    void CutToClipboard();
    // クリップボードの文字列を、選択を置き換えて貼り付ける。
    void PasteFromClipboard();
    // 直前の編集を元に戻す。
    void Undo();

    using TControl::Text;
    Property<int>          MaxLength;
    Property<bool>         ReadOnly;
    Property<TNotifyEvent> OnChange;

protected:
    explicit TCustomEdit(ObjectHandle handle);
    ~TCustomEdit() override = default;

private:
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);

    static int  GetMaxLengthImpl(TObject* owner);
    static void SetMaxLengthImpl(TObject* owner, const int& value);
    static bool GetReadOnlyImpl(TObject* owner);
    static void SetReadOnlyImpl(TObject* owner, const bool& value);

private:
    static int GetSelStartImpl(TObject* owner);
    static void SetSelStartImpl(TObject* owner, const int& value);
    static int GetSelLengthImpl(TObject* owner);
    static void SetSelLengthImpl(TObject* owner, const int& value);
    static std::string GetSelTextImpl(TObject* owner);
    static void SetSelTextImpl(TObject* owner, const std::string& value);
    static bool GetModifiedImpl(TObject* owner);
    static void SetModifiedImpl(TObject* owner, const bool& value);
    static bool GetCanUndoImpl(TObject* owner);
    static char GetPasswordCharImpl(TObject* owner);
    static void SetPasswordCharImpl(TObject* owner, const char& value);
    static TEchoMode GetEchoModeImpl(TObject* owner);
    static void SetEchoModeImpl(TObject* owner, const TEchoMode& value);
    static TEditCharCase GetCharCaseImpl(TObject* owner);
    static void SetCharCaseImpl(TObject* owner, const TEditCharCase& value);
    static TAlignment GetAlignmentImpl(TObject* owner);
    static void SetAlignmentImpl(TObject* owner, const TAlignment& value);
    static std::string GetTextHintImpl(TObject* owner);
    static void SetTextHintImpl(TObject* owner, const std::string& value);
    static bool GetNumbersOnlyImpl(TObject* owner);
    static void SetNumbersOnlyImpl(TObject* owner, const bool& value);
    static bool GetAutoSelectImpl(TObject* owner);
    static void SetAutoSelectImpl(TObject* owner, const bool& value);
    static bool GetHideSelectionImpl(TObject* owner);
    static void SetHideSelectionImpl(TObject* owner, const bool& value);
    static TPoint GetCaretPosImpl(TObject* owner);
    static void SetCaretPosImpl(TObject* owner, const TPoint& value);
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
    explicit TCustomFloatSpinEdit(ObjectHandle handle);
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
    explicit TCustomSpinEdit(ObjectHandle handle);
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

// ラベルの位置(LCL の TLabelPosition と同じ値)。
enum TLabelPosition { lpAbove, lpBelow, lpLeft, lpRight };

// ラベル付きのエディット。EditLabel は LCL が内部で生成したラベルで、初めて取得したときにラッパーが作られる(docs/adr/0028)。
// ラベルの Parent と位置は、エディットの Parent・LabelPosition・LabelSpacing に合わせて LCL が決める
// (位置の反映はフォームの配置が行われるとき。Align と同じく、表示までは行われないことがある)。
class TCustomLabeledEdit : public TCustomEdit
{
public:
    ReadOnlyProperty<TBoundLabel*> EditLabel;
    Property<TLabelPosition>       LabelPosition;
    // ラベルとエディットの間隔(既定は 3)。
    Property<int>                  LabelSpacing;

protected:
    explicit TCustomLabeledEdit(ObjectHandle handle);
    ~TCustomLabeledEdit() override = default;

private:
    static TBoundLabel*   GetEditLabelImpl(TObject* owner);
    static TLabelPosition GetLabelPositionImpl(TObject* owner);
    static void           SetLabelPositionImpl(TObject* owner, const TLabelPosition& value);
    static int            GetLabelSpacingImpl(TObject* owner);
    static void           SetLabelSpacingImpl(TObject* owner, const int& value);
};

class TLabeledEdit : public TCustomLabeledEdit
{
public:
    explicit TLabeledEdit(TComponent* AOwner);

protected:
    ~TLabeledEdit() override = default;
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
    // タブの画像リスト(docs/adr/0030)。各ページの画像は TCustomPage::ImageIndex。
    Property<TCustomImageList*> Images;

protected:
    explicit TCustomTabControl(ObjectHandle handle);
    ~TCustomTabControl() override = default;

private:
    TTabChangingEvent onChanging_;
    bool              onChangingHooked_ = false;
    static void BETH_CALL ChangingTrampoline(ObjectHandle sender, internal::bool_t* allowChange, void* data);

    static int               GetPageCountImpl(TObject* owner);
    static bool              GetMultiLineImpl(TObject* owner);
    static void              SetMultiLineImpl(TObject* owner, const bool& value);
    static bool              GetShowTabsImpl(TObject* owner);
    static void              SetShowTabsImpl(TObject* owner, const bool& value);
    static TTabPosition      GetTabPositionImpl(TObject* owner);
    static void              SetTabPositionImpl(TObject* owner, const TTabPosition& value);
    static TTabChangingEvent GetOnChangingImpl(TObject* owner);
    static void              SetOnChangingImpl(TObject* owner, const TTabChangingEvent& value);

    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);
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

    // 文字列の一覧(TStrings。TabControl1->Tabs->Add("x") のように使う)。
    ReadOnlyProperty<TStrings*> Tabs;

protected:
    ~TTabControl() override = default;

private:
    TStrings tabs_;
    static TStrings* GetTabsImpl(TObject* owner);
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);

    static int           GetTabIndexImpl(TObject* owner);
    static void          SetTabIndexImpl(TObject* owner, const int& value);
    static TNotifyEvent  GetOnChangeImpl(TObject* owner);
    static void          SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

class TTabSheet;

// ページ付きのタブ。ページ(TTabSheet)は VCL と同じく、TTabSheet を生成して PageControl を設定するか、
// AddTabSheet で追加する。ページの上のコントロールは、ページを Parent にして置く。
// Pages[Index] は読み取り専用のインデックス付きプロパティ(ReadOnlyIndexedProperty)。
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

    ReadOnlyIndexedProperty<TTabSheet*> Pages;

    // ページを末尾に追加する。ページは LCL が内部で生成し、Owner はこのページコントロールになる。
    TTabSheet* AddTabSheet();
    // すべてのページを外して破棄する。破棄は LCL の遅延破棄(Application.ReleaseComponent)で、次にメッセージを
    // 処理したとき(または Owner の破棄時)に行われ、そのときにページのラッパーも delete される。
    void       Clear();
    void       SelectNextPage(bool GoForward);

protected:
    ~TPageControl() override = default;

private:
    static TTabSheet* GetPagesImpl(TObject* owner, int Index);
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);

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
    // タブに表示する画像の、PageControl の Images での位置(-1 なら無し。docs/adr/0030)。
    Property<int> ImageIndex;

protected:
    explicit TCustomPage(ObjectHandle handle);
    ~TCustomPage() override = default;

private:
    TNotifyEvent onShow_;
    TNotifyEvent onHide_;
    bool         onShowHooked_ = false;
    bool         onHideHooked_ = false;
    static void BETH_CALL ShowTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL HideTrampoline(ObjectHandle sender, void* data);

    static int          GetPageIndexImpl(TObject* owner);
    static void         SetPageIndexImpl(TObject* owner, const int& value);
    static bool         GetTabVisibleImpl(TObject* owner);
    static void         SetTabVisibleImpl(TObject* owner, const bool& value);
    static TNotifyEvent GetOnShowImpl(TObject* owner);
    static void         SetOnShowImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnHideImpl(TObject* owner);
    static void         SetOnHideImpl(TObject* owner, const TNotifyEvent& value);

    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
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
    explicit TTabSheet(ObjectHandle handle);

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
// ノードが削除されると(ツリービューの破棄に伴う削除も含め)、OnDeletion などの削除の処理がすべて終わった後にラッパーも delete される。
// 削除後にそのポインタへ触れてはならない。
// Items[Index] は直下の子(読み取り専用のインデックス付きプロパティ)。
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
    // 直下の子(Node->Items[i])。
    ReadOnlyIndexedProperty<TTreeNode*> Items;
    // 画像の、ツリービューの Images での位置(-1 なら無し。docs/adr/0030)。SelectedIndex は選択中の画像(-1 なら ImageIndex と同じ)。
    Property<int> ImageIndex;
    Property<int> SelectedIndex;
    // StateImages での位置。OverlayIndex は重ねて描く画像の、Images での位置。
    Property<int> StateIndex;
    Property<int> OverlayIndex;

    // 以下のノードを返すメンバは、該当するノードが無ければ nullptr を返す。
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

    // ---- docs/adr/0051 ----
    // ノードの矩形(ツリービューのクライアント座標)。TextOnly なら文字の部分だけ。
    TRect DisplayRect(bool TextOnly) const;
    // ラベルの編集を始める(ReadOnly なら始めない)。EndEdit は編集を終える(Cancel なら入力を捨てる)。
    bool EditText();
    void EndEdit(bool Cancel);

private:
    static TTreeNode* GetItemsImpl(TObject* owner, int Index);
    friend class ItemRegistry;
    friend class TTreeNodes;
    friend class TCustomTreeView;
    friend class TTreeView;

    explicit TTreeNode(ObjectHandle handle);
    ~TTreeNode() override = default;

    // ハンドルからラッパーを得る(無ければ作る)。nullptr には nullptr を返す。
    static TTreeNode* Wrap(ObjectHandle handle) { return ItemRegistry::Wrap<TTreeNode>(handle); }

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

    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
    static int GetSelectedIndexImpl(TObject* owner);
    static void SetSelectedIndexImpl(TObject* owner, const int& value);
    static int GetStateIndexImpl(TObject* owner);
    static void SetStateIndexImpl(TObject* owner, const int& value);
    static int GetOverlayIndexImpl(TObject* owner);
    static void SetOverlayIndexImpl(TObject* owner, const int& value);


};

// ツリービューのノードの一覧(LCL の TTreeNodes)。ツリービューが所有する実体への非所有のビューで、
// TCanvas と同じくツリービューのメンバとして持ち、ツリービューと寿命が一致する(TCustomTreeView::Items で参照する)。
// Sibling/Parent に nullptr を渡すと最上位のノードになる(VCL と同じ)。
class TTreeNodes : public TPersistent
{
public:
    explicit TTreeNodes(ObjectHandle handle);
    ~TTreeNodes() override = default;

    // すべてのノード(子孫を含む)の数。GetItem の Index は、上から順に数えた位置(AbsoluteIndex)。
    ReadOnlyProperty<int> Count;
    // 上から順(子孫を含む)に数えた位置のノード(TreeView1->Items->Item[i])。
    ReadOnlyIndexedProperty<TTreeNode*> Item;

    TTreeNode* Add(TTreeNode* Sibling, const std::string& S);
    TTreeNode* AddFirst(TTreeNode* Sibling, const std::string& S);
    TTreeNode* AddChild(TTreeNode* Parent, const std::string& S);
    TTreeNode* AddChildFirst(TTreeNode* Parent, const std::string& S);
    // NextNode の前に挿入する。
    TTreeNode* Insert(TTreeNode* NextNode, const std::string& S);
    void       Clear();
    void       Delete(TTreeNode* Node);
    TTreeNode* GetFirstNode() const;
    TTreeNode* FindNodeWithText(const std::string& S) const;
    void       BeginUpdate();
    void       EndUpdate();

private:
    static TTreeNode* GetItemImpl(TObject* owner, int Index);
    static int GetCountImpl(TObject* owner);
};

// ノードを対象とするイベント。
using TTVChangedEvent    = std::function<void(TObject* Sender, TTreeNode* Node)>;
using TTVExpandedEvent   = TTVChangedEvent;
// AllowChange 等には true が入っており、false にすると選択の変更・展開・折りたたみを取りやめる。
using TTVChangingEvent   = std::function<void(TObject* Sender, TTreeNode* Node, bool& AllowChange)>;
using TTVExpandingEvent  = std::function<void(TObject* Sender, TTreeNode* Node, bool& AllowExpansion)>;
using TTVCollapsingEvent = std::function<void(TObject* Sender, TTreeNode* Node, bool& AllowCollapse)>;

// ---- TTreeView の細部(docs/adr/0051。値の順は LCL と同じ) ----

// 並べ替えの基準(TTreeView・TListView。stText は Text(Caption)の順、stData・stBoth は OnCompare で決める。stNone は並べ替えない)。
enum TSortType { stNone, stData, stText, stBoth };
// 複数選択のしかた(MultiSelect が true のとき。既定は msControlSelect)。
enum TMultiSelectStyles
{
    msControlSelect,  // Ctrl を押しながらクリック
    msShiftSelect,    // Shift を押しながらクリック(範囲)
    msVisibleOnly,
    msSiblingOnly
};
using TMultiSelectStyle = Set<TMultiSelectStyles>;
// ツリービューの動作・表示の設定(LCL の TTreeViewOptions)。MultiSelect・ReadOnly 等のプロパティと連動する。
enum TTreeViewOption
{
    tvoAllowMultiselect,
    tvoAutoExpand,
    tvoAutoInsertMark,
    tvoAutoItemHeight,
    tvoHideSelection,
    tvoHotTrack,
    tvoKeepCollapsedNodes,
    tvoReadOnly,
    tvoRightClickSelect,
    tvoRowSelect,
    tvoShowButtons,
    tvoShowLines,
    tvoShowRoot,
    tvoShowSeparators,
    tvoToolTips,
    tvoNoDoubleClickExpand,
    tvoThemedDraw,
    tvoEmptySpaceUnselect
};
using TTreeViewOptions = Set<TTreeViewOption>;
// OnCustomDrawItem の State(LCL の TCustomDrawState)。
enum TCustomDrawStateFlag
{
    cdsSelected,
    cdsGrayed,
    cdsDisabled,
    cdsChecked,
    cdsFocused,
    cdsDefault,
    cdsHot,
    cdsMarked,
    cdsIndeterminate
};
using TCustomDrawState = Set<TCustomDrawStateFlag>;

// 並べ替えで 2 つのノードを比べる(Node1 が前なら負、後ろなら正、同じなら 0 を Compare に入れる)。nullptr に戻すと既定の比較に戻る。
using TTVCompareEvent = std::function<void(TObject* Sender, TTreeNode* Node1, TTreeNode* Node2, int& Compare)>;
// ラベルの編集を始める前(AllowEdit を false にすると編集させない)。
using TTVEditingEvent = std::function<void(TObject* Sender, TTreeNode* Node, bool& AllowEdit)>;
// ラベルの編集を終えたとき(S は入力した文字列。書き換えると、その文字列が Text になる)。
using TTVEditedEvent = std::function<void(TObject* Sender, TTreeNode* Node, std::string& S)>;
// ノードを描く前(DefaultDraw を false にすると、既定の描画をしない。Sender->Canvas と Node->DisplayRect で描く)。
// DefaultDraw のまま Sender->Canvas の Font の色等を変えて既定の描画に使わせるときは、Options から tvoThemedDraw を外す
// (LCL は、テーマで描く(既定)ときは文字をテーマの色で描く)。
using TTVCustomDrawItemEvent =
    std::function<void(TCustomTreeView* Sender, TTreeNode* Node, TCustomDrawState State, bool& DefaultDraw)>;

// 以下のメンバは LCL の TCustomTreeView の public。
class TCustomTreeView : public TCustomControl
{
public:
    using TWinControl::BorderWidth;
    // スクロールバーの出し方(docs/adr/0048)。
    Property<TScrollStyle> ScrollBars;

    ReadOnlyProperty<TTreeNodes*> Items;
    // 選択されているノード(無ければ nullptr)。
    Property<TTreeNode*>          Selected;
    // ノードの画像リスト(docs/adr/0030)。各ノードの画像は TTreeNode::ImageIndex・SelectedIndex。
    Property<TCustomImageList*> Images;
    // 状態(チェック等)の画像リスト。各ノードの画像は TTreeNode::StateIndex。
    Property<TCustomImageList*> StateImages;

    void       FullExpand();
    void       FullCollapse();
    // ノードを文字列の順に並べ替える。
    bool       AlphaSort();
    // X, Y はクライアント座標。そこにノードが無ければ nullptr。
    TTreeNode* GetNodeAt(int X, int Y) const;

    // 動作・表示の設定(tvo… の Set。docs/adr/0051)。
    Property<TTreeViewOptions>  Options;
    Property<TMultiSelectStyle> MultiSelectStyle;
    // 選択されているノード(MultiSelect のとき。Selections[0] … Selections[SelectionCount - 1])。
    ReadOnlyProperty<int>               SelectionCount;
    ReadOnlyIndexedProperty<TTreeNode*> Selections;
    // ラベルを編集しているか。
    bool IsEditing() const;

protected:
    explicit TCustomTreeView(ObjectHandle handle);
    ~TCustomTreeView() override = default;

private:
    friend class TTreeNode;  // TTreeNode::TreeView の Getter から FromHandle を使うため

    TTreeNodes items_;

    static TTreeNodes* GetItemsImpl(TObject* owner);
    static TTreeNode*  GetSelectedImpl(TObject* owner);
    static void        SetSelectedImpl(TObject* owner, TTreeNode* const& value);

    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);
    static TCustomImageList* GetStateImagesImpl(TObject* owner);
    static void SetStateImagesImpl(TObject* owner, TCustomImageList* const& value);

    // ---- docs/adr/0048 ----
    static TScrollStyle GetScrollBarsImpl(TObject* owner);
    static void         SetScrollBarsImpl(TObject* owner, const TScrollStyle& value);

    static TTreeViewOptions  GetOptionsImpl(TObject* owner);
    static void              SetOptionsImpl(TObject* owner, const TTreeViewOptions& value);
    static TMultiSelectStyle GetMultiSelectStyleImpl(TObject* owner);
    static void              SetMultiSelectStyleImpl(TObject* owner, const TMultiSelectStyle& value);
    static int               GetSelectionCountImpl(TObject* owner);
    static TTreeNode*        GetSelectionsImpl(TObject* owner, int Index);
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

    // ---- docs/adr/0051 ----
    // true なら、複数のノードを選べる(MultiSelectStyle の操作で。選んだノードは Selections)。
    Property<bool> MultiSelect;
    // 並べ替えの基準。stText なら、ノードを加えるたびに Text の順に並ぶ。
    Property<TSortType> SortType;
    // 子のノードの字下げの幅(ピクセル)。
    Property<int> Indent;
    // true なら、マウスの下のノードを強調する。
    Property<bool> HotTrack;
    // true なら、右クリックでもノードを選ぶ。
    Property<bool> RightClickSelect;
    // true なら、はみ出したノードの文字をツールチップで表示する。
    Property<bool> ToolTips;
    Property<TTVCompareEvent>        OnCompare;
    Property<TTVEditingEvent>        OnEditing;
    Property<TTVEditedEvent>         OnEdited;
    Property<TTVCustomDrawItemEvent> OnCustomDrawItem;

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
    static void DispatchNode(ObjectHandle sender, ObjectHandle node, Event TTreeView::*slot);
    template<typename Event>
    static void DispatchNodeAllow(ObjectHandle sender, ObjectHandle node, internal::bool_t* allow, Event TTreeView::*slot);

    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, ObjectHandle node, void* data);
    static void BETH_CALL ChangingTrampoline(ObjectHandle sender, ObjectHandle node, internal::bool_t* allow, void* data);
    static void BETH_CALL ExpandingTrampoline(ObjectHandle sender, ObjectHandle node, internal::bool_t* allow, void* data);
    static void BETH_CALL ExpandedTrampoline(ObjectHandle sender, ObjectHandle node, void* data);
    static void BETH_CALL CollapsingTrampoline(ObjectHandle sender, ObjectHandle node, internal::bool_t* allow, void* data);
    static void BETH_CALL CollapsedTrampoline(ObjectHandle sender, ObjectHandle node, void* data);
    static void BETH_CALL DeletionTrampoline(ObjectHandle sender, ObjectHandle node, void* data);

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

    // ---- docs/adr/0051 ----
    static bool GetMultiSelectImpl(TObject* owner);
    static void SetMultiSelectImpl(TObject* owner, const bool& value);
    static TSortType GetSortTypeImpl(TObject* owner);
    static void SetSortTypeImpl(TObject* owner, const TSortType& value);
    static int GetIndentImpl(TObject* owner);
    static void SetIndentImpl(TObject* owner, const int& value);
    static bool GetHotTrackImpl(TObject* owner);
    static void SetHotTrackImpl(TObject* owner, const bool& value);
    static bool GetRightClickSelectImpl(TObject* owner);
    static void SetRightClickSelectImpl(TObject* owner, const bool& value);
    static bool GetToolTipsImpl(TObject* owner);
    static void SetToolTipsImpl(TObject* owner, const bool& value);
    TTVCompareEvent onCompare_;
    bool onCompareHooked_ = false;
    static TTVCompareEvent GetOnCompareImpl(TObject* owner);
    static void SetOnCompareImpl(TObject* owner, const TTVCompareEvent& value);
    static void BETH_CALL CompareTrampoline(ObjectHandle sender, ObjectHandle node1, ObjectHandle node2, internal::int_t* compare, void* data);
    TTVEditingEvent onEditing_;
    bool onEditingHooked_ = false;
    static TTVEditingEvent GetOnEditingImpl(TObject* owner);
    static void SetOnEditingImpl(TObject* owner, const TTVEditingEvent& value);
    static void BETH_CALL EditingTrampoline(ObjectHandle sender, ObjectHandle node, internal::bool_t* allow, void* data);
    TTVEditedEvent onEdited_;
    bool onEditedHooked_ = false;
    static TTVEditedEvent GetOnEditedImpl(TObject* owner);
    static void SetOnEditedImpl(TObject* owner, const TTVEditedEvent& value);
    static void BETH_CALL EditedTrampoline(ObjectHandle sender, ObjectHandle node, internal::str_t s, internal::str_t* result, void* data);
    TTVCustomDrawItemEvent onCustomDrawItem_;
    bool onCustomDrawItemHooked_ = false;
    static TTVCustomDrawItemEvent GetOnCustomDrawItemImpl(TObject* owner);
    static void SetOnCustomDrawItemImpl(TObject* owner, const TTVCustomDrawItemEvent& value);
    static void BETH_CALL CustomDrawItemTrampoline(ObjectHandle sender, ObjectHandle node, internal::uint_t state, internal::bool_t* defaultDraw, void* data);
};

/* ---------------- ListView ---------------- */

// 表示形式・並べ替え・列の文字の寄せ方・OnChange の変更の種類(LCL / VCL と同じ値)。
enum TViewStyle     { vsIcon, vsSmallIcon, vsList, vsReport };
// TSortType は TTreeView と共通(docs/adr/0051 の区間)。
enum TSortDirection { sdAscending, sdDescending };
enum TItemChange    { ctText, ctImage, ctState };
// TListItem::DisplayRect で求める矩形(項目全体・画像・文字・選択の範囲。docs/adr/0052)。
enum TDisplayCode   { drBounds, drIcon, drLabel, drSelectBounds };

class TCustomListView;
class TListView;

// リストビューの項目。TTreeNode と同じく TComponent ではないため、new/Free() はせず TListItems::Add 等で追加し、
// Delete() 等で削除する。同じ項目には常に同じポインタが返り、項目が削除されると(リストビューの破棄に伴う削除も含め)
// OnDeletion などの削除の処理がすべて終わった後にラッパーも delete される。
// Caption は 1 列目、SubItems は 2 列目以降の文字列(ViewStyle が vsReport のときに表示される)。
// SubItems(TStrings)は Item->SubItems->Add("x"); のように操作する。
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

    // 文字列の一覧(TStrings。Item->SubItems->Add("x") のように使う)。
    ReadOnlyProperty<TStrings*> SubItems;
    // 画像の、リストビューの SmallImages・LargeImages での位置(-1 なら無し。docs/adr/0030)。StateIndex は StateImages での位置。
    Property<int> ImageIndex;
    Property<int> StateIndex;

    // この項目を削除する。このラッパーも delete されるため、呼び出し後に触れてはならない。
    void Delete();
    void MakeVisible(bool PartialOK);

    // ---- docs/adr/0052 ----
    // 項目の矩形(リストビューのクライアント座標)。
    TRect DisplayRect(TDisplayCode Code) const;
    // この項目だけを選んで、ラベルの編集を始める(ReadOnly のときや OnEditing で断られたときは始めず false)。
    // 編集は Enter・フォーカスの移動で終わり(OnEdited)、Esc で取り消す。
    bool EditCaption();

private:
    TStrings subItems_;
    static TStrings* GetSubItemsImpl(TObject* owner);
    friend class ItemRegistry;
    friend class TListItems;
    friend class TCustomListView;
    friend class TListView;

    explicit TListItem(ObjectHandle handle);
    ~TListItem() override = default;
    static TListItem* Wrap(ObjectHandle handle) { return ItemRegistry::Wrap<TListItem>(handle); }

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

    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
    static int GetStateIndexImpl(TObject* owner);
    static void SetStateIndexImpl(TObject* owner, const int& value);
};

// リストビューの項目の一覧(LCL の TListItems)。TTreeNodes と同じく、リストビューの値メンバとして持つ非所有のビュー。
// Item[Index] は LCL と同じ名前(Items ではない)。
class TListItems : public TPersistent
{
public:
    explicit TListItems(ObjectHandle handle);
    ~TListItems() override = default;

    // 項目の数。OwnerData のときは、設定した数の項目を表示する(OwnerData でなければ設定しても何もしない。docs/adr/0052)。
    Property<int> Count;
    // ListView1->Items->Item[i]。
    ReadOnlyIndexedProperty<TListItem*> Item;

    // 末尾に(Insert は Index の位置に)空の項目を追加して返す(Caption 等はその後で設定する。VCL と同じ)。
    TListItem* Add();
    TListItem* Insert(int Index);
    void       Delete(int Index);
    void       Clear();
    int        IndexOf(TListItem* Item) const;
    // StartIndex の次(Inclusive なら StartIndex から)から Caption を探す。Partial なら前方一致、Wrap なら末尾から先頭へ続けて探す。
    TListItem* FindCaption(int StartIndex, const std::string& Value, bool Partial, bool Inclusive, bool Wrap) const;
    void       Exchange(int Index1, int Index2);
    void       Move(int FromIndex, int ToIndex);
    void       BeginUpdate();
    void       EndUpdate();

private:
    friend class TCustomListView;  // OwnerData を切り替えたときに、作り直された一覧のハンドルに付け替えるため
    void Rebind(ObjectHandle handle) { handle_ = handle; }

    static TListItem* GetItemImpl(TObject* owner, int Index);
    static int GetCountImpl(TObject* owner);
    static void SetCountImpl(TObject* owner, const int& value);
};

// リストビューの列(LCL の TListColumn。TCollectionItem)。項目と同じく同じ列には常に同じポインタが返る。
// 列のラッパーは、列が破棄されたとき(TListColumns::Delete・Clear、リストビューの破棄)に delete される。
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
    // 見出しの画像の、SmallImages での位置(-1 なら無し。docs/adr/0030)。
    Property<int> ImageIndex;

private:
    friend class ItemRegistry;
    friend class TListColumns;
    friend class TListView;

    explicit TListColumn(ObjectHandle handle);
    ~TListColumn() override = default;
    static TListColumn* Wrap(ObjectHandle handle) { return ItemRegistry::Wrap<TListColumn>(handle); }

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

    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
};

// リストビューの列の一覧(LCL の TListColumns)。リストビューの値メンバとして持つ非所有のビュー。Items[Index] で列を参照する。
class TListColumns : public TPersistent
{
public:
    explicit TListColumns(ObjectHandle handle);
    ~TListColumns() override = default;

    ReadOnlyProperty<int> Count;
    // ListView1->Columns->Items[i]。
    ReadOnlyIndexedProperty<TListColumn*> Items;

    TListColumn* Add();
    // 列を削除する(列のラッパーも delete される)。
    void         Delete(int Index);
    void         Clear();

private:
    static TListColumn* GetItemsImpl(TObject* owner, int Index);
    static int GetCountImpl(TObject* owner);
};

// 項目・列を対象とするイベント。
using TLVDeletedEvent     = std::function<void(TObject* Sender, TListItem* Item)>;
using TLVCheckedItemEvent = TLVDeletedEvent;
using TLVSelectItemEvent  = std::function<void(TObject* Sender, TListItem* Item, bool Selected)>;
using TLVChangeEvent      = std::function<void(TObject* Sender, TListItem* Item, TItemChange Change)>;
using TLVColumnClickEvent = std::function<void(TObject* Sender, TListColumn* Column)>;

// ---- TListView の細部(docs/adr/0052。LCL・VCL と同じ形) ----
// 並べ替えで 2 つの項目を比べる(Item1 が前なら負、後ろなら正、同じなら 0 を Compare に入れる。Data は常に 0)。
// OnCompare があると、SortDirection は使われない(降順にするときは Compare の符号を変える)。nullptr に戻すと既定の比較に戻る。
using TLVCompareEvent = std::function<void(TObject* Sender, TListItem* Item1, TListItem* Item2, int Data, int& Compare)>;
// ラベルの編集を始める前(AllowEdit を false にすると編集させない)。
using TLVEditingEvent = std::function<void(TObject* Sender, TListItem* Item, bool& AllowEdit)>;
// ラベルの編集を終えたとき(AValue は入力した文字列。書き換えると、その文字列が Caption になる)。
using TLVEditedEvent = std::function<void(TObject* Sender, TListItem* Item, std::string& AValue)>;
// OwnerData のとき、表示する項目の内容を求める(Item->Index の項目の Caption・SubItems・ImageIndex を設定する)。
using TLVDataEvent = TLVDeletedEvent;
// 項目を描く前(DefaultDraw を false にすると、既定の描画をしない)。Sender->Canvas の Font・Brush を変えて
// DefaultDraw のままにすると、その色で描かれる。
using TLVCustomDrawItemEvent =
    std::function<void(TCustomListView* Sender, TListItem* Item, TCustomDrawState State, bool& DefaultDraw)>;
// vsReport のとき、2 列目以降(SubItem は 1 から)を描く前。
using TLVCustomDrawSubItemEvent =
    std::function<void(TCustomListView* Sender, TListItem* Item, int SubItem, TCustomDrawState State, bool& DefaultDraw)>;
// OwnerDraw で ViewStyle が vsReport のとき、項目(行)を描く(ARect の範囲に Sender->Canvas で描く)。
using TLVDrawItemEvent = std::function<void(TCustomListView* Sender, TListItem* Item, TRect ARect, TOwnerDrawState State)>;

// 以下のメンバは LCL の TCustomListView の public。
class TCustomListView : public TWinControl
{
public:
    using TWinControl::BorderWidth;
    // スクロールバーの出し方(docs/adr/0048)。
    Property<TScrollStyle> ScrollBars;

    using TWinControl::BorderStyle;

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

    // ---- docs/adr/0052 ----
    // OnCustomDrawItem・OnDrawItem で使う描画先。
    ReadOnlyProperty<TCanvas*> Canvas;
    // 仮想モード。true にすると項目を持たず、Items->Count の数の項目を、表示のたびに OnData で求める。
    // Items->Item[i] は、その位置の内容を入れた 1 つの共有の項目を返す(次に別の位置を求めると内容が変わる)。
    // 切り替えると、それまでの項目はすべて削除される。
    Property<bool> OwnerData;
    // true なら、マウスの下の項目を強調する。
    Property<bool> HotTrack;
    // ラベルを編集しているか。
    bool IsEditing() const;
    // 1 列目(Caption)の昇順に並べる。SortType を stText、SortColumn を 0、SortDirection を sdAscending にする(LCL の仕様)。
    bool AlphaSort();
    // SortType・SortColumn・SortDirection(OnCompare があればそれ)で並べ直す(SortType が stNone なら何もしない)。
    void Sort();

protected:
    explicit TCustomListView(ObjectHandle handle);
    ~TCustomListView() override = default;

private:
    // Canvas の実体への非所有のラッパー(最初に読んだときに作る。LCL の Canvas はコントロールの破棄まで同じもの)。
    std::unique_ptr<TCanvas> canvas_;
    static TCanvas* GetCanvasImpl(TObject* owner);
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

    // ---- docs/adr/0048 ----
    static TScrollStyle GetScrollBarsImpl(TObject* owner);
    static void         SetScrollBarsImpl(TObject* owner, const TScrollStyle& value);

    // ---- docs/adr/0052 ----
    static bool GetOwnerDataImpl(TObject* owner);
    static void SetOwnerDataImpl(TObject* owner, const bool& value);
    static bool GetHotTrackImpl(TObject* owner);
    static void SetHotTrackImpl(TObject* owner, const bool& value);
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
    // リストビュー自身の破棄に伴う削除では呼ばれない(LCL はリストビューの破棄通知の後に項目を削除し、
    // そのときにはリストビューのラッパーが delete されているため)。
    Property<TLVDeletedEvent>     OnDeletion;
    // チェックボックス(Checkboxes)が切り替わったとき。
    Property<TLVCheckedItemEvent> OnItemChecked;
    // 列見出しがクリックされたとき。
    Property<TLVColumnClickEvent> OnColumnClick;
    // 画像リスト(docs/adr/0030)。LCL では TCustomListView の protected で、TListView が公開する。LargeImages は vsIcon、SmallImages はそれ以外の表示形式で使う。
    Property<TCustomImageList*> LargeImages;
    Property<TCustomImageList*> SmallImages;
    Property<TCustomImageList*> StateImages;

    // ---- docs/adr/0052 ----
    // vsReport のときに列見出しを表示するか。
    Property<bool> ShowColumnHeaders;
    // 列見出しをクリックできるか(false なら OnColumnClick も呼ばれない)。
    Property<bool> ColumnClick;
    // true なら、はみ出した項目の文字をツールチップで表示する。
    Property<bool> ToolTips;
    // true で ViewStyle が vsReport なら、項目を OnDrawItem で描く。
    Property<bool> OwnerDraw;
    // true(既定)なら、SortType が stNone でないとき、列見出しのクリックで SortColumn をその列にする(同じ列なら SortDirection を逆にする)。
    Property<bool> AutoSort;
    Property<TLVCompareEvent>           OnCompare;
    Property<TLVDataEvent>              OnData;
    Property<TLVEditingEvent>           OnEditing;
    Property<TLVEditedEvent>            OnEdited;
    Property<TLVCustomDrawItemEvent>    OnCustomDrawItem;
    Property<TLVCustomDrawSubItemEvent> OnCustomDrawSubItem;
    Property<TLVDrawItemEvent>          OnDrawItem;

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

    static void BETH_CALL SelectItemTrampoline(ObjectHandle sender, ObjectHandle item, internal::int_t selected, void* data);
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, ObjectHandle item, internal::int_t change, void* data);
    static void BETH_CALL DeletionTrampoline(ObjectHandle sender, ObjectHandle item, void* data);
    static void BETH_CALL ItemCheckedTrampoline(ObjectHandle sender, ObjectHandle item, void* data);
    static void BETH_CALL ColumnClickTrampoline(ObjectHandle sender, ObjectHandle column, void* data);

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

    static TCustomImageList* GetLargeImagesImpl(TObject* owner);
    static void SetLargeImagesImpl(TObject* owner, TCustomImageList* const& value);
    static TCustomImageList* GetSmallImagesImpl(TObject* owner);
    static void SetSmallImagesImpl(TObject* owner, TCustomImageList* const& value);
    static TCustomImageList* GetStateImagesImpl(TObject* owner);
    static void SetStateImagesImpl(TObject* owner, TCustomImageList* const& value);

    // ---- docs/adr/0052 ----
    static bool GetShowColumnHeadersImpl(TObject* owner);
    static void SetShowColumnHeadersImpl(TObject* owner, const bool& value);
    static bool GetColumnClickImpl(TObject* owner);
    static void SetColumnClickImpl(TObject* owner, const bool& value);
    static bool GetToolTipsImpl(TObject* owner);
    static void SetToolTipsImpl(TObject* owner, const bool& value);
    static bool GetOwnerDrawImpl(TObject* owner);
    static void SetOwnerDrawImpl(TObject* owner, const bool& value);
    static bool GetAutoSortImpl(TObject* owner);
    static void SetAutoSortImpl(TObject* owner, const bool& value);
    TLVCompareEvent onCompare_;
    bool onCompareHooked_ = false;
    static TLVCompareEvent GetOnCompareImpl(TObject* owner);
    static void SetOnCompareImpl(TObject* owner, const TLVCompareEvent& value);
    static void BETH_CALL CompareTrampoline(ObjectHandle sender, ObjectHandle item1, ObjectHandle item2, internal::int_t data, internal::int_t* compare, void* cbData);
    TLVDataEvent onData_;
    bool onDataHooked_ = false;
    static TLVDataEvent GetOnDataImpl(TObject* owner);
    static void SetOnDataImpl(TObject* owner, const TLVDataEvent& value);
    static void BETH_CALL DataTrampoline(ObjectHandle sender, ObjectHandle item, void* data);
    TLVEditingEvent onEditing_;
    bool onEditingHooked_ = false;
    static TLVEditingEvent GetOnEditingImpl(TObject* owner);
    static void SetOnEditingImpl(TObject* owner, const TLVEditingEvent& value);
    static void BETH_CALL EditingTrampoline(ObjectHandle sender, ObjectHandle item, internal::bool_t* allow, void* data);
    TLVEditedEvent onEdited_;
    bool onEditedHooked_ = false;
    static TLVEditedEvent GetOnEditedImpl(TObject* owner);
    static void SetOnEditedImpl(TObject* owner, const TLVEditedEvent& value);
    static void BETH_CALL EditedTrampoline(ObjectHandle sender, ObjectHandle item, internal::str_t s, internal::str_t* result, void* data);
    TLVCustomDrawItemEvent onCustomDrawItem_;
    bool onCustomDrawItemHooked_ = false;
    static TLVCustomDrawItemEvent GetOnCustomDrawItemImpl(TObject* owner);
    static void SetOnCustomDrawItemImpl(TObject* owner, const TLVCustomDrawItemEvent& value);
    static void BETH_CALL CustomDrawItemTrampoline(ObjectHandle sender, ObjectHandle item, internal::uint_t state, internal::bool_t* defaultDraw, void* data);
    TLVCustomDrawSubItemEvent onCustomDrawSubItem_;
    bool onCustomDrawSubItemHooked_ = false;
    static TLVCustomDrawSubItemEvent GetOnCustomDrawSubItemImpl(TObject* owner);
    static void SetOnCustomDrawSubItemImpl(TObject* owner, const TLVCustomDrawSubItemEvent& value);
    static void BETH_CALL CustomDrawSubItemTrampoline(ObjectHandle sender, ObjectHandle item, internal::int_t subItem, internal::uint_t state, internal::bool_t* defaultDraw, void* data);
    TLVDrawItemEvent onDrawItem_;
    bool onDrawItemHooked_ = false;
    static TLVDrawItemEvent GetOnDrawItemImpl(TObject* owner);
    static void SetOnDrawItemImpl(TObject* owner, const TLVDrawItemEvent& value);
    static void BETH_CALL DrawItemTrampoline(ObjectHandle sender, ObjectHandle item, internal::int_t left, internal::int_t top, internal::int_t right, internal::int_t bottom, internal::uint_t state, void* data);
};

// Splitter のドラッグ中の表示のしかた(寄せる辺の ResizeAnchor は TAnchorKind)。
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
    explicit TCustomSplitter(ObjectHandle handle);
    ~TCustomSplitter() override = default;

private:
    TNotifyEvent onMoved_;
    bool         onMovedHooked_ = false;
    static void BETH_CALL MovedTrampoline(ObjectHandle sender, void* data);

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
    // 右端で折り返す(折り返すと横のスクロールバーは出ない)。
    Property<bool> WordWrap;
    // Enter で改行を入れる(false なら、フォームの既定のボタンが押される)。
    Property<bool> WantReturns;
    // Tab でタブ文字を入れる(false なら、次のコントロールにフォーカスが移る)。
    Property<bool> WantTabs;
    // 末尾に 1 行加える(Lines->Add と違い、表示を最後の行までスクロールする)。
    void Append(const std::string& S);

    // スクロールバーの出し方(docs/adr/0048 で int から TScrollStyle にした)。
    Property<TScrollStyle> ScrollBars;

    // 文字列の一覧(TStrings。Memo1->Lines->Add("x") のように使う)。
    ReadOnlyProperty<TStrings*> Lines;

protected:
    explicit TCustomMemo(ObjectHandle handle);
    ~TCustomMemo() override = default;

private:
    TStrings lines_;
    static TStrings* GetLinesImpl(TObject* owner);
    static TScrollStyle GetScrollBarsImpl(TObject* owner);
    static void         SetScrollBarsImpl(TObject* owner, const TScrollStyle& value);

private:
    static bool GetWordWrapImpl(TObject* owner);
    static void SetWordWrapImpl(TObject* owner, const bool& value);
    static bool GetWantReturnsImpl(TObject* owner);
    static void SetWantReturnsImpl(TObject* owner, const bool& value);
    static bool GetWantTabsImpl(TObject* owner);
    static void SetWantTabsImpl(TObject* owner, const bool& value);
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
    using TWinControl::BorderStyle;

    // 見た目と入力(csDropDownList は一覧から選ぶだけで、文字を入力できない)。
    Property<TComboBoxStyle> Style;
    // 一覧を開いたときに表示する項目の数。
    Property<int> DropDownCount;
    // 項目を並べ替えて表示する。
    Property<bool> Sorted;
    // 文字を入力できない(一覧から選ぶことはできる)。
    Property<bool> ReadOnly;
    // 一覧が開いているか。設定すると開く・閉じる。
    Property<bool> DroppedDown;
    // 入力した文字で始まる項目を補う。
    Property<bool> AutoComplete;
    // 一覧から項目を選んだとき(文字の入力では呼ばれない)。
    Property<TNotifyEvent> OnSelect;
    // 一覧を開く直前。
    Property<TNotifyEvent> OnDropDown;
    // 一覧を閉じたとき。
    Property<TNotifyEvent> OnCloseUp;

    using TControl::Text;
    Property<int> ItemIndex;

    // 文字列の一覧(TStrings。ComboBox1->Items->Add("x") のように使う)。
    ReadOnlyProperty<TStrings*> Items;

    // ---- docs/adr/0050 ----
    // Style が csOwnerDrawFixed・csOwnerDrawVariable 等なら、一覧の項目を OnDrawItem で描く。
    // 項目の高さ(lbOwnerDrawFixed 等で使う)。
    Property<int> ItemHeight;
    // OnDrawItem の中で描く先。コントロールが所有する実体への非所有のビュー。
    ReadOnlyProperty<TCanvas*> Canvas;
    Property<TDrawItemEvent> OnDrawItem;
    Property<TMeasureItemEvent> OnMeasureItem;

protected:
    explicit TCustomComboBox(ObjectHandle handle);
    ~TCustomComboBox() override = default;

private:
    // Canvas の実体への非所有のラッパー(最初に読んだときに作る。LCL の Canvas はコントロールの破棄まで同じもの)。
    std::unique_ptr<TCanvas> canvas_;
    static TCanvas* GetCanvasImpl(TObject* owner);
    TStrings items_;
    static TStrings* GetItemsImpl(TObject* owner);
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);

private:
    static TComboBoxStyle GetStyleImpl(TObject* owner);
    static void SetStyleImpl(TObject* owner, const TComboBoxStyle& value);
    static int GetDropDownCountImpl(TObject* owner);
    static void SetDropDownCountImpl(TObject* owner, const int& value);
    static bool GetSortedImpl(TObject* owner);
    static void SetSortedImpl(TObject* owner, const bool& value);
    static bool GetReadOnlyImpl(TObject* owner);
    static void SetReadOnlyImpl(TObject* owner, const bool& value);
    static bool GetDroppedDownImpl(TObject* owner);
    static void SetDroppedDownImpl(TObject* owner, const bool& value);
    static bool GetAutoCompleteImpl(TObject* owner);
    static void SetAutoCompleteImpl(TObject* owner, const bool& value);
    TNotifyEvent onSelect_;
    bool onSelectHooked_ = false;
    static void BETH_CALL SelectTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnSelectImpl(TObject* owner);
    static void SetOnSelectImpl(TObject* owner, const TNotifyEvent& value);
    TNotifyEvent onDropDown_;
    bool onDropDownHooked_ = false;
    static void BETH_CALL DropDownTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnDropDownImpl(TObject* owner);
    static void SetOnDropDownImpl(TObject* owner, const TNotifyEvent& value);
    TNotifyEvent onCloseUp_;
    bool onCloseUpHooked_ = false;
    static void BETH_CALL CloseUpTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnCloseUpImpl(TObject* owner);
    static void SetOnCloseUpImpl(TObject* owner, const TNotifyEvent& value);

    // ---- docs/adr/0050 ----
    static int GetItemHeightImpl(TObject* owner);
    static void SetItemHeightImpl(TObject* owner, const int& value);
    TDrawItemEvent onDrawItem_;
    bool onDrawItemHooked_ = false;
    static TDrawItemEvent GetOnDrawItemImpl(TObject* owner);
    static void SetOnDrawItemImpl(TObject* owner, const TDrawItemEvent& value);
    static void BETH_CALL DrawItemTrampoline(ObjectHandle sender, internal::int_t index, internal::int_t left, internal::int_t top, internal::int_t right, internal::int_t bottom, internal::uint_t state, void* data);
    TMeasureItemEvent onMeasureItem_;
    bool onMeasureItemHooked_ = false;
    static TMeasureItemEvent GetOnMeasureItemImpl(TObject* owner);
    static void SetOnMeasureItemImpl(TObject* owner, const TMeasureItemEvent& value);
    static void BETH_CALL MeasureItemTrampoline(ObjectHandle sender, internal::int_t index, internal::int_t* height, void* data);
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
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);
};

// 利用者による選択の変更(マウス・キー操作とも)では OnClick が呼ばれる(VCL と同じ)。
// プログラムからの ItemIndex の変更では呼ばれない。
class TCustomListBox : public TWinControl
{
public:
    using TWinControl::BorderStyle;

    // 複数の項目を選べるようにする(選んだ項目は Selected[i])。
    Property<bool> MultiSelect;
    // MultiSelect のとき、Shift・Ctrl で範囲・追加の選択をする(false なら、クリックのたびに選択を切り替える)。
    Property<bool> ExtendedSelect;
    // 項目を並べ替えて表示する(Items への追加も並べ替えた位置に入る)。
    Property<bool> Sorted;
    // 一番上に表示している項目。
    Property<int> TopIndex;
    // 選んでいる項目の数(MultiSelect のとき)。
    ReadOnlyProperty<int> SelCount;
    // 項目が選ばれているか(ListBox1->Selected[i])。
    IndexedProperty<bool> Selected;
    // 選択をすべて外す。
    void ClearSelection();
    // すべての項目を選ぶ(MultiSelect のとき)。
    void SelectAll();
    // クライアント領域の座標にある項目。無ければ -1(LCL では Existing によらない。VCL との互換のために受け取る)。
    int ItemAtPos(const TPoint& Pos, bool Existing) const;
    // 選択が変わったとき。User は利用者の操作によるものか(プログラムからの変更なら false)。
    Property<TSelectionChangeEvent> OnSelectionChange;

    Property<int> ItemIndex;

    // 文字列の一覧(TStrings。ListBox1->Items->Add("x") のように使う)。
    ReadOnlyProperty<TStrings*> Items;

    // ---- docs/adr/0050 ----
    // 描き方(lbOwnerDrawFixed・lbOwnerDrawVariable なら OnDrawItem で描く)。
    Property<TListBoxStyle> Style;
    // 項目の高さ(lbOwnerDrawFixed 等で使う)。
    Property<int> ItemHeight;
    // OnDrawItem の中で描く先。コントロールが所有する実体への非所有のビュー。
    ReadOnlyProperty<TCanvas*> Canvas;
    Property<TDrawItemEvent> OnDrawItem;
    Property<TMeasureItemEvent> OnMeasureItem;

protected:
    explicit TCustomListBox(ObjectHandle handle);
    ~TCustomListBox() override = default;

private:
    // Canvas の実体への非所有のラッパー(最初に読んだときに作る。LCL の Canvas はコントロールの破棄まで同じもの)。
    std::unique_ptr<TCanvas> canvas_;
    static TCanvas* GetCanvasImpl(TObject* owner);
    TStrings items_;
    static TStrings* GetItemsImpl(TObject* owner);
    static int  GetItemIndexImpl(TObject* owner);
    static void SetItemIndexImpl(TObject* owner, const int& value);

private:
    static bool GetMultiSelectImpl(TObject* owner);
    static void SetMultiSelectImpl(TObject* owner, const bool& value);
    static bool GetExtendedSelectImpl(TObject* owner);
    static void SetExtendedSelectImpl(TObject* owner, const bool& value);
    static bool GetSortedImpl(TObject* owner);
    static void SetSortedImpl(TObject* owner, const bool& value);
    static int GetTopIndexImpl(TObject* owner);
    static void SetTopIndexImpl(TObject* owner, const int& value);
    static int GetSelCountImpl(TObject* owner);
    static bool GetSelectedImpl(TObject* owner, int index);
    static void SetSelectedImpl(TObject* owner, int index, const bool& value);
    TSelectionChangeEvent onSelectionChange_;
    bool onSelectionChangeHooked_ = false;
    static void BETH_CALL SelectionChangeTrampoline(ObjectHandle sender, internal::bool_t user, void* data);
    static TSelectionChangeEvent GetOnSelectionChangeImpl(TObject* owner);
    static void SetOnSelectionChangeImpl(TObject* owner, const TSelectionChangeEvent& value);

    // ---- docs/adr/0050 ----
    static TListBoxStyle GetStyleImpl(TObject* owner);
    static void SetStyleImpl(TObject* owner, const TListBoxStyle& value);
    static int GetItemHeightImpl(TObject* owner);
    static void SetItemHeightImpl(TObject* owner, const int& value);
    TDrawItemEvent onDrawItem_;
    bool onDrawItemHooked_ = false;
    static TDrawItemEvent GetOnDrawItemImpl(TObject* owner);
    static void SetOnDrawItemImpl(TObject* owner, const TDrawItemEvent& value);
    static void BETH_CALL DrawItemTrampoline(ObjectHandle sender, internal::int_t index, internal::int_t left, internal::int_t top, internal::int_t right, internal::int_t bottom, internal::uint_t state, void* data);
    TMeasureItemEvent onMeasureItem_;
    bool onMeasureItemHooked_ = false;
    static TMeasureItemEvent GetOnMeasureItemImpl(TObject* owner);
    static void SetOnMeasureItemImpl(TObject* owner, const TMeasureItemEvent& value);
    static void BETH_CALL MeasureItemTrampoline(ObjectHandle sender, internal::int_t index, internal::int_t* height, void* data);
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

    // 項目ごとのチェックの状態(CheckListBox1->Checked[i] = true;)。
    IndexedProperty<bool> Checked;

protected:
    explicit TCustomCheckListBox(ObjectHandle handle);
    ~TCustomCheckListBox() override = default;

private:
    static bool GetCheckedImpl(TObject* owner, int index);
    static void SetCheckedImpl(TObject* owner, int index, const bool& value);
    TNotifyEvent onClickCheck_;
    bool         onClickCheckHooked_ = false;
    static void BETH_CALL ClickCheckTrampoline(ObjectHandle sender, void* data);

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
    explicit TCustomStaticText(ObjectHandle handle);
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

class TStatusBar;

// パネルの描き方(LCL の TStatusPanelStyle と同じ値)。psOwnerDraw なら TStatusBar::OnDrawPanel で描く。
enum TStatusPanelStyle
{
    psText,
    psOwnerDraw
};

// パネルの縁(LCL の TStatusPanelBevel と同じ値)。
enum TStatusPanelBevel
{
    pbNone,
    pbLowered,
    pbRaised
};

// ステータスバーのパネル(LCL の TStatusPanel。TCollectionItem。docs/adr/0044)。同じパネルには常に同じポインタが返る。
// ラッパーは、パネルが破棄されたとき(TStatusPanels::Delete・Clear、ステータスバーの破棄)に delete される。
class TStatusPanel : public TPersistent
{
public:
    Property<std::string>       Text;
    // 幅(最後のパネルは残りの幅いっぱいに広がる)。
    Property<int>               Width;
    Property<TAlignment>        Alignment;
    Property<TStatusPanelBevel> Bevel;
    Property<TStatusPanelStyle> Style;
    // 並び順。書き換えるとパネルが移動する。
    Property<int>               Index;

private:
    friend class ItemRegistry;
    friend class TStatusPanels;
    friend class TStatusBar;

    explicit TStatusPanel(ObjectHandle handle);
    ~TStatusPanel() override = default;
    static TStatusPanel* Wrap(ObjectHandle handle) { return ItemRegistry::Wrap<TStatusPanel>(handle); }

    static std::string       GetTextImpl(TObject* owner);
    static void              SetTextImpl(TObject* owner, const std::string& value);
    static int               GetWidthImpl(TObject* owner);
    static void              SetWidthImpl(TObject* owner, const int& value);
    static TAlignment        GetAlignmentImpl(TObject* owner);
    static void              SetAlignmentImpl(TObject* owner, const TAlignment& value);
    static TStatusPanelBevel GetBevelImpl(TObject* owner);
    static void              SetBevelImpl(TObject* owner, const TStatusPanelBevel& value);
    static TStatusPanelStyle GetStyleImpl(TObject* owner);
    static void              SetStyleImpl(TObject* owner, const TStatusPanelStyle& value);
    static int               GetIndexImpl(TObject* owner);
    static void              SetIndexImpl(TObject* owner, const int& value);
};

// パネルの一覧(LCL の TStatusPanels。TCollection)。ステータスバーの値メンバとして持つ非所有のビュー。
class TStatusPanels : public TPersistent
{
public:
    explicit TStatusPanels(ObjectHandle handle);
    ~TStatusPanels() override = default;

    ReadOnlyProperty<int> Count;
    // StatusBar1->Panels->Items[i]。
    ReadOnlyIndexedProperty<TStatusPanel*> Items;

    // 末尾に(Insert は Index の位置に)空のパネルを追加して返す(Text・Width はその後で設定する。VCL と同じ)。
    TStatusPanel* Add();
    TStatusPanel* Insert(int Index);
    // パネルを削除する(パネルのラッパーも delete される)。
    void          Delete(int Index);
    void          Clear();
    void          BeginUpdate();
    void          EndUpdate();

private:
    static TStatusPanel* GetItemsImpl(TObject* owner, int Index);
    static int           GetCountImpl(TObject* owner);
};

// Style が psOwnerDraw のパネルを描くとき(Rect はパネルのクライアント座標での矩形。描画は TStatusBar::Canvas に行う)。
using TDrawPanelEvent = std::function<void(TStatusBar* StatusBar, TStatusPanel* Panel, const TRect& Rect)>;

// ステータス行。LCL では中間の TCustomStatusBar が無く、TWinControl の直接の派生。
// パネルを表示するには SimplePanel を false にする(LCL の既定は true で、SimpleText だけを表示する。VCL の既定は false)。
// 他のコントロールと同じく、フォームのコンストラクタの中で生成・配置してよい
// (LCL の Win32 実装が DLL で失敗する問題は DLL 側で回避済み。docs/adr/0015-... を参照)。
class TStatusBar : public TWinControl
{
public:
    explicit TStatusBar(TComponent* AOwner);

    Property<std::string> SimpleText;
    Property<bool>         SimplePanel;
    ReadOnlyProperty<TStatusPanels*> Panels;
    // 右下のサイズ変更のつまみを出すか(フォームの右下にあり、フォームの大きさを変えられるときだけ出る)。
    Property<bool>         SizeGrip;
    // true なら、Application のヒント(コントロールの Hint)をステータスバーに表示する
    // (SimplePanel なら SimpleText、そうでなければ最初のパネルに。OnHint を設定すると、代わりに OnHint を呼ぶ)。
    // LCL は ShowHint が true のコントロール(か親)にだけ Application のヒントを設定する(VCL は ShowHint によらない)。
    Property<bool>         AutoHint;
    // OnDrawPanel の中で描画する先。ステータスバーが所有する実体への非所有のビュー(TPaintBox::Canvas と同じ)。
    ReadOnlyProperty<TCanvas*> Canvas;

    Property<TDrawPanelEvent> OnDrawPanel;
    // AutoHint のとき、ヒントを表示する代わりに呼ばれる(ヒントは Application->Hint)。
    Property<TNotifyEvent>    OnHint;

    // クライアント座標 (X, Y) にあるパネルの位置。無ければ -1。
    int  GetPanelIndexAt(int X, int Y) const;
    // パネルをまとめて変えるとき、EndUpdate まで再描画を止める。
    void BeginUpdate();
    void EndUpdate();

protected:
    ~TStatusBar() override = default;

private:
    // Canvas の実体への非所有のラッパー(最初に読んだときに作る。LCL の Canvas はコントロールの破棄まで同じもの)。
    std::unique_ptr<TCanvas> canvas_;
    static TCanvas* GetCanvasImpl(TObject* owner);
    TStatusPanels   panels_;
    TDrawPanelEvent onDrawPanel_;
    TNotifyEvent    onHint_;
    bool onDrawPanelHooked_ = false;
    bool onHintHooked_      = false;

    static void BETH_CALL DrawPanelTrampoline(ObjectHandle sender, ObjectHandle panel, internal::int_t left, internal::int_t top,
                                              internal::int_t right, internal::int_t bottom, void* data);
    static void BETH_CALL HintTrampoline(ObjectHandle sender, void* data);

    static std::string GetSimpleTextImpl(TObject* owner);
    static void         SetSimpleTextImpl(TObject* owner, const std::string& value);
    static bool         GetSimplePanelImpl(TObject* owner);
    static void          SetSimplePanelImpl(TObject* owner, const bool& value);
    static TStatusPanels* GetPanelsImpl(TObject* owner);
    static bool         GetSizeGripImpl(TObject* owner);
    static void         SetSizeGripImpl(TObject* owner, const bool& value);
    static bool         GetAutoHintImpl(TObject* owner);
    static void         SetAutoHintImpl(TObject* owner, const bool& value);
    static TDrawPanelEvent GetOnDrawPanelImpl(TObject* owner);
    static void            SetOnDrawPanelImpl(TObject* owner, const TDrawPanelEvent& value);
    static TNotifyEvent    GetOnHintImpl(TObject* owner);
    static void            SetOnHintImpl(TObject* owner, const TNotifyEvent& value);
};

/* ---------------- Shape / SpeedButton / PaintBox / Image ---------------- */

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
    // 縁の線(C++Builder と同じくポインタ。Shape1->Pen->Color = clRed;。docs/adr/0057)。代入は内容のコピー。
    Property<TPen*>   Pen;
    // 中の塗りつぶし(Shape1->Brush->Color = clYellow;。bsClear なら塗りつぶさない)。代入は内容のコピー。
    Property<TBrush*> Brush;
    Property<TShapeType> Shape;

protected:
    explicit TCustomShape(ObjectHandle handle);
    ~TCustomShape() override = default;

private:
    TPen   pen_;
    TBrush brush_;
    static TPen*   GetPenImpl(TObject* owner);
    static void    SetPenImpl(TObject* owner, TPen* const& value);
    static TBrush* GetBrushImpl(TObject* owner);
    static void    SetBrushImpl(TObject* owner, TBrush* const& value);
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
// Glyph 等の意味は TCustomBitBtn と同じ。
class TCustomSpeedButton : public TGraphicControl
{
public:
    Property<bool>          Down;
    Property<int>           GroupIndex;
    Property<bool>          Flat;
    Property<bool>          AllowAllUp;
    Property<TBitmap*>      Glyph;
    Property<int>           NumGlyphs;
    Property<TButtonLayout> Layout;
    Property<int>           Margin;
    Property<int>           Spacing;
    // 画像リスト(docs/adr/0030)。意味は TCustomBitBtn と同じ。
    Property<TCustomImageList*> Images;
    Property<int> ImageIndex;

protected:
    explicit TCustomSpeedButton(ObjectHandle handle);
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

    TBitmap glyph_;

    static TBitmap*      GetGlyphImpl(TObject* owner);
    static void          SetGlyphImpl(TObject* owner, TBitmap* const& value);
    static int           GetNumGlyphsImpl(TObject* owner);
    static void          SetNumGlyphsImpl(TObject* owner, const int& value);
    static TButtonLayout GetLayoutImpl(TObject* owner);
    static void          SetLayoutImpl(TObject* owner, const TButtonLayout& value);
    static int           GetMarginImpl(TObject* owner);
    static void          SetMarginImpl(TObject* owner, const int& value);
    static int           GetSpacingImpl(TObject* owner);
    static void          SetSpacingImpl(TObject* owner, const int& value);

    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);
    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
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
    ReadOnlyProperty<TCanvas*> Canvas;
    Property<TNotifyEvent> OnPaint;

    explicit TPaintBox(TComponent* AOwner);

protected:
    ~TPaintBox() override = default;

private:
    // Canvas の実体への非所有のラッパー(最初に読んだときに作る。LCL の Canvas はコントロールの破棄まで同じもの)。
    std::unique_ptr<TCanvas> canvas_;
    static TCanvas* GetCanvasImpl(TObject* owner);
    TNotifyEvent onPaint_;
    bool         onPaintHooked_ = false;
    static void BETH_CALL PaintTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnPaintImpl(TObject* owner);
    static void         SetOnPaintImpl(TObject* owner, const TNotifyEvent& value);
};

// 画像を表示するコントロール(LCL の TCustomImage。docs/adr/0029)。AutoSize は TControl のもの。
class TCustomImage : public TGraphicControl
{
public:
    // 表示する画像。コントロールが所有し、コントロールと寿命が一致する。代入は内容のコピー。
    Property<TPicture*>        Picture;
    // 画像に描く先。Picture が空なら、コントロールの大きさの TBitmap を作ってからその Canvas を返す(描いた内容は Picture に残る)。
    // Picture の中身が作り直されると別のものになる(ポインタを保存しないこと)。
    ReadOnlyProperty<TCanvas*> Canvas;
    ReadOnlyProperty<bool>     HasGraphic;
    Property<bool>             Center;
    // コントロールの大きさに伸縮する。StretchOutEnabled・StretchInEnabled を false にすると、拡大・縮小を個別に禁止できる。
    Property<bool>             Stretch;
    Property<bool>             StretchOutEnabled;
    Property<bool>             StretchInEnabled;
    // 縦横比を保ってコントロールに収める。
    Property<bool>             Proportional;
    Property<bool>             Transparent;
    // Picture(またはその中身)が変わったときに呼ばれる。
    Property<TNotifyEvent>     OnPictureChanged;
    // 画像リスト(docs/adr/0030)。設定すると、Picture が空のとき Images の ImageIndex 番目の画像を表示する。
    Property<TCustomImageList*> Images;
    Property<int> ImageIndex;

protected:
    explicit TCustomImage(ObjectHandle handle);
    ~TCustomImage() override = default;

private:
    TPicture     picture_;
    CanvasHolder canvas_;
    TNotifyEvent onPictureChanged_;
    bool         onPictureChangedHooked_ = false;

    static TPicture*    GetPictureImpl(TObject* owner);
    static void         SetPictureImpl(TObject* owner, TPicture* const& value);
    static TCanvas*     GetCanvasImpl(TObject* owner);
    static bool         GetHasGraphicImpl(TObject* owner);
    static bool         GetCenterImpl(TObject* owner);
    static void         SetCenterImpl(TObject* owner, const bool& value);
    static bool         GetStretchImpl(TObject* owner);
    static void         SetStretchImpl(TObject* owner, const bool& value);
    static bool         GetStretchOutEnabledImpl(TObject* owner);
    static void         SetStretchOutEnabledImpl(TObject* owner, const bool& value);
    static bool         GetStretchInEnabledImpl(TObject* owner);
    static void         SetStretchInEnabledImpl(TObject* owner, const bool& value);
    static bool         GetProportionalImpl(TObject* owner);
    static void         SetProportionalImpl(TObject* owner, const bool& value);
    static bool         GetTransparentImpl(TObject* owner);
    static void         SetTransparentImpl(TObject* owner, const bool& value);
    static void BETH_CALL PictureChangedTrampoline(ObjectHandle sender, void* data);
    static TNotifyEvent GetOnPictureChangedImpl(TObject* owner);
    static void         SetOnPictureChangedImpl(TObject* owner, const TNotifyEvent& value);

    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);
    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
};

class TImage : public TCustomImage
{
public:
    explicit TImage(TComponent* AOwner);

protected:
    ~TImage() override = default;
};

/* ---------------- Grid ---------------- */

// グリッドの選択範囲(Left/Right が列、Top/Bottom が行)。
using TGridRect = TRect;

// グリッドの Options(LCL の TGridOptions)。Grid1->Options = Grid1->Options << goEditing; のように書く(docs/adr/0061)。
enum TGridOption
{
    goFixedVertLine,
    goFixedHorzLine,
    goVertLine,
    goHorzLine,
    goRangeSelect,
    goDrawFocusSelected,
    goRowSizing,
    goColSizing,
    goRowMoving,
    goColMoving,
    goEditing,
    goAutoAddRows,
    goTabs,
    goRowSelect,
    goAlwaysShowEditor,
    goThumbTracking,
    goColSpanning,
    goRelaxedRowSelect,
    goDblClickAutoSize,
    goSmoothScroll,
    goFixedRowNumbering,
    goScrollKeepVisible,
    goHeaderHotTracking,
    goHeaderPushedLook,
    goSelectionActive,
    goFixedColSizing,
    goDontScrollPartCell,
    goCellHints,
    goTruncCellHints,
    goCellEllipsis,
    goAutoAddRowsSkipContentCheck,
    goRowHighlight
};
using TGridOptions = Set<TGridOption>;

// OnDrawCell の AState(LCL の TGridDrawState)。
enum TGridDrawStateItem
{
    gdSelected,
    gdFocused,
    gdFixed,
    gdHot,
    gdPushed,
    gdRowHighlight
};
using TGridDrawState = Set<TGridDrawStateItem>;

// セルを描画するとき(ARect はセルのクライアント座標での矩形。描画は TCustomDrawGrid::Canvas に行う)。
using TOnDrawCell        = std::function<void(TObject* Sender, int ACol, int ARow, TRect ARect, TGridDrawState AState)>;
// セルが選択される前(CanSelect を false にすると選択させない)。
using TOnSelectCellEvent = std::function<void(TObject* Sender, int ACol, int ARow, bool& CanSelect)>;
// セルが選択された後。
using TOnSelectEvent     = std::function<void(TObject* Sender, int ACol, int ARow)>;
// 見出し(固定行・固定列)がクリックされたとき(IsColumn は列見出しなら true)。
using THdrEvent          = std::function<void(TObject* Sender, bool IsColumn, int Index)>;

// ---- グリッドの細部(docs/adr/0053。LCL・VCL と同じ形) ----
// 並べ替えの向き(ColumnClickSorts・SortColRow)。
enum TSortOrder { soAscending, soDescending };
// セルの編集を始めるときに、編集欄に出す文字列を求める(Value を書き換えると、それが編集欄に出る)。
// TStringGrid では、Value はセルの文字列で始まる。
using TGetEditEvent = std::function<void(TObject* Sender, int ACol, int ARow, std::string& Value)>;
// 編集欄の文字列が変わるたび(TStringGrid では、このイベントの後にセルの文字列になる)。
using TSetEditEvent = std::function<void(TObject* Sender, int ACol, int ARow, const std::string& Value)>;
// セルの編集を終えるとき(別のセルへ移る・Enter。EditorMode = false では呼ばれない)。OldValue は編集を始める前のセルの文字列。
// NewValue を書き換えると、それがセルの値になる。受け付けないときは例外を投げる(LCL が例外を表示し、編集欄に留まる)。
using TValidateEntryEvent =
    std::function<void(TObject* Sender, int ACol, int ARow, const std::string& OldValue, std::string& NewValue)>;
// セルを描く前(Canvas の Brush・Font を変えると、既定の描画がその色で描く。OnDrawCell より前)。
using TOnPrepareCanvasEvent = std::function<void(TObject* Sender, int ACol, int ARow, TGridDrawState AState)>;
// 並べ替えで 2 つのセルを比べる(A が前なら負、後ろなら正、同じなら 0 を Result に入れる)。
// OnCompareCells があると SortOrder は使われない(逆順にするときは Result の符号を変える)。nullptr に戻すと既定の比較に戻る。
using TOnCompareCells = std::function<void(TObject* Sender, int ACol, int ARow, int BCol, int BRow, int& Result)>;
// 行・列の挿入・削除・移動・入れ替えの後(IsColumn は列なら true。sIndex・tIndex は元と先の位置。挿入・削除では範囲)。
using TGridOperationEvent = std::function<void(TObject* Sender, bool IsColumn, int sIndex, int tIndex)>;

// ---- グリッドの Columns(docs/adr/0054) ----

// 列のセルの編集欄の種類(cbsAuto は文字列の入力。cbsPickList は PickList から選ぶ一覧、cbsEllipsis は「…」のボタン付き、
// cbsCheckboxColumn はチェックボックス(クリック・スペースで切り替えるには、グリッドの Options に goEditing が要る)、
// cbsButton・cbsButtonColumn はボタン)。
enum TColumnButtonStyle { cbsAuto, cbsEllipsis, cbsNone, cbsPickList, cbsCheckboxColumn, cbsButton, cbsButtonColumn };

// 列の見出し(LCL の TGridColumnTitle)。列が所有し、列と同じ寿命のビュー(Column->Title->Caption = "Name";)。
class TGridColumnTitle : public TPersistent
{
public:
    explicit TGridColumnTitle(ObjectHandle handle);
    ~TGridColumnTitle() override = default;

    Property<std::string> Caption;
    Property<TAlignment>  Alignment;
    Property<TTextLayout> Layout;
    Property<TColor>      Color;
    // true なら、Caption の改行で複数の行に分けて描く。
    Property<bool>        MultiLine;
    // 見出しの文字のフォント(既定はグリッドの TitleFont)。代入は内容のコピー。
    Property<TFont*>      Font;

private:
    TFont font_;
    static std::string GetCaptionImpl(TObject* owner);
    static void SetCaptionImpl(TObject* owner, const std::string& value);
    static TAlignment GetAlignmentImpl(TObject* owner);
    static void SetAlignmentImpl(TObject* owner, const TAlignment& value);
    static TTextLayout GetLayoutImpl(TObject* owner);
    static void SetLayoutImpl(TObject* owner, const TTextLayout& value);
    static TColor GetColorImpl(TObject* owner);
    static void SetColorImpl(TObject* owner, const TColor& value);
    static bool GetMultiLineImpl(TObject* owner);
    static void SetMultiLineImpl(TObject* owner, const bool& value);
    static TFont* GetFontImpl(TObject* owner);
    static void SetFontImpl(TObject* owner, TFont* const& value);
};

// グリッドの列(LCL の TGridColumn。TCollectionItem)。同じ列には常に同じポインタが返り、列が破棄されたとき
// (TGridColumns::Delete・Clear、グリッドの破棄)にラッパーも delete される(TStatusPanel と同じ)。
// Columns に列があると、グリッドの列(固定列を除く)は Columns の列になる(ColCount は FixedCols + Columns->Count。
// Visible が false の列も、幅 0 の列として数える)。
class TGridColumn : public TPersistent
{
public:
    ReadOnlyProperty<TGridColumnTitle*> Title;
    Property<int>                Width;
    Property<TAlignment>         Alignment;
    // セルの編集欄の種類。
    Property<TColumnButtonStyle> ButtonStyle;
    // ButtonStyle が cbsPickList(か cbsAuto)のときに選べる文字列(Column->PickList->Add("Red");)。
    ReadOnlyProperty<TStrings*>  PickList;
    Property<bool>               ReadOnly;
    Property<bool>               Visible;
    Property<TColor>             Color;
    // セルの文字のフォント(既定はグリッドの Font)。代入は内容のコピー。
    Property<TFont*>             Font;
    Property<TTextLayout>        Layout;
    // AutoFillColumns のときの、幅の下限・上限と、広げる割合。
    Property<int>                MinSize;
    Property<int>                MaxSize;
    Property<int>                SizePriority;
    // PickList の一覧に一度に出す行の数。
    Property<int>                DropDownRows;
    // ButtonStyle が cbsCheckboxColumn のとき、TStringGrid のセルの文字列が ValueChecked ならチェックあり、ValueUnchecked ならなし。
    Property<std::string>        ValueChecked;
    Property<std::string>        ValueUnchecked;
    // 並び順。書き換えると列が移動する。
    Property<int>                Index;

private:
    friend class ItemRegistry;
    friend class TGridColumns;
    friend class TCustomDrawGrid;

    explicit TGridColumn(ObjectHandle handle);
    ~TGridColumn() override = default;
    static TGridColumn* Wrap(ObjectHandle handle) { return ItemRegistry::Wrap<TGridColumn>(handle); }

    TGridColumnTitle title_;
    TStrings         pickList_;
    TFont            font_;

    static TGridColumnTitle* GetTitleImpl(TObject* owner);
    static TStrings* GetPickListImpl(TObject* owner);
    static TFont* GetFontImpl(TObject* owner);
    static void SetFontImpl(TObject* owner, TFont* const& value);
    static int GetWidthImpl(TObject* owner);
    static void SetWidthImpl(TObject* owner, const int& value);
    static TAlignment GetAlignmentImpl(TObject* owner);
    static void SetAlignmentImpl(TObject* owner, const TAlignment& value);
    static TColumnButtonStyle GetButtonStyleImpl(TObject* owner);
    static void SetButtonStyleImpl(TObject* owner, const TColumnButtonStyle& value);
    static bool GetReadOnlyImpl(TObject* owner);
    static void SetReadOnlyImpl(TObject* owner, const bool& value);
    static bool GetVisibleImpl(TObject* owner);
    static void SetVisibleImpl(TObject* owner, const bool& value);
    static TColor GetColorImpl(TObject* owner);
    static void SetColorImpl(TObject* owner, const TColor& value);
    static TTextLayout GetLayoutImpl(TObject* owner);
    static void SetLayoutImpl(TObject* owner, const TTextLayout& value);
    static int GetMinSizeImpl(TObject* owner);
    static void SetMinSizeImpl(TObject* owner, const int& value);
    static int GetMaxSizeImpl(TObject* owner);
    static void SetMaxSizeImpl(TObject* owner, const int& value);
    static int GetSizePriorityImpl(TObject* owner);
    static void SetSizePriorityImpl(TObject* owner, const int& value);
    static int GetDropDownRowsImpl(TObject* owner);
    static void SetDropDownRowsImpl(TObject* owner, const int& value);
    static std::string GetValueCheckedImpl(TObject* owner);
    static void SetValueCheckedImpl(TObject* owner, const std::string& value);
    static std::string GetValueUncheckedImpl(TObject* owner);
    static void SetValueUncheckedImpl(TObject* owner, const std::string& value);
    static int GetIndexImpl(TObject* owner);
    static void SetIndexImpl(TObject* owner, const int& value);
};

// グリッドの列の一覧(LCL の TGridColumns。TCollection)。グリッドの値メンバとして持つ非所有のビュー。
class TGridColumns : public TPersistent
{
public:
    explicit TGridColumns(ObjectHandle handle);
    ~TGridColumns() override = default;

    ReadOnlyProperty<int> Count;
    // LCL では Count と同じ(Visible が false の列も数える)。
    ReadOnlyProperty<int> VisibleCount;
    // StringGrid1->Columns->Items[i]。
    ReadOnlyIndexedProperty<TGridColumn*> Items;

    // 末尾に列を追加して返す(Title・Width はその後で設定する)。
    TGridColumn* Add();
    // 列を削除する(列のラッパーも delete される)。
    void         Delete(int Index);
    void         Clear();
    void         BeginUpdate();
    void         EndUpdate();
    // Title->Caption が aTitle の列(無ければ nullptr)。
    TGridColumn* ColumnByTitle(const std::string& aTitle) const;

private:
    static TGridColumn* GetItemsImpl(TObject* owner, int Index);
    static int GetCountImpl(TObject* owner);
    static int GetVisibleCountImpl(TObject* owner);
};

// セルの編集を始めるときに、編集欄のコントロールを選ぶ。Editor は既定の編集欄(ラッパーが無いものは nullptr)で、
// 自分のコントロール(TComboBox 等)を入れると、それを編集欄にする。書き換えなければ既定の編集欄のまま。
using TSelectEditorEvent = std::function<void(TObject* Sender, int ACol, int ARow, TWinControl*& Editor)>;
// ButtonStyle が cbsCheckboxColumn の列のセルの状態を求める(Value に入れる)。
using TGetCheckboxStateEvent = std::function<void(TObject* Sender, int ACol, int ARow, TCheckBoxState& Value)>;
// cbsCheckboxColumn の列のセルがクリックで切り替わったとき(Value を覚える)。
using TSetCheckboxStateEvent = std::function<void(TObject* Sender, int ACol, int ARow, TCheckBoxState Value)>;
// cbsCheckboxColumn の列のセルが切り替わった後。
using TToggledCheckboxEvent = std::function<void(TObject* Sender, int ACol, int ARow, TCheckBoxState AState)>;

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
    explicit TCustomGrid(ObjectHandle handle) : TCustomControl(handle) {}
    ~TCustomGrid() override = default;
};

// 以下のメンバは LCL では TCustomGrid の protected で、TCustomDrawGrid が public にしている。
class TCustomDrawGrid : public TCustomGrid
{
public:
    // スクロールバーの出し方(docs/adr/0048)。
    Property<TScrollStyle> ScrollBars;

    // OnDrawCell の中で描画する先は、TCustomControl::Canvas。

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

    // ---- docs/adr/0053 ----
    // AutoEdit・AlternateColor・TitleFont・ColumnClickSorts と OnValidateEntry・OnPrepareCanvas・OnCompareCells・OnTopLeftChanged は、
    // LCL では TCustomGrid の protected で、TDrawGrid・TStringGrid が published にしている。

    // true(既定)なら、goEditing のとき、文字を打つとすぐにセルの編集を始める(false なら F2・Enter・ダブルクリックで始める)。
    Property<bool>   AutoEdit;
    // 1 行おきの背景の色(既定は Color と同じ)。
    Property<TColor> AlternateColor;
    // フォーカスのあるセルの枠の色。
    Property<TColor> FocusColor;
    Property<TColor> GridLineColor;
    Property<int>    GridLineWidth;
    // 固定行(見出し)の文字のフォント。グリッドが所有する TFont のビュー。代入は内容のコピー。
    Property<TFont*> TitleFont;
    // true なら、列の幅を広げて(縮めて)グリッドの幅に合わせる。
    Property<bool>   AutoFillColumns;
    // true なら、列見出しのクリックでその列の値で行を並べ替える(同じ列をもう一度クリックすると逆順)。
    Property<bool>   ColumnClickSorts;
    // 並べ替えの向き(SortColRow もこの向きで並べる)。
    Property<TSortOrder>  SortOrder;
    // 最後に ColumnClickSorts で並べ替えた列(無ければ -1)。
    ReadOnlyProperty<int> SortColumn;
    // 行(IsColumn なら列)Index と WithIndex を入れ替える。
    void ExchangeColRow(bool IsColumn, int Index, int WithIndex);

    Property<TGetEditEvent>         OnGetEditText;
    Property<TSetEditEvent>         OnSetEditText;
    Property<TValidateEntryEvent>   OnValidateEntry;
    Property<TOnPrepareCanvasEvent> OnPrepareCanvas;
    Property<TOnCompareCells>       OnCompareCells;
    // スクロールで、表示されている最初の列・行(LeftCol・TopRow)が変わったとき。
    Property<TNotifyEvent>          OnTopLeftChanged;
    // 見出しのドラッグで列の幅・行の高さを変えたとき(IsColumn は列なら true)。
    Property<THdrEvent>             OnHeaderSized;
    Property<TGridOperationEvent>   OnColRowInserted;
    Property<TGridOperationEvent>   OnColRowDeleted;
    Property<TGridOperationEvent>   OnColRowMoved;
    Property<TGridOperationEvent>   OnColRowExchanged;

    // ---- docs/adr/0054 ----
    // 列の一覧(TGridColumns)。列を加えると、列の見出し・幅・編集欄の種類を列ごとに決められる。
    ReadOnlyProperty<TGridColumns*> Columns;
    // 現在のセルの列(Columns が空なら nullptr)。
    ReadOnlyProperty<TGridColumn*>  SelectedColumn;
    // OnSelectEditor・OnButtonClick・OnPickListSelect・OnCheckboxToggled は、LCL では TCustomGrid の protected、
    // OnGetCheckboxState・OnSetCheckboxState は TCustomDrawGrid の protected で、TDrawGrid・TStringGrid が published にしている。
    Property<TSelectEditorEvent>     OnSelectEditor;
    // ButtonStyle が cbsEllipsis・cbsButton・cbsButtonColumn の列のボタンがクリックされたとき。
    Property<TOnSelectEvent>         OnButtonClick;
    // ButtonStyle が cbsPickList の列の一覧で選んだとき。
    Property<TNotifyEvent>           OnPickListSelect;
    // TStringGrid では、OnGetCheckboxState・OnSetCheckboxState が無ければ、セルの文字列と ValueChecked・ValueUnchecked で決める。
    // nullptr に戻すとその動きに戻る。
    Property<TGetCheckboxStateEvent> OnGetCheckboxState;
    Property<TSetCheckboxStateEvent> OnSetCheckboxState;
    Property<TToggledCheckboxEvent>  OnCheckboxToggled;

protected:
    explicit TCustomDrawGrid(ObjectHandle handle);
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

    static void BETH_CALL DrawCellTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row,
                                               internal::int_t left, internal::int_t top, internal::int_t right, internal::int_t bottom,
                                               internal::uint_t state, void* data);
    static void BETH_CALL SelectCellTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::bool_t* canSelect, void* data);
    static void BETH_CALL SelectionTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, void* data);
    static void BETH_CALL HeaderClickTrampoline(ObjectHandle sender, internal::int_t isColumn, internal::int_t index, void* data);

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

    // ---- docs/adr/0048 ----
    static TScrollStyle GetScrollBarsImpl(TObject* owner);
    static void         SetScrollBarsImpl(TObject* owner, const TScrollStyle& value);

    // ---- docs/adr/0053 ----
    static bool GetAutoEditImpl(TObject* owner);
    static void SetAutoEditImpl(TObject* owner, const bool& value);
    static TColor GetAlternateColorImpl(TObject* owner);
    static void SetAlternateColorImpl(TObject* owner, const TColor& value);
    static TColor GetFocusColorImpl(TObject* owner);
    static void SetFocusColorImpl(TObject* owner, const TColor& value);
    static TColor GetGridLineColorImpl(TObject* owner);
    static void SetGridLineColorImpl(TObject* owner, const TColor& value);
    static int GetGridLineWidthImpl(TObject* owner);
    static void SetGridLineWidthImpl(TObject* owner, const int& value);
    static TFont* GetTitleFontImpl(TObject* owner);
    static void SetTitleFontImpl(TObject* owner, TFont* const& value);
    static bool GetAutoFillColumnsImpl(TObject* owner);
    static void SetAutoFillColumnsImpl(TObject* owner, const bool& value);
    static bool GetColumnClickSortsImpl(TObject* owner);
    static void SetColumnClickSortsImpl(TObject* owner, const bool& value);
    static TSortOrder GetSortOrderImpl(TObject* owner);
    static void SetSortOrderImpl(TObject* owner, const TSortOrder& value);
    static int GetSortColumnImpl(TObject* owner);
    TGetEditEvent onGetEditText_;
    bool onGetEditTextHooked_ = false;
    static TGetEditEvent GetOnGetEditTextImpl(TObject* owner);
    static void SetOnGetEditTextImpl(TObject* owner, const TGetEditEvent& value);
    static void BETH_CALL GetEditTextTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::str_t value, internal::str_t* result, void* data);
    TSetEditEvent onSetEditText_;
    bool onSetEditTextHooked_ = false;
    static TSetEditEvent GetOnSetEditTextImpl(TObject* owner);
    static void SetOnSetEditTextImpl(TObject* owner, const TSetEditEvent& value);
    static void BETH_CALL SetEditTextTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::str_t value, void* data);
    TValidateEntryEvent onValidateEntry_;
    bool onValidateEntryHooked_ = false;
    static TValidateEntryEvent GetOnValidateEntryImpl(TObject* owner);
    static void SetOnValidateEntryImpl(TObject* owner, const TValidateEntryEvent& value);
    static void BETH_CALL ValidateEntryTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::str_t oldValue, internal::str_t newValue, internal::str_t* result, void* data);
    TOnPrepareCanvasEvent onPrepareCanvas_;
    bool onPrepareCanvasHooked_ = false;
    static TOnPrepareCanvasEvent GetOnPrepareCanvasImpl(TObject* owner);
    static void SetOnPrepareCanvasImpl(TObject* owner, const TOnPrepareCanvasEvent& value);
    static void BETH_CALL PrepareCanvasTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::uint_t state, void* data);
    TOnCompareCells onCompareCells_;
    bool onCompareCellsHooked_ = false;
    static TOnCompareCells GetOnCompareCellsImpl(TObject* owner);
    static void SetOnCompareCellsImpl(TObject* owner, const TOnCompareCells& value);
    static void BETH_CALL CompareCellsTrampoline(ObjectHandle sender, internal::int_t acol, internal::int_t arow, internal::int_t bcol, internal::int_t brow, internal::int_t* result, void* data);
    TNotifyEvent onTopLeftChanged_;
    bool onTopLeftChangedHooked_ = false;
    static TNotifyEvent GetOnTopLeftChangedImpl(TObject* owner);
    static void SetOnTopLeftChangedImpl(TObject* owner, const TNotifyEvent& value);
    static void BETH_CALL TopLeftChangedTrampoline(ObjectHandle sender, void* data);
    THdrEvent onHeaderSized_;
    bool onHeaderSizedHooked_ = false;
    static THdrEvent GetOnHeaderSizedImpl(TObject* owner);
    static void SetOnHeaderSizedImpl(TObject* owner, const THdrEvent& value);
    static void BETH_CALL HeaderSizedTrampoline(ObjectHandle sender, internal::int_t isColumn, internal::int_t index, void* data);
    TGridOperationEvent onColRowInserted_, onColRowDeleted_, onColRowMoved_, onColRowExchanged_;
    bool onColRowInsertedHooked_ = false, onColRowDeletedHooked_ = false, onColRowMovedHooked_ = false, onColRowExchangedHooked_ = false;
    static TGridOperationEvent GetOnColRowInsertedImpl(TObject* owner);
    static void SetOnColRowInsertedImpl(TObject* owner, const TGridOperationEvent& value);
    static TGridOperationEvent GetOnColRowDeletedImpl(TObject* owner);
    static void SetOnColRowDeletedImpl(TObject* owner, const TGridOperationEvent& value);
    static TGridOperationEvent GetOnColRowMovedImpl(TObject* owner);
    static void SetOnColRowMovedImpl(TObject* owner, const TGridOperationEvent& value);
    static TGridOperationEvent GetOnColRowExchangedImpl(TObject* owner);
    static void SetOnColRowExchangedImpl(TObject* owner, const TGridOperationEvent& value);
    static void BETH_CALL ColRowInsertedTrampoline(ObjectHandle sender, internal::int_t isColumn, internal::int_t sIndex, internal::int_t tIndex, void* data);
    static void BETH_CALL ColRowDeletedTrampoline(ObjectHandle sender, internal::int_t isColumn, internal::int_t sIndex, internal::int_t tIndex, void* data);
    static void BETH_CALL ColRowMovedTrampoline(ObjectHandle sender, internal::int_t isColumn, internal::int_t sIndex, internal::int_t tIndex, void* data);
    static void BETH_CALL ColRowExchangedTrampoline(ObjectHandle sender, internal::int_t isColumn, internal::int_t sIndex, internal::int_t tIndex, void* data);
    static void DispatchColRow(ObjectHandle sender, TGridOperationEvent TCustomDrawGrid::*member, internal::int_t isColumn, internal::int_t sIndex, internal::int_t tIndex);

    TFont titleFont_;

    // ---- docs/adr/0054 ----
    TGridColumns columns_;
    static TGridColumns* GetColumnsImpl(TObject* owner);
    static TGridColumn* GetSelectedColumnImpl(TObject* owner);
    TSelectEditorEvent onSelectEditor_;
    bool onSelectEditorHooked_ = false;
    static TSelectEditorEvent GetOnSelectEditorImpl(TObject* owner);
    static void SetOnSelectEditorImpl(TObject* owner, const TSelectEditorEvent& value);
    static void BETH_CALL SelectEditorTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, ObjectHandle* editor, void* data);
    TOnSelectEvent onButtonClick_;
    bool onButtonClickHooked_ = false;
    static TOnSelectEvent GetOnButtonClickImpl(TObject* owner);
    static void SetOnButtonClickImpl(TObject* owner, const TOnSelectEvent& value);
    static void BETH_CALL ButtonClickTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, void* data);
    TNotifyEvent onPickListSelect_;
    bool onPickListSelectHooked_ = false;
    static TNotifyEvent GetOnPickListSelectImpl(TObject* owner);
    static void SetOnPickListSelectImpl(TObject* owner, const TNotifyEvent& value);
    static void BETH_CALL PickListSelectTrampoline(ObjectHandle sender, void* data);
    TGetCheckboxStateEvent onGetCheckboxState_;
    bool onGetCheckboxStateHooked_ = false;
    static TGetCheckboxStateEvent GetOnGetCheckboxStateImpl(TObject* owner);
    static void SetOnGetCheckboxStateImpl(TObject* owner, const TGetCheckboxStateEvent& value);
    static void BETH_CALL GetCheckboxStateTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::int_t* state, void* data);
    TSetCheckboxStateEvent onSetCheckboxState_;
    bool onSetCheckboxStateHooked_ = false;
    static TSetCheckboxStateEvent GetOnSetCheckboxStateImpl(TObject* owner);
    static void SetOnSetCheckboxStateImpl(TObject* owner, const TSetCheckboxStateEvent& value);
    static void BETH_CALL SetCheckboxStateTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::int_t state, void* data);
    TToggledCheckboxEvent onCheckboxToggled_;
    bool onCheckboxToggledHooked_ = false;
    static TToggledCheckboxEvent GetOnCheckboxToggledImpl(TObject* owner);
    static void SetOnCheckboxToggledImpl(TObject* owner, const TToggledCheckboxEvent& value);
    static void BETH_CALL CheckboxToggledTrampoline(ObjectHandle sender, internal::int_t col, internal::int_t row, internal::int_t state, void* data);
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

    // ---- docs/adr/0053 ----
    // セルごとの利用者データ(ポインタ。StringGrid1->Objects[ACol][ARow])。LCL は解釈も解放もしない。
    IndexedProperty2<void*> Objects;
    // 列 i・行 i のセルの文字列の一覧(StringGrid1->Rows[1]->CommaText = "a,b,c"; のように使う)。
    // Cols[i]->Strings[j] は Cells[i][j]、Rows[i]->Strings[j] は Cells[j][i]。グリッドが所有し、グリッドの破棄まで使える。
    ReadOnlyIndexedProperty<TStrings*> Cols;
    ReadOnlyIndexedProperty<TStrings*> Rows;

protected:
    explicit TCustomStringGrid(ObjectHandle handle);
    ~TCustomStringGrid() override = default;

private:
    static std::string GetCellsImpl(TObject* owner, int ACol, int ARow);
    static void        SetCellsImpl(TObject* owner, int ACol, int ARow, const std::string& value);

    // ---- docs/adr/0053 ----
    // Cols・Rows のビュー。LCL の TStrings はグリッドが位置ごとに作って持ち、グリッドの破棄まで同じものを返すため、ハンドルを覚えてよい。
    class LineStrings : public TStrings
    {
    public:
        explicit LineStrings(ObjectHandle handle) : TStrings(handle) {}
    };
    std::map<int, std::unique_ptr<LineStrings>> cols_, rows_;
    static void*     GetObjectsImpl(TObject* owner, int ACol, int ARow);
    static void      SetObjectsImpl(TObject* owner, int ACol, int ARow, void* const& value);
    static TStrings* GetColsImpl(TObject* owner, int Index);
    static TStrings* GetRowsImpl(TObject* owner, int Index);
};

// セルごとに文字列を持つグリッド。
class TStringGrid : public TCustomStringGrid
{
public:
    explicit TStringGrid(TComponent* AOwner);

protected:
    ~TStringGrid() override = default;
};

/* ---------------- HeaderControl ---------------- */

class TCustomHeaderControl;

// ヘッダーコントロールのセクション(LCL の THeaderSection。TCollectionItem)。同じセクションには常に同じポインタが返る。
// ラッパーは、セクションが破棄されたとき(THeaderSections::Delete・Clear、ヘッダーコントロールの破棄)に delete される。
class THeaderSection : public TPersistent
{
public:
    Property<std::string> Text;
    // Visible が false なら 0 を返す。
    Property<int>         Width;
    Property<int>         MinWidth;
    Property<int>         MaxWidth;
    Property<TAlignment>  Alignment;
    Property<bool>        Visible;
    // 並び順。書き換えるとセクションが移動する。
    Property<int>         Index;
    // クライアント座標での左端・右端。
    ReadOnlyProperty<int> Left;
    ReadOnlyProperty<int> Right;
    // 並べ替えても変わらない位置。
    ReadOnlyProperty<int> OriginalIndex;
    // 画像の、ヘッダーの Images での位置(-1 なら無し。docs/adr/0030)。
    Property<int> ImageIndex;

private:
    friend class ItemRegistry;
    friend class THeaderSections;
    friend class TCustomHeaderControl;

    explicit THeaderSection(ObjectHandle handle);
    ~THeaderSection() override = default;
    static THeaderSection* Wrap(ObjectHandle handle) { return ItemRegistry::Wrap<THeaderSection>(handle); }

    static std::string GetTextImpl(TObject* owner);
    static void        SetTextImpl(TObject* owner, const std::string& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetMinWidthImpl(TObject* owner);
    static void        SetMinWidthImpl(TObject* owner, const int& value);
    static int         GetMaxWidthImpl(TObject* owner);
    static void        SetMaxWidthImpl(TObject* owner, const int& value);
    static TAlignment  GetAlignmentImpl(TObject* owner);
    static void        SetAlignmentImpl(TObject* owner, const TAlignment& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static int         GetIndexImpl(TObject* owner);
    static void        SetIndexImpl(TObject* owner, const int& value);
    static int         GetLeftImpl(TObject* owner);
    static int         GetRightImpl(TObject* owner);
    static int         GetOriginalIndexImpl(TObject* owner);

    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
};

// セクションの一覧(LCL の THeaderSections。TCollection)。ヘッダーコントロールの値メンバとして持つ非所有のビュー。
class THeaderSections : public TPersistent
{
public:
    explicit THeaderSections(ObjectHandle handle);
    ~THeaderSections() override = default;

    ReadOnlyProperty<int> Count;
    // HeaderControl1->Sections->Items[i]。
    ReadOnlyIndexedProperty<THeaderSection*> Items;

    // 末尾に(Insert は Index の位置に)空のセクションを追加して返す(Text 等はその後で設定する。VCL と同じ)。
    THeaderSection* Add();
    THeaderSection* Insert(int Index);
    // セクションを削除する(セクションのラッパーも delete される)。
    void            Delete(int Index);
    void            Clear();
    void            BeginUpdate();
    void            EndUpdate();

private:
    static THeaderSection* GetItemsImpl(TObject* owner, int Index);
    static int             GetCountImpl(TObject* owner);
};

// ドラッグで幅を変えている間の段階(LCL の TSectionTrackState と同じ値)。
enum TSectionTrackState
{
    tsTrackBegin = 0,
    tsTrackMove,
    tsTrackEnd
};

// セクションを対象とするイベント。1 つ目の引数は LCL と同じくヘッダーコントロール(OnSectionDrag だけは Sender)。
using TCustomSectionNotifyEvent = std::function<void(TCustomHeaderControl* HeaderControl, THeaderSection* Section)>;
using TCustomSectionTrackEvent  = std::function<void(TCustomHeaderControl* HeaderControl, THeaderSection* Section,
                                                     int Width, TSectionTrackState State)>;
// DragReorder のとき、セクションをドラッグで移動する前に呼ばれる。AllowDrag を false にすると移動させない。
using TSectionDragEvent         = std::function<void(TObject* Sender, THeaderSection* FromSection, THeaderSection* ToSection,
                                                     bool& AllowDrag)>;

// 以下のメンバは LCL の TCustomHeaderControl の public/published。セクションは OS のコントロールではなく LCL が描画する。
class TCustomHeaderControl : public TCustomControl
{
public:
    ReadOnlyProperty<THeaderSections*> Sections;
    // true にすると、セクションをドラッグで並べ替えられる。
    Property<bool>                     DragReorder;
    // 並べ替えても変わらない位置(OriginalIndex)のセクション。無ければ nullptr。
    ReadOnlyIndexedProperty<THeaderSection*> SectionFromOriginalIndex;

    // セクションがクリックされたとき。
    Property<TCustomSectionNotifyEvent> OnSectionClick;
    // ドラッグで幅を変え終えたとき。
    Property<TCustomSectionNotifyEvent> OnSectionResize;
    // セクションの境界がダブルクリックされたとき。
    Property<TCustomSectionNotifyEvent> OnSectionSeparatorDblClick;
    // ドラッグで幅を変えている間(開始・移動・終了)。
    Property<TCustomSectionTrackEvent>  OnSectionTrack;
    Property<TSectionDragEvent>         OnSectionDrag;
    // ドラッグでの並べ替えが終わったとき。
    Property<TNotifyEvent>              OnSectionEndDrag;
    // セクションの画像リスト(docs/adr/0030)。各セクションの画像は THeaderSection::ImageIndex。
    Property<TCustomImageList*> Images;

    // クライアント座標 P にあるセクションの位置。無ければ -1。
    int GetSectionAt(const TPoint& P) const;

protected:
    explicit TCustomHeaderControl(ObjectHandle handle);
    ~TCustomHeaderControl() override = default;

private:
    THeaderSections           sections_;
    TCustomSectionNotifyEvent onSectionClick_;
    TCustomSectionNotifyEvent onSectionResize_;
    TCustomSectionNotifyEvent onSectionSeparatorDblClick_;
    TCustomSectionTrackEvent  onSectionTrack_;
    TSectionDragEvent         onSectionDrag_;
    TNotifyEvent              onSectionEndDrag_;
    bool onSectionClickHooked_             = false;
    bool onSectionResizeHooked_            = false;
    bool onSectionSeparatorDblClickHooked_ = false;
    bool onSectionTrackHooked_             = false;
    bool onSectionDragHooked_              = false;
    bool onSectionEndDragHooked_           = false;

    static void BETH_CALL SectionClickTrampoline(ObjectHandle sender, ObjectHandle section, void* data);
    static void BETH_CALL SectionResizeTrampoline(ObjectHandle sender, ObjectHandle section, void* data);
    static void BETH_CALL SectionSeparatorDblClickTrampoline(ObjectHandle sender, ObjectHandle section, void* data);
    static void BETH_CALL SectionTrackTrampoline(ObjectHandle sender, ObjectHandle section, internal::int_t width, internal::int_t state, void* data);
    static void BETH_CALL SectionDragTrampoline(ObjectHandle sender, ObjectHandle fromSection, ObjectHandle toSection, internal::bool_t* allow, void* data);
    static void BETH_CALL SectionEndDragTrampoline(ObjectHandle sender, void* data);

    static THeaderSections* GetSectionsImpl(TObject* owner);
    static bool             GetDragReorderImpl(TObject* owner);
    static void             SetDragReorderImpl(TObject* owner, const bool& value);
    static THeaderSection*  GetSectionFromOriginalIndexImpl(TObject* owner, int OriginalIndex);

    static TCustomSectionNotifyEvent GetOnSectionClickImpl(TObject* owner);
    static void                      SetOnSectionClickImpl(TObject* owner, const TCustomSectionNotifyEvent& value);
    static TCustomSectionNotifyEvent GetOnSectionResizeImpl(TObject* owner);
    static void                      SetOnSectionResizeImpl(TObject* owner, const TCustomSectionNotifyEvent& value);
    static TCustomSectionNotifyEvent GetOnSectionSeparatorDblClickImpl(TObject* owner);
    static void                      SetOnSectionSeparatorDblClickImpl(TObject* owner, const TCustomSectionNotifyEvent& value);
    static TCustomSectionTrackEvent  GetOnSectionTrackImpl(TObject* owner);
    static void                      SetOnSectionTrackImpl(TObject* owner, const TCustomSectionTrackEvent& value);
    static TSectionDragEvent         GetOnSectionDragImpl(TObject* owner);
    static void                      SetOnSectionDragImpl(TObject* owner, const TSectionDragEvent& value);
    static TNotifyEvent              GetOnSectionEndDragImpl(TObject* owner);
    static void                      SetOnSectionEndDragImpl(TObject* owner, const TNotifyEvent& value);

    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);
};

// 列の見出しを並べたコントロール。
class THeaderControl : public TCustomHeaderControl
{
public:
    explicit THeaderControl(TComponent* AOwner);

protected:
    ~THeaderControl() override = default;
};

/* ---------------- ToolBar ---------------- */

// 縁を描く辺の集合(LCL の TEdgeBorders に対応)。
enum TEdgeBorder
{
    ebLeft,
    ebTop,
    ebRight,
    ebBottom
};
using TEdgeBorders = Set<TEdgeBorder>;

// 縁の描き方(LCL の TEdgeStyle と同じ値)。
enum TEdgeStyle
{
    esNone = 0,
    esRaised,
    esLowered
};

// ツールボタンの種類(LCL の TToolButtonStyle と同じ値)。
enum TToolButtonStyle
{
    tbsButton = 0,  // 普通のボタン
    tbsCheck,       // クリックで Down が切り替わる(Grouped なら隣り合うボタンのどれか 1 つだけが Down)
    tbsDropDown,    // 右に矢印が付き、矢印で DropdownMenu を表示する
    tbsSeparator,   // 空白
    tbsDivider,     // 線の入った区切り
    tbsButtonDrop   // ボタンと一体の矢印(どこを押しても DropdownMenu を表示する)
};

// 以下のメンバは LCL の TToolWindow の public(TToolBar が published にしている)。
class TToolWindow : public TCustomControl
{
public:
    Property<TEdgeBorders> EdgeBorders;
    Property<TEdgeStyle>   EdgeInner;
    Property<TEdgeStyle>   EdgeOuter;

    void BeginUpdate();
    void EndUpdate();

protected:
    explicit TToolWindow(ObjectHandle handle);
    ~TToolWindow() override = default;

private:
    static TEdgeBorders GetEdgeBordersImpl(TObject* owner);
    static void         SetEdgeBordersImpl(TObject* owner, const TEdgeBorders& value);
    static TEdgeStyle   GetEdgeInnerImpl(TObject* owner);
    static void         SetEdgeInnerImpl(TObject* owner, const TEdgeStyle& value);
    static TEdgeStyle   GetEdgeOuterImpl(TObject* owner);
    static void         SetEdgeOuterImpl(TObject* owner, const TEdgeStyle& value);
};

class TToolButton;

// ツールバー。ボタン(TToolButton)は、Parent をツールバーにすると追加される(VCL と同じ)。既定の Align は alTop。
class TToolBar : public TToolWindow
{
public:
    explicit TToolBar(TComponent* AOwner);

    ReadOnlyProperty<int>                ButtonCount;
    // 並び順のボタン(ToolBar1->Buttons[i])。
    ReadOnlyIndexedProperty<TToolButton*> Buttons;
    // LCL では行数ではなく、Wrapable が false のときに Wrap のボタンで折り返した回数(折り返しが無ければ 0)。
    ReadOnlyProperty<int>                RowCount;
    Property<int>                        ButtonHeight;
    Property<int>                        ButtonWidth;
    // tbsDropDown のボタンの矢印部分の幅。
    Property<int>                        DropDownWidth;
    // 最初のボタンの左の余白。
    Property<int>                        Indent;
    Property<bool>                       Flat;
    // true なら、ボタンの文字をアイコンの右に置く。
    Property<bool>                       List;
    // true なら、ボタンに Caption を表示する(既定は false)。
    Property<bool>                       ShowCaptions;
    Property<bool>                       Transparent;
    // true なら、幅に収まらないボタンを次の行へ折り返す(既定は true)。
    Property<bool>                       Wrapable;
    // ボタンの画像リスト(docs/adr/0030)。HotImages はマウスが上にあるとき、DisabledImages は無効のときに使う(設定しなければ Images から LCL が作る)。各ボタンの画像は TToolButton::ImageIndex。
    Property<TCustomImageList*> Images;
    Property<TCustomImageList*> HotImages;
    Property<TCustomImageList*> DisabledImages;

    void SetButtonSize(int NewButtonWidth, int NewButtonHeight);

protected:
    ~TToolBar() override = default;

private:
    static int          GetButtonCountImpl(TObject* owner);
    static TToolButton* GetButtonsImpl(TObject* owner, int Index);
    static int          GetRowCountImpl(TObject* owner);
    static int          GetButtonHeightImpl(TObject* owner);
    static void         SetButtonHeightImpl(TObject* owner, const int& value);
    static int          GetButtonWidthImpl(TObject* owner);
    static void         SetButtonWidthImpl(TObject* owner, const int& value);
    static int          GetDropDownWidthImpl(TObject* owner);
    static void         SetDropDownWidthImpl(TObject* owner, const int& value);
    static int          GetIndentImpl(TObject* owner);
    static void         SetIndentImpl(TObject* owner, const int& value);
    static bool         GetFlatImpl(TObject* owner);
    static void         SetFlatImpl(TObject* owner, const bool& value);
    static bool         GetListImpl(TObject* owner);
    static void         SetListImpl(TObject* owner, const bool& value);
    static bool         GetShowCaptionsImpl(TObject* owner);
    static void         SetShowCaptionsImpl(TObject* owner, const bool& value);
    static bool         GetTransparentImpl(TObject* owner);
    static void         SetTransparentImpl(TObject* owner, const bool& value);
    static bool         GetWrapableImpl(TObject* owner);
    static void         SetWrapableImpl(TObject* owner, const bool& value);

    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);
    static TCustomImageList* GetHotImagesImpl(TObject* owner);
    static void SetHotImagesImpl(TObject* owner, TCustomImageList* const& value);
    static TCustomImageList* GetDisabledImagesImpl(TObject* owner);
    static void SetDisabledImagesImpl(TObject* owner, TCustomImageList* const& value);
};

// ツールバーのボタン。Caption・OnClick は TControl のものを使う。
class TToolButton : public TGraphicControl
{
public:
    explicit TToolButton(TComponent* AOwner);

    // tbsCheck で Grouped のとき、すべてのボタンを上げた状態にできるか。
    Property<bool>             AllowAllUp;
    // 押された状態(tbsCheck はクリックで切り替わる)。
    Property<bool>             Down;
    Property<bool>             Grouped;
    Property<bool>             Indeterminate;
    Property<bool>             Marked;
    Property<bool>             ShowCaption;
    // true なら、このボタンの後で行を折り返す。
    Property<bool>             Wrap;
    Property<TToolButtonStyle> Style;
    // tbsDropDown・tbsButtonDrop の矢印で表示するポップアップメニュー。
    Property<TPopupMenu*>      DropdownMenu;
    // 設定すると、そのメニュー項目の Caption・Enabled 等を写す。マウスで押すと、その項目の OnClick を呼んでから
    // 子の項目をポップアップメニューとして表示する(DropdownMenu と同じく、メニューを閉じるまで戻らない)。
    Property<TMenuItem*>       MenuItem;
    // tbsDropDown・tbsButtonDrop の矢印がクリックされたとき(DropdownMenu を表示する前)。
    Property<TNotifyEvent>     OnArrowClick;
    // ツールバーの中での位置(ツールバーに置かれていなければ -1)。
    ReadOnlyProperty<int>      Index;
    // 画像の、ツールバーの Images での位置(-1 なら無し。docs/adr/0030)。
    Property<int> ImageIndex;

    // OnClick を呼ぶ(tbsCheck の Down は変えない。Down の切り替えはマウスを離したときに LCL が行う)。
    void Click();
    // OnArrowClick を呼ぶ(DropdownMenu は表示しない)。
    void ArrowClick();
    // ボタンのクライアント座標 X, Y が矢印の部分にあるか。
    bool PointInArrow(int X, int Y) const;

protected:
    ~TToolButton() override = default;

private:
    TNotifyEvent onArrowClick_;
    bool         onArrowClickHooked_ = false;
    static void BETH_CALL ArrowClickTrampoline(ObjectHandle sender, void* data);

    static bool             GetAllowAllUpImpl(TObject* owner);
    static void             SetAllowAllUpImpl(TObject* owner, const bool& value);
    static bool             GetDownImpl(TObject* owner);
    static void             SetDownImpl(TObject* owner, const bool& value);
    static bool             GetGroupedImpl(TObject* owner);
    static void             SetGroupedImpl(TObject* owner, const bool& value);
    static bool             GetIndeterminateImpl(TObject* owner);
    static void             SetIndeterminateImpl(TObject* owner, const bool& value);
    static bool             GetMarkedImpl(TObject* owner);
    static void             SetMarkedImpl(TObject* owner, const bool& value);
    static bool             GetShowCaptionImpl(TObject* owner);
    static void             SetShowCaptionImpl(TObject* owner, const bool& value);
    static bool             GetWrapImpl(TObject* owner);
    static void             SetWrapImpl(TObject* owner, const bool& value);
    static TToolButtonStyle GetStyleImpl(TObject* owner);
    static void             SetStyleImpl(TObject* owner, const TToolButtonStyle& value);
    static TPopupMenu*      GetDropdownMenuImpl(TObject* owner);
    static void             SetDropdownMenuImpl(TObject* owner, TPopupMenu* const& value);
    static TMenuItem*       GetMenuItemImpl(TObject* owner);
    static void             SetMenuItemImpl(TObject* owner, TMenuItem* const& value);
    static TNotifyEvent     GetOnArrowClickImpl(TObject* owner);
    static void             SetOnArrowClickImpl(TObject* owner, const TNotifyEvent& value);
    static int              GetIndexImpl(TObject* owner);

    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
};

/* ---------------- CoolBar ---------------- */

// バンドの左端のつまみの描き方(LCL の TGrabStyle と同じ値)。
enum TGrabStyle
{
    gsSimple = 0,
    gsDouble,
    gsHorLines,
    gsVerLines,
    gsGripper,
    gsButton
};

// クールバーのバンド(LCL の TCoolBand。TCollectionItem)。同じバンドには常に同じポインタが返る。
// ウィンドウを持つコントロールの Parent をクールバーにすると LCL がバンドを自動で追加し、コントロールを外すと削除する。
// ラッパーは、バンドが破棄されたとき(どの経路でも)に delete される。
class TCoolBand : public TPersistent
{
public:
    Property<std::string> Text;
    Property<int>         Width;
    Property<int>         MinWidth;
    Property<int>         MinHeight;
    // true なら、このバンドから新しい行を始める(既定は true)。
    Property<bool>        Break;
    Property<bool>        Visible;
    Property<bool>        FixedSize;
    Property<bool>        FixedBackground;
    Property<bool>        HorizontalOnly;
    Property<TColor>      Color;
    Property<bool>        ParentColor;
    // 並び順。書き換えるとバンドが移動する。
    Property<int>         Index;
    // バンドに置くコントロール。設定するとそのコントロールの Parent がクールバーになり、Align は alNone になる。
    Property<TControl*>   Control;
    // クールバーのクライアント座標での位置(配置の計算の後で決まる)。
    ReadOnlyProperty<int> Left;
    ReadOnlyProperty<int> Top;
    ReadOnlyProperty<int> Right;
    ReadOnlyProperty<int> Height;
    // 画像の、クールバーの Images での位置(-1 なら無し。docs/adr/0030)。
    Property<int> ImageIndex;
    // 背景の画像。バンドが所有する TBitmap のビューで、代入は内容のコピー。
    Property<TBitmap*> Bitmap;

    // 幅を、置いているコントロールに合わせる。
    void AutosizeWidth();

private:
    friend class ItemRegistry;
    friend class TCoolBands;

    explicit TCoolBand(ObjectHandle handle);
    ~TCoolBand() override = default;
    static TCoolBand* Wrap(ObjectHandle handle) { return ItemRegistry::Wrap<TCoolBand>(handle); }

    static std::string GetTextImpl(TObject* owner);
    static void        SetTextImpl(TObject* owner, const std::string& value);
    static int         GetWidthImpl(TObject* owner);
    static void        SetWidthImpl(TObject* owner, const int& value);
    static int         GetMinWidthImpl(TObject* owner);
    static void        SetMinWidthImpl(TObject* owner, const int& value);
    static int         GetMinHeightImpl(TObject* owner);
    static void        SetMinHeightImpl(TObject* owner, const int& value);
    static bool        GetBreakImpl(TObject* owner);
    static void        SetBreakImpl(TObject* owner, const bool& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static bool        GetFixedSizeImpl(TObject* owner);
    static void        SetFixedSizeImpl(TObject* owner, const bool& value);
    static bool        GetFixedBackgroundImpl(TObject* owner);
    static void        SetFixedBackgroundImpl(TObject* owner, const bool& value);
    static bool        GetHorizontalOnlyImpl(TObject* owner);
    static void        SetHorizontalOnlyImpl(TObject* owner, const bool& value);
    static TColor      GetColorImpl(TObject* owner);
    static void        SetColorImpl(TObject* owner, const TColor& value);
    static bool        GetParentColorImpl(TObject* owner);
    static void        SetParentColorImpl(TObject* owner, const bool& value);
    static int         GetIndexImpl(TObject* owner);
    static void        SetIndexImpl(TObject* owner, const int& value);
    static TControl*   GetControlImpl(TObject* owner);
    static void        SetControlImpl(TObject* owner, TControl* const& value);
    static int         GetLeftImpl(TObject* owner);
    static int         GetTopImpl(TObject* owner);
    static int         GetRightImpl(TObject* owner);
    static int         GetHeightImpl(TObject* owner);

    TBitmap bitmap_;
    static int GetImageIndexImpl(TObject* owner);
    static void SetImageIndexImpl(TObject* owner, const int& value);
    static TBitmap* GetBitmapImpl(TObject* owner);
    static void SetBitmapImpl(TObject* owner, TBitmap* const& value);
};

// バンドの一覧(LCL の TCoolBands。TCollection)。クールバーの値メンバとして持つ非所有のビュー。
class TCoolBands : public TPersistent
{
public:
    explicit TCoolBands(ObjectHandle handle);
    ~TCoolBands() override = default;

    ReadOnlyProperty<int> Count;
    // CoolBar1->Bands->Items[i]。
    ReadOnlyIndexedProperty<TCoolBand*> Items;

    // 空のバンドを末尾に追加して返す(Text・Control 等はその後で設定する)。
    TCoolBand* Add();
    // バンドを削除する(バンドのラッパーも delete される。置いていたコントロールは破棄されない)。
    void       Delete(int Index);
    void       Clear();
    void       BeginUpdate();
    void       EndUpdate();
    // Control がそのコントロールのバンド(無ければ nullptr・-1)。
    TCoolBand* FindBand(TControl* AControl) const;
    int        FindBandIndex(TControl* AControl) const;

private:
    static TCoolBand* GetItemsImpl(TObject* owner, int Index);
    static int        GetCountImpl(TObject* owner);
};

// 以下のメンバは LCL の TCustomCoolBar の public(TCoolBar が published にしている)。
// Align は TControl のものを使う(alLeft/alRight にすると Vertical も true になる)。既定は alTop。
class TCustomCoolBar : public TToolWindow
{
public:
    ReadOnlyProperty<TCoolBands*> Bands;
    // true なら、ドラッグでバンドの幅を変えられない。
    Property<bool>                FixedSize;
    // true なら、ドラッグでバンドを並べ替えられない。
    Property<bool>                FixedOrder;
    Property<TGrabStyle>          GrabStyle;
    Property<int>                 GrabWidth;
    Property<int>                 HorizontalSpacing;
    Property<int>                 VerticalSpacing;
    // true なら、バンドの Text を表示する(既定は true)。
    Property<bool>                ShowText;
    Property<bool>                Themed;
    Property<bool>                Vertical;
    // ドラッグでバンドを動かす・幅を変えて、マウスを離したとき。
    Property<TNotifyEvent>        OnChange;
    // バンドの画像リスト(docs/adr/0030)。各バンドの画像は TCoolBand::ImageIndex。
    Property<TCustomImageList*> Images;
    // 背景の画像。クールバーが所有する TBitmap のビューで、代入は内容のコピー。
    Property<TBitmap*> Bitmap;

    // すべてのバンドの幅を、置いているコントロールに合わせる。
    void AutosizeBands();
    // クライアント座標 X, Y にあるバンドの表示上の位置(ABand。無ければ負)と、つまみの上か(AGrabber)。
    void MouseToBandPos(int X, int Y, int& ABand, bool& AGrabber) const;

protected:
    explicit TCustomCoolBar(ObjectHandle handle);
    ~TCustomCoolBar() override = default;

private:
    TCoolBands   bands_;
    TNotifyEvent onChange_;
    bool         onChangeHooked_ = false;
    static void BETH_CALL ChangeTrampoline(ObjectHandle sender, void* data);

    static TCoolBands*  GetBandsImpl(TObject* owner);
    static bool         GetFixedSizeImpl(TObject* owner);
    static void         SetFixedSizeImpl(TObject* owner, const bool& value);
    static bool         GetFixedOrderImpl(TObject* owner);
    static void         SetFixedOrderImpl(TObject* owner, const bool& value);
    static TGrabStyle   GetGrabStyleImpl(TObject* owner);
    static void         SetGrabStyleImpl(TObject* owner, const TGrabStyle& value);
    static int          GetGrabWidthImpl(TObject* owner);
    static void         SetGrabWidthImpl(TObject* owner, const int& value);
    static int          GetHorizontalSpacingImpl(TObject* owner);
    static void         SetHorizontalSpacingImpl(TObject* owner, const int& value);
    static int          GetVerticalSpacingImpl(TObject* owner);
    static void         SetVerticalSpacingImpl(TObject* owner, const int& value);
    static bool         GetShowTextImpl(TObject* owner);
    static void         SetShowTextImpl(TObject* owner, const bool& value);
    static bool         GetThemedImpl(TObject* owner);
    static void         SetThemedImpl(TObject* owner, const bool& value);
    static bool         GetVerticalImpl(TObject* owner);
    static void         SetVerticalImpl(TObject* owner, const bool& value);
    static TNotifyEvent GetOnChangeImpl(TObject* owner);
    static void         SetOnChangeImpl(TObject* owner, const TNotifyEvent& value);

    TBitmap bitmap_;
    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void SetImagesImpl(TObject* owner, TCustomImageList* const& value);
    static TBitmap* GetBitmapImpl(TObject* owner);
    static void SetBitmapImpl(TObject* owner, TBitmap* const& value);
};

// 並べ替え・幅の変更ができるバンドに、コントロールを置くバー。
class TCoolBar : public TCustomCoolBar
{
public:
    explicit TCoolBar(TComponent* AOwner);

protected:
    ~TCoolBar() override = default;
};

/* ---------------- Timer ---------------- */

class TCustomTimer : public TComponent
{
public:
    Property<int>          Interval;
    Property<bool>         Enabled;
    Property<TNotifyEvent> OnTimer;

protected:
    explicit TCustomTimer(ObjectHandle handle);
    ~TCustomTimer() override = default;

private:
    TNotifyEvent onTimer_;
    bool         onTimerHooked_ = false;
    static void BETH_CALL TimerTrampoline(ObjectHandle sender, void* data);
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

/* ---------------- Action(docs/adr/0046) ---------------- */

class TCustomActionList;

// 操作(LCL の TBasicAction)。コントロール・メニュー項目の Action に割り当てると、選んだときに OnExecute が呼ばれる。
class TBasicAction : public TComponent
{
public:
    // OnExecute を呼ぶ(ActionList の OnExecute で Handled にされたときは呼ばない)。呼んだら true。
    bool Execute();
    // OnUpdate を呼ぶ(アイドルのときに LCL が呼ぶものを、すぐに呼ぶ)。
    bool Update();
    // Execute を起こしたコントロール・メニュー項目(プログラムから Execute したときは nullptr)。
    ReadOnlyProperty<TComponent*> ActionComponent;

    // 実行するとき。Sender は Action(起こしたものは ActionComponent)。
    Property<TNotifyEvent> OnExecute;
    // アイドルのときに LCL が呼ぶ。Enabled・Checked 等をここで今の状態に合わせる(割り当てたコントロールにも写る)。
    // 呼ばれるのは、表示中のフォームのコントロール・メインメニューの項目に割り当てた Action だけ(VCL と同じ)。
    Property<TNotifyEvent> OnUpdate;

protected:
    explicit TBasicAction(ObjectHandle handle);
    ~TBasicAction() override = default;

private:
    TNotifyEvent onExecute_;
    TNotifyEvent onUpdate_;
    bool onExecuteHooked_ = false;
    bool onUpdateHooked_  = false;
    static void BETH_CALL ExecuteTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL UpdateTrampoline(ObjectHandle sender, void* data);
    static TComponent*  GetActionComponentImpl(TObject* owner);
    static TNotifyEvent GetOnExecuteImpl(TObject* owner);
    static void         SetOnExecuteImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnUpdateImpl(TObject* owner);
    static void         SetOnUpdateImpl(TObject* owner, const TNotifyEvent& value);
};

// ActionList に入る Action(LCL の TContainedAction)。
class TContainedAction : public TBasicAction
{
public:
    // 属する ActionList。代入すると一覧の末尾に入る(nullptr なら一覧から外す)。
    Property<TCustomActionList*> ActionList;
    // 分類(ActionList の中でまとめて扱うための名前。動作には影響しない)。
    Property<std::string>        Category;
    // ActionList の中の位置。書き換えると移動する。
    Property<int>                Index;

protected:
    explicit TContainedAction(ObjectHandle handle);
    ~TContainedAction() override = default;

private:
    static TCustomActionList* GetActionListImpl(TObject* owner);
    static void               SetActionListImpl(TObject* owner, TCustomActionList* const& value);
    static std::string        GetCategoryImpl(TObject* owner);
    static void               SetCategoryImpl(TObject* owner, const std::string& value);
    static int                GetIndexImpl(TObject* owner);
    static void               SetIndexImpl(TObject* owner, const int& value);
};

// 表示の状態を持つ Action(LCL の TCustomAction)。値を変えると、割り当てたコントロール・メニュー項目にも写る。
class TCustomAction : public TContainedAction
{
public:
    Property<std::string> Caption;
    Property<std::string> Hint;
    Property<bool>        Checked;
    // true にすると、実行するたびに Checked が反転する。
    Property<bool>        AutoCheck;
    // 0 でなければ、同じ GroupIndex の Action のうち 1 つだけが Checked になる。
    Property<int>         GroupIndex;
    Property<bool>        Enabled;
    Property<bool>        Visible;
    // 割り当てたメニュー項目・ボタンの画像の、ActionList の Images での位置(-1 なら無し)。
    Property<int>         ImageIndex;
    // フォームにフォーカスがあるときにこのキーを押すと実行する(割り当てたメニュー項目にも表示される)。
    Property<TShortCut>   ShortCut;
    // true(既定)なら、OnExecute が無いとき Enabled を false にする。
    Property<bool>        DisableIfNoHandler;

protected:
    explicit TCustomAction(ObjectHandle handle);
    ~TCustomAction() override = default;

private:
    static std::string GetCaptionImpl(TObject* owner);
    static void        SetCaptionImpl(TObject* owner, const std::string& value);
    static std::string GetHintImpl(TObject* owner);
    static void        SetHintImpl(TObject* owner, const std::string& value);
    static bool        GetCheckedImpl(TObject* owner);
    static void        SetCheckedImpl(TObject* owner, const bool& value);
    static bool        GetAutoCheckImpl(TObject* owner);
    static void        SetAutoCheckImpl(TObject* owner, const bool& value);
    static int         GetGroupIndexImpl(TObject* owner);
    static void        SetGroupIndexImpl(TObject* owner, const int& value);
    static bool        GetEnabledImpl(TObject* owner);
    static void        SetEnabledImpl(TObject* owner, const bool& value);
    static bool        GetVisibleImpl(TObject* owner);
    static void        SetVisibleImpl(TObject* owner, const bool& value);
    static int         GetImageIndexImpl(TObject* owner);
    static void        SetImageIndexImpl(TObject* owner, const int& value);
    static TShortCut   GetShortCutImpl(TObject* owner);
    static void        SetShortCutImpl(TObject* owner, const TShortCut& value);
    static bool        GetDisableIfNoHandlerImpl(TObject* owner);
    static void        SetDisableIfNoHandlerImpl(TObject* owner, const bool& value);
};

class TAction : public TCustomAction
{
public:
    explicit TAction(TComponent* AOwner);

protected:
    ~TAction() override = default;
};

// ActionList の状態(LCL の TActionListState と同じ値)。asSuspended は Action を実行・更新しない。
enum TActionListState { asNormal, asSuspended, asSuspendedEnabled };

// ActionList の OnExecute・OnUpdate。Handled を true にすると、Action の OnExecute・OnUpdate を呼ばない。
// Sender は ActionList(VCL の TActionEvent には Sender が無いが、ほかのイベントと同じく先頭に置く)。
using TActionEvent = std::function<void(TObject* Sender, TBasicAction* Action, bool& Handled)>;

// Action の一覧(LCL の TCustomActionList)。
class TCustomActionList : public TComponent
{
public:
    ReadOnlyIndexedProperty<TContainedAction*> Actions;
    ReadOnlyProperty<int>                      ActionCount;
    // Action の ImageIndex が指す画像リスト。割り当てたメニュー項目・ボタンにも使われる。
    Property<TCustomImageList*>                Images;
    Property<TActionListState>                 State;
    // どの Action を実行するときにも、Action の OnExecute の前に呼ばれる。
    Property<TActionEvent>                     OnExecute;
    // どの Action を更新するときにも、Action の OnUpdate の前に呼ばれる。
    Property<TActionEvent>                     OnUpdate;

protected:
    explicit TCustomActionList(ObjectHandle handle);
    ~TCustomActionList() override = default;

private:
    TActionEvent onExecute_;
    TActionEvent onUpdate_;
    bool onExecuteHooked_ = false;
    bool onUpdateHooked_  = false;
    static void BETH_CALL ExecuteTrampoline(ObjectHandle sender, ObjectHandle action, internal::bool_t* handled, void* data);
    static void BETH_CALL UpdateTrampoline(ObjectHandle sender, ObjectHandle action, internal::bool_t* handled, void* data);
    static TContainedAction* GetActionsImpl(TObject* owner, int Index);
    static int               GetActionCountImpl(TObject* owner);
    static TCustomImageList* GetImagesImpl(TObject* owner);
    static void              SetImagesImpl(TObject* owner, TCustomImageList* const& value);
    static TActionListState  GetStateImpl(TObject* owner);
    static void              SetStateImpl(TObject* owner, const TActionListState& value);
    static TActionEvent      GetOnExecuteImpl(TObject* owner);
    static void              SetOnExecuteImpl(TObject* owner, const TActionEvent& value);
    static TActionEvent      GetOnUpdateImpl(TObject* owner);
    static void              SetOnUpdateImpl(TObject* owner, const TActionEvent& value);
};

class TActionList : public TCustomActionList
{
public:
    explicit TActionList(TComponent* AOwner);

protected:
    ~TActionList() override = default;
};

/* ---------------- Dialogs(docs/adr/0033) ---------------- */

// ダイアログの共通の基底(LCL の TCommonDialog)。VCL と同じく、プロパティを設定して Execute() を呼び、結果を bool で受け取る
// (if (OpenDialog1->Execute()) Memo1->Lines->LoadFromFile(OpenDialog1->FileName);)。
// TComponent なので、他のコンポーネントと同じく new で生成し、Owner に任せるか Free() で破棄する。1 つを何度でも Execute できる。
class TCommonDialog : public TComponent
{
public:
    // ダイアログのタイトル(空なら OS・LCL の既定)。Win32 の TFontDialog では使われない。
    Property<std::string>      Title;
    // ダイアログが表示されたとき・閉じたとき。
    Property<TNotifyEvent>     OnShow;
    Property<TNotifyEvent>     OnClose;
    // OK で閉じようとしたとき(CanClose を false にすると閉じない)。Win32 ではファイルのダイアログでだけ呼ばれる。
    Property<TCloseQueryEvent> OnCanClose;

    // ダイアログを表示する。閉じるまで戻らず、OK で閉じたら true、キャンセルなら false を返す
    // (TFindDialog・TReplaceDialog はモードレスで、表示してすぐ true を返す)。
    bool Execute();

protected:
    // 具象クラスの派生(TSaveDialog・TReplaceDialog 等)が、生成したハンドルを基底の具象クラスに渡すための目印。
    // 公開のコンストラクタ (TComponent* AOwner) と引数の型を変え、new TOpenDialog(nullptr) を曖昧にしない。
    struct DerivedTag {};

    explicit TCommonDialog(ObjectHandle handle);
    ~TCommonDialog() override = default;

private:
    TNotifyEvent     onShow_;
    TNotifyEvent     onClose_;
    TCloseQueryEvent onCanClose_;
    bool onShowHooked_     = false;
    bool onCloseHooked_    = false;
    bool onCanCloseHooked_ = false;

    static void BETH_CALL ShowTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL CloseTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL CanCloseTrampoline(ObjectHandle sender, internal::bool_t* canClose, void* data);

    static std::string      GetTitleImpl(TObject* owner);
    static void             SetTitleImpl(TObject* owner, const std::string& value);
    static TNotifyEvent     GetOnShowImpl(TObject* owner);
    static void             SetOnShowImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent     GetOnCloseImpl(TObject* owner);
    static void             SetOnCloseImpl(TObject* owner, const TNotifyEvent& value);
    static TCloseQueryEvent GetOnCanCloseImpl(TObject* owner);
    static void             SetOnCanCloseImpl(TObject* owner, const TCloseQueryEvent& value);
};

// ファイルを選ぶダイアログの共通の基底(LCL の TFileDialog)。
class TFileDialog : public TCommonDialog
{
public:
    // 選択したファイルのフルパス(Execute の前に設定すると、初期のファイル名になる)。
    Property<std::string> FileName;
    // "テキスト|*.txt|すべて|*.*" のように、表示名とマスクを | で区切って並べる(1 つのマスクに複数のパターンは ; で区切る)。
    Property<std::string> Filter;
    // 選択されているフィルターの位置(1 始まり)。
    Property<int>         FilterIndex;
    Property<std::string> InitialDir;
    // ファイル名に拡張子が無いときに補う拡張子。LCL は先頭に . を補う("txt" を設定すると ".txt" が返る。VCL は補わない)。
    Property<std::string> DefaultExt;
    // 選択したファイルの一覧(TStrings。ofAllowMultiSelect のとき複数)。
    ReadOnlyProperty<TStrings*> Files;

protected:
    explicit TFileDialog(ObjectHandle handle);
    ~TFileDialog() override = default;

private:
    TStrings files_;

    static std::string GetFileNameImpl(TObject* owner);
    static void        SetFileNameImpl(TObject* owner, const std::string& value);
    static std::string GetFilterImpl(TObject* owner);
    static void        SetFilterImpl(TObject* owner, const std::string& value);
    static int         GetFilterIndexImpl(TObject* owner);
    static void        SetFilterIndexImpl(TObject* owner, const int& value);
    static std::string GetInitialDirImpl(TObject* owner);
    static void        SetInitialDirImpl(TObject* owner, const std::string& value);
    static std::string GetDefaultExtImpl(TObject* owner);
    static void        SetDefaultExtImpl(TObject* owner, const std::string& value);
    static TStrings*   GetFilesImpl(TObject* owner);
};

// TOpenDialog の Options(LCL の TOpenOptions)。TShiftState と同じく Set(docs/adr/0061)。
// 既定は ofEnableSizing・ofViewDetail。Windows だけのもの・古い形式のダイアログだけのものがある(LCL の dialogs.pp を参照)。
enum TOpenOption
{
    ofReadOnly,
    ofOverwritePrompt,      // TSaveDialog: 既存のファイルなら上書きを確かめる
    ofHideReadOnly,
    ofNoChangeDir,
    ofShowHelp,
    ofNoValidate,
    ofAllowMultiSelect,     // 複数のファイルを選べる(Files で受け取る)
    ofExtensionDifferent,
    ofPathMustExist,
    ofFileMustExist,
    ofCreatePrompt,
    ofShareAware,
    ofNoReadOnlyReturn,
    ofNoTestFileCreate,
    ofNoNetworkButton,
    ofNoLongNames,
    ofOldStyleDialog,
    ofNoDereferenceLinks,
    ofNoResolveLinks,
    ofEnableIncludeNotify,
    ofEnableSizing,
    ofDontAddToRecent,
    ofForceShowHidden,
    ofViewDetail,
    ofAutoPreview
};
using TOpenOptions = Set<TOpenOption>;

// ファイルを開くダイアログ。
class TOpenDialog : public TFileDialog
{
public:
    Property<TOpenOptions> Options;

    explicit TOpenDialog(TComponent* AOwner);

protected:
    TOpenDialog(ObjectHandle handle, DerivedTag);
    ~TOpenDialog() override = default;

private:
    static TOpenOptions GetOptionsImpl(TObject* owner);
    static void         SetOptionsImpl(TObject* owner, const TOpenOptions& value);
};

// ファイルを保存するダイアログ。
class TSaveDialog : public TOpenDialog
{
public:
    explicit TSaveDialog(TComponent* AOwner);

protected:
    ~TSaveDialog() override = default;
};

// ディレクトリを選ぶダイアログ(VCL には無く、LCL にある)。選んだディレクトリは FileName で受け取る。
class TSelectDirectoryDialog : public TOpenDialog
{
public:
    explicit TSelectDirectoryDialog(TComponent* AOwner);

protected:
    ~TSelectDirectoryDialog() override = default;
};

// TColorDialog の Options(LCL の TColorDialogOptions)。既定は cdFullOpen(VCL は空)。
enum TColorDialogOption
{
    cdFullOpen,         // 色の作成の部分を最初から開く
    cdPreventFullOpen,  // 色の作成のボタンを無効にする
    cdShowHelp,
    cdSolidColor,
    cdAnyColor
};
using TColorDialogOptions = Set<TColorDialogOption>;

// 色を選ぶダイアログ。
class TColorDialog : public TCommonDialog
{
public:
    // 選択した色(Execute の前に設定すると、初期の色になる)。
    Property<TColor>              Color;
    // 作成した色("ColorA=FFFFFF" のような 名前=値 の行。値は $BBGGRR の 16 進。TStrings)。LCL の既定は ColorA〜ColorT の 20 色。
    ReadOnlyProperty<TStrings*>   CustomColors;
    Property<TColorDialogOptions> Options;

    explicit TColorDialog(TComponent* AOwner);

protected:
    ~TColorDialog() override = default;

private:
    TStrings customColors_;

    static TColor              GetColorImpl(TObject* owner);
    static void                SetColorImpl(TObject* owner, const TColor& value);
    static TStrings*           GetCustomColorsImpl(TObject* owner);
    static TColorDialogOptions GetOptionsImpl(TObject* owner);
    static void                SetOptionsImpl(TObject* owner, const TColorDialogOptions& value);
};

// TFontDialog の Options(LCL の TFontDialogOptions)。既定は fdEffects(下線・取り消し線・色を選べる)。
enum TFontDialogOption
{
    fdAnsiOnly,
    fdTrueTypeOnly,
    fdEffects,
    fdFixedPitchOnly,
    fdForceFontExist,
    fdNoFaceSel,
    fdNoOEMFonts,
    fdNoSimulations,
    fdNoSizeSel,
    fdNoStyleSel,
    fdNoVectorFonts,
    fdShowHelp,
    fdWysiwyg,
    fdLimitSize,       // MinFontSize・MaxFontSize で大きさを制限する
    fdScalableOnly,
    fdApplyButton
};
using TFontDialogOptions = Set<TFontDialogOption>;

// フォントを選ぶダイアログ。
class TFontDialog : public TCommonDialog
{
public:
    // 選択したフォント。ダイアログが所有する TFont のビューで、ダイアログと寿命が一致する。
    // 代入は内容のコピー(nullptr なら何もしない)。FontDialog1->Font = Memo1->Font; で初期のフォントにする。
    Property<TFont*>             Font;
    // 選べる大きさの範囲(Options に fdLimitSize があるときだけ使われる)。
    Property<int>                MinFontSize;
    Property<int>                MaxFontSize;
    Property<TFontDialogOptions> Options;

    explicit TFontDialog(TComponent* AOwner);

protected:
    ~TFontDialog() override = default;

private:
    TFont font_;

    static TFont*             GetFontImpl(TObject* owner);
    static void               SetFontImpl(TObject* owner, TFont* const& value);
    static int                GetMinFontSizeImpl(TObject* owner);
    static void               SetMinFontSizeImpl(TObject* owner, const int& value);
    static int                GetMaxFontSizeImpl(TObject* owner);
    static void               SetMaxFontSizeImpl(TObject* owner, const int& value);
    static TFontDialogOptions GetOptionsImpl(TObject* owner);
    static void               SetOptionsImpl(TObject* owner, const TFontDialogOptions& value);
};

// TFindDialog・TReplaceDialog の Options(LCL の TFindOptions)。ダイアログでの選択(検索の方向・大文字と小文字の区別等)も
// ここに入る。既定は frDown。frFindNext・frReplace・frReplaceAll は、押されたボタンを LCL が OnFind・OnReplace の前に設定する。
enum TFindOption
{
    frDown,                 // 下へ検索する
    frFindNext,
    frHideMatchCase,
    frHideWholeWord,
    frHideUpDown,
    frMatchCase,            // 大文字と小文字を区別する
    frDisableMatchCase,
    frDisableUpDown,
    frDisableWholeWord,
    frReplace,
    frReplaceAll,
    frWholeWord,            // 単語単位で探す
    frShowHelp,
    frEntireScope,
    frHideEntireScope,
    frPromptOnReplace,
    frHidePromptOnReplace,
    frButtonsAtBottom
};
using TFindOptions = Set<TFindOption>;

// 検索のダイアログ。VCL と同じくモードレスで、Execute() は表示してすぐ戻り、利用者が「次を検索」を押すたびに OnFind が呼ばれる
// (検索そのものは OnFind で FindText・Options を見て行う)。閉じるのは利用者か CloseDialog()。
class TFindDialog : public TCommonDialog
{
public:
    Property<std::string>  FindText;
    Property<TFindOptions> Options;
    // ダイアログの位置(画面の座標)。
    Property<int>          Left;
    Property<int>          Top;
    Property<TNotifyEvent> OnFind;

    explicit TFindDialog(TComponent* AOwner);

    // 表示中のダイアログを閉じる。
    void CloseDialog();

protected:
    TFindDialog(ObjectHandle handle, DerivedTag);
    ~TFindDialog() override = default;

    // LCL では TFindDialog の protected。TReplaceDialog が公開する。
    Property<std::string>  ReplaceText;
    Property<TNotifyEvent> OnReplace;

private:
    TNotifyEvent onFind_;
    TNotifyEvent onReplace_;
    bool onFindHooked_    = false;
    bool onReplaceHooked_ = false;

    static void BETH_CALL FindTrampoline(ObjectHandle sender, void* data);
    static void BETH_CALL ReplaceTrampoline(ObjectHandle sender, void* data);

    static std::string  GetFindTextImpl(TObject* owner);
    static void         SetFindTextImpl(TObject* owner, const std::string& value);
    static std::string  GetReplaceTextImpl(TObject* owner);
    static void         SetReplaceTextImpl(TObject* owner, const std::string& value);
    static TFindOptions GetOptionsImpl(TObject* owner);
    static void         SetOptionsImpl(TObject* owner, const TFindOptions& value);
    static int          GetLeftImpl(TObject* owner);
    static void         SetLeftImpl(TObject* owner, const int& value);
    static int          GetTopImpl(TObject* owner);
    static void         SetTopImpl(TObject* owner, const int& value);
    static TNotifyEvent GetOnFindImpl(TObject* owner);
    static void         SetOnFindImpl(TObject* owner, const TNotifyEvent& value);
    static TNotifyEvent GetOnReplaceImpl(TObject* owner);
    static void         SetOnReplaceImpl(TObject* owner, const TNotifyEvent& value);
};

// 置換のダイアログ。「置換」「すべて置換」が押されると OnReplace が呼ばれる(どちらかは Options の frReplace・frReplaceAll で分かる)。
class TReplaceDialog : public TFindDialog
{
public:
    using TFindDialog::ReplaceText;
    using TFindDialog::OnReplace;

    explicit TReplaceDialog(TComponent* AOwner);

protected:
    ~TReplaceDialog() override = default;
};

} // namespace beth

#endif // BETH_HPP
