#include "stdafx.h"

/*
//---------------------------------
#include "d3d9types.h"
#include "d3d9caps.h"

#include "IDirect3DDevice9.h"
*/
/* IID_IDirect3D9 */
/* {81BDCBCA-64D4-426d-AE8D-AD0147F4275C} */
// DEFINE_GUID(IID_IDirect3D9, 0x81bdcbca, 0x64d4, 0x426d, 0xae, 0x8d, 0xad, 0x1, 0x47, 0xf4, 0x27, 0x5c);

// interface DECLSPEC_UUID("81BDCBCA-64D4-426d-AE8D-AD0147F4275C") IDirect3D9;

// typedef interface IDirect3D9                    IDirect3D9;

#ifdef __cplusplus
extern "C" {
#endif

class xrIDirect3D9 : public IDirect3D9
{
protected:
    signed int m_refCount;

public:
    xrIDirect3D9();
    /*** IUnknown methods ***/
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObj);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    /*** IDirect3D9 methods ***/
    HRESULT __stdcall RegisterSoftwareDevice(void* pInitializeFunction);
    unsigned int __stdcall GetAdapterCount();
    HRESULT __stdcall GetAdapterIdentifier(unsigned int Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier);
    unsigned int __stdcall GetAdapterModeCount(unsigned int Adapter, D3DFORMAT Format);
    HRESULT __stdcall EnumAdapterModes(unsigned int Adapter, D3DFORMAT Format, unsigned int Mode, D3DDISPLAYMODE* pMode);
    HRESULT __stdcall GetAdapterDisplayMode(unsigned int Adapter, D3DDISPLAYMODE* pMode);
    HRESULT __stdcall CheckDeviceType(
        unsigned int Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed);
    HRESULT __stdcall CheckDeviceFormat(unsigned int Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage,
        D3DRESOURCETYPE RType, D3DFORMAT CheckFormat);
    HRESULT __stdcall CheckDeviceMultiSampleType(unsigned int Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
        BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels);
    HRESULT __stdcall CheckDepthStencilMatch(unsigned int Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
        D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
    HRESULT __stdcall CheckDeviceFormatConversion(
        unsigned int Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat);
    HRESULT __stdcall GetDeviceCaps(unsigned int Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps);
    HMONITOR __stdcall GetAdapterMonitor(unsigned int Adapter);
    HRESULT __stdcall CreateDevice(unsigned int Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
        D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);

    ///#ifdef D3D_DEBUG_INFO
    const wchar_t * Version;
    //#endif
};

#ifdef __cplusplus
};
#endif
