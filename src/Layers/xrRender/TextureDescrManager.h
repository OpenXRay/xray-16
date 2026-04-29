#pragma once

#include <bitset>

#include "ETextureParams.h"
#include "xrCommon/xr_unordered_map.h"

namespace xray::render::fg
{
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

        // PBR texture names (AI-generated or artist-authored)
        // Legacy separate textures (deprecated):
        shared_str m_metallic_name;
        shared_str m_roughness_name;
        shared_str m_ao_name;
        shared_str m_parallax_name;
        // Consolidated packed PBR texture (R=M, G=R, B=AO, A=Parallax):
        shared_str m_pbr_name;
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

    // Get detail scale for a texture (returns 1.0 if no detail texture)
    float GetDetailScale(const shared_str& tex_name) const;

    // Get PBR texture names (AI-generated or artist-authored)
    // Legacy separate texture getters (deprecated - prefer GetPBRName):
    shared_str GetMetallicName(const shared_str& tex_name) const;
    shared_str GetRoughnessName(const shared_str& tex_name) const;
    shared_str GetAOName(const shared_str& tex_name) const;
    shared_str GetParallaxName(const shared_str& tex_name) const;
    // Consolidated packed PBR texture (R=metallic, G=roughness, B=ao, A=parallax):
    shared_str GetPBRName(const shared_str& tex_name) const;
};

extern ECORE_API CTextureDescrMngr TextureDescr;
} // namespace xray::render::fg
