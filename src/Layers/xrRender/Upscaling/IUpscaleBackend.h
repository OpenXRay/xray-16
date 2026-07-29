#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::fg {

enum class UpscaleBackendType : u32
{
    None = 0,
    FSR,
    DLSS
};

enum class UpscaleQuality : u32
{
    UltraPerformance = 0,
    Performance,
    Balanced,
    Quality,
    Native,
    DLAA
};

struct UpscaleInputs
{
    nvrhi::ITexture* color = nullptr;
    nvrhi::ITexture* depth = nullptr;
    nvrhi::ITexture* motionVectors = nullptr;
    nvrhi::ITexture* exposure = nullptr;
    nvrhi::ITexture* reactiveMask = nullptr;
    nvrhi::ITexture* output = nullptr;
    nvrhi::ITexture* normals = nullptr;
    nvrhi::ITexture* diffuseAlbedo = nullptr;
    nvrhi::ITexture* specularAlbedo = nullptr;
    nvrhi::ITexture* roughness = nullptr;
    nvrhi::ITexture* diffuseHitDistance = nullptr;
    nvrhi::ITexture* specularHitDistance = nullptr;
    nvrhi::ITexture* noisyDiffuse = nullptr;
    nvrhi::ITexture* noisySpecular = nullptr;
    float worldToView[16]{};
    float viewToClip[16]{};
    u32 renderWidth = 0;
    u32 renderHeight = 0;
    u32 displayWidth = 0;
    u32 displayHeight = 0;
    float jitterX = 0.f;
    float jitterY = 0.f;
    float sharpness = 0.f;
    float frameTimeMs = 16.f;
    u32 frameIndex = 0;
    bool reset = false;
    bool enableFG = false;
    bool enableRR = false;
};

class IUpscaleBackend
{
public:
    virtual ~IUpscaleBackend() = default;
    virtual bool Init(nvrhi::IDevice* device) = 0;
    virtual void Shutdown() = 0;
    virtual bool IsAvailable() const = 0;
    virtual UpscaleBackendType GetType() const = 0;
    virtual bool SupportsFG() const { return false; }
    virtual bool SupportsRR() const { return false; }
    virtual bool Evaluate(nvrhi::ICommandList* cmd, const UpscaleInputs& inputs) = 0;
};

IUpscaleBackend* CreateNullUpscaleBackend();
IUpscaleBackend* CreateFSRUpscaleBackend();
IUpscaleBackend* CreateDLSSUpscaleBackend();
IUpscaleBackend* CreateUpscaleBackendAuto();

}
