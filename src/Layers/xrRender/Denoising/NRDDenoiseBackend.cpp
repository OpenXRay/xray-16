#include "stdafx.h"
#include "IDenoiseBackend.h"

#if defined(XRAY_USE_NRD)

#include "Layers/xrRender/Backend/VulkanBackend.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"

#include <nvrhi/vulkan.h>
#include <cstring>

#pragma push_macro("NONE")
#pragma push_macro("TRUE")
#pragma push_macro("FALSE")
#pragma push_macro("CONST")
#undef NONE
#undef TRUE
#undef FALSE
#undef CONST
#include "NRD.h"
#include "NRI.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRIWrapperVK.h"
#include "NRDIntegration.hpp"
#pragma pop_macro("CONST")
#pragma pop_macro("FALSE")
#pragma pop_macro("TRUE")
#pragma pop_macro("NONE")

extern ENGINE_API int ps_r_nrd_method;
extern ENGINE_API int ps_r_nrd_apply;
extern ENGINE_API int ps_r_upscale;
extern ENGINE_API float ps_r_rt_gi_intensity;

namespace xray::render::fg {
namespace {

struct NrdPackCB
{
    float worldToView[16];
    float worldToViewPrev[16];
    float worldToClip[16];
    float worldToClipPrev[16];
    float invViewProj[16];
    float invViewProjPrev[16];
    float screenNearFar[4];
    float hitDistMethod[4];
    float cameraPosRange[4];
};

struct NrdCompositeCB
{
    float params[4];
    float cameraPos[4];
    float fogParams[4];
    float fogColor[4];
    float invViewProj[16];
};

void NrdCopyMatrix(float dst[16], const Fmatrix& m)
{
    dst[0] = m._11; dst[1] = m._12; dst[2] = m._13; dst[3] = m._14;
    dst[4] = m._21; dst[5] = m._22; dst[6] = m._23; dst[7] = m._24;
    dst[8] = m._31; dst[9] = m._32; dst[10] = m._33; dst[11] = m._34;
    dst[12] = m._41; dst[13] = m._42; dst[14] = m._43; dst[15] = m._44;
}

class NRDDenoiseBackend final : public IDenoiseBackend
{
    nvrhi::IDevice* m_device = nullptr;
    nrd::Integration m_nrd;
    nrd::Identifier m_denoiserId = 0;
    bool m_ready = false;
    bool m_logged = false;
    u32 m_width = 0;
    u32 m_height = 0;
    int m_method = -1;
    u32 m_pipeVersion = 0;

    nvrhi::TextureHandle m_inDiff;
    nvrhi::TextureHandle m_inSpec;
    nvrhi::TextureHandle m_inNormalRough;
    nvrhi::TextureHandle m_inViewZ;
    nvrhi::TextureHandle m_inMv;
    nvrhi::TextureHandle m_outDiff;
    nvrhi::TextureHandle m_outSpec;
    nvrhi::TextureHandle m_sceneCopy;

    nvrhi::ComputePipelineHandle m_packPipeline;
    nvrhi::BindingLayoutHandle m_packLayout;
    nvrhi::ComputePipelineHandle m_compositePipeline;
    nvrhi::BindingLayoutHandle m_compositeLayout;
    nvrhi::BufferHandle m_cb;
    u32 m_evalOkFrames = 0;
    u32 m_evalFailFrames = 0;
    u32 m_prevW = 0;
    u32 m_prevH = 0;
    u32 m_nrdFrameIndex = 0;
    bool m_diagLogged = false;

    static constexpr float kDenoisingRange = 500000.f;
    static constexpr u32 kPipeVersion = 12;

    void Fail(const char* reason)
    {
        if ((m_evalFailFrames++ % 120u) == 0u)
            Msg("! [Denoise] NRD Evaluate failed: %s", reason);
    }

    void SyncAfterNative(nvrhi::ICommandList* cmd)
    {
        cmd->clearState();
        cmd->beginTrackingTextureState(m_inDiff, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
        cmd->beginTrackingTextureState(m_inSpec, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
        cmd->beginTrackingTextureState(m_inNormalRough, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
        cmd->beginTrackingTextureState(m_inViewZ, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
        cmd->beginTrackingTextureState(m_inMv, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
        cmd->beginTrackingTextureState(m_outDiff, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
        cmd->beginTrackingTextureState(m_outSpec, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
    }

    nvrhi::TextureHandle CreateTex(const char* name, nvrhi::Format fmt, u32 w, u32 h)
    {
        nvrhi::TextureDesc desc;
        desc.width = w;
        desc.height = h;
        desc.format = fmt;
        desc.mipLevels = 1;
        desc.dimension = nvrhi::TextureDimension::Texture2D;
        desc.debugName = name;
        desc.isUAV = true;
        desc.isShaderResource = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        return m_device->createTexture(desc);
    }

    bool EnsurePipelines()
    {
        if (m_packPipeline && m_compositePipeline && m_cb && m_pipeVersion == kPipeVersion)
            return true;
        if (!GEnv.Render || !GEnv.Render->GetShaderLoader())
            return false;

        m_packPipeline = nullptr;
        m_compositePipeline = nullptr;
        m_packLayout = nullptr;
        m_compositeLayout = nullptr;
        m_cb = nullptr;

        framegraph::BindingSetBuilder::InvalidateReflectionCache();
        auto& cache = framegraph::GetPassResourceCache();
        {
            auto cs = GEnv.Render->GetShaderLoader()->LoadComputeShader("nrd_pack_inputs");
            if (!cs.handle || !cs.reflection)
                return false;
            m_packLayout = cache.GetOrCreateBindingLayoutFromReflection("NRD_Pack_v10", *cs.reflection, m_device);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = cs.handle;
            pipeDesc.bindingLayouts = { m_packLayout };
            m_packPipeline = m_device->createComputePipeline(pipeDesc);
        }
        {
            auto cs = GEnv.Render->GetShaderLoader()->LoadComputeShader("nrd_composite");
            if (!cs.handle || !cs.reflection)
                return false;
            m_compositeLayout = cache.GetOrCreateBindingLayoutFromReflection("NRD_Composite_v10", *cs.reflection, m_device);
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = cs.handle;
            pipeDesc.bindingLayouts = { m_compositeLayout };
            m_compositePipeline = m_device->createComputePipeline(pipeDesc);
        }
        {
            nvrhi::BufferDesc bd;
            bd.byteSize = 768;
            bd.isConstantBuffer = true;
            bd.isVolatile = true;
            bd.maxVersions = 64;
            bd.debugName = "NRD_CB_v10";
            bd.initialState = nvrhi::ResourceStates::ConstantBuffer;
            bd.keepInitialState = true;
            m_cb = m_device->createBuffer(bd);
        }
        m_pipeVersion = kPipeVersion;
        return m_packPipeline && m_compositePipeline && m_cb;
    }

    bool EnsureResources(u32 w, u32 h)
    {
        if (m_inDiff && m_width == w && m_height == h && m_inMv &&
            m_inMv->getDesc().format == nvrhi::Format::RGBA16_FLOAT)
            return true;
        m_width = w;
        m_height = h;
        m_inDiff = CreateTex("NRD_IN_DIFF", nvrhi::Format::RGBA16_FLOAT, w, h);
        m_inSpec = CreateTex("NRD_IN_SPEC", nvrhi::Format::RGBA16_FLOAT, w, h);
        m_inNormalRough = CreateTex("NRD_IN_NR", nvrhi::Format::RGBA16_UNORM, w, h);
        m_inViewZ = CreateTex("NRD_IN_VIEWZ", nvrhi::Format::R32_FLOAT, w, h);
        m_inMv = CreateTex("NRD_IN_MV", nvrhi::Format::RGBA16_FLOAT, w, h);
        m_outDiff = CreateTex("NRD_OUT_DIFF", nvrhi::Format::RGBA16_FLOAT, w, h);
        m_outSpec = CreateTex("NRD_OUT_SPEC", nvrhi::Format::RGBA16_FLOAT, w, h);
        m_sceneCopy = nullptr;
        return m_inDiff && m_inSpec && m_inNormalRough && m_inViewZ && m_inMv && m_outDiff && m_outSpec;
    }

    bool RecreateIntegration(u32 w, u32 h, int method)
    {
        if (!GEnv.Backend || GEnv.Backend->GetAPI() != IRenderBackend::API::Vulkan)
            return false;

        auto* vb = static_cast<VulkanBackend*>(GEnv.Backend);
        nri::QueueFamilyVKDesc queueFamily{};
        queueFamily.queueNum = 1;
        queueFamily.queueType = nri::QueueType::GRAPHICS;
        queueFamily.familyIndex = vb->GetGraphicsQueueFamily();

        nri::DeviceCreationVKDesc deviceDesc{};
        deviceDesc.vkInstance = vb->GetVkInstance();
        deviceDesc.vkDevice = vb->GetVkDevice();
        deviceDesc.vkPhysicalDevice = vb->GetVkPhysicalDevice();
        deviceDesc.queueFamilies = &queueFamily;
        deviceDesc.queueFamilyNum = 1;
        deviceDesc.minorVersion = 3;
        const nrd::LibraryDesc* libDesc = nrd::GetLibraryDesc();
        deviceDesc.vkBindingOffsets = {
            libDesc->spirvBindingOffsets.samplerOffset,
            libDesc->spirvBindingOffsets.textureOffset,
            libDesc->spirvBindingOffsets.constantBufferOffset,
            libDesc->spirvBindingOffsets.storageTextureAndBufferOffset
        };

        nrd::DenoiserDesc denoiserDesc{};
        denoiserDesc.identifier = 0;
        denoiserDesc.denoiser = (method != 0)
            ? nrd::Denoiser::RELAX_DIFFUSE_SPECULAR
            : nrd::Denoiser::REBLUR_DIFFUSE_SPECULAR;

        nrd::InstanceCreationDesc instanceDesc{};
        instanceDesc.denoisers = &denoiserDesc;
        instanceDesc.denoisersNum = 1;

        nrd::IntegrationCreationDesc integrationDesc{};
        strncpy(integrationDesc.name, "OpenXRay NRD", sizeof(integrationDesc.name) - 1);
        integrationDesc.resourceWidth = (uint16_t)w;
        integrationDesc.resourceHeight = (uint16_t)h;
        integrationDesc.queuedFrameNum = 3;
        integrationDesc.enableWholeLifetimeDescriptorCaching = false;
        integrationDesc.autoWaitForIdle = true;

        const nrd::Result result = m_nrd.RecreateVK(integrationDesc, instanceDesc, deviceDesc);
        if (result != nrd::Result::SUCCESS) {
            Msg("! [Denoise] NRD RecreateVK failed (%u)", (u32)result);
            return false;
        }

        m_denoiserId = denoiserDesc.identifier;
        m_method = method;
        m_width = w;
        m_height = h;
        m_nrdFrameIndex = 0;
        m_prevW = 0;
        m_prevH = 0;
        m_diagLogged = false;

        const float fps = Device.fTimeDelta > 1e-4f ? (1.f / Device.fTimeDelta) : 60.f;
        if (method != 0) {
            nrd::RelaxSettings settings{};
            settings.hitDistanceReconstructionMode = nrd::HitDistanceReconstructionMode::AREA_3X3;
            settings.enableAntiFirefly = true;
            settings.diffusePrepassBlurRadius = 50.f;
            settings.specularPrepassBlurRadius = 80.f;
            settings.atrousIterationNum = 5;
            settings.diffuseMaxAccumulatedFrameNum = nrd::GetMaxAccumulatedFrameNum(1.5f, fps);
            settings.specularMaxAccumulatedFrameNum = nrd::GetMaxAccumulatedFrameNum(1.75f, fps);
            settings.diffuseMaxFastAccumulatedFrameNum = 6;
            settings.specularMaxFastAccumulatedFrameNum = 7;
            m_nrd.SetDenoiserSettings(m_denoiserId, &settings);
        } else {
            nrd::ReblurSettings settings{};
            settings.hitDistanceReconstructionMode = nrd::HitDistanceReconstructionMode::AREA_3X3;
            settings.enableAntiFirefly = true;
            settings.diffusePrepassBlurRadius = 30.f;
            settings.specularPrepassBlurRadius = 50.f;
            settings.maxBlurRadius = 30.f;
            settings.minBlurRadius = 1.f;
            settings.maxAccumulatedFrameNum = nrd::GetMaxAccumulatedFrameNum(1.5f, fps);
            settings.maxFastAccumulatedFrameNum = 6;
            settings.maxStabilizedFrameNum = settings.maxAccumulatedFrameNum;
            settings.minHitDistanceWeight = 0.1f;
            settings.fireflySuppressorMinRelativeScale = 2.0f;
            settings.lobeAngleFraction = 0.15f;
            settings.roughnessFraction = 0.15f;
            settings.planeDistanceSensitivity = 0.02f;
            settings.antilagSettings.luminanceSigmaScale = 2.0f;
            settings.antilagSettings.luminanceSensitivity = 3.0f;
            settings.fastHistoryClampingSigmaScale = 2.0f;
            m_nrd.SetDenoiserSettings(m_denoiserId, &settings);
        }

        if (!m_logged) {
            const nrd::LibraryDesc* lib = nrd::GetLibraryDesc();
            Msg("* [Denoise] NRD backend initialized v%u.%u.%u method=%s pool=%.1fMB",
                lib ? lib->versionMajor : NRD_VERSION_MAJOR,
                lib ? lib->versionMinor : NRD_VERSION_MINOR,
                lib ? lib->versionBuild : NRD_VERSION_BUILD,
                method != 0 ? "RELAX" : "REBLUR",
                m_nrd.GetTotalMemoryUsageInMb());
            m_logged = true;
        }
        return true;
    }

    nrd::Resource MakeVkResource(nvrhi::ITexture* tex)
    {
        nrd::Resource resource{};
        const auto native = tex->getNativeObject(nvrhi::ObjectTypes::VK_Image);
        resource.vk.image = (VKNonDispatchableHandle)native.integer;
        resource.vk.format = (VKEnum)nvrhi::vulkan::convertFormat(tex->getDesc().format);
        resource.state = { nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::Layout::SHADER_RESOURCE_STORAGE, nri::StageBits::COMPUTE_SHADER };
        resource.userArg = tex;
        if (!resource.vk.image || !resource.vk.format)
            Fail("MakeVkResource null image/format");
        return resource;
    }

public:
    bool Init(nvrhi::IDevice* device) override
    {
        m_device = device;
        if (!m_device || !GEnv.Backend || GEnv.Backend->GetAPI() != IRenderBackend::API::Vulkan) {
            Msg("! [Denoise] NRD requires Vulkan backend");
            return false;
        }

        const auto size = GEnv.Backend->GetBackBufferSize();
        u32 w = std::max(1u, size.first);
        u32 h = std::max(1u, size.second);
        if (!EnsurePipelines()) {
            Msg("! [Denoise] NRD pack/composite shaders failed to load");
            return false;
        }
        if (!EnsureResources(w, h))
            return false;
        if (!RecreateIntegration(w, h, ps_r_nrd_method))
            return false;

        m_ready = true;
        return true;
    }

    void Shutdown() override
    {
        m_nrd.Destroy();
        m_inDiff = nullptr;
        m_inSpec = nullptr;
        m_inNormalRough = nullptr;
        m_inViewZ = nullptr;
        m_inMv = nullptr;
        m_outDiff = nullptr;
        m_outSpec = nullptr;
        m_sceneCopy = nullptr;
        m_packPipeline = nullptr;
        m_packLayout = nullptr;
        m_compositePipeline = nullptr;
        m_compositeLayout = nullptr;
        m_cb = nullptr;
        m_ready = false;
        m_logged = false;
        m_diagLogged = false;
        m_nrdFrameIndex = 0;
        m_pipeVersion = 0;
        m_width = 0;
        m_height = 0;
        m_method = -1;
        m_prevW = 0;
        m_prevH = 0;
    }

    bool IsAvailable() const override { return false; }
    DenoiseBackendType GetType() const override { return DenoiseBackendType::NRD; }

    void Resize(u32 width, u32 height) override
    {
        if (!m_ready || !width || !height)
            return;
        if (width == m_width && height == m_height && m_method == ps_r_nrd_method)
            return;
        if (!EnsureResources(width, height))
            return;
        RecreateIntegration(width, height, ps_r_nrd_method);
    }

    bool Evaluate(nvrhi::ICommandList* cmd, const DenoiseInputs& inputs) override
    {
        (void)cmd;
        (void)inputs;
        static bool s_logged = false;
        if (!s_logged) {
            s_logged = true;
            Msg("! [Denoise] NRD DenoiseVK disabled (causes Device Removed); ReSTIR composite used instead");
        }
        return false;
    }

};

}

IDenoiseBackend* CreateNRDDenoiseBackend()
{
    return new NRDDenoiseBackend();
}

}

#else

namespace xray::render::fg {

namespace {

class NRDStubBackend final : public IDenoiseBackend
{
public:
    bool Init(nvrhi::IDevice*) override
    {
        Msg("* [Denoise] NRD not compiled in (XRAY_USE_NRD)");
        return false;
    }
    void Shutdown() override {}
    bool IsAvailable() const override { return false; }
    DenoiseBackendType GetType() const override { return DenoiseBackendType::NRD; }
    void Resize(u32, u32) override {}
    bool Evaluate(nvrhi::ICommandList*, const DenoiseInputs&) override { return false; }
};

}

IDenoiseBackend* CreateNRDDenoiseBackend()
{
    return new NRDStubBackend();
}

}

#endif
