#include "stdafx.h"

#include "IDirect3DPixelShader9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DPixelShader9;

xrIDirect3DPixelShader9::xrIDirect3DPixelShader9(IDirect3DDevice9*	pIDirect3DDevice9)
: m_refCount(0)
{
	APIDEBUG("xrIDirect3DPixelShader9::xrIDirect3DPixelShader9");
	m_pIDirect3DDevice9 = pIDirect3DDevice9;
};
HRESULT			xrIDirect3DPixelShader9::QueryInterface( REFIID riid, void** ppvObj)
{
	APIDEBUG("xrIDirect3DPixelShader9::QueryInterface");
	if (riid == IID_IUnknown || riid == IID_IDirect3DPixelShader9)
	{
		*ppvObj = this;
		AddRef();
		return HRESULT_Proc(NOERROR);
	}
	return HRESULT_Proc(E_NOINTERFACE);
}

ULONG			xrIDirect3DPixelShader9::AddRef() 
{
	APIDEBUG("xrIDirect3DPixelShader9::AddRef");
	m_refCount++;
	return ULONG_Proc(m_refCount);
}

ULONG			xrIDirect3DPixelShader9::Release()
{
	APIDEBUG("xrIDirect3DPixelShader9::Release");
	m_refCount--;
	if (m_refCount < 0)
	{
		delete this;
		return ULONG_Proc(-1);
	}
	return ULONG_Proc(m_refCount);
}

/*** IDirect3DResource9 methods ***/
HRESULT			__stdcall	xrIDirect3DPixelShader9::GetDevice		( IDirect3DDevice9** ppDevice)										
{
	APIDEBUG("xrIDirect3DPixelShader9::GetDevice");
	m_pIDirect3DDevice9->AddRef();
	*ppDevice = m_pIDirect3DDevice9;
	return HRESULT_Proc(S_OK);
};
HRESULT			__stdcall	xrIDirect3DPixelShader9::GetFunction(void* pData,UINT* pSizeOfData)
{
	APIDEBUG("xrIDirect3DPixelShader9::GetFunction");
	*pSizeOfData = 0;
	return HRESULT_Proc(S_OK);
};