#include "stdafx.h"
#if defined(XR_PLATFORM_WINDOWS)
#pragma hdrstop

#include <intrin.h> // __rdtsc
#include <process.h>

#if defined(XR_COMPILER_MSVC)
#include <powerbase.h>
#elif defined(XR_COMPILER_GCC)
#include <float.h> // _controlfp
//#include_next <float.h>
//how to include mingw32\i686-w64-mingw32\include\float.h
//instead of mingw32\lib\gcc\i686-w64-mingw32\7.3.0\include\float.h
//?
#endif

#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_E2K)
#include <x86intrin.h> // __rdtsc
#elif defined(XR_ARCHITECTURE_ARM)
#include <sys/syscall.h>
#include <linux/perf_event.h>
#endif // defined(XR_ARCHITECTURE_ARM)

#ifdef XR_PLATFORM_LINUX
#include <fpu_control.h>
#elif defined(XR_PLATFORM_FREEBSD)
#include <sys/sysctl.h>
#include <fenv.h>
typedef unsigned int fpu_control_t __attribute__((__mode__(__HI__)));
#define _FPU_GETCW(x) asm volatile ("fnstcw %0" : "=m" ((*&x)))
#define _FPU_SETCW(x) asm volatile ("fldcw %0" : : "m" ((*&x)))
#define _FPU_EXTENDED FP_PRC_FLD
#define _FPU_DOUBLE 0x200
#define _FPU_SINGLE 0x0
#define _FPU_RC_NEAREST FP_PS
#define _FPU_DEFAULT FP_PD
#endif
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <chrono>
#include <fstream>
#include <string>
#include <stdint.h>
#include <string.h>
#include <pcre.h>
#include <iostream>
#endif
#include <thread>
#include "SDL.h"

#if (defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64) || defined(XR_ARCHITECTURE_E2K)) && !defined(XR_COMPILER_MSVC)
#define _FPU_EXTENDED 0
#define _FPU_DOUBLE 0
#define _FPU_SINGLE 0
#define _FPU_RC_NEAREST 0

#if defined(XR_ARCHITECTURE_ARM)
static class PerfInit
{
public:
    int fddev = -1;

public:
    PerfInit()
    {
        static struct perf_event_attr attr;
        attr.type = PERF_TYPE_HARDWARE;
        attr.config = PERF_COUNT_HW_CPU_CYCLES;
        fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
    }
    ~PerfInit()
    {
        close(fddev);
    }
} s_perf_init;
#endif // defined(XR_ARCHITECTURE_ARM)
#endif // defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64) || defined(XR_ARCHITECTURE_E2K)

typedef struct _PROCESSOR_POWER_INFORMATION
{
    u32 Number;
    u32 MaxMhz;
    u32 CurrentMhz;
    u32 MhzLimit;
    u32 MaxIdleState;
    u32 CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

// Initialized on startup
XRCORE_API Fmatrix Fidentity;
XRCORE_API Dmatrix Didentity;
XRCORE_API CRandom Random;

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
u32 timeGetTime()
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
#if defined(XR_PLATFORM_WINDOWS)
#ifndef XR_ARCHITECTURE_X64
    _controlfp(_PC_24, MCW_PC);
#endif
    _controlfp(_RC_CHOP, MCW_RC);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_EXTENDED & ~_FPU_DOUBLE) | _FPU_SINGLE;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m24r()
{
#if defined(XR_PLATFORM_WINDOWS)
#ifndef XR_ARCHITECTURE_X64
    _controlfp(_PC_24, MCW_PC);
#endif
    _controlfp(_RC_NEAR, MCW_RC);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_EXTENDED & ~_FPU_DOUBLE) | _FPU_SINGLE | _FPU_RC_NEAREST;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m53()
{
#if defined(XR_PLATFORM_WINDOWS)
#ifndef XR_ARCHITECTURE_X64
    _controlfp(_PC_53, MCW_PC);
#endif
    _controlfp(_RC_CHOP, MCW_RC);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_EXTENDED & ~_FPU_SINGLE) | _FPU_DOUBLE;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m53r()
{
#if defined(XR_PLATFORM_WINDOWS)
#ifndef XR_ARCHITECTURE_X64
    _controlfp(_PC_53, MCW_PC);
#endif
    _controlfp(_RC_NEAR, MCW_RC);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_EXTENDED & ~_FPU_SINGLE) | _FPU_DOUBLE | _FPU_RC_NEAREST;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m64()
{
#if defined(XR_PLATFORM_WINDOWS)
#ifndef XR_ARCHITECTURE_X64
    _controlfp(_PC_64, MCW_PC);
#endif
    _controlfp(_RC_CHOP, MCW_RC);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_DOUBLE & ~_FPU_SINGLE) | _FPU_EXTENDED;
    _FPU_SETCW(fpu_cw);
#endif
}

XRCORE_API void m64r()
{
#if defined(XR_PLATFORM_WINDOWS)
#ifndef XR_ARCHITECTURE_X64
    _controlfp(_PC_64, MCW_PC);
#endif
    _controlfp(_RC_NEAR, MCW_RC);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    fpu_control_t fpu_cw;
    _FPU_GETCW(fpu_cw);
    fpu_cw = (fpu_cw & ~_FPU_DOUBLE & ~_FPU_SINGLE) | _FPU_EXTENDED | _FPU_RC_NEAREST;
    _FPU_SETCW(fpu_cw);
#endif
}

void initialize()
{
#if defined(XR_PLATFORM_WINDOWS)
    _clearfp();
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
    fpu_control_t fpu_cw;
    fpu_cw = _FPU_DEFAULT;
    _FPU_SETCW(fpu_cw);
#endif

    // По-умолчанию для плагинов экспорта из 3D-редакторов включена высокая точность вычислений с плавающей точкой
    if (Core.PluginMode)
        m64r();
    else
        m24r();

#if defined(XR_PLATFORM_WINDOWS)
    ::Random.seed(u32(CPU::GetCLK() % (1i64 << 32i64)));
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_FREEBSD)
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
#if defined(XR_COMPILER_MSVC)

#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64)
    return __rdtsc();
#elif defined(XR_ARCHITECTURE_ARM)
    return __rdpmccntr64();
#elif defined(XR_ARCHITECTURE_ARM64)
    return _ReadStatusReg(ARM64_PMCCNTR_EL0);
#else
#error Unsupported architecture
#endif

#elif defined(XR_COMPILER_GCC)

#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_E2K)
    return __rdtsc();
#elif defined(XR_ARCHITECTURE_ARM)
    long long result = 0;
    if (read(s_perf_init.fddev, &result, sizeof(result)) < sizeof(result))
        return 0;
    return result;
#elif defined(XR_ARCHITECTURE_ARM64)
    int64_t virtual_timer_value;
    asm volatile("mrs %0, pmccntr_el0" : "=r"(virtual_timer_value));
    return virtual_timer_value;
#endif

#else
#error Unsupported compiler
#endif
    return 0;
}

XRCORE_API u32 GetCurrentCPU()
{
#if defined(XR_PLATFORM_WINDOWS)
    return GetCurrentProcessorNumber();
#elif defined(XR_PLATFORM_LINUX)
    return static_cast<u32>(sched_getcpu());
#else
    return 0;
#endif
}
} // namespace CPU

bool g_initialize_cpu_called = false;

#if defined(XR_PLATFORM_LINUX)
u32 cpufreq()
{
    u32 cpuFreq = 0;
#if defined(XR_ARCHITECTURE_ARM64) || defined(XR_ARCHITECTURE_ARM)
    xr_string parcedFreq;
    std::ifstream cpuMaxFreq("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (cpuMaxFreq.is_open())
    {
        getline(cpuMaxFreq, parcedFreq);
        cpuFreq = atol(parcedFreq.c_str()) / 1000;
    }
#else
    // CPU frequency is stored in /proc/cpuinfo in lines beginning with "cpu MHz"
    pcstr pattern = "^cpu MHz\\s*:\\s*(\\d+)";
    pcstr pcreErrorStr = nullptr;
    int pcreErrorOffset = 0;

    pcre* reCompiled = pcre_compile(pattern, PCRE_ANCHORED, &pcreErrorStr, &pcreErrorOffset, nullptr);
    if(reCompiled == nullptr)
    {
        return 0;
    }

    std::ifstream ifs("/proc/cpuinfo");
    if(ifs.is_open())
    {
        xr_string line;
        int results[10];
        while(ifs.good())
        {
            std::getline(ifs, line);
            int rc = pcre_exec(reCompiled, 0, line.c_str(), line.length(), 0, 0, results, sizeof(results)/sizeof(results[0]));
            if(rc < 0)
                continue;
            // Match found - extract frequency
            pcstr matchStr = nullptr;
            pcre_get_substring(line.c_str(), results, rc, 1, &matchStr);
            R_ASSERT(matchStr);
            cpuFreq = atol(matchStr);
            pcre_free_substring(matchStr);
            break;
        }
        ifs.close();
    }

    pcre_free(reCompiled);
#endif
    return cpuFreq;
}
#elif defined(XR_PLATFORM_FREEBSD)
u32 cpufreq()
{
    u32 cpuFreq = 0;
    size_t cpuFreqSz = sizeof(cpuFreq);

    sysctlbyname("dev.cpu.0.freq", &cpuFreq, &cpuFreqSz, nullptr, 0);
    return cpuFreq;
}
#endif // #ifdef XR_PLATFORM_LINUX

//------------------------------------------------------------------------------------
void _initialize_cpu()
{
    // General CPU identification
    if (!query_processor_info(&CPU::ID))
        Log("! Can't detect CPU/FPU.");

    Msg("* Detected CPU: %s [%s], F%d/M%d/S%d, 'rdtsc'", CPU::ID.modelName,
        +CPU::ID.vendor, CPU::ID.family, CPU::ID.model, CPU::ID.stepping);

    string256 features;
    xr_strcpy(features, sizeof(features), "RDTSC");

    if (CPU::ID.hasFeature(CpuFeature::InvariantTSC))
        xr_strcat(features, ", Invariant TSC");
    if (CPU::ID.hasFeature(CpuFeature::MMX))
        xr_strcat(features, ", MMX");
    if (CPU::ID.hasFeature(CpuFeature::AltiVec))
        xr_strcat(features, ", AltiVec");
    if (CPU::ID.hasFeature(CpuFeature::_3DNow))
        xr_strcat(features, ", 3DNow!");
    if (CPU::ID.hasFeature(CpuFeature::SSE))
        xr_strcat(features, ", SSE");
    if (CPU::ID.hasFeature(CpuFeature::SSE2))
        xr_strcat(features, ", SSE2");
    if (CPU::ID.hasFeature(CpuFeature::SSE3))
        xr_strcat(features, ", SSE3");
    if (CPU::ID.hasFeature(CpuFeature::MWait))
        xr_strcat(features, ", MONITOR/MWAIT");
    if (CPU::ID.hasFeature(CpuFeature::SSSE3))
        xr_strcat(features, ", SSSE3");
    if (CPU::ID.hasFeature(CpuFeature::SSE41))
        xr_strcat(features, ", SSE4.1");
    if (CPU::ID.hasFeature(CpuFeature::SSE42))
        xr_strcat(features, ", SSE4.2");
    if (CPU::ID.hasFeature(CpuFeature::HyperThreading))
        xr_strcat(features, ", HTT");
    if (CPU::ID.hasFeature(CpuFeature::AVX))
        xr_strcat(features, ", AVX");
    if (CPU::ID.hasFeature(CpuFeature::AVX2))
        xr_strcat(features, ", AVX2");

    Msg("* CPU features: %s", features);
    Msg("* CPU cores/threads: %d/%d", CPU::ID.n_cores, CPU::ID.n_threads);

#if defined(XR_PLATFORM_WINDOWS)
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
    Msg("* CPU current freq: %u MHz", cpufreq());
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
#if defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64)
#define _MM_SET_FLUSH_ZERO_MODE(mode)
#define _MM_SET_DENORMALS_ZERO_MODE(mode)
#else
#include <xmmintrin.h>
#endif

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

    if (CPU::ID.hasFeature(CpuFeature::SSE))
    {
        //_mm_setcsr ( _mm_getcsr() | (_MM_FLUSH_ZERO_ON+_MM_DENORMALS_ZERO_ON) );
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        if (_denormals_are_zero_supported)
        {
#if defined(XR_PLATFORM_WINDOWS)
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
