#pragma once

#include "../xrRender/SH_Texture.h"

typedef struct
{
    bool DepthEnable;
    bool DepthWriteMask;
    D3DCMPFUNC DepthFunc;
    bool StencilEnable;
    unsigned int StencilMask;
    unsigned int StencilWriteMask;
    D3DSTENCILOP StencilFailOp;
    D3DSTENCILOP StencilDepthFailOp;
    D3DSTENCILOP StencilPassOp;
    D3DCMPFUNC StencilFunc;
    unsigned int StencilRef;
} D3D_DEPTH_STENCIL_STATE;

typedef struct
{
    bool BlendEnable;
    D3DBLEND SrcBlend;
    D3DBLEND DestBlend;
    D3DBLENDOP BlendOp;
    D3DBLEND SrcBlendAlpha;
    D3DBLEND DestBlendAlpha;
    D3DBLENDOP BlendOpAlpha;
    unsigned int ColorMask;
} D3D_BLEND_STATE;

class glState
{
private:
    D3DCULL rasterizerCullMode;
    D3D_DEPTH_STENCIL_STATE m_pDepthStencilState;
    D3D_BLEND_STATE m_pBlendState;

    GLuint m_samplerArray[CTexture::mtMaxCombinedShaderTextures];

public:
    glState();

    static glState* Create();

    void Apply();
    void Release();

    void UpdateRenderState(u32 name, u32 value);
    void UpdateSamplerState(u32 stage, u32 name, u32 value);
};
