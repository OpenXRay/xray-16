#include "stdafx.h"
#include "xrCore/Debug/dxerr.h"
#include "NET_Common.h"
#include "NET_Server.h"
#include <functional>
//#include "xrCore/net_utils.h"
#include <dplay/dplay8.h>
#include "NET_Messages.h"
#include "NET_Log.h"
#include "xrGameSpy/xrGameSpy_MainDefs.h"
#include "xrCore/buffer_vector.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

static INetLog* pSvNetLog = nullptr;

#define NET_BANNED_STR "Player banned by server!"
#define NET_PROTECTED_SERVER_STR "Access denied by protected server for this player!"
#define NET_NOTFOR_SUBNET_STR "Your IP does not present in server's subnet"

void dump_URL(pcstr p, IDirectPlay8Address* A);

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
    m_pimpl(xr_new<ClientStatisticImpl>()) {}

IClientStatistic::IClientStatistic(CTimer* timer)
{
    ZeroMemory(this, sizeof(*this));
    m_pimpl = xr_new<ClientStatisticImpl>();
    m_pimpl->device_timer = timer;
    m_pimpl->dwBaseTime = TimeGlobal(m_pimpl->device_timer);
}

IClientStatistic::IClientStatistic(const IClientStatistic& rhs) :
    m_pimpl(xr_new<ClientStatisticImpl>())
{
    *m_pimpl = *rhs.m_pimpl;
}

IClientStatistic::~IClientStatistic()
{
    xr_delete(m_pimpl);
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

static HRESULT WINAPI Handler(PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage)
{
    IPureServer* C = (IPureServer*)pvUserContext;
    return C->net_Handler(dwMessageType, pMessage);
}

//------------------------------------------------------------------------------

void IClient::_SendTo_LL(const void* data, u32 size, u32 flags, u32 timeout)
{
    R_ASSERT(server);
    server->IPureServer::SendTo_LL(ID, const_cast<void*>(data), size, flags, timeout);
}

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

void IPureServer::_Recieve(const void* data, u32 data_size, u32 param)
{
    if (data_size >= NET_PacketSizeLimit)
    {
        Msg("! too large packet size[%d] received, DoS attack?", data_size);
        return;
    }

    NET_Packet packet;
    ClientID id;

    id.set(param);
    packet.construct(data, data_size);
    // u32 currentThreadId = Threading::GetCurrThreadId();
    // Msg("-S- Entering to csMessages from _Receive [%d]", currentThreadId);
    csMessage.Enter();
    // LogStackTrace(
    //		make_string("-S- Entered to csMessages [%d]", currentThreadId).c_str());
    //---------------------------------------
    if (psNET_Flags.test(NETFLAG_LOG_SV_PACKETS))
    {
        if (!pSvNetLog)
            pSvNetLog = xr_new<INetLog>("logs\\net_sv_log.log", TimeGlobal(device_timer));

        if (pSvNetLog)
            pSvNetLog->LogPacket(TimeGlobal(device_timer), &packet, true);
    }
    //---------------------------------------
    u32 result = OnMessage(packet, id);
    // Msg("-S- Leaving from csMessages [%d]", currentThreadId);
    csMessage.Leave();

    if (result)
        SendBroadcast(id, packet, result);
}

//==============================================================================
#ifdef CONFIG_PROFILE_LOCKS
IPureServer::IPureServer(CTimer* timer, bool Dedicated) : m_bDedicated(Dedicated),
                                                          csPlayers(MUTEX_PROFILE_ID(IPureServer::csPlayers)),
                                                          csMessage(MUTEX_PROFILE_ID(csMessage))
#else
IPureServer::IPureServer(CTimer* timer, bool Dedicated) : m_bDedicated(Dedicated)
#endif
{
    device_timer = timer;
    stats.clear();
    stats.dwSendTime = TimeGlobal(device_timer);
    SV_Client = nullptr;
    NET = nullptr;
    net_Address_device = nullptr;
    pSvNetLog = nullptr; // new INetLog("logs//net_sv_log.log", TimeGlobal(device_timer));
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

    xr_delete(pSvNetLog);

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

    //-------------------------------------------------------------------
    bool bPortWasSet = false;
    u32 dwServerPort = START_PORT_LAN_SV;
    if (strstr(options, "portsv="))
    {
        const char* ServerPort = strstr(options, "portsv=") + 7;
        string64 tmpStr = "";
        if (strchr(ServerPort, '/'))
            strncpy_s(tmpStr, ServerPort, strchr(ServerPort, '/') - ServerPort);
        else
            strncpy_s(tmpStr, ServerPort, 63);
        dwServerPort = atol(tmpStr);
        clamp(dwServerPort, u32(START_PORT), u32(END_PORT));
        bPortWasSet = true; // this is not casual game
    }
    //-------------------------------------------------------------------

    if (!psNET_direct_connect)
    {
        //---------------------------
#ifdef DEBUG
        string1024 tmp;
#endif // DEBUG
        //HRESULT CoInitializeExRes = CoInitializeEx(nullptr, 0);
        //if (CoInitializeExRes != S_OK && CoInitializeExRes != S_FALSE)
        //{
        //    DXTRACE_ERR(tmp, CoInitializeExRes);
        //    CHK_DX(CoInitializeExRes);
        //}
        //---------------------------
        // Create the IDirectPlay8Client object.
        HRESULT CoCreateInstanceRes = CoCreateInstance(
            CLSID_DirectPlay8Server, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Server, (LPVOID*)&NET);
        //---------------------------
        if (CoCreateInstanceRes != S_OK)
        {
            DXTRACE_ERR(tmp, CoCreateInstanceRes);
            CHK_DX(CoCreateInstanceRes);
        }
        //---------------------------

        // Initialize IDirectPlay8Client object.
#ifdef DEBUG
        CHK_DX(NET->Initialize(this, Handler, 0));
#else
        CHK_DX(NET->Initialize(this, Handler, DPNINITIALIZE_DISABLEPARAMVAL));
#endif

        bool bSimulator = false;
        if (strstr(Core.Params, "-netsim"))
            bSimulator = true;

        //dump_URL("! sv ", net_Address_device);

        // Set server-player info
        DPN_APPLICATION_DESC dpAppDesc;
        DPN_PLAYER_INFO dpPlayerInfo;
        WCHAR wszName[] = L"XRay Server";

        ZeroMemory(&dpPlayerInfo, sizeof(DPN_PLAYER_INFO));
        dpPlayerInfo.dwSize = sizeof(DPN_PLAYER_INFO);
        dpPlayerInfo.dwInfoFlags = DPNINFO_NAME;
        dpPlayerInfo.pwszName = wszName;
        dpPlayerInfo.pvData = nullptr;
        dpPlayerInfo.dwDataSize = NULL;
        dpPlayerInfo.dwPlayerFlags = 0;

        CHK_DX(NET->SetServerInfo(&dpPlayerInfo, NULL, NULL, DPNSETSERVERINFO_SYNC));

        // Set server/session description
        WCHAR SessionNameUNICODE[4096];
        CHK_DX(MultiByteToWideChar(CP_ACP, 0, session_name, -1, SessionNameUNICODE, 4096));
        // Set server/session description

        // Now set up the Application Description
        ZeroMemory(&dpAppDesc, sizeof(DPN_APPLICATION_DESC));
        dpAppDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
        dpAppDesc.dwFlags = DPNSESSION_CLIENT_SERVER | DPNSESSION_NODPNSVR;
        dpAppDesc.guidApplication = NET_GUID;
        dpAppDesc.pwszSessionName = SessionNameUNICODE;
        dpAppDesc.dwMaxPlayers = (m_bDedicated) ? (dwMaxPlayers + 2) : (dwMaxPlayers + 1);
        dpAppDesc.pvApplicationReservedData = &game_descr;
        dpAppDesc.dwApplicationReservedDataSize = sizeof(game_descr);

        WCHAR SessionPasswordUNICODE[4096];
        if (xr_strlen(password_str))
        {
            CHK_DX(MultiByteToWideChar(CP_ACP, 0, password_str, -1, SessionPasswordUNICODE, 4096));
            dpAppDesc.dwFlags |= DPNSESSION_REQUIREPASSWORD;
            dpAppDesc.pwszPassword = SessionPasswordUNICODE;
        }

        // Create our IDirectPlay8Address Device Address, --- Set the SP for our Device Address
        net_Address_device = nullptr;
        CHK_DX(CoCreateInstance(CLSID_DirectPlay8Address, nullptr, CLSCTX_INPROC_SERVER,
            IID_IDirectPlay8Address, (LPVOID*)&net_Address_device));

        CHK_DX(net_Address_device->SetSP(bSimulator ? &CLSID_NETWORKSIMULATOR_DP8SP_TCPIP : &CLSID_DP8SP_TCPIP));

        u32 dwTraversalMode = DPNA_TRAVERSALMODE_NONE;
        CHK_DX(net_Address_device->AddComponent(DPNA_KEY_TRAVERSALMODE,
            &dwTraversalMode, sizeof(dwTraversalMode), DPNA_DATATYPE_DWORD));

        HRESULT HostSuccess = S_FALSE;
        // We are now ready to host the app and will try different ports
        psNET_Port = dwServerPort;
        while (HostSuccess != S_OK)
        {
            CHK_DX(net_Address_device->AddComponent(DPNA_KEY_PORT, &psNET_Port, sizeof(psNET_Port), DPNA_DATATYPE_DWORD));

            HostSuccess = NET->Host(&dpAppDesc, // AppDesc
                &net_Address_device, 1, // Device Address
                nullptr, nullptr, // Reserved
                nullptr, // Player Context
                0); // dwFlags

            if (HostSuccess != S_OK)
            {
                //			xr_string res = xrDebug::ErrorToString(HostSuccess);
                if (bPortWasSet)
                {
                    Msg("! IPureServer : port %d is BUSY!", psNET_Port);
                    return ErrConnect;
                }
                Msg("! IPureServer : port %d is BUSY!", psNET_Port);

                psNET_Port++;
                if (psNET_Port > END_PORT_LAN)
                {
                    return ErrConnect;
                }
            }
            else
            {
                Msg("- IPureServer : created on port %d!", psNET_Port);
            }
        }

        CHK_DX(HostSuccess);
    } // psNET_direct_connect

    //config_Load();

    if (!psNET_direct_connect)
    {
        BannedList_Load();
        IpList_Load();
    }

    return ErrNoError;
}

void IPureServer::Disconnect()
{
    //.	config_Save		();

    if (!psNET_direct_connect)
    {
        BannedList_Save();
        IpList_Unload();
    }

    if (NET)
        NET->Close(0);

    // Release interfaces
    _RELEASE(net_Address_device);
    _RELEASE(NET);
}

HRESULT IPureServer::net_Handler(u32 dwMessageType, PVOID pMessage)
{
    // HRESULT     hr = S_OK;

    switch (dwMessageType)
    {
    case DPN_MSGID_ENUM_HOSTS_QUERY:
    {
        PDPNMSG_ENUM_HOSTS_QUERY msg = PDPNMSG_ENUM_HOSTS_QUERY(pMessage);
        if (0 == msg->dwReceivedDataSize)
            return S_FALSE;
        if (!xr_stricmp((pcstr)msg->pvReceivedData, "ToConnect"))
            return S_OK;
        if (*((const GUID*)msg->pvReceivedData) != NET_GUID)
            return S_FALSE;
        if (!OnCL_QueryHost())
            return S_FALSE;
        return S_OK;
    }

    case DPN_MSGID_CREATE_PLAYER:
    {
        PDPNMSG_CREATE_PLAYER msg = PDPNMSG_CREATE_PLAYER(pMessage);
        const u32 max_size = 1024;
        char bufferData[max_size];
        DWORD bufferSize = max_size;
        ZeroMemory(bufferData, bufferSize);
        string512 res;

        // retrieve info
        DPN_PLAYER_INFO* Pinfo = (DPN_PLAYER_INFO*)bufferData;
        Pinfo->dwSize = sizeof(DPN_PLAYER_INFO);
        HRESULT _hr = NET->GetClientInfo(msg->dpnidPlayer, Pinfo, &bufferSize, 0);
        if (_hr == DPNERR_INVALIDPLAYER)
        {
            Assign_ServerType(res); // once
            break; // server player
        }

        CHK_DX(_hr);

        //string64 cname;
        //CHK_DX(WideCharToMultiByte( CP_ACP, 0, Pinfo->pwszName, -1, cname, sizeof(cname) , nullptr, nullptr ) );

        SClientConnectData cl_data;
        //xr_strcpy(cl_data.name, cname);

        if (Pinfo->pvData && Pinfo->dwDataSize == sizeof(cl_data))
        {
            cl_data = *((SClientConnectData*)Pinfo->pvData);
        }
        cl_data.clientID.set(msg->dpnidPlayer);

        new_client(&cl_data);
    }
    break;

    case DPN_MSGID_DESTROY_PLAYER:
    {
        PDPNMSG_DESTROY_PLAYER msg = PDPNMSG_DESTROY_PLAYER(pMessage);
        IClient* tmp_client =
            net_players.GetFoundClient(ClientIdSearchPredicate(static_cast<ClientID>(msg->dpnidPlayer)));
        if (tmp_client)
        {
            tmp_client->flags.bConnected = FALSE;
            tmp_client->flags.bReconnect = FALSE;
            OnCL_Disconnected(tmp_client);
            // real destroy
            client_Destroy(tmp_client);
        }
    }
    break;

    case DPN_MSGID_RECEIVE:
    {
        PDPNMSG_RECEIVE pMsg = PDPNMSG_RECEIVE(pMessage);
        void* m_data = pMsg->pReceiveData;
        u32 m_size = pMsg->dwReceiveDataSize;
        DPNID m_sender = pMsg->dpnidSender;

        MSYS_PING* m_ping = (MSYS_PING*)m_data;

        if ((m_size > 2 * sizeof(u32)) && (m_ping->sign1 == 0x12071980) && (m_ping->sign2 == 0x26111975))
        {
            // this is system message
            if (m_size == sizeof(MSYS_PING))
            {
                // ping - save server time and reply
                m_ping->dwTime_Server = TimerAsync(device_timer);
                ClientID ID;
                ID.set(m_sender);
                //IPureServer::SendTo_LL(ID, m_data, m_size, net_flags(false, false, true));
                IPureServer::SendTo_Buf(ID, m_data, m_size, net_flags(false, false, true, true));
            }
        }
        else
        {
            MultipacketReciever::RecievePacket(pMsg->pReceiveData, pMsg->dwReceiveDataSize, m_sender);
        }
    }
    break;

    case DPN_MSGID_INDICATE_CONNECT:
    {
        PDPNMSG_INDICATE_CONNECT msg = (PDPNMSG_INDICATE_CONNECT)pMessage;

        ip_address HAddr;
        GetClientAddress(msg->pAddressPlayer, HAddr);

        if (GetBannedClient(HAddr))
        {
            msg->dwReplyDataSize = sizeof(NET_BANNED_STR);
            msg->pvReplyData = (PVOID)NET_BANNED_STR;
            return S_FALSE;
        }
        // first connected client is SV_Client so if it is NULL then this server client tries to connect ;)
        if (SV_Client && !m_ip_filter.is_ip_present(HAddr.m_data.data))
        {
            msg->dwReplyDataSize = sizeof(NET_NOTFOR_SUBNET_STR);
            msg->pvReplyData = (PVOID)NET_NOTFOR_SUBNET_STR;
            return S_FALSE;
        }
    }
    break;
    }

    return S_OK;
}

void IPureServer::Flush_Clients_Buffers()
{
#if NET_LOG_PACKETS
    Msg("#flush server send-buf");
#endif

    struct LocalSenderFunctor
    {
        static void FlushBuffer(IClient* client) { client->FlushSendBuffer(0); }
    };

    net_players.ForEachClientDo(LocalSenderFunctor::FlushBuffer);
}

void IPureServer::SendTo_Buf(ClientID id, void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
    IClient* tmp_client = net_players.GetFoundClient(ClientIdSearchPredicate(id));
    VERIFY(tmp_client);
    tmp_client->SendPacket(data, size, dwFlags, dwTimeout);
}

void IPureServer::SendTo_LL(ClientID ID /*DPNID ID*/, void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
    //	if (psNET_Flags.test(NETFLAG_LOG_SV_PACKETS)) pSvNetLog->LogData(TimeGlobal(device_timer), data, size);
    if (psNET_Flags.test(NETFLAG_LOG_SV_PACKETS))
    {
        if (!pSvNetLog)
            pSvNetLog = xr_new<INetLog>("logs\\net_sv_log.log", TimeGlobal(device_timer));
        if (pSvNetLog)
            pSvNetLog->LogData(TimeGlobal(device_timer), data, size);
    }

    // send it
    DPN_BUFFER_DESC desc;
    desc.dwBufferSize = size;
    desc.pBufferData = (u8*)(data);

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
            : m_owner(owner), m_data(data), m_size(size), m_dwFlags(dwFlags) {}

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
    /*
    u16 m_type;
    P.r_begin(m_type);
    switch (m_type)
    {
    case M_CHAT:
    {
        string256 buffer;
        P.r_string(buffer);
        printf("RECEIVE: %s\n",buffer);
    }
    break;
    }
    */

    return 0;
}

void IPureServer::OnCL_Connected(IClient* CL) { Msg("* Player 0x%08x connected.\n", CL->ID.value()); }
void IPureServer::OnCL_Disconnected(IClient* CL) { Msg("* Player 0x%08x disconnected.\n", CL->ID.value()); }

bool IPureServer::HasBandwidth(IClient* C)
{
    u32 dwTime = TimeGlobal(device_timer);
    u32 dwInterval = 0;

    if (psNET_direct_connect)
    {
        UpdateClientStatistic(C);
        C->dwTime_LastUpdate = dwTime;
        dwInterval = 1000;
        return true;
    }

    if (psNET_ServerUpdate != 0)
        dwInterval = 1000 / psNET_ServerUpdate;
    if (psNET_Flags.test(NETFLAG_MINIMIZEUPDATES))
        dwInterval = 1000; // approx 2 times per second

    HRESULT hr;
    if (psNET_ServerUpdate != 0 && (dwTime - C->dwTime_LastUpdate) > dwInterval)
    {
        // check queue for "empty" state
        DWORD dwPending;
        hr = NET->GetSendQueueInfo(C->ID.value(), &dwPending, nullptr, 0);
        if (FAILED(hr))
            return false;

        if (dwPending > u32(psNET_ServerPending))
        {
            C->stats.dwTimesBlocked++;
            return false;
        }

        UpdateClientStatistic(C);
        // ok
        C->dwTime_LastUpdate = dwTime;
        return true;
    }
    return false;
}

void IPureServer::UpdateClientStatistic(IClient* C)
{
    // Query network statistic for this client
    DPN_CONNECTION_INFO CI;
    ZeroMemory(&CI, sizeof(CI));
    CI.dwSize = sizeof(CI);
    if (!psNET_direct_connect)
    {
        HRESULT hr = NET->GetConnectionInfo(C->ID.value(), &CI, 0);
        if (FAILED(hr))
            return;
    }
    C->stats.Update(CI);
}

void IPureServer::ClearStatistic()
{
    stats.clear();
    struct StatsClearFunctor
    {
        static void Clear(IClient* client) { client->stats.Clear(); }
    };
    net_players.ForEachClientDo(StatsClearFunctor::Clear);
}

/*
bool IPureServer::DisconnectClient(IClient* C)
{
    if (!C)
        return false;

    string64 Reason = "st_kicked_by_server";
    HRESULT res = NET->DestroyClient(C->ID.value(), Reason, xr_strlen(Reason) + 1, 0);
    CHK_DX(res);
    return true;
}
*/

bool IPureServer::DisconnectClient(IClient* C, pcstr Reason)
{
    if (!C)
        return false;

    HRESULT res = NET->DestroyClient(C->ID.value(), Reason, xr_strlen(Reason) + 1, 0);
    CHK_DX(res);
    return true;
}

bool IPureServer::DisconnectAddress(const ip_address& Address, pcstr reason)
{
    u32 players_count = net_players.ClientsCount();
    buffer_vector<IClient*> PlayersToDisconnect(xr_alloca(players_count * sizeof(IClient*)), players_count);

    struct ToDisconnectFillerFunctor
    {
        IPureServer* m_owner;
        buffer_vector<IClient*>* dest;
        ip_address const* address_to_disconnect;

        ToDisconnectFillerFunctor(
            IPureServer* owner, buffer_vector<IClient*>* dest_disconnect, ip_address const* address)
            : m_owner(owner), dest(dest_disconnect), address_to_disconnect(address) {}

        void operator()(IClient* client)
        {
            ip_address tmp_address;
            m_owner->GetClientAddress(client->ID, tmp_address);
            if (*address_to_disconnect == tmp_address)
            {
                dest->push_back(client);
            }
        }
    };

    ToDisconnectFillerFunctor tmp_functor(this, &PlayersToDisconnect, &Address);
    net_players.ForEachClientDo(tmp_functor);

    buffer_vector<IClient*>::iterator it = PlayersToDisconnect.begin();
    buffer_vector<IClient*>::iterator it_e = PlayersToDisconnect.end();

    for (; it != it_e; ++it)
    {
        DisconnectClient(*it, reason);
    }
    return true;
}

bool IPureServer::GetClientAddress(IDirectPlay8Address* pClientAddress, ip_address& Address, DWORD* pPort)
{
    WCHAR wstrHostname[256] = {0};
    DWORD dwSize = sizeof(wstrHostname);
    DWORD dwDataType = 0;
    CHK_DX(pClientAddress->GetComponentByName(DPNA_KEY_HOSTNAME, wstrHostname, &dwSize, &dwDataType));

    string256 HostName;
    CHK_DX(WideCharToMultiByte(CP_ACP, 0, wstrHostname, -1, HostName, sizeof(HostName), nullptr, nullptr));

    Address.set(HostName);

    if (pPort != nullptr)
    {
        u32 dwPort = 0;
        DWORD dwPortSize = sizeof(dwPort);
        DWORD dwPortDataType = DPNA_DATATYPE_DWORD;
        CHK_DX(pClientAddress->GetComponentByName(DPNA_KEY_PORT, &dwPort, &dwPortSize, &dwPortDataType));
        *pPort = dwPort;
    }

    return true;
}

bool IPureServer::GetClientAddress(ClientID ID, ip_address& Address, DWORD* pPort)
{
    IDirectPlay8Address* pClAddr = nullptr;
    CHK_DX(NET->GetClientAddress(ID.value(), &pClAddr, 0));

    return GetClientAddress(pClAddr, Address, pPort);
}

IBannedClient* IPureServer::GetBannedClient(const ip_address& Address)
{
    for (u32 it = 0; it < BannedAddresses.size(); it++)
    {
        IBannedClient* pBClient = BannedAddresses[it];
        if (pBClient->HAddr == Address)
            return pBClient;
    }
    return nullptr;
}

void IPureServer::BanClient(IClient* C, u32 BanTime)
{
    ip_address ClAddress;
    GetClientAddress(C->ID, ClAddress);
    BanAddress(ClAddress, BanTime);
}

void IPureServer::BanAddress(const ip_address& Address, u32 BanTimeSec)
{
    if (GetBannedClient(Address))
    {
        Msg("Already banned\n");
        return;
    }

    IBannedClient* pNewClient = xr_new<IBannedClient>();
    pNewClient->HAddr = Address;
    time(&pNewClient->BanTime);
    pNewClient->BanTime += BanTimeSec;
    if (pNewClient)
    {
        BannedAddresses.push_back(pNewClient);
        BannedList_Save();
    }
}

void IPureServer::UnBanAddress(const ip_address& Address)
{
    if (!GetBannedClient(Address))
    {
        Msg("! Can't find address %s in ban list.", Address.to_string().c_str());
        return;
    };

    for (u32 it = 0; it < BannedAddresses.size(); it++)
    {
        IBannedClient* pBClient = BannedAddresses[it];
        if (pBClient->HAddr == Address)
        {
            xr_delete(BannedAddresses[it]);
            BannedAddresses.erase(BannedAddresses.begin() + it);
            Msg("Unbanning %s", Address.to_string().c_str());
            BannedList_Save();
            break;
        }
    }
}

void IPureServer::Print_Banned_Addreses()
{
    Msg("- ----banned ip list begin-------");
    for (u32 i = 0; i < BannedAddresses.size(); i++)
    {
        IBannedClient* pBClient = BannedAddresses[i];
        Msg("- %s to %s", pBClient->HAddr.to_string().c_str(), pBClient->BannedTimeTo().c_str());
    }
    Msg("- ----banned ip list end-------");
}

void IPureServer::BannedList_Save()
{
    string_path temp;
    FS.update_path(temp, "$app_data_root$", GetBannedListName());

    CInifile ini(temp, false, false, true);

    for (u32 it = 0; it < BannedAddresses.size(); it++)
    {
        IBannedClient* cl = BannedAddresses[it];
        cl->Save(ini);
    }
}

void IPureServer::BannedList_Load()
{
    string_path temp;
    FS.update_path(temp, "$app_data_root$", GetBannedListName());

    CInifile ini(temp);

    auto it = ini.sections().begin();
    auto it_e = ini.sections().end();

    for (; it != it_e; ++it)
    {
        const shared_str& sect_name = (*it)->Name;
        IBannedClient* Cl = xr_new<IBannedClient>();
        Cl->Load(ini, sect_name);
        BannedAddresses.push_back(Cl);
    }
}

void IPureServer::IpList_Load()
{
    Msg("* Initializing IP filter.");
    m_ip_filter.load();
}

void IPureServer::IpList_Unload()
{
    Msg("* Deinitializing IP filter.");
    m_ip_filter.unload();
}

bool IPureServer::IsPlayerIPDenied(u32 ip_address)
{
    return !m_ip_filter.is_ip_present(ip_address);
}

bool banned_client_comparer(IBannedClient* C1, IBannedClient* C2)
{
    return C1->BanTime > C2->BanTime;
}

void IPureServer::UpdateBannedList()
{
    if (!BannedAddresses.size())
        return;
    std::sort(BannedAddresses.begin(), BannedAddresses.end(), banned_client_comparer);
    time_t T;
    time(&T);

    IBannedClient* Cl = BannedAddresses.back();
    if (Cl->BanTime < T)
    {
        ip_address Address = Cl->HAddr;
        UnBanAddress(Address);
    }
}

pcstr IPureServer::GetBannedListName() const { return "banned_list_ip.ltx"; }
