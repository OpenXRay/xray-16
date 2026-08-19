#ifndef SURFACE_MARKS_H
#define SURFACE_MARKS_H

static const float SURF_MARK_HUD = 2.0;
static const float SURF_MARK_CHAR = 3.0;
static const float SURF_MARK_TERRAIN = 4.0;
static const float SURF_MARK_WATER = 5.0;
static const float SURF_MARK_INTERIOR = 6.0;
static const float SURF_MARK_GRASS = 7.0;
static const float SURF_MARK_FOLIAGE = 8.0;
static const float SURF_FORWARD_LIT_A = 0.03125;

bool IsForwardLitAlpha(float a)
{
    return a > 0.028 && a < 0.035;
}

bool IsHudSurfMark(float w)
{
    return w > 1.5 && w < 2.5;
}

bool IsCharSurfMark(float w)
{
    return w > 2.5 && w < 3.5;
}

bool IsTerrainSurfMark(float w)
{
    return w > 3.5 && w < 4.5;
}

bool IsWaterSurfMark(float w)
{
    return w > 4.5 && w < 5.5;
}

bool IsInteriorSurfMark(float w)
{
    return w > 5.5 && w < 6.5;
}

bool IsGrassSurfMark(float w)
{
    return w > 6.5 && w < 7.5;
}

bool IsFoliageSurfMark(float w)
{
    return w > 7.5 && w < 8.5;
}

bool IsVegSurfMark(float w)
{
    return IsGrassSurfMark(w) || IsFoliageSurfMark(w);
}

bool SameLightZone(float a, float b)
{
    return IsInteriorSurfMark(a) == IsInteriorSurfMark(b);
}

bool IsHudPixel(float depth, float mark)
{
    return IsHudSurfMark(mark);
}

bool SkipRtSurfLighting(float classifyW, float guideW)
{
    return IsWaterSurfMark(classifyW) && !IsHudSurfMark(guideW);
}

bool SameHudSurfClass(float a, float b)
{
    return IsHudSurfMark(a) == IsHudSurfMark(b);
}

float SurfMarkFromGBuffer(float worldPosW, float baseColorA)
{
    if (worldPosW > 1.5)
        return worldPosW;
    if (baseColorA > 1.5)
        return baseColorA;
    return worldPosW;
}

#endif
