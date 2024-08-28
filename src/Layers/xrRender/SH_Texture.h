#ifndef SH_TEXTURE_H
#define SH_TEXTURE_H
#pragma once

#include "xrCore/xr_resource.h"

class CAviPlayerCustom;
class ENGINE_API CTheoraSurface;

class ECORE_API CTexture : public xr_resource_named
{
public:
    enum	MaxTextures
    {
        //	Actually these values are 128
        mtMaxPixelShaderTextures = 16,
        mtMaxVertexShaderTextures = 4,
        mtMaxGeometryShaderTextures = 16,
#ifdef USE_DX11
        mtMaxHullShaderTextures = 16,
        mtMaxDomainShaderTextures = 16,
        mtMaxComputeShaderTextures = 16,
#endif
        mtMaxCombinedShaderTextures =
        mtMaxPixelShaderTextures
        + mtMaxVertexShaderTextures
        + mtMaxGeometryShaderTextures
#ifdef USE_DX11
        + mtMaxHullShaderTextures
        + mtMaxDomainShaderTextures
        + mtMaxComputeShaderTextures
#endif
    };

#if defined(USE_DX11)
    //	Since DX11 allows up to 128 unique textures,
    //	distance between enum values should be at leas 128
    enum ResourceShaderType //	Don't change this since it's hardware-dependent
    {
        rstPixel = 0,
        // Default texture offset
        rstVertex = D3DVERTEXTEXTURESAMPLER0,
        rstGeometry = rstVertex + 256,
        rstHull = rstGeometry + 256,
        rstDomain = rstHull + 256,
        rstCompute = rstDomain + 256,
        rstInvalid = rstCompute + 256
    };
#elif defined(USE_OGL)
    //	Since OGL doesn't differentiate between stages,
    //	distance between enum values should be the max for that stage.
    enum ResourceShaderType
    {
        rstPixel = 0,	//	Default texture offset
        rstVertex = rstPixel + mtMaxPixelShaderTextures,
        rstGeometry = rstVertex + mtMaxVertexShaderTextures,
    };
#else
#   error No graphics API selected or enabled!
#endif

public:
    void apply_load(CBackend& cmd_list, u32 stage);
    void apply_theora(CBackend& cmd_list, u32 stage);
    void apply_avi(CBackend& cmd_list, u32 stage) const;
    void apply_seq(CBackend& cmd_list, u32 stage);
    void apply_normal(CBackend& cmd_list, u32 stage) const;

    void set_slice(int slice);

    void Preload();
    void Load();
    void PostLoad();
    void Unload();
    // void Apply(u32 dwStage);

#if defined(USE_DX11)
    void surface_set(ID3DBaseTexture* surf);
    [[nodiscard]] ID3DBaseTexture* surface_get() const;
#elif defined(USE_OGL)
    void surface_set(GLenum target, GLuint surf);
    [[nodiscard]] GLuint surface_get() const;
#else
#   error No graphics API selected or enabled!
#endif

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

#if defined(USE_DX11)
    ID3DShaderResourceView* get_SRView() { return m_pSRView; }
#endif

private:
    [[nodiscard]] BOOL desc_valid() const
    {
        return pSurface == desc_cache;
    }

    void desc_enshure()
    {
        if (!desc_valid())
            desc_update();
    }

    void desc_update();
#if defined(USE_DX11)
    void Apply(CBackend& cmd_list, u32 dwStage) const;
    D3D_USAGE GetUsage();
#endif

    //	Class data
public: //	Public class members (must be encapsulated further)
    struct
    {
        u32 bLoaded : 1;
        u32 bUser : 1;
        u32 seqCycles : 1;
        u32 MemoryUsage : 28;
    } flags;

    fastdelegate::FastDelegate2<CBackend&,u32> bind;

    CAviPlayerCustom* pAVI;
    CTheoraSurface* pTheora;
    float m_material;
    shared_str m_bumpmap;

    union
    {
        u32 m_play_time; // sync theora time
        u32 seqMSPF; // Sequence data milliseconds per frame
    };

    int curr_slice{ -1 };
    int last_slice{ -1 };

private:
#if defined(USE_DX11)
    ID3DBaseTexture* pSurface{};
    ID3DBaseTexture* pTempSurface{};
    // Sequence data
    xr_vector<ID3DBaseTexture*> seqDATA;

    // Description
    u32 m_width;
    u32 m_height;
    ID3DBaseTexture* desc_cache;
    D3D_TEXTURE2D_DESC desc;
#elif defined(USE_OGL)
    GLuint pSurface;
    GLuint pBuffer;
    // Sequence data
    xr_vector<GLuint> seqDATA;
    // Description
    GLint m_width;
    GLint m_height;
    GLuint desc_cache;
    GLenum desc;
#else
#   error No graphics API selected or enabled!
#endif

#if defined(USE_DX11)
    ID3DShaderResourceView* m_pSRView{ nullptr };
    ID3DShaderResourceView* srv_all{ nullptr };
    xr_vector<ID3DShaderResourceView*> srv_per_slice;
    // Sequence view data
    xr_vector<ID3DShaderResourceView*> m_seqSRView;
#endif
};

struct resptrcode_texture : public resptr_base<CTexture>
{
    void create(LPCSTR _name);
    void destroy() { _set(nullptr); }
    shared_str bump_get() { return _get()->m_bumpmap; }
    bool bump_exist() { return 0 != bump_get().size(); }
};

typedef resptr_core<CTexture, resptrcode_texture> ref_texture;

#endif
