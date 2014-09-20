#include "pch_script.h"
#include "xrServer_Objects_ALife_All.h"
#include "level.h"
#include "game_cl_base.h"
#include "net_queue.h"
#include "ai_space.h"
#include "game_level_cross_table.h"
#include "level_graph.h"
#include "client_spawn_manager.h"
#include "../xrEngine/xr_object.h"
#include "../xrEngine/IGame_Persistent.h"

void CLevel::cl_Process_Spawn(NET_Packet& P)
{
	// Begin analysis
	shared_str			s_name;
	P.r_stringZ			(s_name);

	// Create DC (xrSE)
	CSE_Abstract*	E	= F_entity_Create	(*s_name);
	R_ASSERT2(E, *s_name);


	E->Spawn_Read		(P);
	if (E->s_flags.is(M_SPAWN_UPDATE))
		E->UPDATE_Read	(P);

	if (!E->match_configuration())
	{
		F_entity_Destroy(E);
		return;
	}
//-------------------------------------------------
//.	Msg ("M_SPAWN - %s[%d][%x] - %d %d", *s_name,  E->ID, E,E->ID_Parent, Device.dwFrame);
//-------------------------------------------------
	//force object to be local for server client
	if (OnServer())		{
		E->s_flags.set(M_SPAWN_OBJECT_LOCAL, TRUE);
	};

	/*
	game_spawn_queue.push_back(E);
	if (g_bDebugEvents)		ProcessGameSpawns();
	/*/
	g_sv_Spawn					(E);

	F_entity_Destroy			(E);
	//*/
};

void CLevel::g_cl_Spawn		(LPCSTR name, u8 rp, u16 flags, Fvector pos)
{
	// Create
	CSE_Abstract*		E	= F_entity_Create(name);
	VERIFY				(E);

	// Fill
	E->s_name			= name;
	E->set_name_replace	("");
//.	E->s_gameid			=	u8(GameID());
	E->s_RP				=	rp;
	E->ID				=	0xffff;
	E->ID_Parent		=	0xffff;
	E->ID_Phantom		=	0xffff;
	E->s_flags.assign	(flags);
	E->RespawnTime		=	0;
	E->o_Position		= pos;

	// Send
	NET_Packet			P;
	E->Spawn_Write		(P,TRUE);
	Send				(P,net_flags(TRUE));

	// Destroy
	F_entity_Destroy	(E);
}

#ifdef DEBUG
	extern Flags32				psAI_Flags;
	extern float				debug_on_frame_gather_stats_frequency;
#	include "ai_debug.h"
#endif // DEBUG

void CLevel::g_sv_Spawn		(CSE_Abstract* E)
{
#ifdef DEBUG_MEMORY_MANAGER
	u32							E_mem = 0;
	if (g_bMEMO)	{
		lua_gc					(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		lua_gc					(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		E_mem					= Memory.mem_usage();	
		Memory.stat_calls		= 0;
	}
#endif // DEBUG_MEMORY_MANAGER
	//-----------------------------------------------------------------
//	CTimer		T(false);

#ifdef DEBUG
//	Msg					("* CLIENT: Spawn: %s, ID=%d", *E->s_name, E->ID);
#endif

	// Optimization for single-player only	- minimize traffic between client and server
	if	(GameID()	== eGameIDSingle)		psNET_Flags.set	(NETFLAG_MINIMIZEUPDATES,TRUE);
	else								psNET_Flags.set	(NETFLAG_MINIMIZEUPDATES,FALSE);

	// Client spawn
//	T.Start		();
	CObject*	O		= Objects.Create	(*E->s_name);
	// Msg				("--spawn--CREATE: %f ms",1000.f*T.GetAsync());

//	T.Start		();
#ifdef DEBUG_MEMORY_MANAGER
	mem_alloc_gather_stats		(false);
#endif // DEBUG_MEMORY_MANAGER
	if (0==O || (!O->net_Spawn	(E))) 
	{
		O->net_Destroy			( );
		if(!g_dedicated_server)
			client_spawn_manager().clear(O->ID());
		Objects.Destroy			(O);
		Msg						("! Failed to spawn entity '%s'",*E->s_name);
#ifdef DEBUG_MEMORY_MANAGER
		mem_alloc_gather_stats	(!!psAI_Flags.test(aiDebugOnFrameAllocs));
#endif // DEBUG_MEMORY_MANAGER
	} else {
#ifdef DEBUG_MEMORY_MANAGER
		mem_alloc_gather_stats	(!!psAI_Flags.test(aiDebugOnFrameAllocs));
#endif // DEBUG_MEMORY_MANAGER
		if(!g_dedicated_server)
			client_spawn_manager().callback(O);
		//Msg			("--spawn--SPAWN: %f ms",1000.f*T.GetAsync());
		
		if ((E->s_flags.is(M_SPAWN_OBJECT_LOCAL)) && 
			(E->s_flags.is(M_SPAWN_OBJECT_ASPLAYER)) )	
		{
			if (IsDemoPlayStarted())
			{
				if (E->s_flags.is(M_SPAWN_OBJECT_PHANTOM))
				{
					SetControlEntity	(O);
					SetEntity			(O);	//do not switch !!!
					SetDemoSpectator	(O);
				}
			} else
			{
				if (CurrentEntity() != NULL) 
				{
					CGameObject* pGO = smart_cast<CGameObject*>(CurrentEntity());
					if (pGO) pGO->On_B_NotCurrentEntity();
				}
				SetControlEntity	(O);
				SetEntity			(O);	//do not switch !!!
			}
		}

		if (0xffff != E->ID_Parent)	
		{
			/*
			// Generate ownership-event
			NET_Packet			GEN;
			GEN.w_begin			(M_EVENT);
			GEN.w_u32			(E->m_dwSpawnTime);//-NET_Latency);
			GEN.w_u16			(GE_OWNERSHIP_TAKE);
			GEN.w_u16			(E->ID_Parent);
			GEN.w_u16			(u16(O->ID()));
			game_events->insert	(GEN);
			/*/
			NET_Packet	GEN;
			GEN.write_start();
			GEN.read_start();
			GEN.w_u16			(u16(O->ID()));
			cl_Process_Event(E->ID_Parent, GE_OWNERSHIP_TAKE, GEN);
			//*/
		}
	}

	/*if (E->s_flags.is(M_SPAWN_UPDATE)) {
		NET_Packet				temp;
		temp.B.count			= 0;
		E->UPDATE_Write			(temp);
		if (temp.B.count > 0)
		{
			temp.r_seek				(0);
			O->net_Import			(temp);
		}
		}*/ //:(

	//---------------------------------------------------------
	Game().OnSpawn				(O);
	//---------------------------------------------------------
#ifdef DEBUG_MEMORY_MANAGER
	if (g_bMEMO) {
		lua_gc					(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		lua_gc					(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		Msg						("* %20s : %d bytes, %d ops", *E->s_name,Memory.mem_usage()-E_mem, Memory.stat_calls );
	}
#endif // DEBUG_MEMORY_MANAGER
}

CSE_Abstract *CLevel::spawn_item		(LPCSTR section, const Fvector &position, u32 level_vertex_id, u16 parent_id, bool return_item)
{
	CSE_Abstract			*abstract = F_entity_Create(section);
	R_ASSERT3				(abstract,"Cannot find item with section",section);
	CSE_ALifeDynamicObject	*dynamic_object = smart_cast<CSE_ALifeDynamicObject*>(abstract);
	if (dynamic_object && ai().get_level_graph()) {
		dynamic_object->m_tNodeID	= level_vertex_id;
		if (ai().level_graph().valid_vertex_id(level_vertex_id) && ai().get_game_graph() && ai().get_cross_table())
			dynamic_object->m_tGraphID	= ai().cross_table().vertex(level_vertex_id).game_vertex_id();
	}

	//оружие спавним с полным магазинои
	CSE_ALifeItemWeapon* weapon = smart_cast<CSE_ALifeItemWeapon*>(abstract);
	if(weapon)
		weapon->a_elapsed	= weapon->get_ammo_magsize();
	
	// Fill
	abstract->s_name		= section;
	abstract->set_name_replace	(section);
//.	abstract->s_gameid		= u8(GameID());
	abstract->o_Position	= position;
	abstract->s_RP			= 0xff;
	abstract->ID			= 0xffff;
	abstract->ID_Parent		= parent_id;
	abstract->ID_Phantom	= 0xffff;
	abstract->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
	abstract->RespawnTime	= 0;

	if (!return_item) {
		NET_Packet				P;
		abstract->Spawn_Write	(P,TRUE);
		Send					(P,net_flags(TRUE));
		F_entity_Destroy		(abstract);
		return					(0);
	}
	else
		return				(abstract);
}

void	CLevel::ProcessGameSpawns	()
{
	while (!game_spawn_queue.empty())
	{
		CSE_Abstract*	E			= game_spawn_queue.front();

		g_sv_Spawn					(E);

		F_entity_Destroy			(E);

		game_spawn_queue.pop_front	();
	}
}
