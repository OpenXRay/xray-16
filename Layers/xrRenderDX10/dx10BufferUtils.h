#ifndef	dx10BufferUtils_included
#define	dx10BufferUtils_included
#pragma once
#if defined(USE_DX10) || defined(USE_DX11)

namespace dx10BufferUtils
{
HRESULT	CreateVertexBuffer( ID3DVertexBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
HRESULT	CreateIndexBuffer( ID3DIndexBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
HRESULT	CreateConstantBuffer( ID3DBuffer** ppBuffer, UINT DataSize);
void	ConvertVertexDeclaration( const xr_vector<D3DVERTEXELEMENT9> &declIn, xr_vector<D3D_INPUT_ELEMENT_DESC> &declOut);
};

#endif	//	USE_DX10
#endif	//	dx10BufferUtils_included