#include "stdafx.h"

#include "IDirect3DIndexBuffer9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DIndexBuffer9;

xrIDirect3DIndexBuffer9::xrIDirect3DIndexBuffer9(IDirect3DDevice9*	pIDirect3DDevice9, UINT iLength,DWORD iUsage,D3DFORMAT iFormat,D3DPOOL iPool)
	: m_refCount(0)
{
	APIDEBUG("xrIDirect3DIndexBuffer9::xrIDirect3DIndexBuffer9");
	m_pIDirect3DDevice9 = pIDirect3DDevice9;
	
//#ifdef D3D_DEBUG_INFO
	//-----------------------------------------------
	Name = NULL;
	Length = iLength;
	Usage = iUsage;
	Format = iFormat;
	Pool = iPool;
	Priority = 0;
	LockCount = 0;
	CreationCallStack = NULL;
	//-----------------------------------------------
//#endif
	
	switch(Format)
	{
	case D3DFMT_INDEX16:
		m_pBuffer = new BYTE[Length * 2];
		break;
	case D3DFMT_INDEX32:
		m_pBuffer = new BYTE[Length * 4];
		break;
	}

};
/*** IUnknown methods ***/
HRESULT			xrIDirect3DIndexBuffer9::QueryInterface( REFIID riid, void** ppvObj)
{
	APIDEBUG("xrIDirect3DIndexBuffer9::QueryInterface");
	if (riid == IID_IUnknown || riid == IID_IDirect3DIndexBuffer9)
	{
		*ppvObj = this;
		AddRef();
		return HRESULT_Proc(NOERROR);
	}
	return HRESULT_Proc(E_NOINTERFACE);
}

ULONG			xrIDirect3DIndexBuffer9::AddRef() 
{
	APIDEBUG("xrIDirect3DIndexBuffer9::AddRef");
	m_refCount++;
	return ULONG_Proc(m_refCount);
}

ULONG			xrIDirect3DIndexBuffer9::Release()
{
	APIDEBUG("xrIDirect3DIndexBuffer9::Release");
	m_refCount--;
	if (m_refCount < 0)
	{
		delete[]	m_pBuffer;
		delete this;
		return ULONG_Proc(-1);
	}
	return ULONG_Proc(m_refCount);
}

/*** IDirect3DResource9 methods ***/
HRESULT			__stdcall	xrIDirect3DIndexBuffer9::GetDevice		( IDirect3DDevice9** ppDevice)										
{
	APIDEBUG("xrIDirect3DIndexBuffer9::GetDevice");
	m_pIDirect3DDevice9->AddRef();
	*ppDevice = m_pIDirect3DDevice9;
	return HRESULT_Proc(S_OK);
};
HRESULT			__stdcall	xrIDirect3DIndexBuffer9::SetPrivateData	( REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)	{APIDEBUG("xrIDirect3DIndexBuffer9::SetPrivateData");return HRESULT_Proc(S_OK);};
HRESULT			__stdcall	xrIDirect3DIndexBuffer9::GetPrivateData	( REFGUID refguid,void* pData,DWORD* pSizeOfData)					{APIDEBUG("xrIDirect3DIndexBuffer9::GetPrivateData");return HRESULT_Proc(S_OK);};
HRESULT			__stdcall	xrIDirect3DIndexBuffer9::FreePrivateData( REFGUID refguid)													{APIDEBUG("xrIDirect3DIndexBuffer9::FreePrivateData");return HRESULT_Proc(S_OK);};
DWORD			__stdcall	xrIDirect3DIndexBuffer9::SetPriority	( DWORD PriorityNew)												{APIDEBUG("xrIDirect3DIndexBuffer9::SetPriority");DWORD old = Priority; Priority = PriorityNew;  return DWORD_Proc(old);};
DWORD			__stdcall	xrIDirect3DIndexBuffer9::GetPriority	()																	{APIDEBUG("xrIDirect3DIndexBuffer9::GetPriority");return DWORD_Proc(Priority);};
void			__stdcall	xrIDirect3DIndexBuffer9::PreLoad		()																	{APIDEBUG("xrIDirect3DIndexBuffer9::PreLoad");return VOID_proc();};
D3DRESOURCETYPE	__stdcall	xrIDirect3DIndexBuffer9::GetType		()																	{APIDEBUG("xrIDirect3DIndexBuffer9::GetType");return D3DRESOURCETYPE(0);};
HRESULT			__stdcall	xrIDirect3DIndexBuffer9::Lock			( UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags)		
{
	APIDEBUG("xrIDirect3DIndexBuffer9::Lock");
	*ppbData = m_pBuffer + OffsetToLock;
	return HRESULT_Proc(S_OK);
};
HRESULT			__stdcall	xrIDirect3DIndexBuffer9::Unlock			()																	{APIDEBUG("xrIDirect3DIndexBuffer9::Unlock");return HRESULT_Proc(S_OK);};
HRESULT			__stdcall	xrIDirect3DIndexBuffer9::GetDesc		( D3DINDEXBUFFER_DESC *pDesc)										{APIDEBUG("xrIDirect3DIndexBuffer9::GetDesc");return HRESULT_Proc(S_OK);};
