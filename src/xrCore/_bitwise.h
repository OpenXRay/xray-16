#pragma once
#ifndef _BITWISE_H
#define _BITWISE_H
#include "math_constants.h"
#include "_types.h"

// float values defines
#define fdSGN 0x080000000 // mask for sign bit

// integer math on floats
IC bool negative(const float f) { return f < 0; }
IC bool positive(const float f) { return f >= 0; }
IC void set_negative(float& f) { f = -fabsf(f); }
IC void set_positive(float& f) { f = fabsf(f); }

/*
 * Here are a few nice tricks for 2's complement based machines
 * that I discovered a few months ago.
 */
IC int btwLowestBitMask(int v) { return v & -v; }
IC u32 btwLowestBitMask(u32 x) { return x & ~(x - 1); }
/* Ok, so now we are cooking on gass. Here we use this function for some */
/* rather useful utility functions */
IC bool btwIsPow2(int v) { return btwLowestBitMask(v) == v; }
IC bool btwIsPow2(u32 v) { return btwLowestBitMask(v) == v; }
IC int btwPow2_Ceil(int v)
{
    int i = btwLowestBitMask(v);
    while (i < v)
        i <<= 1;
    return i;
}
IC u32 btwPow2_Ceil(u32 v)
{
    u32 i = btwLowestBitMask(v);
    while (i < v)
        i <<= 1;
    return i;
}

// Couple more tricks
// Counting number of nonzero bits for 8bit number:
IC u8 btwCount1(u8 v)
{
    v = (v & 0x55) + (v >> 1 & 0x55);
    v = (v & 0x33) + (v >> 2 & 0x33);
    return (v & 0x0f) + (v >> 4 & 0x0f);
}

// same for 32bit
IC u32 btwCount1(u32 v)
{
#ifdef __GNUC__
    return __builtin_popcount(v);
#else
    const u32 g31 = 0x49249249ul; // = 0100_1001_0010_0100_1001_0010_0100_1001
    const u32 g32 = 0x381c0e07ul; // = 0011_1000_0001_1100_0000_1110_0000_0111
    v = (v & g31) + (v >> 1 & g31) + (v >> 2 & g31);
    v = (v + (v >> 3) & g32) + (v >> 6 & g32);
    return v + (v >> 9) + (v >> 18) + (v >> 27) & 0x3f;
#endif
}

IC u64 btwCount1(u64 v)
{
#ifdef __GNUC__
    return __builtin_popcountll(v);
#else
    return btwCount1(u32(v & u32(-1))) + btwCount1(u32(v >> u64(32))); 
#endif
}

IC int iFloor(float x)
{
    return std::floor(x);
}

/* intCeil() is a non-interesting variant, since effectively
 ceil(x) == -floor(-x)
 */
IC int iCeil(float x)
{
    return std::ceil(x);
}

// Only for [0..1] (positive) range
IC float apx_asin(const float x)
{
    const float c1 = 0.892399f;
    const float c3 = 1.693204f;
    const float c5 = -3.853735f;
    const float c7 = 2.838933f;

    const float x2 = x * x;
    const float d = x * (c1 + x2 * (c3 + x2 * (c5 + x2 * c7)));

    return d;
}
// Only for [0..1] (positive) range
IC float apx_acos(const float x) { return PI_DIV_2 - apx_asin(x); }
#endif
