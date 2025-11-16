#include "stdafx.h"
#include "ImGuiPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/ImGuiRendererNVRHI.h"
#include <imgui.h>

namespace xray::render::passes {

framegraph::VirtualResourceHandle setupImGuiPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle inputTarget,
    ng::ImGuiRendererNVRHI* imguiRenderer,
    u32 width,
    u32 height)
{
    using namespace framegraph;

    struct ImGuiPassData {
        VirtualResourceHandle input;
        VirtualResourceHandle output;
        ng::ImGuiRendererNVRHI* renderer;
        u32 width;
        u32 height;
    };

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
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("ImGui Pass");

            if (!data.renderer) {
                cmdList->endMarker();
                return;
            }

            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();

            if (!drawData || drawData->TotalVtxCount == 0) {
                cmdList->endMarker();
                return;
            }

            auto* outputRT = fg.GetPhysicalTexture(data.output);

            if (!outputRT) {
                Msg("! [ImGuiPass] Failed to get output texture");
                cmdList->endMarker();
                return;
            }

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputRT);

            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            data.renderer->Render(drawData, framebuffer, cmdList);

            Msg("* [ImGuiPass] ImGui rendered (%d vertices)", drawData->TotalVtxCount);

            cmdList->endMarker();
        }
    );

    return passData.output;
}

}