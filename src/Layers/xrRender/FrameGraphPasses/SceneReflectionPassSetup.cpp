#include "stdafx.h"
#include "SceneReflectionPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

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
} // namespace

VirtualResourceHandle setupSceneReflectionCapture(
    FrameGraph& fg,
    VirtualResourceHandle litSceneColor,
    u32 width,
    u32 height)
{
    if (!litSceneColor.is_valid() || width == 0 || height == 0)
        return litSceneColor;

    ResourceDesc desc;
    desc.type = ResourceDesc::Type::Texture2D;
    desc.width = width;
    desc.height = height;
    desc.format = nvrhi::Format::RGBA16_FLOAT;
    desc.isRenderTarget = true;
    // Must NOT be transient — aliasing reused normal/worldPos memory → green/purple SSR
    desc.isTransient = false;
    desc.debugName = "rt_SceneReflection";
    auto dst = fg.CreateTexture("rt_SceneReflection", desc);

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
