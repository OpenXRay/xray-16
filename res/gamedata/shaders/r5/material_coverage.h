#ifndef MATERIAL_COVERAGE_H
#define MATERIAL_COVERAGE_H

#include "bindless_common.h"

bool MaterialHasAlphaTest(MaterialData mat)
{
    return (mat.flags & MAT_FLAG_ALPHA_TEST) != 0u;
}

bool MaterialHasAlphaBlend(MaterialData mat)
{
    return (mat.flags & MAT_FLAG_ALPHA_BLEND) != 0u;
}

bool MaterialHasAlphaCoverage(MaterialData mat)
{
    return (mat.flags & (MAT_FLAG_ALPHA_TEST | MAT_FLAG_ALPHA_BLEND)) != 0u;
}

float MaterialAlphaTestClip(MaterialData mat, float alpha)
{
    return MaterialHasAlphaTest(mat) ? alpha - mat.alphaRef : 1.0;
}

bool MaterialAlphaTestRejects(MaterialData mat, float alpha)
{
    return MaterialHasAlphaTest(mat) && alpha < mat.alphaRef;
}

float MaterialAlphaBlendClip(MaterialData mat, float alpha, float threshold)
{
    return MaterialHasAlphaBlend(mat) ? saturate(alpha) - threshold : 1.0;
}

float MaterialBlendOpacity(MaterialData mat, float alpha)
{
    return MaterialHasAlphaBlend(mat) ? saturate(alpha) : 1.0;
}

float MaterialRayShadowAlpha(MaterialData mat, float2 uv)
{
    return SampleDiffuseLevel(mat, uv).a;
}

#ifndef BINDLESS_NO_IMPLICIT_GRAD
float MaterialRasterShadowAlpha(MaterialData mat, float2 uv)
{
    return MaterialHasAlphaBlend(mat) ? SampleDiffuseLevel(mat, uv).a : SampleDiffuse(mat, uv).a;
}
#endif

#endif
