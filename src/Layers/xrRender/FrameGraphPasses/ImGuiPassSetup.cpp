// xrRender/FrameGraphPasses/ImGuiPassSetup.cpp
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

        // Setup lambda
        [inputTarget, imguiRenderer, width, height](FrameGraph& builder, PassHandle passHandle, ImGuiPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.renderer = imguiRenderer;

            // Read input
            data.input = passBuilder.read(inputTarget, ResourceState::ShaderResource);

            // Write to same target (ImGui renders on top)
            data.output = passBuilder.write(inputTarget, ResourceState::RenderTarget);
        },

        // Execute lambda
        [](const ImGuiPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            if (!data.renderer) {
                return;  // No ImGui renderer available
            }

            // Generate ImGui draw data
            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();

            if (!drawData || drawData->TotalVtxCount == 0) {
                return;  // Nothing to render
            }

            // Get physical resource
            auto* outputRT = fg.GetPhysicalTexture(data.output);

            if (!outputRT) {
                Msg("! [ImGuiPass] Failed to get output texture");
                return;
            }

            // Create framebuffer for ImGui
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(outputRT);

            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            // Render ImGui using the explicit framebuffer/cmdList method
            data.renderer->Render(drawData, framebuffer, cmdList);
        }
    );

    return passData.output;
}

} // namespace xray::render::fg::passes
