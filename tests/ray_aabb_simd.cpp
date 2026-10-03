#include <limits>
#if defined(__ARM_NEON) || defined(__aarch64__)
#define SSE2NEON_PRECISE_MINMAX 1
#include "sse2neon/sse2neon.h"
#else
#include <xmmintrin.h>
#endif
#include "src/xrCDB/RayQuerySimd.h"
#include "src/xrCDB/RayAabbSimd.h"

struct Case { float data[12]; int hit; };
static const Case cases[] = {
    {{124.466537f, -6.54939651f, 183.403625f, 2.5002861f, 0.781922102f, 8.08235931f, 125.613922f, -5.66516733f, 178.069275f, 0.0f, -1.0f, 0.0f}, 1},
    {{135.098572f, -3.06876755f, 181.624512f, 3.25753784f, 1.98182011f, 5.24744415f, 134.496475f, -5.67908335f, 180.527756f, 0.0f, 1.0f, 0.0f}, 1},
    {{124.054192f, -3.67069602f, 183.403625f, 2.71134186f, 1.43952012f, 8.48545074f, 123.519043f, -5.66414404f, 183.70578f, 0.0f, 1.0f, 0.0f}, 1},
    {{132.488953f, -7.19435167f, 177.946335f, 4.42656708f, 1.07503271f, 1.9178009f, 129.680099f, -5.68539524f, 178.849121f, 0.0f, -1.0f, 0.0f}, 1},
    {{132.488953f, -7.19435167f, 177.946335f, 4.42656708f, 1.07503271f, 1.9178009f, 129.172302f, -5.68582487f, 178.571899f, 0.0f, -1.0f, 0.0f}, 1},
    {{106.944344f, 1.48465347f, 182.016342f, 2.60402679f, 0.772610664f, 1.52476501f, 104.6063f, 0.317174196f, 181.929108f, 0.0f, 1.0f, 0.0f}, 1},
    {{101.480148f, -1.22119021f, 186.053604f, 2.81028748f, 0.844687223f, 2.6499939f, 101.084404f, 0.31951189f, 187.437851f, 0.0f, -1.0f, 0.0f}, 1},
    {{139.578857f, -4.69246864f, 184.840485f, 19.7126808f, 3.6055212f, 26.6014938f, 135.062424f, -0.935263276f, 183.640076f, 0.0f, -1.0f, 0.0f}, 1},
    {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 2.0f, 0.5f, 0.0f, -1.0f, 0.0f}, 1},
    {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 1.0f, 2.0f, 0.5f, 0.0f, -1.0f, 0.0f}, 1},
    {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.01f, 2.0f, 0.5f, 0.0f, -1.0f, 0.0f}, 0},
    {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 2.0f, 0.0f, 0.0f, -1.0f, 0.0f}, 1},
    {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, 1},
    {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 2.0f, 0.5f, 0.0f, 1.0f, 0.0f}, 0},
    {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f}, 1},
    {{0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 2.0f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f}, 1}
};

extern "C" int test(int index)
{
    if (index < 0)
        return sizeof(cases) / sizeof(cases[0]);
#if defined(__ARM_NEON) || defined(__aarch64__)
    if (!CDB::use_simd_ray_query(false)) return 2;
#else
    if (!CDB::use_simd_ray_query(true)) return 2;
#endif
    const auto& c = cases[index];
    alignas(16) float lo[4]{}, hi[4]{}, origin[4]{}, inverse[4]{};
    for (int k = 0; k < 3; ++k)
    {
        lo[k] = c.data[k] - c.data[k + 3];
        hi[k] = c.data[k] + c.data[k + 3];
        origin[k] = c.data[k + 6];
        inverse[k] = 1.f / c.data[k + 9];
    }
    float distance = 0;
    return int(CDB::ray_aabb_simd(lo, hi, origin, inverse, distance)) == c.hit ? 0 : 1;
}

#ifndef XR_TEST_ARM_ENTRY
#include <cstdio>
int main()
{
    for (int i = 0; i < test(-1); ++i)
        if (const int error = test(i))
        {
            std::printf("Ray/AABB case %d failed: %d\n", i, error);
            return 1;
        }
    std::printf("Ray/AABB: %d cases passed\n", test(-1));
}
#endif
