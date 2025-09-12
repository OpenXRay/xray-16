///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "sakeMain.h"
#include "sakeRequest.h"
#include "../common/gsAvailable.h"
#include "../common/gsCore.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// General

gsi_char gSakeUploadUrlOverride[SAKE_MAX_URL_LENGTH];
gsi_char gSakeDownloadUrlOverride[SAKE_MAX_URL_LENGTH];


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKEStartupResult SAKE_CALL sakeStartup(SAKE * sakePtr)
{
	SAKE sake;

	GS_ASSERT(sakePtr);

	// check for availability
	if(__GSIACResult != GSIACAvailable)
		return SAKEStartupResult_NOT_AVAILABLE;

	// check that the core is initialized
	if(gsCoreIsShutdown())
		return SAKEStartupResult_CORE_SHUTDOWN;

	// allocate the sake object
	sake = (SAKE)gsimalloc(sizeof(SAKEInternal));
	if(sake == NULL)
		return SAKEStartupResult_OUT_OF_MEMORY;

	// init the sake object
	memset(sake, 0, sizeof(SAKEInternal));

	// store the object in the user pointer
	*sakePtr = sake;

	return SAKEStartupResult_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SAKE_CALL sakeShutdown(SAKE sake)
{
	GS_ASSERT(sake);

	//TODO: ensure that there are no pending operations
	//      that might reference this object

	// free the struct
	gsifree(sake);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Authentication


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SAKE_CALL sakeSetGame(SAKE sake, const gsi_char * gameName, int gameId, const gsi_char *secretKey)
{
	GS_ASSERT(sake);
	GS_ASSERT(gameName && (_tcslen(gameName) <= SAKEI_GAME_NAME_LENGTH));
	GS_ASSERT(gameId >= 0);
	GS_ASSERT(secretKey && (_tcslen(secretKey) <= SAKEI_SECRET_KEY_LENGTH));

#ifdef GSI_UNICODE
	// convert gamename and secretkey to ascii for executing requests
	UCS2ToAsciiString(gameName, sake->mGameName);
	UCS2ToAsciiString(secretKey, sake->mSecretKey);
#else
	strcpy(sake->mGameName, gameName);
	strcpy(sake->mSecretKey, secretKey);
#endif

	sake->mGameId = gameId;
	sake->mIsGameAuthenticated = gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SAKE_CALL sakeSetProfile(SAKE sake, int profileId, const char *loginTicket)
{
	GS_ASSERT(sake);
	GS_ASSERT(loginTicket);
	GS_ASSERT(strlen(loginTicket) == SAKEI_LOGIN_TICKET_LENGTH);

	sake->mProfileId = profileId;
	memcpy(sake->mLoginTicket, loginTicket, SAKEI_LOGIN_TICKET_LENGTH + 1);
	sake->mIsProfileAuthenticated = gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Requests


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKEStartRequestResult SAKE_CALL sakeGetStartRequestResult(SAKE sake)
{
	GS_ASSERT(sake);

	return sake->mStartRequestResult;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static SAKERequest SAKE_CALL sakeiRunRequest(SAKE sake, void *input,
                                             SAKERequestCallback callback, void *userData,
                                             SAKEIRequestType type,
                                             SAKEStartRequestResult (*startRequestFunc)(SAKERequest request))
{
	SAKERequest request;

	GS_ASSERT(sake);

	request = sakeiInitRequest(sake, type, input, callback, userData);
	if(!request)
		return NULL;

	sake->mStartRequestResult = startRequestFunc(request);
	if(sake->mStartRequestResult != SAKEStartRequestResult_SUCCESS)
	{
		sakeiFreeRequest(request);
		return NULL;
	}

	return request;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeCreateRecord(SAKE sake, SAKECreateRecordInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_CREATE_RECORD, sakeiStartCreateRecordRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeUpdateRecord(SAKE sake, SAKEUpdateRecordInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_UPDATE_RECORD, sakeiStartUpdateRecordRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeDeleteRecord(SAKE sake, SAKEDeleteRecordInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_DELETE_RECORD, sakeiStartDeleteRecordRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeSearchForRecords(SAKE sake, SAKESearchForRecordsInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_SEARCH_FOR_RECORDS, sakeiStartSearchForRecordsRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeGetMyRecords(SAKE sake, SAKEGetMyRecordsInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_GET_MY_RECORDS, sakeiStartGetMyRecordsRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeGetSpecificRecords(SAKE sake, SAKEGetSpecificRecordsInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_GET_SPECIFIC_RECORDS, sakeiStartGetSpecificRecordsRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeGetRandomRecord(SAKE sake, SAKEGetRandomRecordInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_GET_RANDOM_RECORD, sakeiStartGetRandomRecordRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeRateRecord(SAKE sake, SAKERateRecordInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_RATE_RECORD, sakeiStartRateRecordRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeGetRecordLimit(SAKE sake, SAKEGetRecordLimitInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_GET_RECORD_LIMIT, sakeiStartGetRecordLimitRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeGetRecordCount(SAKE sake, SAKEGetRecordCountInput *input, SAKERequestCallback callback, void *userData)
{
	return sakeiRunRequest(sake, input, callback, userData, SAKEIRequestType_GET_RECORD_COUNT, sakeiStartGetRecordCountRequest);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// read request utility
SAKEField * SAKE_CALL sakeGetFieldByName(const char *name, SAKEField *fields, int numFields)
{
	int i;

	GS_ASSERT(name);
	GS_ASSERT(fields);
	GS_ASSERT(numFields >= 0);

	for(i = 0 ; i < numFields ; i++)
	{
		if(strcmp(fields[i].mName, name) == 0)
			return &fields[i];
	}

	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Files


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Set the URL format to be used by sakeGetFileDownloadUrl
gsi_bool SAKE_CALL sakeSetFileDownloadURL(SAKE sake, gsi_char url[SAKE_MAX_URL_LENGTH])
{
	GS_ASSERT(sake);
	GS_ASSERT(url);

	if(!sake || !url)
		return gsi_false;

	_tcscpy(gSakeDownloadUrlOverride, url);
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool SAKE_CALL sakeGetFileDownloadURL(SAKE sake, int fileId, gsi_char url[SAKE_MAX_URL_LENGTH])
{
	int rcode;

	GS_ASSERT(sake);
	GS_ASSERT(fileId != 0);
	GS_ASSERT(url);
	GS_ASSERT(sake->mIsGameAuthenticated);
	GS_ASSERT(sake->mIsProfileAuthenticated);

	if(!sake || !url || !sake->mIsGameAuthenticated || !sake->mIsProfileAuthenticated)
		return gsi_false;

	if (gSakeDownloadUrlOverride[0] != '\0')
	{
		rcode = _tsnprintf(url, SAKE_MAX_URL_LENGTH, _T("%s?gameid=%d&pid=%d"), 
			gSakeDownloadUrlOverride, sake->mGameId, sake->mProfileId);
	}
	else
	{
		#ifdef GSI_UNICODE
		{
			// use capital %S to convert the gamename to a wide string
			rcode = _tsnprintf(url, SAKE_MAX_URL_LENGTH, _T("http://%S.sake.%S/SakeFileServer/download.aspx?fileid=%d&gameid=%d&pid=%d"),
				sake->mGameName, GSI_DOMAIN_NAME, fileId, sake->mGameId, sake->mProfileId);
		}
		#else
		{
			rcode = _tsnprintf(url, SAKE_MAX_URL_LENGTH, _T("http://%s.sake.%s/SakeFileServer/download.aspx?fileid=%d&gameid=%d&pid=%d"),
				sake->mGameName, GSI_DOMAIN_NAME, fileId, sake->mGameId, sake->mProfileId);
		}
		#endif
	}

	if(rcode < 0)
		return gsi_false;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool SAKE_CALL sakeSetFileUploadURL(SAKE sake, gsi_char url[SAKE_MAX_URL_LENGTH])
{
	GS_ASSERT(sake);
	GS_ASSERT(url);

	if(!sake || !url)
		return gsi_false;

	_tcscpy(gSakeUploadUrlOverride, url);
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool SAKE_CALL sakeGetFileUploadURL(SAKE sake, gsi_char url[SAKE_MAX_URL_LENGTH])
{
	int rcode;

	GS_ASSERT(sake);
	GS_ASSERT(url);
	GS_ASSERT(sake->mIsGameAuthenticated);
	GS_ASSERT(sake->mIsProfileAuthenticated);

	if(!sake || !url || !sake->mIsGameAuthenticated || !sake->mIsProfileAuthenticated)
		return gsi_false;

	if (gSakeUploadUrlOverride[0] != '\0')
	{
		rcode = _tsnprintf(url, SAKE_MAX_URL_LENGTH, _T("%s?gameid=%d&pid=%d"), 
			gSakeUploadUrlOverride, sake->mGameId, sake->mProfileId);
	}
	else
	{
		#ifdef GSI_UNICODE
		{
			// use capital %S to convert the gamename to a wide string
			rcode = _tsnprintf(url, SAKE_MAX_URL_LENGTH, _T("http://%S.sake.%S/SakeFileServer/upload.aspx?gameid=%d&pid=%d"),
				sake->mGameName, GSI_DOMAIN_NAME, sake->mGameId, sake->mProfileId);
		}
		#else
		{
			rcode = _tsnprintf(url, SAKE_MAX_URL_LENGTH, _T("http://%s.sake.%s/SakeFileServer/upload.aspx?gameid=%d&pid=%d"),
				sake->mGameName, GSI_DOMAIN_NAME, sake->mGameId, sake->mProfileId);
		}
		#endif
	}

	if(rcode < 0)
		return gsi_false;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static SAKEFileResult SAKE_CALL sakeiParseFileResult(int resultCode)
{
	if(resultCode >= SAKEFileResult_UNKNOWN_ERROR)
		return SAKEFileResult_UNKNOWN_ERROR;
	return (SAKEFileResult)resultCode;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_bool SAKE_CALL sakeiGetHeaderValueInt(const char *headers, const char *headerName, int *value)
{
	const char * header;
	int rcode;

	GS_ASSERT(headers);
	GS_ASSERT(headerName);
	GS_ASSERT(value);
#ifdef _DEBUG
	// headerName must include the trailing colon
	GS_ASSERT(headerName[strlen(headerName) - 1] == ':');
#endif

	// find this header in the list of headers
	header = strstr(headers, headerName);
	if(header)
	{
		// skip the header name
		header += strlen(headerName);

		// scan in the result
		rcode = sscanf(header, " %d", value);
		if(rcode == 1)
			return gsi_true;
	}

	return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool SAKE_CALL sakeGetFileResultFromHeaders(const char *headers, SAKEFileResult *result)
{
	int resultCode;
	gsi_bool foundResultCode;

	foundResultCode = sakeiGetHeaderValueInt(headers, SAKE_FILE_RESULT_HEADER, &resultCode);

	if(gsi_is_false(foundResultCode))
		return gsi_false;

	*result = sakeiParseFileResult(resultCode);
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool SAKE_CALL sakeGetFileIdFromHeaders(const char *headers, int *fileId)
{
	return sakeiGetHeaderValueInt(headers, SAKE_FILE_ID_HEADER, fileId);
}
