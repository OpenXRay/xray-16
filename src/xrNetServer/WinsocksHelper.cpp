#include "stdafx.h"
#include "WinsocksHelper.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma warning(push)
#pragma warning(disable:4995)

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma warning(pop)

#pragma comment(lib, "ws2_32.lib")

unsigned long WinsocksHelper::GetIpAddress(LPCSTR hostName)
{
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        Msg("[WinsocksHelper] WSAStartup failed: %d\n", iResult);
        WSACleanup();
        return 0;
    }

    hostent* remoteHost = gethostbyname(hostName);
    if (remoteHost != nullptr &&
        remoteHost->h_length > 0 &&
        remoteHost->h_addrtype == AF_INET // Supports only IPv4 (AF_INET)
        )
    {
        in_addr addr;
        addr.s_addr = *(u_long*)remoteHost->h_addr_list[0];
        u_long ip = htonl(addr.S_un.S_addr);

        WSACleanup();
        return ip;
    }

    WSACleanup();
    return 0;
}
