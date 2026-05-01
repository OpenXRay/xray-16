#include "stdafx.h"
#include "dx11StateCache.h"

namespace xray::render::fg
{
dx11StateCache<ID3DRasterizerState, D3D_RASTERIZER_DESC> RSManager;
dx11StateCache<ID3DDepthStencilState, D3D_DEPTH_STENCIL_DESC> DSSManager;
dx11StateCache<ID3DBlendState, D3D_BLEND_DESC> BSManager;

template <class IDeviceState, class StateDecs>
dx11StateCache<IDeviceState, StateDecs>::dx11StateCache()
{
    static const int iMasRSStates = 10;
    m_StateArray.reserve(iMasRSStates);
}

template <class IDeviceState, class StateDecs>
dx11StateCache<IDeviceState, StateDecs>::~dx11StateCache()
{
    ClearStateArray();
    //	VERIFY(m_StateArray.empty());
}

/*
template <class IDeviceState, class StateDecs>
void
dx11StateCache<IDeviceState, StateDecs>
::FlushStates()
{
    ClearStateArray();
}
*/

template <class IDeviceState, class StateDecs>
void dx11StateCache<IDeviceState, StateDecs>::ClearStateArray()
{
    for (u32 i = 0; i < m_StateArray.size(); ++i)
    {
        _RELEASE(m_StateArray[i].m_pState);
    }

    m_StateArray.clear();
}

template <>
void dx11StateCache<ID3DRasterizerState, D3D_RASTERIZER_DESC>::CreateState(
    D3D_RASTERIZER_DESC desc, ID3DRasterizerState** ppIState)
{
    CHK_DX(HW.pDevice->CreateRasterizerState(&desc, ppIState));

//	TODO: DX11: Remove this.
#ifdef DEBUG
    Msg("ID3DRasterizerState #%d created.", m_StateArray.size());
#endif //	DEBUG
}

template <>
void dx11StateCache<ID3DDepthStencilState, D3D_DEPTH_STENCIL_DESC>::CreateState(
    D3D_DEPTH_STENCIL_DESC desc, ID3DDepthStencilState** ppIState)
{
    CHK_DX(HW.pDevice->CreateDepthStencilState(&desc, ppIState));

//	TODO: DX11: Remove this.
#ifdef DEBUG
    Msg("ID3DDepthStencilState #%d created.", m_StateArray.size());
#endif //	DEBUG
}

template <>
void dx11StateCache<ID3DBlendState, D3D_BLEND_DESC>::CreateState(D3D_BLEND_DESC desc, ID3DBlendState** ppIState)
{
    CHK_DX(HW.pDevice->CreateBlendState(&desc, ppIState));

//	TODO: DX11: Remove this.
#ifdef DEBUG
    Msg("ID3DBlendState #%d created.", m_StateArray.size());
#endif //	DEBUG
}

}
