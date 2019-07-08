#pragma once
#include "net_shared.h"

class IPureServer;

class XRNETSERVER_API IClient
{
public:
    IClient() {}
    virtual ~IClient() {}

    ClientID ID;
};

IC bool operator==(IClient const* pClient, ClientID const& ID) { return pClient->ID == ID; }

//==============================================================================

class CSE_Abstract;
class CServerInfo;
class IServerGameState;

class XRNETSERVER_API IPureServer
{
public:
    enum EConnect
    {
        ErrConnect,
        ErrMax,
        ErrNoError = ErrMax,
    };

protected:
    shared_str connect_options;
    IClient* SV_Client;

    //
    Lock csMessage;
    CTimer* device_timer;

    IClient* ID_to_client(ClientID ID, bool ScanAll = false);

public:
    IPureServer(CTimer* timer, bool Dedicated = false);
    virtual ~IPureServer();

    virtual EConnect Connect(pcstr session_name);

    // extended functionality
    virtual void client_Destroy(IClient* C) = 0; // destroy client info

    IClient* GetServerClient() const { return SV_Client; };

    const shared_str& GetConnectOptions() const { return connect_options; }
    virtual IServerGameState* GetGameState() = 0;
    virtual u16 PerformIDgen(u16 ID) = 0;
    virtual void FreeID(u16 ID, u32 time) = 0;

    virtual CSE_Abstract* entity_Create(pcstr name) = 0;
    virtual void entity_Destroy(CSE_Abstract*& entity) = 0;

    virtual CSE_Abstract* Process_spawn(NET_Packet& packet, ClientID sender, bool mainEntityAsParent = false,
        CSE_Abstract* currentEntity = nullptr) = 0;

private:
#ifdef DEBUG
    bool sender_functor_invoked;
#endif
};
