#include "stdafx.h"
#include "GameSpy_ATLAS.h"

CGameSpy_ATLAS::CGameSpy_ATLAS	()
{
	m_interface			= NULL;
	Init				();
}

CGameSpy_ATLAS::~CGameSpy_ATLAS	()
{
	if (m_interface)
	{
		scShutdown(m_interface);
	}
}

void CGameSpy_ATLAS::Init()
{
	SCResult init_res = scInitialize(GAMESPY_GAMEID, &m_interface);
	VERIFY(init_res == SCResult_NO_ERROR);
	if (init_res != SCResult_NO_ERROR)
	{
		Msg("! GameSpy ATLAS: failed to initialize, error code: %d", init_res);
	}
}

void CGameSpy_ATLAS::Think()
{
	scThink(m_interface);
}

shared_str const CGameSpy_ATLAS::TryToTranslate(GHTTPResult httpResult)
{
	return "mp_gamespy_http_error";
}

shared_str const CGameSpy_ATLAS::TryToTranslate(WSLoginValue loginValue)
{
	return "mp_gamespy_ws_login_error";
}

shared_str const CGameSpy_ATLAS::TryToTranslate(SCResult result)
{
	return "mp_gamespy_atlas_error";
}

u32 CGameSpy_ATLAS::WSLoginProfile(shared_str const & email,
									shared_str const & nick,
									shared_str const & password,
									WSLoginCallback callback,
									void * userData)
{
	return static_cast<u32>(wsLoginProfile(GP_PARTNERID_GAMESPY, GAMESPY_GP_NAMESPACE_ID,
			nick.c_str(), email.c_str(), password.c_str(), NULL, callback, userData));
}

SCResult CGameSpy_ATLAS::CreateSession(const GSLoginCertificate * theCertificate,
									   const GSLoginPrivateData * thePrivateData,
									   SCCreateSessionCallback theCallback,
									   gsi_time theTimeoutMs,
									   void * theUserData)
{
	return scCreateSession(
		m_interface,
		theCertificate,
		thePrivateData,
		theCallback,
		theTimeoutMs,
		theUserData
	);
}

SCResult CGameSpy_ATLAS::SetReportIntention(const gsi_u8 theConnectionId[SC_CONNECTION_GUID_SIZE],
											gsi_bool isAuthoritative,
											const GSLoginCertificate * theCertificate,
											const GSLoginPrivateData * thePrivateData,
											SCSetReportIntentionCallback theCallback,
											gsi_time theTimeoutMs,
											void * theUserData)
{
	return scSetReportIntention(
		m_interface,
		theConnectionId,
		isAuthoritative,
		theCertificate,
		thePrivateData,
		theCallback,
		theTimeoutMs,
		theUserData
	);
}

char const*	CGameSpy_ATLAS::GetConnectionId()
{
	return scGetConnectionId(m_interface);
}

SCResult CGameSpy_ATLAS::SubmitReport(const SCReportPtr theReport,
									  gsi_bool isAuthoritative,
									  const GSLoginCertificate * theCertificate,
									  const GSLoginPrivateData * thePrivateData,
									  SCSubmitReportCallback 	theCallback,
									  gsi_time 					theTimeoutMs,
									  void * 					theUserData)
{
	return scSubmitReport(
		m_interface,
		theReport,
		isAuthoritative,
		theCertificate,
		thePrivateData,
		theCallback,
		theTimeoutMs,
		theUserData
	);
}

SCResult CGameSpy_ATLAS::CreateReport(gsi_u32 		theHeaderVersion,
									  gsi_u32 		thePlayerCount,
									  gsi_u32 		theTeamCount,
									  SCReportPtr * 	theReportOut)
{
	return scCreateReport(
		m_interface,
		theHeaderVersion,
		thePlayerCount,
		theTeamCount,
		theReportOut
	);
}
SCResult CGameSpy_ATLAS::ReportBeginGlobalData(SCReportPtr theReportData)
{
	return scReportBeginGlobalData(theReportData);
}

SCResult CGameSpy_ATLAS::ReportBeginPlayerData(SCReportPtr theReportData)
{
	return scReportBeginPlayerData(theReportData);
}

SCResult CGameSpy_ATLAS::ReportBeginNewPlayer(SCReportPtr theReportData)
{
	return scReportBeginNewPlayer(theReportData);
}

SCResult CGameSpy_ATLAS::ReportSetPlayerData(SCReportPtr	theReport,
											 gsi_u32		thePlayerIndex,
											 const gsi_u8	thePlayerConnectionId[SC_CONNECTION_GUID_SIZE],
											 gsi_u32		thePlayerTeamIndex,
											 SCGameResult	theResult,
											 gsi_u32 		theProfileId,
											 const GSLoginCertificate * theCertificate)
{
	gsi_u8 	zero_auth_data[16];
	ZeroMemory(zero_auth_data, sizeof(zero_auth_data));
	return scReportSetPlayerData(
		theReport,
		thePlayerIndex,
		thePlayerConnectionId,
		thePlayerTeamIndex,
		theResult,
		theProfileId,
		theCertificate,
		zero_auth_data
	);
}

SCResult CGameSpy_ATLAS::ReportAddIntValue(SCReportPtr theReportData,
										   gsi_u16 theKeyId,
										   gsi_i32 theValue)
{
	return scReportAddIntValue(theReportData, theKeyId, theValue);
}

SCResult CGameSpy_ATLAS::ReportAddStringValue(SCReportPtr theReportData,
											  gsi_u16 theKeyId,
											  const gsi_char * theValue)
{
	return scReportAddStringValue(theReportData, theKeyId, theValue);
}

SCResult CGameSpy_ATLAS::ReportEnd(SCReportPtr 	theReport,
								   gsi_bool		isAuth,
								   SCGameStatus theStatus)
{
	return scReportEnd(theReport, isAuth, theStatus);
}
