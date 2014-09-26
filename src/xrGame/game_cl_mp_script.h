#pragma once

#include "game_cl_mp.h"
#include "script_export_space.h"
class CScriptGameObject;

class game_cl_mp_script : public game_cl_mp
{
	typedef game_cl_mp	inherited;
protected:
	CScriptGameObject*				GetObjectByGameID		(u16 id);
	game_PlayerState*				GetLocalPlayer			(){return local_player;};
public:
									game_cl_mp_script		();
	virtual		bool				CanBeReady				(){return false;};
	virtual		void				GetMapEntities			(xr_vector<SZoneMapEntityData>& dst)	{};
	virtual		void				shedule_Update			(u32 dt);
	virtual		game_PlayerState*	createPlayerState()		{return inherited::createPlayerState(); };

				void				EventGen				( NET_Packet* P, u16 type, u16 dest);
				void				GameEventGen			( NET_Packet* P, u16 dest);
				void				EventSend				( NET_Packet* P);
				LPCSTR				GetRoundTime			();
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(game_cl_mp_script)
#undef script_type_list
#define script_type_list save_type_list(game_cl_mp_script)
