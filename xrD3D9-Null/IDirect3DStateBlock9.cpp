#include "stdafx.h"

#include "IDirect3DStateBlock9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DStateBlock9;

xrIDirect3DStateBlock9::xrIDirect3DStateBlock9(IDirect3DDevice9*	pIDirect3DDevice9)
: m_refCount(0)
{
	APIDEBUG("xrIDirect3DStateBlock9::xrIDirect3DStateBlock9");
	m_pIDirect3DDevice9 = pIDirect3DDevice9;
};
HRESULT			xrIDirect3DStateBlock9::QueryInterface( REFIID riid, void** ppvObj)
{
	APIDEBUG("xrIDirect3DStateBlock9::QueryInterface");
	if (riid == IID_IUnknown || riid == IID_IDirect3DStateBlock9)
	{
		*ppvObj = this;
		AddRef();
		return HRESULT_Proc(NOERROR);
	}
	return HRESULT_Proc(E_NOINTERFACE);
}

ULONG			xrIDirect3DStateBlock9::AddRef() 
{
	APIDEBUG("xrIDirect3DStateBlock9::AddRef");
	m_refCount++;
	return ULONG_Proc(m_refCount);
}

ULONG			xrIDirect3DStateBlock9::Release()
{
	APIDEBUG("xrIDirect3DStateBlock9::Release");
	m_refCount--;
	if (m_refCount < 0)
	{
		delete this;
		return ULONG_Proc(-1);
	}
	return ULONG_Proc(m_refCount);
}

/*** IDirect3DResource9 methods ***/
HRESULT			__stdcall	xrIDirect3DStateBlock9::GetDevice		( IDirect3DDevice9** ppDevice)										
{
	APIDEBUG("xrIDirect3DStateBlock9::GetDevice");
	m_pIDirect3DDevice9->AddRef();
	*ppDevice = m_pIDirect3DDevice9;
	return HRESULT_Proc(S_OK);
};

HRESULT			__stdcall	xrIDirect3DStateBlock9::Capture() { APIDEBUG("xrIDirect3DStateBlock9::Capture"); return HRESULT_Proc(S_OK); };
HRESULT			__stdcall	xrIDirect3DStateBlock9::Apply() { APIDEBUG("xrIDirect3DStateBlock9::Apply"); return HRESULT_Proc(S_OK);};