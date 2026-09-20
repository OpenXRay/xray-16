#ifndef DETAIL_SOURCE_COMMON_H
#define DETAIL_SOURCE_COMMON_H

#include "detail_blade_common.h"

StructuredBuffer<DetailInstance> g_DetailChunks[] : register(t0, space2);

DetailInstance LoadDetailInstance(uint2 address)
{
    return g_DetailChunks[NonUniformResourceIndex(address.x)][address.y];
}

#endif
