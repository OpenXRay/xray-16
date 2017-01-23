#include "stdafx.h"
#include "dx10StateCache.h"

dx10StateCache<ID3DRasterizerState, D3D_RASTERIZER_DESC>		RSManager;
dx10StateCache<ID3DDepthStencilState, D3D_DEPTH_STENCIL_DESC>	DSSManager;
dx10StateCache<ID3DBlendState, D3D_BLEND_DESC>					BSManager;

template <class IDeviceState, class StateDecs>
dx10StateCache<IDeviceState, StateDecs>
::dx10StateCache()
{
	static const int iMasRSStates = 10;
	m_StateArray.reserve(iMasRSStates);
}

template <class IDeviceState, class StateDecs>
dx10StateCache<IDeviceState, StateDecs>
::~dx10StateCache()
{
	ClearStateArray();
//	VERIFY(m_StateArray.empty());
}

/*
template <class IDeviceState, class StateDecs>
void 
dx10StateCache<IDeviceState, StateDecs>
::FlushStates()
{
	ClearStateArray();
}
*/

template <class IDeviceState, class StateDecs>
void 
dx10StateCache<IDeviceState, StateDecs>
::ClearStateArray()
{
	for (u32 i=0; i<m_StateArray.size(); ++i)
	{
		_RELEASE(m_StateArray[i].m_pState);
	}

	m_StateArray.clear_not_free();
}

template <>
void 
dx10StateCache<ID3DRasterizerState, D3D_RASTERIZER_DESC>
::CreateState( D3D_RASTERIZER_DESC desc, ID3DRasterizerState** ppIState )
{
	CHK_DX(HW.pDevice->CreateRasterizerState( &desc, ppIState));

	//	TODO: DX10: Remove this.
#ifdef	DEBUG
	Msg("ID3DRasterizerState #%d created.", m_StateArray.size());
#endif	//	DEBUG
}

template <>
void 
dx10StateCache<ID3DDepthStencilState, D3D_DEPTH_STENCIL_DESC>
::CreateState( D3D_DEPTH_STENCIL_DESC desc, ID3DDepthStencilState** ppIState )
{
	CHK_DX(HW.pDevice->CreateDepthStencilState( &desc, ppIState));

	//	TODO: DX10: Remove this.
#ifdef	DEBUG
	Msg("ID3DDepthStencilState #%d created.", m_StateArray.size());
#endif	//	DEBUG
}

template <>
void 
dx10StateCache<ID3DBlendState, D3D_BLEND_DESC>
::CreateState( D3D_BLEND_DESC desc, ID3DBlendState** ppIState )
{
	CHK_DX(HW.pDevice->CreateBlendState( &desc, ppIState));

	//	TODO: DX10: Remove this.
#ifdef	DEBUG
	Msg("ID3DBlendState #%d created.", m_StateArray.size());
#endif	//	DEBUG
}

/*
template <>
void 
dx10StateCache<ID3DxxRasterizerState, D3D_RASTERIZER_DESC>
::ResetDescription( D3D_RASTERIZER_DESC &desc )
{
	ZeroMemory(&desc, sizeof(desc));
	desc.FillMode = D3D_FILL_SOLID;
	desc.CullMode = D3Dxx_CULL_BACK;
	desc.FrontCounterClockwise = FALSE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0.0f;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthClipEnable = TRUE;
	desc.ScissorEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
}

template <>
void 
dx10StateCache<ID3DxxDepthStencilState, D3D_DEPTH_STENCIL_DESC>
::ResetDescription( D3D_DEPTH_STENCIL_DESC &desc )
{
	ZeroMemory(&desc, sizeof(desc));
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = D3D_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3Dxx_COMPARISON_LESS;
	desc.StencilEnable = TRUE;
	desc.StencilReadMask = 0xFF;
	desc.StencilWriteMask = 0xFF;

	desc.FrontFace.StencilFailOp = D3Dxx_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3Dxx_STENCIL_OP_KEEP;
	desc.FrontFace.StencilPassOp = D3Dxx_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFunc = D3Dxx_COMPARISON_ALWAYS;

	desc.BackFace.StencilFailOp = D3Dxx_STENCIL_OP_KEEP;
	desc.BackFace.StencilDepthFailOp = D3Dxx_STENCIL_OP_KEEP;
	desc.BackFace.StencilPassOp = D3Dxx_STENCIL_OP_KEEP;
	desc.BackFace.StencilFunc = D3Dxx_COMPARISON_ALWAYS;
}

template <>
void 
dx10StateCache< ID3DxxBlendState , D3D_BLEND_DESC >
::ResetDescription( D3D_BLEND_DESC &desc )
{
	ZeroMemory(&desc, sizeof(desc));

	desc.AlphaToCoverageEnable = FALSE;
	desc.SrcBlend = D3Dxx_BLEND_ONE;
	desc.DestBlend = D3Dxx_BLEND_ZERO;
	desc.BlendOp = D3Dxx_BLEND_OP_ADD;
	desc.SrcBlendAlpha = D3Dxx_BLEND_ONE;
	desc.DestBlendAlpha = D3Dxx_BLEND_ZERO;
	desc.BlendOpAlpha = D3Dxx_BLEND_OP_ADD;

	for ( int i=0; i<8; ++i)
	{
		desc.BlendEnable[i] = FALSE;
		desc.RenderTargetWriteMask[i] = D3Dxx_COLOR_WRITE_ENABLE_ALL;
	}
}
*/