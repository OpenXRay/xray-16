///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "sakeRequest.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const char * GSI_SAKE_SERVICE_NAMESPACES[GSI_SAKE_SERVICE_NAMESPACE_COUNT] =
{
	"ns1=\"http://gamespy.net/sake\""
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define SAKEI_SOAP_URL_FORMAT   "http://%s.sake." GSI_DOMAIN_NAME "/SakeStorageServer/StorageServer.asmx"
char sakeiSoapUrl[SAKE_MAX_URL_LENGTH] = "";


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKERequest SAKE_CALL sakeiInitRequest(SAKE sake, SAKEIRequestType type, void *input, SAKERequestCallback callback, void *userData)
{
	SAKERequest request;

	GS_ASSERT(sake);

	// init the request init result to success
	sake->mStartRequestResult = SAKEStartRequestResult_SUCCESS;

	// check for input
	if(!input)
	{
		sake->mStartRequestResult = SAKEStartRequestResult_BAD_INPUT;
		return NULL;
	}

	// check for authentication
	if(gsi_is_false(sake->mIsGameAuthenticated) || gsi_is_false(sake->mIsProfileAuthenticated))
	{
		sake->mStartRequestResult = SAKEStartRequestResult_NOT_AUTHENTICATED;
		return NULL;
	}

	// allocate memory for the request object
	request = (SAKERequest)gsimalloc(sizeof(SAKERequestInternal));
	if(!request)
	{
		sake->mStartRequestResult = SAKEStartRequestResult_OUT_OF_MEMORY;
		return NULL;
	}

	// init the request object
	memset(request, 0, sizeof(SAKERequestInternal));
	request->mSake = sake;
	request->mType = type;
	request->mInput = input;
	request->mCallback = callback;
	request->mUserData = userData;

	return request;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void SAKE_CALL sakeiFreeRequest(SAKERequest request)
{
	GS_ASSERT(request);

	// this should already be freed by the time we get here
	GS_ASSERT(request->mOutput == NULL);

	// free the request
	gsifree(request);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static SAKERequestResult SAKE_CALL sakeiCheckHttpResult(GHTTPResult httpResult)
{
	switch(httpResult)
	{
	case GHTTPSuccess:
		return SAKERequestResult_SUCCESS;
	case GHTTPOutOfMemory:
		return SAKERequestResult_OUT_OF_MEMORY;
	default:
		return SAKERequestResult_CONNECTION_ERROR;
	}
}

static SAKERequestResult SAKE_CALL sakeiCheckSakeResult(const char * sakeResult)
{
	if(strcmp(sakeResult, "Success") == 0)
		return SAKERequestResult_SUCCESS;
	else if(strcmp(sakeResult, "SecretKeyInvalid") == 0)
		return SAKERequestResult_SECRET_KEY_INVALID;
	else if(strcmp(sakeResult, "ServiceDisabled") == 0)
		return SAKERequestResult_SERVICE_DISABLED;
	else if(strcmp(sakeResult, "DatabaseUnavailable") == 0)
		return SAKERequestResult_DATABASE_UNAVAILABLE;
	else if(strcmp(sakeResult, "LoginTicketInvalid") == 0)
		return SAKERequestResult_LOGIN_TICKET_INVALID;
	else if(strcmp(sakeResult, "LoginTicketExpired") == 0)
		return SAKERequestResult_LOGIN_TICKET_EXPIRED;
	else if(strcmp(sakeResult, "TableNotFound") == 0)
		return SAKERequestResult_TABLE_NOT_FOUND;
	else if(strcmp(sakeResult, "RecordNotFound") == 0)
		return SAKERequestResult_RECORD_NOT_FOUND;
	else if(strcmp(sakeResult, "FieldNotFound") == 0)
		return SAKERequestResult_FIELD_NOT_FOUND;
	else if(strcmp(sakeResult, "FieldTypeInvalid") == 0)
		return SAKERequestResult_FIELD_TYPE_INVALID;
	else if(strcmp(sakeResult, "NoPermission") == 0)
		return SAKERequestResult_NO_PERMISSION;
	else if(strcmp(sakeResult, "RecordLimitReached") == 0)
		return SAKERequestResult_RECORD_LIMIT_REACHED;
	else if(strcmp(sakeResult, "AlreadyRated") == 0)
		return SAKERequestResult_ALREADY_RATED;
	else if(strcmp(sakeResult, "NotRateable") == 0)
		return SAKERequestResult_NOT_RATEABLE;
	else if(strcmp(sakeResult, "NotOwned") == 0)
		return SAKERequestResult_NOT_OWNED;
	else if(strcmp(sakeResult, "FilterInvalid") == 0)
		return SAKERequestResult_FILTER_INVALID;
	else if(strcmp(sakeResult, "SortInvalid") == 0)
		return SAKERequestResult_SORT_INVALID;
	else if(strcmp(sakeResult, "TargetFilterInvalid") == 0)
		return SAKERequestResult_TARGET_FILTER_INVALID;
	else
		return SAKERequestResult_UNKNOWN_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void SAKE_CALL sakeiSoapCallback(GHTTPResult httpResult, GSXmlStreamWriter requestData, GSXmlStreamReader responseData, void *userData)
{
	SAKERequest request = (SAKERequest)userData;
	void *output = NULL;
	SAKERequestResult result;
	char resultString[32];

	// sanity check
	GS_ASSERT(request);
	GS_ASSERT(request->mSake);
	GS_ASSERT(request->mInfo);
	if(!request || !request->mSake || !request->mInfo)
		return;

	result = sakeiCheckHttpResult(httpResult);
	if(result == SAKERequestResult_SUCCESS)
	{
		if(gsi_is_false(gsXmlMoveToStart(responseData)) ||
		   gsi_is_false(gsXmlMoveToNext(responseData, request->mInfo->mResponseTag)) ||
		   gsi_is_false(gsXmlReadChildAsStringNT(responseData, request->mInfo->mResultTag, resultString, sizeof(resultString))))
		{
			result = SAKERequestResult_MALFORMED_RESPONSE;
		}
		else
		{
			result = sakeiCheckSakeResult(resultString);

			// fill in the output
			if(result == SAKERequestResult_SUCCESS)
			{
				if(request->mInfo->mSakeOutputSize != 0)
				{
					request->mOutput = gsimalloc(request->mInfo->mSakeOutputSize);
					if(request->mOutput)
					{
						request->mSoapResponse = responseData;
						result = request->mInfo->mProcessSoapResponseFunc(request);
						if(result == SAKERequestResult_SUCCESS)
							output = request->mOutput;
					}
					else
					{
						result = SAKERequestResult_OUT_OF_MEMORY;
					}
				}
			}
		}
	}

	// call the callback
	if(request->mCallback)
		request->mCallback(request->mSake, request, result, request->mInput, output, request->mUserData);

	// free data
	if(request->mInfo->mFreeDataFunc)
		request->mInfo->mFreeDataFunc(request);

	// free the output data
	gsifree(request->mOutput);
	request->mOutput = NULL;

	// free the sake request
	sakeiFreeRequest(request);

	GSI_UNUSED(requestData);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static SAKEStartRequestResult SAKE_CALL sakeiSetupRequest(SAKERequest request)
{
	SAKEStartRequestResult result;
	SAKEIRequestInfo * info;

	GS_ASSERT(request);
	GS_ASSERT(request->mSake);
	GS_ASSERT(request->mInfo);

	// store a utility pointer to the info
	info = request->mInfo;

	// check the input
	result = info->mValidateInputFunc(request);
	if(result != SAKEStartRequestResult_SUCCESS)
		return result;

	// create the xml request stream
	request->mSoapRequest = gsXmlCreateStreamWriter(GSI_SAKE_SERVICE_NAMESPACES, GSI_SAKE_SERVICE_NAMESPACE_COUNT);
	if(request->mSoapRequest == NULL)
		return SAKEStartRequestResult_OUT_OF_MEMORY;

	// open the stream
	gsXmlWriteOpenTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, request->mInfo->mFuncName);

	// this info is included with every request
	gsXmlWriteIntElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "gameid", (gsi_u32)request->mSake->mGameId);
	gsXmlWriteStringElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "secretKey", request->mSake->mSecretKey);
	gsXmlWriteStringElement(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, "loginTicket", request->mSake->mLoginTicket);

	// fill in the request data
	result = info->mFillSoapRequestFunc(request);
	if(result != SAKEStartRequestResult_SUCCESS)
	{
		gsXmlFreeWriter(request->mSoapRequest);
		request->mSoapRequest = NULL;
		if(info->mFreeDataFunc)
			info->mFreeDataFunc(request);
		return result;
	}

	// close the stream and writer
	gsXmlWriteCloseTag(request->mSoapRequest, GSI_SAKE_SERVICE_NAMESPACE, request->mInfo->mFuncName);
	gsXmlCloseWriter(request->mSoapRequest);

	return SAKEStartRequestResult_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void SAKE_CALL sakeiExecuteRequest(SAKERequest request)
{
	if(sakeiSoapUrl[0] == '\0')
	{
		int rcode;
		rcode = snprintf(sakeiSoapUrl, SAKE_MAX_URL_LENGTH, SAKEI_SOAP_URL_FORMAT, request->mSake->mGameName);
		GS_ASSERT(rcode >= 0);
		GSI_UNUSED(rcode);
	}
	gsiExecuteSoap(sakeiSoapUrl, request->mInfo->mSoapAction, request->mSoapRequest, sakeiSoapCallback, request);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SAKEStartRequestResult SAKE_CALL sakeiStartRequest(SAKERequest request, SAKEIRequestInfo * info)
{
	SAKEStartRequestResult result;

	request->mInfo = info;

	result = sakeiSetupRequest(request);
	if(result != SAKEStartRequestResult_SUCCESS)
		return result;

	sakeiExecuteRequest(request);

	return SAKEStartRequestResult_SUCCESS;
}
