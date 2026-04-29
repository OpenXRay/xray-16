#pragma once

namespace xray::render::fg
{
struct light_indirect
{
    Fvector P;
    Fvector D;
    float E;
    IRender_Sector::sector_id_t S;
};
} // namespace xray::render::fg
