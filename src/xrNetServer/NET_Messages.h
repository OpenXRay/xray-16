#pragma once
#include "NET_Common.h"
#pragma warning(push)
#pragma warning(disable:4995)
#include <steam/steamnetworkingtypes.h>
#pragma warning(pop)

// Direct Play defines

#define DPNSEND_SYNC DPNOP_SYNC
#define DPNSEND_NOCOPY 0x0001
#define DPNSEND_NOCOMPLETE 0x0002
#define DPNSEND_COMPLETEONPROCESS 0x0004
#define DPNSEND_GUARANTEED 0x0008
#define DPNSEND_NONSEQUENTIAL 0x0010
#define DPNSEND_NOLOOPBACK 0x0020
#define DPNSEND_PRIORITY_LOW 0x0040
#define DPNSEND_PRIORITY_HIGH 0x0080

#pragma pack(push,1)

#define	DPNSEND_IMMEDIATELLY 0x0100

IC u32 net_flags(
    bool bReliable = false, bool bSequental = true, bool bHighPriority = false, bool bSendImmediatelly = false)
{
    return (bReliable ? DPNSEND_GUARANTEED : DPNSEND_NOCOMPLETE) | (bSequental ? 0 : DPNSEND_NONSEQUENTIAL) |
        (bHighPriority ? DPNSEND_PRIORITY_HIGH : 0) | (bSendImmediatelly ? DPNSEND_IMMEDIATELLY : 0);
}

IC int convert_flags_for_steam(u32 flags)
{
    int steam_flags;
    bool bReliable = (flags & DPNSEND_GUARANTEED);
    bool bHighPriority = (flags & DPNSEND_PRIORITY_HIGH);

    if (bReliable)
    {
        steam_flags = (!bHighPriority) ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_ReliableNoNagle;
    }
    else
    {
        steam_flags = k_nSteamNetworkingSend_UnreliableNoDelay; // k_nSteamNetworkingSend_Unreliable|k_nSteamNetworkingSend_NoDelay|k_nSteamNetworkingSend_NoNagle;
    }

    // Ignore
    //bool bSequental = !(flags & DPNSEND_NONSEQUENTIAL);
    // Ignore because it is a custom realization
    //bool bSendImmediatelly = (flags & DPNSEND_IMMEDIATELLY);

    return steam_flags;
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

struct	MSYS_CLIENT_DATA
{
	u32 sign1;	// 0x02281488;
	u32 sign2;	// 0x01488228;

	string64		name;
	string64		pass;
	u32					process_id;
};

struct	MSYS_GAME_DESCRIPTION
{
    u32 sign1; // 0x02281488;
    u32 sign2; // 0x01488228;

    GameDescriptionData data;
};

#pragma pack(pop)
