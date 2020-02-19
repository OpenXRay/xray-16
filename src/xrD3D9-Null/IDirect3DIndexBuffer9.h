

#ifdef __cplusplus
extern "C" {
#endif

class xrIDirect3DIndexBuffer9 : public IDirect3DIndexBuffer9
{
protected:
    signed int m_refCount;
    unsigned char* m_pBuffer;
    IDirect3DDevice9* m_pIDirect3DDevice9;

public:
    xrIDirect3DIndexBuffer9(
        IDirect3DDevice9* pIDirect3DDevice9, unsigned int iLength, DWORD iUsage, D3DFORMAT iFormat, D3DPOOL iPool);
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
    HRESULT __stdcall Lock(unsigned int OffsetToLock, unsigned int SizeToLock, void** ppbData, DWORD Flags);
    HRESULT __stdcall Unlock();
    HRESULT __stdcall GetDesc(D3DINDEXBUFFER_DESC* pDesc);

    //#ifdef D3D_DEBUG_INFO
    const wchar_t * Name;
    unsigned int Length;
    DWORD Usage;
    D3DFORMAT Format;
    D3DPOOL Pool;
    DWORD Priority;
    unsigned int LockCount;
    const wchar_t * CreationCallStack;
    //#endif
};

#ifdef __cplusplus
};
#endif
