#include "stdafx.h"

#include <thread>
#include <SDL3/SDL.h>

// Initialized on startup
XRCORE_API Fmatrix Fidentity;
XRCORE_API Dmatrix Didentity;
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

    listFeature("LSX",     SDL_HasLSX());
    listFeature("LASX",    SDL_HasLASX());


    Msg("* CPU features: %s", features);
    Msg("* CPU threads: %d", std::thread::hardware_concurrency());

    Log("");
    Fidentity.identity(); // Identity matrix
    Didentity.identity(); // Identity matrix
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
#if 0
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
