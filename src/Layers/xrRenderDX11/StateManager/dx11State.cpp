#include "stdafx.h"
#include "dx11State.h"

#include "dx11StateCache.h"

dx11State::dx11State()
    : m_pRasterizerState(0), m_pDepthStencilState(0), m_pBlendState(0), m_uiStencilRef(u32(-1)), m_uiAlphaRef(0)
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

    state_code.UpdateState(*pState);

    pState->m_pRasterizerState = RCache.RSManager.GetState(state_code);
    pState->m_pDepthStencilState = RCache.DSSManager.GetState(state_code);
    pState->m_pBlendState = RCache.BSManager.GetState(state_code);
    // ID3DxxDevice::CreateSamplerState

    //	Create samplers here
    {
        InitSamplers(pState->m_VSSamplers, state_code, CTexture::rstVertex);
        InitSamplers(pState->m_PSSamplers, state_code, CTexture::rstPixel);
        InitSamplers(pState->m_GSSamplers, state_code, CTexture::rstGeometry);
#ifdef USE_DX11
        InitSamplers(pState->m_HSSamplers, state_code, CTexture::rstHull);
        InitSamplers(pState->m_DSSamplers, state_code, CTexture::rstDomain);
        InitSamplers(pState->m_CSSamplers, state_code, CTexture::rstCompute);
#endif
    }

    return pState;
}

HRESULT dx11State::Apply()
{
    VERIFY(m_pRasterizerState);
    RCache.StateManager.SetRasterizerState(m_pRasterizerState);
    VERIFY(m_pDepthStencilState);
    RCache.StateManager.SetDepthStencilState(m_pDepthStencilState);
    if (m_uiStencilRef != -1)
        RCache.StateManager.SetStencilRef(m_uiStencilRef);
    VERIFY(m_pBlendState);
    RCache.StateManager.SetBlendState(m_pBlendState);
    RCache.StateManager.SetAlphaRef(m_uiAlphaRef);

    RCache.SSManager.GSApplySamplers(m_GSSamplers);
    RCache.SSManager.VSApplySamplers(m_VSSamplers);
    RCache.SSManager.PSApplySamplers(m_PSSamplers);
#ifdef USE_DX11
    RCache.SSManager.HSApplySamplers(m_HSSamplers);
    RCache.SSManager.DSApplySamplers(m_DSSamplers);
    RCache.SSManager.CSApplySamplers(m_CSSamplers);
#endif

    //	static const float BlendFactor[4] = {0.000f, 0.000f, 0.000f, 0.000f};
    //	static const UINT SampleMask = 0xffffffff;

    //	VERIFY(m_pRasterizerState);
    //	HW.pDevice->RSSetState(m_pRasterizerState);
    //	VERIFY(m_pDepthStencilState);
    //	HW.pDevice->OMSetDepthStencilState(m_pDepthStencilState, m_uiStencilRef);
    //	VERIFY(m_pBlendState);
    //	HW.pDevice->OMSetBlendState(m_pBlendState, BlendFactor, SampleMask);

    return S_OK;
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

    state_code.UpdateDesc(descArray, SamplerUsed, iBaseSamplerIndex);

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
                SamplerArray.push_back(RCache.SSManager.GetState(descArray[i]));
            else
                SamplerArray.push_back(u32(dx11SamplerStateCache::hInvalidHandle));
        }
    }
}
