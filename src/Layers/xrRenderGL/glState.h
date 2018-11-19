#pragma once

#include "../xrRender/SH_Texture.h"

typedef struct
{
    BOOL DepthEnable;
    BOOL DepthWriteMask;
    D3DCMPFUNC DepthFunc;
    BOOL StencilEnable;
    UINT StencilMask;
    UINT StencilWriteMask;
    D3DSTENCILOP StencilFailOp;
    D3DSTENCILOP StencilDepthFailOp;
    D3DSTENCILOP StencilPassOp;
    D3DCMPFUNC StencilFunc;
    UINT StencilRef;
} D3D_DEPTH_STENCIL_STATE;

typedef struct
{
    BOOL BlendEnable;
    D3DBLEND SrcBlend;
    D3DBLEND DestBlend;
    D3DBLENDOP BlendOp;
    D3DBLEND SrcBlendAlpha;
    D3DBLEND DestBlendAlpha;
    D3DBLENDOP BlendOpAlpha;
    UINT ColorMask;
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
    ~glState();

    void Apply();
    void Release();

    void UpdateRenderState(u32 name, u32 value);
    void UpdateSamplerState(u32 stage, u32 name, u32 value);
};
