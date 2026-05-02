#include "stdafx.h"
#include "dx11State.h"

#include "dx11StateCache.h"
#include "Layers/xrRenderDX11/dx11SimulatorStateConvert.h"

namespace xray::render::fg
{
dx11State::dx11State()
{
}

dx11State::~dx11State()
{
    //	m_pRasterizerState is a weak link
    //	m_pDepthStencilState is a weak link
    //	m_pBlendState is a weak link
}

dx11State* dx11State::Create(SimulatorStates& state_code)
{
    dx11State* pState = xr_new<dx11State>();

    UpdateStateDX11(state_code, *pState);

    // TODO: possibly lock on state managers access

    pState->m_pRasterizerState   = RSManager.GetState(state_code);
    pState->m_pDepthStencilState = DSSManager.GetState(state_code);
    pState->m_pBlendState        = BSManager.GetState(state_code);

    //	Create samplers here
    {
        InitSamplers(pState->m_VSSamplers, state_code, CTexture::rstVertex);
        InitSamplers(pState->m_PSSamplers, state_code, CTexture::rstPixel);
        InitSamplers(pState->m_GSSamplers, state_code, CTexture::rstGeometry);
        InitSamplers(pState->m_HSSamplers, state_code, CTexture::rstHull);
        InitSamplers(pState->m_DSSamplers, state_code, CTexture::rstDomain);
        InitSamplers(pState->m_CSSamplers, state_code, CTexture::rstCompute);
    }

    return pState;
}


void dx11State::Release()
{
    dx11State* pState = this;
    xr_delete/*<dx11State>*/(pState);
}

void dx11State::InitSamplers(tSamplerHArray& SamplerArray, SimulatorStates& state_code, int iBaseSamplerIndex)
{
    D3D_SAMPLER_DESC descArray[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
    bool SamplerUsed[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];

    for (int i = 0; i < D3D_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
    {
        SamplerUsed[i] = false;
        dx11StateUtils::ResetDescription(descArray[i]);
    }

    UpdateDescDX11(state_code, descArray, SamplerUsed, iBaseSamplerIndex);

    int iMaxSampler = D3D_COMMONSHADER_SAMPLER_SLOT_COUNT - 1;
    for (; iMaxSampler > -1; --iMaxSampler)
    {
        if (SamplerUsed[iMaxSampler])
            break;
    }

    if (iMaxSampler > -1)
    {
        SamplerArray.reserve(iMaxSampler + 1);
        for (int i = 0; i <= iMaxSampler; ++i)
        {
            if (SamplerUsed[i])
                SamplerArray.push_back(SSManager.GetState(descArray[i]));
            else
                SamplerArray.push_back(u32(dx11SamplerStateCache::hInvalidHandle));
        }
    }
}
} // namespace xray::render::fg
