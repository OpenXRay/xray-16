#include "stdafx.h"

#include "IDirect3DVertexShader9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DVertexShader9;

xrIDirect3DVertexShader9::xrIDirect3DVertexShader9(IDirect3DDevice9*	pIDirect3DDevice9)
: m_refCount(0)
{
	APIDEBUG("xrIDirect3DVertexShader9::xrIDirect3DVertexShader9");
	m_pIDirect3DDevice9 = pIDirect3DDevice9;
};
HRESULT			xrIDirect3DVertexShader9::QueryInterface( REFIID riid, void** ppvObj)
{
	APIDEBUG("xrIDirect3DVertexShader9::QueryInterface");
	if (riid == IID_IUnknown || riid == IID_IDirect3DVertexShader9)
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

ULONG			xrIDirect3DVertexShader9::AddRef() 
{
	APIDEBUG("xrIDirect3DVertexShader9::AddRef");
	m_refCount++;
	return m_refCount;
}

ULONG			xrIDirect3DVertexShader9::Release()
{
	APIDEBUG("xrIDirect3DVertexShader9::Release");
	m_refCount--;
	if (m_refCount < 0)
	{
		delete this;
		return -1;
	}
	return m_refCount;
}

/*** IDirect3DResource9 methods ***/
HRESULT			__stdcall	xrIDirect3DVertexShader9::GetDevice		( IDirect3DDevice9** ppDevice)										
{
	APIDEBUG("xrIDirect3DVertexShader9::GetDevice");
	m_pIDirect3DDevice9->AddRef();
	*ppDevice = m_pIDirect3DDevice9;
	return S_OK;
};
HRESULT			__stdcall	xrIDirect3DVertexShader9::GetFunction(void* pData,UINT* pSizeOfData)
{
	APIDEBUG("xrIDirect3DVertexShader9::GetFunction");
	*pSizeOfData = 0;
	return S_OK;
};