#ifndef dx10State_included
#define dx10State_included
#pragma once

class SimulatorStates;

#include "dx10SamplerStateCache.h"

class dx10State
{
    //	Public interface
public:
    dx10State(); //	These have to be private bu new/xr_delete don't support this
    ~dx10State();

    static dx10State* Create(SimulatorStates& state_code);

    //	DX9 unified interface
    HRESULT Apply();
    void Release();

    //	DX10 specific
    void UpdateStencilRef(u32 Ref) { m_uiStencilRef = Ref; }
    void UpdateAlphaRef(u32 Ref) { m_uiAlphaRef = Ref; }
    //	User restricted interface
private:
    typedef dx10SamplerStateCache::HArray tSamplerHArray;

private:
    static void InitSamplers(tSamplerHArray& SamplerArray, SimulatorStates& state_code, int iBaseSamplerIndex);

private:
    //	All states are supposed to live along all application lifetime
    ID3DRasterizerState* m_pRasterizerState; //	Weak link
    ID3DDepthStencilState* m_pDepthStencilState; //	Weak link
    ID3DBlendState* m_pBlendState; //	Weak link

    tSamplerHArray m_VSSamplers;
    tSamplerHArray m_PSSamplers;
    tSamplerHArray m_GSSamplers;
#ifdef USE_DX11
    tSamplerHArray m_CSSamplers;
    tSamplerHArray m_HSSamplers;
    tSamplerHArray m_DSSamplers;
#endif

    u32 m_uiStencilRef;
    u32 m_uiAlphaRef;

    //	Private data
private:
};

#endif //	dx10State_included
