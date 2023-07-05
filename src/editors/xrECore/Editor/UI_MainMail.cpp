//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

//---------------------------------------------------------------------------
#if 0
bool TUI::CreateMailslot()
{
	xr_string slot_name = xr_string("\\\\.\\mailslot\\")+xr_string(UI->EditorName());
    hMailSlot = ::CreateMailslot(slot_name.c_str(),
        0,                             // no maximum message size
        MAILSLOT_WAIT_FOREVER,         // no time-out for operations
        (LPSECURITY_ATTRIBUTES) NULL); // no security attributes
    return (hMailSlot != INVALID_HANDLE_VALUE);
}
//---------------------------------------------------------------------------

void TUI::CheckMailslot()
{
    DWORD cbMessage, cMessage, cbRead;
	BOOL fResult;
	LPSTR lpszBuffer;

    cbMessage = cMessage = cbRead = 0;
    fResult = GetMailslotInfo(hMailSlot,	// mailslot handle
        (LPDWORD) NULL,						// no maximum message size
        &cbMessage,							// size of next message
        &cMessage,							// number of messages
        (LPDWORD) NULL);					// no read time-out
	R_ASSERT2(fResult,"Can't get mailslot info");
	if (cbMessage == MAILSLOT_NO_MESSAGE) return;
    while (cMessage != 0)  // retrieve all messages
	{
		// Allocate memory for the message.
		lpszBuffer = (LPSTR) GlobalAlloc(GPTR, cbMessage);
		lpszBuffer[0] = '\0';
		fResult = ReadFile(hMailSlot, lpszBuffer, cbMessage, &cbRead, (LPOVERLAPPED) NULL);
		if (!fResult) {
			GlobalFree((HGLOBAL) lpszBuffer);
			R_ASSERT(0);
			//throw Exception("Can't ReadFile");
		}
		OnReceiveMail(lpszBuffer);
		GlobalFree((HGLOBAL) lpszBuffer);
		fResult = GetMailslotInfo(hMailSlot,	// mailslot handle
			(LPDWORD) NULL,							// no maximum message size
			&cbMessage,								// size of next message
			&cMessage,								// number of messages
			(LPDWORD) NULL);						// no read time-out
		R_ASSERT2(fResult,"Can't get mailslot info");
    }
}
//---------------------------------------------------------------------------

void TUI::OnReceiveMail(LPCSTR msg)
{
	int cnt = _GetItemCount(msg,' ');
    if (cnt){
		xr_string M = xr_string(msg);
		for (char& ch : M)
		{
			ch = tolower(ch);
		}
		xr_string name = UI->EditorName();
		for (char& ch : name)
		{
			ch = toupper(ch);
		}
        xr_string p[2];
		_GetItem(msg,0,p[0],' ',"",false);
        if (cnt>1) _GetItems(msg,1,cnt,( char*)p[1].c_str(),' ');
        if (p[0]=="exit")
		{
        	ELog.DlgMsg(mtInformation,"'%s EDITOR': Critical update!", name.c_str());
            while (1){
            	if (ExecCommand(COMMAND_EXIT)){
	                ExecCommand(COMMAND_QUIT);
                    break;
                }
            }
        }else if (p[0]=="quit")
		{
        	ELog.Msg(mtInformation,"'%s EDITOR': Super critical update!", name.c_str());
        	ExecCommand(COMMAND_SAVE_BACKUP);
        	ExecCommand(COMMAND_QUIT);
        }
		else if (p[0]=="info")
		{
        	if (cnt>1) ELog.DlgMsg(mtInformation,"'%s EDITOR': %s", name.c_str(),p[1].c_str());
        }else if (p[0]=="error")
		{
        	if (cnt>1) ELog.DlgMsg(mtError,"'%s EDITOR': %s", name.c_str(),p[1].c_str());
        }
    }
}
//---------------------------------------------------------------------------

void TUI::SendMail(LPCSTR name, LPCSTR dest, LPCSTR msg)
{
	HANDLE	hFile; 
	DWORD	cbWritten;
	BOOL	fResult;
	char    cName[256];

	sprintf(cName,"\\\\%s\\mailslot\\%s",name,dest);
	hFile = CreateFile(
		cName, 
		GENERIC_WRITE, 
		FILE_SHARE_READ,  // required to write to a mailslot 
		(LPSECURITY_ATTRIBUTES) NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 
		(HANDLE) NULL);  
	if (hFile == INVALID_HANDLE_VALUE) 	return;
	fResult = WriteFile(
		hFile, 
		msg, 
		(u32) lstrlen(msg) + 1,
		&cbWritten,     
		(LPOVERLAPPED) NULL);  
	if (!fResult) return;
	fResult = CloseHandle(hFile);
	if (!fResult) return;
}
//---------------------------------------------------------------------------

#endif