#ifndef BASECOLOR_PACK_H
#define BASECOLOR_PACK_H

float PackBaseColorA(float metallic, float sssMask)
{
    if (sssMask > 0.01)
        return saturate(0.5 + 0.5 * sssMask);
    return saturate(metallic) * 0.499;
}

float UnpackMetallicFromBaseA(float a)
{
    return (a < 0.5) ? saturate(a / 0.499) : 0.0;
}

float UnpackSSSMaskFromBaseA(float a)
{
    return (a >= 0.5) ? saturate((a - 0.5) * 2.0) : 0.0;
}

float UnpackGBufferMetallic(float a, bool packedWithSSS, out float sssMask)
{
    sssMask = 0.0;
    if (packedWithSSS)
    {
        sssMask = UnpackSSSMaskFromBaseA(a);
        return UnpackMetallicFromBaseA(a);
    }
    if (a >= 0.5)
        return saturate(a);
    return saturate(a / 0.499);
}

#endif
