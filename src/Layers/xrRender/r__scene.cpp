#include "stdafx.h"
#include "r__scene.h"
#include "xrCDB/ISpatial.h"
#include "xrCDB/Intersect.hpp"
#include "xrEngine/IGame_Level.h"

namespace xray::render::fg
{
R_scene_geometry Scene;

R_scene_geometry::R_scene_geometry() : Sectors_xrc("scene") {}

void R_scene_geometry::load(const xr_vector<CSector::level_sector_data_t>& sectors_data,
                            const xr_vector<CPortal::level_portal_data_t>& portals_data)
{
    ZoneScoped;

    const auto portals_count = portals_data.size();
    const auto sectors_count = sectors_data.size();

    Sectors.resize(sectors_count);
    Portals.resize(portals_count);

    for (u32 idx = 0; idx < portals_count; ++idx)
    {
        auto* portal = xr_new<CPortal>();
        Portals[idx] = portal;
    }

    for (u32 idx = 0; idx < sectors_count; ++idx)
    {
        auto* sector = xr_new<CSector>();

        sector->unique_id = static_cast<IRender_Sector::sector_id_t>(idx);
        sector->setup(sectors_data[idx]);
        Sectors[idx] = sector;
    }

    for (u32 idx = 0; idx < portals_count; ++idx)
    {
        auto* portal = static_cast<CPortal*>(Portals[idx]);
        portal->setup(portals_data[idx], Sectors);
    }
}

void R_scene_geometry::unload()
{
    for (auto* sector : Sectors)
        xr_delete(sector);
    Sectors.clear();

    for (auto* portal : Portals)
        xr_delete(portal);
    Portals.clear();

    xr_delete(rmPortals);
    largest_sector_id = IRender_Sector::INVALID_SECTOR_ID;
}

IRender_Sector::sector_id_t R_scene_geometry::detect_sector(const Fvector& P)
{
    Fvector dir{ 0, -1, 0 };
    auto sector = detect_sector(P, dir);
    if (sector == IRender_Sector::INVALID_SECTOR_ID)
    {
        dir = { 0, 1, 0 };
        sector = detect_sector(P, dir);
    }
    return sector;
}

IRender_Sector::sector_id_t R_scene_geometry::detect_sector(const Fvector& P, Fvector& dir)
{
    int id1 = -1;
    float range1 = 500.f;
    if (rmPortals)
    {
        Sectors_xrc.ray_query(CDB::OPT_ONLYNEAREST, rmPortals, P, dir, range1);
        if (Sectors_xrc.r_count())
        {
            CDB::RESULT* RP1 = Sectors_xrc.r_begin();
            id1 = RP1->id;
            range1 = RP1->range;
        }
    }

    int id2 = -1;
    float range2 = range1;
    Sectors_xrc.ray_query(CDB::OPT_ONLYNEAREST, g_pGameLevel->ObjectSpace.GetStaticModel(), P, dir, range2);
    if (Sectors_xrc.r_count())
    {
        CDB::RESULT* RP2 = Sectors_xrc.r_begin();
        id2 = RP2->id;
        range2 = RP2->range;
    }

    int ID;
    if (id1 >= 0)
    {
        if (id2 >= 0)
            ID = (range1 <= range2 + EPS) ? id1 : id2;
        else
            ID = id1;
    }
    else if (id2 >= 0)
        ID = id2;
    else
        return IRender_Sector::INVALID_SECTOR_ID;

    if (ID == id1)
    {
        CDB::TRI* pTri = rmPortals->get_tris() + ID;
        CPortal* pPortal = Portals[pTri->dummy];
        return pPortal->getSectorFacing(P)->unique_id;
    }
    CDB::TRI* pTri = g_pGameLevel->ObjectSpace.GetStaticTris() + ID;
    return static_cast<IRender_Sector::sector_id_t>(pTri->sector);
}
}
