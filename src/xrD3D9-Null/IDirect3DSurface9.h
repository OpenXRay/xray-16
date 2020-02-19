//---------------------------------

#ifdef __cplusplus
extern "C" {
#endif

class xrIDirect3DSurface9 : public IDirect3DSurface9
{
protected:
    signed int m_refCount;
    IDirect3DDevice9* m_pIDirect3DDevice9;
    unsigned char* m_pLockedData;

public:
    xrIDirect3DSurface9(IDirect3DDevice9* pIDirect3DDevice9, unsigned int Width, unsigned int Height, D3DFORMAT Format,
        D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality);
    /*** IUnknown methods ***/
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObj);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    /*** IDirect3DResource9 methods ***/
    HRESULT __stdcall GetDevice(IDirect3DDevice9** ppDevice);
    HRESULT __stdcall SetPrivateData(REFGUID refguid, const void* pData, DWORD SizeOfData, DWORD Flags);
    HRESULT __stdcall GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData);
    HRESULT __stdcall FreePrivateData(REFGUID refguid);
    DWORD __stdcall SetPriority(DWORD PriorityNew);
    DWORD __stdcall GetPriority();
    void __stdcall PreLoad();
    D3DRESOURCETYPE __stdcall GetType();
    HRESULT __stdcall GetContainer(REFIID riid, void** ppContainer);
    HRESULT __stdcall GetDesc(D3DSURFACE_DESC* pDesc);
    HRESULT __stdcall LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags);
    HRESULT __stdcall UnlockRect();
    HRESULT __stdcall GetDC(HDC* phdc);
    HRESULT __stdcall ReleaseDC(HDC hdc);

    //#ifdef D3D_DEBUG_INFO
    const wchar_t * Name;
    unsigned int Width;
    unsigned int Height;
    DWORD Usage;
    D3DFORMAT Format;
    D3DPOOL Pool;
    D3DMULTISAMPLE_TYPE MultiSampleType;
    DWORD MultiSampleQuality;
    DWORD Priority;
    unsigned int LockCount;
    unsigned int DCCount;
    const wchar_t * CreationCallStack;
    //#endif
};
#ifdef __cplusplus
};
#endif
