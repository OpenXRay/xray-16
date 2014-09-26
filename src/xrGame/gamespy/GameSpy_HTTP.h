#pragma once

#include "GameSpy_FuncDefs.h"

class CGameSpy_HTTP
{
private:
	HMODULE	m_hGameSpyDLL;

	void	LoadGameSpy(HMODULE hGameSpyDLL);
public:
	CGameSpy_HTTP();
	CGameSpy_HTTP(HMODULE hGameSpyDLL);
	~CGameSpy_HTTP();

	void		StartUp		();
	void		CleanUp		();

	void		DownloadFile	(LPCSTR URL, LPCSTR FileName);
	void		StopDownload	();
	void		Think		();

	
private:
	GHTTPRequest	m_LastRequest;

	GAMESPY_FN_VAR_DECL(void, ghttpStartup, ());
	GAMESPY_FN_VAR_DECL(void, ghttpCleanup, ());
	GAMESPY_FN_VAR_DECL(void, ghttpThink,());
	GAMESPY_FN_VAR_DECL(void, ghttpCancelRequest, ( GHTTPRequest request )); 

	GAMESPY_FN_VAR_DECL(GHTTPRequest, ghttpSaveA,
		(
		const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
		const char * filename,  // The path and name to store the file as locally.
		GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
		ghttpCompletedCallback completedCallback,  // Called when the file has been received.
		void * param                // User-data to be passed to the callbacks.
		));

	GAMESPY_FN_VAR_DECL(GHTTPRequest, ghttpSaveExA,
		(
		const char * URL,       // The URL for the file ("http://host.domain[:port]/path/filename").
		const char * filename,  // The path and name to store the file as locally.
		const char * headers,   // Optional headers to pass with the request.  Can be NULL or "".
		void* post,             // Optional data to be posted.
		GHTTPBool throttle,         // If true, throttle this connection's download speed.
		GHTTPBool blocking,         // If true, this call doesn't return until the file has been recevied.
		ghttpProgressCallback progressCallback,    // Called periodically with progress updates.
		ghttpCompletedCallback completedCallback,  // Called when the file has been received.
		void * param                // User-data to be passed to the callbacks.
		));
public:
	GAMESPY_FN_VAR_DECL(void, GetGameID,	(int* GameID, int verID));
};