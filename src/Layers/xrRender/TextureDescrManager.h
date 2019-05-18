#pragma once

#include <bitset>

#include "ETextureParams.h"
#include "xrCommon/xr_unordered_map.h"

class cl_dt_scaler;

class CTextureDescrMngr
{
    struct texture_assoc
    {
        shared_str detail_name;
        std::bitset<2> usage;
        enum
        {
            flDiffuseDetail,
            flBumpDetail
        };
        texture_assoc() { usage.reset(); }
    };
    struct texture_spec
    {
        shared_str m_bump_name;
        float m_material;
        bool m_use_steep_parallax;
    };
    struct texture_desc
    {
        texture_assoc* m_assoc;
        texture_spec* m_spec;
        texture_desc() : m_assoc(nullptr), m_spec(nullptr) {}
    };

    using map_TD = xr_unordered_map<shared_str, texture_desc>;
    using map_CS = xr_unordered_map<shared_str, cl_dt_scaler*>;

    map_TD m_texture_details;
    map_CS m_detail_scalers;

    void LoadTHM(pcstr initial, bool listTHM);
    void LoadLTX(pcstr initial, bool listTHM);

public:
    ~CTextureDescrMngr();
    void Load();
    void UnLoad();

public:
    shared_str GetBumpName(const shared_str& tex_name) const;
    float GetMaterial(const shared_str& tex_name) const;
    void GetTextureUsage(const shared_str& tex_name, bool& bDiffuse, bool& bBump) const;
    BOOL GetDetailTexture(const shared_str& tex_name, LPCSTR& res, R_constant_setup*& CS) const;
    BOOL UseSteepParallax(const shared_str& tex_name) const;
};
