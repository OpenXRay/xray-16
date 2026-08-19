#include "stdafx.h"
#include "IUpscaleBackend.h"
#include "StreamlineDLSS.h"
#include "Layers/xrRender/Backend/VulkanBackend.h"

extern ENGINE_API int ps_r_upscale;
extern ENGINE_API int ps_r_dlss;
extern ENGINE_API int ps_r_dlss_fg;
extern ENGINE_API int ps_r_dlss_rr;

namespace xray::render::fg {
namespace {

class NullUpscaleBackend final : public IUpscaleBackend
{
public:
    bool Init(nvrhi::IDevice*) override { return true; }
    void Shutdown() override {}
    bool IsAvailable() const override { return false; }
    UpscaleBackendType GetType() const override { return UpscaleBackendType::None; }
    bool Evaluate(nvrhi::ICommandList*, const UpscaleInputs&) override { return false; }
};

class DLSSUpscaleBackend final : public IUpscaleBackend
{
    bool m_ready = false;
public:
    bool Init(nvrhi::IDevice*) override
    {
#if defined(XRAY_USE_DLSS)
        auto* vk = dynamic_cast<VulkanBackend*>(GEnv.Backend);
        if (!vk)
        {
            Msg("! [Upscale] DLSS requires Vulkan backend");
            return false;
        }
        if (Streamline_IsDLSSAvailable())
        {
            m_ready = true;
        }
        else
        {
            StreamlineVulkanInfo info{};
            info.instance = vk->GetVkInstance();
            info.physicalDevice = vk->GetVkPhysicalDevice();
            info.device = vk->GetVkDevice();
            info.graphicsQueueFamily = vk->GetGraphicsQueueFamily();
            m_ready = Streamline_SetVulkanInfo(info);
        }
        if (m_ready)
            Msg("* [Upscale] DLSS/NGX backend ready (fg=%d rr=%d)",
                Streamline_IsFGAvailable() ? 1 : 0,
                Streamline_IsRRAvailable() ? 1 : 0);
        else
            Msg("! [Upscale] DLSS/NGX unavailable");
#else
        m_ready = false;
        Msg("* [Upscale] DLSS/NGX SDK not compiled in (XRAY_USE_DLSS)");
#endif
        return m_ready;
    }
    void Shutdown() override
    {
#if defined(XRAY_USE_DLSS)
        Streamline_Shutdown();
#endif
        m_ready = false;
    }
    bool IsAvailable() const override { return m_ready; }
    UpscaleBackendType GetType() const override { return UpscaleBackendType::DLSS; }
    bool SupportsFG() const override
    {
#if defined(XRAY_USE_DLSS)
        return m_ready && Streamline_IsFGAvailable();
#else
        return false;
#endif
    }
    bool SupportsRR() const override
    {
#if defined(XRAY_USE_DLSS)
        return m_ready && Streamline_IsRRAvailable();
#else
        return false;
#endif
    }
    bool Evaluate(nvrhi::ICommandList* cmd, const UpscaleInputs& inputs) override
    {
        if (!m_ready || !cmd || !inputs.color || !inputs.output)
            return false;
#if defined(XRAY_USE_DLSS)
        UpscaleInputs in = inputs;
        in.enableFG = in.enableFG && ps_r_dlss_fg != 0 && Streamline_IsFGAvailable();
        in.enableRR = in.enableRR && ps_r_dlss_rr != 0 && Streamline_IsRRAvailable();
        if (Streamline_EvaluateDLSS(cmd, in))
            return true;
#endif
        if (inputs.renderWidth == inputs.displayWidth &&
            inputs.renderHeight == inputs.displayHeight &&
            inputs.color != inputs.output)
        {
            nvrhi::TextureSlice slice;
            cmd->copyTexture(inputs.output, slice, inputs.color, slice);
            return true;
        }
        return false;
    }
};

}

IUpscaleBackend* CreateNullUpscaleBackend() { return new NullUpscaleBackend(); }
IUpscaleBackend* CreateDLSSUpscaleBackend() { return new DLSSUpscaleBackend(); }

IUpscaleBackend* CreateUpscaleBackendAuto()
{
    if (ps_r_dlss != 0 && ps_r_upscale == 0)
        ps_r_upscale = 2;

    if (ps_r_upscale == 1)
    {
        Msg("! [Upscale] FSR removed; use r_upscale 2 (DLSS)");
        ps_r_upscale = 0;
    }

    if (ps_r_upscale == 3)
    {
        ps_r_upscale = 2;
        Msg("* [Upscale] Auto selected DLSS");
    }

    if (ps_r_upscale == 2)
    {
        auto* dlss = CreateDLSSUpscaleBackend();
        if (dlss->Init(GEnv.Backend ? GEnv.Backend->GetDevice() : nullptr) && dlss->IsAvailable())
            return dlss;
        delete dlss;
        Msg("! [Upscale] DLSS unavailable");
        ps_r_upscale = 0;
    }

    auto* nullBackend = CreateNullUpscaleBackend();
    nullBackend->Init(nullptr);
    return nullBackend;
}

}
