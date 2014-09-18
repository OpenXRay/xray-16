#include "stdafx.h"

#include "IDirect3DVertexDeclaration9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DVertexDeclaration9;

xrIDirect3DVertexDeclaration9::xrIDirect3DVertexDeclaration9(IDirect3DDevice9*	pIDirect3DDevice9)
: m_refCount(0)
{
	APIDEBUG("xrIDirect3DVertexDeclaration9::xrIDirect3DVertexDeclaration9");
	m_pIDirect3DDevice9 = pIDirect3DDevice9;
};
HRESULT			xrIDirect3DVertexDeclaration9::QueryInterface( REFIID riid, void** ppvObj)
{
	APIDEBUG("xrIDirect3DVertexDeclaration9::QueryInterface");
	if (riid == IID_IUnknown || riid == IID_IDirect3DVertexDeclaration9)
	{
		*ppvObj = this;
		AddRef();
		return HRESULT_Proc(NOERROR);
	}
	return HRESULT_Proc(E_NOINTERFACE);
}

ULONG			xrIDirect3DVertexDeclaration9::AddRef() 
{
	APIDEBUG("xrIDirect3DVertexDeclaration9::AddRef");
	m_refCount++;
	return ULONG_Proc(m_refCount);
}

ULONG			xrIDirect3DVertexDeclaration9::Release()
{
	APIDEBUG("xrIDirect3DVertexDeclaration9::Release");
	m_refCount--;
	if (m_refCount < 0)
	{
		delete this;
		return ULONG_Proc(-1);
	}
	return ULONG_Proc(m_refCount);
}

/*** IDirect3DResource9 methods ***/
HRESULT			__stdcall	xrIDirect3DVertexDeclaration9::GetDevice		( IDirect3DDevice9** ppDevice)										
{
	APIDEBUG("xrIDirect3DVertexDeclaration9::GetDevice");
	m_pIDirect3DDevice9->AddRef();
	*ppDevice = m_pIDirect3DDevice9;
	return HRESULT_Proc(S_OK);
};
HRESULT			__stdcall	xrIDirect3DVertexDeclaration9::GetDeclaration( D3DVERTEXELEMENT9* pD3DVertexElement9,UINT* pNumElements)
{
	APIDEBUG("xrIDirect3DVertexDeclaration9::GetDeclaration");
	return HRESULT_Proc(S_OK);
};