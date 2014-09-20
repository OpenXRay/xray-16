 /*
GameSpy GHTTP SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#ifndef _GHTTPPOST_H_
#define _GHTTPPOST_H_

#include "ghttp.h"
#include "ghttpBuffer.h"
#include "../darray.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	GHIPostingError,
	GHIPostingDone,
	GHIPostingPosting,
	GHIPostingWaitForContinue
} GHIPostingResult;

typedef struct GHIPostingState
{
	DArray states;
	int index;
	int bytesPosted;
	int totalBytes;
	ghttpPostCallback callback;
	void * param;
	GHTTPBool waitPostContinue; // does DIME need to wait for continue?
	GHTTPBool completed; // prevent re-post in the event of a redirect.
} GHIPostingState;

GHTTPPost ghiNewPost
(
	void
);

void ghiPostSetAutoFree
(
	GHTTPPost post,
	GHTTPBool autoFree
);

GHTTPBool ghiIsPostAutoFree
(
	GHTTPPost post
);

void ghiFreePost
(
	GHTTPPost post
);

GHTTPBool ghiPostAddString
(
	GHTTPPost post,
	const char * name,
	const char * string
);

GHTTPBool ghiPostAddFileFromDisk
(
	GHTTPPost post,
	const char * name,
	const char * filename,
	const char * reportFilename,
	const char * contentType
);

GHTTPBool ghiPostAddFileFromMemory
(
	GHTTPPost post,
	const char * name,
	const char * buffer,
	int bufferLen,
	const char * reportFilename,
	const char * contentType
);

GHTTPBool ghiPostAddXml
(
	GHTTPPost post,
	GSXmlStreamWriter xmlSoap
);

void ghiPostSetCallback
(
	GHTTPPost post,
	ghttpPostCallback callback,
	void * param
);

const char * ghiPostGetContentType
(
	struct GHIConnection * connection
);

GHTTPBool ghiPostInitState
(
	struct GHIConnection * connection
);

void ghiPostCleanupState
(
	struct GHIConnection * connection
);

GHIPostingResult ghiPostDoPosting
(
	struct GHIConnection * connection
);

#ifdef __cplusplus
}
#endif

#endif
