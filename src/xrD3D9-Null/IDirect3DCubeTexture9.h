

#ifdef __cplusplus
extern "C" {
#endif

class xrIDirect3DCubeTexture9 : public IDirect3DCubeTexture9
{
protected:
    signed int m_refCount;
    IDirect3DDevice9* m_pIDirect3DDevice9;

public:
    xrIDirect3DCubeTexture9(IDirect3DDevice9* pIDirect3DDevice9, unsigned int iWidth, unsigned int iHeight, unsigned int iLevels, DWORD iUsage,
        D3DFORMAT iFormat, D3DPOOL iPool);
    /*** IUnknown methods ***/
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObj);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

      /*** IDirect3DBaseTexture9 methods ***/
    HRESULT __stdcall GetDevice(IDirect3DDevice9** ppDevice);
    HRESULT __stdcall SetPrivateData(REFGUID refguid, const void* pData, DWORD SizeOfData, DWORD Flags);
    HRESULT __stdcall GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData);
    HRESULT __stdcall FreePrivateData(REFGUID refguid);
    DWORD __stdcall SetPriority(DWORD PriorityNew);
    DWORD __stdcall GetPriority();
    void __stdcall PreLoad();
    D3DRESOURCETYPE __stdcall GetType();
    DWORD __stdcall SetLOD(DWORD LODNew);
    DWORD __stdcall GetLOD();
    DWORD __stdcall GetLevelCount();
    HRESULT __stdcall SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType);
    D3DTEXTUREFILTERTYPE __stdcall GetAutoGenFilterType();
    void __stdcall GenerateMipSubLevels();
    HRESULT __stdcall GetLevelDesc(unsigned int Level, D3DSURFACE_DESC* pDesc);
    HRESULT __stdcall GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, unsigned int Level, IDirect3DSurface9** ppCubeMapSurface);
    HRESULT __stdcall LockRect(
        D3DCUBEMAP_FACES FaceType, unsigned int Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags);
    HRESULT __stdcall UnlockRect(D3DCUBEMAP_FACES FaceType, unsigned int Level);
    HRESULT __stdcall AddDirtyRect(D3DCUBEMAP_FACES FaceType, const RECT* pDirtyRect);

    //#ifdef D3D_DEBUG_INFO
    const wchar_t * Name;
    unsigned int Width;
    unsigned int Height;
    unsigned int Levels;
    DWORD Usage;
    D3DFORMAT Format;
    D3DPOOL Pool;
    DWORD Priority;
    DWORD LOD;
    D3DTEXTUREFILTERTYPE FilterType;
    unsigned int LockCount;
    const wchar_t * CreationCallStack;
    //#endif
};

#ifdef __cplusplus
};
#endif
