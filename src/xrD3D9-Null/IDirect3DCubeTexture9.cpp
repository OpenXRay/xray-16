#include "stdafx.h"

#include "IDirect3DCubeTexture9.h"
#include "IDirect3DSurface9.h"

#include "xrD3D9-Null_OutProc.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3DCubeTexture9;

xrIDirect3DCubeTexture9::xrIDirect3DCubeTexture9(IDirect3DDevice9* pIDirect3DDevice9, UINT iWidth, UINT iHeight,
    UINT iLevels, DWORD iUsage, D3DFORMAT iFormat, D3DPOOL iPool)
    : m_refCount(0), Name(nullptr), Width(iWidth), Height(iHeight), Levels(iLevels), Usage(iUsage), Format(iFormat)
      //#ifdef D3D_DEBUG_INFO
      ,
      Pool(iPool), Priority(0)
//#endif
{
    APIDEBUG("xrIDirect3DCubeTexture9::xrIDirect3DCubeTexture9");
    m_pIDirect3DDevice9 = pIDirect3DDevice9;
    //-------------------------------------------------------------
    LOD = 0;
    FilterType = D3DTEXTUREFILTERTYPE(0);
    LockCount = 0;
    CreationCallStack = nullptr;
}

/*** IUnknown methods ***/
HRESULT xrIDirect3DCubeTexture9::QueryInterface(REFIID riid, void** ppvObj)
{
    APIDEBUG("xrIDirect3DCubeTexture9::QueryInterface");
    if (riid == IID_IUnknown || riid == IID_IDirect3DCubeTexture9)
    {
        *ppvObj = this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

ULONG xrIDirect3DCubeTexture9::AddRef()
{
    APIDEBUG("xrIDirect3DCubeTexture9::AddRef");
    m_refCount++;
    return m_refCount;
}

ULONG xrIDirect3DCubeTexture9::Release()
{
    APIDEBUG("xrIDirect3DCubeTexture9::Release");
    m_refCount--;
    if (m_refCount < 0)
    {
        delete this;
        return -1;
    }
    return m_refCount;
}

/*** IDirect3DBaseTexture9 methods ***/
HRESULT xrIDirect3DCubeTexture9::GetDevice(IDirect3DDevice9** ppDevice)
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetDevice");
    m_pIDirect3DDevice9->AddRef();
    *ppDevice = m_pIDirect3DDevice9;
    return S_OK;
}
HRESULT xrIDirect3DCubeTexture9::SetPrivateData(REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags)
{
    APIDEBUG("xrIDirect3DCubeTexture9::SetPrivateData");
    return S_OK;
};
HRESULT xrIDirect3DCubeTexture9::GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData)
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetPrivateData");
    return S_OK;
};
HRESULT xrIDirect3DCubeTexture9::FreePrivateData(REFGUID refguid)
{
    APIDEBUG("xrIDirect3DCubeTexture9::FreePrivateData");
    return S_OK;
};
DWORD xrIDirect3DCubeTexture9::SetPriority(DWORD PriorityNew)
{
    APIDEBUG("xrIDirect3DCubeTexture9::SetPriority");
    DWORD old = Priority;
    Priority = PriorityNew;
    return old;
};
DWORD xrIDirect3DCubeTexture9::GetPriority()
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetPriority");
    return Priority;
};
void xrIDirect3DCubeTexture9::PreLoad() { APIDEBUG("xrIDirect3DCubeTexture9::PreLoad"); };
D3DRESOURCETYPE xrIDirect3DCubeTexture9::GetType()
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetType");
    return D3DRTYPE_TEXTURE;
};
DWORD xrIDirect3DCubeTexture9::SetLOD(DWORD LODNew)
{
    APIDEBUG("xrIDirect3DCubeTexture9::SetLOD");
    DWORD old = LOD;
    LOD = LODNew;
    return old;
};
DWORD xrIDirect3DCubeTexture9::GetLOD()
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetLOD");
    return LOD;
};
DWORD xrIDirect3DCubeTexture9::GetLevelCount()
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetLevelCount");
    return Levels;
};
HRESULT xrIDirect3DCubeTexture9::SetAutoGenFilterType(D3DTEXTUREFILTERTYPE iFilterType)
{
    APIDEBUG("xrIDirect3DCubeTexture9::SetAutoGenFilterType");
    FilterType = iFilterType;
    return S_OK;
};
D3DTEXTUREFILTERTYPE xrIDirect3DCubeTexture9::GetAutoGenFilterType()
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetAutoGenFilterType");
    return FilterType;
};
void xrIDirect3DCubeTexture9::GenerateMipSubLevels() { APIDEBUG("xrIDirect3DCubeTexture9::GenerateMipSubLevels"); };
HRESULT xrIDirect3DCubeTexture9::GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc)
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetLevelDesc");

    pDesc->Format = Format;
    pDesc->Type = D3DRTYPE_TEXTURE;
    pDesc->Usage = Usage;
    pDesc->Pool = Pool;

    pDesc->MultiSampleType = D3DMULTISAMPLE_TYPE(0);
    pDesc->MultiSampleQuality = 0;
    pDesc->Width = Width;
    pDesc->Height = Height;

    return S_OK;
};

HRESULT xrIDirect3DCubeTexture9::GetCubeMapSurface(
    D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9** ppCubeMapSurface)
{
    APIDEBUG("xrIDirect3DCubeTexture9::GetCubeMapSurface");

    *ppCubeMapSurface = NULL;
    xrIDirect3DSurface9* I =
        new xrIDirect3DSurface9(m_pIDirect3DDevice9, Width, Height, Format, D3DMULTISAMPLE_TYPE(0), 0);
    *ppCubeMapSurface = I;

    return S_OK;
};
HRESULT xrIDirect3DCubeTexture9::LockRect(
    D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags)
{
    APIDEBUG("xrIDirect3DCubeTexture9::LockRect");
    return S_OK;
};
HRESULT xrIDirect3DCubeTexture9::UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level)
{
    APIDEBUG("xrIDirect3DCubeTexture9::UnlockRect");
    return S_OK;
};
HRESULT xrIDirect3DCubeTexture9::AddDirtyRect(D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect)
{
    APIDEBUG("xrIDirect3DCubeTexture9::AddDirtyRect");
    return S_OK;
};
