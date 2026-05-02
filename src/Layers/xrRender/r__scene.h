#pragma once

#include "r__sector.h"

namespace CDB { class MODEL; }

namespace xray::render::fg
{
struct R_scene_geometry
{
    xr_vector<CSector*> Sectors;
    xr_vector<CPortal*> Portals;
    xrXRC Sectors_xrc;
    CDB::MODEL* rmPortals{ nullptr };
    IRender_Sector::sector_id_t largest_sector_id{ IRender_Sector::INVALID_SECTOR_ID };

    R_scene_geometry();
    ~R_scene_geometry() = default;

    void load(const xr_vector<CSector::level_sector_data_t>& sectors,
              const xr_vector<CPortal::level_portal_data_t>& portals);
    void unload();

    ICF IRender_Portal* get_portal(size_t id) const
    {
        VERIFY(id < Portals.size());
        return Portals[id];
    }
    ICF IRender_Sector* get_sector(size_t id) const
    {
        VERIFY(id < Sectors.size());
        return Sectors[id];
    }
    IRender_Sector::sector_id_t detect_sector(const Fvector& P);
    IRender_Sector::sector_id_t detect_sector(const Fvector& P, Fvector& D);
};

extern R_scene_geometry Scene;
}
