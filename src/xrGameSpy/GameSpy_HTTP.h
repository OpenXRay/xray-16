#pragma once

#include "xrCore/xrCore.h"
#include "xrGameSpy/xrGameSpy.h"

class XRGAMESPY_API CGameSpy_HTTP
{
public:
    using CompletionCallback = fastdelegate::FastDelegate<void(bool)>;
    using ProgressCallback = fastdelegate::FastDelegate<void(u64 received, u64 total)>;

	CGameSpy_HTTP();
	~CGameSpy_HTTP();

	void		StartUp		();
	void		CleanUp		();

	void		DownloadFile	(LPCSTR URL, LPCSTR FileName, CompletionCallback &completed, ProgressCallback &progress);
	void		StopDownload	();
	void		Think		();
	
private:
	GHTTPRequest	m_LastRequest;
};
