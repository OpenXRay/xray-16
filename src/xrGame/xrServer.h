#pragma once
// xrServer.h: interface for the xrServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XRSERVER_H__65728A25_16FC_4A7B_8CCE_D798CA5EC64E__INCLUDED_)
#define AFX_XRSERVER_H__65728A25_16FC_4A7B_8CCE_D798CA5EC64E__INCLUDED_
#pragma once

#if defined(WINDOWS)
#include "xrNetServer/NET_Server.h"
#elif defined(LINUX)
#include "xrNetServer/empty/NET_Server.h"
#endif
#include "game_sv_base.h"
#include "id_generator.h"
#include "xrEngine/mp_logging.h"
#include "secure_messaging.h"
#include "xrServer_updates_compressor.h"
#include "xrClientsPool.h"
#include "xrCommon/xr_unordered_map.h"

#ifdef DEBUG
//. #define SLOW_VERIFY_ENTITIES
#endif

class CSE_Abstract;

constexpr u32 NET_Latency = 50; // time in (ms)

// XXX: check if u16 used for entity's id. If true, then this must be changed, if we want to increase the number of ID's.
using xrS_entities = xr_unordered_map<u16, CSE_Abstract*>;

class xrClientData : public IClient
{
public:
    CSE_Abstract* owner;
    BOOL net_Ready;
    BOOL net_Accepted;

    BOOL net_PassUpdates;
    u32 net_LastMoveUpdateTime;

    game_PlayerState* ps;
    struct
    {
        u8 m_maxPingWarnings;
        u32 m_dwLastMaxPingWarningTime;
    } m_ping_warn;
    struct
    {
        BOOL m_has_admin_rights;
        u32 m_dwLoginTime;
    } m_admin_rights;

    shared_str m_cdkey_digest;
    secure_messaging::key_t m_secret_key;
    s32 m_last_key_sync_request_seed;

    xrClientData();
    virtual ~xrClientData();
    virtual void Clear();
};

// main
struct svs_respawn
{
    u32 timestamp;
    u16 phantom;
};
IC bool operator<(const svs_respawn& A, const svs_respawn& B) { return A.timestamp < B.timestamp; }
struct CheaterToKick
{
    shared_str reason;
    ClientID cheater_id;
};
typedef xr_vector<CheaterToKick> cheaters_t;

namespace file_transfer
{
class server_site;
} // namespace file_transfer

class clientdata_proxy;
class server_info_uploader;

class xrServer : public IPureServer
{
private:
    xrS_entities entities;
    xr_multiset<svs_respawn> q_respawn;
    xr_vector<u16> conn_spawned_ids;
    cheaters_t m_cheaters;

    file_transfer::server_site* m_file_transfers;
    clientdata_proxy* m_screenshot_proxies[MAX_PLAYERS_COUNT * 2];
    void initialize_screenshot_proxies();
    void deinitialize_screenshot_proxies();

    typedef server_updates_compressor::send_ready_updates_t::const_iterator update_iterator_t;
    update_iterator_t m_update_begin;
    update_iterator_t m_update_end;
    server_updates_compressor m_updator;

    void MakeUpdatePackets();
    void SendUpdatePacketsToAll();
    u32 m_last_updates_size;
    u32 m_last_update_time;

    void SendServerInfoToClient(ClientID const& new_client);
    server_info_uploader& GetServerInfoUploader();

    void LoadServerInfo();

    typedef xr_vector<server_info_uploader*> info_uploaders_t;

    info_uploaders_t m_info_uploaders;
    IReader* m_server_logo;
    IReader* m_server_rules;

    struct DelayedPacket
    {
        ClientID SenderID;
        NET_Packet Packet;
        bool operator==(const DelayedPacket& other) { return SenderID == other.SenderID; }
    };

    Lock DelayedPackestCS;
    xr_deque<DelayedPacket> m_aDelayedPackets;
    void ProceedDelayedPackets();
    void AddDelayedPacket(NET_Packet& Packet, ClientID Sender);
    u32 OnDelayedMessage(NET_Packet& P, ClientID sender); // Non-Zero means broadcasting with "flags" as returned

    void SendUpdatesToAll();
    void _stdcall SendGameUpdateTo(IClient* client);

private:
    typedef CID_Generator<u32, // time identifier type
        u8, // compressed id type
        u16, // id type
        u8, // block id type
        u16, // chunk id type
        0, // min value
        u16(-2), // max value
        256, // block size
        u16(-1) // invalid id
        >
        id_generator_type;

private:
    id_generator_type m_tID_Generator;
    secure_messaging::seed_generator m_seed_generator;
    struct ServerStatistics
    {
        CStatTimer Update;

        ServerStatistics() { FrameStart(); }
        void FrameStart() { Update.FrameStart(); }
        void FrameEnd() { Update.FrameEnd(); }
    };
    ServerStatistics stats;

protected:
    game_sv_GameState* game;

    void Server_Client_Check(IClient* CL);
    void PerformCheckClientsForMaxPing();

public:
    virtual IServerGameState* GetGameState() override { return game; }
    void Export_game_type(IClient* CL);
    void Perform_game_export();
    BOOL PerformRP(CSE_Abstract* E);
    void PerformMigration(CSE_Abstract* E, xrClientData* from, xrClientData* to);

    IC void clear_ids() { m_tID_Generator = id_generator_type(); }
    virtual u16 PerformIDgen(u16 ID) override { return (m_tID_Generator.tfGetID(ID)); }
    virtual void FreeID(u16 ID, u32 time) override { return (m_tID_Generator.vfFreeID(ID, time)); }
    void Perform_connect_spawn(CSE_Abstract* E, xrClientData* to, NET_Packet& P);
    void Perform_transfer(NET_Packet& PR, NET_Packet& PT, CSE_Abstract* what, CSE_Abstract* from, CSE_Abstract* to);
    void Perform_reject(CSE_Abstract* what, CSE_Abstract* from, int delta);
    virtual void Perform_destroy(CSE_Abstract* tpSE_Abstract, u32 mode) override;

    CSE_Abstract* Process_spawn(NET_Packet& P, ClientID sender,
                                bool bSpawnWithClientsMainEntityAsParent = false, CSE_Abstract* tpExistedEntity = nullptr) override;
    void Process_update(NET_Packet& P, ClientID sender);
    void Process_save(NET_Packet& P, ClientID sender);
    void Process_event(NET_Packet& P, ClientID sender);
    void Process_event_ownership(NET_Packet& P, ClientID sender, u32 time, u16 ID, BOOL bForced = FALSE);
    bool Process_event_reject(NET_Packet& P, const ClientID sender, const u32 time, const u16 id_parent,
        const u16 id_entity, bool send_message = true);
    void Process_event_destroy(NET_Packet& P, ClientID sender, u32 time, u16 ID, NET_Packet* pEPack);
    void Process_event_activate(NET_Packet& P, const ClientID sender, const u32 time, const u16 id_parent,
        const u16 id_entity, bool send_message = true);

    xrClientData* SelectBestClientToMigrateTo(CSE_Abstract* E, BOOL bForceAnother = FALSE);
    void SendConnectResult(IClient* CL, u8 res, u8 res1, pcstr ResultStr);
    void __stdcall SendConfigFinished(ClientID const& clientId);
    void SendProfileCreationError(IClient* CL, char const* reason);
    void AttachNewClient(IClient* CL);
    virtual void OnBuildVersionRespond(IClient* CL, NET_Packet& P);

protected:
    xrClientsPool m_disconnected_clients;
    bool CheckAdminRights(const shared_str& user, const shared_str& pass, string512& reason);
    virtual IClient* new_client(SClientConnectData* cl_data);

    virtual bool Check_ServerAccess(IClient* CL, string512& reason) { return true; }
    virtual bool NeedToCheckClient_GameSpy_CDKey(IClient* CL) { return false; }
    virtual void Check_GameSpy_CDKey_Success(IClient* CL);
    void RequestClientDigest(IClient* CL);
    void ProcessClientDigest(xrClientData* xrCL, NET_Packet* P);
    void KickCheaters();

    virtual bool NeedToCheckClient_BuildVersion(IClient* CL);
    virtual void Check_BuildVersion_Success(IClient* CL);

    void SendConnectionData(IClient* CL);
    void OnChatMessage(NET_Packet* P, xrClientData* CL);
    void OnProcessClientMapData(NET_Packet& P, ClientID const& clientID);

private:
    void PerformSecretKeysSync(xrClientData* xrCL);
    void PerformSecretKeysSyncAck(xrClientData* xrCL, NET_Packet& P);

protected:
    void OnSecureMessage(NET_Packet& P, xrClientData* xrClSender);

public:
    // constr / destr
    xrServer();
    virtual ~xrServer();

    // extended functionality
    virtual u32 OnMessage(NET_Packet& P, ClientID sender); // Non-Zero means broadcasting with "flags" as returned
    u32 OnMessageSync(NET_Packet& P, ClientID sender);
    virtual void OnCL_Connected(IClient* CL);
    virtual void OnCL_Disconnected(IClient* CL);
    virtual bool OnCL_QueryHost();
    virtual void SendTo_LL(ClientID ID, void* data, u32 size, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/, u32 dwTimeout = 0);
    void SecureSendTo(xrClientData* xrCL, NET_Packet& P, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/, u32 dwTimeout = 0);
    virtual void SendBroadcast(ClientID exclude, NET_Packet& P, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/);
    void GetPooledState(xrClientData* xrCL);
    void ClearDisconnectedPool() { m_disconnected_clients.Clear(); };
    virtual IClient* client_Create(); // create client info
    virtual void client_Replicate(); // replicate current state to client
    virtual IClient* client_Find_Get(ClientID ID); // Find earlier disconnected client
    virtual void client_Destroy(IClient* C); // destroy client info

    // utilities
    virtual CSE_Abstract* entity_Create(pcstr name) override;
    virtual void entity_Destroy(CSE_Abstract*& P) override;
    u32 GetEntitiesNum() { return entities.size(); };
    CSE_Abstract* GetEntity(u32 Num);
    u32 const GetLastUpdatesSize() const { return m_last_updates_size; };
    xrClientData* ID_to_client(ClientID ID, bool ScanAll = false)
    {
        return (xrClientData*)(IPureServer::ID_to_client(ID, ScanAll));
    }
    CSE_Abstract* ID_to_entity(u16 ID);

    // main
    virtual EConnect Connect(shared_str& session_name, GameDescriptionData& game_descr);
    virtual void Disconnect();
    virtual void Update();
    void SLS_Default();
    void SLS_Clear();
    void SLS_Save(IWriter& fs);
    void SLS_Load(IReader& fs);
    shared_str level_name(const shared_str& server_options) const;
    shared_str level_version(const shared_str& server_options) const;
    static LPCSTR get_map_download_url(LPCSTR level_name, LPCSTR level_version);

    void create_direct_client();
    BOOL IsDedicated() const { return m_bDedicated; };
    virtual void Assign_ServerType(string512& res){};
    virtual bool HasPassword() { return false; }
    virtual bool HasProtected() { return false; }
    void AddCheater(shared_str const& reason, ClientID const& cheaterID);
    void MakeScreenshot(ClientID const& admin_id, ClientID const& cheater_id);
    void MakeConfigDump(ClientID const& admin_id, ClientID const& cheater_id);

    virtual void GetServerInfo(CServerInfo* si);
    void SendPlayersInfo(ClientID const& to_client);

public:
    xr_string ent_name_safe(u16 eid);
#ifdef DEBUG
    bool verify_entities() const;
    void verify_entity(const CSE_Abstract* entity) const;
#endif
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);
};

#ifdef DEBUG
enum e_dbg_net_Draw_Flags
{

    dbg_draw_actor_alive = (1 << 0),
    dbg_draw_actor_dead = (1 << 1),
    dbg_draw_customzone = (1 << 2),
    dbg_draw_teamzone = (1 << 3),
    dbg_draw_invitem = (1 << 4),
    dbg_draw_actor_phys = (1 << 5),
    dbg_draw_customdetector = (1 << 6),
    dbg_destroy = (1 << 7),
    dbg_draw_autopickupbox = (1 << 8),
    dbg_draw_rp = (1 << 9),
    dbg_draw_climbable = (1 << 10),
    dbg_draw_skeleton = (1 << 11)
};
extern Flags32 dbg_net_Draw_Flags;
#endif

#endif // !defined(AFX_XRSERVER_H__65728A25_16FC_4A7B_8CCE_D798CA5EC64E__INCLUDED_)
