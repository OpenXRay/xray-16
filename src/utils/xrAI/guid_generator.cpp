////////////////////////////////////////////////////////////////////////////
//	Module 		: guid_generator.cpp
//	Created 	: 21.03.2005
//  Modified 	: 21.03.2005
//	Author		: Dmitriy Iassenev
//	Description : GUID generator
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#pragma hdrstop

#include "guid_generator.h"

#ifdef WINVER
#include <rpcdce.h>
#include <rpc.h>
#pragma comment(lib, "rpcrt4.lib")
#endif

xrGUID generate_guid()
{
    xrGUID result;
#ifdef WINVER
    static_assert(sizeof(xrGUID) == sizeof(GUID), "Different GUID types.");
    GUID _result;
    RPC_STATUS gen_result = UuidCreate(&_result);
    memcpy(&result, &_result, sizeof(_result));
    switch (gen_result)
    {
    case RPC_S_OK: return (result);
    case RPC_S_UUID_LOCAL_ONLY: return (result);
    case RPC_S_UUID_NO_ADDRESS:
    default: break;
    }
#endif
    static_assert(sizeof(result) >= sizeof(u64), "GUID must have size greater or equal to the long long.");
    ZeroMemory(&result, sizeof(result));
    u64 temp = CPU::GetCLK();
    memcpy(&result, &temp, sizeof(temp));
    return (result);
}

LPCSTR generate_guid(const xrGUID& guid, pstr buffer, const size_t& buffer_size)
{
#ifdef WINVER
    static_assert(sizeof(xrGUID) == sizeof(GUID), "Different GUID types.");
    GUID temp;
    memcpy(&temp, &guid, sizeof(guid));
    RPC_CSTR temp2;
    RPC_STATUS status = UuidToString(&temp, &temp2);
    switch (status)
    {
    case RPC_S_OK: break;
    case RPC_S_OUT_OF_MEMORY: NODEFAULT;
    default: NODEFAULT;
    }
    VERIFY(buffer_size > xr_strlen((LPCSTR)temp2));
    xr_strcpy(buffer, buffer_size, (LPCSTR)temp2);
    RpcStringFree(&temp2);
    return (buffer);
#else
    NODEFAULT;
#endif // WINVER
}
