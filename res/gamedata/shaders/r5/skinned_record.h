#ifndef SKINNED_RECORD_H
#define SKINNED_RECORD_H

struct SkinnedDrawRecord
{
    float4x4 world;
    uint boneOffset;
    uint splatOffset;
    uint splatCount;
    uint prevFirstVertex;
    float4 bounds;
};

#endif
