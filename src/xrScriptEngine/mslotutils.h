#pragma once
#include "xrCore/xrCore.h"

class CMailSlotMsg
{
private:
    char m_buff[2048];
    u32 m_len;
    int m_pos;

    void Read(void* dst, int sz)
    {
        memcpy(dst, m_buff + m_pos, sz);
        m_pos += sz;
    }

    void Write(const void* src, int sz)
    {
        memcpy(m_buff + m_pos, src, sz);
        m_pos += sz;
        m_len = m_pos;
    }

public:
    CMailSlotMsg() { Reset(); }
    void Reset()
    {
        m_len = 0;
        m_pos = 0;
        memset(m_buff, 0, 2048);
    }

    void SetBuffer(const char* b, int sz)
    {
        Reset();
        memcpy(m_buff, b, sz);
        m_len = sz;
        m_pos = 0;
    }

    void* GetBuffer() { return m_buff; }
    void SetLen(u32 l) { m_len = l; }
    u32 GetLen() const { return m_len; }
    bool r_string(char* dst)
    {
        int sz;
        r_int(sz);
        Read(dst, sz + 1);
        return TRUE;
    }

    bool w_string(const char* dst)
    {
        size_t sz = strlen(dst);
        w_int((int)sz);
        Write(dst, (int)(sz + 1));
        return TRUE;
    }

    bool r_float(float& dst)
    {
        Read(&dst, sizeof(float));
        return TRUE;
    }

    bool w_float(const float src)
    {
        Write(&src, sizeof(float));
        return TRUE;
    }

    bool r_int(int& dst)
    {
        Read(&dst, sizeof(int));
        return TRUE;
    }

    bool w_int(const int src)
    {
        Write(&src, sizeof(int));
        return TRUE;
    }

    bool r_buff(void* dst, int sz)
    {
        Read(dst, sz);
        return TRUE;
    }

    bool w_buff(void* src, int sz)
    {
        Write(src, sz);
        return TRUE;
    }
};

inline HANDLE CreateMailSlotByName(const char* slotName)
{
#if defined(WINDOWS)
    HANDLE hSlot = CreateMailslot(slotName,
        0, // no maximum message size
        MAILSLOT_WAIT_FOREVER, // no time-out for operations
        (LPSECURITY_ATTRIBUTES)NULL); // no security attributes
    return hSlot;
#elif defined(LINUX)
    return NULL;
#endif
}

inline bool CheckExisting(const char* slotName)
{
    HANDLE hFile;
    bool res;
#if defined(WINDOWS)
    hFile = CreateFile(slotName, GENERIC_WRITE,
        FILE_SHARE_READ, // required to write to a mailslot
        (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    res = hFile != INVALID_HANDLE_VALUE;
    if (res)
        CloseHandle(hFile);
#endif
    return res;
}

inline bool SendMailslotMessage(const char* slotName, CMailSlotMsg& msg)
{
    bool fResult;
    HANDLE hFile;
    DWORD cbWritten;

#if defined(WINDOWS)
    hFile = CreateFile(slotName, GENERIC_WRITE,
        FILE_SHARE_READ, // required to write to a mailslot
        (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    R_ASSERT(hFile != INVALID_HANDLE_VALUE);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;
    fResult = WriteFile(hFile, msg.GetBuffer(), msg.GetLen(), &cbWritten, (LPOVERLAPPED)NULL);
    R_ASSERT(fResult);
    fResult = CloseHandle(hFile);
    R_ASSERT(fResult);
#endif
    return fResult;
}

inline bool CheckMailslotMessage(HANDLE hSlot, CMailSlotMsg& msg)
{
    DWORD cbMessage, cMessage, cbRead;
    bool fResult;
#if defined(WINDOWS)
    HANDLE hEvent;
    OVERLAPPED ov;
    cbMessage = cMessage = cbRead = 0;
    hEvent = CreateEvent(NULL, FALSE, FALSE, "__Slot");
    if (!hEvent)
        return FALSE;
    ov.Offset = 0;
    ov.OffsetHigh = 0;
    ov.hEvent = hEvent;
    fResult = GetMailslotInfo(hSlot, // mailslot handle
        (LPDWORD)NULL, // no maximum message size
        &cbMessage, // size of next message
        &cMessage, // number of messages
        (LPDWORD)NULL); // no read time-out
    R_ASSERT(fResult);
    if (!fResult || cbMessage == MAILSLOT_NO_MESSAGE)
    {
        CloseHandle(hEvent);
        return false;
    }
    msg.Reset();
    fResult = ReadFile(hSlot, msg.GetBuffer(), cbMessage, &cbRead, &ov);
    msg.SetLen(cbRead);
    R_ASSERT(fResult);
    CloseHandle(hEvent);
#endif
    return fResult;
}
