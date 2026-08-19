#pragma once

#include <nvrhi/nvrhi.h>
#include "xrCore/xrCore.h"

namespace xray::render::fg {

static constexpr u32 RESTIR_MAX_LIGHTS = 2048;
static constexpr u32 RESTIR_RESERVOIR_STRIDE = 16;

struct ReSTIRPackedReservoir
{
    u32 data[4];
};
static_assert(sizeof(ReSTIRPackedReservoir) == RESTIR_RESERVOIR_STRIDE, "Packed reservoir must be 16 bytes");

class ReSTIRMemoryManager
{
public:
    static ReSTIRMemoryManager& Instance();

    void Init(nvrhi::IDevice* device);
    void Shutdown();

    void Ensure(u32 width, u32 height);
    void DestroyResources();

    nvrhi::IBuffer* GetReservoirBuffer(u32 index) const;
    nvrhi::ITexture* GetDirectLighting() const { return m_directLighting; }
    nvrhi::ITexture* GetNoisyDiffuse() const { return m_noisyDiffuse; }
    nvrhi::ITexture* GetNoisySpecular() const { return m_noisySpecular; }
    nvrhi::ITexture* GetHitDistance() const { return m_hitDistance; }
    nvrhi::ITexture* GetWetAccum() const { return m_wetAccum; }
    nvrhi::ITexture* GetSkyOpen() const { return m_skyOpen; }
    nvrhi::ITexture* GetSunshafts() const { return m_sunshafts; }
    nvrhi::ITexture* GetSunshaftsHist() const { return m_sunshaftsHist; }
    u32 GetShaftWidth() const { return m_shaftWidth; }
    u32 GetShaftHeight() const { return m_shaftHeight; }
    u32 GetGiWidth() const { return m_giWidth ? m_giWidth : m_width; }
    u32 GetGiHeight() const { return m_giHeight ? m_giHeight : m_height; }
    u32 GetFullWidth() const { return m_width; }
    u32 GetFullHeight() const { return m_height; }
    nvrhi::ITexture* GetDdgiAmbient() const { return m_ddgiAmbient; }
    nvrhi::ITexture* GetDdgiProbes() const { return m_ddgiProbes; }
    nvrhi::IBuffer* GetLightDataBuffer() const { return m_lightData; }
    nvrhi::IBuffer* GetPlaceholderBuffer() const { return m_placeholderBuffer; }
    nvrhi::ITexture* GetPlaceholderCube() const { return m_placeholderCube; }
    nvrhi::ITexture* GetPlaceholderTex() const { return m_placeholderTex; }
    nvrhi::ITexture* GetPlaceholderTex3D() const { return m_placeholderTex3D; }
    nvrhi::ITexture* GetPlaceholderColorTex() const { return m_placeholderColorTex; }
    nvrhi::ITexture* GetDIReservoir(u32 index) const { return m_diReservoir[index & 1]; }
    nvrhi::ITexture* GetSpecReservoirA(u32 index) const { return m_specReservoirA[index & 1]; }
    nvrhi::ITexture* GetSpecReservoirB(u32 index) const { return m_specReservoirB[index & 1]; }
    nvrhi::ITexture* GetBlurTemp() const { return m_blurTemp; }
    nvrhi::ITexture* GetBlurTempSpec() const { return m_blurTempSpec; }
    nvrhi::ITexture* GetHistDiffuse() const { return m_histDiffuse; }
    nvrhi::ITexture* GetHistSpecular() const { return m_histSpecular; }
    nvrhi::ITexture* GetBlueNoise() const { return m_blueNoise; }
    nvrhi::ITexture* GetSunVis(u32 index) const { return m_sunVis[index & 1]; }
    nvrhi::ITexture* GetPTReservoirA(u32 index) const { return m_ptReservoirA[index & 1]; }
    nvrhi::ITexture* GetPTReservoirB(u32 index) const { return m_ptReservoirB[index & 1]; }
    nvrhi::ITexture* GetPTDupMap() const { return m_ptDupMap; }
    nvrhi::ITexture* GetPairingTex(u32 index) const { return m_pairingTex[index % 3]; }
    void EnsureBlueNoiseUploaded(nvrhi::ICommandList* cmd);
    nvrhi::IBuffer* GetIrradianceCache() const { return m_irradianceCache; }
    u32 GetIrradianceCacheSize() const { return m_irradianceCacheSize; }
    void EnsureIrradianceCache(u32 entries);
    void RequestHistoryReset();
    bool ConsumeHistoryReset();
    void ClearHistoryTargets(nvrhi::ICommandList* cmd);

    nvrhi::BindingSetHandle& TemporalBindingSet(u32 idx) { return m_temporalBS[idx & 1]; }
    nvrhi::BindingSetHandle& SpatialBindingSet(u32 idx) { return m_spatialBS[idx & 1]; }
    void ClearBindingSets();

    u32 GetWidth() const { return m_width; }
    u32 GetHeight() const { return m_height; }
    u32 GetPixelCount() const { return GetGiWidth() * GetGiHeight(); }
    bool IsReady() const { return m_reservoir[0] != nullptr; }

    void UploadLights(nvrhi::ICommandList* cmd, const void* lights, u32 lightCount, u32 strideBytes);

    static constexpr u32 WorkIndex() { return 0; }
    static constexpr u32 HistoryIndex() { return 1; }

private:
    void CreatePersistent(u32 width, u32 height);

    nvrhi::DeviceHandle m_device;
    nvrhi::BufferHandle m_reservoir[2];
    nvrhi::TextureHandle m_directLighting;
    nvrhi::TextureHandle m_noisyDiffuse;
    nvrhi::TextureHandle m_noisySpecular;
    nvrhi::TextureHandle m_hitDistance;
    nvrhi::TextureHandle m_wetAccum;
    nvrhi::TextureHandle m_skyOpen;
    nvrhi::TextureHandle m_sunshafts;
    nvrhi::TextureHandle m_sunshaftsHist;
    nvrhi::TextureHandle m_ddgiAmbient;
    nvrhi::TextureHandle m_ddgiProbes;
    nvrhi::BufferHandle m_lightData;
    nvrhi::BufferHandle m_placeholderBuffer;
    nvrhi::TextureHandle m_placeholderCube;
    nvrhi::TextureHandle m_placeholderTex;
    nvrhi::TextureHandle m_placeholderColorTex;
    nvrhi::TextureHandle m_placeholderTex3D;
    nvrhi::TextureHandle m_diReservoir[2];
    nvrhi::TextureHandle m_specReservoirA[2];
    nvrhi::TextureHandle m_specReservoirB[2];
    nvrhi::TextureHandle m_blurTemp;
    nvrhi::TextureHandle m_blurTempSpec;
    nvrhi::TextureHandle m_histDiffuse;
    nvrhi::TextureHandle m_histSpecular;
    nvrhi::TextureHandle m_blueNoise;
    nvrhi::TextureHandle m_sunVis[2];
    nvrhi::TextureHandle m_ptReservoirA[2];
    nvrhi::TextureHandle m_ptReservoirB[2];
    nvrhi::TextureHandle m_ptDupMap;
    nvrhi::TextureHandle m_pairingTex[3];
    xr_vector<u8> m_blueNoisePixels;
    bool m_blueNoiseUploaded = false;
    nvrhi::BufferHandle m_irradianceCache;
    u32 m_irradianceCacheSize = 0;
    nvrhi::BindingSetHandle m_temporalBS[2];
    nvrhi::BindingSetHandle m_spatialBS[2];
    u32 m_width = 0;
    u32 m_height = 0;
    u32 m_giWidth = 0;
    u32 m_giHeight = 0;
    u32 m_shaftWidth = 0;
    u32 m_shaftHeight = 0;
    bool m_resetHistory = false;
};

}
