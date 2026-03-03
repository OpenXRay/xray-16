#ifndef SMOKE_COMMON_H
#define SMOKE_COMMON_H

struct SmokeSimPoint
{
    float px, py, pz;
    float lifetime;
    float vx, vy, vz;
    float age;
    float width;
    float seedF;
    float pad0;
    float pad1;
};

struct TrailControlPoint
{
    float posX, posY, posZ;
    float dirX, dirY, dirZ;
    float ageNorm;
    float cumDist;
};

#include "noise4d.h"

#endif
