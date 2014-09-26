#include "stdafx.h"
#include "GameSpy_ATLAS.h"

CGameSpy_ATLAS::CGameSpy_ATLAS	(HMODULE hGameSpyDLL)
{
	m_interface			= NULL;
	LoadGameSpyATLAS	(hGameSpyDLL);
	Init				();
}

CGameSpy_ATLAS::~CGameSpy_ATLAS	()
{
	if (m_interface)
	{
		xrGS_scShutdown(m_interface);
	}
}

void CGameSpy_ATLAS::LoadGameSpyATLAS(HMODULE hGameSpyDLL)
{
	GAMESPY_LOAD_FN(xrGS_wsLoginProfile);
	GAMESPY_LOAD_FN(xrGS_scInitialize);
	GAMESPY_LOAD_FN(xrGS_scShutdown);
	GAMESPY_LOAD_FN(xrGS_scThink);
	GAMESPY_LOAD_FN(xrGS_scCreateSession);
	GAMESPY_LOAD_FN(xrGS_scGetSessionId);
	GAMESPY_LOAD_FN(xrGS_scSetSessionId);
	GAMESPY_LOAD_FN(xrGS_scSetReportIntention);
	GAMESPY_LOAD_FN(xrGS_scGetConnectionId);
	GAMESPY_LOAD_FN(xrGS_scCreateReport);
	GAMESPY_LOAD_FN(xrGS_scReportBeginGlobalData);
	GAMESPY_LOAD_FN(xrGS_scReportBeginPlayerData);
	GAMESPY_LOAD_FN(xrGS_scReportBeginNewPlayer);
	GAMESPY_LOAD_FN(xrGS_scReportSetPlayerData);
	GAMESPY_LOAD_FN(xrGS_scReportAddIntValue);
	GAMESPY_LOAD_FN(xrGS_scReportAddStringValue);
	GAMESPY_LOAD_FN(xrGS_scReportEnd);
	GAMESPY_LOAD_FN(xrGS_scSubmitReport);
}

void CGameSpy_ATLAS::Init()
{
	SCResult init_res = xrGS_scInitialize(&m_interface);
	VERIFY(init_res == SCResult_NO_ERROR);
	if (init_res != SCResult_NO_ERROR)
	{
		Msg("! GameSpy ATLAS: failed to initialize, error code: %d", init_res);
	}
}

void CGameSpy_ATLAS::Think()
{
	xrGS_scThink(m_interface);
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
	return static_cast<u32>(
		xrGS_wsLoginProfile(
			nick.c_str(),
			email.c_str(),
			password.c_str(),
			NULL,
			callback,
			userData
		)
	);
}

SCResult CGameSpy_ATLAS::CreateSession(const GSLoginCertificate * theCertificate,
									   const GSLoginPrivateData * thePrivateData,
									   SCCreateSessionCallback theCallback,
									   gsi_time theTimeoutMs,
									   void * theUserData)
{
	return xrGS_scCreateSession(
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
	return xrGS_scSetReportIntention(
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
	return xrGS_scGetConnectionId(m_interface);
}

SCResult CGameSpy_ATLAS::SubmitReport(const SCReportPtr theReport,
									  gsi_bool isAuthoritative,
									  const GSLoginCertificate * theCertificate,
									  const GSLoginPrivateData * thePrivateData,
									  SCSubmitReportCallback 	theCallback,
									  gsi_time 					theTimeoutMs,
									  void * 					theUserData)
{
	return xrGS_scSubmitReport(
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
	return xrGS_scCreateReport(
		m_interface,
		theHeaderVersion,
		thePlayerCount,
		theTeamCount,
		theReportOut
	);
}
SCResult CGameSpy_ATLAS::ReportBeginGlobalData(SCReportPtr theReportData)
{
	return xrGS_scReportBeginGlobalData(theReportData);
}

SCResult CGameSpy_ATLAS::ReportBeginPlayerData(SCReportPtr theReportData)
{
	return xrGS_scReportBeginPlayerData(theReportData);
}

SCResult CGameSpy_ATLAS::ReportBeginNewPlayer(SCReportPtr theReportData)
{
	return xrGS_scReportBeginNewPlayer(theReportData);
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
	return xrGS_scReportSetPlayerData(
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
	return xrGS_scReportAddIntValue(theReportData, theKeyId, theValue);
}

SCResult CGameSpy_ATLAS::ReportAddStringValue(SCReportPtr theReportData,
											  gsi_u16 theKeyId,
											  const gsi_char * theValue)
{
	return xrGS_scReportAddStringValue(theReportData, theKeyId, theValue);
}

SCResult CGameSpy_ATLAS::ReportEnd(SCReportPtr 	theReport,
								   gsi_bool		isAuth,
								   SCGameStatus theStatus)
{
	return xrGS_scReportEnd(theReport, isAuth, theStatus);
}