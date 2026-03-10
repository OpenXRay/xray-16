#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"
#include "FrameGraph/ShaderLoader.h"
#include "FrameGraph/PassResourceCache.h"
#include "FrameGraph/BindingSetBuilder.h"
#include <d3dcompiler.h>

namespace xray::render::ng {

bool ImGuiRendererNVRHI::CreateShaders()
{
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
    nvrhi::VertexAttributeDesc vertexAttributes[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(ImDrawVert, pos))
            .setBufferIndex(0)
            .setElementStride(sizeof(ImDrawVert)),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(ImDrawVert, uv))
            .setBufferIndex(0)
            .setElementStride(sizeof(ImDrawVert)),
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

    m_renderState.blendState.targets[0].setBlendEnable(true);
    m_renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
    m_renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::InvSrcAlpha);
    m_renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
    m_renderState.blendState.targets[0].setSrcBlendAlpha(nvrhi::BlendFactor::One);
    m_renderState.blendState.targets[0].setDestBlendAlpha(nvrhi::BlendFactor::InvSrcAlpha);
    m_renderState.blendState.targets[0].setBlendOpAlpha(nvrhi::BlendOp::Add);
    m_renderState.blendState.targets[0].setColorWriteMask(nvrhi::ColorMask::All);

    m_renderState.rasterState.setFillMode(nvrhi::RasterFillMode::Solid);
    m_renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);
    m_renderState.rasterState.setScissorEnable(true);
    m_renderState.rasterState.setDepthClipEnable(true);

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

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.setVertexShader(m_vertexShader);
    pipelineDesc.setPixelShader(m_pixelShader);
    pipelineDesc.setInputLayout(m_inputLayout);
    pipelineDesc.addBindingLayout(m_bindingLayout);
    pipelineDesc.setRenderState(m_renderState);
    pipelineDesc.setPrimType(nvrhi::PrimitiveType::TriangleList);

    nvrhi::FramebufferInfoEx framebufferInfo;
    framebufferInfo.addColorFormat(nvrhi::Format::RGBA8_UNORM);

    m_pipeline = m_device->createGraphicsPipeline(pipelineDesc, framebufferInfo);

    if (!m_pipeline)
    {
        Msg("! Failed to create ImGui graphics pipeline");
        return false;
    }

    return true;
}

bool ImGuiRendererNVRHI::CreateResourceBindings()
{
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

}
