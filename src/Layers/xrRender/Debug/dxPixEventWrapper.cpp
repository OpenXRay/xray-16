#include "stdafx.h"
#include "dxPixEventWrapper.h"

#if defined(DEBUG) || defined(COC_DEBUG)

void dxPixSetDebugName(ID3DDeviceChild* resource, const shared_str& name)
{
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.c_str());
}

#endif //	DEBUG
