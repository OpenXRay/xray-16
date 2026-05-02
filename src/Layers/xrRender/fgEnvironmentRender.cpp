#include "stdafx.h"

#include "fgEnvironmentRender.h"

#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraphPasses/PassVertexFormats.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/xr_efflensflare.h"

namespace xray::render::fg
{
namespace
{
static const Fvector3 hbox_verts[24] = {
    {-1.f, -1.f,   -1.f}, {-1.f, -1.01f, -1.f},
    { 1.f, -1.f,   -1.f}, { 1.f, -1.01f, -1.f},
    {-1.f, -1.f,    1.f}, {-1.f, -1.01f,  1.f},
    { 1.f, -1.f,    1.f}, { 1.f, -1.01f,  1.f},
    {-1.f,  1.f,   -1.f}, {-1.f,  1.f,   -1.f},
    { 1.f,  1.f,   -1.f}, { 1.f,  1.f,   -1.f},
    {-1.f,  1.f,    1.f}, {-1.f,  1.f,    1.f},
    { 1.f,  1.f,    1.f}, { 1.f,  1.f,    1.f},
    {-1.f, -0.01f, -1.f}, {-1.f, -1.f,   -1.f},
    { 1.f, -0.01f, -1.f}, { 1.f, -1.f,   -1.f},
    { 1.f, -0.01f,  1.f}, { 1.f, -1.f,    1.f},
    {-1.f, -0.01f,  1.f}, {-1.f, -1.f,    1.f}
};

static const u16 hbox_faces[20 * 3] = {
    0,   2,  3,   3,   1,  0,
    4,   5,  7,   7,   6,  4,
    0,   1,  9,   9,   8,  0,
    8,   9,  5,   5,   4,  8,
    1,   3, 10,  10,   9,  1,
    9,  10,  7,   7,   5,  9,
    3,   2, 11,  11,  10,  3,
   10,  11,  6,   6,   7, 10,
    2,   0,  8,   8,  11,  2,
   11,   8,  4,   4,   6, 11
};
}

void FGEnvDescriptorRender::Copy(IEnvDescriptorRender& _in)
{
    *this = *static_cast<FGEnvDescriptorRender*>(&_in);
}

void FGEnvDescriptorRender::OnDeviceCreate(CEnvDescriptor& owner)
{
    if (owner.sky_texture_name.size())
        sky_texture.create(owner.sky_texture_name.c_str());
    if (owner.sky_texture_env_name.size())
        sky_texture_env.create(owner.sky_texture_env_name.c_str());
    if (owner.clouds_texture_name.size())
        clouds_texture.create(owner.clouds_texture_name.c_str());
}

void FGEnvDescriptorRender::OnDeviceDestroy()
{
    sky_texture.destroy();
    sky_texture_env.destroy();
    clouds_texture.destroy();
}

FGEnvironmentRender::FGEnvironmentRender()
{
    tsky0.create(r2_T_sky0);
    tsky1.create(r2_T_sky1);
    t_envmap_0.create(r2_T_envs0);
    t_envmap_1.create(r2_T_envs1);
    tonemap.create(r2_RT_luminance_cur);
}

void FGEnvironmentRender::Copy(IEnvironmentRender& _in)
{
    *this = *static_cast<FGEnvironmentRender*>(&_in);
}

const particles_systems::library_interface& FGEnvironmentRender::particles_systems_library()
{
    return RImplementation.m_PSLibrary;
}

void FGEnvironmentRender::Clear()
{
    std::pair<u32, ref_texture> zero = std::make_pair(u32(0), ref_texture(nullptr));
    sky_r_textures.clear();
    sky_r_textures.push_back(zero);
    sky_r_textures.push_back(zero);
    sky_r_textures.push_back(zero);

    clouds_r_textures.clear();
    clouds_r_textures.push_back(zero);
    clouds_r_textures.push_back(zero);
    clouds_r_textures.push_back(zero);
}

void FGEnvironmentRender::lerp(CEnvDescriptorMixer& currentEnv, IEnvDescriptorRender* inA, IEnvDescriptorRender* inB)
{
    auto* pA = static_cast<FGEnvDescriptorRender*>(inA);
    auto* pB = static_cast<FGEnvDescriptorRender*>(inB);

    sky_r_textures.clear();
    sky_r_textures.emplace_back(tsky0_tstage, pA->sky_texture);
    sky_r_textures.emplace_back(tsky1_tstage, pB->sky_texture);
    if (tonemap_tstage_2sky != u32(-1))
        sky_r_textures.emplace_back(tonemap_tstage_2sky, tonemap);

    clouds_r_textures.clear();
    clouds_r_textures.emplace_back(tclouds0_tstage, pA->clouds_texture);
    clouds_r_textures.emplace_back(tclouds1_tstage, pB->clouds_texture);
    if (tonemap_tstage_clouds != u32(-1))
        clouds_r_textures.emplace_back(tonemap_tstage_clouds, tonemap);

    auto e0 = sky_r_textures[0].second->surface_get();
    auto e1 = sky_r_textures[1].second->surface_get();
    tsky0->surface_set(e0);
    _RELEASE(e0);
    tsky1->surface_set(e1);
    _RELEASE(e1);

    const bool menu_pp = g_pGamePersistent->OnRenderPPUI_query();
    e0 = menu_pp ? nullptr : pA->sky_texture_env->surface_get();
    e1 = menu_pp ? nullptr : pB->sky_texture_env->surface_get();
    t_envmap_0->surface_set(e0);
    _RELEASE(e0);
    t_envmap_1->surface_set(e1);
    _RELEASE(e1);
}

void FGEnvironmentRender::OnDeviceCreate()
{
    if (GEnv.isDedicatedServer)
        return;
}

void FGEnvironmentRender::OnDeviceDestroy()
{
    sky_r_textures.clear();
    clouds_r_textures.clear();

    tsky0->surface_set(nullptr);
    tsky1->surface_set(nullptr);
    t_envmap_0->surface_set(nullptr);
    t_envmap_1->surface_set(nullptr);
    tonemap->surface_set(nullptr);

    tsky0_tstage = 0;
    tsky1_tstage = 0;
    tclouds0_tstage = 0;
    tclouds1_tstage = 0;
    tonemap_tstage_2sky = u32(-1);
    tonemap_tstage_clouds = u32(-1);

    m_skyVertexBuffer = nullptr;
    m_skyIndexBuffer = nullptr;
    m_skyConstantBuffer = nullptr;
    m_skyPlaceholderCube = nullptr;
    m_skySampler = nullptr;
    m_skyVS = nullptr;
    m_skyPS = nullptr;
    m_skyInputLayout = nullptr;
    m_skyBindingLayout = nullptr;
    m_skyPipeline = nullptr;
    m_skyInitialized = false;

    m_sunVertexBuffer = nullptr;
    m_sunIndexBuffer = nullptr;
    m_sunPlaceholderTex = nullptr;
    m_sunVS = nullptr;
    m_sunPS = nullptr;
    m_sunInputLayout = nullptr;
    m_sunBindingLayout = nullptr;
    m_sunPipeline = nullptr;
    m_sunInitialized = false;

    m_device = nullptr;
}

void FGEnvironmentRender::InitSkyResources()
{
    if (m_skyInitialized)
        return;

    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* renderDevice = fgRenderer->GetRenderDevice();
    m_device = renderDevice->GetNVRHIDevice();
    R_ASSERT(m_device);

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = 12 * sizeof(passes::SkyVertex);
    vbDesc.debugName = "FGEnv_SkyVB";
    vbDesc.isVertexBuffer = true;
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    m_skyVertexBuffer = m_device->createBuffer(vbDesc);
    R_ASSERT2(m_skyVertexBuffer, "FGEnv: createBuffer(VB) failed");

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = sizeof(hbox_faces);
    ibDesc.debugName = "FGEnv_SkyIB";
    ibDesc.isIndexBuffer = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    m_skyIndexBuffer = m_device->createBuffer(ibDesc);
    R_ASSERT2(m_skyIndexBuffer, "FGEnv: createBuffer(IB) failed");

    nvrhi::TextureDesc cubeDesc;
    cubeDesc.width = 1;
    cubeDesc.height = 1;
    cubeDesc.format = nvrhi::Format::RGBA8_UNORM;
    cubeDesc.dimension = nvrhi::TextureDimension::TextureCube;
    cubeDesc.arraySize = 6;
    cubeDesc.debugName = "FGEnv_SkyPlaceholderCube";
    cubeDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    cubeDesc.keepInitialState = true;
    m_skyPlaceholderCube = m_device->createTexture(cubeDesc);
    R_ASSERT2(m_skyPlaceholderCube, "FGEnv: placeholder cubemap createTexture failed");

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
    samplerDesc.setAllFilters(true);
    m_skySampler = m_device->createSampler(samplerDesc);

    {
        nvrhi::CommandListHandle uploadCmd = m_device->createCommandList();
        uploadCmd->open();
        uploadCmd->writeBuffer(m_skyIndexBuffer, hbox_faces, sizeof(hbox_faces));
        u32 skyBlue = 0xFF8080FF;
        for (u32 face = 0; face < 6; ++face)
            uploadCmd->writeTexture(m_skyPlaceholderCube, face, 0, &skyBlue, sizeof(skyBlue));
        uploadCmd->close();
        m_device->executeCommandList(uploadCmd);
    }

    auto* shaderLoader = RImplementation.GetShaderLoader();
    R_ASSERT(shaderLoader);

    auto vsResult = shaderLoader->LoadVertexShader("sky_forward");
    auto psResult = shaderLoader->LoadPixelShader("sky_forward");
    if (!vsResult.handle || !psResult.handle)
    {
        vsResult = shaderLoader->LoadVertexShader("sky2");
        psResult = shaderLoader->LoadPixelShader("sky2");
    }
    R_ASSERT2(vsResult.handle && psResult.handle, "FGEnv: failed to load sky shaders");
    m_skyVS = vsResult.handle;
    m_skyPS = psResult.handle;

    auto& cache = framegraph::GetPassResourceCache();
    m_skyBindingLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "FGEnv_Sky", *vsResult.reflection, *psResult.reflection, m_device);
    R_ASSERT2(m_skyBindingLayout, "FGEnv: createBindingLayout failed");

    nvrhi::VertexAttributeDesc vertexAttribs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(offsetof(passes::SkyVertex, position))
            .setElementStride(sizeof(passes::SkyVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setOffset(offsetof(passes::SkyVertex, color))
            .setElementStride(sizeof(passes::SkyVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setArraySize(2)
            .setOffset(offsetof(passes::SkyVertex, texcoord0))
            .setElementStride(sizeof(passes::SkyVertex)),
    };
    m_skyInputLayout = cache.GetOrCreateInputLayout("FGEnv_Sky", vertexAttribs, 3, m_skyVS, m_device);

    nvrhi::RenderState renderState;
    renderState.blendState.targets[0].setBlendEnable(false);
    renderState.depthStencilState.setDepthTestEnable(false);
    renderState.depthStencilState.setDepthWriteEnable(false);
    renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.setVertexShader(m_skyVS);
    pipelineDesc.setPixelShader(m_skyPS);
    pipelineDesc.addBindingLayout(m_skyBindingLayout);
    pipelineDesc.setInputLayout(m_skyInputLayout);
    pipelineDesc.setRenderState(renderState);
    pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);

    m_skyPipeline = cache.GetOrCreatePipeline("FGEnv_Sky", pipelineDesc, fbInfo, m_device);
    R_ASSERT2(m_skyPipeline, "FGEnv: createGraphicsPipeline failed");

    m_skyInitialized = true;
}

void FGEnvironmentRender::DrawSky(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, CEnvironment* environment, u32 width, u32 height)
{
    if (!environment || !cmdList || !framebuffer)
        return;

    InitSkyResources();
    if (!m_skyInitialized)
        return;

    CEnvDescriptor& env = environment->CurrentEnv;

    Fmatrix mSky;
    mSky.rotateY(env.sky_rotation);
    mSky.translate_over(Device.vCameraPosition);

    u32 skyColor = color_rgba(
        iFloor(env.sky_color.x * 255.f),
        iFloor(env.sky_color.y * 255.f),
        iFloor(env.sky_color.z * 255.f),
        iFloor(environment->CurrentEnv.weight * 255.f));

    passes::SkyVertex vertices[12];
    for (u32 v = 0; v < 12; ++v)
    {
        vertices[v].position = hbox_verts[v * 2];
        vertices[v].color = skyColor;
        vertices[v].texcoord0 = hbox_verts[v * 2 + 1];
        vertices[v].texcoord1 = hbox_verts[v * 2 + 1];
    }
    cmdList->writeBuffer(m_skyVertexBuffer, vertices, sizeof(vertices));

    auto& cache = framegraph::GetPassResourceCache();

    passes::DynamicTransforms dynamicCB = {};
    passes::FillDynamicTransforms(dynamicCB, mSky);
    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* renderDevice = fgRenderer->GetRenderDevice();
    auto dynamicCBBuffer = cache.GetOrCreateVolatileCB("FGEnv_Sky", "DynamicCB", sizeof(passes::DynamicTransforms), renderDevice);
    cmdList->writeBuffer(dynamicCBBuffer, &dynamicCB, sizeof(dynamicCB));

    nvrhi::ITexture* sky0Tex = nullptr;
    nvrhi::ITexture* sky1Tex = nullptr;
    auto* texManager = renderDevice->GetFGResourceManager()
        ? renderDevice->GetFGResourceManager()->GetTextureManager() : nullptr;
    if (texManager && environment->Current[0] && environment->Current[1])
    {
        const shared_str& skyName0 = environment->Current[0]->sky_texture_name;
        const shared_str& skyName1 = environment->Current[1]->sky_texture_name;
        if (skyName0.size())
            sky0Tex = texManager->GetNVRHITexture(texManager->LoadTexture(skyName0.c_str()));
        if (skyName1.size())
            sky1Tex = texManager->GetNVRHITexture(texManager->LoadTexture(skyName1.c_str()));
    }
    if (!sky0Tex) sky0Tex = m_skyPlaceholderCube.Get();
    if (!sky1Tex) sky1Tex = m_skyPlaceholderCube.Get();

    auto* vsRefl = RImplementation.GetShaderLoader()->GetCachedReflection("sky_forward", ".vs");
    auto* psRefl = RImplementation.GetShaderLoader()->GetCachedReflection("sky_forward", ".ps");
    if (!vsRefl || !psRefl)
        return;

    framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, m_device, "FGEnv_Sky");
    bsb.ConstantBuffer("dynamic_transforms", dynamicCBBuffer);
    bsb.Texture("s_sky0", sky0Tex);
    bsb.Texture("s_sky1", sky1Tex);

    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), m_skyBindingLayout, m_device);

    auto* colorRT = framebuffer->getDesc().colorAttachments[0].texture;
    if (colorRT)
        cmdList->clearTextureFloat(colorRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));

    nvrhi::Viewport viewport;
    viewport.minX = 0; viewport.minY = 0;
    viewport.maxX = static_cast<float>(width);
    viewport.maxY = static_cast<float>(height);
    viewport.minZ = 0.0f; viewport.maxZ = 1.0f;

    nvrhi::GraphicsState state;
    state.pipeline = m_skyPipeline;
    state.framebuffer = framebuffer;
    state.viewport.addViewportAndScissorRect(viewport);
    state.addBindingSet(bindingSet);
    state.vertexBuffers = {{m_skyVertexBuffer, 0, 0}};
    state.indexBuffer = {m_skyIndexBuffer, nvrhi::Format::R16_UINT, 0};

    cmdList->setGraphicsState(state);
    cmdList->drawIndexed(nvrhi::DrawArguments{60, 1, 0, 0, 0});
}

void FGEnvironmentRender::InitSunResources()
{
    if (m_sunInitialized)
        return;

    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* renderDevice = fgRenderer->GetRenderDevice();
    m_device = renderDevice->GetNVRHIDevice();
    R_ASSERT(m_device);

    nvrhi::TextureDesc texDesc;
    texDesc.width = 1;
    texDesc.height = 1;
    texDesc.format = nvrhi::Format::RGBA8_UNORM;
    texDesc.debugName = "FGEnv_SunPlaceholder";
    texDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    texDesc.keepInitialState = true;
    m_sunPlaceholderTex = m_device->createTexture(texDesc);

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = 4 * sizeof(passes::SunVertex);
    vbDesc.debugName = "FGEnv_SunVB";
    vbDesc.isVertexBuffer = true;
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    m_sunVertexBuffer = m_device->createBuffer(vbDesc);

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = 6 * sizeof(u16);
    ibDesc.debugName = "FGEnv_SunIB";
    ibDesc.isIndexBuffer = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    m_sunIndexBuffer = m_device->createBuffer(ibDesc);

    {
        nvrhi::CommandListHandle uploadCmd = m_device->createCommandList();
        uploadCmd->open();
        u32 white = 0xFFFFFFFF;
        uploadCmd->writeTexture(m_sunPlaceholderTex, 0, 0, &white, sizeof(white));
        u16 indices[] = {0, 1, 2, 2, 1, 3};
        uploadCmd->writeBuffer(m_sunIndexBuffer, indices, sizeof(indices));
        uploadCmd->close();
        m_device->executeCommandList(uploadCmd);
    }

    auto* shaderLoader = RImplementation.GetShaderLoader();
    R_ASSERT(shaderLoader);
    auto vsResult = shaderLoader->LoadVertexShader("sun_forward");
    auto psResult = shaderLoader->LoadPixelShader("sun_forward");
    R_ASSERT2(vsResult.handle && psResult.handle, "FGEnv: failed to load sun shaders");
    m_sunVS = vsResult.handle;
    m_sunPS = psResult.handle;

    auto& cache = framegraph::GetPassResourceCache();
    m_sunBindingLayout = cache.GetOrCreateBindingLayoutFromReflection(
        "FGEnv_Sun", *vsResult.reflection, *psResult.reflection, m_device);
    R_ASSERT(m_sunBindingLayout);

    nvrhi::VertexAttributeDesc attribs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(offsetof(passes::SunVertex, position))
            .setElementStride(sizeof(passes::SunVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setOffset(offsetof(passes::SunVertex, color))
            .setElementStride(sizeof(passes::SunVertex)),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(passes::SunVertex, u))
            .setElementStride(sizeof(passes::SunVertex)),
    };
    m_sunInputLayout = cache.GetOrCreateInputLayout("FGEnv_Sun", attribs, std::size(attribs), m_sunVS, m_device);

    nvrhi::RenderState renderState;
    renderState.blendState.targets[0].setBlendEnable(true);
    renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::One);
    renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::One);
    renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
    renderState.depthStencilState.setDepthTestEnable(false);
    renderState.depthStencilState.setDepthWriteEnable(false);
    renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.inputLayout = m_sunInputLayout;
    pipelineDesc.VS = m_sunVS;
    pipelineDesc.PS = m_sunPS;
    pipelineDesc.bindingLayouts = {m_sunBindingLayout};
    pipelineDesc.renderState = renderState;
    pipelineDesc.primType = nvrhi::PrimitiveType::TriangleList;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);

    m_sunPipeline = cache.GetOrCreatePipeline("FGEnv_Sun", pipelineDesc, fbInfo, m_device);
    R_ASSERT(m_sunPipeline);

    m_sunInitialized = true;
}

void FGEnvironmentRender::DrawSun(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, CEnvironment* environment, u32 width, u32 height)
{
    if (!environment || !cmdList || !framebuffer)
        return;

    CLensFlare* lensFlare = environment->eff_LensFlare;
    if (!lensFlare)
        return;

    CLensFlareDescriptor* flareDesc = lensFlare->GetCurrent();
    if (!flareDesc || !flareDesc->m_Flags.is(CLensFlareDescriptor::flSource))
        return;

    InitSunResources();
    if (!m_sunInitialized)
        return;

    const CEnvDescriptorMixer& env = environment->CurrentEnv;

    Fvector vSunDir;
    vSunDir.mul(env.sun_dir, -1.0f);
    vSunDir.normalize();

    float fDot = vSunDir.dotproduct(Device.vCameraDirection);
    if (fDot <= 0.01f)
        return;

    float fDistance = env.far_plane * 0.75f;

    Fvector vecLight;
    vecLight.set(vSunDir);
    vecLight.mul(fDistance / fDot);
    vecLight.add(Device.vCameraPosition);

    Fvector vecX, vecY;
    vecX.set(Device.vCameraRight);
    vecY.crossproduct(vecX, Device.vCameraDirection);

    float sunRadius = flareDesc->m_Source.fRadius;
    bool ignoreColor = flareDesc->m_Source.ignore_color;

    Fcolor sunColor;
    if (ignoreColor)
        sunColor.set(1.0f, 1.0f, 1.0f, 1.0f);
    else
        sunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z, 1.0f);

    const float intensity = 2.0f;
    sunColor.r *= intensity;
    sunColor.g *= intensity;
    sunColor.b *= intensity;

    Fvector vecSx, vecSy;
    vecSx.mul(vecX, sunRadius * fDistance);
    vecSy.mul(vecY, sunRadius * fDistance);

    u32 c = sunColor.get();

    passes::SunVertex vertices[4];
    vertices[0].position.x = vecLight.x + vecSx.x - vecSy.x;
    vertices[0].position.y = vecLight.y + vecSx.y - vecSy.y;
    vertices[0].position.z = vecLight.z + vecSx.z - vecSy.z;
    vertices[0].color = c; vertices[0].u = 0.0f; vertices[0].v = 0.0f;

    vertices[1].position.x = vecLight.x + vecSx.x + vecSy.x;
    vertices[1].position.y = vecLight.y + vecSx.y + vecSy.y;
    vertices[1].position.z = vecLight.z + vecSx.z + vecSy.z;
    vertices[1].color = c; vertices[1].u = 0.0f; vertices[1].v = 1.0f;

    vertices[2].position.x = vecLight.x - vecSx.x - vecSy.x;
    vertices[2].position.y = vecLight.y - vecSx.y - vecSy.y;
    vertices[2].position.z = vecLight.z - vecSx.z - vecSy.z;
    vertices[2].color = c; vertices[2].u = 1.0f; vertices[2].v = 0.0f;

    vertices[3].position.x = vecLight.x - vecSx.x + vecSy.x;
    vertices[3].position.y = vecLight.y - vecSx.y + vecSy.y;
    vertices[3].position.z = vecLight.z - vecSx.z + vecSy.z;
    vertices[3].color = c; vertices[3].u = 1.0f; vertices[3].v = 1.0f;

    cmdList->writeBuffer(m_sunVertexBuffer, vertices, sizeof(vertices));

    auto& cache = framegraph::GetPassResourceCache();
    auto* fgRenderer = static_cast<FrameGraphRenderer*>(GEnv.Render);
    auto* renderDevice = fgRenderer->GetRenderDevice();

    auto dynamicCBBuffer = cache.GetOrCreateVolatileCB(
        "Frame", "DynamicTransforms", sizeof(passes::DynamicTransforms), renderDevice);

    nvrhi::ITexture* sunTex = nullptr;
    const shared_str& sunTexName = flareDesc->m_Source.texture;
    if (sunTexName.size())
    {
        auto* texManager = renderDevice->GetFGResourceManager()
            ? renderDevice->GetFGResourceManager()->GetTextureManager() : nullptr;
        if (texManager)
            sunTex = texManager->GetNVRHITexture(texManager->LoadTexture(sunTexName.c_str()));
    }
    if (!sunTex)
        sunTex = m_sunPlaceholderTex.Get();

    auto* vsRefl = RImplementation.GetShaderLoader()->GetCachedReflection("sun_forward", ".vs");
    auto* psRefl = RImplementation.GetShaderLoader()->GetCachedReflection("sun_forward", ".ps");
    if (!vsRefl || !psRefl)
        return;

    framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, m_device, "FGEnv_Sun");
    bsb.ConstantBuffer("dynamic_transforms", dynamicCBBuffer)
       .Texture("s_sun", sunTex);
    auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), m_sunBindingLayout, m_device);

    nvrhi::GraphicsState state;
    state.pipeline = m_sunPipeline;
    state.framebuffer = framebuffer;
    state.bindings = {bindingSet};
    state.vertexBuffers = {{m_sunVertexBuffer, 0, 0}};
    state.indexBuffer = {m_sunIndexBuffer, nvrhi::Format::R16_UINT, 0};
    state.viewport = nvrhi::ViewportState().addViewportAndScissorRect(
        nvrhi::Viewport(static_cast<float>(width), static_cast<float>(height)));

    cmdList->setGraphicsState(state);
    cmdList->drawIndexed(nvrhi::DrawArguments{6, 1, 0, 0, 0});
}
} // namespace xray::render::fg
