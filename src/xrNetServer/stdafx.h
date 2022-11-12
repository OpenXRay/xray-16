#pragma once

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrCore/_std_extensions.h"

#if defined(XR_PLATFORM_WINDOWS)
#pragma warning(push)
#pragma warning(disable : 4995)
#include <DPlay/dplay8.h>
#pragma warning(pop)
#endif

#include "NET_Shared.h"

#ifdef XR_PLATFORM_WINDOWS
// {0218FA8B-515B-4bf2-9A5F-2F079D1759F3}
static constexpr GUID NET_GUID =
{
    0x218fa8b, 0x515b, 0x4bf2, { 0x9a, 0x5f, 0x2f, 0x7, 0x9d, 0x17, 0x59, 0xf3 }
};

// {8D3F9E5E-A3BD-475b-9E49-B0E77139143C}
static constexpr GUID CLSID_NETWORKSIMULATOR_DP8SP_TCPIP =
{
    0x8d3f9e5e, 0xa3bd, 0x475b, { 0x9e, 0x49, 0xb0, 0xe7, 0x71, 0x39, 0x14, 0x3c }
};
#endif // XR_PLATFORM_WINDOWS
