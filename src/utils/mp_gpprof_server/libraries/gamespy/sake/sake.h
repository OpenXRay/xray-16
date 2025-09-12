///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __SAKE_H__
#define __SAKE_H__


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../common/gsCommon.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
extern "C" {
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef SAKE_CALL
	#define SAKE_CALL
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// URL for sake webservice
#define SAKE_MAX_URL_LENGTH 128
extern char sakeiSoapUrl[SAKE_MAX_URL_LENGTH];

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// General
typedef struct SAKEInternal *SAKE;

typedef enum
{
	SAKEStartupResult_SUCCESS,
	SAKEStartupResult_NOT_AVAILABLE,
	SAKEStartupResult_CORE_SHUTDOWN,
	SAKEStartupResult_OUT_OF_MEMORY
} SAKEStartupResult;

SAKEStartupResult SAKE_CALL sakeStartup(SAKE *sakePtr);
void              SAKE_CALL sakeShutdown(SAKE sake);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Authentication
void SAKE_CALL sakeSetGame(SAKE sake, const gsi_char *gameName, int gameId, const gsi_char *secretKey);
void SAKE_CALL sakeSetProfile(SAKE sake, int profileId, const char *loginTicket);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Fields
typedef enum
{
	SAKEFieldType_BYTE,
	SAKEFieldType_SHORT,
	SAKEFieldType_INT,
	SAKEFieldType_FLOAT,
	SAKEFieldType_ASCII_STRING,
	SAKEFieldType_UNICODE_STRING,
	SAKEFieldType_BOOLEAN,
	SAKEFieldType_DATE_AND_TIME,
	SAKEFieldType_BINARY_DATA,
	SAKEFieldType_INT64,
	SAKEFieldType_NUM_FIELD_TYPES
} SAKEFieldType;

typedef struct
{
	gsi_u8 *mValue;
	int     mLength;
} SAKEBinaryData;

typedef union
{
	gsi_u8          mByte;
	gsi_i16         mShort;
	gsi_i32         mInt;
	float           mFloat;
	char           *mAsciiString;
	gsi_u16        *mUnicodeString;
	gsi_bool        mBoolean;
	time_t          mDateAndTime;
	SAKEBinaryData  mBinaryData;
	gsi_i64         mInt64;
} SAKEValue;

typedef struct
{
	char         *mName;
	SAKEFieldType mType;
	SAKEValue     mValue;
} SAKEField;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Requests
typedef struct SAKERequestInternal *SAKERequest;

typedef enum
{
	SAKEStartRequestResult_SUCCESS,
	SAKEStartRequestResult_NOT_AUTHENTICATED,
	SAKEStartRequestResult_OUT_OF_MEMORY,
	SAKEStartRequestResult_BAD_INPUT,
	SAKEStartRequestResult_BAD_TABLEID,
	SAKEStartRequestResult_BAD_FIELDS,
	SAKEStartRequestResult_BAD_NUM_FIELDS,
	SAKEStartRequestResult_BAD_FIELD_NAME,
	SAKEStartRequestResult_BAD_FIELD_TYPE,
	SAKEStartRequestResult_BAD_FIELD_VALUE,
	SAKEStartRequestResult_BAD_OFFSET,
	SAKEStartRequestResult_BAD_MAX,
	SAKEStartRequestResult_BAD_RECORDIDS,
	SAKEStartRequestResult_BAD_NUM_RECORDIDS,
	SAKEStartRequestResult_UNKNOWN_ERROR
} SAKEStartRequestResult;

typedef enum
{
	SAKERequestResult_SUCCESS,
	SAKERequestResult_SECRET_KEY_INVALID,
	SAKERequestResult_SERVICE_DISABLED,
	SAKERequestResult_CONNECTION_TIMEOUT,
	SAKERequestResult_CONNECTION_ERROR,
	SAKERequestResult_MALFORMED_RESPONSE,
	SAKERequestResult_OUT_OF_MEMORY,
	SAKERequestResult_DATABASE_UNAVAILABLE,
	SAKERequestResult_LOGIN_TICKET_INVALID,
	SAKERequestResult_LOGIN_TICKET_EXPIRED,
	SAKERequestResult_TABLE_NOT_FOUND,
	SAKERequestResult_RECORD_NOT_FOUND,
	SAKERequestResult_FIELD_NOT_FOUND,
	SAKERequestResult_FIELD_TYPE_INVALID,
	SAKERequestResult_NO_PERMISSION,
	SAKERequestResult_RECORD_LIMIT_REACHED,
	SAKERequestResult_ALREADY_RATED,
	SAKERequestResult_NOT_RATEABLE,
	SAKERequestResult_NOT_OWNED,
	SAKERequestResult_FILTER_INVALID,
	SAKERequestResult_SORT_INVALID,
	SAKERequestResult_TARGET_FILTER_INVALID,
	SAKERequestResult_UNKNOWN_ERROR
} SAKERequestResult;

typedef void (*SAKERequestCallback)(SAKE sake, SAKERequest request, SAKERequestResult result, void *inputData, void *outputData, void *userData);

///////////////////////////////////////////////////////////////////////////////
// get start request result
SAKEStartRequestResult SAKE_CALL sakeGetStartRequestResult(SAKE sake);

///////////////////////////////////////////////////////////////////////////////
// create record
typedef struct
{
	char      *mTableId;
	SAKEField *mFields;
	int        mNumFields;
} SAKECreateRecordInput;
typedef struct
{
	int mRecordId;
} SAKECreateRecordOutput;
SAKERequest SAKE_CALL sakeCreateRecord(SAKE sake, SAKECreateRecordInput *input, SAKERequestCallback callback, void *userData);

//////////////////////////// ///////////////////////////////////////////////////
// update record
typedef struct
{
	char      *mTableId;
	int        mRecordId;
	SAKEField *mFields;
	int        mNumFields;
} SAKEUpdateRecordInput;
SAKERequest SAKE_CALL sakeUpdateRecord(SAKE sake, SAKEUpdateRecordInput *input, SAKERequestCallback callback, void *userData);

///////////////////////////////////////////////////////////////////////////////
// delete record
typedef struct
{
	char *mTableId;
	int   mRecordId;
} SAKEDeleteRecordInput;
SAKERequest SAKE_CALL sakeDeleteRecord(SAKE sake, SAKEDeleteRecordInput *input, SAKERequestCallback callback, void *userData);

///////////////////////////////////////////////////////////////////////////////
// search for records
typedef struct
{
	char      *mTableId;
	char     **mFieldNames;
	int        mNumFields;
	gsi_char  *mFilter;
	char      *mSort;
	int        mOffset;
	int        mMaxRecords;
	gsi_char  *mTargetRecordFilter;
	int        mSurroundingRecordsCount;
	int       *mOwnerIds;
	int		   mNumOwnerIds;
	gsi_bool   mCacheFlag;
} SAKESearchForRecordsInput;
typedef struct
{
	int         mNumRecords;
	SAKEField **mRecords;
} SAKESearchForRecordsOutput;
SAKERequest SAKE_CALL sakeSearchForRecords(SAKE sake, SAKESearchForRecordsInput *input, SAKERequestCallback callback, void *userData);

///////////////////////////////////////////////////////////////////////////////
// get my records
typedef struct
{
	char      *mTableId;
	char     **mFieldNames;
	int        mNumFields;
} SAKEGetMyRecordsInput;
typedef struct
{
	int         mNumRecords;
	SAKEField **mRecords;
} SAKEGetMyRecordsOutput;
SAKERequest SAKE_CALL sakeGetMyRecords(SAKE sake, SAKEGetMyRecordsInput *input, SAKERequestCallback callback, void *userData);

///////////////////////////////////////////////////////////////////////////////
// get specific records
typedef struct
{
	char      *mTableId;
	int       *mRecordIds;
	int        mNumRecordIds;
	char     **mFieldNames;
	int        mNumFields;
} SAKEGetSpecificRecordsInput;
typedef struct
{
	int         mNumRecords;
	SAKEField **mRecords;
} SAKEGetSpecificRecordsOutput;
SAKERequest SAKE_CALL sakeGetSpecificRecords(SAKE sake, SAKEGetSpecificRecordsInput *input, SAKERequestCallback callback, void *userData);

///////////////////////////////////////////////////////////////////////////////
// get random record
typedef struct
{
	char      *mTableId;
	char     **mFieldNames;
	int        mNumFields;
	gsi_char  *mFilter;
} SAKEGetRandomRecordInput;
typedef struct
{
	SAKEField *mRecord;
} SAKEGetRandomRecordOutput;
SAKERequest SAKE_CALL sakeGetRandomRecord(SAKE sake, SAKEGetRandomRecordInput *input, SAKERequestCallback callback, void *userData);

///////////////////////////////////////////////////////////////////////////////
// rate record
typedef struct
{
	char  *mTableId;
	int    mRecordId;
	gsi_u8 mRating;
} SAKERateRecordInput;
typedef struct
{
	int mNumRatings;
	float mAverageRating;
} SAKERateRecordOutput;
SAKERequest SAKE_CALL sakeRateRecord(SAKE sake, SAKERateRecordInput *input, SAKERequestCallback callback, void *userData);

///////////////////////////////////////////////////////////////////////////////
// get record limit
typedef struct
{
	char *mTableId;
} SAKEGetRecordLimitInput;
typedef struct
{
	int mLimitPerOwner;
	int mNumOwned;
} SAKEGetRecordLimitOutput;
SAKERequest SAKE_CALL sakeGetRecordLimit(SAKE sake, SAKEGetRecordLimitInput *input, SAKERequestCallback callback, void *userData);

///////////////////////////////////////////////////////////////////////////////
// get record count
typedef struct
{
	char	  *mTableId;
	gsi_char  *mFilter;
	gsi_bool   mCacheFlag;
} SAKEGetRecordCountInput;
typedef struct
{
	int	       mCount;
} SAKEGetRecordCountOutput;
SAKERequest SAKE_CALL sakeGetRecordCount(SAKE sake, SAKEGetRecordCountInput *input, SAKERequestCallback callback, void *userData);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// read request utility
SAKEField * SAKE_CALL sakeGetFieldByName(const char *name, SAKEField *fields, int numFields);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Files

#define SAKE_FILE_RESULT_HEADER "Sake-File-Result:"
#define SAKE_FILE_ID_HEADER "Sake-File-Id:"

// Sake-File-Result from the HTTP response header
typedef enum
{
	SAKEFileResult_SUCCESS           = 0,
	SAKEFileResult_BAD_HTTP_METHOD   = 1,
	SAKEFileResult_BAD_FILE_COUNT    = 2,
	SAKEFileResult_MISSING_PARAMETER = 3,
	SAKEFileResult_FILE_NOT_FOUND    = 4,
	SAKEFileResult_FILE_TOO_LARGE    = 5,
	SAKEFileResult_SERVER_ERROR      = 6,
	SAKEFileResult_UNKNOWN_ERROR
} SAKEFileResult;

gsi_bool SAKE_CALL sakeSetFileDownloadURL(SAKE sake, gsi_char url[SAKE_MAX_URL_LENGTH]);
gsi_bool SAKE_CALL sakeSetFileUploadURL(SAKE sake, gsi_char url[SAKE_MAX_URL_LENGTH]);

gsi_bool SAKE_CALL sakeGetFileDownloadURL(SAKE sake, int fileId, gsi_char url[SAKE_MAX_URL_LENGTH]);
gsi_bool SAKE_CALL sakeGetFileUploadURL(SAKE sake, gsi_char url[SAKE_MAX_URL_LENGTH]);

gsi_bool SAKE_CALL sakeGetFileResultFromHeaders(const char *headers, SAKEFileResult *result);
gsi_bool SAKE_CALL sakeGetFileIdFromHeaders(const char *headers, int *fileId);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
} // extern "C"
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // __SAKE_H__
