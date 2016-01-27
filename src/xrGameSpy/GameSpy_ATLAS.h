#ifndef GAMESPY_ATLAS_INCLUDED
#define GAMESPY_ATLAS_INCLUDED

#include "xrCore/xrCore.h"
#include "xrGameSpy/xrGameSpy.h"

class XRGAMESPY_API CGameSpy_ATLAS
{
public:
				CGameSpy_ATLAS			();
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
}; //class CGameSpy_ATLAS

#endif //#ifndef GAMESPY_ATLAS_INCLUDED
