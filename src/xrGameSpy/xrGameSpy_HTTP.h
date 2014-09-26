#pragma once
#include "xrGameSpy_MainDefs.h"
#include "gamespy/ghttp/ghttp.h"
extern "C"
{
	EXPORT_FN_DECL(void, ghttpStartup, ());
	EXPORT_FN_DECL(void, ghttpCleanup, ());
	EXPORT_FN_DECL(void, ghttpThink,());
	EXPORT_FN_DECL(void, ghttpCancelRequest, ( GHTTPRequest request )); 



	EXPORT_FN_DECL(GHTTPRequest, ghttpSaveA,
		(
		const gsi_char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
		const gsi_char * filename,  // The path and name to store the file as locally.
		GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
		ghttpCompletedCallback completedCallback,  // Called when the file has been received.
		void * param                // User-data to be passed to the callbacks.
		));

	EXPORT_FN_DECL(GHTTPRequest, ghttpSaveExA,
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
		));
}