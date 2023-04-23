#pragma once

#include "xrEngine/Render.h"
#include "xrCDB/ISpatial.h"
#include "r__dsgraph_types.h"
#include "r__dsgraph_structure.h"
#include "r__sector.h"
#include "xr_effgamma.h"


// Common part of interface implementation for all D3D renderers
class D3DXRenderBase : public IRender, public pureFrame
{
public:
    //friend class CSkeletonX; // Stats.Skinning
    //friend class CKinematics; // Stats.Animation
    RenderStatistics BasicStats;

public:
    //	Gamma correction functions
    virtual void setGamma(float fGamma) override;
    virtual void setBrightness(float fGamma) override;
    virtual void setContrast(float fGamma) override;
    virtual void updateGamma() override;

    //	Destroy
    virtual void OnDeviceDestroy(bool bKeepTextures) override;
    virtual void Destroy() override;
    virtual void Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) override;

    //	Init
    virtual void ObtainRequiredWindowFlags(u32& /*windowFlags*/) override;
    virtual void SetupStates() override;
    virtual void OnDeviceCreate(const char* shName) override;
    virtual void Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) override;

    //	Overdraw
    virtual void overdrawBegin() override;
    virtual void overdrawEnd() override;
    //	Resources control
    virtual void DeferredLoad(bool E) override;
    virtual void ResourcesDeferredUpload() override;
    virtual void ResourcesDeferredUnload() override;
    virtual void ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps) override;
    virtual void ResourcesDestroyNecessaryTextures() override;
    virtual void ResourcesStoreNecessaryTextures() override;
    virtual void ResourcesDumpMemoryUsage() override;
    //	HWSupport
    virtual bool HWSupportsShaderYUV2RGB() override;
    //	Device state
    virtual DeviceState GetDeviceState() override;
    virtual bool GetForceGPU_REF() override;
    virtual u32 GetCacheStatPolys() override;
    virtual void Begin() override;
    virtual void Clear() override;
    virtual void End() override;
    virtual void ClearTarget() override;
    virtual void SetCacheXform(Fmatrix& mView, Fmatrix& mProject) override;
    virtual void OnAssetsChanged() override;
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    RenderContext GetCurrentContext() const override { return IRender::PrimaryContext; }
    void MakeContextCurrent(RenderContext /*context*/) override {}

    enum
    {
        eRDSG_MAIN, // shadred with forward
#if 0//RENDER != R_R1
        eRDSG_RAIN,
        eRDSG_SHADOW_0, // cascade#0 or shadowed light use
        eRDSG_SHADOW_1, // cascade#1 or shadowed light use
        eRDSG_SHADOW_2, // cascade#2 or shadowed light use
        //eRDSG_AUX_0..N if not engough
#endif
        eRDSG_NUM_CONTEXTS
    };

    // Lifetime tracking helpers
    ICF R_dsgraph_structure& alloc_context(u32 context_id)
    {
        VERIFY(context_id < eRDSG_NUM_CONTEXTS);
        VERIFY(dsgraph_pool[context_id].second == false);
        VERIFY(dsgraph_pool[context_id].first.context_id == R_dsgraph_structure::INVALID_CONTEXT_ID);
        dsgraph_pool[context_id].first.context_id = context_id;
        dsgraph_pool[context_id].second = true;
        return dsgraph_pool[context_id].first;
    }
    
    ICF void release_context(u32 context_id)
    {
        VERIFY(context_id < eRDSG_NUM_CONTEXTS);
        VERIFY(dsgraph_pool[context_id].second == true);
        VERIFY(dsgraph_pool[context_id].first.context_id != R_dsgraph_structure::INVALID_CONTEXT_ID);
        dsgraph_pool[context_id].first.reset();
        dsgraph_pool[context_id].second = false;
    }

    ICF void cleanup_contexts()
    {
        for (int context_id = 0; context_id < eRDSG_NUM_CONTEXTS; ++context_id)
        {
            dsgraph_pool[context_id].first.reset();
            dsgraph_pool[context_id].second = false;
            dsgraph_pool[context_id].first.context_id = R_dsgraph_structure::INVALID_CONTEXT_ID; // tmp
        }
    }

    ICF R_dsgraph_structure& get_context(u32 context_id)
    {
        VERIFY(context_id < eRDSG_NUM_CONTEXTS);
        VERIFY(dsgraph_pool[context_id].second == true);
        return dsgraph_pool[context_id].first;
    }

public:
    CResourceManager* Resources{};
    ref_shader m_WireShader;
    ref_shader m_SelectionShader;
    ref_shader m_PortalFadeShader;
    ref_geom   m_PortalFadeGeom;

protected:
    std::pair<R_dsgraph_structure, bool> dsgraph_pool[eRDSG_NUM_CONTEXTS];
private:
#if defined(USE_DX9) || defined(USE_DX11)
    CGammaControl m_Gamma;
#endif

protected:
    bool b_loaded{};
};
