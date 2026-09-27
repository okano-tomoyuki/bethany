#include "internal/api.h"

#include <cassert>
#include <mutex>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
using no_vcl_module_t = HMODULE;
#else
#include <dlfcn.h>
using no_vcl_module_t = void*;
#endif

#include "no_vcl.hpp"

namespace no_vcl
{
namespace internal
{

namespace
{

/* 関数ポインタ型と thread_local 変数 */
#define NO_VCL_DECLARE(ret, name, params, args) \
    typedef ret (NO_VCL_CALL *name##_fn) params; \
    thread_local name##_fn name##_ = nullptr;
NO_VCL_FUNCS(NO_VCL_DECLARE)
#undef NO_VCL_DECLARE

/* 直前のエラー(docs/adr/0031)。DLL の公開関数の中で例外が起きると、DLL が OnDllError で知らせる
   (通知は失敗した呼び出しと同じスレッドで、その呼び出しから戻る前に行われる)。 */
struct LastError
{
    bool        set = false;
    std::string className;
    std::string message;
};
thread_local LastError g_lastError;

typedef void (NO_VCL_CALL *error_callback_t)(str_t className, str_t message);

void NO_VCL_CALL OnDllError(str_t className, str_t message)
{
    g_lastError.set = true;
    g_lastError.className = className ? className : "";
    g_lastError.message = message ? message : "";
}

// 呼び出しの後に、DLL の中で例外が起きていれば Exception として送出する。
void ThrowIfLastError()
{
    if (!g_lastError.set)
        return;
    g_lastError.set = false;
    throw Exception(g_lastError.className, g_lastError.message);
}

template<typename R>
struct Checked
{
    template<typename F>
    static R Call(F f)
    {
        R result = f();
        ThrowIfLastError();
        return result;
    }
};

template<>
struct Checked<void>
{
    template<typename F>
    static void Call(F f)
    {
        f();
        ThrowIfLastError();
    }
};

/* 関数ポインタマッピング */
template<typename Func>
void no_vcl_map(Func& f, no_vcl_module_t m, const char* n)
{
#if defined(_WIN32) || defined(_WIN64)
    void* p = reinterpret_cast<void*>(::GetProcAddress(m, n));
#else
    void* p = ::dlsym(m, n);
#endif
    f = reinterpret_cast<Func>(p);
}

void no_vcl_init(void)
{
    static no_vcl_module_t m = nullptr;
    static std::once_flag once;

    if (!m)
    {
        std::call_once(once, [&](){
#if defined(_WIN32) || defined(_WIN64)
            m = ::LoadLibraryA("no_vcl.dll");
#else
            m = ::dlopen("libno_vcl.so", RTLD_LAZY);
#endif
            // エラーの通知先は DLL 全体で 1 つ。
            void (NO_VCL_CALL *setErrorCallback)(error_callback_t) = nullptr;
            no_vcl_map(setErrorCallback, m, "Error_SetCallback");
            assert(setErrorCallback != nullptr);
            setErrorCallback(&OnDllError);
        });
    }

#define NO_VCL_MAP(ret, name, params, args) no_vcl_map(name##_, m, #name);
    NO_VCL_FUNCS(NO_VCL_MAP)
#undef NO_VCL_MAP
}

#define NO_VCL_INIT_CHECK(f) \
    do { \
        if (!f) \
        { \
            no_vcl_init(); \
        } \
        assert(f != nullptr); \
    } while (0)

} // namespace

// 中継関数。呼び出しの最初に直前のエラーをクリアし、戻った後に確かめる。
#define NO_VCL_DEFINE(ret, name, params, args) \
    ret name params \
    { \
        NO_VCL_INIT_CHECK(name##_); \
        g_lastError.set = false; \
        return Checked<ret>::Call([&]() { return name##_ args; }); \
    }
NO_VCL_FUNCS(NO_VCL_DEFINE)
#undef NO_VCL_DEFINE

} // namespace internal
} // namespace no_vcl
