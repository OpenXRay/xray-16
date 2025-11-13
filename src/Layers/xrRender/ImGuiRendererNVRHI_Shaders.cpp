#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include "FrameGraph/ShaderLoader.h"
#include <d3dcompiler.h>

namespace xray::render::ng {

//=============================================================================
// Phase 4: Shader and Pipeline State Creation
//=============================================================================

bool ImGuiRendererNVRHI::CreateShaders()
{
    // Use the FrameGraph ShaderLoader to compile our ImGui shaders
    framegraph::ShaderLoader shaderLoader(m_renderDevice, m_renderDevice->GetSlangCompiler());

    // Load and compile shaders (we don't need bytecode for ImGui)
    m_vertexShader = shaderLoader.LoadVertexShader("imgui", "main", nullptr);

    if (!m_vertexShader)
    {
        Msg("! Failed to create ImGui vertex shader");
        return false;
    }

    m_pixelShader = shaderLoader.LoadPixelShader("imgui", "main", nullptr);

    if (!m_pixelShader)
    {
        Msg("! Failed to create ImGui pixel shader");
        return false;
    }

    Msg("* ImGui shaders compiled successfully");
    return true;
}

bool ImGuiRendererNVRHI::CreatePipelineState()
{
    // Create input layout matching ImDrawVert
    nvrhi::VertexAttributeDesc vertexAttributes[] = {
        // Position
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(ImDrawVert, pos))
            .setBufferIndex(0)
            .setElementStride(sizeof(ImDrawVert)),
        // UV
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(ImDrawVert, uv))
            .setBufferIndex(0)
            .setElementStride(sizeof(ImDrawVert)),
        // Color
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setOffset(offsetof(ImDrawVert, col))
            .setBufferIndex(0)
            .setElementStride(sizeof(ImDrawVert))
    };

    m_inputLayout = m_device->createInputLayout(
        vertexAttributes,
        uint32_t(std::size(vertexAttributes)),
        m_vertexShader
    );

    if (!m_inputLayout)
    {
        Msg("! Failed to create ImGui input layout");
        return false;
    }

    // Setup render state for UI rendering
    // Blend state for UI
    m_renderState.blendState.targets[0].setBlendEnable(true);
    m_renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
    m_renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::InvSrcAlpha);
    m_renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
    m_renderState.blendState.targets[0].setSrcBlendAlpha(nvrhi::BlendFactor::One);
    m_renderState.blendState.targets[0].setDestBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha);
    m_renderState.blendState.targets[0].setBlendOpAlpha(nvrhi::BlendOp::Add);
    m_renderState.blendState.targets[0].setColorWriteMask(nvrhi::ColorMask::All);

    // Rasterizer state
    m_renderState.rasterState.setFillMode(nvrhi::RasterFillMode::Solid);
    m_renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None); // No culling for UI
    m_renderState.rasterState.setScissorEnable(true); // Enable scissor for clipping
    m_renderState.rasterState.setDepthClipEnable(true);

    // Depth stencil state (no depth testing for UI)
    m_renderState.depthStencilState.setDepthTestEnable(false);
    m_renderState.depthStencilState.setDepthWriteEnable(false);
    m_renderState.depthStencilState.setStencilEnable(false);

    // Create binding layout for resources
    nvrhi::BindingLayoutDesc bindingLayoutDesc;
    bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
    bindingLayoutDesc.bindings = {
        // Constant buffer for projection matrix
        nvrhi::BindingLayoutItem::ConstantBuffer(0),
        // Texture
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        // Sampler
        nvrhi::BindingLayoutItem::Sampler(0)
    };

    m_bindingLayout = m_device->createBindingLayout(bindingLayoutDesc);

    if (!m_bindingLayout)
    {
        Msg("! Failed to create ImGui binding layout");
        return false;
    }

    // Create the pipeline state object
    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.setVertexShader(m_vertexShader);
    pipelineDesc.setPixelShader(m_pixelShader);
    pipelineDesc.setInputLayout(m_inputLayout);
    pipelineDesc.addBindingLayout(m_bindingLayout);
    pipelineDesc.setRenderState(m_renderState);
    pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);

    // Note: ImGui typically renders to the final output render target
    // which is RGBA8_UNORM format for LDR displays
    // If we need HDR support, we'd need to create a separate pipeline for RGBA16F

    // Create a framebuffer info to specify the render target format
    nvrhi::FramebufferInfoEx framebufferInfo;
    framebufferInfo.addColorFormat(nvrhi::Format::RGBA8_UNORM);  // Standard LDR format

    m_pipeline = m_device->createGraphicsPipeline(pipelineDesc, framebufferInfo);

    if (!m_pipeline)
    {
        Msg("! Failed to create ImGui graphics pipeline");
        return false;
    }

    // Note: Resource bindings will be created after the font texture is requested
    // by ImGui through the modern texture API (ProcessTextureRequests)

    return true;
}

bool ImGuiRendererNVRHI::CreateResourceBindings()
{
    // This should only be called after we have a valid font texture
    if (!m_fontTexture)
    {
        Msg("! CreateResourceBindings called without font texture");
        return false;
    }

    if (!m_bindingLayout)
    {
        Msg("! CreateResourceBindings called without binding layout");
        return false;
    }

    // Create resource bindings with the actual font texture
    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, m_constantBuffer),
        nvrhi::BindingSetItem::Texture_SRV(0, m_fontTexture),
        nvrhi::BindingSetItem::Sampler(0, m_fontSampler)
    };

    m_resourceBindings = m_device->createBindingSet(bindingSetDesc, m_bindingLayout);

    if (!m_resourceBindings)
    {
        Msg("! Failed to create ImGui resource bindings");
        return false;
    }

    Msg("* ImGui resource bindings created successfully");
    return true;
}

} // namespace xray::render::ng
