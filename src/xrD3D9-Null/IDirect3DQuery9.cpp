#include "stdafx.h"
#include "IDirect3DQuery9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DQuery9;

xrIDirect3DQuery9::xrIDirect3DQuery9(IDirect3DDevice9* pIDirect3DDevice9, D3DQUERYTYPE rType) : m_refCount(0)
{
    APIDEBUG("xrIDirect3DQuery9::xrIDirect3DQuery9");
    DataSize = 0;
    memcpy(&Type, &rType, sizeof(rType));
    CreationCallStack = NULL;
    m_pIDirect3DDevice9 = pIDirect3DDevice9;
};

/*** IUnknown methods ***/
HRESULT xrIDirect3DQuery9::QueryInterface(REFIID riid, void** ppvObj)
{
    APIDEBUG("xrIDirect3DQuery9::QueryInterface");
    if (riid == IID_IUnknown || riid == IID_IDirect3DQuery9)
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

ULONG xrIDirect3DQuery9::AddRef()
{
    APIDEBUG("xrIDirect3DQuery9::AddRef");
    m_refCount++;
    return m_refCount;
}

ULONG xrIDirect3DQuery9::Release()
{
    APIDEBUG("xrIDirect3DQuery9::Release");
    m_refCount--;
    if (m_refCount < 0)
    {
        delete this;
        return -1;
    }

    return m_refCount;
}

/*** IDirect3DQuery9 methods ***/
HRESULT xrIDirect3DQuery9::GetDevice(IDirect3DDevice9** ppDevice)
{
    APIDEBUG("xrIDirect3DQuery9::GetDevice");
    m_pIDirect3DDevice9->AddRef();
    *ppDevice = m_pIDirect3DDevice9;
    return S_OK;
};
D3DQUERYTYPE xrIDirect3DQuery9::GetType()
{
    APIDEBUG("xrIDirect3DQuery9::GetType");
    return Type;
};
DWORD xrIDirect3DQuery9::GetDataSize()
{
    APIDEBUG("xrIDirect3DQuery9::GetDataSize");
    return DataSize;
};
HRESULT xrIDirect3DQuery9::Issue(DWORD dwIssueFlags)
{
    APIDEBUG("xrIDirect3DQuery9::Issue");
    return S_OK;
};
HRESULT xrIDirect3DQuery9::GetData(void* pData, DWORD dwSize, DWORD dwGetDataFlags)
{
    APIDEBUG("xrIDirect3DQuery9::GetData");
    return S_OK;
};
//-----------------------------------------------------------------------
/*
HRESULT		xrIDirect3DQuery9::HRESULT_Proc(HRESULT ret)
{
    return ret;
}

ULONG		xrIDirect3DQuery9::ULONG_Proc(ULONG ret)
{
    return ret;
}

DWORD		xrIDirect3DQuery9::DWORD_Proc(DWORD ret)
{
    return ret;
}
*/
