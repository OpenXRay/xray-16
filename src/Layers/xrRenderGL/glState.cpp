#include "stdafx.h"
#include "glState.h"

glState::glState()
{
    // Clear the sampler array
    memset(m_samplerArray, 0, CTexture::mtMaxCombinedShaderTextures * sizeof(GLuint));

    rasterizerCullMode = D3DCULL_CCW;

    m_pDepthStencilState.DepthEnable = TRUE;
    m_pDepthStencilState.DepthFunc = D3DCMP_LESSEQUAL;
    m_pDepthStencilState.DepthWriteMask = TRUE;
    m_pDepthStencilState.StencilEnable = TRUE;
    m_pDepthStencilState.StencilFailOp = D3DSTENCILOP_KEEP;
    m_pDepthStencilState.StencilDepthFailOp = D3DSTENCILOP_KEEP;
    m_pDepthStencilState.StencilPassOp = D3DSTENCILOP_KEEP;
    m_pDepthStencilState.StencilFunc = D3DCMP_ALWAYS;
    m_pDepthStencilState.StencilMask = 0xFFFFFFFF;
    m_pDepthStencilState.StencilWriteMask = 0xFFFFFFFF;
    m_pDepthStencilState.StencilRef = 0;

    m_pBlendState.BlendEnable = TRUE;
    m_pBlendState.SrcBlend = D3DBLEND_ONE;
    m_pBlendState.DestBlend = D3DBLEND_ZERO;
    m_pBlendState.SrcBlendAlpha = D3DBLEND_ONE;
    m_pBlendState.DestBlendAlpha = D3DBLEND_ZERO;
    m_pBlendState.BlendOp = D3DBLENDOP_ADD;
    m_pBlendState.BlendOpAlpha = D3DBLENDOP_ADD;
    m_pBlendState.ColorMask = 0xF;
}

glState::~glState()
{
    Release();
}

//	TODO: OGL: Does the render cache provide enough state management?
void glState::Apply()
{
    // TODO: OGL: Use glBindSamplers if ARB_multi_bind is supported.
    for (size_t stage = 0; stage < CTexture::mtMaxCombinedShaderTextures; stage++)
    {
        if (m_samplerArray[stage])
            glBindSampler(stage, m_samplerArray[stage]);
    }

    RCache.set_CullMode(rasterizerCullMode);
    RCache.set_Z(m_pDepthStencilState.DepthEnable);
    RCache.set_ZFunc(m_pDepthStencilState.DepthFunc);
    RCache.set_Stencil(
        m_pDepthStencilState.StencilEnable,
        m_pDepthStencilState.StencilFunc,
        m_pDepthStencilState.StencilRef,
        m_pDepthStencilState.StencilMask,
        m_pDepthStencilState.StencilWriteMask,
        m_pDepthStencilState.StencilFailOp,
        m_pDepthStencilState.StencilPassOp,
        m_pDepthStencilState.StencilDepthFailOp
    );

    CHK_GL(glDepthMask(m_pDepthStencilState.DepthWriteMask ? GL_TRUE : GL_FALSE));

    if (m_pBlendState.BlendEnable)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    CHK_GL(glBlendFuncSeparate(
        glStateUtils::ConvertBlendArg(m_pBlendState.SrcBlend),
        glStateUtils::ConvertBlendArg(m_pBlendState.DestBlend),
        glStateUtils::ConvertBlendArg(m_pBlendState.SrcBlendAlpha),
        glStateUtils::ConvertBlendArg(m_pBlendState.DestBlendAlpha)
    ));
    CHK_GL(glBlendEquationSeparate(
        glStateUtils::ConvertBlendOp(m_pBlendState.BlendOp),
        glStateUtils::ConvertBlendOp(m_pBlendState.BlendOpAlpha)
    ));

    RCache.set_ColorWriteEnable(m_pBlendState.ColorMask);
}

void glState::Release()
{
    // Delete any generated samplers in the array
    CHK_GL(glDeleteSamplers(CTexture::mtMaxCombinedShaderTextures, m_samplerArray));

    // Clear the sampler array
    memset(m_samplerArray, 0, CTexture::mtMaxCombinedShaderTextures * sizeof(GLuint));
}

void glState::UpdateRenderState(u32 name, u32 value)
{
    switch (name)
    {
    case D3DRS_CULLMODE:
        rasterizerCullMode = (D3DCULL)value;
        break;

    case D3DRS_ZENABLE:
        m_pDepthStencilState.DepthEnable = value ? TRUE : FALSE;
        break;

    case D3DRS_ZWRITEENABLE:
        m_pDepthStencilState.DepthWriteMask = value ? TRUE : FALSE;
        break;

    case D3DRS_ZFUNC:
        m_pDepthStencilState.DepthFunc = (D3DCMPFUNC)value;
        break;

    case D3DRS_STENCILENABLE:
        m_pDepthStencilState.StencilEnable = value ? TRUE : FALSE;
        break;

    case D3DRS_STENCILMASK:
        m_pDepthStencilState.StencilMask = (UINT)value;
        break;

    case D3DRS_STENCILWRITEMASK:
        m_pDepthStencilState.StencilWriteMask = (UINT)value;
        break;

    case D3DRS_STENCILFAIL:
        m_pDepthStencilState.StencilFailOp = (D3DSTENCILOP)value;
        break;

    case D3DRS_STENCILZFAIL:
        m_pDepthStencilState.StencilDepthFailOp = (D3DSTENCILOP)value;
        break;

    case D3DRS_STENCILPASS:
        m_pDepthStencilState.StencilPassOp = (D3DSTENCILOP)value;
        break;

    case D3DRS_STENCILFUNC:
        m_pDepthStencilState.StencilFunc = (D3DCMPFUNC)value;
        break;

    case D3DRS_STENCILREF:
        m_pDepthStencilState.StencilRef = value;
        break;

    case D3DRS_SRCBLEND:
        m_pBlendState.SrcBlend = (D3DBLEND)value;
        break;

    case D3DRS_DESTBLEND:
        m_pBlendState.DestBlend = (D3DBLEND)value;
        break;

    case D3DRS_BLENDOP:
        m_pBlendState.BlendOp = (D3DBLENDOP)value;
        break;

    case D3DRS_SRCBLENDALPHA:
        m_pBlendState.SrcBlendAlpha = (D3DBLEND)value;
        break;

    case D3DRS_DESTBLENDALPHA:
        m_pBlendState.DestBlendAlpha = (D3DBLEND)value;
        break;

    case D3DRS_BLENDOPALPHA:
        m_pBlendState.BlendOpAlpha = (D3DBLENDOP)value;
        break;

    case D3DRS_ALPHABLENDENABLE:
        m_pBlendState.BlendEnable = value ? TRUE : FALSE;
        break;

    case D3DRS_COLORWRITEENABLE:
    case D3DRS_COLORWRITEENABLE1:
    case D3DRS_COLORWRITEENABLE2:
    case D3DRS_COLORWRITEENABLE3:
        m_pBlendState.ColorMask = (UINT)value;
        break;

    case D3DRS_LIGHTING:
    case D3DRS_FOGENABLE:
    case D3DRS_ALPHATESTENABLE:
    case D3DRS_ALPHAREF:
        // Deprecated
        break;

    default:
        VERIFY(!"Render state not implemented");
        break;
    }
}

void glState::UpdateSamplerState(u32 stage, u32 name, u32 value)
{
    if (stage < 0 || CTexture::mtMaxCombinedShaderTextures < stage)
        return;

    GLint currentFilter = (GLint)GL_NEAREST;

    if (m_samplerArray[stage] == NULL)
        glGenSamplers(1, &m_samplerArray[stage]);
    else if (name == D3DSAMP_MINFILTER || name == D3DSAMP_MIPFILTER)
        glGetSamplerParameteriv(m_samplerArray[stage], GL_TEXTURE_MIN_FILTER, &currentFilter);

    switch (name)
    {
    case D3DSAMP_ADDRESSU: /* D3DTEXTUREADDRESS for U coordinate */
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_WRAP_S, glStateUtils::ConvertTextureAddressMode(
            value)));
        break;
    case D3DSAMP_ADDRESSV: /* D3DTEXTUREADDRESS for V coordinate */
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_WRAP_T, glStateUtils::ConvertTextureAddressMode(
            value)));
        break;
    case D3DSAMP_ADDRESSW: /* D3DTEXTUREADDRESS for W coordinate */
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_WRAP_R, glStateUtils::ConvertTextureAddressMode(
            value)));
        break;
    case D3DSAMP_BORDERCOLOR: /* D3DCOLOR */
    {
        GLuint color[] = {color_get_R(value), color_get_G(value), color_get_B(value), color_get_A(value)};
        CHK_GL(glSamplerParameterIuiv(m_samplerArray[stage], GL_TEXTURE_BORDER_COLOR, color));
    }
        break;
    case D3DSAMP_MAGFILTER: /* D3DTEXTUREFILTER filter to use for magnification */
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_MAG_FILTER, glStateUtils::ConvertTextureFilter(
            value)));
        break;
    case D3DSAMP_MINFILTER: /* D3DTEXTUREFILTER filter to use for minification */
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_MIN_FILTER, glStateUtils::ConvertTextureFilter(
            value, currentFilter)));
        break;
    case D3DSAMP_MIPFILTER: /* D3DTEXTUREFILTER filter to use between mipmaps during minification */
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_MIN_FILTER, glStateUtils::ConvertTextureFilter(
            value, currentFilter, true)));
        break;
    case D3DSAMP_MIPMAPLODBIAS: /* float Mipmap LOD bias */
        CHK_GL(glSamplerParameterf(m_samplerArray[stage], GL_TEXTURE_LOD_BIAS, *(float*)value));
        break;
    case D3DSAMP_MAXMIPLEVEL: /* DWORD 0..(n-1) LOD index of largest map to use (0 == largest) */
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_MAX_LEVEL, value));
        break;
    case D3DSAMP_MAXANISOTROPY: /* DWORD maximum anisotropy */
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_MAX_ANISOTROPY_EXT, value));
        break;
    case XRDX10SAMP_COMPARISONFILTER:
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_COMPARE_MODE, value ? (GLint)
            GL_COMPARE_REF_TO_TEXTURE : (GLint)GL_NONE));
        break;
    case XRDX10SAMP_COMPARISONFUNC:
        CHK_GL(glSamplerParameteri(m_samplerArray[stage], GL_TEXTURE_COMPARE_FUNC, value));
        break;
    default:
        VERIFY(!"Sampler state not implemented");
        break;
    }
}
