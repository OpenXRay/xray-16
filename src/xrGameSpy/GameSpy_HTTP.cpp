#include "StdAfx.h"
#include "GameSpy_HTTP.h"

CGameSpy_HTTP::CGameSpy_HTTP()
{
	m_LastRequest	= -1;
	StartUp();
};

CGameSpy_HTTP::~CGameSpy_HTTP()
{
	CleanUp();
}

void		CGameSpy_HTTP::StartUp		()
{
	ghttpStartup();
}

void		CGameSpy_HTTP::CleanUp		()
{
    ghttpCleanup();
}

void		CGameSpy_HTTP::Think		()
{
    ghttpThink();
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

class DownloadContext
{
public:
    using CompletionCallback = CGameSpy_HTTP::CompletionCallback;
    using ProgressCallback = CGameSpy_HTTP::ProgressCallback;
    
    CompletionCallback &Completed;
    ProgressCallback &Progress;

    DownloadContext(CompletionCallback &completed, ProgressCallback &progress) :
        Completed(completed), Progress(progress)
    {}
};

void __cdecl ProgressHandler(GHTTPRequest request, GHTTPState state, const char *buffer,
    GHTTPByteCount bufferLen, GHTTPByteCount received, GHTTPByteCount total, void *param)
{
    auto ctx = static_cast<DownloadContext*>(param);
    if (state==GHTTPReceivingFile && total)
        ctx->Progress(received, total);
}

GHTTPBool __cdecl CompletedHandler(GHTTPRequest request, GHTTPResult result,
    char *buffer, GHTTPByteCount bufferLen, void *param)
{
    auto ctx = static_cast<DownloadContext*>(param);
	switch (result)
	{
	case GHTTPSuccess:
        ctx->Completed(true);
		break;
	default:
		Msg ("! CompletedCallBack Result - %s", GHTTPResultStr[result]);
        ctx->Completed(false);
		break;
	}
	
//	CGameSpy_HTTP* pGSHTTP = (CGameSpy_HTTP*) param;
//	if (pGSHTTP) pGSHTTP->StopDownload();
	return GHTTPTrue;
}

void CGameSpy_HTTP::DownloadFile(LPCSTR URL, LPCSTR FileName,
    CompletionCallback &completed, ProgressCallback &progress)
{	
    DownloadContext ctx(completed, progress);
//	GHTTPRequest res = xrGS_ghttpSaveA(URL, FileName, GHTTPFalse, CompletedCallBack, this);
	Msg		("URL:  %s",URL);
	Msg		("File: %s",FileName);
	m_LastRequest = ghttpSaveExA(URL, FileName, "", NULL, GHTTPFalse, GHTTPFalse, ProgressHandler, CompletedHandler, &ctx);
	Msg		("Code: %d",m_LastRequest);
	if (m_LastRequest < 0)
        completed(false);
}

void		CGameSpy_HTTP::StopDownload	()
{
	if (m_LastRequest != -1)
        ghttpCancelRequest(m_LastRequest);
	m_LastRequest = -1;
}
