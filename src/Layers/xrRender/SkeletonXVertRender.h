#pragma once

namespace xray::render::fg
{
#pragma pack(push, 2)
struct vertRender // T&B are not skinned, because in R2 skinning occurs always in hardware
{
    Fvector P;
    Fvector N;
    float u, v;
};
#pragma pack(pop)
} // namespace xray::render::fg
