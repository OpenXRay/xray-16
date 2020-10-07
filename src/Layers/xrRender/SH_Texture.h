#ifndef SH_TEXTURE_H
#define SH_TEXTURE_H
#pragma once

#include "xrCore/xr_resource.h"

class ENGINE_API CAviPlayerCustom;
class CTheoraSurface;

class ECORE_API CTexture : public xr_resource_named
{
public:
#ifndef USE_DX9
    enum	MaxTextures
    {
        //	Actually these values are 128
        mtMaxPixelShaderTextures = 16,
        mtMaxVertexShaderTextures = 4,
        mtMaxGeometryShaderTextures = 16,
#	ifdef USE_DX11
        mtMaxHullShaderTextures = 16,
        mtMaxDomainShaderTextures = 16,
        mtMaxComputeShaderTextures = 16,
#	endif
        mtMaxCombinedShaderTextures =
        mtMaxPixelShaderTextures
        + mtMaxVertexShaderTextures
        + mtMaxGeometryShaderTextures
#	ifdef USE_DX11
        + mtMaxHullShaderTextures
        + mtMaxDomainShaderTextures
        + mtMaxComputeShaderTextures
#	endif
    };
#else // USE_DX9
    enum MaxTextures
    {
        mtMaxPixelShaderTextures = 16,
        mtMaxVertexShaderTextures = 4,
        mtMaxCombinedShaderTextures =
        mtMaxPixelShaderTextures
        + mtMaxVertexShaderTextures
    };
#endif // !USE_DX9

#ifdef USE_OGL
    //	Since OGL doesn't differentiate between stages,
    //	distance between enum values should be the max for that stage.
    enum ResourceShaderType
    {
        rstPixel = 0,	//	Default texture offset
        rstVertex = rstPixel + mtMaxPixelShaderTextures,
        rstGeometry = rstVertex + mtMaxVertexShaderTextures,
    };
#else
    //	Since DX10 allows up to 128 unique textures,
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
#endif // USE_OGL

public:
    void __stdcall apply_load(u32 stage);
    void __stdcall apply_theora(u32 stage);
    void __stdcall apply_avi(u32 stage);
    void __stdcall apply_seq(u32 stage);
    void __stdcall apply_normal(u32 stage);

    void Preload();
    void Load();
    void PostLoad();
    void Unload(void);
    // void Apply(u32 dwStage);

#ifdef USE_OGL
    void surface_set(GLenum target, GLuint surf);
    GLuint surface_get();
#else
    void surface_set(ID3DBaseTexture* surf);
    ID3DBaseTexture* surface_get();
#endif // USE_OGL


    BOOL isUser() { return flags.bUser; }

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
    void video_Pause(BOOL state);
    void video_Stop();
    BOOL video_IsPlaying();

    CTexture();
    virtual ~CTexture();

#if !defined(USE_DX9) && !defined(USE_OGL)
    ID3DShaderResourceView* get_SRView() { return m_pSRView; }
#endif

private:
    BOOL desc_valid() { return pSurface == desc_cache; }

    void desc_enshure()
    {
        if (!desc_valid())
            desc_update();
    }

    void desc_update();
#if !defined(USE_DX9) && !defined(USE_OGL)
    void Apply(u32 dwStage);
    void ProcessStaging();
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
#if !defined(USE_DX9) && !defined(USE_OGL)
        u32 bLoadedAsStaging : 1;
#endif
    } flags;

    fastdelegate::FastDelegate1<u32> bind;

    CAviPlayerCustom* pAVI;
    CTheoraSurface* pTheora;
    float m_material;
    shared_str m_bumpmap;

    union
    {
        u32 m_play_time; // sync theora time
        u32 seqMSPF; // Sequence data milliseconds per frame
    };

private:
#ifdef USE_OGL
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
    ID3DBaseTexture* pSurface;
    // Sequence data
    xr_vector<ID3DBaseTexture*> seqDATA;

    // Description
    u32 m_width;
    u32 m_height;
    ID3DBaseTexture* desc_cache;
    D3D_TEXTURE2D_DESC desc;
#endif // USE_OGL

#if !defined(USE_DX9) && !defined(USE_OGL)
    ID3DShaderResourceView* m_pSRView;
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
