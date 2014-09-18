#include "StdAfx.h"
#include "GameSpy_HTTP.h"
#include "GameSpy_Base_Defs.h"
#include "../MainMenu.h"

CGameSpy_HTTP::CGameSpy_HTTP()
{
	m_hGameSpyDLL = NULL;
	m_LastRequest	= -1;

	LPCSTR			g_name	= "xrGameSpy.dll";
	Log				("Loading DLL:",g_name);
	m_hGameSpyDLL			= LoadLibrary	(g_name);
	if (0==m_hGameSpyDLL)	R_CHK			(GetLastError());
	R_ASSERT2		(m_hGameSpyDLL,"GameSpy DLL raised exception during loading or there is no game DLL at all");

	LoadGameSpy(m_hGameSpyDLL);

	StartUp();
};

CGameSpy_HTTP::CGameSpy_HTTP(HMODULE hGameSpyDLL)
{
	m_hGameSpyDLL = NULL;
	m_LastRequest	= -1;

	LoadGameSpy(hGameSpyDLL);

	StartUp();
};

CGameSpy_HTTP::~CGameSpy_HTTP()
{
	CleanUp();

	if (m_hGameSpyDLL)
	{
		FreeLibrary(m_hGameSpyDLL);
		m_hGameSpyDLL = NULL;
	}
};
void	CGameSpy_HTTP::LoadGameSpy(HMODULE hGameSpyDLL)
{
	//-----------------------------------------------------
	GAMESPY_LOAD_FN(xrGS_ghttpStartup);
	GAMESPY_LOAD_FN(xrGS_ghttpCleanup);
	GAMESPY_LOAD_FN(xrGS_ghttpThink);

	GAMESPY_LOAD_FN(xrGS_ghttpSaveA);
	GAMESPY_LOAD_FN(xrGS_ghttpSaveExA);
	GAMESPY_LOAD_FN(xrGS_ghttpCancelRequest);

	GAMESPY_LOAD_FN(xrGS_GetGameID);

}

void		CGameSpy_HTTP::StartUp		()
{
	xrGS_ghttpStartup();
}

void		CGameSpy_HTTP::CleanUp		()
{
	xrGS_ghttpCleanup();
}

void		CGameSpy_HTTP::Think		()
{
	xrGS_ghttpThink();
}
void __cdecl ProgressCallback ( GHTTPRequest request, GHTTPState state, const char * buffer, GHTTPByteCount bufferLen, GHTTPByteCount bytesReceived, GHTTPByteCount totalSize, void * param )
{
	if (state == GHTTPReceivingFile && totalSize != 0)
		MainMenu()->OnDownloadPatchProgress(bytesReceived, totalSize);
}

string128	GHTTPResultStr	[] =  {
	"GHTTPSuccess",               // 0:  Successfully retrieved file.
	"GHTTPOutOfMemory",           // 1:  A memory allocation failed.
	"GHTTPBufferOverflow",        // 2:  The user-supplied buffer was too small to hold the file.
	"GHTTPParseURLFailed",        // 3:  There was an error parsing the URL.
	"GHTTPHostLookupFailed",      // 4:  Failed looking up the hostname.
	"GHTTPSocketFailed",          // 5:  Failed to create/initialize/read/write a socket.
	"GHTTPConnectFailed",         // 6:  Failed connecting to the http server.
	"GHTTPBadResponse",           // 7:  Error understanding a response from the server.
	"GHTTPRequestRejected",       // 8:  The request has been rejected by the server.
	"GHTTPUnauthorized",          // 9:  Not authorized to get the file.
	"GHTTPForbidden",             // 10: The server has refused to send the file.
	"GHTTPFileNotFound",          // 11: Failed to find the file on the server.
	"GHTTPServerError",           // 12: The server has encountered an internal error.
	"GHTTPFileWriteFailed",       // 13: An error occured writing to the local file (for ghttpSaveFile[Ex]).
	"GHTTPFileReadFailed",        // 14: There was an error reading from a local file (for posting files from disk).
	"GHTTPFileIncomplete",        // 15: Download started but was interrupted.  Only reported if file size is known.
	"GHTTPFileToBig",             // 16: The file is to big to be downloaded (size exceeds range of interal data types)
	"GHTTPEncryptionError",       // 17: Error with encryption engine.
	"GHTTPRequestCancelled"       // 18: User requested cancel and/or graceful close.
};

GHTTPBool	__cdecl	CompletedCallBack	(GHTTPRequest request, GHTTPResult result, char * buffer, GHTTPByteCount bufferLen, void * param )
{
	switch (result)
	{
	case GHTTPSuccess:
		MainMenu()->OnDownloadPatchSuccess();
		break;
	default:
		Msg ("! CompletedCallBack Result - %s", GHTTPResultStr[result]);
		MainMenu()->OnDownloadPatchError();
		break;

	}
	
//	CGameSpy_HTTP* pGSHTTP = (CGameSpy_HTTP*) param;
//	if (pGSHTTP) pGSHTTP->StopDownload();
	return GHTTPTrue;
}

void		CGameSpy_HTTP::DownloadFile(LPCSTR URL, LPCSTR FileName)
{	

//	GHTTPRequest res = xrGS_ghttpSaveA(URL, FileName, GHTTPFalse, CompletedCallBack, this);
	Msg		("URL:  %s",URL);
	Msg		("File: %s",FileName);
	m_LastRequest = xrGS_ghttpSaveExA(URL, FileName, "", NULL, GHTTPFalse, GHTTPFalse, ProgressCallback, CompletedCallBack, this);
	Msg		("Code: %d",m_LastRequest);
	if (m_LastRequest < 0)
	{
		MainMenu()->OnDownloadPatchError();
	}
}

void		CGameSpy_HTTP::StopDownload	()
{
	if (m_LastRequest != -1)	xrGS_ghttpCancelRequest(m_LastRequest);
	m_LastRequest = -1;
}
