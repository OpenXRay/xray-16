#include "stdafx.h"

#include "IDirect3DIndexBuffer9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DIndexBuffer9;

xrIDirect3DIndexBuffer9::xrIDirect3DIndexBuffer9(
    IDirect3DDevice9* pIDirect3DDevice9, UINT iLength, DWORD iUsage, D3DFORMAT iFormat, D3DPOOL iPool)
    : m_refCount(0), m_pIDirect3DDevice9(pIDirect3DDevice9)
//#ifdef D3D_DEBUG_INFO
    , Name(nullptr), Length(iLength), Usage(iUsage), Format(iFormat), Pool(iPool),
      Priority(0), LockCount(0), CreationCallStack(nullptr)
//#endif
{
    APIDEBUG("xrIDirect3DIndexBuffer9::xrIDirect3DIndexBuffer9");

    switch (Format)
    {
    case D3DFMT_INDEX16: m_pBuffer = new BYTE[Length * 2]; break;
    case D3DFMT_INDEX32: m_pBuffer = new BYTE[Length * 4]; break;
    }
};
/*** IUnknown methods ***/
HRESULT xrIDirect3DIndexBuffer9::QueryInterface(REFIID riid, void** ppvObj)
{
    APIDEBUG("xrIDirect3DIndexBuffer9::QueryInterface");
    if (riid == IID_IUnknown || riid == IID_IDirect3DIndexBuffer9)
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

ULONG xrIDirect3DIndexBuffer9::AddRef()
{
    APIDEBUG("xrIDirect3DIndexBuffer9::AddRef");
    m_refCount++;
    return m_refCount;
}

ULONG xrIDirect3DIndexBuffer9::Release()
{
    APIDEBUG("xrIDirect3DIndexBuffer9::Release");
    m_refCount--;
    if (m_refCount < 0)
    {
        delete[] m_pBuffer;
        delete this;
        return -1;
    }
    return m_refCount;
}

/*** IDirect3DResource9 methods ***/
HRESULT __stdcall xrIDirect3DIndexBuffer9::GetDevice(IDirect3DDevice9** ppDevice)
{
    APIDEBUG("xrIDirect3DIndexBuffer9::GetDevice");
    m_pIDirect3DDevice9->AddRef();
    *ppDevice = m_pIDirect3DDevice9;
    return S_OK;
};
HRESULT __stdcall xrIDirect3DIndexBuffer9::SetPrivateData(
    REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags)
{
    APIDEBUG("xrIDirect3DIndexBuffer9::SetPrivateData");
    return S_OK;
};
HRESULT __stdcall xrIDirect3DIndexBuffer9::GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData)
{
    APIDEBUG("xrIDirect3DIndexBuffer9::GetPrivateData");
    return S_OK;
};
HRESULT __stdcall xrIDirect3DIndexBuffer9::FreePrivateData(REFGUID refguid)
{
    APIDEBUG("xrIDirect3DIndexBuffer9::FreePrivateData");
    return S_OK;
};
DWORD __stdcall xrIDirect3DIndexBuffer9::SetPriority(DWORD PriorityNew)
{
    APIDEBUG("xrIDirect3DIndexBuffer9::SetPriority");
    DWORD old = Priority;
    Priority = PriorityNew;
    return old;
};
DWORD __stdcall xrIDirect3DIndexBuffer9::GetPriority()
{
    APIDEBUG("xrIDirect3DIndexBuffer9::GetPriority");
    return Priority;
};
void __stdcall xrIDirect3DIndexBuffer9::PreLoad() { APIDEBUG("xrIDirect3DIndexBuffer9::PreLoad"); };
D3DRESOURCETYPE __stdcall xrIDirect3DIndexBuffer9::GetType()
{
    APIDEBUG("xrIDirect3DIndexBuffer9::GetType");
    return D3DRESOURCETYPE(0);
};
HRESULT __stdcall xrIDirect3DIndexBuffer9::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
{
    APIDEBUG("xrIDirect3DIndexBuffer9::Lock");
    *ppbData = m_pBuffer + OffsetToLock;
    return S_OK;
};
HRESULT __stdcall xrIDirect3DIndexBuffer9::Unlock()
{
    APIDEBUG("xrIDirect3DIndexBuffer9::Unlock");
    return S_OK;
};
HRESULT __stdcall xrIDirect3DIndexBuffer9::GetDesc(D3DINDEXBUFFER_DESC* pDesc)
{
    APIDEBUG("xrIDirect3DIndexBuffer9::GetDesc");
    return S_OK;
};
