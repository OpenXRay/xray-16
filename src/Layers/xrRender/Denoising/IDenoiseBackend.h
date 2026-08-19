#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::fg {

enum class DenoiseBackendType : u32
{
    None = 0,
    NRD,
    DLSSRayReconstruction
};

struct DenoiseInputs
{
    nvrhi::ITexture* noisyDiffuse = nullptr;
    nvrhi::ITexture* noisySpecular = nullptr;
    nvrhi::ITexture* hitDistance = nullptr;
    nvrhi::ITexture* shadowMask = nullptr;
    nvrhi::ITexture* normals = nullptr;
    nvrhi::ITexture* roughness = nullptr;
    nvrhi::ITexture* depth = nullptr;
    nvrhi::ITexture* worldPos = nullptr;
    nvrhi::ITexture* baseColor = nullptr;
    nvrhi::ITexture* classifyWorldPos = nullptr;
    nvrhi::ITexture* motionVectors = nullptr;
    nvrhi::ITexture* directLighting = nullptr;
    nvrhi::ITexture* sceneColorIn = nullptr;
    nvrhi::ITexture* outDiffuse = nullptr;
    nvrhi::ITexture* outSpecular = nullptr;
    nvrhi::ITexture* outSceneColor = nullptr;
    u32 width = 0;
    u32 height = 0;
    float jitterX = 0.f;
    float jitterY = 0.f;
    float jitterPrevX = 0.f;
    float jitterPrevY = 0.f;
    float exposure = 1.f;
    float nearZ = 0.001f;
    float farZ = 500.f;
    float viewToClip[16] = {};
    float viewToClipPrev[16] = {};
    float worldToView[16] = {};
    float worldToViewPrev[16] = {};
    u32 frameIndex = 0;
    bool reset = false;
};

class IDenoiseBackend
{
public:
    virtual ~IDenoiseBackend() = default;
    virtual bool Init(nvrhi::IDevice* device) = 0;
    virtual void Shutdown() = 0;
    virtual bool IsAvailable() const = 0;
    virtual DenoiseBackendType GetType() const = 0;
    virtual void Resize(u32 width, u32 height) = 0;
    virtual bool Evaluate(nvrhi::ICommandList* cmd, const DenoiseInputs& inputs) = 0;
};

IDenoiseBackend* CreateNullDenoiseBackend();
IDenoiseBackend* CreateNRDDenoiseBackend();
IDenoiseBackend* CreateDenoiseBackendAuto(bool preferDlssRR);

bool IsVendorDenoiseActive(const IDenoiseBackend* backend);

}
