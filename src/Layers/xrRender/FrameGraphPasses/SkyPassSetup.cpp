#include "stdafx.h"
#include "SkyPassSetup.h"
#include "ShaderConstants.h"
#include "ExposurePassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/dxEnvironmentRender.h"
#include "xrEngine/Environment.h"

namespace RENDER_NAMESPACE {
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

static Fvector3 hbox_verts[24] = {
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

static u16 hbox_faces[20 * 3] = {
    0,   2,  3,
    3,   1,  0,
    4,   5,  7,
    7,   6,  4,
    0,   1,  9,
    9,   8,  0,
    8,   9,  5,
    5,   4,  8,
    1,   3, 10,
    10,  9,  1,
    9,  10,  7,
    7,   5,  9,
    3,   2, 11,
    11, 10,  3,
    10, 11,  6,
    6,   7, 10,
    2,   0,  8,
    8,  11,  2,
    11,  8,  4,
    4,   6, 11
};

void InitializeSkyGeometry(ng::RenderDevice* device, SkyPassState& state) {
    if (state.initialized || !device) return;

    auto* nvrhiDevice = device->GetNVRHIDevice();
    if (!nvrhiDevice) return;

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = 12 * sizeof(SkyVertex);
    vbDesc.debugName = "SkyVertexBuffer";
    vbDesc.isVertexBuffer = true;
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    state.vertexBuffer = nvrhiDevice->createBuffer(vbDesc);

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = 20 * 3 * sizeof(u16);
    ibDesc.debugName = "SkyIndexBuffer";
    ibDesc.isIndexBuffer = true;
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    state.indexBuffer = nvrhiDevice->createBuffer(ibDesc);
    
    nvrhi::TextureDesc placeholderDesc;
    placeholderDesc.width = 1;
    placeholderDesc.height = 1;
    placeholderDesc.format = nvrhi::Format::RGBA8_UNORM;
    placeholderDesc.dimension = nvrhi::TextureDimension::TextureCube;
    placeholderDesc.arraySize = 6;
    placeholderDesc.debugName = "PlaceholderSkyCubemap";
    placeholderDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    placeholderDesc.keepInitialState = true;
    state.placeholderCubemap = nvrhiDevice->createTexture(placeholderDesc);

    if (GEnv.Backend) {
        GEnv.Backend->UploadBufferData(state.indexBuffer, hbox_faces, sizeof(hbox_faces));
    }

    nvrhi::CommandListHandle cmdList = nvrhiDevice->createCommandList();
    cmdList->open();
    u32 skyBlue = 0xFF8080FF;
    for (u32 face = 0; face < 6; face++) {
        cmdList->writeTexture(state.placeholderCubemap, face, 0, &skyBlue, sizeof(skyBlue));
    }

    cmdList->close();
    nvrhiDevice->executeCommandList(cmdList);

    state.initialized = true;
    Msg("* [SkyPass] Sky geometry initialized");
}

void ShutdownSkyGeometry(SkyPassState& state) {
    state.vertexBuffer = nullptr;
    state.indexBuffer = nullptr;
    state.placeholderCubemap = nullptr;
    state.initialized = false;
}

framegraph::VirtualResourceHandle setupSkyPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle depthInput,
    CEnvironment* environment,
    u32 width,
    u32 height,
    SkyPassState& skyState)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<SkyPassData>(
        "Sky",

        [colorInput, depthInput, device, environment, width, height, &skyState](
            FrameGraph& builder, PassHandle passHandle, SkyPassData& data) {

            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.environment = environment;
            data.passState = &skyState;
            data.width = width;
            data.height = height;

            data.colorOutput = passBuilder.write(colorInput, ResourceState::RenderTarget);

            data.depthOutput = passBuilder.read(depthInput, ResourceState::DepthStencilRead);
        },

        [](const SkyPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            if (!data.environment || !data.passState->initialized) {
                return;
            }

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            auto* colorRT = fg.GetPhysicalTexture(data.colorOutput);
            auto* depthRT = fg.GetPhysicalTexture(data.depthOutput);

            if (!colorRT || !depthRT) {
                Msg("! [SkyPass] Failed to get render targets");
                return;
            }

            CEnvDescriptor& env = data.environment->CurrentEnv;

            Fmatrix mSky;
            mSky.rotateY(env.sky_rotation);
            mSky.translate_over(Device.vCameraPosition);

            u32 skyColor = color_rgba(
                iFloor(env.sky_color.x * 255.f),
                iFloor(env.sky_color.y * 255.f),
                iFloor(env.sky_color.z * 255.f),
                iFloor(data.environment->CurrentEnv.weight * 255.f)
            );

            SkyVertex vertices[12];
            for (u32 v = 0; v < 12; v++) {
                vertices[v].position = hbox_verts[v * 2];
                vertices[v].color = skyColor;
                vertices[v].texcoord0 = hbox_verts[v * 2 + 1];
                vertices[v].texcoord1 = hbox_verts[v * 2 + 1];
            }

            cmdList->writeBuffer(data.passState->vertexBuffer, vertices, sizeof(vertices));

            if (!RImplementation.m_shaderLoader) {
                Msg("! [SkyPass] ShaderLoader not initialized");
                return;
            }

            auto vsResult = RImplementation.m_shaderLoader->LoadVertexShader("sky_forward");
            auto psResult = RImplementation.m_shaderLoader->LoadPixelShader("sky_forward");

            if (!vsResult.handle || !psResult.handle) {
                Msg("! [SkyPass] Failed to load sky_forward shaders, falling back to sky2");
                vsResult = RImplementation.m_shaderLoader->LoadVertexShader("sky2");
                psResult = RImplementation.m_shaderLoader->LoadPixelShader("sky2");

                if (!vsResult.handle || !psResult.handle) {
                    Msg("! [SkyPass] Failed to load sky shaders");
                    return;
                }
            }

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::IDevice* device = cmdList->getDevice();

            nvrhi::BindingLayoutDesc bindingLayoutDesc;
            bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
            bindingLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::Texture_SRV(1),
                nvrhi::BindingLayoutItem::Sampler(0)
            };

            auto bindingLayout = cache.GetOrCreateBindingLayout("SkyPass", bindingLayoutDesc, device);

            nvrhi::RenderState renderState;
            renderState.blendState.targets[0].setBlendEnable(false);
            renderState.depthStencilState.setDepthTestEnable(false);
            renderState.depthStencilState.setDepthWriteEnable(false);
            renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

            nvrhi::VertexAttributeDesc vertexAttribs[] = {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setOffset(offsetof(SkyVertex, position))
                    .setElementStride(sizeof(SkyVertex)),
                nvrhi::VertexAttributeDesc()
                    .setName("COLOR")
                    .setFormat(nvrhi::Format::RGBA8_UNORM)
                    .setOffset(offsetof(SkyVertex, color))
                    .setElementStride(sizeof(SkyVertex)),
                nvrhi::VertexAttributeDesc()
                    .setName("TEXCOORD")
                    .setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setArraySize(2)
                    .setOffset(offsetof(SkyVertex, texcoord0))
                    .setElementStride(sizeof(SkyVertex)),
            };

            auto inputLayout = cache.GetOrCreateInputLayout("SkyPass", vertexAttribs, 3, vsResult.handle, device);

            nvrhi::GraphicsPipelineDesc pipelineDesc;
            pipelineDesc.setVertexShader(vsResult.handle);
            pipelineDesc.setPixelShader(psResult.handle);
            pipelineDesc.addBindingLayout(bindingLayout);
            pipelineDesc.setInputLayout(inputLayout);
            pipelineDesc.setRenderState(renderState);
            pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);

            nvrhi::FramebufferInfoEx fbInfo;
            fbInfo.addColorFormat(colorRT->getDesc().format);

            auto pipeline = cache.GetOrCreatePipeline("SkyPass", pipelineDesc, fbInfo, device);

            if (!pipeline) {
                Msg("! [SkyPass] Failed to create pipeline");
                return;
            }

            DynamicTransforms dynamicCB = {};
            FillDynamicTransforms(dynamicCB, mSky);

            auto dynamicCBBuffer = cache.GetOrCreateVolatileCB("SkyPass", "DynamicCB", sizeof(DynamicTransforms), 16, device);
            cmdList->writeBuffer(dynamicCBBuffer, &dynamicCB, sizeof(dynamicCB));

            StaticGlobals staticCB = {};
            FillGlobalConstants(staticCB);
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);
            FillSunConstants(staticCB, sunData);

            auto staticCBBuffer = cache.GetOrCreateVolatileCB("SkyPass", "StaticCB", sizeof(StaticGlobals), 16, device);
            cmdList->writeBuffer(staticCBBuffer, &staticCB, sizeof(staticCB));

            nvrhi::ITexture* sky0Tex = nullptr;
            nvrhi::ITexture* sky1Tex = nullptr;

            auto* resourceManager = data.device->GetFGResourceManager();
            resources::TextureManager* texManager = resourceManager ? resourceManager->GetTextureManager() : nullptr;

            if (texManager && data.environment->Current[0] && data.environment->Current[1]) {
                const shared_str& skyName0 = data.environment->Current[0]->sky_texture_name;
                const shared_str& skyName1 = data.environment->Current[1]->sky_texture_name;

                if (skyName0.size()) {
                    auto handle0 = texManager->LoadTexture(skyName0.c_str());
                    sky0Tex = texManager->GetNVRHITexture(handle0);
                }
                if (skyName1.size()) {
                    auto handle1 = texManager->LoadTexture(skyName1.c_str());
                    sky1Tex = texManager->GetNVRHITexture(handle1);
                }
            }

            if (!sky0Tex) sky0Tex = data.passState->placeholderCubemap.Get();
            if (!sky1Tex) sky1Tex = data.passState->placeholderCubemap.Get();

            nvrhi::SamplerDesc samplerDesc;
            samplerDesc.setAllFilters(true);
            samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
            auto sampler = cache.GetOrCreateSampler("SkyPass", samplerDesc, device);

            nvrhi::BindingSetDesc bindingSetDesc;
            bindingSetDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, dynamicCBBuffer),
                nvrhi::BindingSetItem::ConstantBuffer(2, staticCBBuffer),
                nvrhi::BindingSetItem::Texture_SRV(0, sky0Tex),
                nvrhi::BindingSetItem::Texture_SRV(1, sky1Tex),
                nvrhi::BindingSetItem::Sampler(0, sampler)
            };

            auto bindingSet = cache.GetOrCreateBindingSet(bindingSetDesc, bindingLayout, device);

            cmdList->clearTextureFloat(colorRT, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            auto framebuffer = cache.GetOrCreateFramebuffer("SkyPass", fbDesc, device);

            nvrhi::Viewport viewport;
            viewport.minX = 0;
            viewport.minY = 0;
            viewport.maxX = static_cast<float>(data.width);
            viewport.maxY = static_cast<float>(data.height);
            viewport.minZ = 0.0f;
            viewport.maxZ = 1.0f;

            nvrhi::GraphicsState state;
            state.pipeline = pipeline;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(viewport);
            state.addBindingSet(bindingSet);
            state.vertexBuffers = {{data.passState->vertexBuffer, 0, 0}};
            state.indexBuffer = {data.passState->indexBuffer, nvrhi::Format::R16_UINT, 0};

            cmdList->setGraphicsState(state);

            nvrhi::DrawArguments drawArgs;
            drawArgs.vertexCount = 12;
            drawArgs.instanceCount = 1;
            drawArgs.startIndexLocation = 0;
            drawArgs.startVertexLocation = 0;

            cmdList->drawIndexed(nvrhi::DrawArguments{60, 1, 0, 0, 0});
        }
    );

    return passData.colorOutput;
}

}
