#include "stdafx.h"

#include "IDirect3DSurface9.h"

#include "xrD3D9-Null_OutProc.h"

#ifdef _DEBUG
#include "crtdbg.h"
#endif

const GUID DECLSPEC_SELECTANY IID_IDirect3DSurface9;

xrIDirect3DSurface9::xrIDirect3DSurface9	(IDirect3DDevice9* pIDirect3DDevice9, UINT iWidth,UINT iHeight,D3DFORMAT iFormat,D3DMULTISAMPLE_TYPE iMultiSample,DWORD iMultisampleQuality)
	: Width(iWidth), Height(iHeight),Format(iFormat),MultiSampleType(iMultiSample),MultiSampleQuality(iMultisampleQuality),m_refCount(0)
{
	APIDEBUG("xrIDirect3DSurface9::xrIDirect3DSurface9");

	m_pIDirect3DDevice9 = pIDirect3DDevice9;
	//-----------------------------------------------
	Name = NULL;	
	Usage = 0;	
	Pool = D3DPOOL(0);	
	MultiSampleQuality = 0;
	Priority = 0;
	LockCount = 0;
	DCCount = 0;
	CreationCallStack = NULL;
	//-----------------------------------------------
	m_pLockedData = NULL;
};

/*** IUnknown methods ***/
HRESULT			xrIDirect3DSurface9::QueryInterface( REFIID riid, void** ppvObj)
{
	APIDEBUG("xrIDirect3DSurface9::QueryInterface");
	if (riid == IID_IUnknown || riid == IID_IDirect3DSurface9)
	{
		*ppvObj = this;
		AddRef();
		return HRESULT_Proc(NOERROR);
	}
	return HRESULT_Proc(E_NOINTERFACE);
}

ULONG			xrIDirect3DSurface9::AddRef() 
{
	APIDEBUG("xrIDirect3DSurface9::AddRef");
	m_refCount++;
	return ULONG_Proc(m_refCount);
}

ULONG			xrIDirect3DSurface9::Release()
{
	APIDEBUG("xrIDirect3DSurface9::Release");
	m_refCount--;
	if (m_refCount < 0)
	{
		delete this;
		return ULONG_Proc(-1);
	}
	return ULONG_Proc(m_refCount);
}

/*** IDirect3DResource9 methods ***/
HRESULT			xrIDirect3DSurface9::GetDevice		( IDirect3DDevice9** ppDevice)										
{
	APIDEBUG("xrIDirect3DSurface9::GetDevice");
	m_pIDirect3DDevice9->AddRef();
	*ppDevice = m_pIDirect3DDevice9;
	return HRESULT_Proc(S_OK);
};

HRESULT			xrIDirect3DSurface9::SetPrivateData	( REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)	{APIDEBUG("xrIDirect3DSurface9::SetPrivateData");return HRESULT_Proc(S_OK);};
HRESULT			xrIDirect3DSurface9::GetPrivateData	( REFGUID refguid,void* pData,DWORD* pSizeOfData)					{APIDEBUG("xrIDirect3DSurface9::GetPrivateData");return HRESULT_Proc(S_OK);};
HRESULT			xrIDirect3DSurface9::FreePrivateData( REFGUID refguid) 													{APIDEBUG("xrIDirect3DSurface9::FreePrivateData");return HRESULT_Proc(S_OK);};
DWORD			xrIDirect3DSurface9::SetPriority	( DWORD PriorityNew) 												{APIDEBUG("xrIDirect3DSurface9::SetPriority"); DWORD old = Priority; Priority = PriorityNew;  return DWORD_Proc(old);};
DWORD			xrIDirect3DSurface9::GetPriority	() 																	{APIDEBUG("xrIDirect3DSurface9::GetPriority");return DWORD_Proc(Priority);};
void			xrIDirect3DSurface9::PreLoad		() 																	{APIDEBUG("xrIDirect3DSurface9::PreLoad"); VOID_proc();};
D3DRESOURCETYPE	xrIDirect3DSurface9::GetType		() 																	{APIDEBUG("xrIDirect3DSurface9::GetType");return D3DRESOURCETYPE(0);};
HRESULT			xrIDirect3DSurface9::GetContainer	( REFIID riid,void** ppContainer) 									{APIDEBUG("xrIDirect3DSurface9::GetContainer");return HRESULT_Proc(S_OK);};
HRESULT			xrIDirect3DSurface9::GetDesc		( D3DSURFACE_DESC *pDesc)
{
	APIDEBUG("xrIDirect3DSurface9::GetDesc");

	pDesc->Format	= Format;
	pDesc->Type		= D3DRTYPE_SURFACE;
	pDesc->Usage	= Usage;
	pDesc->Pool		= Pool;

	pDesc->MultiSampleType	= MultiSampleType;
	pDesc->MultiSampleQuality = MultiSampleQuality;
	pDesc->Width	= Width;
	pDesc->Height	= Height;
	return HRESULT_Proc(S_OK);
};
HRESULT			xrIDirect3DSurface9::LockRect		( D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) 		
{
	APIDEBUG("xrIDirect3DSurface9::LockRect");
#ifdef _DEBUG
	if (m_pLockedData != NULL)
	{
		_ASSERT(0);
	}
#endif
	UINT RWidth = (NULL == pRect) ? Width : (pRect->right - pRect->left);
	UINT RHeight = (NULL == pRect) ? Height : (pRect->bottom - pRect->top);
	m_pLockedData = new BYTE[RWidth*RHeight*4];
	pLockedRect->Pitch = 4;
	pLockedRect->pBits = m_pLockedData;

	return HRESULT_Proc(S_OK);
};
HRESULT			xrIDirect3DSurface9::UnlockRect		() 																	
{
	APIDEBUG("xrIDirect3DSurface9::UnlockRect");
	
#ifdef _DEBUG
	if (m_pLockedData == NULL)
	{
		_ASSERT(0);
	}
#endif
	delete[] m_pLockedData;
	m_pLockedData = NULL;

	return HRESULT_Proc(S_OK);
};
HRESULT			xrIDirect3DSurface9::GetDC			( HDC *phdc) 														{APIDEBUG("xrIDirect3DSurface9::GetDC");return HRESULT_Proc(S_OK);};
HRESULT			xrIDirect3DSurface9::ReleaseDC		( HDC hdc) 															{APIDEBUG("xrIDirect3DSurface9::ReleaseDC");return HRESULT_Proc(S_OK);};