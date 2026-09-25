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
constexpr const char* kSkyCubeName = "SkyCube";
constexpr const char* kSkySourceShader = "sky_source";
}

bool SkyEnvironmentParams::Matches(const SkyEnvironmentParams& other) const
{
    return sky0 == other.sky0 && sky1 == other.sky1
        && std::abs(blend - other.blend) <= kBlendTolerance
        && std::abs(rotation - other.rotation) <= kRotationTolerance
        && energy == other.energy && groundAlbedo == other.groundAlbedo;
}

bool SkyEnvironmentFrame::Valid() const
{
    return cube.is_valid() && texture;
}

void SkyEnvironment::Initialize(RenderDevice* device)
{
    m_device = device;
}

void SkyEnvironment::Shutdown()
{
    InvalidatePipelines();
    m_cube = nullptr;
    m_device = nullptr;
}

void SkyEnvironment::Invalidate()
{
    m_sourceValid = false;
}

void SkyEnvironment::InvalidatePipelines()
{
    m_sourceLayout = nullptr;
    m_sourcePipeline = nullptr;
    m_pipelineFailed = false;
    m_sourceValid = false;
}

bool SkyEnvironment::Prepare(SkyEnvironmentParams& params)
{
    if (!g_pGamePersistent || !EnsureCube() || !EnsureSourcePipeline())
        return false;

    CEnvironment& environment = g_pGamePersistent->Environment();
    if (!ResolveSkyTextures(environment, params.sky0, params.sky1)
        || params.sky0->getDesc().dimension != nvrhi::TextureDimension::TextureCube
        || params.sky1->getDesc().dimension != nvrhi::TextureDimension::TextureCube)
        return false;

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

void SkyEnvironment::CommitSource(const SkyEnvironmentParams& params)
{
    m_source = params;
    m_sourceValid = true;
    ++m_revision;
}

void SkyEnvironment::DiscardSource()
{
    m_sourceValid = false;
}

SkyEnvironmentFrame SkyEnvironment::Use(framegraph::FrameGraph& graph) const
{
    SkyEnvironmentFrame frame;
    if (!m_cube)
        return frame;

    const nvrhi::TextureDesc& native = m_cube->getDesc();
    framegraph::ResourceDesc desc;
    desc.type = framegraph::ResourceDesc::Type::TextureCube;
    desc.width = native.width;
    desc.height = native.height;
    desc.depth = native.depth;
    desc.arraySize = native.arraySize;
    desc.mipLevels = native.mipLevels;
    desc.format = native.format;
    desc.sampleCount = native.sampleCount;
    desc.isUAV = true;
    desc.isTransient = false;
    desc.debugName = kSkyCubeName;
    frame.cube = graph.ImportTexture(kSkyCubeName, m_cube, desc);
    frame.texture = m_cube;
    frame.revision = m_revision;
    return frame;
}

RenderDevice* SkyEnvironment::GetDevice() const
{
    return m_device;
}

nvrhi::IBindingLayout* SkyEnvironment::GetSourceLayout() const
{
    return m_sourceLayout;
}

nvrhi::IComputePipeline* SkyEnvironment::GetSourcePipeline() const
{
    return m_sourcePipeline;
}

bool SkyEnvironment::EnsureCube()
{
    if (m_cube)
        return true;

    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    if (!nvDevice)
        return false;

    nvrhi::TextureDesc desc;
    desc.dimension = nvrhi::TextureDimension::TextureCube;
    desc.width = kCubeSize;
    desc.height = kCubeSize;
    desc.arraySize = 6;
    desc.mipLevels = 1;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.isUAV = true;
    desc.debugName = kSkyCubeName;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    m_cube = nvDevice->createTexture(desc);
    m_sourceValid = false;
    if (!m_cube)
        Msg("! [SkyEnvironment] sky cube could not be created");
    return m_cube != nullptr;
}

bool SkyEnvironment::EnsureSourcePipeline()
{
    if (m_sourcePipeline)
        return true;
    if (m_pipelineFailed)
        return false;

    nvrhi::IDevice* nvDevice = m_device ? m_device->GetNVRHIDevice() : nullptr;
    auto* loader = GEnv.Render ? GEnv.Render->GetShaderLoader() : nullptr;
    if (!nvDevice || !loader)
        return false;

    auto shader = loader->LoadComputeShader(kSkySourceShader);
    if (!shader.handle || !shader.reflection)
    {
        Msg("! [SkyEnvironment] %s shader unavailable, ray traced lighting has no sky", kSkySourceShader);
        m_pipelineFailed = true;
        return false;
    }

    auto& cache = framegraph::GetPassResourceCache();
    m_sourceLayout = cache.GetOrCreateBindingLayoutFromReflection("SkySource", *shader.reflection, nvDevice);
    if (m_sourceLayout)
    {
        nvrhi::ComputePipelineDesc desc;
        desc.CS = shader.handle;
        desc.bindingLayouts = { m_sourceLayout };
        m_sourcePipeline = cache.GetOrCreateComputePipeline("SkySource", desc, nvDevice);
    }
    if (m_sourcePipeline)
        return true;

    Msg("! [SkyEnvironment] sky source pipeline could not be created, ray traced lighting has no sky");
    m_pipelineFailed = true;
    return false;
}
}
