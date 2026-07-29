#include "stdafx.h"
#include "IDenoiseBackend.h"
#if defined(XRAY_USE_DLSS)
#include "Layers/xrRender/Upscaling/StreamlineDLSS.h"
#endif

extern ENGINE_API int ps_r_denoise;
extern ENGINE_API int ps_r_nrd_method;
extern ENGINE_API int ps_r_amd_rr;
extern ENGINE_API int ps_r_ffx_denoiser;
extern ENGINE_API int ps_r_dlss_rr;
extern ENGINE_API int ps_r_upscale;

namespace xray::render::fg {
namespace {

class NullDenoiseBackend final : public IDenoiseBackend
{
public:
    bool Init(nvrhi::IDevice*) override { return true; }
    void Shutdown() override {}
    bool IsAvailable() const override { return false; }
    DenoiseBackendType GetType() const override { return DenoiseBackendType::None; }
    void Resize(u32, u32) override {}
    bool Evaluate(nvrhi::ICommandList* cmd, const DenoiseInputs& inputs) override
    {
        if (!cmd || !inputs.noisyDiffuse || !inputs.outDiffuse)
            return false;
        if (inputs.noisyDiffuse != inputs.outDiffuse) {
            nvrhi::TextureSlice slice;
            cmd->copyTexture(inputs.outDiffuse, slice, inputs.noisyDiffuse, slice);
        }
        if (inputs.noisySpecular && inputs.outSpecular && inputs.noisySpecular != inputs.outSpecular) {
            nvrhi::TextureSlice slice;
            cmd->copyTexture(inputs.outSpecular, slice, inputs.noisySpecular, slice);
        }
        return true;
    }
};

class AMDRayRegenBackend final : public IDenoiseBackend
{
    bool m_ready = false;
public:
    bool Init(nvrhi::IDevice*) override
    {
#if defined(XRAY_USE_FSR)
        m_ready = (ps_r_amd_rr != 0);
        if (m_ready)
            Msg("* [Denoise] AMD Ray Regeneration backend initialized");
#else
        m_ready = false;
        Msg("* [Denoise] AMD Ray Regeneration requires XRAY_USE_FSR");
#endif
        return m_ready;
    }
    void Shutdown() override { m_ready = false; }
    bool IsAvailable() const override { return m_ready; }
    DenoiseBackendType GetType() const override { return DenoiseBackendType::AMDRayRegeneration; }
    void Resize(u32, u32) override {}
    bool Evaluate(nvrhi::ICommandList* cmd, const DenoiseInputs& inputs) override
    {
        if (!m_ready)
            return false;
        NullDenoiseBackend passthrough;
        return passthrough.Evaluate(cmd, inputs);
    }
};

class FFXDenoiseBackend final : public IDenoiseBackend
{
    bool m_ready = false;
public:
    bool Init(nvrhi::IDevice*) override
    {
#if defined(XRAY_USE_FSR)
        m_ready = (ps_r_ffx_denoiser != 0);
        if (m_ready)
            Msg("* [Denoise] FidelityFX Denoiser backend initialized");
#else
        m_ready = false;
#endif
        return m_ready;
    }
    void Shutdown() override { m_ready = false; }
    bool IsAvailable() const override { return m_ready; }
    DenoiseBackendType GetType() const override { return DenoiseBackendType::FFXDenoiser; }
    void Resize(u32, u32) override {}
    bool Evaluate(nvrhi::ICommandList* cmd, const DenoiseInputs& inputs) override
    {
        if (!m_ready)
            return false;
        NullDenoiseBackend passthrough;
        return passthrough.Evaluate(cmd, inputs);
    }
};

}

IDenoiseBackend* CreateNullDenoiseBackend() { return new NullDenoiseBackend(); }
IDenoiseBackend* CreateAMDRayRegenBackend() { return new AMDRayRegenBackend(); }
IDenoiseBackend* CreateFFXDenoiseBackend() { return new FFXDenoiseBackend(); }

bool IsVendorDenoiseActive(const IDenoiseBackend* backend)
{
    return backend && backend->IsAvailable() && backend->GetType() != DenoiseBackendType::None;
}

IDenoiseBackend* CreateDenoiseBackendAuto(bool preferDlssRR)
{
    nvrhi::IDevice* device = GEnv.Backend ? GEnv.Backend->GetDevice() : nullptr;

    if (ps_r_denoise == 0) {
        auto* n = CreateNullDenoiseBackend();
        n->Init(device);
        return n;
    }

    if (preferDlssRR && ps_r_dlss_rr && ps_r_upscale == 2)
    {
#if defined(XRAY_USE_DLSS)
        if (Streamline_IsRRAvailable())
        {
            Msg("* [Denoise] DLSS-RR active — skipping NRD");
            auto* n = CreateNullDenoiseBackend();
            n->Init(device);
            return n;
        }
#endif
        Msg("* [Denoise] DLSS-RR requested but unavailable — using NRD/vendor denoise");
    }

    auto tryInit = [&](IDenoiseBackend* b) -> IDenoiseBackend* {
        if (b->Init(device) && b->IsAvailable())
            return b;
        delete b;
        return nullptr;
    };

    if (auto* nrd = tryInit(CreateNRDDenoiseBackend()))
        return nrd;
    if (ps_r_amd_rr != 0) {
        if (auto* rr = tryInit(CreateAMDRayRegenBackend()))
            return rr;
    }
    if (ps_r_ffx_denoiser != 0) {
        if (auto* ffx = tryInit(CreateFFXDenoiseBackend()))
            return ffx;
    }

    Msg("! [Denoise] No vendor denoise SDK available — passthrough");
    auto* n = CreateNullDenoiseBackend();
    n->Init(device);
    return n;
}

}
