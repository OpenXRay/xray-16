//----------------------------------------------------
// file: NetDeviceLog.h
//----------------------------------------------------

#ifndef _INCDEF_NETDEVICELOG_H_
#define _INCDEF_NETDEVICELOG_H_

// -------
#define NLOG_CONSOLE_OUT
// -------

class CExportConsole{
protected:

	bool m_Valid;
	HANDLE m_hThread;
	DWORD m_ThreadId;


public:

	HWND m_hParent;
	HWND m_hWindow;
	HINSTANCE m_hInstance;

	CRITICAL_SECTION m_CSection;

	bool m_Enter;
	char m_EnterBuffer[256];

	class _ConsoleMsg{
	public:
		char buf[1024];
		_ConsoleMsg(LPCSTR b){ strcpy(buf,b); }
	};
		
	std::list<_ConsoleMsg> m_Messages;

	float fMaxVal, fStatusProgress;
public:

	bool Init( HINSTANCE _Inst, HWND _Window );
	void Clear();

	void print	(TMsgDlgType mt, const char *buf);

	bool valid();

	void ProgressStart(float max_val, const char* text=0);
	void ProgressEnd();
	void ProgressInc();
	void ProgressUpdate(float val);

	void StayOnTop	(BOOL flag);

	CExportConsole();
	~CExportConsole();
};

extern CExportConsole EConsole;

#endif /*_INCDEF_NETDEVICELOG_H_*/

