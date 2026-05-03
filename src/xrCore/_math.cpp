#include "stdafx.h"

#include <thread>
#include <SDL3/SDL.h>

#if (defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_X86)) && (defined(_MSC_VER) || defined(__clang__))
#   include <intrin.h>
#endif

XRCORE_API Fmatrix Fidentity;
XRCORE_API CRandom Random;

namespace
{
    struct CpuFeatures
    {
        bool sse{}, sse2{}, sse3{}, ssse3{}, sse4_1{}, sse4_2{}, avx{}, avx2{}, avx512f{};
        CpuFeatures()
        {
#if defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_X86)
            int leaf1[4]{}, leaf7[4]{};
#   if defined(_MSC_VER) || defined(__clang__)
            __cpuidex(leaf1, 1, 0);
            __cpuidex(leaf7, 7, 0);
#   else
            __asm__ __volatile__("cpuid" : "=a"(leaf1[0]), "=b"(leaf1[1]), "=c"(leaf1[2]), "=d"(leaf1[3]) : "a"(1), "c"(0));
            __asm__ __volatile__("cpuid" : "=a"(leaf7[0]), "=b"(leaf7[1]), "=c"(leaf7[2]), "=d"(leaf7[3]) : "a"(7), "c"(0));
#   endif
            sse     = (leaf1[3] >> 25) & 1;
            sse2    = (leaf1[3] >> 26) & 1;
            sse3    = (leaf1[2] >>  0) & 1;
            ssse3   = (leaf1[2] >>  9) & 1;
            sse4_1  = (leaf1[2] >> 19) & 1;
            sse4_2  = (leaf1[2] >> 20) & 1;
            avx     = (leaf1[2] >> 28) & 1;
            avx2    = (leaf7[1] >>  5) & 1;
            avx512f = (leaf7[1] >> 16) & 1;
#endif
        }
    };
    const CpuFeatures& cpu()
    {
        static const CpuFeatures features;
        return features;
    }
}

namespace CPU
{
XRCORE_API bool HasSSE     = cpu().sse;
XRCORE_API bool HasSSE2    = cpu().sse2;
XRCORE_API bool HasSSE42   = cpu().sse4_2;
XRCORE_API bool HasAVX     = cpu().avx;
XRCORE_API bool HasAVX2    = cpu().avx2;
XRCORE_API bool HasAVX512F = cpu().avx512f;

XRCORE_API u64 qpc_freq = SDL_GetPerformanceFrequency();

XRCORE_API u32 qpc_counter = 0;

XRCORE_API u64 QPC() noexcept
{
    u64 _dest = SDL_GetPerformanceCounter();
    qpc_counter++;
    return _dest;
}

XRCORE_API u32 GetTicks()
{
    return static_cast<u32>(SDL_GetTicks());
}
} // namespace CPU

//------------------------------------------------------------------------------------
void _initialize_cpu()
{
    ZoneScoped;

    // General CPU identification
    string256 features{};

    const auto listFeature = [&](pcstr featureName, bool hasFeature)
    {
        if (hasFeature)
        {
            if (!features[0])
                xr_strcpy(features, featureName);
            else
            {
                xr_strcat(features, ", ");
                xr_strcat(features, featureName);
            }
        }
    };

    // x86
    listFeature("SSE",     CPU::HasSSE);
    listFeature("SSE2",    CPU::HasSSE2);
    listFeature("SSE3",    cpu().sse3);
    listFeature("SSSE3",   cpu().ssse3);
    listFeature("SSE41",   cpu().sse4_1);
    listFeature("SSE42",   CPU::HasSSE42);
    listFeature("AVX",     CPU::HasAVX);
    listFeature("AVX2",    CPU::HasAVX2);
    listFeature("AVX512F", CPU::HasAVX512F);

    listFeature("AltiVec", SDL_HasAltiVec());
    listFeature("ARMSIMD", SDL_HasARMSIMD());
    listFeature("NEON",    SDL_HasNEON());
    listFeature("LSX",     SDL_HasLSX());
    listFeature("LASX",    SDL_HasLASX());

    Msg("* CPU features: %s", features);
    Msg("* CPU threads: %d", std::thread::hardware_concurrency());

    Log("");
    Fidentity.identity(); // Identity matrix
    Random.seed(u32(CPU::QPC() % (s64(1) << s32(32))));

    pvInitializeStatics(); // Lookup table for compressed normals

    _initialize_cpu_thread();
}

// per-thread initialization
#if defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64) || defined(XR_ARCHITECTURE_PPC64)
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

    if (CPU::HasSSE)
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
