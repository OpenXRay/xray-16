#include "stdafx.h"
#include "xrCore/Debug/dxerr.h"
#include "NET_Server.h"
#include <functional>

XRNETSERVER_API ClientID BroadcastCID(0xffffffff);

//------------------------------------------------------------------------------
IClient* IPureServer::ID_to_client(ClientID ID, bool ScanAll)
{
    if (ID.value())
        if (SV_Client->ID == ID)
            return SV_Client;

    return nullptr;
}

//==============================================================================
IPureServer::IPureServer(CTimer* timer, bool Dedicated)
{
    device_timer = timer;
    SV_Client = nullptr;
#ifdef DEBUG
    sender_functor_invoked = false;
#endif
}

IPureServer::~IPureServer()
{
    SV_Client = nullptr;
}

IPureServer::EConnect IPureServer::Connect(pcstr options)
{
    connect_options = options;

    return ErrNoError;
}
