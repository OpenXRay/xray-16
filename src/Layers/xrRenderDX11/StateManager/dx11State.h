#pragma once

#include "dx11SamplerStateCache.h"

namespace xray::render::RENDER_NAMESPACE
{
class SimulatorStates;

class dx11State
{
    //	Public interface
public:
    dx11State(); //	These have to be private bu new/xr_delete don't support this
    ~dx11State();

    static dx11State* Create(SimulatorStates& state_code);

    //	DX9 unified interface
    HRESULT Apply(CBackend& cmd_list);
    void Release();

    //	DX11 specific
    void UpdateStencilRef(u32 Ref) { m_uiStencilRef = Ref; }
    void UpdateAlphaRef(u32 Ref) { m_uiAlphaRef = Ref; }

    // Accessors for NVRHI/FrameGraph integration
    ID3DRasterizerState* GetRasterizerState() const { return m_pRasterizerState; }
    ID3DDepthStencilState* GetDepthStencilState() const { return m_pDepthStencilState; }
    ID3DBlendState* GetBlendState() const { return m_pBlendState; }
    u32 GetStencilRef() const { return m_uiStencilRef; }

    //	User restricted interface
private:
    typedef dx11SamplerStateCache::HArray tSamplerHArray;

private:
    static void InitSamplers(tSamplerHArray& SamplerArray, SimulatorStates& state_code, int iBaseSamplerIndex);

private:
    //	All states are supposed to live along all application lifetime
    ID3DRasterizerState* m_pRasterizerState{ nullptr }; //	Weak link
    ID3DDepthStencilState* m_pDepthStencilState{ nullptr }; //	Weak link
    ID3DBlendState* m_pBlendState{ nullptr }; //	Weak link

    tSamplerHArray m_VSSamplers;
    tSamplerHArray m_PSSamplers;
    tSamplerHArray m_GSSamplers;
    tSamplerHArray m_CSSamplers;
    tSamplerHArray m_HSSamplers;
    tSamplerHArray m_DSSamplers;

    u32 m_uiStencilRef{ u32(-1) };
    u32 m_uiAlphaRef{ 0 };

    //	Private data
private:
};
} // namespace xray::render::RENDER_NAMESPACE
