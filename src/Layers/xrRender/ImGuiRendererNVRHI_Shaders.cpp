#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include "FrameGraph/ShaderLoader.h"
#include "FrameGraph/PassResourceCache.h"
#include "FrameGraph/BindingSetBuilder.h"

namespace xray::render::fg {

//=============================================================================
// Phase 4: Shader and Pipeline State Creation
//=============================================================================

bool ImGuiRendererNVRHI::CreateShaders()
{
    // Use the FrameGraph ShaderLoader to compile our ImGui shaders
    m_vertexShader = GEnv.Render->GetShaderLoader()->LoadVertexShader("imgui", "main").handle;

    if (!m_vertexShader)
    {
        Msg("! Failed to create ImGui vertex shader");
        return false;
    }

    m_pixelShader = GEnv.Render->GetShaderLoader()->LoadPixelShader("imgui", "main").handle;

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

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* vsRefl = shaderLoader->GetCachedReflection("imgui", ".vs");
    auto* psRefl = shaderLoader->GetCachedReflection("imgui", ".ps");
    auto& cache = framegraph::GetPassResourceCache();

    if (vsRefl && psRefl)
        m_bindingLayout = cache.GetOrCreateBindingLayoutFromReflection("ImGui", *vsRefl, *psRefl, m_device);
    else if (psRefl)
        m_bindingLayout = cache.GetOrCreateBindingLayoutFromReflection("ImGui", *psRefl, m_device);
    else
        Msg("! ImGui shader reflection not available");

    if (!m_bindingLayout)
    {
        Msg("! Failed to create ImGui binding layout");
        return false;
    }

    m_pipelineDesc = nvrhi::GraphicsPipelineDesc();
    m_pipelineDesc.setVertexShader(m_vertexShader);
    m_pipelineDesc.setPixelShader(m_pixelShader);
    m_pipelineDesc.setInputLayout(m_inputLayout);
    m_pipelineDesc.addBindingLayout(m_bindingLayout);
    m_pipelineDesc.setRenderState(m_renderState);
    m_pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);

    return true;
}

nvrhi::IGraphicsPipeline* ImGuiRendererNVRHI::GetOrCreatePipelineForFramebuffer(nvrhi::IFramebuffer* fb)
{
    if (!fb || !m_pipelineDesc.VS)
        return nullptr;

    const auto& fbInfo = fb->getFramebufferInfo();
    auto& cache = framegraph::GetPassResourceCache();
    auto handle = cache.GetOrCreatePipeline("ImGui", m_pipelineDesc, fbInfo, m_device.Get());
    return handle.Get();
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

    auto* imgShaderLoader = GEnv.Render->GetShaderLoader();
    auto* imgVsRefl = imgShaderLoader->GetCachedReflection("imgui", ".vs");
    auto* imgPsRefl = imgShaderLoader->GetCachedReflection("imgui", ".ps");

    if (imgVsRefl && imgPsRefl) {
        framegraph::BindingSetBuilder bsb(*imgVsRefl, *imgPsRefl, m_device, "ImGui.Init");
        bsb.ConstantBuffer("vertexBuffer", m_constantBuffer)
           .Texture("texture0", m_fontTexture);
        m_resourceBindings = m_device->createBindingSet(bsb.Build(), m_bindingLayout);
    } else if (imgPsRefl) {
        framegraph::BindingSetBuilder bsb(*imgPsRefl, m_device, "ImGui.PerDraw");
        bsb.ConstantBuffer("vertexBuffer", m_constantBuffer)
           .Texture("texture0", m_fontTexture);
        m_resourceBindings = m_device->createBindingSet(bsb.Build(), m_bindingLayout);
    }

    if (!m_resourceBindings)
    {
        Msg("! Failed to create ImGui resource bindings");
        return false;
    }

    Msg("* ImGui resource bindings created successfully");
    return true;
}

} // namespace xray::render::fg
