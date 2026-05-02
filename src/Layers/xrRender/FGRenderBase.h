#pragma once

#include "xrEngine/Render.h"
#include "Layers/xrRender/xr_effgamma.h"

class CResourceManager;

namespace xray::render::fg
{
class FGRenderBase : public IRender, public pureFrame
{
public:
    RenderStatistics BasicStats;

public:
    void setGamma(float fGamma) override;
    void setBrightness(float fGamma) override;
    void setContrast(float fGamma) override;
    void updateGamma() override;

    void OnDeviceDestroy(bool bKeepTextures) override;
    void Destroy() override;
    void Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) override;

    void ObtainRequiredWindowFlags(u32& windowFlags) override;
    void SetupStates() override;
    void OnDeviceCreate(pcstr shName) override;
    void Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2) override;

    void overdrawBegin() override {}
    void overdrawEnd() override {}

    void DeferredLoad(bool E) override;
    void ResourcesDeferredUpload() override;
    void ResourcesDeferredUnload() override;
    void ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps) override;
    void ResourcesDestroyNecessaryTextures() override;
    void ResourcesStoreNecessaryTextures() override;
    void ResourcesDumpMemoryUsage() override;

    bool HWSupportsShaderYUV2RGB() override { return false; }

    DeviceState GetDeviceState() override;
    bool GetForceGPU_REF() override;
    u32 GetCacheStatPolys() override;
    void Begin() override;
    void Clear() override {}
    void End() override;
    void ClearTarget() override {}
    void SetCacheXform(Fmatrix& mView, Fmatrix& mProject) override {}
    void OnAssetsChanged() override;
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    xrImTextureData GetImGuiTextureId(pcstr texture_name) override;

    void ConvertLegacyAssetsToPBR() override;

    RenderContext GetCurrentContext() const override { return IRender::PrimaryContext; }
    void MakeContextCurrent(RenderContext) override {}

public:
    CResourceManager* Resources{ nullptr };

protected:
    bool b_loaded{ false };

private:
    CGammaControl m_Gamma;
};
}
