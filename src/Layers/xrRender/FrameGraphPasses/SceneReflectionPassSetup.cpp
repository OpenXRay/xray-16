#include "stdafx.h"
#include "SceneReflectionPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
struct SceneReflectionCaptureData
{
    VirtualResourceHandle src;
    VirtualResourceHandle dst;
};

nvrhi::TextureHandle EnsureSceneReflectionTexture(
    nvrhi::IDevice* nvDevice,
    SceneReflectionPassState& state,
    u32 width,
    u32 height)
{
    if (!nvDevice || width == 0 || height == 0)
        return nullptr;

    if (state.texture && state.width == width && state.height == height)
        return state.texture;

    nvrhi::TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.isRenderTarget = true;
    desc.isShaderResource = true;
    desc.debugName = "rt_SceneReflection";
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    state.texture = nvDevice->createTexture(desc);
    state.width = width;
    state.height = height;
    if (!state.texture)
    {
        Msg("! [SceneReflection] Failed to create rt_SceneReflection %ux%u RGBA16F",
            width, height);
        state.width = 0;
        state.height = 0;
    }
    return state.texture;
}
} // namespace

VirtualResourceHandle setupSceneReflectionCapture(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle litSceneColor,
    u32 width,
    u32 height,
    SceneReflectionPassState& state)
{
    if (!litSceneColor.is_valid() || width == 0 || height == 0 || !device)
        return litSceneColor;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    nvrhi::ITexture* phys = EnsureSceneReflectionTexture(nvDevice, state, width, height).Get();
    if (!phys)
        return litSceneColor;

    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.isRenderTarget = true;
    desc.isTransient = false;
    desc.debugName = "rt_SceneReflection";
    auto dst = fg.ImportTexture("rt_SceneReflection", phys, desc);

    (void)fg.addCallbackPass<SceneReflectionCaptureData>(
        "SceneReflectionCapture",
        [litSceneColor, dst](FrameGraph& builder, PassHandle passHandle, SceneReflectionCaptureData& data)
        {
            RenderPassBuilder pb(builder, passHandle);
            data.src = pb.read(litSceneColor, ResourceState::CopySource);
            data.dst = pb.write(dst, ResourceState::CopyDest);
            pb.sideEffects();
        },
        [](const SceneReflectionCaptureData& data, const FrameGraph& graph, fg::RenderContext* ctx)
        {
            if (!ctx)
                return;
            auto* cmd = ctx->GetCommandList();
            auto* src = graph.GetPhysicalTexture(data.src);
            auto* dst = graph.GetPhysicalTexture(data.dst);
            if (!cmd || !src || !dst || src == dst)
                return;

            cmd->setTextureState(src, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
            cmd->setTextureState(dst, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
            cmd->copyTexture(dst, nvrhi::TextureSlice(), src, nvrhi::TextureSlice());
            cmd->setTextureState(dst, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

            static bool s_logged = false;
            if (!s_logged)
            {
                Msg("* [SceneReflection] Captured lit scene → rt_SceneReflection (wet/SSR source)");
                s_logged = true;
            }
        });

    return dst;
}

} // namespace
