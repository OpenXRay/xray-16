#include "stdafx.h"
#include "dxPixEventWrapper.h"

#if defined(DEBUG) || defined(COC_DEBUG)

CScopedPixEvent::CScopedPixEvent(LPCWSTR eventName)
{
#ifdef USE_DX11
    HW.UserDefinedAnnotation->BeginEvent(eventName);
#else
    D3DPERF_BeginEvent(D3DCOLOR_RGBA(127, 0, 0, 255), eventName);
#endif
}

CScopedPixEvent::~CScopedPixEvent()
{
#ifdef USE_DX11
    HW.UserDefinedAnnotation->EndEvent();
#else
    D3DPERF_EndEvent();
#endif
}

///////////////////////////////////////////////

#if defined(USE_DX10) || defined(USE_DX11)
void dxPixSetDebugName(ID3DDeviceChild* resource, const shared_str& name)
{
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
}
#endif

#endif //	DEBUG
