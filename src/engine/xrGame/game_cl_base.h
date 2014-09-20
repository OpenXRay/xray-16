#pragma once

#include "game_base.h"
#include "../xrCore/client_id.h"
#include "WeaponAmmo.h"

class	NET_Packet;
class	CGameObject;
class	CUIGameCustom;
class	CUI;
class	CUIDialogWnd;

struct SZoneMapEntityData{
	Fvector	pos;
	u32		color;
	SZoneMapEntityData(){pos.set(.0f,.0f,.0f);color = 0xff00ff00;}
};

struct WeaponUsageStatistic;

class	game_cl_GameState	: public game_GameState, public ISheduled
{
	typedef game_GameState	inherited;
	shared_str							m_game_type_name;
protected:
	CUIGameCustom*						m_game_ui_custom;
	u16									m_u16VotingEnabled;	
	bool								m_bServerControlHits;	

public:
	typedef associative_vector<ClientID,game_PlayerState*>	PLAYERS_MAP;
	typedef PLAYERS_MAP::iterator							PLAYERS_MAP_IT;
	typedef PLAYERS_MAP::const_iterator						PLAYERS_MAP_CIT;

	PLAYERS_MAP							players;
	ClientID							local_svdpnid;
	game_PlayerState*					local_player;
	game_PlayerState*					lookat_player();


	WeaponUsageStatistic				*m_WeaponUsageStatistic;
private:
				void				switch_Phase			(u32 new_phase)		{inherited::switch_Phase(new_phase);};
protected:

	virtual		void				OnSwitchPhase			(u32 old_phase, u32 new_phase);	

	//for scripting enhancement
	virtual		void				TranslateGameMessage	(u32 msg, NET_Packet& P);


	virtual		shared_str			shedule_Name			() const		{ return shared_str("game_cl_GameState"); };
	virtual		float				shedule_Scale			()				{ return 1.0f;};
	virtual		bool				shedule_Needed			()				{return true;};

				void				sv_GameEventGen			(NET_Packet& P);
				void				sv_EventSend			(NET_Packet& P);
public:
									game_cl_GameState		();
	virtual							~game_cl_GameState		();
				LPCSTR				type_name				() const {return *m_game_type_name;};
				void				set_type_name			(LPCSTR s);
	virtual		void				Init					(){};
	virtual		void				net_import_state		(NET_Packet& P);
	virtual		void				net_import_update		(NET_Packet& P);
	virtual		void				net_import_GameTime		(NET_Packet& P);						// update GameTime only for remote clients
	virtual		void				net_signal				(NET_Packet& P);

	virtual		bool				OnKeyboardPress			(int key);
	virtual		bool				OnKeyboardRelease		(int key);

				void				OnGameMessage			(NET_Packet& P);

	virtual		char*				getTeamSection			(int Team){return NULL;};

				game_PlayerState*	GetPlayerByGameID		(u32 GameID);
				game_PlayerState*	GetPlayerByOrderID		(u32 id);
				ClientID			GetClientIDByOrderID	(u32 id);
				u32					GetPlayersCount			() const {return players.size();};
	virtual		CUIGameCustom*		createGameUI			(){return NULL;};
	virtual		void				SetGameUI				(CUIGameCustom*){};
	virtual		void				GetMapEntities			(xr_vector<SZoneMapEntityData>& dst)	{};


	virtual		void				shedule_Update			(u32 dt);

	void							u_EventGen				(NET_Packet& P, u16 type, u16 dest);
	void							u_EventSend				(NET_Packet& P);

	virtual		void				ChatSay					(LPCSTR phrase, bool bAll)	{};
	virtual		void				OnChatMessage			(NET_Packet* P)	{};
	virtual		void				OnWarnMessage			(NET_Packet* P)	{};
	virtual		void				OnRadminMessage			(u16 type, NET_Packet* P)	{};
	

	virtual		bool				IsVotingEnabled			()	{return m_u16VotingEnabled != 0;};
	virtual		bool				IsVotingEnabled			(u16 flag) {return (m_u16VotingEnabled & flag) != 0;};
	virtual		bool				IsVotingActive			()	{ return false; };
	virtual		void				SetVotingActive			( bool Active )	{ };
	virtual		void				SendStartVoteMessage	(LPCSTR args)	{};
	virtual		void				SendVoteYesMessage		()	{};
	virtual		void				SendVoteNoMessage		()	{};
	virtual		void				OnVoteStart				(NET_Packet& P)	{};
	virtual		void				OnVoteStop				(NET_Packet& P)	{};

	virtual		void				OnRender				()	{};
	virtual		bool				IsServerControlHits		()	{return m_bServerControlHits;};
	virtual		bool				IsEnemy					(game_PlayerState* ps)	{return false;};
	virtual		bool				IsEnemy					(CEntityAlive* ea1, CEntityAlive* ea2)	{return false;};
	virtual		bool				PlayerCanSprint			(CActor* pActor) {return true;};

	virtual		void				OnSpawn					(CObject* pObj)	{};
	virtual		void				OnDestroy				(CObject* pObj)	{};

	virtual		void				OnPlayerFlagsChanged	(game_PlayerState* ps)	{};
	virtual		void				OnNewPlayerConnected	(ClientID const & newClient) {};
	virtual		void				OnPlayerVoted			(game_PlayerState* ps)	{};
	virtual		void				SendPickUpEvent			(u16 ID_who, u16 ID_what);

	virtual		bool				IsPlayerInTeam			(game_PlayerState* ps, ETeam team) {return ps->team == team;};
	virtual		void				OnConnected				();
};
