#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "FrameGraph/BindingSetBuilder.h"
#include "FrameGraph/ShaderCache.h"
#include "FrameGraph/ShaderLoader.h"

namespace xray::render::ng {

void ImGuiRendererNVRHI::UploadDrawData(ImDrawData* drawData, nvrhi::ICommandList* cmdList)
{
    if (drawData->TotalVtxCount > (int)m_vertexBufferSize)
    {
        int newVtxSize = drawData->TotalVtxCount + 5000;
        ResizeBuffers(newVtxSize, m_indexBufferSize);
    }

    if (drawData->TotalIdxCount > (int)m_indexBufferSize)
    {
        int newIdxSize = drawData->TotalIdxCount + 10000;
        ResizeBuffers(m_vertexBufferSize, newIdxSize);
    }

    size_t vtxOffset = 0;
    size_t idxOffset = 0;

    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* imCmdList = drawData->CmdLists[n];

        size_t vtxSize = imCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        cmdList->writeBuffer(m_vertexBuffer, imCmdList->VtxBuffer.Data, vtxSize, vtxOffset * sizeof(ImDrawVert));

        size_t idxSize = imCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        cmdList->writeBuffer(m_indexBuffer, imCmdList->IdxBuffer.Data, idxSize, idxOffset * sizeof(ImDrawIdx));

        vtxOffset += imCmdList->VtxBuffer.Size;
        idxOffset += imCmdList->IdxBuffer.Size;
    }
}

nvrhi::ITexture* ImGuiRendererNVRHI::GetTextureFromImTextureID(ImTextureID id)
{
    if (id == ImGui::GetIO().Fonts->TexRef.GetTexID())
    {
        return m_fontTexture.Get();
    }

    return reinterpret_cast<nvrhi::ITexture*>(id);
}

void ImGuiRendererNVRHI::UpdateTextureBinding(ImTextureID textureId)
{
    nvrhi::ITexture* texture = GetTextureFromImTextureID(textureId);
    if (!texture)
    {
        texture = m_fontTexture.Get();
    }

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("imgui", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("imgui", ".ps");

    if (vsRefl && psRefl) {
        framegraph::BindingSetBuilder bsb(*vsRefl, *psRefl, m_device, "ImGui.PerDraw");
        bsb.ConstantBuffer("vertexBuffer", m_constantBuffer)
           .Texture("texture0", nvrhi::TextureHandle(texture));
        m_resourceBindings = m_device->createBindingSet(bsb.Build(), m_bindingLayout);
    }
}

void ImGuiRendererNVRHI::SetupRenderState(ImDrawData* drawData, nvrhi::ICommandList* cmdList)
{
    m_viewport.minX = 0.0f;
    m_viewport.minY = 0.0f;
    m_viewport.maxX = drawData->DisplaySize.x * drawData->FramebufferScale.x;
    m_viewport.maxY = drawData->DisplaySize.y * drawData->FramebufferScale.y;
    m_viewport.minZ = 0.0f;
    m_viewport.maxZ = 1.0f;

    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

    ImGuiConstants constants;
    constants.mvpMatrix[0][0] = 2.0f / (R - L);
    constants.mvpMatrix[0][1] = 0.0f;
    constants.mvpMatrix[0][2] = 0.0f;
    constants.mvpMatrix[0][3] = 0.0f;

    constants.mvpMatrix[1][0] = 0.0f;
    constants.mvpMatrix[1][1] = 2.0f / (T - B);
    constants.mvpMatrix[1][2] = 0.0f;
    constants.mvpMatrix[1][3] = 0.0f;

    constants.mvpMatrix[2][0] = 0.0f;
    constants.mvpMatrix[2][1] = 0.0f;
    constants.mvpMatrix[2][2] = 0.5f;
    constants.mvpMatrix[2][3] = 0.0f;

    constants.mvpMatrix[3][0] = (R + L) / (L - R);
    constants.mvpMatrix[3][1] = (T + B) / (B - T);
    constants.mvpMatrix[3][2] = 0.5f;
    constants.mvpMatrix[3][3] = 1.0f;

    cmdList->writeBuffer(m_constantBuffer, &constants, sizeof(ImGuiConstants));
}

void ImGuiRendererNVRHI::RenderDrawData(ImDrawData* drawData, nvrhi::ICommandList* cmdList)
{
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        return;

    UploadDrawData(drawData, cmdList);

    SetupRenderState(drawData, cmdList);

    nvrhi::GraphicsState graphicsState;
    graphicsState.pipeline = m_pipeline;
    graphicsState.framebuffer = m_currentFramebuffer;
    graphicsState.bindings = { m_resourceBindings };
    graphicsState.vertexBuffers = {
        { m_vertexBuffer, 0, 0 }
    };
    graphicsState.indexBuffer = { m_indexBuffer, nvrhi::Format::R16_UINT, 0 };

    int globalVtxOffset = 0;
    int globalIdxOffset = 0;
    ImVec2 clipOff = drawData->DisplayPos;
    ImVec2 clipScale = drawData->FramebufferScale;
    ImTextureID lastTextureId = 0;

    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* imCmdList = drawData->CmdLists[n];

        for (int cmd_i = 0; cmd_i < imCmdList->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &imCmdList->CmdBuffer[cmd_i];

            if (pcmd->UserCallback != nullptr)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                {
                    SetupRenderState(drawData, cmdList);
                }
                else
                {
                    pcmd->UserCallback(imCmdList, pcmd);
                }
            }
            else
            {
                ImVec4 clipRect;
                clipRect.x = (pcmd->ClipRect.x - clipOff.x) * clipScale.x;
                clipRect.y = (pcmd->ClipRect.y - clipOff.y) * clipScale.y;
                clipRect.z = (pcmd->ClipRect.z - clipOff.x) * clipScale.x;
                clipRect.w = (pcmd->ClipRect.w - clipOff.y) * clipScale.y;

                if (clipRect.x < drawData->DisplaySize.x * drawData->FramebufferScale.x &&
                    clipRect.y < drawData->DisplaySize.y * drawData->FramebufferScale.y &&
                    clipRect.z >= 0.0f &&
                    clipRect.w >= 0.0f)
                {
                    nvrhi::Rect scissor;
                    scissor.minX = (int)clipRect.x;
                    scissor.minY = (int)clipRect.y;
                    scissor.maxX = (int)clipRect.z;
                    scissor.maxY = (int)clipRect.w;

                    ImTextureID texId = pcmd->GetTexID();
                    if (texId != lastTextureId)
                    {
                        UpdateTextureBinding(texId);
                        lastTextureId = texId;

                        graphicsState.bindings = { m_resourceBindings };
                    }

                    graphicsState.viewport = nvrhi::ViewportState();
                    graphicsState.viewport.addViewport(m_viewport);
                    graphicsState.viewport.addScissorRect(scissor);

                    cmdList->setGraphicsState(graphicsState);

                    nvrhi::DrawArguments args;
                    args.setVertexCount(pcmd->ElemCount);
                    args.setStartIndexLocation(pcmd->IdxOffset + globalIdxOffset);
                    args.setStartVertexLocation(pcmd->VtxOffset + globalVtxOffset);

                    cmdList->drawIndexed(args);
                }
            }
        }

        globalIdxOffset += imCmdList->IdxBuffer.Size;
        globalVtxOffset += imCmdList->VtxBuffer.Size;
    }
}

}
