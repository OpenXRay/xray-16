#include "stdafx.h"
#pragma hdrstop

#ifdef DEBUG
#ifndef LINUX // FIXME!!!
static HANDLE hLocalSlot = INVALID_HANDLE_VALUE;
#endif
extern void msParse(LPCSTR cmd);

void msCreate(LPCSTR name)
{
#ifndef LINUX // FIXME!!!
    string256 fn;
    xr_sprintf(fn, sizeof(fn), "\\\\.\\mailslot\\%s", name);
    hLocalSlot = CreateMailslot(fn,
        0, // no maximum message size
        MAILSLOT_WAIT_FOREVER, // no time-out for operations
        (LPSECURITY_ATTRIBUTES)NULL); // no security attributes
    if (hLocalSlot == INVALID_HANDLE_VALUE)
        return;
    // Msg ("* mailSLOT successfully created.");
#endif
}

void msRead(void)
{
#ifndef LINUX // FIXME!!!
    DWORD cbMessage, cMessage, cbRead;
    BOOL fResult;
    LPSTR lpszBuffer;

    cbMessage = cMessage = cbRead = 0;
    fResult = GetMailslotInfo(hLocalSlot, // mailslot handle
        (LPDWORD)NULL, // no maximum message size
        &cbMessage, // size of next message
        &cMessage, // number of messages
        (LPDWORD)NULL); // no read time-out
    if (!fResult)
        return;
    if (cbMessage == MAILSLOT_NO_MESSAGE)
        return;
    while (cMessage != 0) // retrieve all messages
    {
        // Allocate memory for the message.
        lpszBuffer = (LPSTR)GlobalAlloc(GPTR, cbMessage);
        lpszBuffer[0] = '\0';
        fResult = ReadFile(hLocalSlot, lpszBuffer, cbMessage, &cbRead, (LPOVERLAPPED)NULL);
        if (!fResult)
        {
            GlobalFree((HGLOBAL)lpszBuffer);
            return;
        }
        msParse(lpszBuffer);
        GlobalFree((HGLOBAL)lpszBuffer);
        fResult = GetMailslotInfo(hLocalSlot, // mailslot handle
            (LPDWORD)NULL, // no maximum message size
            &cbMessage, // size of next message
            &cMessage, // number of messages
            (LPDWORD)NULL); // no read time-out
        if (!fResult)
            return;
    }
#endif
}

void msWrite(char* name, char* dest, char* msg)
{
#ifndef LINUX // FIXME!!!
    HANDLE hFile;
    DWORD cbWritten;
    BOOL fResult;
    char cName[256];

    xr_sprintf(cName, sizeof(cName), "\\\\%s\\mailslot\\%s", name, dest);
    hFile = CreateFile(cName, GENERIC_WRITE,
        FILE_SHARE_READ, // required to write to a mailslot
        (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;
    fResult = WriteFile(hFile, msg, (u32)lstrlen(msg) + 1, &cbWritten, (LPOVERLAPPED)NULL);
    fResult = CloseHandle(hFile);
#endif
}

#endif
