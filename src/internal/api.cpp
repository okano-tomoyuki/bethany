#include <bethany/internal/api.h>

#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
using beth_module_t = HMODULE;
#else
#include <dlfcn.h>
using beth_module_t = void*;
#endif

#include <bethany/beth.hpp>

namespace beth
{
namespace internal
{

namespace
{

/* 関数ポインタ型と thread_local 変数 */
#define BETH_DECLARE(ret, name, params, args) \
    typedef ret (BETH_CALL *name##_fn) params; \
    thread_local name##_fn name##_ = nullptr;
BETH_FUNCS(BETH_DECLARE)
#undef BETH_DECLARE

/* 直前のエラー(docs/adr/0031)。DLL の公開関数の中で例外が起きると、DLL が OnDllError で知らせる
   (通知は失敗した呼び出しと同じスレッドで、その呼び出しから戻る前に行われる)。 */
struct LastError
{
    bool        set = false;
    std::string className;
    std::string message;
};
thread_local LastError g_lastError;

typedef void (BETH_CALL *error_callback_t)(str_t className, str_t message);

void BETH_CALL OnDllError(str_t className, str_t message)
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

/*
 * DLL が見つからない・DLL に関数が無い(ヘッダより古い DLL を読み込んだ)ときに、理由を知らせて終了する。
 * assert にすると Release のビルドでは消えて、ヌルポインタの呼び出しで落ちるため。GUI のアプリでは標準エラー出力が見えないので、
 * Windows ではメッセージボックスも出す。
 */
[[noreturn]] void beth_fatal(const std::string& message)
{
    std::fprintf(stderr, "Bethany: %s\n", message.c_str());
    std::fflush(stderr);
#if defined(_WIN32) || defined(_WIN64)
    ::MessageBoxA(nullptr, message.c_str(), "Bethany", MB_OK | MB_ICONERROR);
#endif
    std::abort();
}

/* 関数ポインタマッピング */
template<typename Func>
void beth_map(Func& f, beth_module_t m, const char* n)
{
#if defined(_WIN32) || defined(_WIN64)
    void* p = reinterpret_cast<void*>(::GetProcAddress(m, n));
#else
    void* p = ::dlsym(m, n);
#endif
    f = reinterpret_cast<Func>(p);
}

void beth_init(void)
{
    static beth_module_t m = nullptr;
    static std::once_flag once;

    if (!m)
    {
        std::call_once(once, [&](){
#if defined(_WIN32) || defined(_WIN64)
            m = ::LoadLibraryA("beth.dll");
#else
            m = ::dlopen("libbeth.so", RTLD_LAZY);
#endif
#if defined(_WIN32) || defined(_WIN64)
            if (!m)
                beth_fatal("beth.dll was not found. Put beth.dll next to the executable (beth_deploy() in CMake).");
#else
            if (!m)
                beth_fatal("libbeth.so was not found.");
#endif
            // エラーの通知先は DLL 全体で 1 つ。
            void (BETH_CALL *setErrorCallback)(error_callback_t) = nullptr;
            beth_map(setErrorCallback, m, "Error_SetCallback");
            if (!setErrorCallback)
                beth_fatal("The Bethany shared library does not export Error_SetCallback.");
            setErrorCallback(&OnDllError);
        });
    }

#define BETH_MAP(ret, name, params, args) beth_map(name##_, m, #name);
    BETH_FUNCS(BETH_MAP)
#undef BETH_MAP
}

#define BETH_INIT_CHECK(name) \
    do { \
        if (!name##_) \
        { \
            beth_init(); \
            if (!name##_) \
                beth_fatal(std::string("The Bethany shared library has no function ") + #name + \
                           ". It is probably older than the headers: use the beth.dll of the same version."); \
        } \
    } while (0)

} // namespace

// 中継関数。呼び出しの最初に直前のエラーをクリアし、戻った後に確かめる。
#define BETH_DEFINE(ret, name, params, args) \
    ret name params \
    { \
        BETH_INIT_CHECK(name); \
        g_lastError.set = false; \
        return Checked<ret>::Call([&]() { return name##_ args; }); \
    }
BETH_FUNCS(BETH_DEFINE)
#undef BETH_DEFINE

} // namespace internal
} // namespace beth
