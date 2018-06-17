#pragma once

#include "game_base.h"
#include "alife_space.h"

#include "xrCore/client_id.h"
#include "game_sv_event_queue.h"
#include "xrNetServer/NET_Server.h"

class CSE_Abstract;
class xrServer;
// [OLES] Policy:
// it_*     - means ordinal number of player
// id_*     - means player-id
// eid_*    - means entity-id
// XXX: define type aliases (PlayerId, EntityId)
class GameEventQueue;

class IServerGameState : public virtual IGameState
{
public:
    virtual ~IServerGameState() = 0;
    virtual void OnPlayerConnect(ClientID id) = 0;
    virtual void OnPlayerDisconnect(ClientID id, char* name, u16 gameId) = 0;
    virtual void OnPlayerConnectFinished(ClientID id) = 0;
//    virtual void SetRoundResult(ERoundEnd_Result result) = 0;
    virtual game_PlayerState* get_eid(u16 id) = 0;
    virtual void* get_client(u16 id) = 0;
    virtual game_PlayerState* get_id(ClientID id) = 0;
    virtual LPCSTR get_name_id(ClientID id) = 0;
    virtual u16 get_id_2_eid(ClientID id) = 0;
    virtual CSE_Abstract* get_entity_from_eid(u16 id) = 0;
    virtual void signal_Syncronize() = 0;
    virtual void OnSwitchPhase(u32 oldPhase, u32 newPhase) = 0;
    virtual xr_vector<u16>* get_children(ClientID id) = 0;
    virtual BOOL OnPreCreate(CSE_Abstract* entity) = 0;
    virtual void OnCreate(u16 idWho) = 0;
    virtual void OnPostCreate(u16 idWho) = 0;
    // TRUE=allow ownership, FALSE=denied
    virtual BOOL OnTouch(u16 eidWho, u16 eidTarget, BOOL forced = FALSE) = 0;
    virtual void OnDetach(u16 eidWho, u16 eidTarget) = 0;
    virtual BOOL OnActivate(u16 eidWho, u16 eidTarget) = 0;
    virtual void OnDestroyObject(u16 eidWho) = 0;
    // кто-то получил Hit
    virtual void OnHit(u16 hitterId, u16 targetId, NET_Packet& packet) = 0;
    // игрок получил Hit
    virtual void Create(shared_str& options) = 0;
    virtual void Update() = 0;
    // full state
    virtual void net_Export_State(NET_Packet& packet, ClientID receiverId) = 0;
    // just incremental update for specific client
    virtual void net_Export_Update(NET_Packet& packet, ClientID receiverId, ClientID id) = 0;
    // update GameTime only for remote clients
    virtual void net_Export_GameTime(NET_Packet& packet) = 0;
    virtual bool change_level(NET_Packet& packet, ClientID sender) = 0;
    virtual void save_game(NET_Packet& packet, ClientID sender) = 0;
    virtual bool load_game(NET_Packet& packet, ClientID sender) = 0;
    virtual void reload_game(NET_Packet& packet, ClientID sender) = 0;
    virtual void switch_distance(NET_Packet& packet, ClientID sender) = 0;
    virtual BOOL isFriendlyFireEnabled() = 0;
    virtual BOOL CanHaveFriendlyFire() = 0;
    virtual void teleport_object(NET_Packet& packet, u16 id) = 0;
    virtual void add_restriction(NET_Packet& packet, u16 id) = 0;
    virtual void remove_restriction(NET_Packet& packet, u16 id) = 0;
    virtual void remove_all_restrictions(NET_Packet& packet, u16 id) = 0;
    virtual bool custom_sls_default() = 0;
    virtual void sls_default() = 0;
    virtual shared_str level_name(const shared_str& options) const = 0;
    virtual void on_death(CSE_Abstract* dst, CSE_Abstract* src) = 0;
};

IC IServerGameState::~IServerGameState() {}
class game_sv_GameState : public game_GameState, public IServerGameState
{
    typedef game_GameState inherited;

protected:
    xrServer* m_server;

    GameEventQueue* m_event_queue;

    // Events
    virtual void OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);

public:
	BOOL							sv_force_sync;
	void							GenerateGameMessage		(NET_Packet &P);

									game_sv_GameState		();
	virtual							~game_sv_GameState		();
	// Main accessors
	virtual		game_PlayerState*	get_eid					(u16 id);
	virtual		void*				get_client				(u16 id); //if exist
	virtual		game_PlayerState*	get_id					(ClientID id);
	virtual		LPCSTR				get_name_id				(ClientID id);								
				LPCSTR				get_player_name_id		(ClientID id);								
	virtual		u16					get_id_2_eid			(ClientID id);
				CSE_Abstract*		get_entity_from_eid		(u16 id);

	// Signals
	virtual		void				signal_Syncronize		();

#ifdef DEBUG
	virtual		void				OnRender				();
#endif
	
	virtual		void				OnSwitchPhase			(u32 old_phase, u32 new_phase);	
				CSE_Abstract*		spawn_begin				(LPCSTR N);
				CSE_Abstract*		spawn_end				(CSE_Abstract* E, ClientID id);

	// Utilities
	float							get_option_f			(LPCSTR lst, LPCSTR name, float def = 0.0f);
	s32								get_option_i			(LPCSTR lst, LPCSTR name, s32 def = 0);
	string64&						get_option_s			(LPCSTR lst, LPCSTR name, LPCSTR def = 0);

	virtual		xr_vector<u16>*		get_children			(ClientID id_who);
	void							u_EventGen				(NET_Packet& P, u16 type, u16 dest	);
	void							u_EventSend				(NET_Packet& P, u32 dwFlags = DPNSEND_GUARANTEED);

    // Events
    virtual BOOL OnPreCreate(CSE_Abstract* E) { return TRUE; };
    virtual void OnCreate(u16 id_who){};
    virtual void OnPostCreate(u16 id_who){};
    virtual BOOL OnTouch(u16 eid_who, u16 eid_target, BOOL bForced = FALSE) = 0; // TRUE=allow ownership, FALSE=denied
    virtual void OnDetach(u16 eid_who, u16 eid_target) = 0;
    virtual BOOL OnActivate(u16 eid_who, u16 eid_target) { return TRUE; };
    virtual void OnDestroyObject(u16 eid_who);

	virtual		void				OnHit					(u16 id_hitter, u16 id_hitted, NET_Packet& P);	//кто-то получил Hit
	virtual		void				OnPlayerConnect(ClientID id_who);
	virtual		void				OnPlayerDisconnect(ClientID id_who, LPSTR Name, u16 GameID);
	virtual		void				OnPlayerConnectFinished(ClientID id_who)	{};

    // Main
    virtual void Create(shared_str& options);
    virtual void Update();
    virtual void net_Export_State(NET_Packet& P, ClientID id_to); // full state
    virtual void net_Export_Update(
        NET_Packet& P, ClientID id_to, ClientID id); // just incremental update for specific client
    virtual void net_Export_GameTime(NET_Packet& P); // update GameTime only for remote clients

    virtual bool change_level(NET_Packet& net_packet, ClientID sender);
    virtual void save_game(NET_Packet& net_packet, ClientID sender);
    virtual bool load_game(NET_Packet& net_packet, ClientID sender);
    virtual void reload_game(NET_Packet& net_packet, ClientID sender);
    virtual void switch_distance(NET_Packet& net_packet, ClientID sender);

    void AddDelayedEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);
    void ProcessDelayedEvent();
    // this method will delete all events for entity that already not exist (in case when player was kicked)
    void CleanDelayedEventFor(u16 id_entity_victim);
    void CleanDelayedEventFor(ClientID const& clientId);
    void CleanDelayedEvents();

    virtual BOOL isFriendlyFireEnabled() { return FALSE; };
    virtual BOOL CanHaveFriendlyFire() = 0;
    virtual void teleport_object(NET_Packet& packet, u16 id);
    virtual void add_restriction(NET_Packet& packet, u16 id);
    virtual void remove_restriction(NET_Packet& packet, u16 id);
    virtual void remove_all_restrictions(NET_Packet& packet, u16 id);
    virtual bool custom_sls_default() { return false; };
    virtual void sls_default(){};
    virtual shared_str level_name(const shared_str& server_options) const;

    static shared_str parse_level_name(const shared_str& server_options);
    static shared_str parse_level_version(const shared_str& server_options);

	virtual		void				on_death				(CSE_Abstract *e_dest, CSE_Abstract *e_src);
};
