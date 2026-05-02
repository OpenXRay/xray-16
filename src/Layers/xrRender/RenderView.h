#pragma once

#include "r__sector.h"

namespace xray::render::fg
{
struct RenderView
{
    u32 phase{};
    u32 portal_traverse_flags{};
    u32 spatial_traverse_flags{};
    u32 spatial_types{ STYPE_RENDERABLE };
    float query_box_side{ EPS_L * 20.0f };
    Fvector view_pos{};
    Fmatrix xform{};
    CFrustum view_frustum{};
    IRender_Sector::sector_id_t sector_id{ IRender_Sector::INVALID_SECTOR_ID };
    bool pmask[2]{ true, true };
    bool pmask_wmark{ false };
    bool use_hom{ false };
    bool precise_portals{ false };
    bool is_main_pass{ false };
    bool mt_calculate{ false };

    void reset()
    {
        query_box_side = EPS_L * 20.0f;
        use_hom = false;
        precise_portals = false;
        is_main_pass = false;
        spatial_traverse_flags = 0;
        portal_traverse_flags = 0;
        spatial_types = STYPE_RENDERABLE;
    }

    void r_pmask(bool _1, bool _2, bool _wm = false)
    {
        pmask[0] = _1;
        pmask[1] = _2;
        pmask_wmark = _wm;
    }
};
}
