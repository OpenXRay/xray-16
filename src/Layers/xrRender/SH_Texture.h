#pragma once

#include "xrCore/xr_resource.h"
#include <nvrhi/nvrhi.h>

class CAviPlayerCustom;
class ENGINE_API CTheoraSurface;

namespace xray::render::fg
{
class ECORE_API CTexture : public xr_resource_named
{
public:
    enum MaxTextures
    {
        mtMaxPixelShaderTextures = 16,
        mtMaxVertexShaderTextures = 4,
        mtMaxGeometryShaderTextures = 16,
        mtMaxHullShaderTextures = 16,
        mtMaxDomainShaderTextures = 16,
        mtMaxComputeShaderTextures = 16,
        mtMaxCombinedShaderTextures =
            mtMaxPixelShaderTextures
            + mtMaxVertexShaderTextures
            + mtMaxGeometryShaderTextures
            + mtMaxHullShaderTextures
            + mtMaxDomainShaderTextures
            + mtMaxComputeShaderTextures
    };

    enum ResourceShaderType
    {
        rstPixel = 0,
        rstVertex = 257,
        rstGeometry = rstVertex + 256,
        rstHull = rstGeometry + 256,
        rstDomain = rstHull + 256,
        rstCompute = rstDomain + 256,
        rstInvalid = rstCompute + 256
    };

public:
    void apply_load(CBackend& cmd_list, u32 stage);
    void apply_theora(CBackend& cmd_list, u32 stage);
    void apply_avi(CBackend& cmd_list, u32 stage);
    void apply_seq(CBackend& cmd_list, u32 stage);
    void apply_normal(CBackend& cmd_list, u32 stage);

    void set_slice(int slice);

    void Preload();
    void Load();
    void PostLoad();
    void Unload();

    void surface_set(nvrhi::TextureHandle tex);
    [[nodiscard]] nvrhi::ITexture* surface_get_native() const { return nvrhiTexture.Get(); }

    [[nodiscard]] BOOL isUser() const
    {
        return flags.bUser;
    }

    u32 get_Width()
    {
        desc_enshure();
        return m_width;
    }

    u32 get_Height()
    {
        desc_enshure();
        return m_height;
    }

    void video_Sync(u32 _time) { m_play_time = _time; }
    void video_Play(BOOL looped, u32 _time = 0xFFFFFFFF);
    void video_Pause(BOOL state) const;
    void video_Stop() const;
    [[nodiscard]] BOOL video_IsPlaying() const;

    CTexture();
    virtual ~CTexture();

    ImTextureID GetImTextureID();

private:
    [[nodiscard]] BOOL desc_valid() const { return nvrhiTexture.Get() == desc_cache; }

    void desc_enshure()
    {
        if (!desc_valid())
            desc_update();
    }

    void desc_update();

    void Apply(CBackend& cmd_list, u32 dwStage);

public:
    struct
    {
        u32 bLoaded : 1;
        u32 bUser : 1;
        u32 seqCycles : 1;
        u32 MemoryUsage : 28;
    } flags;

    fastdelegate::FastDelegate2<CBackend&, u32> bind;

    CAviPlayerCustom* pAVI;
    CTheoraSurface* pTheora;
    float m_material;
    shared_str m_bumpmap;

    shared_str m_metallic;
    shared_str m_roughness;
    shared_str m_ao;
    shared_str m_parallax;

    union
    {
        u32 m_play_time; // sync theora time
        u32 seqMSPF;     // Sequence data milliseconds per frame
    };

    int curr_slice{ -1 };
    int last_slice{ -1 };

    nvrhi::TextureHandle nvrhiTexture;
    xr_vector<nvrhi::TextureHandle> seqNvrhiTextures;

private:
    u32 m_width{};
    u32 m_height{};
    nvrhi::ITexture* desc_cache{};
};

struct resptrcode_texture : public resptr_base<CTexture>
{
    void create(LPCSTR _name);
    void destroy() { _set(nullptr); }
    shared_str bump_get() { return _get()->m_bumpmap; }
    bool bump_exist() { return 0 != bump_get().size(); }

    shared_str metallic_get() { return _get()->m_metallic; }
    bool metallic_exist() { return 0 != metallic_get().size(); }

    shared_str roughness_get() { return _get()->m_roughness; }
    bool roughness_exist() { return 0 != roughness_get().size(); }

    shared_str ao_get() { return _get()->m_ao; }
    bool ao_exist() { return 0 != ao_get().size(); }

    shared_str parallax_get() { return _get()->m_parallax; }
    bool parallax_exist() { return 0 != parallax_get().size(); }
};

typedef resptr_core<CTexture, resptrcode_texture> ref_texture;
} // namespace xray::render::fg
