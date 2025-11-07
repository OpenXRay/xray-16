#include "stdafx.h"
#include "ImGuiRendererNVRHI.h"

namespace xray::render::ng {

// ImGui vertex shader HLSL source
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

// ImGui pixel shader HLSL source
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

//=============================================================================
// Phase 4: Shader and Pipeline State Creation
//=============================================================================

bool ImGuiRendererNVRHI::CreateShaders()
{
    // TODO: Proper shader compilation integration
    // NVRHI expects precompiled bytecode, not source code
    // For now, we'll create dummy shaders and integrate with the actual shader system later

    // Create vertex shader
    nvrhi::ShaderDesc vsDesc;
    vsDesc.setDebugName("ImGui Vertex Shader");
    vsDesc.setEntryName("main");

    // NOTE: This is a placeholder - actual implementation needs to:
    // 1. Compile HLSL to bytecode using D3DCompile or DXC
    // 2. Pass the compiled bytecode to createShader
    // 3. Or integrate with the existing shader system

    // Temporary: Create empty shader (will fail at runtime)
    const char dummyBytecode[] = { 0x01, 0x02, 0x03, 0x04 };
    m_vertexShader = m_device->createShader(vsDesc, dummyBytecode, sizeof(dummyBytecode));

    if (!m_vertexShader)
    {
        Msg("! Failed to create ImGui vertex shader");
        // Don't fail here, we'll fix this later
        // return false;
    }

    // Create pixel shader
    nvrhi::ShaderDesc psDesc;
    psDesc.setDebugName("ImGui Pixel Shader");
    psDesc.setEntryName("main");

    m_pixelShader = m_device->createShader(psDesc, dummyBytecode, sizeof(dummyBytecode));

    if (!m_pixelShader)
    {
        Msg("! Failed to create ImGui pixel shader");
        // Don't fail here, we'll fix this later
        // return false;
    }

    // TODO: Implement proper shader compilation
    Msg("* ImGui shaders need proper compilation integration");

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
            .setName("TEXCOORD0")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(offsetof(ImDrawVert, uv))
            .setBufferIndex(0)
            .setElementStride(sizeof(ImDrawVert)),
        // Color
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

    // Note: Render target format will be set at render time based on current framebuffer
    // For now, assume standard format
    // TODO: Get actual render target format from the framebuffer

    m_pipeline = m_device->createGraphicsPipeline(pipelineDesc, /* framebuffer */ nullptr);

    if (!m_pipeline)
    {
        Msg("! Failed to create ImGui graphics pipeline");
        return false;
    }

    // Create initial resource bindings
    nvrhi::BindingSetDesc bindingSetDesc;
    bindingSetDesc.bindings = {
        // Will be updated with actual resources during rendering
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

} // namespace xray::render::ng