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
