#include "../../common/gsCommon.h"
#include "../../common/gsAvailable.h"
#include "../../common/gsCore.h"
#include "../../common/gsSoap.h"
#include "../sake.h"
#include "../../ghttp/ghttp.h"
#include "../../gp/gp.h"

#if defined(_NITRO)
	#include "../../common/nitro/screen.h"
	#define printf Printf
	#define vprintf VPrintf
#endif

#define URL NULL

#define GAMENAME	_T("gmtest")
#define SECRET_KEY	_T("HA6zkS")
#define GAMEID		0

#define PRODUCTID	0		
#define NAMESPACEID 0
#define NICKNAME    _T("gptestc1")
#define EMAIL       _T("gptestc@gptestc.com")
#define PASSWORD    _T("gptestc")


#define CHECK_GP_RESULT(func, errString) if(func != GP_NO_ERROR) { printf("%s\n", errString); exit(0); }

#define SAKE_UPLOAD_AMOUNT 5028


typedef struct DataStruct
{
	char pData[SAKE_UPLOAD_AMOUNT];
} DataStruct;

int NumOperations = 0;
GPConnection * pconn;
GPProfile profileid;

#ifdef GSI_COMMON_DEBUG
	static void DebugCallback(GSIDebugCategory theCat, GSIDebugType theType,
	                          GSIDebugLevel theLevel, const char * theTokenStr,
	                          va_list theParamList)
	{
		GSI_UNUSED(theLevel);

		printf("[%s][%s] ", 
				gGSIDebugCatStrings[theCat], 
				gGSIDebugTypeStrings[theType]);

		vprintf(theTokenStr, theParamList);
	}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static GSIACResult CheckServices(void)
{
	GSIACResult aResult;
	GSIStartAvailableCheck(GAMENAME);

	// Continue processing while the check is in progress
	do
	{
		aResult = GSIAvailableCheckThink();
		msleep(10);
	}
	while(aResult == GSIACWaiting);

	// Check the result
	switch(aResult)
	{
	case GSIACAvailable:
		printf("Online Services are available\r\n");
		break;
	case GSIACUnavailable:
		printf("Online services are unavailable\r\n");
		printf("Please visit www.mygame.com for more information.\r\n");
		break;
	case GSIACTemporarilyUnavailable:
		printf("Online services are temporarily unavailable.\r\n");
		printf("Please visit www.mygame.com for more information.\r\n");
		break;
	default:
		break;
	};
		
	return aResult;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void ConnectResponse(GPConnection * pconnection, GPConnectResponseArg * arg, void * param)
{
	if(arg->result == GP_NO_ERROR)
		printf("Connected to GP\n");
	else
		printf("GP Connection Attempt Failed\n");

	profileid = arg->profile;
		
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

//GP callbacks, everything is a noop except for the error callback
static void Error(GPConnection * pconnection, GPErrorArg * arg, void * param)
{
	gsi_char * errorCodeString;
	gsi_char * resultString;

#define RESULT(x) case x: resultString = _T(#x); break;
	switch(arg->result)
	{
	RESULT(GP_NO_ERROR)
	RESULT(GP_MEMORY_ERROR)
	RESULT(GP_PARAMETER_ERROR)
	RESULT(GP_NETWORK_ERROR)
	RESULT(GP_SERVER_ERROR)
	default:
		resultString = _T("Unknown result!\n");
	}

#define ERRORCODE(x) case x: errorCodeString = _T(#x); break;
	switch(arg->errorCode)
	{
	ERRORCODE(GP_GENERAL)
	ERRORCODE(GP_PARSE)
	ERRORCODE(GP_NOT_LOGGED_IN)
	ERRORCODE(GP_BAD_SESSKEY)
	ERRORCODE(GP_DATABASE)
	ERRORCODE(GP_NETWORK)
	ERRORCODE(GP_FORCED_DISCONNECT)
	ERRORCODE(GP_CONNECTION_CLOSED)
	ERRORCODE(GP_LOGIN)
	ERRORCODE(GP_LOGIN_TIMEOUT)
	ERRORCODE(GP_LOGIN_BAD_NICK)
	ERRORCODE(GP_LOGIN_BAD_EMAIL)
	ERRORCODE(GP_LOGIN_BAD_PASSWORD)
	ERRORCODE(GP_LOGIN_BAD_PROFILE)
	ERRORCODE(GP_LOGIN_PROFILE_DELETED)
	ERRORCODE(GP_LOGIN_CONNECTION_FAILED)
	ERRORCODE(GP_LOGIN_SERVER_AUTH_FAILED)
	ERRORCODE(GP_NEWUSER)
	ERRORCODE(GP_NEWUSER_BAD_NICK)
	ERRORCODE(GP_NEWUSER_BAD_PASSWORD)
	ERRORCODE(GP_UPDATEUI)
	ERRORCODE(GP_UPDATEUI_BAD_EMAIL)
	ERRORCODE(GP_NEWPROFILE)
	ERRORCODE(GP_NEWPROFILE_BAD_NICK)
	ERRORCODE(GP_NEWPROFILE_BAD_OLD_NICK)
	ERRORCODE(GP_UPDATEPRO)
	ERRORCODE(GP_UPDATEPRO_BAD_NICK)
	ERRORCODE(GP_ADDBUDDY)
	ERRORCODE(GP_ADDBUDDY_BAD_FROM)
	ERRORCODE(GP_ADDBUDDY_BAD_NEW)
	ERRORCODE(GP_ADDBUDDY_ALREADY_BUDDY)
	ERRORCODE(GP_AUTHADD)
	ERRORCODE(GP_AUTHADD_BAD_FROM)
	ERRORCODE(GP_AUTHADD_BAD_SIG)
	ERRORCODE(GP_STATUS)
	ERRORCODE(GP_BM)
	ERRORCODE(GP_BM_NOT_BUDDY)
	ERRORCODE(GP_GETPROFILE)
	ERRORCODE(GP_GETPROFILE_BAD_PROFILE)
	ERRORCODE(GP_DELBUDDY)
	ERRORCODE(GP_DELBUDDY_NOT_BUDDY)
	ERRORCODE(GP_DELPROFILE)
	ERRORCODE(GP_DELPROFILE_LAST_PROFILE)
	ERRORCODE(GP_SEARCH)
	ERRORCODE(GP_SEARCH_CONNECTION_FAILED)
	default:
		errorCodeString = _T("Unknown error code!\n");
	}

	if(arg->fatal)
	{
		printf( "-----------\n");
		printf( "GP FATAL ERROR\n");
		printf( "-----------\n");
	}
	else
	{
		printf( "-----\n");
		printf( "GP ERROR\n");
		printf( "-----\n");
	}
	_tprintf( _T("RESULT: %s (%d)\n"), resultString, arg->result);
	_tprintf( _T("ERROR CODE: %s (0x%X)\n"), errorCodeString, arg->errorCode);
	_tprintf( _T("ERROR STRING: %s\n"), arg->errorString);
	
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

static void RecvBuddyRequest(GPConnection * pconnection, GPRecvBuddyMessageArg * arg, void * param)
{
	GSI_UNUSED(arg);
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}


static void RecvBuddyStatus(GPConnection * pconnection, GPRecvBuddyMessageArg * arg, void * param)
{
	GSI_UNUSED(arg);
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

static void RecvBuddyMessage(GPConnection * pconnection, GPRecvBuddyMessageArg * arg, void * param)
{
	GSI_UNUSED(arg);
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

static void RecvGameInvite(GPConnection * pconnection, GPRecvBuddyMessageArg * arg, void * param)
{
	GSI_UNUSED(arg);
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}


static void TransferCallback(GPConnection * pconnection, GPRecvBuddyMessageArg * arg, void * param)
{
	GSI_UNUSED(arg);
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

static void RecvBuddyAuth(GPConnection * pconnection, GPRecvBuddyMessageArg * arg, void * param)
{
	GSI_UNUSED(arg);
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

static void RecvBuddyRevoke(GPConnection * pconnection, GPRecvBuddyMessageArg * arg, void * param)
{
	GSI_UNUSED(arg);
	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int UnicodeStringLen(const unsigned short * str)
{
	const unsigned short * end = str;
	while(*end++)
		{}
	return (end - str - 1);
}

static void PrintSeperator(void)
{
	printf("*******************\n");
}

static const char * FieldTypeToString(SAKEField * field)
{
	static char buffer[32];
	SAKEFieldType type = field->mType;

	if(type == SAKEFieldType_BYTE)
		return "byte";
	if(type == SAKEFieldType_SHORT)
		return "short";
	if(type == SAKEFieldType_INT)
		return "int";
	if(type == SAKEFieldType_FLOAT)
		return "float";
	if(type == SAKEFieldType_ASCII_STRING)
		return "asciiString";
	if(type == SAKEFieldType_UNICODE_STRING)
		return "unicodeString";
	if(type == SAKEFieldType_BOOLEAN)
		return "boolean";
	if(type == SAKEFieldType_DATE_AND_TIME)
		return "dateAndTime";
	if(type == SAKEFieldType_BINARY_DATA)
	{
		sprintf(buffer, "binaryData-%d", field->mValue.mBinaryData.mLength);
		return buffer;
	}
	if (type == SAKEFieldType_INT64)
	{
		return "int64";
	}
	return "ERROR!!  Invalid value type set";
}

static const char * FieldValueToString(SAKEField * field)
{
	static char buffer[64];
	SAKEFieldType type = field->mType;
	SAKEValue * value = &field->mValue;

	if(type == SAKEFieldType_BYTE)
		sprintf(buffer, "%d", (int)value->mByte);
	else if(type == SAKEFieldType_SHORT)
		sprintf(buffer, "%d", (int)value->mShort);
	else if(type == SAKEFieldType_INT)
		sprintf(buffer, "%d", value->mInt);
	else if(type == SAKEFieldType_FLOAT)
		sprintf(buffer, "%0.3f", value->mFloat);
	else if(type == SAKEFieldType_ASCII_STRING)
		return value->mAsciiString;
	else if(type == SAKEFieldType_UNICODE_STRING)
	{
		// cap the value if it is too long (note, this is destructive)
		if(UnicodeStringLen(value->mUnicodeString) > 20)
			value->mUnicodeString[20] = 0;
		UCS2ToAsciiString(value->mUnicodeString, buffer);
	}
	else if(type == SAKEFieldType_BOOLEAN)
		return (value->mBoolean)?"true":"false";
	else if(type == SAKEFieldType_DATE_AND_TIME)
	{
		char * str = gsiSecondsToString(&value->mDateAndTime);
		str[strlen(str) - 1] = '\0';
		return str;
	}
	else if(type == SAKEFieldType_BINARY_DATA)
	{
		int i;
		int len = min(value->mBinaryData.mLength, 8);
		for(i = 0 ; i < len ; i++)
			sprintf(buffer + (len*2), "%0X", value->mBinaryData.mValue[i]);
		buffer[len*2] = '\0';
	}
	else if(type == SAKEFieldType_INT64)
	{
		gsiInt64ToString(buffer, value->mInt64);
	}
	else
		return "ERROR!!  Invalid value type set";

	return buffer;
}

static const char * RequestResultToString(SAKERequestResult requestResult)
{
	switch(requestResult)
	{
	case SAKERequestResult_SUCCESS:
		return "SUCCESS";
	case SAKERequestResult_CONNECTION_TIMEOUT:
		return "CONNECTION_TIMEOUT";
	case SAKERequestResult_CONNECTION_ERROR:
		return "CONNECTION_ERROR";
	case SAKERequestResult_MALFORMED_RESPONSE:
		return "MALFORMED_RESPONSE";
	case SAKERequestResult_OUT_OF_MEMORY:
		return "OUT_OF_MEMORY";
	case SAKERequestResult_DATABASE_UNAVAILABLE:
		return "DATABASE_UNAVAILABLE";
	case SAKERequestResult_LOGIN_TICKET_INVALID:
		return "LOGIN_TICKET_INVALID";
	case SAKERequestResult_LOGIN_TICKET_EXPIRED:
		return "LOGIN_TICKET_EXPIRED";
	case SAKERequestResult_TABLE_NOT_FOUND:
		return "TABLE_NOT_FOUND";
	case SAKERequestResult_RECORD_NOT_FOUND:
		return "RECORD_NOT_FOUND";
	case SAKERequestResult_FIELD_NOT_FOUND:
		return "FIELD_NOT_FOUND";
	case SAKERequestResult_FIELD_TYPE_INVALID:
		return "FIELD_TYPE_INVALID";
	case SAKERequestResult_NO_PERMISSION:
		return "NO_PERMISSION";
	case SAKERequestResult_RECORD_LIMIT_REACHED:
		return "RECORD_LIMIT_REACHED";
	case SAKERequestResult_ALREADY_RATED:
		return "ALREADY_RATED";
	case SAKERequestResult_NOT_RATEABLE:
		return "NOT_RATEABLE";
	case SAKERequestResult_NOT_OWNED:
		return "NOT_OWNED";
	case SAKERequestResult_FILTER_INVALID:
		return "FILTER_INVALID";
	case SAKERequestResult_SORT_INVALID:
		return "SORT_INVALID";
	case SAKERequestResult_UNKNOWN_ERROR:
		return "UNKNOWN_ERROR";
	default:
		break;
	}

	return "Unrecognized error";
}

// prints request result, returns gsi_false for errors
static gsi_bool HandleRequestResult(SAKERequestResult requestResult, const char * requestType)
{
	if(requestResult != SAKERequestResult_SUCCESS)
	{
		printf("%s - Error - %s\n", requestType, RequestResultToString(requestResult));
		return gsi_false;
	}

	printf("%s - Success\n", requestType);
	return gsi_true;
}

static void DisplayReadResults(SAKEField ** records, int numRecords, int numFields)
{
	SAKEField * field;
	int recordIndex;
	int fieldIndex;

	PrintSeperator();
	printf("Num Records: %d\n", numRecords);
	PrintSeperator();

	for(recordIndex = 0 ; recordIndex < numRecords ; recordIndex++)
	{
		for(fieldIndex = 0 ; fieldIndex < numFields ; fieldIndex++)
		{
			field = &records[recordIndex][fieldIndex];
			printf("%s[%s]=%s\n", field->mName, FieldTypeToString(field), FieldValueToString(field));
		}

		PrintSeperator();
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CreateRecordCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	SAKECreateRecordOutput * output = (SAKECreateRecordOutput *)outputData;

	NumOperations--;

	if(HandleRequestResult(result, "CreateRecord") == gsi_false)
		return;
	
	printf("Created recordid %d\n", output->mRecordId);

	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(inputData);
	GSI_UNUSED(userData);
}

static void UpdateRecordCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	NumOperations--;
	
	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(inputData);
	GSI_UNUSED(outputData);
	GSI_UNUSED(userData);

	if(HandleRequestResult(result, "UpdateRecord") == gsi_false)
		return;
}

static void DeleteRecordCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	NumOperations--;

	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(inputData);
	GSI_UNUSED(outputData);
	GSI_UNUSED(userData);

	if(HandleRequestResult(result, "DeleteRecord") == gsi_false)
		return;
}

static void SearchForRecordsCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	SAKESearchForRecordsInput * input = (SAKESearchForRecordsInput *)inputData;
	SAKESearchForRecordsOutput * output = (SAKESearchForRecordsOutput *)outputData;

	NumOperations--;

	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(userData);

	if(HandleRequestResult(result, "SearchForRecords") == gsi_false)
		return;

	DisplayReadResults(output->mRecords, output->mNumRecords, input->mNumFields);
}

static void GetMyRecordsCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	SAKEGetMyRecordsInput * input = (SAKEGetMyRecordsInput *)inputData;
	SAKEGetMyRecordsOutput * output = (SAKEGetMyRecordsOutput *)outputData;

	NumOperations--;

	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(userData);

	if(HandleRequestResult(result, "GetMyRecords") == gsi_false)
		return;

	DisplayReadResults(output->mRecords, output->mNumRecords, input->mNumFields);
}

static void GetSpecificRecordsCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	SAKEGetSpecificRecordsInput * input = (SAKEGetSpecificRecordsInput *)inputData;
	SAKEGetSpecificRecordsOutput * output = (SAKEGetSpecificRecordsOutput *)outputData;

	NumOperations--;

	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(userData);

	if(HandleRequestResult(result, "GetSpecificRecords") == gsi_false)
		return;

	if(output->mNumRecords > 0)
	{
		SAKEField * field;
		field = sakeGetFieldByName("score", output->mRecords[0], input->mNumFields);
		GSI_UNUSED(field);
	}
	else
	{
		printf("No record found\n");
	}

	DisplayReadResults(output->mRecords, output->mNumRecords, input->mNumFields);
}

static void GetRandomRecordCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	SAKEGetRandomRecordInput * input = (SAKEGetRandomRecordInput *)inputData;
	SAKEGetRandomRecordOutput * output = (SAKEGetRandomRecordOutput *)outputData;
	SAKEField *records[1];
	int        numRecords;

	NumOperations--;

	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(userData);

	if(HandleRequestResult(result, "GetRandomRecord") == gsi_false)
		return;

	records[0] = output->mRecord;
	if(records[0] != NULL)
		numRecords = 1;
	else
		numRecords = 0;

	DisplayReadResults(records, numRecords, input->mNumFields);
}

static void RateRecordCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	SAKERateRecordOutput * output = (SAKERateRecordOutput *)outputData;

	NumOperations--;

	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(inputData);
	GSI_UNUSED(userData);

	if(HandleRequestResult(result, "RateRecord") == gsi_false)
		return;

	printf("NumRatings: %d\n", output->mNumRatings);
	printf("AverageRating: %0.3f\n", output->mAverageRating);
}

static void GetRecordLimitCallback(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData)
{
	SAKEGetRecordLimitOutput * output = (SAKEGetRecordLimitOutput *)outputData;

	NumOperations--;

	GSI_UNUSED(sake);
	GSI_UNUSED(request);
	GSI_UNUSED(inputData);
	GSI_UNUSED(userData);

	if(HandleRequestResult(result, "GetRecordLimit") == gsi_false)
		return;

	printf("LimitPerOwner: %d\n", output->mLimitPerOwner);
	printf("NumOwned: %d\n", output->mNumOwned);
}

static gsi_bool gUploadResult;
static int gUploadedFileId;
static GHTTPBool UploadCompletedCallback(GHTTPRequest request, GHTTPResult result, char * buffer, GHTTPByteCount bufferLen, void * param)
{
	SAKEFileResult fileResult;

	gUploadResult = gsi_false;
	NumOperations--;

	GSI_UNUSED(param);
	GSI_UNUSED(bufferLen);
	GSI_UNUSED(buffer);

	if(result != GHTTPSuccess)
	{
		printf("File Upload: GHTTP Error: %d\n", result);
		return GHTTPTrue;
	}

	if(!sakeGetFileResultFromHeaders(ghttpGetHeaders(request), &fileResult))
	{
		printf("File Upload: Failed to find Sake-File-Result header\n");
		return GHTTPTrue;
	}

	if(fileResult != SAKEFileResult_SUCCESS)
	{
		printf("File Upload: SakeFileResult != success: %d\n", fileResult);
		return GHTTPTrue;
	}

	if(!sakeGetFileIdFromHeaders(ghttpGetHeaders(request), &gUploadedFileId))
	{
		printf("File Upload: Unable to parse Sake-File-Id header\n");
		return GHTTPTrue;
	}

	printf("File Upload: Uploaded fileId: %d\n", gUploadedFileId);
	gUploadResult = gsi_true;

	return GHTTPTrue;
}

static GHTTPBool DownloadCompletedCallback(GHTTPRequest request, GHTTPResult result, char * buffer, GHTTPByteCount bufferLen, void * param)
{
	SAKEFileResult fileResult;

	NumOperations--;

	GSI_UNUSED(param);
	GSI_UNUSED(buffer);

	if(result != GHTTPSuccess)
	{
		printf("File Download: GHTTP Error: %d\n", result);
		return GHTTPTrue;
	}

	if(!sakeGetFileResultFromHeaders(ghttpGetHeaders(request), &fileResult))
	{
		printf("File Download: Failed to find Sake-File-Result header\n");
		return GHTTPTrue;
	}

	if(fileResult != SAKEFileResult_SUCCESS)
	{
		printf("File Download: SakeFileResult != success: %d\n", fileResult);
		return GHTTPTrue;
	}

	printf("File Download: Downloaded %d byte file\n", bufferLen);
 
	return GHTTPTrue;
}

static void postCallback(GHTTPRequest request, int bytesPosted, int totalBytes, int objectsPosted, int totalObjects, void * param)
{
	printf("==============================\n");
	printf("* bytesPosted: %7d\n", bytesPosted);
	printf("* totalBytes: %7d\n", totalBytes);
	printf("* objectsPosted: %7d\n", objectsPosted);
	printf("* totalObjects: %7d\n", totalObjects);
	GSI_UNUSED(request);
	GSI_UNUSED(param);
} 

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void RunTests(SAKE sake)
{
#if 1
	{
		static SAKECreateRecordInput input;
		static SAKEField field;
		SAKERequest request;
		SAKEStartRequestResult startRequestResult;

		input.mTableId = "test";
		field.mName = "MyAsciiString";
		field.mType = SAKEFieldType_ASCII_STRING;
		field.mValue.mAsciiString = "this is a record";
		input.mFields = &field;
		input.mNumFields = 1;

		request = sakeCreateRecord(sake, &input, CreateRecordCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		static SAKEUpdateRecordInput input;
		static SAKEField fields[16];
		static char binaryData[] = "\x12\x34\x56\xAB\xCD";
#if defined(_PS2) || defined(_MACOSX)
		static gsi_u16 unicodeString[] = {'u','n','i','c','o','d','e','\0'};
#else
		static gsi_u16 *unicodeString = L"unicode";
#endif
		int index = 0;
		SAKERequest request;
		SAKEStartRequestResult startRequestResult;
		

		input.mTableId = "test";
		input.mRecordId = 158;
		fields[index].mName = "MyByte";
		fields[index].mType = SAKEFieldType_BYTE;
		fields[index].mValue.mByte = 123;
		index++;
		fields[index].mName = "MyShort";
		fields[index].mType = SAKEFieldType_SHORT;
		fields[index].mValue.mShort = 12345;
		index++;
		fields[index].mName = "MyInt";
		fields[index].mType = SAKEFieldType_INT;
		fields[index].mValue.mInt = 123456789;
		index++;
		fields[index].mName = "MyFloat";
		fields[index].mType = SAKEFieldType_FLOAT;
		fields[index].mValue.mFloat = 3.14159265f;
		index++;
		fields[index].mName = "MyAsciiString";
		fields[index].mType = SAKEFieldType_ASCII_STRING;
		fields[index].mValue.mAsciiString = "ascii";
		index++;
		fields[index].mName = "MyUnicodeString";
		fields[index].mType = SAKEFieldType_UNICODE_STRING;
		fields[index].mValue.mUnicodeString = unicodeString;
		index++;
		fields[index].mName = "MyBoolean";
		fields[index].mType = SAKEFieldType_BOOLEAN;
		fields[index].mValue.mBoolean = gsi_true;
		index++;
		fields[index].mName = "MyDateAndTime";
		fields[index].mType = SAKEFieldType_DATE_AND_TIME;
		fields[index].mValue.mDateAndTime = time(NULL);
		index++;
		fields[index].mName = "MyBinaryData";
		fields[index].mType = SAKEFieldType_BINARY_DATA;
		fields[index].mValue.mBinaryData.mValue = (gsi_u8*) binaryData;
		fields[index].mValue.mBinaryData.mLength = (int)strlen(binaryData);
		index++;
		input.mFields = fields;
		input.mNumFields = index;

		request = sakeUpdateRecord(sake, &input, UpdateRecordCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		static SAKEDeleteRecordInput input;
		SAKERequest request;
		SAKEStartRequestResult startRequestResult;

		input.mTableId = "test";
		input.mRecordId = 150;

		request = sakeDeleteRecord(sake, &input, DeleteRecordCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		static SAKESearchForRecordsInput input;
		static SAKERequest request;
		static char *fieldNames[] = { "score", "recordid" };
		SAKEStartRequestResult startRequestResult;

		input.mTableId = "scores";
		input.mFieldNames = fieldNames;
		input.mNumFields = (sizeof(fieldNames) / sizeof(fieldNames[0]));
		input.mFilter = _T("score < 50");
		input.mSort = "score desc";
		input.mOffset = 0;
		input.mMaxRecords = 3;

		request = sakeSearchForRecords(sake, &input, SearchForRecordsCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		static SAKEGetMyRecordsInput input;
		static SAKERequest request;
		static char *fieldNames[] = { "recordid", "ownerid", "MyByte", "MyShort",
		                              "MyInt", "MyFloat", "MyAsciiString",
		                              "MyUnicodeString", "MyBoolean", "MyDateAndTime",
		                              "MyBinaryData", "MyFileID", "num_ratings",
		                              "average_rating" };
		SAKEStartRequestResult startRequestResult;

		input.mTableId = "test";
		input.mFieldNames = fieldNames;
		input.mNumFields = (sizeof(fieldNames) / sizeof(fieldNames[0]));

		request = sakeGetMyRecords(sake, &input, GetMyRecordsCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		static SAKEGetSpecificRecordsInput input;
		static SAKERequest request;
		static int recordIds[] = { 1, 2, 4, 5 };
		static char *fieldNames[] = { "recordid", "ownerid", "score" };
		SAKEStartRequestResult startRequestResult;

		input.mTableId = "scores";
		input.mRecordIds = recordIds;
		input.mNumRecordIds = (sizeof(recordIds) / sizeof(recordIds[0]));
		input.mFieldNames = fieldNames;
		input.mNumFields = (sizeof(fieldNames) / sizeof(fieldNames[0]));

		request = sakeGetSpecificRecords(sake, &input, GetSpecificRecordsCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		static SAKEGetRandomRecordInput input;
		static SAKERequest request;
		//static char *fieldNames[] = { "recordid", "matchname" };
		static char *fieldNames[] = { "recordid", "score" };
		SAKEStartRequestResult startRequestResult;

		input.mTableId = "levels";
		//input.mTableId = "archive";
		input.mFieldNames = fieldNames;
		input.mNumFields = (sizeof(fieldNames) / sizeof(fieldNames[0]));
		input.mFilter = NULL;
		//input.mFilter = "my_rating > 100";
		request = sakeGetRandomRecord(sake, &input, GetRandomRecordCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		static SAKERateRecordInput input;
		static SAKERequest request;
		SAKEStartRequestResult startRequestResult;

		input.mTableId = "test";
		input.mRecordId = 158;
		input.mRating = 200;

		request = sakeRateRecord(sake, &input, RateRecordCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		static SAKEGetRecordLimitInput input;
		static SAKERequest request;
		SAKEStartRequestResult startRequestResult;

		input.mTableId = "nicks";

		request = sakeGetRecordLimit(sake, &input, GetRecordLimitCallback, NULL);
		if(!request)
		{
			startRequestResult = sakeGetStartRequestResult(sake);
			printf("Failed to start request: %d\n", startRequestResult);
		}
		else
		{
			NumOperations++;
		}
	}
#endif
#if 1
	{
		GHTTPPost post;
		GHTTPRequest request;
		//const char *memFile = "This is a test file";
		gsi_char url[SAKE_MAX_URL_LENGTH];
		
		int i;
		DataStruct data;

		for (i=0; i<SAKE_UPLOAD_AMOUNT-1; i++)
		{
			//put in random data into the struct
			data.pData[i] = 'a';
		}
		data.pData[i] = '\0';


		ghttpStartup();

		// get an upload url
		if(sakeGetFileUploadURL(sake, url))
			_tprintf(_T("Upload URL: %s\n"), url);
		else
			printf("Failed to get upload url!\n");

		// upload a file
		post = ghttpNewPost();
		ghttpPostSetCallback(post, postCallback, NULL);
		//ghttpPostAddFileFromMemory(post, "memory.file", memFile, (int)strlen(memFile), "memory.file", NULL);
		ghttpPostAddFileFromMemory(post, _T("memory.file"), (const char *)&data, SAKE_UPLOAD_AMOUNT, _T("memory.file"), NULL);
		request = ghttpPostEx(url, NULL, post, GHTTPFalse, GHTTPTrue, NULL, UploadCompletedCallback, NULL);
		
		if(request == -1)
		{
			printf("Error starting file upload\n");
		}
		else
		{
			NumOperations++;
		}

		if(gUploadResult)
		{
			// get a download url
			if(sakeGetFileDownloadURL(sake, gUploadedFileId, url))
				_tprintf(_T("Download URL: %s\n"), url);
			else
				printf("Failed to get download url!\n");

			// download a file
			request = ghttpSave(url, _T("downloaded.file"), GHTTPTrue, DownloadCompletedCallback, NULL);
			if(request == -1)
			{
				printf("Error starting file download\n");
			}
			else
			{
				NumOperations++;
			}
		}

		ghttpCleanup();
	}
#endif
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void LoginAndAuthenticate(SAKE sake)
{
	GPConnection connection;
	char loginTicket[GP_LOGIN_TICKET_LEN];

	pconn = &connection;

	// initialize GP
	printf("Initializing GP...\n");
	CHECK_GP_RESULT(gpInitialize(pconn, PRODUCTID, NAMESPACEID, GP_PARTNERID_GAMESPY), "gpInitialize failed");

	// setup callbacks
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_ERROR, (GPCallback)Error, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_REQUEST, (GPCallback)RecvBuddyRequest, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_STATUS, (GPCallback)RecvBuddyStatus, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_MESSAGE, (GPCallback)RecvBuddyMessage, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_GAME_INVITE,	(GPCallback)RecvGameInvite, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_TRANSFER_CALLBACK, (GPCallback)TransferCallback, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_AUTH, (GPCallback)RecvBuddyAuth, NULL), "gpSetCallback failed");
	CHECK_GP_RESULT(gpSetCallback(pconn, GP_RECV_BUDDY_REVOKE, (GPCallback)RecvBuddyRevoke, NULL), "gpSetCallback failed");

	// connect to GP
	printf("Connecting to GP...\n");
	CHECK_GP_RESULT(gpConnect(pconn, NICKNAME, EMAIL, PASSWORD, GP_NO_FIREWALL, GP_BLOCKING, (GPCallback)ConnectResponse, NULL), "gpConnect failed");
	
	// retrieve the login ticket
	CHECK_GP_RESULT(gpGetLoginTicket(pconn, loginTicket), "gpGetLoginTicket failed");
	
	// process
	CHECK_GP_RESULT(gpProcess(pconn), "gpProcess failed");


	// authenticate Sake with the login ticket and the profileid returned from login
	printf("Authenticating with Sake\n");
	sakeSetGame(sake, GAMENAME, GAMEID, SECRET_KEY);
	sakeSetProfile(sake, profileid, loginTicket);

	// Disconnect from GP
	gpDisconnect(pconn);
	printf("Disconnected from GP\n");

	// Destroy GP
	/////////
	gpDestroy(pconn);
	printf("Destroyed GP\n");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32) && !defined(_XBOX) && defined(_DEBUG)
#include <crtdbg.h>
#endif

int test_main(int argc, char* argv[]);  // CW needs this prototyped

int test_main(int argc, char* argv[])
{
	SAKEStartupResult startupResult;
	SAKE sake;

	GSI_UNUSED(argc);
	GSI_UNUSED(argv);

	// setup the common debugging
#ifdef GSI_COMMON_DEBUG
	gsSetDebugCallback(DebugCallback);
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Hardcore);
#endif

    // enable Win32 C Runtime debugging 
#if defined(_WIN32) && !defined(_XBOX) && defined(_DEBUG)
    {
        int tempFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        _CrtSetDbgFlag(tempFlag | _CRTDBG_LEAK_CHECK_DF);
    }
#endif

	// availability check
	printf("Checking availability\n");
	if(CheckServices() != GSIACAvailable)
		return 0;

	// Init the GameSpy SDK Core (required by SOAP SDKs)
	printf("Initializing the GameSpy Core\n");
	gsCoreInitialize();

	// startup sake
	printf("Starting up Sake\n");
	startupResult = sakeStartup(&sake);
	if(startupResult != SAKEStartupResult_SUCCESS)
	{
		printf("Startup failed: %d\n", startupResult);
		return 0;
	}

	// login to GP and use data to authenticate Sake
	LoginAndAuthenticate(sake);

	// run the tests
	RunTests(sake);

	// keep thinking
	printf("Entering think loop\n");
	while(NumOperations > 0)
	{
		msleep(5);
		gsCoreThink(0);
	}

	// shutdown sake
	printf("Shutting down Sake\n");
	sakeShutdown(sake);

	// shutdown the core
	printf("Shutting down the GameSpy Core\n");
	gsCoreShutdown();

    // Wait for core shutdown 
    //   (should be instantaneous unless you have multiple cores)
    while(gsCoreIsShutdown() == GSCore_SHUTDOWN_PENDING)
    {
        gsCoreThink(0);
        msleep(5);
    }

	return 0;
}
