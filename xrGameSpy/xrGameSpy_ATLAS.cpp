#include "stdafx.h"
#include "xrGameSpy_MainDefs.h"
#include "xrGameSpy_ATLAS.h"
#include "GameSpy/GP/gp.h"

XRGAMESPY_API gsi_u32 xrGS_wsLoginProfile(const gsi_char * profileNick,
										  const gsi_char * email,
										  const gsi_char * password,
										  const gsi_char * cdkeyhash,
										  WSLoginCallback callback,
										  void * userData)
{
	return wsLoginProfile(
		GP_PARTNERID_GAMESPY,
		GAMESPY_GP_NAMESPACE_ID,
		profileNick,
		email,
		password,
		cdkeyhash,
		callback,
		userData
	);
}

XRGAMESPY_API SCResult xrGS_scInitialize(SCInterfacePtr * theInterfaceOut)
{
	return scInitialize(GAMESPY_GAMEID, theInterfaceOut);
}

XRGAMESPY_API SCResult xrGS_scShutdown(SCInterfacePtr theInterface)
{
	return scShutdown(theInterface);
}

XRGAMESPY_API SCResult xrGS_scThink(SCInterfacePtr theInterface)
{
	return scThink(theInterface);
}

XRGAMESPY_API SCResult xrGS_scCreateSession(SCInterfacePtr theInterface,
											const GSLoginCertificate * theCertificate,
											const GSLoginPrivateData * thePrivateData,
											SCCreateSessionCallback theCallback,
											gsi_time theTimeoutMs,
											void * theUserData)
{
	return scCreateSession(
		theInterface,
		theCertificate,
		thePrivateData,
		theCallback,
		theTimeoutMs,
		theUserData
	);
}

XRGAMESPY_API const char * xrGS_scGetSessionId(const SCInterfacePtr theInterface)
{
	return scGetSessionId(theInterface);
}

XRGAMESPY_API SCResult xrGS_scSetSessionId(const SCInterfacePtr theInterface,
										   const gsi_u8 theSessionId[SC_SESSION_GUID_SIZE])
{
	return scSetSessionId(
		theInterface,
		theSessionId
	);
}

XRGAMESPY_API SCResult xrGS_scSetReportIntention(const SCInterfacePtr theInterface,
												 const gsi_u8 theConnectionId[SC_CONNECTION_GUID_SIZE],
												 gsi_bool isAuthoritative,
												 const GSLoginCertificate * theCertificate,
												 const GSLoginPrivateData * thePrivateData,
												 SCSetReportIntentionCallback theCallback,
												 gsi_time theTimeoutMs,
												 void * theUserData)
{
	return scSetReportIntention(
		theInterface,
		theConnectionId,
		isAuthoritative,
		theCertificate,
		thePrivateData,
		theCallback,
		theTimeoutMs,
		theUserData
	);
}

XRGAMESPY_API const char * xrGS_scGetConnectionId(const SCInterfacePtr theInterface)
{
	return scGetConnectionId(theInterface);
}

XRGAMESPY_API SCResult xrGS_scCreateReport(const SCInterfacePtr theInterface,
										   gsi_u32 		theHeaderVersion,
										   gsi_u32 		thePlayerCount,
										   gsi_u32 		theTeamCount,
										   SCReportPtr * 	theReportOut)
{
	return scCreateReport(
		theInterface,
		theHeaderVersion,
		thePlayerCount,
		theTeamCount,
		theReportOut
	);
}

XRGAMESPY_API SCResult xrGS_scReportBeginGlobalData(SCReportPtr theReportData)
{
	return scReportBeginGlobalData(theReportData);
}

XRGAMESPY_API SCResult xrGS_scReportBeginPlayerData(SCReportPtr theReportData)
{
	return scReportBeginPlayerData(theReportData);
}

XRGAMESPY_API SCResult xrGS_scReportBeginNewPlayer(SCReportPtr theReportData)
{
	return scReportBeginNewPlayer(theReportData);
}

XRGAMESPY_API SCResult xrGS_scReportSetPlayerData  (SCReportPtr theReport,
													gsi_u32 thePlayerIndex,
													const gsi_u8  thePlayerConnectionId[SC_CONNECTION_GUID_SIZE],
													gsi_u32 thePlayerTeamId,
													SCGameResult theResult,
													gsi_u32 theProfileId,
													const GSLoginCertificate * theCertificate,
													const gsi_u8  theAuthHash[16])
{
	gsi_u8	tmp_auth_data[16];
	ZeroMemory(tmp_auth_data, sizeof(tmp_auth_data));
	return scReportSetPlayerData(
		theReport,
		thePlayerIndex,
		thePlayerConnectionId,
		thePlayerTeamId,
		theResult,
		theProfileId,
		theCertificate,
		tmp_auth_data //not used
	);
}

XRGAMESPY_API SCResult xrGS_scReportAddIntValue(SCReportPtr theReportData,
												gsi_u16 theKeyId,
												gsi_i32 theValue)
{
	return scReportAddIntValue(
		theReportData,
		theKeyId,
		theValue
	);
}
 
XRGAMESPY_API SCResult xrGS_scReportAddStringValue(SCReportPtr theReportData,
												   gsi_u16 theKeyId,
												   const gsi_char * theValue)
{
	return scReportAddStringValue(
		theReportData,
		theKeyId,
		theValue
	);
}

XRGAMESPY_API SCResult xrGS_scReportEnd(SCReportPtr 	theReport,
										gsi_bool		isAuth,
										SCGameStatus 	theStatus)
{
	return scReportEnd(
		theReport,
		isAuth,
		theStatus
	);
}

XRGAMESPY_API SCResult xrGS_scSubmitReport(const SCInterfacePtr theInterface,
										   const SCReportPtr theReport,
										   gsi_bool isAuthoritative,
										   const GSLoginCertificate *	theCertificate,
										   const GSLoginPrivateData *	thePrivateData,
										   SCSubmitReportCallback 		theCallback,
										   gsi_time 					theTimeoutMs,
										   void * 						theUserData)
{
	return scSubmitReport(
		theInterface,
		theReport,
		isAuthoritative,
		theCertificate,
		thePrivateData,
		theCallback,
		theTimeoutMs,
		theUserData
	);
}