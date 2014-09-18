 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _GHTTP_H_
#define _GHTTP_H_

#include <stdlib.h>

#include "../common/gsCommon.h"
#include "../common/gsXML.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSI_UNICODE
#define ghttpGet	ghttpGetA
#define ghttpGetEx	ghttpGetExA
#define ghttpSave	ghttpSaveA
#define ghttpSaveEx	ghttpSaveExA
#define ghttpStream	ghttpStreamA
#define ghttpStreamEx	ghttpStreamExA
#define ghttpHead	ghttpHeadA
#define ghttpHeadEx	ghttpHeadExA
#define ghttpPost	ghttpPostA
#define ghttpPostEx	ghttpPostExA
#define ghttpPostAddString	ghttpPostAddStringA
#define ghttpPostAddFileFromDisk	ghttpPostAddFileFromDiskA
#define ghttpPostAddFileFromMemory	ghttpPostAddFileFromMemoryA
#else
#define ghttpGet	ghttpGetW
#define ghttpGetEx	ghttpGetExW
#define ghttpSave	ghttpSaveW
#define ghttpSaveEx	ghttpSaveExW
#define ghttpStream	ghttpStreamW
#define ghttpStreamEx	ghttpStreamExW
#define ghttpHead	ghttpHeadW
#define ghttpHeadEx	ghttpHeadExW
#define ghttpPost	ghttpPostW
#define ghttpPostEx	ghttpPostExW
#define ghttpPostAddString	ghttpPostAddStringW
#define ghttpPostAddFileFromDisk	ghttpPostAddFileFromDiskW
#define ghttpPostAddFileFromMemory	ghttpPostAddFileFromMemoryW
#endif

// Boolean.
///////////
typedef enum
{
	GHTTPFalse,
	GHTTPTrue
} GHTTPBool;

// ByteCount.
/////////////
#if (GSI_MAX_INTEGRAL_BITS >= 64)
typedef gsi_i64  GHTTPByteCount;
#else
typedef gsi_i32  GHTTPByteCount;
#endif

// The current state of an http request.
////////////////////////////////////////
typedef enum
{
	GHTTPSocketInit,			// Socket creation and initialization.
	GHTTPHostLookup,            // Resolving hostname to IP (asynchronously if possible).
	GHTTPLookupPending,			// Asychronous DNS lookup pending.
	GHTTPConnecting,            // Waiting for socket connect to complete.
	GHTTPSecuringSession,		// Setup secure channel.
	GHTTPSendingRequest,        // Sending the request.
	GHTTPPosting,               // Positing data (skipped if not posting).
	GHTTPWaiting,               // Waiting for a response.
	GHTTPReceivingStatus,       // Receiving the response status.
	GHTTPReceivingHeaders,      // Receiving the headers.
	GHTTPReceivingFile          // Receiving the file.
} GHTTPState;

// The result of an http request.
/////////////////////////////////
typedef enum
{
	GHTTPSuccess,               // 0:  Successfully retrieved file.
	GHTTPOutOfMemory,           // 1:  A memory allocation failed.
	GHTTPBufferOverflow,        // 2:  The user-supplied buffer was too small to hold the file.
	GHTTPParseURLFailed,        // 3:  There was an error parsing the URL.
	GHTTPHostLookupFailed,      // 4:  Failed looking up the hostname.
	GHTTPSocketFailed,          // 5:  Failed to create/initialize/read/write a socket.
	GHTTPConnectFailed,         // 6:  Failed connecting to the http server.
	GHTTPBadResponse,           // 7:  Error understanding a response from the server.
	GHTTPRequestRejected,       // 8:  The request has been rejected by the server.
	GHTTPUnauthorized,          // 9:  Not authorized to get the file.
	GHTTPForbidden,             // 10: The server has refused to send the file.
	GHTTPFileNotFound,          // 11: Failed to find the file on the server.
	GHTTPServerError,           // 12: The server has encountered an internal error.
	GHTTPFileWriteFailed,       // 13: An error occured writing to the local file (for ghttpSaveFile[Ex]).
	GHTTPFileReadFailed,        // 14: There was an error reading from a local file (for posting files from disk).
	GHTTPFileIncomplete,        // 15: Download started but was interrupted.  Only reported if file size is known.
	GHTTPFileToBig,             // 16: The file is to big to be downloaded (size exceeds range of interal data types)
	GHTTPEncryptionError,       // 17: Error with encryption engine.
	GHTTPRequestCancelled       // 18: User requested cancel and/or graceful close.
} GHTTPResult;

// Encryption engines
typedef enum
{
	GHTTPEncryptionEngine_None,
	GHTTPEncryptionEngine_GameSpy,   // must add /common/gsSSL.h and /common/gsSSL.c to project
	GHTTPEncryptionEngine_MatrixSsl, // must define MATRIXSSL and include matrixssl source files
	GHTTPEncryptionEngine_RevoEx,    // must define REVOEXSSL and include RevoEX SSL source files
	
	GHTTPEncryptionEngine_Default    // Will use GameSpy unless another engine is defined
	                                 //   using MATRIXSSL or REVOEXSSL
} GHTTPEncryptionEngine;

// Represents an http file request.
///////////////////////////////////
typedef int GHTTPRequest;

// Invalid GHTTPRequest values represent an error
///////////////////////////////////
#ifdef GHTTP_EXTENDEDERROR
	typedef enum
	{
		GHTTPErrorStart				= -8,
		GHTTPFailedToOpenFile,
		GHTTPInvalidPost,
		GHTTPInsufficientMemory,
		GHTTPInvalidFileName,	
		GHTTPInvalidBufferSize,
		GHTTPInvalidURL,
		GHTTPUnspecifiedError		= -1
	} GHTTPRequestError;
#else
	// Backwards compatibility, developers may have relied on -1 as the only error code
	typedef enum
	{
		GHTTPErrorStart				= -1,
		GHTTPFailedToOpenFile		= -1,
		GHTTPInvalidPost			= -1,
		GHTTPInsufficientMemory		= -1,
		GHTTPInvalidFileName		= -1,	
		GHTTPInvalidBufferSize		= -1,
		GHTTPInvalidURL				= -1,
		GHTTPUnspecifiedError		= -1
	} GHTTPRequestError;
#endif

#define IS_GHTTP_ERROR(x) (x<0)

// Data that can be posted to the server.
// Don't try to access this object directly,
// use the ghttpPost*() functions.
////////////////////////////////////////////
typedef struct GHIPost * GHTTPPost;


// Called with updates on the current state of the request.
// The buffer should not be accessed once this callback returns.
// If ghttpGetFile[Ex] was used, buffer contains all of the data that has been 
//   received so far, and bufferSize is the total number of bytes received.
// If ghttpSaveFile[Ex] was used, buffer only contains the most recent data
//   that has been received.  This same data is saved to the file.  The buffer
//   will not be valid after this callback returns.
// If ghttpStreamFileEx was used, buffer only contains the most recent data
//   that has been received.  This data will be lost once the callback
//   returns, and should be copied if it needs to be saved.  bufferSize
//   is the number of bytes in the current block of data.
//////////////////////////////////////////////////////////////////////////////
typedef void (* ghttpProgressCallback)
(
	GHTTPRequest request,       // The request.
	GHTTPState state,           // The current state of the request.
	const char * buffer,        // The file's bytes so far, NULL if state<GHTTPReceivingFile.
	GHTTPByteCount bufferLen,   // The number of bytes in the buffer, 0 if state<GHTTPReceivingFile.
	GHTTPByteCount bytesReceived, // The total number of bytes receivied, 0 if state<GHTTPReceivingFile.
	GHTTPByteCount totalSize,   // The total size of the file, -1 if unknown.
	void * param                // User-data.
);

// Called when the entire file has been received.
// If ghttpStreamFileEx or ghttpSaveFile[Ex] was used,
//   buffer is NULL, bufferLen is the number of bytes
//   in the file, and the return value is ignored.
// If ghttpGetFile[Ex] was used, return true to have
//   the buffer freed, false if the app will free the
//   buffer.  If true, the buffer cannot be accessed
//   once the callback returns.  If false, the app can
//   use the buffer even after this call returns, but
//   must free it at some later point.  There will always
//   be a file, even if there was an error, although for
//   errors it may be an empty file.
////////////////////////////////////////////////////////
typedef GHTTPBool (* ghttpCompletedCallback)
(
	GHTTPRequest request,       // The request.
	GHTTPResult result,         // The result (success or an error).
	char * buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
	GHTTPByteCount bufferLen,   // The file's length.
	void * param                // User-data.
);

// Does all necessary initialization.
// Startup/Cleanup is reference counted, so always call
// ghttpStartup() and ghttpCleanup() in pairs.
///////////////////////////////////////////////////////
void ghttpStartup
(
	void
);

// Cleans up any resources being used by this library.
// Startup/Cleanup is reference counted, so always call
// ghttpStartup() and ghttpCleanup() in pairs.
//////////////////////////////////////////////////////
void ghttpCleanup
(
	void
);

// Get a file from an http server.
// Returns GHTTPRequestError if an error occurs.
//////////////////////////////////
GHTTPRequest ghttpGet
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Get a file from an http server.
// Returns GHTTPRequestError if an error occurs.
// Allows an optional user-supplied buffer to be used,
// optional extra http headers,
// and an optional progress callback.
// The optional headers must be 0 or more HTTP headers,
//   each terminated by a CR-LF pair (0xD, 0xA).
// If using a user-supplied buffer:
//   set buffer to the buffer to use,
//   set bufferSize to the size of the buffer in bytes.
// To have the library allocate a buffer:
//   set buffer to NULL, set bufferSize to 0
///////////////////////////////////////////////////////
GHTTPRequest ghttpGetEx
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const gsi_char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
	char * buffer,              // Optional user-supplied buffer.  Set to NULL to have one allocated.
	int bufferSize,             // The size of the user-supplied buffer in bytes.  0 if buffer is NULL.
	GHTTPPost post,             // Optional data to be posted.
	GHTTPBool throttle,         // If true, throttle this connection's download speed.
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpProgressCallback progressCallback,    // Called periodically with progress updates.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Gets a file and saves it to disk.
// Returns GHTTPRequestError if an error occurs.
////////////////////////////////////
GHTTPRequest ghttpSave
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const gsi_char * filename,  // The path and name to store the file as locally.
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Gets a file and saves it to disk.
// Returns GHTTPRequestError if an error occurs.
// Allows optional extra http headers and
// an optional progress callback.
/////////////////////////////////////////
GHTTPRequest ghttpSaveEx
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const gsi_char * filename,  // The path and name to store the file as locally.
	const gsi_char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
	GHTTPPost post,             // Optional data to be posted.
	GHTTPBool throttle,         // If true, throttle this connection's download speed.
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpProgressCallback progressCallback,    // Called periodically with progress updates.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Streams a file from an http server.
// Returns GHTTPRequestError if an error occurs.
//////////////////////////////////////
GHTTPRequest ghttpStream
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	GHTTPBool blocking,         // If true, this call doesn't return until the file has finished streaming.
	ghttpProgressCallback progressCallback,    // Called whenever new data is received.
	ghttpCompletedCallback completedCallback,  // Called when the file has finished streaming.
	void * param                // User-data to be passed to the callbacks.
);

// Streams a file from an http server.
// Returns GHTTPRequestError if an error occurs.
// Allows optional extra http headers.
//////////////////////////////////////
GHTTPRequest ghttpStreamEx
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const gsi_char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
	GHTTPPost post,             // Optional data to be posted.
	GHTTPBool throttle,         // If true, throttle this connection's download speed.
	GHTTPBool blocking,         // If true, this call doesn't return until the file has finished streaming.
	ghttpProgressCallback progressCallback,    // Called whenever new data is received.
	ghttpCompletedCallback completedCallback,  // Called when the file has finished streaming.
	void * param                // User-data to be passed to the callbacks.
);

// Does a file request without actually getting the file.
// Use this to check the headers returned by a server when a request is made.
// Returns GHTTPRequestError if an error occurs.
/////////////////////////////////////////////////////////////////////////////
GHTTPRequest ghttpHead
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	GHTTPBool blocking,         // If true, this call doesn't return until finished
	ghttpCompletedCallback completedCallback,  // Called when the request has finished.
	void * param                // User-data to be passed to the callbacks.
);

// Does a file request without actually getting the file.
// Use this to check the headers returned by a server when a request is made.
// Returns GHTTPRequestError if an error occurs.
// Allows optional extra http headers.
/////////////////////////////////////////////////////////////////////////////
GHTTPRequest ghttpHeadEx
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const gsi_char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
	GHTTPBool throttle,         // If true, throttle this connection's download speed.
	GHTTPBool blocking,         // If true, this call doesn't return until finished
	ghttpProgressCallback progressCallback,    // Called whenever new data is received.
	ghttpCompletedCallback completedCallback,  // Called when the request has finished.
	void * param                // User-data to be passed to the callbacks.
);

// Does an HTTP POST, which can be used to upload data to a web server.
// The post parameter must be a valid GHTTPPost, setup with the data to be uploaded.
// No data will be returned from this request.  If data is needed, use one of the
// ghttp*FileEx() functions, and pass in a GHTTPPost object.
// Returns GHTTPRequestError if an error occurs.
///////////////////////////////////////////////////////////////////////////////////
GHTTPRequest ghttpPost
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	GHTTPPost post,             // The data to be posted.
	GHTTPBool blocking,         // If true, this call doesn't return until finished
	ghttpCompletedCallback completedCallback,  // Called when the file has finished streaming.
	void * param                // User-data to be passed to the callbacks.
);

// Does an HTTP POST, which can be used to upload data to a web server.
// The post parameter must be a valid GHTTPPost, setup with the data to be uploaded.
// No data will be returned from this request.  If data is needed, use one of the
// ghttp*FileEx() functions, and pass in a GHTTPPost object.
// Returns GHTTPRequestError if an error occurs.
// Allows optional extra http headers and
// an optional progress callback.
///////////////////////////////////////////////////////////////////////////////////
GHTTPRequest ghttpPostEx
(
	const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const gsi_char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
	GHTTPPost post,             // The data to be posted.
	GHTTPBool throttle,         // If true, throttle this connection's download speed.
	GHTTPBool blocking,         // If true, this call doesn't return until finished
	ghttpProgressCallback progressCallback,    // Called whenever new data is received.
	ghttpCompletedCallback completedCallback,  // Called when the file has finished streaming.
	void * param                // User-data to be passed to the callbacks.
);

// Processes all current http requests.
///////////////////////////////////////
void ghttpThink
(
	void
);

// Process one particular http request.
///////////////////////////////////////
GHTTPBool ghttpRequestThink
(
	GHTTPRequest request
);

// Cancels the request.  Socket is closed.
///////////////////////
void ghttpCancelRequest
(
	GHTTPRequest request
);

// Closes a request gracefull using shutdown(s, SD_SEND)
//    PS2-Insock does not support partial shutdown.
void ghttpCloseRequest
(
	GHTTPRequest request
);


// Gets the current state of this request.
//////////////////////////////////////////
GHTTPState ghttpGetState
(
	GHTTPRequest request
);

// Gets the status code and status string for a request.
// A pointer to the status string is returned, or NULL on error.
// Only valid if the GHTTPState for this request
// is greater than GHTTPReceivingStatus.
////////////////////////////////////////////////////////////////
const char * ghttpGetResponseStatus
(
	GHTTPRequest request,       // The request to get the response state of.
	int * statusCode            // If not NULL, the status code is stored here.
);

// Gets headers returned by the http server.
// Only valid if the GHTTPState for this
// request is GHTTPReceivingFile.
////////////////////////////////////////////
const char * ghttpGetHeaders
(
	GHTTPRequest request
);

// Gets the URL for a given request.
////////////////////////////////////
const char * ghttpGetURL
(
	GHTTPRequest request
);

// Sets a proxy server address.  The address should be of the
// form "<server>[:port]".  If port is omitted, 80 will be used.
// If server is NULL or "", no proxy server will be used.
// This should not be called while there are any current requests.
//////////////////////////////////////////////////////////////////
GHTTPBool ghttpSetProxy
(
	const char * server
);

// Sets a proxy server for a specific request.  The address should be of the
// form "<server>[:port]".  If port is omitted, 80 will be used.
// If server is NULL or "", no proxy server will be used.
//////////////////////////////////////////////////////////////////
GHTTPBool ghttpSetRequestProxy
(
	GHTTPRequest request,
	const char * server
);

// Used to start/stop throttling an existing connection.
// This may not be as efficient as starting a request
// with the desired setting.
////////////////////////////////////////////////////////
void ghttpSetThrottle
(
	GHTTPRequest request,
	GHTTPBool throttle
);

// Used to adjust the throttle settings.
////////////////////////////////////////
void ghttpThrottleSettings
(
	int bufferSize,     // The number of bytes to get each receive.
	gsi_time timeDelay  // How often to receive data, in milliseconds.
);

// Used to throttle based on time, not on bandwidth
// Prevents recv-loop blocking on ultrafast connections without directly limiting transfer rate
////////////////////////////////////////
void ghttpSetMaxRecvTime
(
	GHTTPRequest request,
	gsi_time maxRecvTime
);

// Creates a new post object, which is used to represent data to send to
// the web server as part of a request.
// After getting the post object, use the ghttpPostAdd*() functions
// to add data to the object, and ghttPostSetCallback() to add a
// callback to monitor the progress of the data upload.
// By default post objects automatically free themselves after posting.
// To use the same post with more than one request, set auto-free to false,
// then use ghttpFreePost to free it _after_ every request its being used
// in is _completed_.
// Returns NULL on error.
///////////////////////////////////////////////////////////////////////////
GHTTPPost ghttpNewPost
(
	void
);

// Sets a post object's auto-free flag.
// By default post objects automatically free themselves after posting.
// To use the same post with more than one request, set auto-free to false,
// then use ghttpFreePost to free it _after_ every request its being used
// in is _completed_.
///////////////////////////////////////////////////////////////////////////
void ghttpPostSetAutoFree
(
	GHTTPPost post,
	GHTTPBool autoFree
);

// Frees a post object.
///////////////////////
void ghttpFreePost
(
	GHTTPPost post              // The post object to free.
);

// Adds a string to the post object.
////////////////////////////////////
GHTTPBool ghttpPostAddString
(
	GHTTPPost post,             // The post object to add to.
	const gsi_char * name,      // The name to attach to this string.
	const gsi_char * string     // The actual string.
);

// Adds a disk file to the post object.
// The reportFilename is what is reported to the server as the filename.
// If NULL or empty, the filename will be used (including any possible path).
// The contentType is the MIME type to report for this file.
// If NULL, "application/octet-stream" is used.
// The file isn't read from until the data is actually sent to the server.
// Returns false for any error.
/////////////////////////////////////////////////////////////////////////////
GHTTPBool ghttpPostAddFileFromDisk
(
	GHTTPPost post,                 // The post object to add to.
	const gsi_char * name,          // The name to attach to this file.
	const gsi_char * filename,      // The name (and possibly path) to the file to upload.
	const gsi_char * reportFilename,// The filename given to the web server.
	const gsi_char * contentType    // The MIME type for this file.
);

// Adds a file, in memory, to the post object.
// The reportFilename is what is reported to the server as the filename.
// Cannot be NULL or empty.
// The contentType is the MIME type to report for this file.
// If NULL, "application/octet-stream" is used.
// The data is NOT copied off in this call.  The data pointer is read from
// as the data is actually sent to the server.  The pointer must remain
// valid during requests.
// Returns false for any error.
//////////////////////////////////////////////////////////////////////////
GHTTPBool ghttpPostAddFileFromMemory
(
	GHTTPPost post,             // The post object to add to.
	const gsi_char * name,      // The name to attach to this string.
	const char * buffer,		// The data to send.
	int bufferLen,              // The number of bytes of data to send.
	const gsi_char * reportFilename,  // The filename given to the web server.
	const gsi_char * contentType    // The MIME type for this file.
);

// Adds an XML SOAP object to the post object.
// See ghttpNewSoap and other Soap related functions
// Content-Type = text/xml
// The most common use of this function is to add ghttpSoap data
GHTTPBool ghttpPostAddXml
(
	GHTTPPost post,
	GSXmlStreamWriter xmlSoap
);

// Called during requests to let the app know how much of the post
// data has been uploaded.
//////////////////////////////////////////////////////////////////
typedef void (* ghttpPostCallback)
(
	GHTTPRequest request,       // The request.
	int bytesPosted,            // The number of bytes of data posted so far.
	int totalBytes,             // The total number of bytes being posted.
	int objectsPosted,          // The total number of data objects uploaded so far.
	int totalObjects,           // The total number of data objects to upload.
	void * param                // User-data.
);

// Set the callback for a post object.
//////////////////////////////////////
void ghttpPostSetCallback
(
	GHTTPPost post,             // The post object to set the callback on.
	ghttpPostCallback callback, // The callback to call when using this post object.
	void * param                // User-data passed to the callback.
);

// Use ssl encryption engine
GHTTPBool ghttpSetRequestEncryptionEngine
(
	GHTTPRequest request,
	GHTTPEncryptionEngine engine
);


// These are defined for backwards compatibility with the "file" function names.
////////////////////////////////////////////////////////////////////////////////
#define ghttpGetFile(a, b, c, d)                      ghttpGet(a, b, c, d)
#define ghttpGetFileEx(a, b, c, d, e, f, g, h, i, j)  ghttpGetEx(a, b, c, d, e, f, g, h, i, j)
#define ghttpSaveFile(a, b, c, d, e)                  ghttpSave(a, b, c, d, e)
#define ghttpSaveFileEx(a, b, c, d, e, f, g, h, i)    ghttpSaveEx(a, b, c, d, e, f, g, h, i)
#define ghttpStreamFile(a, b, c, d, e)                ghttpStream(a, b, c, d, e)
#define ghttpStreamFileEx(a, b, c, d, e, f, g, h)     ghttpStreamEx(a, b, c, d, e, f, g, h)
#define ghttpHeadFile(a, b, c, d)                     ghttpHead(a, b, c, d)
#define ghttpHeadFileEx(a, b, c, d, e, f, g)          ghttpHeadEx(a, b, c, d, e, f, g)

// This ASCII version needs to be define even in UNICODE mode
GHTTPRequest ghttpGetA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	GHTTPBool blocking,     // If true, this call doesn't return until the file has been recevied.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param            // User-data to be passed to the callbacks.
);
#define ghttpGetFileA(a, b, c, d)                      ghttpGetA(a, b, c, d)


#ifdef __cplusplus
}
#endif

#endif
