#include "stdafx.h"
#include "xrGameSpy_MainDefs.h"
#include "xrGameSpy_SAKE.h"

XRGAMESPY_API SAKEStartupResult	xrGS_sakeStartup(SAKE *sakePtr)
{
	SAKEStartupResult tmp_res = sakeStartup(sakePtr);
	if (tmp_res == SAKEStartupResult_SUCCESS)
	{
		char secret_key[32];
		memset(secret_key, 0, sizeof(secret_key));
		FillSecretKey(secret_key);
		sakeSetGame(*sakePtr, GAMESPY_GAMENAME, GAMESPY_GAMEID, secret_key);
	}
	return tmp_res;
}

XRGAMESPY_API void xrGS_sakeShutdown(SAKE sake)
{
	sakeShutdown(sake);
}

XRGAMESPY_API void xrGS_sakeSetProfile(SAKE sake,int profileId,const char *loginTicket)
{
	sakeSetProfile(sake, profileId, loginTicket);
}

XRGAMESPY_API SAKEStartRequestResult xrGS_sakeGetStartRequestResult(SAKE sake)
{
	return sakeGetStartRequestResult(sake);
}

XRGAMESPY_API SAKERequest xrGS_sakeGetMyRecords(SAKE sake,
												SAKEGetMyRecordsInput * input,
												SAKERequestCallback callback,
												void * userData)
{
	return sakeGetMyRecords(
		sake,
		input,
		callback,
		userData
	);
}

XRGAMESPY_API SAKERequest xrGS_sakeCreateRecord(SAKE sake,
												SAKECreateRecordInput * input,
												SAKERequestCallback callback,
												void * userdata)
{
	return sakeCreateRecord(
		sake,
		input,
		callback,
		userdata
	);
}

XRGAMESPY_API SAKERequest xrGS_sakeUpdateRecord(SAKE sake,
												SAKEUpdateRecordInput * input,
												SAKERequestCallback callback,
												void * userdata)
{
	return sakeUpdateRecord(
		sake,
		input,
		callback,
		userdata
	);
}

XRGAMESPY_API SAKERequest xrGS_sakeSearchForRecords(SAKE sake,
													SAKESearchForRecordsInput * input,
													SAKERequestCallback callback,
													void * userData)
{
	return sakeSearchForRecords(
		sake,
		input,
		callback,
		userData
	);
}
