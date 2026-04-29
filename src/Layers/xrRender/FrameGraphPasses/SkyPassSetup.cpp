#include "stdafx.h"
#include "SkyPassSetup.h"
#include "PassVertexFormats.h"
#include "ShaderConstants.h"
#include "ExposurePassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/dxEnvironmentRender.h"
#include "xrEngine/Environment.h"

namespace fg {
    extern CRender RImplementation;
}

namespace xray::render::fg::passes {

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

void InitializeSkyGeometry(fg::RenderDevice* device, SkyPassState& state) {
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

    if (!GEnv.FrameGraphRenderer->GetShaderLoader())
        return;

    auto vsResult = GEnv.FrameGraphRenderer->GetShaderLoader()->LoadVertexShader("sky_forward");
    auto psResult = GEnv.FrameGraphRenderer->GetShaderLoader()->LoadPixelShader("sky_forward");

    if (!vsResult.handle || !psResult.handle) {
        vsResult = GEnv.FrameGraphRenderer->GetShaderLoader()->LoadVertexShader("sky2");
        psResult = GEnv.FrameGraphRenderer->GetShaderLoader()->LoadPixelShader("sky2");
        if (!vsResult.handle || !psResult.handle)
            return;
    }

    auto& cache = framegraph::GetPassResourceCache();

    state.bindingLayout = cache.GetOrCreateBindingLayoutFromReflection("SkyPass", *vsResult.reflection, *psResult.reflection, nvrhiDevice);

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

    state.inputLayout = cache.GetOrCreateInputLayout("SkyPass", vertexAttribs, 3, vsResult.handle, nvrhiDevice);

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.setVertexShader(vsResult.handle);
    pipelineDesc.setPixelShader(psResult.handle);
    pipelineDesc.addBindingLayout(state.bindingLayout);
    pipelineDesc.setInputLayout(state.inputLayout);
    pipelineDesc.setRenderState(renderState);
    pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);

    state.pipeline = cache.GetOrCreatePipeline("SkyPass", pipelineDesc, fbInfo, nvrhiDevice);

    state.initialized = true;
    Msg("* [SkyPass] Sky geometry initialized");
}

void ShutdownSkyGeometry(SkyPassState& state) {
    state.vertexBuffer = nullptr;
    state.indexBuffer = nullptr;
    state.placeholderCubemap = nullptr;
    state.pipeline = nullptr;
    state.bindingLayout = nullptr;
    state.inputLayout = nullptr;
    state.initialized = false;
}

framegraph::VirtualResourceHandle setupSkyPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle depthInput,
    CEnvironment* environment,
    u32 width,
    u32 height,
    SkyPassState& skyState)
{
    using namespace framegraph;

    InitializeSkyGeometry(device, skyState);

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
           fg::RenderContext* ctx) {

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

            if (!data.passState->pipeline)
                return;

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::IDevice* device = cmdList->getDevice();

            DynamicTransforms dynamicCB = {};
            FillDynamicTransforms(dynamicCB, mSky);

            auto dynamicCBBuffer = cache.GetOrCreateVolatileCB("SkyPass", "DynamicCB", sizeof(DynamicTransforms), data.device);
            cmdList->writeBuffer(dynamicCBBuffer, &dynamicCB, sizeof(dynamicCB));

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

            auto* vsRefl = GEnv.FrameGraphRenderer->GetShaderLoader()->GetCachedReflection("sky_forward", ".vs");
            auto* psRefl = GEnv.FrameGraphRenderer->GetShaderLoader()->GetCachedReflection("sky_forward", ".ps");
            if (!vsRefl || !psRefl) return;
            framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, device, "Sky");
            bsb.ConstantBuffer("dynamic_transforms", dynamicCBBuffer);
            bsb.Texture("s_sky0", sky0Tex);
            bsb.Texture("s_sky1", sky1Tex);

            auto bindingSet = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->bindingLayout, device);

            cmdList->clearTextureFloat(colorRT, nvrhi::AllSubresources, nvrhi::Color(0.0f));

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
            state.pipeline = data.passState->pipeline;
            state.framebuffer = framebuffer;
            state.viewport.addViewportAndScissorRect(viewport);
            state.addBindingSet(bindingSet);
            state.vertexBuffers = {{data.passState->vertexBuffer, 0, 0}};
            state.indexBuffer = {data.passState->indexBuffer, nvrhi::Format::R16_UINT, 0};

            cmdList->setGraphicsState(state);

            cmdList->drawIndexed(nvrhi::DrawArguments{60, 1, 0, 0, 0});
        }
    );

    return passData.colorOutput;
}

}
