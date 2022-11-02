#ifndef _LOCAL_RAND
#define _LOCAL_RAND
#include "xr_types.h"
#include "xrCore/xrDebug.h"

/*
u32 dwRandSeed;
IC u32 dwfRandom(u32 dwRange)
{
u32 dwResult;
__asm {
mov eax,dwRange _eax = dwRange
imul edx,dwRandSeed,08088405H
inc edx
mov dwRandSeed,edx dwRandSeed = (dwRandSeed * 08088405H)+1
mul edx return (u64(dwRange) * u64(dwRandSeed)) >> 32
mov dwResult,edx
}
return(dwResult);
}
*/

class XRCORE_API CRandom
{
    volatile s32 holdrand;

public:
    CRandom() : holdrand(1){};
    CRandom(s32 _seed) : holdrand(_seed){};

    void seed(s32 val) { holdrand = val; }
    s32 maxI() { return 32767; }
    ICN s32 randI() noexcept { return (holdrand = holdrand * 214013L + 2531011L) >> 16 & 0x7fff; }
    s32 randI(s32 max) { VERIFY(max); return randI() % max; };
    s32 randI(s32 min, s32 max) { return min + randI(max - min); }
    s32 randIs(s32 range) { return randI(-range, range); }
    s32 randIs(s32 range, s32 offs) { return offs + randIs(range); }
    float maxF() { return 32767.f; }
    float randF() { return float(randI()) / maxF(); }
    float randF(float max) { return randF() * max; }
    float randF(float min, float max) { return min + randF(max - min); }
    float randFs(float range) { return randF(-range, range); }
    float randFs(float range, float offs) { return offs + randFs(range); }
};

XRCORE_API extern CRandom Random;

#endif
