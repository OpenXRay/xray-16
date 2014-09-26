 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

// ASCII PROTOTYPES FOR USE IN UNICODE MODE
// INCLUDED TO SILENCE CODEWARRIOR WARNINGS
#ifndef _GHTTPASCII_H_
#define _GHTTPASCII_H_

#include "../common/gsCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

// Get a file from an http server.
// Returns GHTTPRequestError if an error occurs.
//////////////////////////////////
GHTTPRequest ghttpGetA
(
	const char * URL,       	// The URL for the file ("http://host.domain[:port]/path/filename").
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
GHTTPRequest ghttpGetExA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
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
GHTTPRequest ghttpSaveA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * filename,  // The path and name to store the file as locally.
	GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
	ghttpCompletedCallback completedCallback,  // Called when the file has been received.
	void * param                // User-data to be passed to the callbacks.
);

// Gets a file and saves it to disk.
// Returns GHTTPRequestError if an error occurs.
// Allows optional extra http headers and
// an optional progress callback.
/////////////////////////////////////////
GHTTPRequest ghttpSaveExA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * filename,  // The path and name to store the file as locally.
	const char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
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
GHTTPRequest ghttpStreamA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	GHTTPBool blocking,         // If true, this call doesn't return until the file has finished streaming.
	ghttpProgressCallback progressCallback,    // Called whenever new data is received.
	ghttpCompletedCallback completedCallback,  // Called when the file has finished streaming.
	void * param                // User-data to be passed to the callbacks.
);

// Streams a file from an http server.
// Returns GHTTPRequestError if an error occurs.
// Allows optional extra http headers.
//////////////////////////////////////
GHTTPRequest ghttpStreamExA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
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
GHTTPRequest ghttpHeadA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	GHTTPBool blocking,         // If true, this call doesn't return until finished
	ghttpCompletedCallback completedCallback,  // Called when the request has finished.
	void * param                // User-data to be passed to the callbacks.
);

// Does a file request without actually getting the file.
// Use this to check the headers returned by a server when a request is made.
// Returns GHTTPRequestError if an error occurs.
// Allows optional extra http headers.
/////////////////////////////////////////////////////////////////////////////
GHTTPRequest ghttpHeadExA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
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
GHTTPRequest ghttpPostA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
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
GHTTPRequest ghttpPostExA
(
	const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
	const char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
	GHTTPPost post,             // The data to be posted.
	GHTTPBool throttle,         // If true, throttle this connection's download speed.
	GHTTPBool blocking,         // If true, this call doesn't return until finished
	ghttpProgressCallback progressCallback,    // Called whenever new data is received.
	ghttpCompletedCallback completedCallback,  // Called when the file has finished streaming.
	void * param                // User-data to be passed to the callbacks.
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
GHTTPBool ghttpSetProxyA
(
	const char * server
);

// Adds a string to the post object.
////////////////////////////////////
GHTTPBool ghttpPostAddStringA
(
	GHTTPPost post,             // The post object to add to.
	const char * name,      // The name to attach to this string.
	const char * string     // The actual string.
);

// Adds a disk file to the post object.
// The reportFilename is what is reported to the server as the filename.
// If NULL or empty, the filename will be used (including any possible path).
// The contentType is the MIME type to report for this file.
// If NULL, "application/octet-stream" is used.
// The file isn't read from until the data is actually sent to the server.
// Returns false for any error.
/////////////////////////////////////////////////////////////////////////////
GHTTPBool ghttpPostAddFileFromDiskA
(
	GHTTPPost post,                 // The post object to add to.
	const char * name,          // The name to attach to this file.
	const char * filename,      // The name (and possibly path) to the file to upload.
	const char * reportFilename,// The filename given to the web server.
	const char * contentType    // The MIME type for this file.
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
GHTTPBool ghttpPostAddFileFromMemoryA
(
	GHTTPPost post,             // The post object to add to.
	const char * name,      // The name to attach to this string.
	const char * buffer,		// The data to send.
	int bufferLen,              // The number of bytes of data to send.
	const char * reportFilename,  // The filename given to the web server.
	const char * contentType    // The MIME type for this file.
);


#ifdef __cplusplus
}
#endif

#endif
