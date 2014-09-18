#ifndef XRGAMESPY_SAKE
#define XRGAMESPY_SAKE

#include "xrGameSpy_MainDefs.h"
#include "GameSpy/sake/sake.h"

extern "C"
{

EXPORT_FN_DECL(SAKEStartupResult,		sakeStartup,				(SAKE *sakePtr));
EXPORT_FN_DECL(void,					sakeShutdown,				(SAKE sake));
EXPORT_FN_DECL(void,					sakeSetProfile,				(SAKE sake,
																	 int profileId,
																	 const char *loginTicket));
EXPORT_FN_DECL(SAKEStartRequestResult,	sakeGetStartRequestResult,	(SAKE sake));
EXPORT_FN_DECL(SAKERequest,				sakeGetMyRecords,			(SAKE sake,
																	 SAKEGetMyRecordsInput * input,
																	 SAKERequestCallback callback,
																	 void * userData));
EXPORT_FN_DECL(SAKERequest,				sakeCreateRecord,			(SAKE sake,
																	 SAKECreateRecordInput * input,
																	 SAKERequestCallback callback,
																	 void * userdata));
EXPORT_FN_DECL(SAKERequest,				sakeUpdateRecord,			(SAKE sake,
																	 SAKEUpdateRecordInput * input,
																	 SAKERequestCallback callback,
																	 void * userdata));

EXPORT_FN_DECL(SAKERequest,				sakeSearchForRecords,		(SAKE sake,
																	 SAKESearchForRecordsInput * input,
																	 SAKERequestCallback callback,
																	 void * userData));

} //extern "C"

#endif //#ifndef XRGAMESPY_SAKE