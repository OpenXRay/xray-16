#include "stdafx.h"
#include "ImGuiPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/ImGuiRendererNVRHI.h"
#include <imgui.h>

namespace xray::render::fg::passes {

framegraph::VirtualResourceHandle setupImGuiPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    fg::ImGuiRendererNVRHI* imguiRenderer,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<ImGuiPassData>(
        "ImGui",

        [inputTarget, imguiRenderer, width, height](FrameGraph& builder, PassHandle passHandle, ImGuiPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.renderer = imguiRenderer;

            data.input = passBuilder.read(inputTarget, ResourceState::ShaderResource);

            data.output = passBuilder.write(inputTarget, ResourceState::RenderTarget);
        },

        [](const ImGuiPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            if (!data.renderer) {
                return;
            }

            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();

            if (!drawData || drawData->TotalVtxCount == 0) {
                return;
            }

            auto* outputRT = fg.GetPhysicalTexture(data.output);

            if (!outputRT) {
                Msg("! [ImGuiPass] Failed to get output texture");
                return;
            }

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputRT);

            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            data.renderer->Render(drawData, framebuffer, cmdList);
        }
    );

    return passData.output;
}

}
