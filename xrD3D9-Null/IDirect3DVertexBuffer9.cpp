#include "stdafx.h"

#include "IDirect3DVertexBuffer9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DVertexBuffer9;

xrIDirect3DVertexBuffer9::xrIDirect3DVertexBuffer9(IDirect3DDevice9*	pIDirect3DDevice9, UINT iLength,DWORD iUsage,DWORD iFVF,D3DPOOL iPool)
: m_refCount(0)
{
	APIDEBUG("xrIDirect3DVertexBuffer9::xrIDirect3DVertexBuffer9");
	m_pIDirect3DDevice9 = pIDirect3DDevice9;
	//-----------------------------------------------
	Name = NULL;
	Length = iLength;
	Usage = iUsage;
	m_FVF = iFVF;
	Pool = iPool;
	Priority = 0;
	LockCount = 0;
	CreationCallStack = NULL;
	//-----------------------------------------------	
	m_pBuffer = new BYTE[Length];

};
/*** IUnknown methods ***/
HRESULT			xrIDirect3DVertexBuffer9::QueryInterface( REFIID riid, void** ppvObj)
{
	APIDEBUG("xrIDirect3DVertexBuffer9::QueryInterface");
	if (riid == IID_IUnknown || riid == IID_IDirect3DVertexBuffer9)
	{
		*ppvObj = this;
		AddRef();
		return HRESULT_Proc(NOERROR);
	}
	return HRESULT_Proc(E_NOINTERFACE);
}

ULONG			xrIDirect3DVertexBuffer9::AddRef() 
{
	APIDEBUG("xrIDirect3DVertexBuffer9::AddRef");
	m_refCount++;
	return ULONG_Proc(m_refCount);
}

ULONG			xrIDirect3DVertexBuffer9::Release()
{
	APIDEBUG("xrIDirect3DVertexBuffer9::Release");
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
HRESULT			__stdcall	xrIDirect3DVertexBuffer9::GetDevice		( IDirect3DDevice9** ppDevice)										
{
	APIDEBUG("xrIDirect3DVertexBuffer9::GetDevice");
	m_pIDirect3DDevice9->AddRef();
	*ppDevice = m_pIDirect3DDevice9;
	return HRESULT_Proc(S_OK);
};
HRESULT			__stdcall	xrIDirect3DVertexBuffer9::SetPrivateData	( REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)	{APIDEBUG("xrIDirect3DVertexBuffer9::SetPrivateData");return HRESULT_Proc(S_OK);};
HRESULT			__stdcall	xrIDirect3DVertexBuffer9::GetPrivateData	( REFGUID refguid,void* pData,DWORD* pSizeOfData)					{APIDEBUG("xrIDirect3DVertexBuffer9::GetPrivateData");return HRESULT_Proc(S_OK);};
HRESULT			__stdcall	xrIDirect3DVertexBuffer9::FreePrivateData( REFGUID refguid)													{APIDEBUG("xrIDirect3DVertexBuffer9::FreePrivateData");return HRESULT_Proc(S_OK);};
DWORD			__stdcall	xrIDirect3DVertexBuffer9::SetPriority	( DWORD PriorityNew)												{APIDEBUG("xrIDirect3DVertexBuffer9::SetPriority");DWORD old = Priority; Priority = PriorityNew;  return DWORD_Proc(old);};
DWORD			__stdcall	xrIDirect3DVertexBuffer9::GetPriority	()																	{APIDEBUG("xrIDirect3DVertexBuffer9::GetPriority");return DWORD_Proc(Priority);};
void			__stdcall	xrIDirect3DVertexBuffer9::PreLoad		()																	{APIDEBUG("xrIDirect3DVertexBuffer9::PreLoad");return VOID_proc();};
D3DRESOURCETYPE	__stdcall	xrIDirect3DVertexBuffer9::GetType		()																	{APIDEBUG("xrIDirect3DVertexBuffer9::GetType");return D3DRESOURCETYPE(0);};
HRESULT			__stdcall	xrIDirect3DVertexBuffer9::Lock			( UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags)		
{
	APIDEBUG("xrIDirect3DVertexBuffer9::Lock");
	*ppbData = m_pBuffer + OffsetToLock;
	return HRESULT_Proc(S_OK);
};
HRESULT			__stdcall	xrIDirect3DVertexBuffer9::Unlock			()																	{APIDEBUG("xrIDirect3DVertexBuffer9::Unlock");return HRESULT_Proc(S_OK);};
HRESULT			__stdcall	xrIDirect3DVertexBuffer9::GetDesc		( D3DVERTEXBUFFER_DESC *pDesc)										{APIDEBUG("xrIDirect3DVertexBuffer9::GetDesc");return HRESULT_Proc(S_OK);};
