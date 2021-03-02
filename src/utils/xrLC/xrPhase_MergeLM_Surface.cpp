#include "stdafx.h"
#include "build.h"
#include "xrPhase_MergeLM_Rect.h"
#include "utils/xrLC_Light/xrdeflector.h"

#if defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_E2K)
#include <intrin.h>
#else
#include <mmintrin.h>
#include <emmintrin.h>
#endif

static u8 surface[c_LMAP_size * c_LMAP_size];
const u32 alpha_ref = 254 - BORDER;

// Initialization
void _InitSurface() { FillMemory(surface, c_LMAP_size * c_LMAP_size, 0); }
// Rendering of rect
void _rect_register(L_rect& R, lm_layer* D, BOOL bRotate)
{
    u8* lm = &*(D->marker.begin());
    u32 s_x = D->width + 2 * BORDER;
    u32 s_y = D->height + 2 * BORDER;

    if (!bRotate)
    {
        // Normal (and fastest way)
        for (u32 y = 0; y < s_y; y++)
        {
            u8* P = surface + (y + R.a.y) * c_LMAP_size + R.a.x; // destination scan-line
            u8* S = lm + y * s_x;
            for (u32 x = 0; x < s_x; x++, P++, S++)
                if (*S >= alpha_ref)
                    *P = 255;
        }
    }
    else
    {
        // Rotated :(
        for (u32 y = 0; y < s_x; y++)
        {
            u8* P = surface + (y + R.a.y) * c_LMAP_size + R.a.x; // destination scan-line
            for (u32 x = 0; x < s_y; x++, P++)
                if (lm[x * s_x + y] >= alpha_ref)
                    *P = 255;
        }
    }
}

// Test of per-pixel intersection (surface test)
bool Place_Perpixel(L_rect& R, lm_layer* D, BOOL bRotate)
{
    u8* lm = &*(D->marker.begin());
    int s_x = D->width + 2 * BORDER;
    int s_y = D->height + 2 * BORDER;
    int x;

#ifdef _M_X64
    __m128i mm_alpha_ref = _mm_set1_epi8(alpha_ref);
    __m128i mm_zero = _mm_setzero_si128();
#else
    const __m64 mm_alpha_ref = _mm_set1_pi8(alpha_ref);
    const __m64 mm_zero = _mm_setzero_si64();
#endif

    if (!bRotate)
    {
        // Normal (and fastest way)
        for (int y = 0; y < s_y; y++)
        {
            u8* P = surface + (y + R.a.y) * c_LMAP_size + R.a.x; // destination scan-line
            u8* S = lm + y * s_x;
            // accelerated part
            for (x = 0; x < s_x - 8; x += 8, P += 8, S += 8)
            {
                // if ( (*P) && ( *S >= alpha_ref ) ) goto r_false;	// overlap
#if defined(XR_ARCHITECTURE_X64) || defined(XR_ARCHITECTURE_E2K)
                __m128i regS = _mm_set1_epi64x(*((__int64*)S));
                __m128i regP = _mm_set1_epi64x(*((__int64*)P));
                __m128i mm_max = _mm_max_epu8(regS, mm_alpha_ref);
                __m128i mm_cmp = _mm_cmpeq_epi8(mm_max, mm_alpha_ref);
                __m128i mm_andn = _mm_andnot_si128(mm_cmp, regP);
                __m128i mm_andn_low = _mm_move_epi64(mm_andn);
                __m128i mm_sad = _mm_sad_epu8(mm_andn_low, mm_zero);
                if (_mm_cvtsi128_si32(mm_sad))
                    return false;
#else
                __m64 mm_max = _mm_max_pu8(*(__m64*)S, mm_alpha_ref);
                __m64 mm_cmp = _mm_cmpeq_pi8(mm_max, mm_alpha_ref);
                __m64 mm_andn = _mm_andnot_si64(mm_cmp, *(__m64*)P);
                __m64 mm_sad = _mm_sad_pu8(mm_andn, mm_zero);
                if (_mm_cvtsi64_si32(mm_sad))
                {
                    _mm_empty();
                    return false;
                }
#endif
            }
            // remainder part
            for (; x < s_x; x++, P++, S++)
                if ((*P) && (*S >= alpha_ref))
                {
#ifdef XR_ARCHITECTURE_X86
                    _mm_empty();
#endif
                    return false;
                }
        }
    }
    else
    {
        // Rotated :(
        for (int y = 0; y < s_x; y++)
        {
            u8* P = surface + (y + R.a.y) * c_LMAP_size + R.a.x; // destination scan-line
            for (x = 0; x < s_y; x++, P++)
                if ((*P) && (lm[x * s_x + y] >= alpha_ref))
                {
#ifdef XR_ARCHITECTURE_X86
                    _mm_empty();
#endif
                    return false;
                }
        }
    }

    // It's OK to place it
#ifdef XR_ARCHITECTURE_X86
    _mm_empty();
#endif
    return true;
}

// Check for intersection
BOOL _rect_place(L_rect& r, lm_layer* D)
{
    L_rect R;
    int _X;
    u8* temp_surf;

    // Normal
    {
        int x_max = c_LMAP_size - r.b.x;
        int y_max = c_LMAP_size - r.b.y;
        for (int _Y = 0; _Y < y_max; _Y++)
        {
            temp_surf = surface + _Y * c_LMAP_size;
            // accelerated part
            for (_X = 0; _X < x_max - 8;)
            {
#ifdef _M_X64
                __m128i init = _mm_set1_epi64x(*(temp_surf + _X));
                __m128i m64_cmp = _mm_cmpeq_epi8(init, _mm_setzero_si128());
                __m128i m64_cmp_low = _mm_move_epi64(m64_cmp);
                __m128i m64_work = _mm_sad_epu8(m64_cmp_low, _mm_setzero_si128());

                if (!_mm_cvtsi128_si32(m64_work)) {
                    _X += 8;
                    continue;
                }
#else
                __m64 m64_cmp = _mm_cmpeq_pi8(*(__m64*)(temp_surf + _X), _mm_setzero_si64());
                __m64 m64_work = _mm_sad_pu8(m64_cmp, _mm_setzero_si64());

                if (!_mm_cvtsi64_si32(m64_work))
                {
                    _X += 8;
                    continue;
                }
#endif
                if (temp_surf[_X])
                {
                    _X++;
                    continue;
                }

                R.init(_X, _Y, _X + r.b.x, _Y + r.b.y);

                _X++;

                if (Place_Perpixel(R, D, FALSE))
                {
                    _rect_register(R, D, FALSE);
                    r.set(R);
#ifdef XR_ARCHITECTURE_X86
                    _mm_empty();
#endif
                    return TRUE;
                }
            }
            // remainder part
            for (; _X < x_max; _X++)
            {
                if (temp_surf[_X])
                    continue;
                R.init(_X, _Y, _X + r.b.x, _Y + r.b.y);
                if (Place_Perpixel(R, D, FALSE))
                {
                    _rect_register(R, D, FALSE);
                    r.set(R);
#ifdef XR_ARCHITECTURE_X86
                    _mm_empty();
#endif
                    return TRUE;
                }
            }
        }
    }

    // Rotated
    {
        int x_max = c_LMAP_size - r.b.y;
        int y_max = c_LMAP_size - r.b.x;
        for (int _Y = 0; _Y < y_max; _Y++)
        {
            temp_surf = surface + _Y * c_LMAP_size;
            // accelerated part
            for (_X = 0; _X < x_max - 8;)
            {
#ifdef _M_X64
                __m128i init = _mm_set1_epi64x(*(temp_surf + _X));
                __m128i m64_cmp = _mm_cmpeq_epi8(init, _mm_setzero_si128());
                __m128i m64_cmp_low = _mm_move_epi64(m64_cmp);
                __m128i m64_work = _mm_sad_epu8(m64_cmp_low, _mm_setzero_si128());

                if (!_mm_cvtsi128_si32(m64_work)) {
                    _X += 8;
                    continue;
                }
#else
                __m64 m64_cmp = _mm_cmpeq_pi8(*(__m64*)(temp_surf + _X), _mm_setzero_si64());
                __m64 m64_work = _mm_sad_pu8(m64_cmp, _mm_setzero_si64());

                if (!_mm_cvtsi64_si32(m64_work))
                {
                    _X += 8;
                    continue;
                }
#endif
                if (temp_surf[_X])
                {
                    _X++;
                    continue;
                }

                R.init(_X, _Y, _X + r.b.y, _Y + r.b.x);

                _X++;

                if (Place_Perpixel(R, D, TRUE))
                {
                    _rect_register(R, D, TRUE);
                    r.set(R);
#ifdef XR_ARCHITECTURE_X86
                    _mm_empty();
#endif
                    return TRUE;
                }
            }
            // remainder part
            for (; _X < x_max; _X++)
            {
                if (temp_surf[_X])
                    continue;
                R.init(_X, _Y, _X + r.b.y, _Y + r.b.x);
                if (Place_Perpixel(R, D, TRUE))
                {
                    _rect_register(R, D, TRUE);
                    r.set(R);
#ifdef XR_ARCHITECTURE_X86
                    _mm_empty();
#endif
                    return TRUE;
                }
            }
        }
    }

#ifdef XR_ARCHITECTURE_X86
    _mm_empty();
#endif
    return FALSE;
}
