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

#include "no_vcl_c.h"

#include "no_vcl_funcs.h"

namespace
{

/* 関数ポインタ型と thread_local 変数 */
#define NO_VCL_DECLARE(ret, name, params, args) \
    typedef ret (NO_VCL_CALL *name##_fn) params; \
    thread_local name##_fn name##_ = nullptr;
NO_VCL_FUNCS(NO_VCL_DECLARE)
#undef NO_VCL_DECLARE

/* 直前のエラー(docs/adr/0031)。DLL の公開関数の中で例外が起きると、DLL が OnDllError で知らせる。
   各関数の呼び出しの最初にクリアするため、「直前の呼び出しが失敗したか」を表す。 */
struct LastError
{
    bool        set = false;
    std::string className;
    std::string message;
};
thread_local LastError g_lastError;

typedef void (NO_VCL_CALL *no_vcl_error_callback_t)(no_vcl_str_t className, no_vcl_str_t message);

void NO_VCL_CALL OnDllError(no_vcl_str_t className, no_vcl_str_t message)
{
    g_lastError.set = true;
    g_lastError.className = className ? className : "";
    g_lastError.message = message ? message : "";
}

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
            // エラーの通知先は DLL 全体で 1 つ(通知は失敗した呼び出しと同じスレッドで行われる)。
            void (NO_VCL_CALL *setErrorCallback)(no_vcl_error_callback_t) = nullptr;
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

extern "C" {

#define NO_VCL_DEFINE(ret, name, params, args) \
    ret NO_VCL_CALL no_vcl_##name params \
    { \
        NO_VCL_INIT_CHECK(name##_); \
        g_lastError.set = false; \
        return name##_ args; \
    }
NO_VCL_FUNCS(NO_VCL_DEFINE)
#undef NO_VCL_DEFINE

no_vcl_bool_t NO_VCL_CALL no_vcl_HasLastError(void)          { return g_lastError.set ? 1 : 0; }
no_vcl_str_t  NO_VCL_CALL no_vcl_GetLastErrorClassName(void) { return g_lastError.set ? g_lastError.className.c_str() : ""; }
no_vcl_str_t  NO_VCL_CALL no_vcl_GetLastErrorMessage(void)   { return g_lastError.set ? g_lastError.message.c_str() : ""; }
void          NO_VCL_CALL no_vcl_ClearLastError(void)        { g_lastError.set = false; }

} // extern "C"
