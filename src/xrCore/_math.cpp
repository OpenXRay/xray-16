#include "stdafx.h"
#if defined(WINDOWS)
#pragma hdrstop

#include <intrin.h> // __rdtsc
#include <process.h>

#if defined(_MSC_VER)
#include <powerbase.h>
#elif defined(__GNUC__)
#include <float.h> // _controlfp
//#include_next <float.h>
//how to include mingw32\i686-w64-mingw32\include\float.h
//instead of mingw32\lib\gcc\i686-w64-mingw32\7.3.0\include\float.h
//?
#endif

#elif defined(LINUX)
#include <x86intrin.h> // __rdtsc
#include <fpu_control.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <chrono>
#endif
#include <thread>
#include "SDL.h"

typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

// Initialized on startup
XRCORE_API Fmatrix Fidentity;
XRCORE_API Dmatrix Didentity;
XRCORE_API CRandom Random;

#if defined(LINUX)
DWORD timeGetTime()
{
    return SDL_GetTicks();
}
#endif

/*
Функции управления точностью вычислений с плавающей точкой.
Более подробную информацию можно получить здесь:
https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/control87-controlfp-control87-2
Число 24, 53 и 64 - определяют ограничение точности в битах.
Наличие 'r' - включает округление результатов.
Реально в движке используются только m24r и m64r. И один раз m64 - возможно ошибка?
*/
namespace FPU
{
XRCORE_API void m24()
{
#if defined(WINDOWS)
#ifndef XR_X64
    _controlfp(_PC_24, MCW_PC);
#endif
    _controlfp(_RC_CHOP, MCW_RC);
#elif defined(LINUX)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_EXTENDED & ~_FPU_DOUBLE) | _FPU_SINGLE;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m24r()
{
#if defined(WINDOWS)
#ifndef XR_X64
    _controlfp(_PC_24, MCW_PC);
#endif
    _controlfp(_RC_NEAR, MCW_RC);
#elif defined(LINUX)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_EXTENDED & ~_FPU_DOUBLE) | _FPU_SINGLE | _FPU_RC_NEAREST;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m53()
{
#if defined(WINDOWS)
#ifndef XR_X64
    _controlfp(_PC_53, MCW_PC);
#endif
    _controlfp(_RC_CHOP, MCW_RC);
#elif defined(LINUX)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_EXTENDED & ~_FPU_SINGLE) | _FPU_DOUBLE;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m53r()
{
#if defined(WINDOWS)
#ifndef XR_X64
    _controlfp(_PC_53, MCW_PC);
#endif
    _controlfp(_RC_NEAR, MCW_RC);
#elif defined(LINUX)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_EXTENDED & ~_FPU_SINGLE) | _FPU_DOUBLE | _FPU_RC_NEAREST;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m64()
{
#if defined(WINDOWS)
#ifndef XR_X64
    _controlfp(_PC_64, MCW_PC);
#endif
    _controlfp(_RC_CHOP, MCW_RC);
#elif defined(LINUX)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_DOUBLE & ~_FPU_SINGLE) | _FPU_EXTENDED;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m64r()
{
#if defined(WINDOWS)
#ifndef XR_X64
    _controlfp(_PC_64, MCW_PC);
#endif
    _controlfp(_RC_NEAR, MCW_RC);
#elif defined(LINUX)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_DOUBLE & ~_FPU_SINGLE) | _FPU_EXTENDED | _FPU_RC_NEAREST;
    _FPU_SETCW(fpu_cw);
#endif
}

void initialize()
{
#if defined(WINDOWS)
    _clearfp();
#elif defined(LINUX)
    fpu_control_t fpu_cw;
    fpu_cw = _FPU_DEFAULT;
    _FPU_SETCW(fpu_cw);
#endif

    // По-умолчанию для плагинов экспорта из 3D-редакторов включена высокая точность вычислений с плавающей точкой
    if (Core.PluginMode)
        m64r();
    else
        m24r();

#if defined(WINDOWS)
    ::Random.seed(u32(CPU::GetCLK() % (1i64 << 32i64)));
#elif defined(LINUX)
    ::Random.seed(u32(CPU::GetCLK() % ((u64)0x1 << 32)));
#endif
}
};

namespace CPU
{
XRCORE_API u64 qpc_freq = SDL_GetPerformanceFrequency();

XRCORE_API u32 qpc_counter = 0;

XRCORE_API processor_info ID;

XRCORE_API u64 QPC() noexcept
{
    u64 _dest = SDL_GetPerformanceCounter();
    qpc_counter++;
    return _dest;
}

XRCORE_API u64 GetCLK()
{
    return __rdtsc();
}
} // namespace CPU

bool g_initialize_cpu_called = false;

//------------------------------------------------------------------------------------
void _initialize_cpu()
{

    string256 features;
    xr_strcpy(features, sizeof(features), "RDTSC");
    if (SDL_HasAltiVec()) xr_strcat(features, ", AltiVec");
    if (SDL_HasMMX()) xr_strcat(features, ", MMX");
    if (SDL_Has3DNow()) xr_strcat(features, ", 3DNow!");
    if (SDL_HasSSE()) xr_strcat(features, ", SSE");
    if (SDL_HasSSE2()) xr_strcat(features, ", SSE2");
    if (SDL_HasSSE3()) xr_strcat(features, ", SSE3");
    if (SDL_HasSSE41()) xr_strcat(features, ", SSE4.1");
    if (SDL_HasSSE42()) xr_strcat(features, ", SSE4.2");
    if (SDL_HasAVX()) xr_strcat(features, ", AVX");
    if (SDL_HasAVX2()) xr_strcat(features, ", AVX2");

    Msg("* CPU features: %s", features);
    Msg("* CPU cores/threads: %d/%d", std::thread::hardware_concurrency(), SDL_GetCPUCount());

#if defined(WINDOWS)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    const size_t cpusCount = sysInfo.dwNumberOfProcessors;

    xr_vector<PROCESSOR_POWER_INFORMATION> cpusInfo(cpusCount);
    CallNtPowerInformation(ProcessorInformation, nullptr, 0, cpusInfo.data(),
                           sizeof(PROCESSOR_POWER_INFORMATION) * cpusCount);

    for (size_t i = 0; i < cpusInfo.size(); i++)
    {
        const PROCESSOR_POWER_INFORMATION& cpuInfo = cpusInfo[i];
        Msg("* CPU%zu current freq: %lu MHz, max freq: %lu MHz",
            i, cpuInfo.CurrentMhz, cpuInfo.MaxMhz);
    }
#else
    Msg("* CPU current freq: %lu MHz", CPU::qpc_freq);
#endif
    Log("");
    Fidentity.identity(); // Identity matrix
    Didentity.identity(); // Identity matrix
    pvInitializeStatics(); // Lookup table for compressed normals
    FPU::initialize();
    _initialize_cpu_thread();

    g_initialize_cpu_called = true;
}

// per-thread initialization
#include <xmmintrin.h>
#define _MM_DENORMALS_ZERO_MASK 0x0040
#define _MM_DENORMALS_ZERO_ON 0x0040
#define _MM_FLUSH_ZERO_MASK 0x8000
#define _MM_FLUSH_ZERO_ON 0x8000
#define _MM_SET_FLUSH_ZERO_MODE(mode) _mm_setcsr((_mm_getcsr() & ~_MM_FLUSH_ZERO_MASK) | (mode))
#define _MM_SET_DENORMALS_ZERO_MODE(mode) _mm_setcsr((_mm_getcsr() & ~_MM_DENORMALS_ZERO_MASK) | (mode))
static BOOL _denormals_are_zero_supported = TRUE;
extern void __cdecl _terminate();

void _initialize_cpu_thread()
{
    xrDebug::OnThreadSpawn();

    // По-умолчанию для плагинов экспорта из 3D-редакторов включена высокая точность вычислений с плавающей точкой
    if (Core.PluginMode)
        FPU::m64r();
    else
        FPU::m24r();

    if (SDL_HasSSE())
    {
        //_mm_setcsr ( _mm_getcsr() | (_MM_FLUSH_ZERO_ON+_MM_DENORMALS_ZERO_ON) );
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        if (_denormals_are_zero_supported)
        {
#if defined(WINDOWS)
            __try
            {
                _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                _denormals_are_zero_supported = FALSE;
            }
#else
            try
            {
                _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
            }
            catch (...)
            {
                _denormals_are_zero_supported = FALSE;
            }
#endif
        }

    }
}

// threading API
#pragma pack(push, 8)
struct THREAD_NAME
{
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
};

void thread_name(const char* name)
{
    Msg("start new thread [%s]", name);
#if defined(WINDOWS)
    THREAD_NAME tn;
    tn.dwType = 0x1000;
    tn.szName = name;
    tn.dwThreadID = DWORD(-1);
    tn.dwFlags = 0;

    __try
    {
        RaiseException(0x406D1388, 0, sizeof(tn) / sizeof(DWORD), (ULONG_PTR*)&tn);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
#else
    prctl(PR_SET_NAME, name, 0, 0, 0);
#endif
}
#pragma pack(pop)

struct THREAD_STARTUP
{
    thread_t* entry;
    char* name;
    void* args;
};
#if defined(WINDOWS)
void __cdecl thread_entry(void* _params)
#elif defined(LINUX)
void *__cdecl thread_entry(void* _params)
#endif
{
    // initialize
    THREAD_STARTUP* startup = (THREAD_STARTUP*)_params;
    thread_name(startup->name);
    thread_t* entry = startup->entry;
    void* arglist = startup->args;
    xr_delete(startup);
    _initialize_cpu_thread();

    // call
    entry(arglist);
#ifdef LINUX
    return nullptr;
#endif
}

void thread_spawn(thread_t* entry, const char* name, unsigned stack, void* arglist)
{
    xrDebug::Initialize();

    THREAD_STARTUP* startup = new THREAD_STARTUP();
    startup->entry = entry;
    startup->name = (char*)name;
    startup->args = arglist;
#if defined(WINDOWS)
    _beginthread(thread_entry, stack, startup);
#elif defined(LINUX)
    pthread_t handle = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, stack);
    pthread_create(&handle, &attr, &thread_entry, startup);
    pthread_attr_destroy(&attr);
#endif
}

void spline1(float t, Fvector* p, Fvector* ret)
{
    float t2 = t * t;
    float t3 = t2 * t;
    float m[4];

    ret->x = 0.0f;
    ret->y = 0.0f;
    ret->z = 0.0f;
    m[0] = (0.5f * ((-1.0f * t3) + (2.0f * t2) + (-1.0f * t)));
    m[1] = (0.5f * ((3.0f * t3) + (-5.0f * t2) + (0.0f * t) + 2.0f));
    m[2] = (0.5f * ((-3.0f * t3) + (4.0f * t2) + (1.0f * t)));
    m[3] = (0.5f * ((1.0f * t3) + (-1.0f * t2) + (0.0f * t)));

    for (int i = 0; i < 4; i++)
    {
        ret->x += p[i].x * m[i];
        ret->y += p[i].y * m[i];
        ret->z += p[i].z * m[i];
    }
}

void spline2(float t, Fvector* p, Fvector* ret)
{
    float s = 1.0f - t;
    float t2 = t * t;
    float t3 = t2 * t;
    float m[4];

    m[0] = s * s * s;
    m[1] = 3.0f * t3 - 6.0f * t2 + 4.0f;
    m[2] = -3.0f * t3 + 3.0f * t2 + 3.0f * t + 1;
    m[3] = t3;

    ret->x = (p[0].x * m[0] + p[1].x * m[1] + p[2].x * m[2] + p[3].x * m[3]) / 6.0f;
    ret->y = (p[0].y * m[0] + p[1].y * m[1] + p[2].y * m[2] + p[3].y * m[3]) / 6.0f;
    ret->z = (p[0].z * m[0] + p[1].z * m[1] + p[2].z * m[2] + p[3].z * m[3]) / 6.0f;
}

#define beta1 1.0f
#define beta2 0.8f

void spline3(float t, Fvector* p, Fvector* ret)
{
    float s = 1.0f - t;
    float t2 = t * t;
    float t3 = t2 * t;
    float b12 = beta1 * beta2;
    float b13 = b12 * beta1;
    float delta = 2.0f - b13 + 4.0f * b12 + 4.0f * beta1 + beta2 + 2.0f;
    float d = 1.0f / delta;
    float b0 = 2.0f * b13 * d * s * s * s;
    float b3 = 2.0f * t3 * d;
    float b1 = d * (2 * b13 * t * (t2 - 3 * t + 3) + 2 * b12 * (t3 - 3 * t2 + 2) + 2 * beta1 * (t3 - 3 * t + 2) +
                       beta2 * (2 * t3 - 3 * t2 + 1));
    float b2 = d * (2 * b12 * t2 * (-t + 3) + 2 * beta1 * t * (-t2 + 3) + beta2 * t2 * (-2 * t + 3) + 2 * (-t3 + 1));

    ret->x = p[0].x * b0 + p[1].x * b1 + p[2].x * b2 + p[3].x * b3;
    ret->y = p[0].y * b0 + p[1].y * b1 + p[2].y * b2 + p[3].y * b3;
    ret->z = p[0].z * b0 + p[1].z * b1 + p[2].z * b2 + p[3].z * b3;
}
