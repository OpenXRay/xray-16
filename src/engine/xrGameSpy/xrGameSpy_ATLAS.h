#ifndef XRGAMESPY_ATLAS_INCLUDED
#define XRGAMESPY_ATLAS_INCLUDED


#include "xrGameSpy_MainDefs.h"
#include "GameSpy/sc/sc.h"

extern "C"
{

//export from webservices
EXPORT_FN_DECL(gsi_u32,					wsLoginProfile,			(const gsi_char * profileNick,
																 const gsi_char * email,
																 const gsi_char * password,
																 const gsi_char * cdkeyhash,
																 WSLoginCallback callback,
																 void * userData));
//export from sc
EXPORT_FN_DECL(SCResult,				scInitialize,			(SCInterfacePtr * theInterfaceOut));
EXPORT_FN_DECL(SCResult,				scShutdown,				(SCInterfacePtr theInterface));
EXPORT_FN_DECL(SCResult,				scThink,				(SCInterfacePtr theInterface));
EXPORT_FN_DECL(SCResult,				scCreateSession,		(SCInterfacePtr theInterface,
																 const GSLoginCertificate * theCertificate,
																 const GSLoginPrivateData * thePrivateData,
																 SCCreateSessionCallback theCallback,
																 gsi_time theTimeoutMs,
																 void * theUserData));
EXPORT_FN_DECL(const char *,			scGetSessionId,			(const SCInterfacePtr theInterface));
EXPORT_FN_DECL(SCResult,				scSetSessionId,			(const SCInterfacePtr 	theInterface,
																 const gsi_u8 		theSessionId[SC_SESSION_GUID_SIZE]));
EXPORT_FN_DECL(SCResult,				scSetReportIntention,	(const SCInterfacePtr theInterface,
																 const gsi_u8 theConnectionId[SC_CONNECTION_GUID_SIZE],
																 gsi_bool isAuthoritative,
																 const GSLoginCertificate * theCertificate,
																 const GSLoginPrivateData * thePrivateData,
																 SCSetReportIntentionCallback theCallback,
																 gsi_time theTimeoutMs,
																 void * theUserData));
EXPORT_FN_DECL(const char *,			scGetConnectionId,		(const SCInterfacePtr theInterface));
EXPORT_FN_DECL(SCResult,				scCreateReport,			(const SCInterfacePtr 	theInterface,
																 gsi_u32 		theHeaderVersion,
																 gsi_u32 		thePlayerCount,
																 gsi_u32 		theTeamCount,
																 SCReportPtr * 	theReportOut));
EXPORT_FN_DECL(SCResult,				scReportBeginGlobalData,(SCReportPtr theReportData));
EXPORT_FN_DECL(SCResult,				scReportBeginPlayerData,(SCReportPtr theReportData));
EXPORT_FN_DECL(SCResult,				scReportBeginNewPlayer,	(SCReportPtr theReportData));


EXPORT_FN_DECL(SCResult,				scReportSetPlayerData,	(SCReportPtr theReport,
																 gsi_u32 thePlayerIndex,
																 const gsi_u8  thePlayerConnectionId[SC_CONNECTION_GUID_SIZE],
																 gsi_u32 thePlayerTeamId,
																 SCGameResult theResult,
																 gsi_u32 theProfileId,
																 const GSLoginCertificate * theCertificate,
																 const gsi_u8  theAuthHash[16]));

EXPORT_FN_DECL(SCResult,				scReportAddIntValue,	(SCReportPtr theReportData,
																 gsi_u16 theKeyId,
																 gsi_i32 theValue));
 
EXPORT_FN_DECL(SCResult,				scReportAddStringValue,	(SCReportPtr theReportData,
																 gsi_u16 theKeyId,
																 const gsi_char * theValue));
EXPORT_FN_DECL(SCResult,				scReportEnd,			(SCReportPtr 	theReport,
																 gsi_bool		isAuth,
																 SCGameStatus 	theStatus));


EXPORT_FN_DECL(SCResult,				scSubmitReport,			(const SCInterfacePtr theInterface,
																 const SCReportPtr theReport,
																 gsi_bool isAuthoritative,
																 const GSLoginCertificate * theCertificate,
																 const GSLoginPrivateData * thePrivateData,
																 SCSubmitReportCallback 	theCallback,
																 gsi_time 					theTimeoutMs,
																 void * 					theUserData));
}//extern "C"

#endif //#ifndef XRGAMESPY_ATLAS_INCLUDED