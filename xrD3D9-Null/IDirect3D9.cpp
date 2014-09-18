#include "stdafx.h"
#include "IDirect3D9.h"
#include "IDirect3DDevice9.h"
#include "xrD3D9-Null_OutProc.h"

#include <stdlib.h>
#include <objbase.h>
#include <windows.h>
#include "stdio.h"

const GUID DECLSPEC_SELECTANY IID_IDirect3D9;

xrIDirect3D9::xrIDirect3D9() : m_refCount(0)
{
	APIDEBUG("xrIDirect3D9::xrIDirect3D9");
//#ifdef D3D_DEBUG_INFO
	Version = NULL;
//#endif
};

ULONG	xrIDirect3D9::AddRef(void)
{
	APIDEBUG("xrIDirect3D9::AddRef");
	m_refCount++;
	return ULONG_Proc(m_refCount);
}

ULONG	xrIDirect3D9::Release(void)
{
	APIDEBUG("xrIDirect3D9::Release");
	m_refCount--;
	if (m_refCount < 0)
	{
		delete this;
		return ULONG_Proc(-1);
	}
	return ULONG_Proc(m_refCount);
}

HRESULT xrIDirect3D9::QueryInterface(const IID &iid, void FAR* FAR* ppvObj)
{
	APIDEBUG("xrIDirect3D9::QueryInterface");
	if (iid == IID_IUnknown || iid == IID_IDirect3D9)
	{
		*ppvObj = this;
		AddRef();
		return HRESULT_Proc(NOERROR);
	}
	return HRESULT_Proc(E_NOINTERFACE);
}

HRESULT xrIDirect3D9::RegisterSoftwareDevice(void* pInitializeFunction)
{
	APIDEBUG("xrIDirect3D9::RegisterSoftwareDevice");
	return HRESULT_Proc(S_OK);
};
UINT xrIDirect3D9::GetAdapterCount()
{
	APIDEBUG("xrIDirect3D9::GetAdapterCount");
	return ULONG_Proc(1);
};

HRESULT xrIDirect3D9::GetAdapterIdentifier(UINT Adapter,DWORD Flags,D3DADAPTER_IDENTIFIER9* pIdentifier)
{
	APIDEBUG("xrIDirect3D9::GetAdapterIdentifier");
	sprintf_s(pIdentifier->Driver, "Default Driver");
	sprintf_s(pIdentifier->Description, "Default X-Ray Dedicated Adapter");
	sprintf_s(pIdentifier->DeviceName, "Dedicated");
	return HRESULT_Proc(S_OK);
};
UINT xrIDirect3D9::GetAdapterModeCount( UINT Adapter,D3DFORMAT Format)
{
	APIDEBUG("xrIDirect3D9::GetAdapterModeCount");
	return ULONG_Proc(1);
};

HRESULT xrIDirect3D9::EnumAdapterModes(UINT Adapter,D3DFORMAT Format,UINT Mode,D3DDISPLAYMODE* pMode)
{
	APIDEBUG("xrIDirect3D9::EnumAdapterModes");
	return HRESULT_Proc(S_OK);
};
HRESULT xrIDirect3D9::GetAdapterDisplayMode( UINT Adapter,D3DDISPLAYMODE* pMode)
{
	APIDEBUG("xrIDirect3D9::GetAdapterDisplayMode");
	pMode->Format = D3DFMT_A8R8G8B8;
	return HRESULT_Proc(S_OK);
};
HRESULT xrIDirect3D9::CheckDeviceType( UINT Adapter,D3DDEVTYPE DevType,D3DFORMAT AdapterFormat,D3DFORMAT BackBufferFormat,BOOL bWindowed)
{
	APIDEBUG("xrIDirect3D9::CheckDeviceType");
	return HRESULT_Proc(S_OK);
};
HRESULT xrIDirect3D9::CheckDeviceFormat( UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,DWORD Usage,D3DRESOURCETYPE RType,D3DFORMAT CheckFormat)
{
	APIDEBUG("xrIDirect3D9::CheckDeviceFormat");
	return HRESULT_Proc(S_OK);
};
HRESULT xrIDirect3D9::CheckDeviceMultiSampleType( UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SurfaceFormat,BOOL Windowed,D3DMULTISAMPLE_TYPE MultiSampleType,DWORD* pQualityLevels)
{
	APIDEBUG("xrIDirect3D9::CheckDeviceMultiSampleType");
	return HRESULT_Proc(S_OK);
};
HRESULT xrIDirect3D9::CheckDepthStencilMatch( UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,D3DFORMAT RenderTargetFormat,D3DFORMAT DepthStencilFormat)
{
	APIDEBUG("xrIDirect3D9::CheckDepthStencilMatch");
	return HRESULT_Proc(S_OK);
};
HRESULT xrIDirect3D9::CheckDeviceFormatConversion( UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SourceFormat,D3DFORMAT TargetFormat)
{
	APIDEBUG("xrIDirect3D9::CheckDeviceFormatConversion");
	return HRESULT_Proc(S_OK);
};
HRESULT xrIDirect3D9::GetDeviceCaps( UINT Adapter,D3DDEVTYPE DeviceType,D3DCAPS9* pCaps)
{
	APIDEBUG("xrIDirect3D9::GetDeviceCaps");
	if (pCaps)
		ZeroMemory(pCaps,sizeof(D3DCAPS9));
	return HRESULT_Proc(S_OK);
};
HMONITOR xrIDirect3D9::GetAdapterMonitor( UINT Adapter)
{
	APIDEBUG("xrIDirect3D9::GetAdapterMonitor");
	return NULL;
};

HRESULT xrIDirect3D9::CreateDevice	(UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface)
{
	APIDEBUG("xrIDirect3D9::CreateDevice");
	*ppReturnedDeviceInterface = NULL;
	xrIDirect3DDevice9* I = new xrIDirect3DDevice9(this, pPresentationParameters);
	*ppReturnedDeviceInterface = I;
	return HRESULT_Proc(S_OK);
};
//-------------------------------------------------------------------
/*
HRESULT		xrIDirect3D9::HRESULT_Proc(HRESULT ret)
{
	return ret;
}

ULONG		xrIDirect3D9::ULONG_Proc(ULONG ret)
{
	return ret;
}
*/


