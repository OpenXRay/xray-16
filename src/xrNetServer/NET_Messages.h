#pragma once
#pragma pack(push, 1)

#define DPNSEND_IMMEDIATELLY 0x0100

IC u32 net_flags(
    bool bReliable = false, bool bSequental = true, bool bHighPriority = false, bool bSendImmediatelly = false)
{
#ifdef LINUX // FIXME!!!
    return 0;
#else
    return (bReliable ? DPNSEND_GUARANTEED : DPNSEND_NOCOMPLETE) | (bSequental ? 0 : DPNSEND_NONSEQUENTIAL) |
        (bHighPriority ? DPNSEND_PRIORITY_HIGH : 0) | (bSendImmediatelly ? DPNSEND_IMMEDIATELLY : 0);
#endif
}

struct MSYS_CONFIG
{
    u32 sign1; // 0x12071980;
    u32 sign2; // 0x26111975;
};

struct MSYS_PING
{
    u32 sign1; // 0x12071980;
    u32 sign2; // 0x26111975;
    u32 dwTime_ClientSend;
    u32 dwTime_Server;
    u32 dwTime_ClientReceive;
};

#pragma pack(pop)
