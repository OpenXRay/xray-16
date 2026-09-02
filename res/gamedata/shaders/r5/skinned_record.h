#ifndef SKINNED_RECORD_H
#define SKINNED_RECORD_H

struct SkinnedDrawRecord
{
    float4x4 world;
    uint boneOffset;
    uint splatOffset;
    uint splatCount;
    uint pad;
    float4 bounds;
};

#endif
