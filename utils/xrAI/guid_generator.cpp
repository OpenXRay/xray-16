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
#	include <rpcdce.h>
#	include <rpc.h>
#	pragma comment(lib,"rpcrt4.lib")
#endif

xrGUID generate_guid()
{
	xrGUID			result;
#ifdef WINVER
	STATIC_CHECK	(sizeof(xrGUID) == sizeof(GUID),Different_GUID_types);
	GUID			_result;
	RPC_STATUS		gen_result = UuidCreate(&_result);
	Memory.mem_copy	(&result,&_result,sizeof(_result));
	switch (gen_result) {
		case RPC_S_OK				: return(result);
		case RPC_S_UUID_LOCAL_ONLY	: return(result);
		case RPC_S_UUID_NO_ADDRESS	: 
		default						: break;
	}
#endif
	STATIC_CHECK	(sizeof(result) >= sizeof(u64),GUID_must_have_size_greater_or_equal_to_the_long_long);
	ZeroMemory		(&result,sizeof(result));
	u64				temp = CPU::GetCLK();
	Memory.mem_copy	(&result,&temp,sizeof(temp));
	return			(result);
}

LPCSTR generate_guid(const xrGUID &guid, LPSTR buffer, const u32 &buffer_size)
{
#ifdef WINVER
	STATIC_CHECK	(sizeof(xrGUID) == sizeof(GUID),Different_GUID_types);
	GUID			temp;
	Memory.mem_copy	(&temp,&guid,sizeof(guid));
	RPC_CSTR		temp2;
	RPC_STATUS		status = UuidToString(&temp,&temp2);
	switch (status) {
		case RPC_S_OK				: break;
		case RPC_S_OUT_OF_MEMORY	: NODEFAULT;
		default						: NODEFAULT;
	}
	VERIFY			(buffer_size > xr_strlen((LPCSTR)temp2));
	xr_strcpy		(buffer, buffer_size, (LPCSTR)temp2);
	RpcStringFree	(&temp2);
	return			(buffer);
#else
	NODEFAULT;
#endif // WINVER
}
