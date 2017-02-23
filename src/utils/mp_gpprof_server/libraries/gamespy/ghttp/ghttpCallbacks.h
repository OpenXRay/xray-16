/*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _GHTTPCALLBACKS_H_
#define _GHTTPCALLBACKS_H_

#include "ghttpMain.h"
#include "ghttpConnection.h"

#ifdef __cplusplus
extern "C" {
#endif

// Call the completed callback for this connection.
///////////////////////////////////////////////////
void ghiCallCompletedCallback
(
	GHIConnection * connection
);

// Call the progress callback for this connection.
//////////////////////////////////////////////////
void ghiCallProgressCallback
(
	GHIConnection * connection,
	const char * buffer,
	GHTTPByteCount bufferLen
);

// Call the post callback for this connection.
//////////////////////////////////////////////
void ghiCallPostCallback
(
	GHIConnection * connection
);

#ifdef __cplusplus
}
#endif

#endif
