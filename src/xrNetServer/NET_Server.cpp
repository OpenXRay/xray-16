#include "stdafx.h"
#include "xrCore/Debug/dxerr.h"
#include "NET_Server.h"
#include <functional>
#include <dplay/dplay8.h>
#include "NET_Messages.h"
#include "xrCore/buffer_vector.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

#define NET_BANNED_STR "Player banned by server!"
#define NET_PROTECTED_SERVER_STR "Access denied by protected server for this player!"
#define NET_NOTFOR_SUBNET_STR "Your IP does not present in server's subnet"

pcstr nameTraffic = "traffic.net";

XRNETSERVER_API int psNET_ServerUpdate = 30; // FPS
XRNETSERVER_API int psNET_ServerPending = 3;

XRNETSERVER_API ClientID BroadcastCID(0xffffffff);

void ip_address::set(pcstr src_string)
{
    u32 buff[4];
    int cnt = sscanf(src_string, "%d.%d.%d.%d", &buff[0], &buff[1], &buff[2], &buff[3]);
    if (cnt == 4)
    {
        m_data.a1 = u8(buff[0] & 0xff);
        m_data.a2 = u8(buff[1] & 0xff);
        m_data.a3 = u8(buff[2] & 0xff);
        m_data.a4 = u8(buff[3] & 0xff);
    }
    else
    {
        Msg("! Bad ipAddress format [%s]", src_string);
        m_data.data = 0;
    }
}

xr_string ip_address::to_string() const
{
    string128 res;
    xr_sprintf(res, sizeof(res), "%d.%d.%d.%d", m_data.a1, m_data.a2, m_data.a3, m_data.a4);
    return res;
}

void IBannedClient::Load(CInifile& ini, const shared_str& sect)
{
    HAddr.set(sect.c_str());

    tm _tm_banned;
    const shared_str& time_to = ini.r_string(sect, "time_to");
    int res_t = sscanf(time_to.c_str(), "%02d.%02d.%d_%02d:%02d:%02d", &_tm_banned.tm_mday, &_tm_banned.tm_mon,
                       &_tm_banned.tm_year, &_tm_banned.tm_hour, &_tm_banned.tm_min, &_tm_banned.tm_sec);
    VERIFY(res_t == 6);

    _tm_banned.tm_mon -= 1;
    _tm_banned.tm_year -= 1900;

    BanTime = mktime(&_tm_banned);

    Msg("- loaded banned client %s to %s", HAddr.to_string().c_str(), BannedTimeTo().c_str());
}

void IBannedClient::Save(CInifile& ini)
{
    ini.w_string(HAddr.to_string().c_str(), "time_to", BannedTimeTo().c_str());
}

xr_string IBannedClient::BannedTimeTo() const
{
    string256 res;
    tm* _tm_banned = _localtime64(&BanTime);
    xr_sprintf(res, sizeof(res), "%02d.%02d.%d_%02d:%02d:%02d", _tm_banned->tm_mday, _tm_banned->tm_mon + 1,
               _tm_banned->tm_year + 1900, _tm_banned->tm_hour, _tm_banned->tm_min, _tm_banned->tm_sec);

    return res;
}

IClient::IClient(CTimer* timer) : stats(timer), server(nullptr)
{
    dwTime_LastUpdate = 0;
    flags.bLocal = FALSE;
    flags.bConnected = FALSE;
    flags.bReconnect = FALSE;
    flags.bVerified = TRUE;
}

IClient::~IClient() {}

struct ClientStatisticImpl
{
    DPN_CONNECTION_INFO ci_last;
    u32 mps_receive, mps_receive_base;
    u32 mps_send, mps_send_base;
    u32 dwBaseTime;
    CTimer* device_timer;
};

IClientStatistic::IClientStatistic() :
    m_pimpl(new ClientStatisticImpl) {}

IClientStatistic::IClientStatistic(CTimer* timer)
{
    ZeroMemory(this, sizeof(*this));
    m_pimpl = new ClientStatisticImpl;
    m_pimpl->device_timer = timer;
    m_pimpl->dwBaseTime = TimeGlobal(m_pimpl->device_timer);
}

IClientStatistic::IClientStatistic(const IClientStatistic& rhs) :
    m_pimpl(new ClientStatisticImpl)
{
    *m_pimpl = *rhs.m_pimpl;
}

IClientStatistic::~IClientStatistic()
{
    delete m_pimpl;
}

void IClientStatistic::Update(DPN_CONNECTION_INFO& CI)
{
    const u32 time_global = TimeGlobal(m_pimpl->device_timer);
    if (time_global - m_pimpl->dwBaseTime >= 999)
    {
        m_pimpl->dwBaseTime = time_global;

        m_pimpl->mps_receive = CI.dwMessagesReceived - m_pimpl->mps_receive_base;
        m_pimpl->mps_receive_base = CI.dwMessagesReceived;

        u32 cur_msend = CI.dwMessagesTransmittedHighPriority + CI.dwMessagesTransmittedNormalPriority +
            CI.dwMessagesTransmittedLowPriority;
        m_pimpl->mps_send = cur_msend - m_pimpl->mps_send_base;
        m_pimpl->mps_send_base = cur_msend;

        dwBytesSendedPerSec = dwBytesSended;
        dwBytesSended = 0;
        dwBytesReceivedPerSec = dwBytesReceived;
        dwBytesReceived = 0;
    }
    m_pimpl->ci_last = CI;
}

u32 IClientStatistic::getPing() const { return m_pimpl->ci_last.dwRoundTripLatencyMS; }
u32 IClientStatistic::getBPS() const { return m_pimpl->ci_last.dwThroughputBPS; }
u32 IClientStatistic::getPeakBPS() const { return m_pimpl->ci_last.dwPeakThroughputBPS; }
u32 IClientStatistic::getDroppedCount() const { return m_pimpl->ci_last.dwPacketsDropped; }
u32 IClientStatistic::getRetriedCount() const { return m_pimpl->ci_last.dwPacketsRetried; }
u32 IClientStatistic::getMPS_Receive() const { return m_pimpl->mps_receive; }
u32 IClientStatistic::getMPS_Send() const { return m_pimpl->mps_send; }
u32 IClientStatistic::getReceivedPerSec() const { return dwBytesReceivedPerSec; }
u32 IClientStatistic::getSendedPerSec() const { return dwBytesSendedPerSec; }

void IClientStatistic::Clear()
{
    // XXX: Ugly, ugly hack (just following the lead of original code). FIX!
    ClientStatisticImpl* const saved_impl = m_pimpl;
    CTimer* const saved_timer = m_pimpl->device_timer;
    ZeroMemory(this, sizeof(*this));
    ZeroMemory(m_pimpl, sizeof(*m_pimpl));
    m_pimpl = saved_impl;
    m_pimpl->device_timer = saved_timer;
    m_pimpl->dwBaseTime = TimeGlobal(m_pimpl->device_timer);
}

// {0218FA8B-515B-4bf2-9A5F-2F079D1759F3}
static const GUID NET_GUID = {0x218fa8b, 0x515b, 0x4bf2, {0x9a, 0x5f, 0x2f, 0x7, 0x9d, 0x17, 0x59, 0xf3}};
// {8D3F9E5E-A3BD-475b-9E49-B0E77139143C}
static const GUID CLSID_NETWORKSIMULATOR_DP8SP_TCPIP = {
    0x8d3f9e5e, 0xa3bd, 0x475b, {0x9e, 0x49, 0xb0, 0xe7, 0x71, 0x39, 0x14, 0x3c}};

//------------------------------------------------------------------------------
IClient* IPureServer::ID_to_client(ClientID ID, bool ScanAll)
{
    if (0 == ID.value())
        return nullptr;
    IClient* ret_client = GetClientByID(ID);
    if (ret_client || !ScanAll)
        return ret_client;

    return nullptr;
}

//==============================================================================
#ifdef CONFIG_PROFILE_LOCKS
IPureServer::IPureServer(CTimer* timer, bool Dedicated) : csPlayers(MUTEX_PROFILE_ID(IPureServer::csPlayers)),
                                                          csMessage(MUTEX_PROFILE_ID(csMessage))
#else
IPureServer::IPureServer(CTimer* timer, bool Dedicated)
#endif
{
    device_timer = timer;
    stats.clear();
    stats.dwSendTime = TimeGlobal(device_timer);
    SV_Client = nullptr;
    NET = nullptr;
#ifdef DEBUG
    sender_functor_invoked = false;
#endif
}

IPureServer::~IPureServer()
{
    for (u32 it = 0; it < BannedAddresses.size(); it++)
        xr_delete(BannedAddresses[it]);

    BannedAddresses.clear();

    SV_Client = nullptr;

    psNET_direct_connect = false;
}

IPureServer::EConnect IPureServer::Connect(pcstr options, GameDescriptionData& game_descr)
{
    connect_options = options;
    psNET_direct_connect = false;

    if (strstr(options, "/single"))
        psNET_direct_connect = true;

    // Parse options
    string4096 session_name;

    string64 password_str = "";
    u32 dwMaxPlayers = 0;

    // certainly we can use game_descr structure for determining level_name,
    // but for backward compatibility we save next line...
    xr_strcpy(session_name, options);
    if (strchr(session_name, '/'))
        *strchr(session_name, '/') = 0;

    if (strstr(options, "psw="))
    {
        const char* PSW = strstr(options, "psw=") + 4;
        if (strchr(PSW, '/'))
            strncpy_s(password_str, PSW, strchr(PSW, '/') - PSW);
        else
            strncpy_s(password_str, PSW, 63);
    }
    if (strstr(options, "maxplayers="))
    {
        const char* sMaxPlayers = strstr(options, "maxplayers=") + 11;
        string64 tmpStr = "";
        if (strchr(sMaxPlayers, '/'))
            strncpy_s(tmpStr, sMaxPlayers, strchr(sMaxPlayers, '/') - sMaxPlayers);
        else
            strncpy_s(tmpStr, sMaxPlayers, 63);
        dwMaxPlayers = atol(tmpStr);
    }
    if (dwMaxPlayers > 32 || dwMaxPlayers < 1)
        dwMaxPlayers = 32;
#ifdef DEBUG
    Msg("MaxPlayers = %d", dwMaxPlayers);
#endif // #ifdef DEBUG

    return ErrNoError;
}

void IPureServer::Disconnect()
{
    if (NET)
        NET->Close(0);

    // Release interfaces
    _RELEASE(NET);
}

void IPureServer::SendTo_LL(ClientID ID /*DPNID ID*/, void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
    // send it
    DPN_BUFFER_DESC desc;
    desc.dwBufferSize = size;
    desc.pBufferData = LPBYTE(data);

#ifdef _DEBUG
    u32 time_global = TimeGlobal(device_timer);
    if (time_global - stats.dwSendTime >= 999)
    {
        stats.dwBytesPerSec = (stats.dwBytesPerSec * 9 + stats.dwBytesSended) / 10;
        stats.dwBytesSended = 0;
        stats.dwSendTime = time_global;
    }
    if (ID.value())
        stats.dwBytesSended += size;
#endif

    // verify
    VERIFY(desc.dwBufferSize);
    VERIFY(desc.pBufferData);

    DPNHANDLE hAsync = 0;
    HRESULT _hr = NET->SendTo(ID.value(), &desc, 1, dwTimeout, nullptr, &hAsync, dwFlags | DPNSEND_COALESCE);

    //	Msg("- IPureServer::SendTo_LL [%d]", size);

    if (SUCCEEDED(_hr) || (DPNERR_CONNECTIONLOST == _hr))
        return;

    R_CHK(_hr);
}

void IPureServer::SendTo(ClientID ID /*DPNID ID*/, NET_Packet& P, u32 dwFlags, u32 dwTimeout)
{
    SendTo_LL(ID, P.B.data, P.B.count, dwFlags, dwTimeout);
}

void IPureServer::SendBroadcast_LL(ClientID exclude, void* data, u32 size, u32 dwFlags)
{
    struct ClientExcluderPredicate
    {
        ClientID id_to_exclude;
        ClientExcluderPredicate(ClientID exclude) : id_to_exclude(exclude) {}

        bool operator()(IClient* client)
        {
            if (client->ID == id_to_exclude)
                return false;
            if (!client->flags.bConnected)
                return false;
            return true;
        }
    };

    struct ClientSenderFunctor
    {
        IPureServer* m_owner;
        void* m_data;
        u32 m_size;
        u32 m_dwFlags;

        ClientSenderFunctor(IPureServer* owner, void* data, u32 size, u32 dwFlags)
            : m_owner(owner), m_data(data), m_size(size), m_dwFlags(dwFlags) { }

        void operator()(IClient* client) { m_owner->SendTo_LL(client->ID, m_data, m_size, m_dwFlags); }
    };

    ClientSenderFunctor temp_functor(this, data, size, dwFlags);
    net_players.ForFoundClientsDo(ClientExcluderPredicate(exclude), temp_functor);
}

void IPureServer::SendBroadcast(ClientID exclude, NET_Packet& P, u32 dwFlags)
{
    // Perform broadcasting
    SendBroadcast_LL(exclude, P.B.data, P.B.count, dwFlags);
}

u32 IPureServer::OnMessage(NET_Packet& P, ClientID sender) // Non-Zero means broadcasting with "flags" as returned
{
    return 0;
}
