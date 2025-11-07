#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"

namespace xray::render::ng {

static const char* s_ImGuiVertexShader = R"(
cbuffer vertexBuffer : register(b0) {
    float4x4 ProjectionMatrix;
};

struct VS_INPUT {
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv = input.uv;
    return output;
}
)";

static const char* s_ImGuiPixelShader = R"(
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(PS_INPUT input) : SV_Target {
    float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
    return out_col;
}
)";

bool ImGuiRendererNVRHI::CreateShaders()
{

    nvrhi::ShaderDesc vsDesc;
    vsDesc.setDebugName("ImGui Vertex Shader");
    vsDesc.setEntryName("main");

    const char dummyBytecode[] = { 0x01, 0x02, 0x03, 0x04 };
    m_vertexShader = m_device->createShader(vsDesc, dummyBytecode, sizeof(dummyBytecode));

    if (!m_vertexShader)
    {
        Msg("! Failed to create ImGui vertex shader");
    }

    nvrhi::ShaderDesc psDesc;
    psDesc.setDebugName("ImGui Pixel Shader");
    psDesc.setEntryName("main");

    m_pixelShader = m_device->createShader(psDesc, dummyBytecode, sizeof(dummyBytecode));

    if (!m_pixelShader)
    {
        Msg("! Failed to create ImGui pixel shader");
    }

    Msg("* ImGui shaders need proper compilation integration");

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
            .setName("TEXCOORD0")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(ImDrawVert, uv))
            .setBufferIndex(0)
            .setElementStride(sizeof(ImDrawVert)),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR0")
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

    nvrhi::BindingLayoutDesc bindingLayoutDesc;
    bindingLayoutDesc.visibility = nvrhi::ShaderType::All;
    bindingLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::ConstantBuffer(0),
        nvrhi::BindingLayoutItem::Texture_SRV(0),
        nvrhi::BindingLayoutItem::Sampler(0)
    };

    m_bindingLayout = m_device->createBindingLayout(bindingLayoutDesc);

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

    m_pipeline = m_device->createGraphicsPipeline(pipelineDesc, nullptr);

    if (!m_pipeline)
    {
        Msg("! Failed to create ImGui graphics pipeline");
        return false;
    }

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

    return true;
}

}