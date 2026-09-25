// TextureManager.h: interface for the CTextureManager class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/TextureDescrManager.h"

#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
// defs
class ECORE_API CResourceManager
{
private:
    struct str_pred
    {
        bool operator()(LPCSTR x, LPCSTR y) const { return xr_strcmp(x, y) < 0; }
    };
    struct texture_detail
    {
        const char* T;
    };

public:
    using map_Blender = xr_map<const char*, IBlender*, str_pred>;
    using map_Texture = xr_map<const char*, CTexture*, str_pred>;
    //	DX10 cut DEFINE_MAP_PRED(const char*,CRTC*,			map_RTC,		map_RTCIt,			str_pred);
    using map_VS = xr_map<const char*, SVS*, str_pred>;
    using map_GS = xr_map<const char*, SGS*, str_pred>;
    using map_HS = xr_map<const char*, SHS*, str_pred>;
    using map_DS = xr_map<const char*, SDS*, str_pred>;
    using map_CS = xr_map<const char*, SCS*, str_pred>;
#if defined(USE_OGL)
    using map_PP = xr_map<const char*, SPP*, str_pred>;
#endif
    using map_PS = xr_map<const char*, SPS*, str_pred>;
    using map_TD = xr_map<const char*, texture_detail, str_pred>;

private:
    // data
    map_Blender m_blenders;
    map_Texture m_textures;
    //	DX10 cut map_RTC												m_rtargets_c;
    map_VS m_vs;
    map_PS m_ps;
    map_GS m_gs;
    map_DS m_ds;
    map_HS m_hs;
    map_CS m_cs;
#if defined(USE_OGL)
    map_PP m_pp;
#endif

    map_TD m_td;

    xr_vector<SDeclaration*> v_declarations;
    xr_vector<SGeometry*> v_geoms;

    xr_vector<ref_texture> m_necessary;
public:
    BOOL bDeferredLoad;
    bool m_shader_fallback_allowed;

    // Miscelaneous
    void _ParseList(sh_list& dest, LPCSTR names);
    IBlender* _FindBlender(LPCSTR Name);
    void _GetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps);
    void _DumpMemoryUsage();

    // Blender property extraction for D3D12/Framegraph renderer
    enum class BlendMode : u8 {
        Opaque,         // No blending, full depth write
        AlphaTest,      // Alpha test only, full depth write
        AlphaBlend,     // Standard alpha blend (srcAlpha, invSrcAlpha)
        Additive,       // Additive blend (one, one) or (srcAlpha, one)
        Multiply,       // Multiply blend (destColor, zero)
        Multiply2X,     // Double multiply (destColor, srcColor)
    };
    struct BlenderProperties {
        BlendMode blendMode = BlendMode::Opaque;
        u32 alphaRef = 0;           // 0-255 alpha threshold (for AlphaTest)
        bool writesDepth = true;    // Whether this shader writes to depth buffer
        bool strictB2F = false;     // Requires back-to-front sorting (from oStrictSorting)
        bool foliage = false;
    };
    bool GetBlenderProperties(LPCSTR blenderName, BlenderProperties& outProps);
    //.	BOOL							_GetDetailTexture	(LPCSTR Name, LPCSTR& T, R_constant_setup* &M);

    // Debug
    void DBG_VerifyGeoms();
    void DBG_VerifyTextures();

    // Editor cooperation
    void ED_UpdateBlender(LPCSTR Name, IBlender* data);
#ifdef _EDITOR
    void ED_UpdateTextures(AStringVec* names);
#endif

    // Low level resource creation
    CTexture* _CreateTexture(LPCSTR Name);
    void _DeleteTexture(const CTexture* T);

//	DX10 cut CRTC*							_CreateRTC			(LPCSTR Name, u32 size,	D3DFORMAT f);
//	DX10 cut void							_DeleteRTC			(const CRTC*	RT	);

#if defined(USE_OGL)
    SPP* _CreatePP(pcstr vs, pcstr ps, pcstr gs, pcstr hs, pcstr ds);
    void _DeletePP(const SPP* p);
#endif

    void _DeleteGS(const SGS* GS);

    void _DeleteHS(const SHS* HS);

    void _DeleteDS(const SDS* DS);

    void _DeleteCS(const SCS* CS);

    void _DeletePS(const SPS* PS);

    void _DeleteVS(const SVS* VS);

    SDeclaration* _CreateDecl(const VertexElement* dcl);
    void _DeleteDecl(const SDeclaration* dcl);

    CResourceManager() : bDeferredLoad(TRUE)
    {
#if RENDER == R_R1 || RENDER == R_R2
        m_shader_fallback_allowed = !!strstr(Core.Params, "-lack_of_shaders");
#else // For another renderers we should always allow fallback
        m_shader_fallback_allowed = true;
#endif
    }

    ~CResourceManager();

    void OnDeviceCreate(IReader* F);
    void OnDeviceCreate(LPCSTR name);
    void OnDeviceDestroy(BOOL bKeepTextures);

    void reset_begin();
    void reset_end();

    void CompatibilityCheck();

    SGeometry* CreateGeom(const VertexElement* decl, VertexBufferHandle vb, IndexBufferHandle ib);
    SGeometry* CreateGeom(u32 FVF, VertexBufferHandle vb, IndexBufferHandle ib);
    SGeometry* CreateLogicalGeom(const VertexElement* decl);

    void DeleteGeom(const SGeometry* VS);
    void DiscardGeometryBuffer(nvrhi::IBuffer* buffer);
    void DeferredLoad(BOOL E) { bDeferredLoad = E; }
    void DeferredUpload();
    void DeferredUnload();
    void UnloadTextures();
    void Evict();
    void StoreNecessaryTextures();
    void DestroyNecessaryTextures();
    void Dump(bool bBrief);

private:
    template <typename T>
    bool reclaim(xr_vector<T*>& vec, const T* ptr)
    {
        auto it = vec.begin();
        const auto end = vec.cend();

        for (; it != end; ++it)
        {
            if (*it == ptr)
            {
                vec.erase(it);
                return true;
            }
        }
        return false;
    }
};
} // namespace xray::render::fg
