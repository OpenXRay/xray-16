#include "stdafx.h"
#include "SkyEnvironment.h"
#include "fgEnvironmentRender.h"
#include "xrRender_console.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"

namespace xray::render::fg
{
namespace
{
constexpr float kBlendTolerance = 1.0f / 256.0f;
constexpr float kRotationTolerance = 0.001f;
constexpr float kTintTolerance = 1.0f / 512.0f;
constexpr const char* kSkyCubeName = "SkyCube";
constexpr const char* kSkySpecularName = "SkySpecular";
constexpr const char* kSkyDFGName = "SkyDFG";
constexpr const char* kSkyIrradianceName = "SkyIrradiance";
constexpr const char* kStageShaders[u32(SkyComputeStage::Count)] =
{
    "sky_source",
    "sky_downsample",
    "sky_irradiance",
    "sky_specular",
    "sky_dfg"
};

u32 FullMipCount(u32 size)
{
    u32 levels = 1;
    while ((size >> levels) > 0)
        ++levels;
    return levels;
}

bool NearlyEqual(const Fvector& a, const Fvector& b, float tolerance)
{
    return std::abs(a.x - b.x) <= tolerance && std::abs(a.y - b.y) <= tolerance && std::abs(a.z - b.z) <= tolerance;
}

nvrhi::TextureHandle CreateSkyTexture(nvrhi::IDevice* device, nvrhi::TextureDimension dimension, u32 size, u32 mipLevels,
    const char* name)
{
    nvrhi::TextureDesc desc;
    desc.dimension = dimension;
    desc.width = size;
    desc.height = size;
    desc.arraySize = dimension == nvrhi::TextureDimension::TextureCube ? 6 : 1;
    desc.mipLevels = mipLevels;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.isUAV = true;
    desc.debugName = name;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    return device->createTexture(desc);
}

framegraph::ResourceDesc SkyImportDesc(nvrhi::ITexture* texture, framegraph::ResourceDesc::Type type, const char* name)
{
    const nvrhi::TextureDesc& native = texture->getDesc();
    framegraph::ResourceDesc desc;
    desc.type = type;
    desc.width = native.width;
    desc.height = native.height;
    desc.depth = native.depth;
    desc.arraySize = native.arraySize;
    desc.mipLevels = native.mipLevels;
    desc.format = native.format;
    desc.sampleCount = native.sampleCount;
    desc.isUAV = true;
    desc.isTransient = false;
    desc.debugName = name;
    return desc;
}
}

bool SkyEnvironmentParams::Matches(const SkyEnvironmentParams& other) const
{
    return sky0 == other.sky0 && sky1 == other.sky1
        && NearlyEqual(tint, other.tint, kTintTolerance)
        && std::abs(blend - other.blend) <= kBlendTolerance
        && std::abs(rotation - other.rotation) <= kRotationTolerance
        && energy == other.energy && groundAlbedo == other.groundAlbedo;
}

bool SkyEnvironmentFrame::Valid() const
{
    return cube.is_valid() && texture && sourceReady;
}

void SkyEnvironment::Initialize(RenderDevice* device)
{
    m_device = device;
    EnsureResources();
}

void SkyEnvironment::Shutdown()
{
    InvalidatePipelines();
    m_cube = nullptr;
    m_specular = nullptr;
    m_dfg = nullptr;
    m_irradiance = nullptr;
    m_device = nullptr;
}

void SkyEnvironment::Invalidate()
{
    m_sourceValid = false;
    m_lightingReady = false;
}

void SkyEnvironment::InvalidatePipelines()
{
    for (auto& stage : m_pipelines)
        stage = SkyComputePipeline();
    m_pipelineFailed = false;
    m_sourceValid = false;
    m_lightingReady = false;
    m_dfgReady = false;
}

bool SkyEnvironment::Prepare(SkyEnvironmentParams& params)
{
    if (!g_pGamePersistent || !EnsureResources() || !EnsurePipelines())
        return false;

    CEnvironment& environment = g_pGamePersistent->Environment();
    if (!ResolveSkyTextures(environment, params.sky0, params.sky1)
        || params.sky0->getDesc().dimension != nvrhi::TextureDimension::TextureCube
        || params.sky1->getDesc().dimension != nvrhi::TextureDimension::TextureCube)
        return false;

    const Fvector3& tint = environment.CurrentEnv.sky_color;
    params.tint.set(tint.x, tint.y, tint.z);
    params.blend = std::clamp(environment.CurrentEnv.weight, 0.0f, 1.0f);
    params.rotation = environment.CurrentEnv.sky_rotation;
    params.energy = std::max(ps_r_sky_energy, 0.0f);
    params.groundAlbedo = std::clamp(ps_r_sky_ground_albedo, 0.0f, 1.0f);
    return true;
}

bool SkyEnvironment::NeedsSource(const SkyEnvironmentParams& params) const
{
    return !m_sourceValid || !m_source.Matches(params);
}

bool SkyEnvironment::NeedsDFG() const
{
    return !m_dfgReady;
}

void SkyEnvironment::CommitSource(const SkyEnvironmentParams& params)
{
    m_source = params;
    m_sourceValid = true;
    ++m_revision;
}

void SkyEnvironment::DiscardSource()
{
    m_sourceValid = false;
    m_lightingReady = false;
}

bool SkyEnvironment::IsSourceValid() const
{
    return m_sourceValid;
}

void SkyEnvironment::CompleteLighting(bool ready)
{
    m_lightingReady = ready && m_sourceValid;
}

void SkyEnvironment::CompleteDFG(bool ready)
{
    m_dfgReady = ready;
}

SkyEnvironmentFrame SkyEnvironment::Use(framegraph::FrameGraph& graph, bool sourceReady) const
{
    SkyEnvironmentFrame frame;
    if (!m_cube || !m_specular || !m_dfg || !m_irradiance)
        return frame;

    frame.cube = graph.ImportTexture(kSkyCubeName, m_cube,
        SkyImportDesc(m_cube, framegraph::ResourceDesc::Type::TextureCube, kSkyCubeName));
    frame.specular = graph.ImportTexture(kSkySpecularName, m_specular,
        SkyImportDesc(m_specular, framegraph::ResourceDesc::Type::TextureCube, kSkySpecularName));
    frame.dfg = graph.ImportTexture(kSkyDFGName, m_dfg,
        SkyImportDesc(m_dfg, framegraph::ResourceDesc::Type::Texture2D, kSkyDFGName));

    framegraph::ResourceDesc irradianceDesc;
    irradianceDesc.type = framegraph::ResourceDesc::Type::Buffer;
    irradianceDesc.bufferSize = m_irradiance->getDesc().byteSize;
    irradianceDesc.structStride = m_irradiance->getDesc().structStride;
    irradianceDesc.allowUAV = true;
    irradianceDesc.isTransient = false;
    irradianceDesc.debugName = kSkyIrradianceName;
    frame.irradiance = graph.ImportBuffer(kSkyIrradianceName, m_irradiance, irradianceDesc);

    frame.texture = m_cube;
    frame.revision = m_revision;
    frame.sourceReady = sourceReady && m_sourceValid;
    frame.lightingReady = m_lightingReady && m_dfgReady;
    return frame;
}

RenderDevice* SkyEnvironment::GetDevice() const
{
    return m_device;
}

const SkyComputePipeline& SkyEnvironment::GetPipeline(SkyComputeStage stage) const
{
    return m_pipelines[u32(stage)];
}

bool SkyEnvironment::EnsureResources()
{
    if (m_cube && m_specular && m_dfg && m_irradiance)
        return true;

    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    if (!nvDevice)
        return false;

    if (!m_cube)
        m_cube = CreateSkyTexture(nvDevice, nvrhi::TextureDimension::TextureCube, kCubeSize, FullMipCount(kCubeSize), kSkyCubeName);
    if (!m_specular)
        m_specular = CreateSkyTexture(nvDevice, nvrhi::TextureDimension::TextureCube, kSpecularSize, kSpecularLevels, kSkySpecularName);
    if (!m_dfg)
        m_dfg = CreateSkyTexture(nvDevice, nvrhi::TextureDimension::Texture2D, kDFGSize, 1, kSkyDFGName);
    if (!m_irradiance)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = kIrradianceCoefficients * sizeof(Fvector4);
        desc.structStride = sizeof(Fvector4);
        desc.canHaveUAVs = true;
        desc.debugName = kSkyIrradianceName;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_irradiance = nvDevice->createBuffer(desc);
    }

    m_sourceValid = false;
    m_lightingReady = false;
    m_dfgReady = false;
    if (m_cube && m_specular && m_dfg && m_irradiance)
        return true;

    Msg("! [SkyEnvironment] sky resources could not be created");
    return false;
}

bool SkyEnvironment::EnsurePipelines()
{
    bool ready = true;
    for (const auto& stage : m_pipelines)
        ready = ready && stage.pipeline;
    if (ready)
        return true;
    if (m_pipelineFailed)
        return false;

    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!nvDevice || !loader)
        return false;

    auto& cache = framegraph::GetPassResourceCache();
    for (u32 index = 0; index < u32(SkyComputeStage::Count); ++index)
    {
        SkyComputePipeline& stage = m_pipelines[index];
        if (stage.pipeline)
            continue;

        const char* shaderName = kStageShaders[index];
        auto shader = loader->LoadComputeShader(shaderName);
        if (shader.handle && shader.reflection)
        {
            stage.shader = shaderName;
            stage.layout = cache.GetOrCreateBindingLayoutFromReflection(shaderName, *shader.reflection, nvDevice);
            if (stage.layout)
            {
                nvrhi::ComputePipelineDesc desc;
                desc.CS = shader.handle;
                desc.bindingLayouts = { stage.layout };
                stage.pipeline = cache.GetOrCreateComputePipeline(shaderName, desc, nvDevice);
            }
        }
        if (!stage.pipeline)
        {
            Msg("! [SkyEnvironment] %s pipeline could not be created, sky lighting is disabled", shaderName);
            m_pipelineFailed = true;
            return false;
        }
    }
    return true;
}
}
