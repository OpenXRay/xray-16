#include "stdafx.h"

#include <thread>
#include <SDL3/SDL.h>

// Initialized on startup
XRCORE_API Fmatrix Fidentity;
XRCORE_API CRandom Random;

namespace CPU
{
XRCORE_API bool HasSSE     = SDL_HasSSE();
XRCORE_API bool HasSSE2    = SDL_HasSSE2();
XRCORE_API bool HasSSE42   = SDL_HasSSE42();

XRCORE_API bool HasAVX     = SDL_HasAVX();

XRCORE_API bool HasAVX2    = SDL_HasAVX2();

XRCORE_API bool HasAVX512F = SDL_HasAVX512F();

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
    return SDL_GetTicks();
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
    listFeature("MMX",     SDL_HasMMX());
    listFeature("SSE",     SDL_HasSSE());
    listFeature("SSE2",    CPU::HasSSE2);
    listFeature("SSE3",    SDL_HasSSE3());
    listFeature("SSE41",   SDL_HasSSE41());
    listFeature("SSE42",   CPU::HasSSE42);
    listFeature("AVX",     CPU::HasAVX);
    listFeature("AVX2",    CPU::HasAVX2);
    listFeature("AVX512F", CPU::HasAVX512F);

    // Other architectures
    listFeature("AltiVec", SDL_HasAltiVec());
    listFeature("ARMSIMD", SDL_HasARMSIMD());
    listFeature("NEON",    SDL_HasNEON());
#if SDL_VERSION_ATLEAST(2, 24, 0)
    listFeature("LSX",     SDL_HasLSX());
    listFeature("LASX",    SDL_HasLASX());
#endif

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
#if defined(__SSE3__) || defined(_MSC_VER)
#include <pmmintrin.h>
#endif
#ifndef _MM_DENORMALS_ZERO_ON
#define _MM_DENORMALS_ZERO_ON 0
#define _MM_SET_DENORMALS_ZERO_MODE(mode) ((void)0)
#endif
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
