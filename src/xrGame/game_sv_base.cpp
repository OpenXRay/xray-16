#include "stdafx.h"
#include "Common/LevelGameDef.h"
#include "xrScriptEngine/script_process.hpp"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrScriptEngine/script_engine.hpp"
#include "Level.h"
#include "xrserver.h"
#include "ai_space.h"
#include "game_sv_event_queue.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "string_table.h"
#include "xrNetServer/NET_Messages.h"

#include "debug_renderer.h"

ENGINE_API	bool g_dedicated_server;

BOOL	net_sv_control_hit	= FALSE;


// Main
game_PlayerState*	game_sv_GameState::get_id					(ClientID id)							
{
    xrClientData* C = (xrClientData*)m_server->ID_to_client(id);
    if (0 == C)
        return NULL;
    else
        return C->ps;
}

LPCSTR				game_sv_GameState::get_name_id				(ClientID id)							
{
    xrClientData* C = (xrClientData*)m_server->ID_to_client(id);
    return C == NULL ? NULL : C->ps->getName();
}

LPCSTR game_sv_GameState::get_player_name_id(ClientID id)
{
    xrClientData* xrCData = m_server->ID_to_client(id);
    return xrCData == NULL ? "unknown" : xrCData->ps->getName();
}

u16					game_sv_GameState::get_id_2_eid				(ClientID id)
{
	xrClientData*	C	= (xrClientData*)m_server->ID_to_client	(id);
	if (0==C)			return 0xffff;
	CSE_Abstract*	E	= C->owner;
	if (0==E)			return 0xffff;
	return E->ID;
}

game_PlayerState* game_sv_GameState::get_eid(u16 id) // if exist
{
    CSE_Abstract* entity = get_entity_from_eid(id);

    if (entity)
    {
        if (entity->owner)
        {
            if (entity->owner->ps)
            {
                if (entity->owner->ps->GameID == id)
                    return entity->owner->ps;
            }
        }
    }
    //-------------------------------------------------
    struct id_searcher
    {
        u16 id_to_search;
        bool operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            if (!tmp_client->ps)
                return false;
            return tmp_client->ps->HasOldID(id_to_search);
        }
    };
    id_searcher tmp_predicate;
    tmp_predicate.id_to_search = id;
    xrClientData* tmp_client = static_cast<xrClientData*>(m_server->FindClient(tmp_predicate));
    if (tmp_client)
        return tmp_client->ps;
    return NULL;
}

void* game_sv_GameState::get_client(u16 id) // if exist
{
    CSE_Abstract* entity = get_entity_from_eid(id);
    if (entity && entity->owner && entity->owner->ps && entity->owner->ps->GameID == id)
        return entity->owner;
    struct client_searcher
    {
        u16 binded_id;
        bool operator()(IClient* client)
        {
            xrClientData* tmp_client = static_cast<xrClientData*>(client);
            if (!tmp_client || !tmp_client->ps)
                return false;
            return tmp_client->ps->HasOldID(binded_id);
        }
    };
    client_searcher searcher_predicate;
    searcher_predicate.binded_id = id;
    return m_server->FindClient(searcher_predicate);
}

CSE_Abstract* game_sv_GameState::get_entity_from_eid(u16 id) { return m_server->ID_to_entity(id); }
// Utilities
xr_vector<u16>*		game_sv_GameState::get_children				(ClientID id)
{
    xrClientData* C = (xrClientData*)m_server->ID_to_client(id);
    if (0 == C)
        return 0;
    CSE_Abstract* E = C->owner;
    if (0 == E)
        return 0;
    return &(E->children);
}

s32 game_sv_GameState::get_option_i(LPCSTR lst, LPCSTR name, s32 def)
{
    string64 op;
    strconcat(sizeof(op), op, "/", name, "=");
    if (strstr(lst, op))
        return atoi(strstr(lst, op) + xr_strlen(op));
    else
        return def;
}

float game_sv_GameState::get_option_f(LPCSTR lst, LPCSTR name, float def)
{
    string64 op;
    strconcat(sizeof(op), op, "/", name, "=");
    LPCSTR found = strstr(lst, op);

    if (found)
    {
        float val;
        int cnt = sscanf(found + xr_strlen(op), "%f", &val);
        VERIFY(cnt == 1);
        return val;
        //.		return atoi	(strstr(lst,op)+xr_strlen(op));
    }
    else
        return def;
}

string64& game_sv_GameState::get_option_s(LPCSTR lst, LPCSTR name, LPCSTR def)
{
    static string64 ret;

    string64 op;
    strconcat(sizeof(op), op, "/", name, "=");
    LPCSTR start = strstr(lst, op);
    if (start)
    {
        LPCSTR begin = start + xr_strlen(op);
        sscanf(begin, "%[^/]", ret);
    }
    else
    {
        if (def)
            xr_strcpy(ret, def);
        else
            ret[0] = 0;
    }
    return ret;
}
void game_sv_GameState::signal_Syncronize() { sv_force_sync = TRUE; }
// Network
void game_sv_GameState::net_Export_State						(NET_Packet& P, ClientID to)
{
}

void game_sv_GameState::net_Export_Update(NET_Packet& P, ClientID id_to, ClientID id)
{
};

void game_sv_GameState::net_Export_GameTime(NET_Packet& P)
{
};

void game_sv_GameState::OnPlayerConnect(ClientID /**id_who/**/) { signal_Syncronize(); }
void game_sv_GameState::OnPlayerDisconnect(ClientID id_who, LPSTR, u16) { signal_Syncronize(); }


void game_sv_GameState::Create					(shared_str &options)
{
    if (!GEnv.isDedicatedServer)
    {
        // loading scripts
        auto& scriptEngine = *GEnv.ScriptEngine;
        scriptEngine.remove_script_process(ScriptProcessor::Game);
        string_path S;
        FS.update_path(S, "$game_config$", "script.ltx");
        CInifile* l_tpIniFile = new CInifile(S);
        R_ASSERT(l_tpIniFile);

        if (l_tpIniFile->section_exist(type_name()))
        {
            shared_str scripts;
            if (l_tpIniFile->r_string(type_name(), "script"))
                scripts = l_tpIniFile->r_string(type_name(), "script");
            else
                scripts = "";
            scriptEngine.add_script_process(ScriptProcessor::Game, scriptEngine.CreateScriptProcess("game", scripts));
        }
        xr_delete(l_tpIniFile);
    }
}

CSE_Abstract*		game_sv_GameState::spawn_begin				(LPCSTR N)
{
    CSE_Abstract* A = F_entity_Create(N);
    R_ASSERT(A); // create SE
    A->s_name = N; // ltx-def
    //.	A->s_gameid			=	u8(m_type);							// game-type
    A->s_RP = 0xFE; // use supplied
    A->ID = 0xffff; // server must generate ID
    A->ID_Parent = 0xffff; // no-parent
    A->ID_Phantom = 0xffff; // no-phantom
    A->RespawnTime = 0; // no-respawn
    return A;
}

CSE_Abstract* game_sv_GameState::spawn_end(CSE_Abstract* E, ClientID id)
{
    NET_Packet P;
    u16 skip_header;
    E->Spawn_Write(P, TRUE);
    P.r_begin(skip_header);
    CSE_Abstract* N = m_server->Process_spawn(P, id);
    F_entity_Destroy(E);

    return N;
}

void game_sv_GameState::GenerateGameMessage(NET_Packet& P) { P.w_begin(M_GAMEMESSAGE); };
void game_sv_GameState::u_EventGen(NET_Packet& P, u16 type, u16 dest)
{
    P.w_begin(M_EVENT);
    P.w_u32(Level().timeServer()); // Device.TimerAsync());
    P.w_u16(type);
    P.w_u16(dest);
}

void game_sv_GameState::u_EventSend(NET_Packet& P, u32 dwFlags) { m_server->SendBroadcast(BroadcastCID, P, dwFlags); }
void game_sv_GameState::Update()
{
    if (!GEnv.isDedicatedServer)
    {
        if (Level().game)
        {
            CScriptProcess* script_process = GEnv.ScriptEngine->script_process(ScriptProcessor::Game);
            if (script_process)
                script_process->update();
        }
    }
}

void game_sv_GameState::OnDestroyObject(u16 eid_who)
{
}

game_sv_GameState::game_sv_GameState()
{
	VERIFY(g_pGameLevel);
	m_server					= Level().Server;
	m_event_queue = new GameEventQueue();
}

game_sv_GameState::~game_sv_GameState()
{
    if (!GEnv.isDedicatedServer)
        GEnv.ScriptEngine->remove_script_process(ScriptProcessor::Game);
    xr_delete(m_event_queue);
}

bool game_sv_GameState::change_level(NET_Packet& net_packet, ClientID sender) { return (true); }
void game_sv_GameState::save_game(NET_Packet& net_packet, ClientID sender) {}
bool game_sv_GameState::load_game(NET_Packet& net_packet, ClientID sender) { return (true); }
void game_sv_GameState::reload_game(NET_Packet& net_packet, ClientID sender) {}
void game_sv_GameState::switch_distance(NET_Packet& net_packet, ClientID sender) {}
void game_sv_GameState::OnHit(u16 id_hitter, u16 id_hitted, NET_Packet& P)
{
}

void game_sv_GameState::OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender)
{
    switch (type)
    {
    case GAME_EVENT_PLAYER_CONNECTED:
    {
        ClientID ID;
        tNetPacket.r_clientID(ID);
        OnPlayerConnect(ID);
    }
    break;

    case GAME_EVENT_PLAYER_DISCONNECTED:
    {
        ClientID ID;
        tNetPacket.r_clientID(ID);
        string1024 PlayerName;
        tNetPacket.r_stringZ(PlayerName);
        u16 GameID = tNetPacket.r_u16();
        OnPlayerDisconnect(ID, PlayerName, GameID);
    }
    break;

    case GAME_EVENT_PLAYER_KILLED: {
    }
    break;
    case GAME_EVENT_ON_HIT:
    {
        u16 id_dest = tNetPacket.r_u16();
        u16 id_src = tNetPacket.r_u16();
        CSE_Abstract* e_src = get_entity_from_eid(id_src);

        if (!e_src) // && !IsGameTypeSingle() added by andy because of Phantom does not have server entity
        {
            if (IsGameTypeSingle())
                break;

            game_PlayerState* ps = get_eid(id_src);
            if (!ps)
                break;
            id_src = ps->GameID;
        }

        OnHit(id_src, id_dest, tNetPacket);
        m_server->SendBroadcast(BroadcastCID, tNetPacket, net_flags(TRUE, TRUE));
    }
    break;
    case GAME_EVENT_CREATE_CLIENT:
    {
        IClient* CL = (IClient*)m_server->ID_to_client(sender);
        VERIFY2(CL, "bad create client message GAME_EVENT_CREATE_CLIENT");
        if (CL == NULL)
        {
            break;
        }

        CL->flags.bConnected = TRUE;
        m_server->AttachNewClient(CL);
    }
    break;
    case GAME_EVENT_PLAYER_AUTH:
    {
        IClient* CL = m_server->ID_to_client(sender);
        m_server->OnBuildVersionRespond(CL, tNetPacket);
    }
    break;
    case GAME_EVENT_CREATE_PLAYER_STATE:
    {
        xrClientData* CL = m_server->ID_to_client(sender);
        R_ASSERT2(CL, make_string("M_CREATE_PLAYER_STATE: client 0x%08x not found", sender.value()).c_str());
        CL->ps = createPlayerState(&tNetPacket);
        CL->ps->m_online_time = Level().timeServer();
        CL->ps->DeathTime = Device.dwTimeGlobal;

        if (psNET_direct_connect) // IsGameTypeSingle())
            break;

        if (Level().IsDemoPlay())
            break;

        if (GEnv.isDedicatedServer && (CL == m_server->GetServerClient()))
            break;

    }
    break;
    default:
    {
        string16 tmp;
        R_ASSERT3(0, "Game Event not implemented!!!", xr_itoa(type, tmp, 10));
    };
    };
}

void game_sv_GameState::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
    inherited::OnSwitchPhase(old_phase, new_phase);
    signal_Syncronize();
}

void game_sv_GameState::AddDelayedEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender)
{
    //	OnEvent(tNetPacket,type,time,sender);
    if (IsGameTypeSingle())
    {
        m_event_queue->Create(tNetPacket, type, time, sender);
        return;
    }
    switch (type)
    {
    case GAME_EVENT_PLAYER_STARTED:
    case GAME_EVENT_PLAYER_READY:
    case GAME_EVENT_VOTE_START:
    case GAME_EVENT_VOTE_YES:
    case GAME_EVENT_VOTE_NO:
    case GAME_EVENT_PLAYER_AUTH:
    case GAME_EVENT_CREATE_PLAYER_STATE: { m_event_queue->Create(tNetPacket, type, time, sender);
    }
    break;
    default: { m_event_queue->CreateSafe(tNetPacket, type, time, sender);
    }
    break;
    }
}

void game_sv_GameState::ProcessDelayedEvent()
{
    GameEvent* ge = NULL;
    while ((ge = m_event_queue->Retreive()) != 0)
    {
        OnEvent(ge->P, ge->type, ge->time, ge->sender);
        m_event_queue->Release();
    }
}

class EventDeleterPredicate
{
private:
    u16 id_entity_victim;

public:
    EventDeleterPredicate() { id_entity_victim = u16(-1); }
    EventDeleterPredicate(u16 id_entity) { id_entity_victim = id_entity; }
    bool __stdcall PredicateDelVictim(GameEvent* const ge)
    {
        bool ret_val = false;
        switch (ge->type)
        {
        case GAME_EVENT_PLAYER_KILLED:
        case GAME_EVENT_PLAYER_HITTED:
        {
            u32 tmp_pos = ge->P.r_tell();
            u16 id_entity_for = ge->P.r_u16();
            if (id_entity_for == id_entity_victim)
                ret_val = true;
            ge->P.r_seek(tmp_pos);
        }
        break;
        };
        return ret_val;
    }
    bool __stdcall PredicateForAll(GameEvent* const ge)
    {
        Msg("- Erasing [%d] event before start.", ge->type);
        return true;
    }
};

void game_sv_GameState::CleanDelayedEventFor(u16 id_entity_victim)
{
    EventDeleterPredicate event_deleter(id_entity_victim);
    m_event_queue->EraseEvents(fastdelegate::MakeDelegate(&event_deleter, &EventDeleterPredicate::PredicateDelVictim));
}

class EventDeleteForClientPredicate
{
public:
    EventDeleteForClientPredicate(ClientID const& clientId) : m_client_id(clientId) {}
    EventDeleteForClientPredicate(EventDeleteForClientPredicate const& copy) : m_client_id(copy.m_client_id) {}
    bool __stdcall Predicate(GameEvent* const ge)
    {
        if (ge && (ge->sender == m_client_id))
        {
            Msg("- Erasing event for not valid client [0x%08x]", m_client_id.value());
            return true;
        }
        return false;
    }

private:
    EventDeleteForClientPredicate& operator=(EventDeleteForClientPredicate const& copy) {}
    ClientID const m_client_id;
}; // class EventDeleteForClientPredicate

void game_sv_GameState::CleanDelayedEventFor(ClientID const& clientId)
{
    EventDeleteForClientPredicate event_deleter(clientId);
    m_event_queue->EraseEvents(fastdelegate::MakeDelegate(&event_deleter, &EventDeleteForClientPredicate::Predicate));
}

void game_sv_GameState::CleanDelayedEvents()
{
    EventDeleterPredicate event_deleter;
    m_event_queue->EraseEvents(fastdelegate::MakeDelegate(&event_deleter, &EventDeleterPredicate::PredicateForAll));
}

void game_sv_GameState::teleport_object	(NET_Packet &packet, u16 id)
{
}

void game_sv_GameState::add_restriction	(NET_Packet &packet, u16 id)
{
}

void game_sv_GameState::remove_restriction(NET_Packet &packet, u16 id)
{
}

void game_sv_GameState::remove_all_restrictions	(NET_Packet &packet, u16 id)
{
}

shared_str game_sv_GameState::level_name		(const shared_str &server_options) const
{
    return parse_level_name(server_options);
}

LPCSTR default_map_version = "1.0";
LPCSTR map_ver_string = "ver=";

shared_str game_sv_GameState::parse_level_version(const shared_str& server_options)
{
    const char* map_ver = strstr(server_options.c_str(), map_ver_string);
    string128 result_version;
    if (map_ver)
    {
        map_ver += sizeof(map_ver_string);
        if (strchr(map_ver, '/'))
            strncpy_s(result_version, map_ver, strchr(map_ver, '/') - map_ver);
        else
            xr_strcpy(result_version, map_ver);
    }
    else
    {
        xr_strcpy(result_version, default_map_version);
    }
    return shared_str(result_version);
}

shared_str game_sv_GameState::parse_level_name(const shared_str& server_options)
{
    string64 l_name = "";
    VERIFY(_GetItemCount(*server_options, '/'));
    return (_GetItem(*server_options, 0, l_name, '/'));
}

void game_sv_GameState::on_death(CSE_Abstract* e_dest, CSE_Abstract* e_src)
{
    CSE_ALifeCreatureAbstract* creature = smart_cast<CSE_ALifeCreatureAbstract*>(e_dest);
    if (!creature)
        return;

    VERIFY(creature->get_killer_id() == ALife::_OBJECT_ID(-1));
    creature->set_killer_id(e_src->ID);
}

//  [7/5/2005]
#ifdef DEBUG
extern Flags32 dbg_net_Draw_Flags;
#endif



//  [7/5/2005]

class NameSearcherPredicate
{
public:
    NameSearcherPredicate(char const* name, IClient const* to_exclude) : m_name(name), m_exclude(to_exclude) {}
    inline bool operator()(IClient* client) const
    {
        if (client == m_exclude)
            return false;

        xrClientData* tmp_cl = static_cast<xrClientData*>(client);
        if (!tmp_cl || !tmp_cl->ps)
            return false;

        return (xr_strcmp(tmp_cl->ps->getName(), m_name) == 0);
    }

private:
	char const *	m_name;
	IClient const * m_exclude;
}; //class NameSearcherPredicate
