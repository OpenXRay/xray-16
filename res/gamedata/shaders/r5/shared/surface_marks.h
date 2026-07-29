#ifndef SURFACE_MARKS_H
#define SURFACE_MARKS_H

static const float SURF_MARK_HUD = 2.0;
static const float SURF_MARK_CHAR = 3.0;
static const float SURF_MARK_TERRAIN = 4.0;
static const float SURF_MARK_WATER = 5.0;

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
    return w > 4.5;
}

bool SkipRtSurfLighting(float classifyW, float guideW)
{
    if (IsHudSurfMark(guideW))
        return false;
    return IsWaterSurfMark(classifyW);
}

bool SameHudSurfClass(float a, float b)
{
    return IsHudSurfMark(a) == IsHudSurfMark(b);
}

#endif
