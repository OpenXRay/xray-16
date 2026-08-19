#include "stdafx.h"
#include "IDenoiseBackend.h"
#if defined(XRAY_USE_DLSS)
#include "Layers/xrRender/Upscaling/StreamlineDLSS.h"
#endif

extern ENGINE_API int ps_r_denoise;
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

}

IDenoiseBackend* CreateNullDenoiseBackend() { return new NullDenoiseBackend(); }

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
        Msg("* [Denoise] DLSS-RR requested but unavailable — using NRD");
    }

    auto tryInit = [&](IDenoiseBackend* b) -> IDenoiseBackend* {
        if (b->Init(device) && b->IsAvailable())
            return b;
        delete b;
        return nullptr;
    };

    if (auto* nrd = tryInit(CreateNRDDenoiseBackend()))
        return nrd;

    Msg("! [Denoise] No vendor denoise SDK available — passthrough");
    auto* n = CreateNullDenoiseBackend();
    n->Init(device);
    return n;
}

}
