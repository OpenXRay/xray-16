#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"

namespace xray::render::ng {

//=============================================================================
// Phase 5: Vertex/Index Buffer Handling
//=============================================================================

void ImGuiRendererNVRHI::UploadDrawData(ImDrawData* drawData, nvrhi::ICommandList* cmdList)
{
    // Check if we need to grow vertex/index buffers
    if (drawData->TotalVtxCount > (int)m_vertexBufferSize)
    {
        int newVtxSize = drawData->TotalVtxCount + 5000; // Add some slack
        ResizeBuffers(newVtxSize, m_indexBufferSize);
    }

    if (drawData->TotalIdxCount > (int)m_indexBufferSize)
    {
        int newIdxSize = drawData->TotalIdxCount + 10000; // Add some slack
        ResizeBuffers(m_vertexBufferSize, newIdxSize);
    }

    // Upload vertex and index data for all command lists
    size_t vtxOffset = 0;
    size_t idxOffset = 0;

    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* imCmdList = drawData->CmdLists[n];

        // Write vertex data
        size_t vtxSize = imCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        // writeBuffer signature: (buffer, data, size, offset)
        cmdList->writeBuffer(m_vertexBuffer, imCmdList->VtxBuffer.Data, vtxSize, vtxOffset * sizeof(ImDrawVert));

        // Write index data
        size_t idxSize = imCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        cmdList->writeBuffer(m_indexBuffer, imCmdList->IdxBuffer.Data, idxSize, idxOffset * sizeof(ImDrawIdx));

        vtxOffset += imCmdList->VtxBuffer.Size;
        idxOffset += imCmdList->IdxBuffer.Size;
    }
}

//=============================================================================
// Phase 6: Texture Binding Helpers
//=============================================================================

nvrhi::ITexture* ImGuiRendererNVRHI::GetTextureFromImTextureID(ImTextureID id)
{
    // ImTextureID is typically a pointer to the texture resource
    // For font texture, we store the nvrhi texture handle directly
    if (id == ImGui::GetIO().Fonts->TexRef.GetTexID())
    {
        return m_fontTexture.Get();
    }

    // For user textures, they should pass nvrhi::ITexture* as ImTextureID
    return reinterpret_cast<nvrhi::ITexture*>(id);
}

void ImGuiRendererNVRHI::UpdateTextureBinding(ImTextureID textureId)
{
    nvrhi::ITexture* texture = GetTextureFromImTextureID(textureId);
    if (!texture)
    {
        texture = m_fontTexture.Get(); // Fallback to font texture
    }

    // Update binding set with new texture
    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
        nvrhi::BindingSetItem::Texture_SRV(0, nvrhi::TextureHandle(texture)),
        nvrhi::BindingSetItem::Sampler(0, m_fontSampler)
    };

    // Note: In production, you'd want to cache binding sets per texture
    m_resourceBindings = m_device->createBindingSet(bindingSetDesc, m_bindingLayout);
}

//=============================================================================
// Phase 7: Main Render Function Implementation
//=============================================================================

void ImGuiRendererNVRHI::SetupRenderState(ImDrawData* drawData, nvrhi::ICommandList* cmdList)
{
    // Setup viewport (store in member so it can be used in graphics state)
    m_viewport.minX = 0.0f;
    m_viewport.minY = 0.0f;
    m_viewport.maxX = drawData->DisplaySize.x * drawData->FramebufferScale.x;
    m_viewport.maxY = drawData->DisplaySize.y * drawData->FramebufferScale.y;
    m_viewport.minZ = 0.0f;
    m_viewport.maxZ = 1.0f;

    // Setup orthographic projection matrix
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

    // Upload projection matrix to constant buffer
    cmdList->writeBuffer(m_constantBuffer, &constants, sizeof(ImGuiConstants));
}

void ImGuiRendererNVRHI::RenderDrawData(ImDrawData* drawData, nvrhi::ICommandList* cmdList)
{
    // Avoid rendering when minimized
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        return;

    // Upload vertex/index buffers
    UploadDrawData(drawData, cmdList);

    // Setup render state
    SetupRenderState(drawData, cmdList);

    // Set graphics pipeline state
    nvrhi::GraphicsState graphicsState;
    graphicsState.pipeline = m_pipeline;
    graphicsState.framebuffer = m_currentFramebuffer;  // Use the framebuffer passed to Render()
    graphicsState.viewport.addViewport(m_viewport);    // Set viewport calculated in SetupRenderState()
    graphicsState.bindings = { m_resourceBindings };
    graphicsState.vertexBuffers = {
        { m_vertexBuffer, 0, 0 }
    };
    graphicsState.indexBuffer = { m_indexBuffer, nvrhi::Format::R16_UINT, 0 };

    // Apply complete graphics state
    cmdList->setGraphicsState(graphicsState);

    // Render command lists
    int globalVtxOffset = 0;
    int globalIdxOffset = 0;
    ImVec2 clipOff = drawData->DisplayPos;
    ImVec2 clipScale = drawData->FramebufferScale;
    ImTextureID lastTextureId = 0; // ImTextureID is a void*, 0 is a valid initial value

    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList* imCmdList = drawData->CmdLists[n];

        for (int cmd_i = 0; cmd_i < imCmdList->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &imCmdList->CmdBuffer[cmd_i];

            if (pcmd->UserCallback != nullptr)
            {
                // Execute user callback
                // (ImDrawCallback_ResetRenderState is a special callback value to reset render state)
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
                // Project scissor/clipping rectangles into framebuffer space
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
                    // Apply scissor/clipping rectangle
                    nvrhi::Rect scissor;
                    scissor.minX = (int)clipRect.x;
                    scissor.minY = (int)clipRect.y;
                    scissor.maxX = (int)clipRect.z;
                    scissor.maxY = (int)clipRect.w;

                    // Set scissor rect in graphics state
                    graphicsState.viewport.addScissorRect(scissor);

                    // Bind texture if changed
                    ImTextureID texId = pcmd->GetTexID();
                    if (texId != lastTextureId)
                    {
                        UpdateTextureBinding(texId);
                        lastTextureId = texId;

                        // Update graphics state with new bindings
                        graphicsState.bindings = { m_resourceBindings };
                    }

                    // Update graphics state (with scissor and possibly new bindings)
                    cmdList->setGraphicsState(graphicsState);

                    // Draw
                    nvrhi::DrawArguments args;
                    args.setVertexCount(pcmd->ElemCount); // For indexed draw, vertexCount is actually index count
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

} // namespace xray::render::ng
