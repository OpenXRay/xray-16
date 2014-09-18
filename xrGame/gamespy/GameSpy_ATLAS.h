#ifndef GAMESPY_ATLAS_INCLUDED
#define GAMESPY_ATLAS_INCLUDED

#include "GameSpy_FuncDefs.h"


class CGameSpy_ATLAS
{
public:
				CGameSpy_ATLAS			(HMODULE hGameSpyDLL);
				~CGameSpy_ATLAS			();

	void		Think					();
	u32			WSLoginProfile			(shared_str const & email,
										 shared_str const & nick,
										 shared_str const & password,
										 WSLoginCallback callback,
										 void * userData);

	SCResult	CreateSession			(const GSLoginCertificate * theCertificate,
										 const GSLoginPrivateData * thePrivateData,
										 SCCreateSessionCallback theCallback,
										 gsi_time theTimeoutMs,
										 void * theUserData);
	
	SCResult	SetReportIntention		(const gsi_u8 theConnectionId[SC_CONNECTION_GUID_SIZE],
										 gsi_bool isAuthoritative,
										 const GSLoginCertificate * theCertificate,
										 const GSLoginPrivateData * thePrivateData,
										 SCSetReportIntentionCallback theCallback,
										 gsi_time theTimeoutMs,
										 void * theUserData);
	char const*	GetConnectionId			();
	SCResult	SubmitReport			(const SCReportPtr theReport,
										 gsi_bool isAuthoritative,
										 const GSLoginCertificate * theCertificate,
										 const GSLoginPrivateData * thePrivateData,
										 SCSubmitReportCallback 	theCallback,
										 gsi_time 					theTimeoutMs,
										 void * 					theUserData);

	SCResult	CreateReport			(gsi_u32 		theHeaderVersion,
										 gsi_u32 		thePlayerCount,
										 gsi_u32 		theTeamCount,
										 SCReportPtr * 	theReportOut);
	SCResult	ReportBeginGlobalData	(SCReportPtr theReportData);
	SCResult	ReportBeginPlayerData	(SCReportPtr theReportData);
	SCResult	ReportBeginNewPlayer	(SCReportPtr theReportData);
	SCResult	ReportSetPlayerData		(SCReportPtr	theReport,
										 gsi_u32		thePlayerIndex,
										 const gsi_u8	thePlayerConnectionId[SC_CONNECTION_GUID_SIZE],
										 gsi_u32		thePlayerTeamIndex,
										 SCGameResult	theResult,
										 gsi_u32 		theProfileId,
										 const GSLoginCertificate * theCertificate);
	SCResult	ReportAddIntValue		(SCReportPtr theReportData,
										 gsi_u16 theKeyId,
										 gsi_i32 theValue);
	SCResult	ReportAddStringValue	(SCReportPtr theReportData,
										 gsi_u16 theKeyId,
										 const gsi_char * theValue);
	SCResult	ReportEnd				(SCReportPtr 	theReport,
										 gsi_bool		isAuth,
										 SCGameStatus 	theStatus);

	static shared_str const				TryToTranslate(GHTTPResult httpResult);
	static shared_str const				TryToTranslate(WSLoginValue loginValue);
	static shared_str const				TryToTranslate(SCResult result);
private:
	SCInterfacePtr					m_interface;

	void							Init							();
	void							LoadGameSpyATLAS				(HMODULE hGameSpyDLL);

	GAMESPY_FN_VAR_DECL(gsi_u32,			wsLoginProfile,			(const gsi_char * profileNick,
																	 const gsi_char * email,
																	 const gsi_char * password,
																	 const gsi_char * cdkeyhash,
																	 WSLoginCallback callback,
																	 void * userData));
	//export from sc
	GAMESPY_FN_VAR_DECL(SCResult,			scInitialize,			(SCInterfacePtr * theInterfaceOut));
	GAMESPY_FN_VAR_DECL(SCResult,			scShutdown,				(SCInterfacePtr theInterface));
	GAMESPY_FN_VAR_DECL(SCResult,			scThink,				(SCInterfacePtr theInterface));
	GAMESPY_FN_VAR_DECL(SCResult,			scCreateSession,		(SCInterfacePtr theInterface,
																	 const GSLoginCertificate * theCertificate,
																	 const GSLoginPrivateData * thePrivateData,
																	 SCCreateSessionCallback theCallback,
																	 gsi_time theTimeoutMs,
																	 void * theUserData));
	GAMESPY_FN_VAR_DECL(const char *,		scGetSessionId,			(const SCInterfacePtr theInterface));
	GAMESPY_FN_VAR_DECL(SCResult,			scSetSessionId,			(const SCInterfacePtr 	theInterface,
																	 const gsi_u8 		theSessionId[SC_SESSION_GUID_SIZE]));
	GAMESPY_FN_VAR_DECL(SCResult,			scSetReportIntention,	(const SCInterfacePtr theInterface,
																	 const gsi_u8 theConnectionId[SC_CONNECTION_GUID_SIZE],
																	 gsi_bool isAuthoritative,
																	 const GSLoginCertificate * theCertificate,
																	 const GSLoginPrivateData * thePrivateData,
																	 SCSetReportIntentionCallback theCallback,
																	 gsi_time theTimeoutMs,
																	 void * theUserData));
	GAMESPY_FN_VAR_DECL(const char *,		scGetConnectionId,		(const SCInterfacePtr theInterface));
	GAMESPY_FN_VAR_DECL(SCResult,			scCreateReport,			(const SCInterfacePtr 	theInterface,
																	 gsi_u32 		theHeaderVersion,
																	 gsi_u32 		thePlayerCount,
																	 gsi_u32 		theTeamCount,
																	 SCReportPtr * 	theReportOut));
	GAMESPY_FN_VAR_DECL(SCResult,			scReportBeginGlobalData,(SCReportPtr theReportData));
	GAMESPY_FN_VAR_DECL(SCResult,			scReportBeginPlayerData,(SCReportPtr theReportData));
	GAMESPY_FN_VAR_DECL(SCResult,			scReportBeginNewPlayer,	(SCReportPtr theReportData));
	GAMESPY_FN_VAR_DECL(SCResult,			scReportSetPlayerData,	(SCReportPtr	theReport,
																	 gsi_u32		thePlayerIndex,
																	 const gsi_u8	thePlayerConnectionId[SC_CONNECTION_GUID_SIZE],
																	 gsi_u32		thePlayerTeamIndex,
																	 SCGameResult	theResult,
																	 gsi_u32 		theProfileId, 
																	 const GSLoginCertificate * theCertificate,
																	 const gsi_u8 	theAuthData[16]));
	GAMESPY_FN_VAR_DECL(SCResult,			scReportAddIntValue,	(SCReportPtr theReportData,
																	 gsi_u16 theKeyId,
																	 gsi_i32 theValue));
	 
	GAMESPY_FN_VAR_DECL(SCResult,			scReportAddStringValue,	(SCReportPtr theReportData,
																	 gsi_u16 theKeyId,
																	 const gsi_char * theValue));
	GAMESPY_FN_VAR_DECL(SCResult,			scReportEnd,			(SCReportPtr 	theReport,
																	 gsi_bool		isAuth,
																	 SCGameStatus 	theStatus));


	GAMESPY_FN_VAR_DECL(SCResult,			scSubmitReport,			(const SCInterfacePtr theInterface,
																	 const SCReportPtr theReport,
																	 gsi_bool isAuthoritative,
																	 const GSLoginCertificate * theCertificate,
																	 const GSLoginPrivateData * thePrivateData,
																	 SCSubmitReportCallback 	theCallback,
																	 gsi_time 					theTimeoutMs,
																	 void * 					theUserData));
}; //class CGameSpy_ATLAS

#endif //#ifndef GAMESPY_ATLAS_INCLUDED