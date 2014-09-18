#include "stdafx.h"
#include "windows.h"
#include "xrGameSpy_MainDefs.h"

#include "xrGameSpy_HTTP.h"

XRGAMESPY_API void xrGS_ghttpStartup()
{
	ghttpStartup();
}

XRGAMESPY_API void xrGS_ghttpCleanup()
{
	ghttpCleanup();
}

XRGAMESPY_API void xrGS_ghttpThink()
{
	ghttpThink();
}

XRGAMESPY_API GHTTPRequest xrGS_ghttpSaveA		   (
			   const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
			   const gsi_char * filename,  // The path and name to store the file as locally.
			   GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
			   ghttpCompletedCallback completedCallback,  // Called when the file has been received.
			   void * param                // User-data to be passed to the callbacks.
			   )
{
	return ghttpSaveA( URL, filename, blocking, completedCallback, param);
}

XRGAMESPY_API GHTTPRequest xrGS_ghttpSaveExA	   (
			   const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
			   const gsi_char * filename,  // The path and name to store the file as locally.
			   const gsi_char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
			   GHTTPPost post,             // Optional data to be posted.
			   GHTTPBool throttle,         // If true, throttle this connection's download speed.
			   GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
			   ghttpProgressCallback progressCallback,    // Called periodically with progress updates.
			   ghttpCompletedCallback completedCallback,  // Called when the file has been received.
			   void * param                // User-data to be passed to the callbacks.
			   )
{
	return ghttpSaveExA		   ( URL, filename, headers, post, throttle, blocking, progressCallback, completedCallback, param);
}

XRGAMESPY_API void xrGS_ghttpCancelRequest ( GHTTPRequest request )
{
	ghttpCancelRequest ( request );
};