#include "stdafx.h"
#include "NET_Common.h"
#include "NET_Client.h"
#include "NET_Server.h"
#include "NET_Messages.h"
#include "NET_Log.h"
#include "xrCore/Threading/Lock.hpp"

#include "xrGameSpy/xrGameSpy_MainDefs.h"

#include "SDL.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#include "xrCore/Debug/dxerr.h"

//#pragma warning(pop)

// {0218FA8B-515B-4bf2-9A5F-2F079D1759F3}
static const GUID NET_GUID = {0x218fa8b, 0x515b, 0x4bf2, {0x9a, 0x5f, 0x2f, 0x7, 0x9d, 0x17, 0x59, 0xf3}};

// {8D3F9E5E-A3BD-475b-9E49-B0E77139143C}
static const GUID CLSID_NETWORKSIMULATOR_DP8SP_TCPIP = {
    0x8d3f9e5e, 0xa3bd, 0x475b, {0x9e, 0x49, 0xb0, 0xe7, 0x71, 0x39, 0x14, 0x3c}};

const GUID CLSID_DirectPlay8Client = {0x743f1dc6, 0x5aba, 0x429f, {0x8b, 0xdf, 0xc5, 0x4d, 0x03, 0x25, 0x3d, 0xc2}};

// {DA825E1B-6830-43d7-835D-0B5AD82956A2}
const GUID CLSID_DirectPlay8Server = {0xda825e1b, 0x6830, 0x43d7, {0x83, 0x5d, 0x0b, 0x5a, 0xd8, 0x29, 0x56, 0xa2}};

// {286F484D-375E-4458-A272-B138E2F80A6A}
const GUID CLSID_DirectPlay8Peer = {0x286f484d, 0x375e, 0x4458, {0xa2, 0x72, 0xb1, 0x38, 0xe2, 0xf8, 0x0a, 0x6a}};

// CLSIDs added for DirectX 9

// {FC47060E-6153-4b34-B975-8E4121EB7F3C}
const GUID CLSID_DirectPlay8ThreadPool = {0xfc47060e, 0x6153, 0x4b34, {0xb9, 0x75, 0x8e, 0x41, 0x21, 0xeb, 0x7f, 0x3c}};

// {E4C1D9A2-CBF7-48bd-9A69-34A55E0D8941}
const GUID CLSID_DirectPlay8NATResolver = {
    0xe4c1d9a2, 0xcbf7, 0x48bd, {0x9a, 0x69, 0x34, 0xa5, 0x5e, 0x0d, 0x89, 0x41}};

/****************************************************************************
 *
 * DirectPlay8 Interface IIDs
 *
 ****************************************************************************/

typedef REFIID DP8REFIID;

// {5102DACD-241B-11d3-AEA7-006097B01411}
const GUID IID_IDirectPlay8Client = {0x5102dacd, 0x241b, 0x11d3, {0xae, 0xa7, 0x00, 0x60, 0x97, 0xb0, 0x14, 0x11}};

// {5102DACE-241B-11d3-AEA7-006097B01411}
const GUID IID_IDirectPlay8Server = {0x5102dace, 0x241b, 0x11d3, {0xae, 0xa7, 0x00, 0x60, 0x97, 0xb0, 0x14, 0x11}};

// {5102DACF-241B-11d3-AEA7-006097B01411}
const GUID IID_IDirectPlay8Peer = {0x5102dacf, 0x241b, 0x11d3, {0xae, 0xa7, 0x00, 0x60, 0x97, 0xb0, 0x14, 0x11}};

// IIDs added for DirectX 9

// {0D22EE73-4A46-4a0d-89B2-045B4D666425}
const GUID IID_IDirectPlay8ThreadPool = {0xd22ee73, 0x4a46, 0x4a0d, {0x89, 0xb2, 0x04, 0x5b, 0x4d, 0x66, 0x64, 0x25}};

// {A9E213F2-9A60-486f-BF3B-53408B6D1CBB}
const GUID IID_IDirectPlay8NATResolver = {0xa9e213f2, 0x9a60, 0x486f, {0xbf, 0x3b, 0x53, 0x40, 0x8b, 0x6d, 0x1c, 0xbb}};

// {53934290-628D-11D2-AE0F-006097B01411}
const GUID CLSID_DP8SP_IPX = {0x53934290, 0x628d, 0x11d2, {0xae, 0x0f, 0x00, 0x60, 0x97, 0xb0, 0x14, 0x11}};

// {6D4A3650-628D-11D2-AE0F-006097B01411}
const GUID CLSID_DP8SP_MODEM = {0x6d4a3650, 0x628d, 0x11d2, {0xae, 0x0f, 0x00, 0x60, 0x97, 0xb0, 0x14, 0x11}};

// {743B5D60-628D-11D2-AE0F-006097B01411}
const GUID CLSID_DP8SP_SERIAL = {0x743b5d60, 0x628d, 0x11d2, {0xae, 0x0f, 0x00, 0x60, 0x97, 0xb0, 0x14, 0x11}};

// {EBFE7BA0-628D-11D2-AE0F-006097B01411}
const GUID CLSID_DP8SP_TCPIP = {0xebfe7ba0, 0x628d, 0x11d2, {0xae, 0x0f, 0x00, 0x60, 0x97, 0xb0, 0x14, 0x11}};

// Service providers added for DirectX 9

// {995513AF-3027-4b9a-956E-C772B3F78006}
const GUID CLSID_DP8SP_BLUETOOTH = {0x995513af, 0x3027, 0x4b9a, {0x95, 0x6e, 0xc7, 0x72, 0xb3, 0xf7, 0x80, 0x06}};

const GUID CLSID_DirectPlay8Address = {0x934a9523, 0xa3ca, 0x4bc5, {0xad, 0xa0, 0xd6, 0xd9, 0x5d, 0x97, 0x94, 0x21}};

const GUID IID_IDirectPlay8Address = {0x83783300, 0x4063, 0x4c8a, {0x9d, 0xb3, 0x82, 0x83, 0x0a, 0x7f, 0xeb, 0x31}};

static INetLog* pClNetLog = nullptr;

void dump_URL(pcstr p, IDirectPlay8Address* A)
{
    string256 aaaa;
    DWORD aaaa_s = sizeof(aaaa);
    R_CHK(A->GetURLA(aaaa, &aaaa_s));
    Log(p, aaaa);
}

#ifdef CONFIG_PROFILE_LOCKS
INetQueue::INetQueue() : pcs(new Lock(MUTEX_PROFILE_ID(INetQueue)))
#else
INetQueue::INetQueue() : pcs(new Lock)
#endif
{
    unused.reserve(128);
    for (int i = 0; i < 16; i++)
        unused.push_back(new NET_Packet());
}

INetQueue::~INetQueue()
{
    pcs->Enter();
    u32 it;
    for (it = 0; it < unused.size(); it++)
        xr_delete(unused[it]);
    for (it = 0; it < ready.size(); it++)
        xr_delete(ready[it]);
    pcs->Leave();
    delete pcs;
}

static u32 LastTimeCreate = 0;

NET_Packet* INetQueue::Create()
{
    NET_Packet* P = nullptr;
    //pcs->Enter();
//#ifdef _DEBUG
    //Msg("- INetQueue::Create - ready %d, unused %d", ready.size(), unused.size());
//#endif
    if (unused.empty())
    {
        ready.push_back(new NET_Packet());
        P = ready.back();
        //---------------------------------------------
        LastTimeCreate = SDL_GetTicks();
        //---------------------------------------------
    }
    else
    {
        ready.push_back(unused.back());
        unused.pop_back();
        P = ready.back();
    }
    //pcs->Leave();
    return P;
}

NET_Packet* INetQueue::Create(const NET_Packet& _other)
{
    NET_Packet* P = nullptr;
    pcs->Enter();
//#ifdef _DEBUG
    //Msg("- INetQueue::Create - ready %d, unused %d", ready.size(), unused.size());
//#endif
    if (unused.empty())
    {
        ready.push_back(new NET_Packet());
        P = ready.back();
        //---------------------------------------------
        LastTimeCreate = SDL_GetTicks();
        //---------------------------------------------
    }
    else
    {
        ready.push_back(unused.back());
        unused.pop_back();
        P = ready.back();
    }
    CopyMemory(P, &_other, sizeof(NET_Packet));
    pcs->Leave();
    return P;
}

NET_Packet* INetQueue::Retreive()
{
    NET_Packet* P = nullptr;
    //pcs->Enter();
//#ifdef _DEBUG
    //Msg("INetQueue::Retreive - ready %d, unused %d", ready.size(), unused.size());
//#endif
    if (!ready.empty())
        P = ready.front();
    //---------------------------------------------
    else
    {
        u32 tmp_time = SDL_GetTicks() - 60000;
        u32 size = unused.size();
        if ((LastTimeCreate < tmp_time) && (size > 32))
        {
            xr_delete(unused.back());
            unused.pop_back();
        }
    }
    //---------------------------------------------
    //pcs->Leave();
    return P;
}

void INetQueue::Release()
{
    //pcs->Enter();
//#ifdef _DEBUG
    //Msg("INetQueue::Release - ready %d, unused %d", ready.size(), unused.size());
//#endif
    VERIFY(!ready.empty());
    //---------------------------------------------
    u32 tmp_time = SDL_GetTicks() - 60000;
    u32 size = unused.size();
    ready.front()->B.count = 0;
    if ((LastTimeCreate < tmp_time) && (size > 32))
    {
        xr_delete(ready.front());
    }
    else
        unused.push_back(ready.front());
    //---------------------------------------------
    ready.pop_front();
    //pcs->Leave();
}

void INetQueue::LockQ()
{
    pcs->Enter();
}

void INetQueue::UnlockQ()
{
    pcs->Leave();
}

const u32 syncQueueSize = 512;
const int syncSamples = 256;

class XRNETSERVER_API syncQueue
{
    u32 table[syncQueueSize];
    u32 write;
    u32 count;

public:
    syncQueue() { clear(); }

    void push(u32 value)
    {
        table[write++] = value;
        if (write == syncQueueSize)
            write = 0;

        if (count <= syncQueueSize)
            count++;
    }

    u32* begin() { return table; }
    u32* end() { return table + count; }
    u32 size() const { return count; }

    void clear()
    {
        write = 0;
        count = 0;
    }
} net_DeltaArray;

//-------
XRNETSERVER_API Flags32 psNET_Flags = {0};
XRNETSERVER_API int psNET_ClientUpdate = 30; // FPS
XRNETSERVER_API int psNET_ClientPending = 2;
XRNETSERVER_API char psNET_Name[32] = "Player";
XRNETSERVER_API bool psNET_direct_connect = false;

/****************************************************************************
 *
 * DirectPlay8 Service Provider GUIDs
 *
 ****************************************************************************/

static HRESULT WINAPI Handler(PVOID pvUserContext, DWORD dwMessageType, PVOID pMessage)
{
    IPureClient* C = (IPureClient*)pvUserContext;
    return C->net_Handler(dwMessageType, pMessage);
}

//------------------------------------------------------------------------------

void IPureClient::_SendTo_LL(const void* data, u32 size, u32 flags, u32 timeout)
{
    IPureClient::SendTo_LL(const_cast<void*>(data), size, flags, timeout);
}

//------------------------------------------------------------------------------

void IPureClient::_Recieve(const void* data, u32 data_size, u32 /*param*/)
{
    MSYS_PING* cfg = (MSYS_PING*)data;
    net_Statistic.dwBytesReceived += data_size;

    if ((data_size >= 2 * sizeof(u32)) && (cfg->sign1 == 0x12071980) && (cfg->sign2 == 0x26111975))
    {
        // Internal system message
        if (data_size == sizeof(MSYS_PING))
        {
            // It is reverted(server) ping
            u32 time = TimerAsync(device_timer);
            u32 ping = time - (cfg->dwTime_ClientSend);
            u32 delta = cfg->dwTime_Server + ping / 2 - time;

            net_DeltaArray.push(delta);
            Sync_Average();
            return;
        }

        if (data_size == sizeof(MSYS_CONFIG))
        {
            net_Connected = EnmConnectionCompleted;
            return;
        }
        Msg("! Unknown system message");
    }
    if (net_Connected == EnmConnectionCompleted)
    {
        // one of the messages - decompress it

        if (psNET_Flags.test(NETFLAG_LOG_CL_PACKETS))
        {
            if (!pClNetLog)
                pClNetLog = new INetLog("logs//net_cl_log.log", timeServer());

            if (pClNetLog)
                pClNetLog->LogData(timeServer(), const_cast<void*>(data), data_size, true);
        }

        OnMessage(const_cast<void*>(data), data_size);
    }
}

//==============================================================================
#ifdef CONFIG_PROFILE_LOCKS
IPureClient::IPureClient(CTimer* timer) : net_Statistic(timer), net_csEnumeration(new Lock(MUTEX_PROFILE_ID(IPureClient::net_csEnumeration)))
#else
IPureClient::IPureClient(CTimer* timer) : net_Statistic(timer), net_csEnumeration(new Lock)
#endif
{
    NET = nullptr;
    net_Address_server = nullptr;
    net_Address_device = nullptr;
    device_timer = timer;
    net_TimeDelta_User = 0;
    net_Time_LastUpdate = 0;
    net_TimeDelta = 0;
    net_TimeDelta_Calculated = 0;

    pClNetLog = nullptr; // new INetLog("logs//net_cl_log.log", timeServer());
}

IPureClient::~IPureClient()
{
    xr_delete(pClNetLog);
    pClNetLog = nullptr;
    psNET_direct_connect = false;
    delete net_csEnumeration;
}

bool IPureClient::Connect(pcstr options)
{
    R_ASSERT(options);
    net_Disconnected = false;

    if (!psNET_direct_connect)
    {
        string256 server_name = "";
        //xr_strcpy(server_name,options);
        if (strchr(options, '/'))
            strncpy_s(server_name, options, strchr(options, '/') - options);
        if (strchr(server_name, '/'))
            *strchr(server_name, '/') = 0;

        string64 password_str = "";
        if (strstr(options, "psw="))
        {
            const char* PSW = strstr(options, "psw=") + 4;
            if (strchr(PSW, '/'))
                strncpy_s(password_str, PSW, strchr(PSW, '/') - PSW);
            else
                xr_strcpy(password_str, PSW);
        }

        string64 user_name_str = "";
        if (strstr(options, "name="))
        {
            const char* NM = strstr(options, "name=") + 5;
            if (strchr(NM, '/'))
                strncpy_s(user_name_str, NM, strchr(NM, '/') - NM);
            else
                xr_strcpy(user_name_str, NM);
        }

        string64 user_pass = "";
        if (strstr(options, "pass="))
        {
            const char* UP = strstr(options, "pass=") + 5;
            if (strchr(UP, '/'))
                strncpy_s(user_pass, UP, strchr(UP, '/') - UP);
            else
                xr_strcpy(user_pass, UP);
        }

        int psSV_Port = START_PORT_LAN_SV;
        if (strstr(options, "port="))
        {
            string64 portstr;
            xr_strcpy(portstr, strstr(options, "port=") + 5);
            if (strchr(portstr, '/'))
                *strchr(portstr, '/') = 0;
            psSV_Port = atol(portstr);
            clamp(psSV_Port, int(START_PORT), int(END_PORT));
        }

        bool bPortWasSet = false;
        int psCL_Port = START_PORT_LAN_CL;
        if (strstr(options, "portcl="))
        {
            string64 portstr;
            xr_strcpy(portstr, strstr(options, "portcl=") + 7);
            if (strchr(portstr, '/'))
                *strchr(portstr, '/') = 0;
            psCL_Port = atol(portstr);
            clamp(psCL_Port, int(START_PORT), int(END_PORT));
            bPortWasSet = true;
        }
        //Msg("* Client connect on port %d\n", psNET_Port);

        //
        net_Connected = EnmConnectionWait;
        net_Syncronised = false;
        net_Disconnected = false;

        //---------------------------
        string1024 tmp = "";
        //HRESULT CoInitializeExRes = CoInitializeEx(nullptr, 0);
        //if (CoInitializeExRes != S_OK && CoInitializeExRes != S_FALSE)
        //{
        //    DXTRACE_ERR(tmp, CoInitializeExRes);
        //    CHK_DX(CoInitializeExRes);
        //}
        //---------------------------
        // Create the IDirectPlay8Client object.
        HRESULT CoCreateInstanceRes = CoCreateInstance(
            CLSID_DirectPlay8Client, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Client, (LPVOID*)&NET);
        //---------------------------
        if (CoCreateInstanceRes != S_OK)
        {
            DXTRACE_ERR(tmp, CoCreateInstanceRes);
            CHK_DX(CoCreateInstanceRes);
        }
        //---------------------------

        // Initialize IDirectPlay8Client object.
#ifdef DEBUG
        R_CHK(NET->Initialize(this, Handler, 0));
#else
        R_CHK(NET->Initialize(this, Handler, DPNINITIALIZE_DISABLEPARAMVAL));
#endif
        bool bSimulator = false;
        if (strstr(Core.Params, "-netsim"))
            bSimulator = true;

        // Create our IDirectPlay8Address Device Address, --- Set the SP for our Device Address
        net_Address_device = nullptr;
        R_CHK(CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address,
            (LPVOID*)&net_Address_device));
            R_CHK(net_Address_device->SetSP(bSimulator ? &CLSID_NETWORKSIMULATOR_DP8SP_TCPIP : &CLSID_DP8SP_TCPIP));

            // Create our IDirectPlay8Address Server Address, --- Set the SP for our Server Address
            WCHAR ServerNameUNICODE[256];
            R_CHK(MultiByteToWideChar(CP_ACP, 0, server_name, -1, ServerNameUNICODE, 256));

            net_Address_server = nullptr;
            R_CHK(CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay8Address,
                (LPVOID*)&net_Address_server));
                R_CHK(net_Address_server->SetSP(bSimulator ? &CLSID_NETWORKSIMULATOR_DP8SP_TCPIP : &CLSID_DP8SP_TCPIP));
                R_CHK(net_Address_server->AddComponent(
                    DPNA_KEY_HOSTNAME, ServerNameUNICODE, 2 * u32(wcslen(ServerNameUNICODE) + 1), DPNA_DATATYPE_STRING));
                R_CHK(net_Address_server->AddComponent(DPNA_KEY_PORT, &psSV_Port, sizeof(psSV_Port), DPNA_DATATYPE_DWORD));

                // Debug
                // dump_URL		("! cl ",	net_Address_device);
                // dump_URL		("! en ",	net_Address_server);

                // Now set up the Application Description
                DPN_APPLICATION_DESC dpAppDesc;
                ZeroMemory(&dpAppDesc, sizeof(DPN_APPLICATION_DESC));
                dpAppDesc.dwSize = sizeof(DPN_APPLICATION_DESC);
                dpAppDesc.guidApplication = NET_GUID;

                // Setup client info
                /*xr_strcpy( tmp, server_name );
                xr_strcat( tmp, "/name=" );
                xr_strcat( tmp, user_name_str );
                xr_strcat( tmp, "/" );*/

                WCHAR ClientNameUNICODE[256];
                R_CHK(MultiByteToWideChar(CP_ACP, 0, user_name_str, -1, ClientNameUNICODE, 256));

                {
                    DPN_PLAYER_INFO Pinfo;
                    ZeroMemory(&Pinfo, sizeof(Pinfo));
                    Pinfo.dwSize = sizeof(Pinfo);
                    Pinfo.dwInfoFlags = DPNINFO_NAME | DPNINFO_DATA;
                    Pinfo.pwszName = ClientNameUNICODE;

                    SClientConnectData cl_data;
                    cl_data.process_id = GetCurrentProcessId();
                    xr_strcpy(cl_data.name, user_name_str);
                    xr_strcpy(cl_data.pass, user_pass);

                    Pinfo.pvData = &cl_data;
                    Pinfo.dwDataSize = sizeof(cl_data);

                    R_CHK(NET->SetClientInfo(&Pinfo, nullptr, nullptr, DPNSETCLIENTINFO_SYNC));
                }
                if (xr_stricmp(server_name, "localhost") == 0)
                {
                    WCHAR SessionPasswordUNICODE[4096];
                    if (xr_strlen(password_str))
                    {
                        CHK_DX(MultiByteToWideChar(CP_ACP, 0, password_str, -1, SessionPasswordUNICODE, 4096));
                        dpAppDesc.dwFlags |= DPNSESSION_REQUIREPASSWORD;
                        dpAppDesc.pwszPassword = SessionPasswordUNICODE;
                    }

                    u32 c_port = u32(psCL_Port);
                    HRESULT res = S_FALSE;
                    while (res != S_OK)
                    {
                        R_CHK(net_Address_device->AddComponent(DPNA_KEY_PORT, &c_port, sizeof(c_port), DPNA_DATATYPE_DWORD));
                        res = NET->Connect(&dpAppDesc, // pdnAppDesc
                                           net_Address_server, // pHostAddr
                                           net_Address_device, // pDeviceInfo
                                           nullptr, // pdnSecurity
                                           nullptr, // pdnCredentials
                                           nullptr, 0, // pvUserConnectData/Size
                                           nullptr, // pvAsyncContext
                                           nullptr, // pvAsyncHandle
                                           DPNCONNECT_SYNC); // dwFlags
                        if (res != S_OK)
                        {
                            //			xr_string res = xrDebug::ErrorToString(HostSuccess);

                            if (bPortWasSet)
                            {
                                Msg("! IPureClient : port %d is BUSY!", c_port);
                                return false;
                            }
                            Msg("! IPureClient : port %d is BUSY!", c_port);

                            c_port++;
                            if (c_port > END_PORT_LAN)
                            {
                                return false;
                            }
                        }
                        else
                        {
                            Msg("- IPureClient : created on port %d!", c_port);
                        }
                    }

                    //		R_CHK(res);
                    if (res != S_OK)
                        return false;

                    // Create ONE node
                    HOST_NODE NODE;

                    // Copy the Host Address
                    R_CHK(net_Address_server->Duplicate(&NODE.pHostAddress));

                    // Retreive session name
                    char desc[4096];
                    ZeroMemory(desc, sizeof(desc));
                    DPN_APPLICATION_DESC* dpServerDesc = (DPN_APPLICATION_DESC*)desc;
                    DWORD dpServerDescSize = sizeof(desc);
                    dpServerDesc->dwSize = sizeof(DPN_APPLICATION_DESC);
                    R_CHK(NET->GetApplicationDesc(dpServerDesc, &dpServerDescSize, 0));
                    if (!dpServerDesc->dwApplicationReservedDataSize || !dpServerDesc->pvApplicationReservedData)
                    {
                        OnInvalidHost();
                        return false;
                    }
                    CopyMemory(&m_game_description, dpServerDesc->pvApplicationReservedData,
                        dpServerDesc->dwApplicationReservedDataSize);
                    if (dpServerDesc->pwszSessionName)
                    {
                        string4096 dpSessionName;
                        R_CHK(WideCharToMultiByte(
                            CP_ACP, 0, dpServerDesc->pwszSessionName, -1, dpSessionName, sizeof(dpSessionName), nullptr, nullptr));
                        NODE.dpSessionName = (pstr)&dpSessionName[0];
                    }
                    net_Hosts.push_back(NODE);
                }
                else
                {
                    string64 EnumData;
                    EnumData[0] = 0;
                    xr_strcat(EnumData, "ToConnect");
                    DWORD EnumSize = xr_strlen(EnumData) + 1;
                    // We now have the host address so lets enum
                    u32 c_port = psCL_Port;
                    HRESULT res = S_FALSE;
                    while (res != S_OK && c_port <= END_PORT)
                    {
                        R_CHK(net_Address_device->AddComponent(DPNA_KEY_PORT, &c_port, sizeof(c_port), DPNA_DATATYPE_DWORD));

                        res = NET->EnumHosts(&dpAppDesc, // pApplicationDesc - for unknown reason
                                             net_Address_server, // pdpaddrHost
                                             net_Address_device, // pdpaddrDeviceInfo
                                             EnumData, EnumSize, // pvUserEnumData, size
                                             10, // dwEnumCount
                                             1000, // dwRetryInterval
                                             1000, // dwTimeOut
                                             nullptr, // pvUserContext
                                             nullptr, // pAsyncHandle
                                             DPNENUMHOSTS_SYNC // dwFlags
                        );
                        if (res != S_OK)
                        {
                            //xr_string res = xrDebug::ErrorToString(HostSuccess);
                            switch (res)
                            {
                            case DPNERR_INVALIDHOSTADDRESS:
                            {
                                OnInvalidHost();
                                return false;
                            }

                            case DPNERR_SESSIONFULL:
                            {
                                OnSessionFull();
                                return false;
                            }

                            }

                            if (bPortWasSet)
                            {
                                Msg("! IPureClient : port %d is BUSY!", c_port);
                                return false;
                            }
#ifdef DEBUG
                            Msg("! IPureClient : port %d is BUSY!", c_port);

                            //pcstr x = DXGetErrorString9(res);
                            string1024 tmp2 = "";
                            DXTRACE_ERR(tmp2, res);
#endif
                            c_port++;
                        }
                        else
                            Msg("- IPureClient : created on port %d!", c_port);
                    }

                    // ****** Connection
                    IDirectPlay8Address* pHostAddress = nullptr;
                    if (net_Hosts.empty())
                    {
                        OnInvalidHost();
                        return false;
                    }

                    WCHAR SessionPasswordUNICODE[4096];
                    if (xr_strlen(password_str))
                    {
                        CHK_DX(MultiByteToWideChar(CP_ACP, 0, password_str, -1, SessionPasswordUNICODE, 4096));
                        dpAppDesc.dwFlags |= DPNSESSION_REQUIREPASSWORD;
                        dpAppDesc.pwszPassword = SessionPasswordUNICODE;
                    }

                    net_csEnumeration->Enter();
                    // real connect
                    for (u32 I = 0; I < net_Hosts.size(); I++)
                        Msg("* HOST #%d: %s\n", I + 1, *net_Hosts[I].dpSessionName);

                    R_CHK(net_Hosts.front().pHostAddress->Duplicate(&pHostAddress));
                    // dump_URL		("! c2s ",	pHostAddress);
                    res = NET->Connect(&dpAppDesc, // pdnAppDesc
                                       pHostAddress, // pHostAddr
                                       net_Address_device, // pDeviceInfo
                                       nullptr, // pdnSecurity
                                       nullptr, // pdnCredentials
                                       nullptr, 0, // pvUserConnectData/Size
                                       nullptr, // pvAsyncContext
                                       nullptr, // pvAsyncHandle
                                       DPNCONNECT_SYNC); // dwFlags
                    //		R_CHK(res);
                    net_csEnumeration->Leave();
                    _RELEASE(pHostAddress);
#ifdef DEBUG
                    //		const char* x = DXGetErrorString9(res);
            string1024 tmp2 = "";
            DXTRACE_ERR(tmp2, res);
#endif
                    switch (res)
                    {
                    case DPNERR_INVALIDPASSWORD:
                    {
                        OnInvalidPassword();
                    }
                        break;
                    case DPNERR_SESSIONFULL:
                    {
                        OnSessionFull();
                    }
                        break;
                    case DPNERR_CANTCREATEPLAYER:
                    {
                        Msg("! Error: Can\'t create player");
                    }
                        break;
                    }
                    if (res != S_OK)
                        return false;
                }

                // Caps
                /*
                GUID			sp_guid;
                DPN_SP_CAPS		sp_caps;
        
                net_Address_device->GetSP(&sp_guid);
                ZeroMemory		(&sp_caps,sizeof(sp_caps));
                sp_caps.dwSize	= sizeof(sp_caps);
                R_CHK			(NET->GetSPCaps(&sp_guid,&sp_caps,0));
                sp_caps.dwSystemBufferSize	= 0;
                R_CHK			(NET->SetSPCaps(&sp_guid,&sp_caps,0));
                R_CHK			(NET->GetSPCaps(&sp_guid,&sp_caps,0));
                */
                } // psNET_direct_connect
                // Sync
                net_TimeDelta = 0;
                return true;
                }

                void IPureClient::Disconnect()
                {
                    if (NET)
                        NET->Close(0);

                    // Clean up Host _list_
                    net_csEnumeration->Enter();
                    for (u32 i = 0; i < net_Hosts.size(); i++)
                    {
                        HOST_NODE& N = net_Hosts[i];
                        _RELEASE(N.pHostAddress);
                    }
                    net_Hosts.clear();
                    net_csEnumeration->Leave();

                    // Release interfaces
                    _SHOW_REF("cl_netADR_Server", net_Address_server);
                    _RELEASE(net_Address_server);
                    _SHOW_REF("cl_netADR_Device", net_Address_device);
                    _RELEASE(net_Address_device);
                    _SHOW_REF("cl_netCORE", NET);
                    _RELEASE(NET);

                    net_Connected = EnmConnectionWait;
                    net_Syncronised = false;
                }

                HRESULT IPureClient::net_Handler(u32 dwMessageType, PVOID pMessage)
                {
                    // HRESULT     hr = S_OK;

                    switch (dwMessageType)
                    {
                    case DPN_MSGID_ENUM_HOSTS_RESPONSE:
                    {
                        PDPNMSG_ENUM_HOSTS_RESPONSE pEnumHostsResponseMsg;
                        const DPN_APPLICATION_DESC* pDesc;
                        // HOST_NODE*                      pHostNode = NULL;
                        // WCHAR*                          pwszSession = NULL;

                        pEnumHostsResponseMsg = (PDPNMSG_ENUM_HOSTS_RESPONSE)pMessage;
                        pDesc = pEnumHostsResponseMsg->pApplicationDescription;

                        if (pDesc->dwApplicationReservedDataSize && pDesc->pvApplicationReservedData)
                        {
                            R_ASSERT(pDesc->dwApplicationReservedDataSize == sizeof(m_game_description));
                            CopyMemory(&m_game_description, pDesc->pvApplicationReservedData, pDesc->dwApplicationReservedDataSize);
                        }

                        // Insert each host response if it isn't already present
                        net_csEnumeration->Enter();
                        bool bHostRegistered = false;
                        for (u32 I = 0; I < net_Hosts.size(); I++)
                        {
                            HOST_NODE& N = net_Hosts[I];
                            if (pDesc->guidInstance == N.pdpAppDesc->guidInstance)
                            {
                                // This host is already in the list
                                bHostRegistered = true;
                                break;
                            }
                        }

                        if (!bHostRegistered)
                        {
                            // This host session is not in the list then so insert it.
                            HOST_NODE NODE;
                            ZeroMemory(&NODE, sizeof(HOST_NODE));

                            // Copy the Host Address
                            R_CHK(pEnumHostsResponseMsg->pAddressSender->Duplicate(&NODE.pHostAddress));
                            CopyMemory(NODE.pdpAppDesc, pDesc, sizeof(DPN_APPLICATION_DESC));

                            // Null out all the pointers we aren't copying
                            NODE.pdpAppDesc->pwszSessionName = nullptr;
                            NODE.pdpAppDesc->pwszPassword = nullptr;
                            NODE.pdpAppDesc->pvReservedData = nullptr;
                            NODE.pdpAppDesc->dwReservedDataSize = 0;
                            NODE.pdpAppDesc->pvApplicationReservedData = nullptr;
                            NODE.pdpAppDesc->dwApplicationReservedDataSize = 0;

                            if (pDesc->pwszSessionName)
                            {
                                string4096 dpSessionName;
                                R_CHK(WideCharToMultiByte(
                                    CP_ACP, 0, pDesc->pwszSessionName, -1, dpSessionName, sizeof(dpSessionName), nullptr, nullptr));
                                NODE.dpSessionName = (pstr)(&dpSessionName[0]);
                            }

                            net_Hosts.push_back(NODE);
                        }
                        net_csEnumeration->Leave();
                    }
                        break;

                    case DPN_MSGID_RECEIVE:
                    {
                        PDPNMSG_RECEIVE pMsg = (PDPNMSG_RECEIVE)pMessage;

                        RecievePacket(pMsg->pReceiveData, pMsg->dwReceiveDataSize);
                    }
                        break;
                    case DPN_MSGID_TERMINATE_SESSION:
                    {
                        PDPNMSG_TERMINATE_SESSION pMsg = (PDPNMSG_TERMINATE_SESSION)pMessage;
                        auto m_data = (pstr)pMsg->pvTerminateData;
                        u32 m_size = pMsg->dwTerminateDataSize;
                        HRESULT m_hResultCode = pMsg->hResultCode;

                        net_Disconnected = true;

                        if (m_size != 0)
                        {
                            OnSessionTerminate(m_data);
#ifdef DEBUG
                            //Msg("- Session terminated : %s", m_data);
#endif
                        }
                        else
                        {
#ifdef DEBUG
                            OnSessionTerminate((xrDebug::ErrorToString(m_hResultCode)));
                            Msg("- Session terminated : %s", (xrDebug::ErrorToString(m_hResultCode)));
#endif
                        }
                    }
                        break;
                    default:
                    {
#if 1
                        pcstr msg = "";
                        switch (dwMessageType)
                        {
                        case DPN_MSGID_ADD_PLAYER_TO_GROUP: msg = "DPN_MSGID_ADD_PLAYER_TO_GROUP";
                            break;
                        case DPN_MSGID_ASYNC_OP_COMPLETE: msg = "DPN_MSGID_ASYNC_OP_COMPLETE";
                            break;
                        case DPN_MSGID_CLIENT_INFO: msg = "DPN_MSGID_CLIENT_INFO";
                            break;
                        case DPN_MSGID_CONNECT_COMPLETE:
                        {
                            PDPNMSG_CONNECT_COMPLETE pMsg = (PDPNMSG_CONNECT_COMPLETE)pMessage;
#ifdef DEBUG
                            //pcstr x = DXGetErrorString9(pMsg->hResultCode);
                            if (pMsg->hResultCode != S_OK)
                            {
                                string1024 tmp = "";
                                DXTRACE_ERR(tmp, pMsg->hResultCode);
                            }
#endif
                            if (pMsg->dwApplicationReplyDataSize)
                            {
                                string256 ResStr = "";
                                strncpy_s(ResStr, (char*)pMsg->pvApplicationReplyData, pMsg->dwApplicationReplyDataSize);
                                Msg("Connection result : %s", ResStr);
                            }
                            else
                                msg = "DPN_MSGID_CONNECT_COMPLETE";
                        }
                            break;
                        case DPN_MSGID_CREATE_GROUP: msg = "DPN_MSGID_CREATE_GROUP";
                            break;
                        case DPN_MSGID_CREATE_PLAYER: msg = "DPN_MSGID_CREATE_PLAYER";
                            break;
                        case DPN_MSGID_DESTROY_GROUP: msg = "DPN_MSGID_DESTROY_GROUP";
                            break;
                        case DPN_MSGID_DESTROY_PLAYER: msg = "DPN_MSGID_DESTROY_PLAYER";
                            break;
                        case DPN_MSGID_ENUM_HOSTS_QUERY: msg = "DPN_MSGID_ENUM_HOSTS_QUERY";
                            break;
                        case DPN_MSGID_GROUP_INFO: msg = "DPN_MSGID_GROUP_INFO";
                            break;
                        case DPN_MSGID_HOST_MIGRATE: msg = "DPN_MSGID_HOST_MIGRATE";
                            break;
                        case DPN_MSGID_INDICATE_CONNECT: msg = "DPN_MSGID_INDICATE_CONNECT";
                            break;
                        case DPN_MSGID_INDICATED_CONNECT_ABORTED: msg = "DPN_MSGID_INDICATED_CONNECT_ABORTED";
                            break;
                        case DPN_MSGID_PEER_INFO: msg = "DPN_MSGID_PEER_INFO";
                            break;
                        case DPN_MSGID_REMOVE_PLAYER_FROM_GROUP: msg = "DPN_MSGID_REMOVE_PLAYER_FROM_GROUP";
                            break;
                        case DPN_MSGID_RETURN_BUFFER: msg = "DPN_MSGID_RETURN_BUFFER";
                            break;
                        case DPN_MSGID_SEND_COMPLETE: msg = "DPN_MSGID_SEND_COMPLETE";
                            break;
                        case DPN_MSGID_SERVER_INFO: msg = "DPN_MSGID_SERVER_INFO";
                            break;
                        case DPN_MSGID_TERMINATE_SESSION: msg = "DPN_MSGID_TERMINATE_SESSION";
                            break;
                        default: msg = "???";
                            break;
                        }
                        //Msg("! ************************************ : %s", msg);
#endif
                    }
                        break;
                    }

                    return S_OK;
                }

                void IPureClient::OnMessage(void* data, u32 size)
                {
                    // One of the messages - decompress it
                    net_Queue.LockQ();
                    NET_Packet* P = net_Queue.Create();

                    P->construct(data, size);
                    P->timeReceive = timeServer_Async(); // TimerAsync(device_timer);

                    u16 m_type;
                    P->r_begin(m_type);
                    net_Queue.UnlockQ();
                }

                void IPureClient::timeServer_Correct(u32 sv_time, u32 cl_time)
                {
                    u32 ping = net_Statistic.getPing();
                    u32 delta = sv_time + ping / 2 - cl_time;
                    net_DeltaArray.push(delta);
                    Sync_Average();
                }

                void IPureClient::SendTo_LL(void* data, u32 size, u32 dwFlags, u32 dwTimeout)
                {
                    if (net_Disconnected)
                        return;

                    if (psNET_Flags.test(NETFLAG_LOG_CL_PACKETS))
                    {
                        if (!pClNetLog)
                            pClNetLog = new INetLog("logs\\net_cl_log.log", timeServer());
                        if (pClNetLog)
                            pClNetLog->LogData(timeServer(), data, size);
                    }
                    DPN_BUFFER_DESC desc;

                    desc.dwBufferSize = size;
                    desc.pBufferData = (BYTE*)data;

                    net_Statistic.dwBytesSended += size;

                    // verify
                    VERIFY(desc.dwBufferSize);
                    VERIFY(desc.pBufferData);
                    VERIFY(NET);

                    DPNHANDLE hAsync = 0;
                    HRESULT hr = NET->Send(&desc, 1, dwTimeout, nullptr, &hAsync, dwFlags | DPNSEND_COALESCE);

                    //	Msg("- Client::SendTo_LL [%d]", size);
                    if (FAILED(hr))
                    {
                        Msg("! ERROR: Failed to send net-packet, reason: %s", xrDebug::ErrorToString(hr));
                        //pcstr x = DXGetErrorString9(hr);
                        string1024 tmp = "";
                        DXTRACE_ERR(tmp, hr);
                    }

                    //	UpdateStatistic();
                }

                void IPureClient::Send(NET_Packet& packet, u32 dwFlags, u32 dwTimeout)
                {
                    SendPacket(packet.B.data, packet.B.count, dwFlags, dwTimeout);
                }

                void IPureClient::Flush_Send_Buffer() { FlushSendBuffer(0); }

                bool IPureClient::net_HasBandwidth()
                {
                    u32 dwTime = TimeGlobal(device_timer);
                    u32 dwInterval = 0;
                    if (net_Disconnected)
                        return false;

                    if (psNET_ClientUpdate != 0)
                        dwInterval = 1000 / psNET_ClientUpdate;
                    if (psNET_Flags.test(NETFLAG_MINIMIZEUPDATES))
                        dwInterval = 1000; // approx 3 times per second

                    if (psNET_direct_connect)
                    {
                        if (0 != psNET_ClientUpdate && (dwTime - net_Time_LastUpdate) > dwInterval)
                        {
                            net_Time_LastUpdate = dwTime;
                            return true;
                        }
                        return false;
                    }
                    if (0 != psNET_ClientUpdate && (dwTime - net_Time_LastUpdate) > dwInterval)
                    {
                        HRESULT hr;
                        R_ASSERT(NET);
                        // check queue for "empty" state
                        DWORD dwPending = 0;
                        hr = NET->GetSendQueueInfo(&dwPending, nullptr, 0);
                        if (FAILED(hr))
                            return false;

                        if (dwPending > u32(psNET_ClientPending))
                        {
                            net_Statistic.dwTimesBlocked++;
                            return false;
                        }

                        UpdateStatistic();

                        // ok
                        net_Time_LastUpdate = dwTime;
                        return true;
                    }
                    return false;
                }

                void IPureClient::UpdateStatistic()
                {
                    // Query network statistic for this client
                    DPN_CONNECTION_INFO CI;
                    ZeroMemory(&CI, sizeof(CI));
                    CI.dwSize = sizeof(CI);
                    HRESULT hr = NET->GetConnectionInfo(&CI, 0);
                    if (FAILED(hr))
                        return;

                    net_Statistic.Update(CI);
                }

                void IPureClient::Sync_Thread()
                {
                    MSYS_PING clPing;

                    //***** Ping server
                    net_DeltaArray.clear();
                    R_ASSERT(NET);
                    for (; NET && !net_Disconnected;)
                    {
                        // Waiting for queue empty state
                        if (net_Syncronised)
                            break; // Sleep(2000);
                        DWORD dwPending = 0;
                        do
                        {
                            R_CHK(NET->GetSendQueueInfo(&dwPending, nullptr, 0));
                            Sleep(1);
                        }
                        while (dwPending);

                        // Construct message
                        clPing.sign1 = 0x12071980;
                        clPing.sign2 = 0x26111975;
                        clPing.dwTime_ClientSend = TimerAsync(device_timer);

                        // Send it
                        __try
                        {
                            DPN_BUFFER_DESC desc;
                            DPNHANDLE hAsync = 0;
                            desc.dwBufferSize = sizeof(clPing);
                            desc.pBufferData = LPBYTE(&clPing);
                            if (nullptr == NET || net_Disconnected)
                                break;

                            if (FAILED(NET->Send(&desc, 1, 0, nullptr, &hAsync, net_flags(false, false, true))))
                            {
                                Msg("* CLIENT: SyncThread: EXIT. (failed to send - disconnected?)");
                                break;
                            }
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            Msg("* CLIENT: SyncThread: EXIT. (failed to send - disconnected?)");
                            break;
                        }

                        // Waiting for reply-packet to arrive
                        if (!net_Syncronised)
                        {
                            u32 old_size = net_DeltaArray.size();
                            u32 timeBegin = TimerAsync(device_timer);
                            while ((net_DeltaArray.size() == old_size) && (TimerAsync(device_timer) - timeBegin < 5000))
                                Sleep(1);

                            if (net_DeltaArray.size() >= syncSamples)
                            {
                                net_Syncronised = true;
                                net_TimeDelta = net_TimeDelta_Calculated;
                                //Msg("* CL_TimeSync: DELTA: %d", net_TimeDelta);
                            }
                        }
                    }
                }

                void IPureClient::Sync_Average()
                {
                    //***** Analyze results
                    s64 summary_delta = 0;
                    s32 size = net_DeltaArray.size();
                    u32* I = net_DeltaArray.begin();
                    u32* E = I + size;
                    for (; I != E; I++)
                        summary_delta += *((int*)I);

                    s64 frac = s64(summary_delta) % s64(size);
                    if (frac < 0)
                        frac = -frac;
                    summary_delta /= s64(size);
                    if (frac > s64(size / 2))
                        summary_delta += (summary_delta < 0) ? -1 : 1;
                    net_TimeDelta_Calculated = s32(summary_delta);
                    net_TimeDelta = (net_TimeDelta * 5 + net_TimeDelta_Calculated) / 6;
                    //	Msg("* CLIENT: d(%d), dc(%d), s(%d)",net_TimeDelta,net_TimeDelta_Calculated,size);
                }

                void sync_thread(void* P)
                {
                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
                    IPureClient* C = (IPureClient*)P;
                    C->Sync_Thread();
                }

                void IPureClient::net_Syncronize()
                {
                    net_Syncronised = false;
                    net_DeltaArray.clear();
                    thread_spawn(sync_thread, "network-time-sync", 0, this);
                }

                bool IPureClient::net_isDisconnected() const { return net_Disconnected; }

                void IPureClient::ClearStatistic() { net_Statistic.Clear(); }
                bool IPureClient::net_IsSyncronised() { return net_Syncronised; }

                IPureClient::HOST_NODE::HOST_NODE() :
                    pdpAppDesc(new DPN_APPLICATION_DESC),
                    pHostAddress(nullptr) {}

                IPureClient::HOST_NODE::HOST_NODE(const HOST_NODE& rhs) :
                    pdpAppDesc(new DPN_APPLICATION_DESC)
                {
                    *pdpAppDesc = *rhs.pdpAppDesc;
                    pHostAddress = rhs.pHostAddress;
                    dpSessionName = rhs.dpSessionName;
                }

                IPureClient::HOST_NODE::HOST_NODE(HOST_NODE&& rhs) noexcept :
                    pdpAppDesc(rhs.pdpAppDesc)
                {
                    pHostAddress = rhs.pHostAddress;
                    dpSessionName.swap(rhs.dpSessionName);
                    rhs.pdpAppDesc = nullptr;
                    rhs.pHostAddress = nullptr;
                }

                IPureClient::HOST_NODE::~HOST_NODE() noexcept
                {
                    delete pdpAppDesc;
                }

#include <WINSOCK2.H>
#include <Ws2tcpip.h>

                bool IPureClient::GetServerAddress(ip_address& pAddress, DWORD* pPort)
                {
                    *pPort = 0;
                    if (!net_Address_server)
                        return false;

                    WCHAR wstrHostname[2048] = {0};
                    DWORD dwHostNameSize = sizeof(wstrHostname);
                    DWORD dwHostNameDataType = DPNA_DATATYPE_STRING;
                    CHK_DX(
                        net_Address_server->GetComponentByName(DPNA_KEY_HOSTNAME, wstrHostname, &dwHostNameSize, &dwHostNameDataType));

                    string2048 HostName;
                    CHK_DX(WideCharToMultiByte(CP_ACP, 0, wstrHostname, -1, HostName, sizeof(HostName), nullptr, nullptr));

                    hostent* pHostEnt = gethostbyname(HostName);
                    pstr localIP = inet_ntoa(*(struct in_addr*)*pHostEnt->h_addr_list);
                    pHostEnt = gethostbyname(pHostEnt->h_name);
                    localIP = inet_ntoa(*(struct in_addr*)*pHostEnt->h_addr_list);
                    pAddress.set(localIP);

                    //pAddress[0] = (char)(*(struct in_addr *)*pHostEnt->h_addr_list).s_net;
                    //pAddress[1] = (char)(*(struct in_addr *)*pHostEnt->h_addr_list).s_host;
                    //pAddress[2] = (char)(*(struct in_addr *)*pHostEnt->h_addr_list).s_lh;
                    //pAddress[3] = (char)(*(struct in_addr *)*pHostEnt->h_addr_list).s_impno;

                    DWORD dwPort = 0;
                    DWORD dwPortSize = sizeof(dwPort);
                    DWORD dwPortDataType = DPNA_DATATYPE_DWORD;
                    CHK_DX(net_Address_server->GetComponentByName(DPNA_KEY_PORT, &dwPort, &dwPortSize, &dwPortDataType));
                    *pPort = dwPort;

                    return true;
                };
