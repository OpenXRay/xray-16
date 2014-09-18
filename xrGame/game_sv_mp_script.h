#pragma once

#include "game_sv_mp.h"

class game_sv_mp_script : public game_sv_mp
{
	typedef game_sv_mp inherited;
private:
	virtual		void				Create					(shared_str &options);

public:
									game_sv_mp_script		():inherited(){};
	virtual							~game_sv_mp_script		(){};
	virtual		void				Create					(LPCSTR options){};
	virtual		void				Update					() {inherited::Update();};
	virtual		void				OnPlayerConnect			(ClientID id_who);
	virtual		void				OnPlayerDisconnect		(ClientID id_who, LPSTR Name, u16 GameID);

	virtual		void				net_Export_State		(NET_Packet& P, ClientID id_to);
	virtual		void				OnEvent					(NET_Packet &P, u16 type, u32 time, ClientID sender );
	virtual		game_PlayerState*	createPlayerState()		{return inherited::createPlayerState(); };


	virtual		void				OnPlayerKillPlayer		(ClientID id_killer, ClientID id_killed){};
	virtual		void				OnPlayerHitPlayer		(u16 id_hitter, u16 id_hitted, NET_Packet& P){}; //игрок получил Hit
	virtual		BOOL				OnTouch					(u16 eid_who, u16 eid_target, BOOL bForced = FALSE){return true;};			// TRUE=allow ownership, FALSE=denied
	virtual		void				OnDetach				(u16 eid_who, u16 eid_target){};

protected:
				void				SetHitParams			(NET_Packet* P, float impulse, float power);
				float				GetHitParamsPower		(NET_Packet* P);
				float				GetHitParamsImpulse		(NET_Packet* P);
virtual		void				switch_Phase			(u32 new_phase);
				void				SpawnPlayer				(ClientID id, LPCSTR N, LPCSTR SkinName, RPoint rp);


	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(game_sv_mp_script)
#undef script_type_list
#define script_type_list save_type_list(game_sv_mp_script)

