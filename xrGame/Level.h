#if !defined(AFX_LEVEL_H__38F63863_DB0C_494B_AFAB_C495876EC671__INCLUDED_)
#define AFX_LEVEL_H__38F63863_DB0C_494B_AFAB_C495876EC671__INCLUDED_
#pragma once

#include "../xrEngine/igame_level.h"
#include "../xrEngine/IGame_Persistent.h"
#include "../xrNetServer/net_client.h"
#include "script_export_space.h"
#include "../xrEngine/StatGraph.h"
#include "xrMessages.h"
#include "alife_space.h"
#include "../xrcore/xrDebug.h"
#include "xrServer.h"
#include "GlobalFeelTouch.hpp"

#include "Level_network_map_sync.h"
#include "secure_messaging.h"
#include "traffic_optimization.h"


class	CHUDManager;
class	CParticlesObject;
class	xrServer;
class	game_cl_GameState;
class	NET_Queue_Event;
class	CSE_Abstract;
class	CSpaceRestrictionManager;
class	CSeniorityHierarchyHolder;
class	CClientSpawnManager;
class	CGameObject;
class	CAutosaveManager;
class	CPHCommander;
class	CLevelDebug;
class	CLevelSoundManager;
class	CGameTaskManager;
class	CZoneList;
class	message_filter;
class	demoplay_control;
class	demo_info;

#ifdef DEBUG
	class	CDebugRenderer;
#endif

extern float g_fov;

const int maxRP					= 64;
const int maxTeams				= 32;

//class CFogOfWar;
class CFogOfWarMngr;
class CBulletManager;
class CMapManager;

namespace file_transfer
{
	class client_site;
}; //namespace file_transfer

class CLevel					: public IGame_Level, public IPureClient
{
	#include "Level_network_Demo.h"
	void						ClearAllObjects			();
private:
#ifdef DEBUG
	bool						m_bSynchronization;
	bool						m_bEnvPaused;
#endif
protected:
	typedef IGame_Level			inherited;
	
	CLevelSoundManager			*m_level_sound_manager;

	// movement restriction manager
	CSpaceRestrictionManager	*m_space_restriction_manager;
	// seniority hierarchy holder
	CSeniorityHierarchyHolder	*m_seniority_hierarchy_holder;
	// client spawn_manager
	CClientSpawnManager			*m_client_spawn_manager;
	// autosave manager
	CAutosaveManager			*m_autosave_manager;
#ifdef DEBUG
	// debug renderer
	CDebugRenderer				*m_debug_renderer;
#endif

	CPHCommander				*m_ph_commander;
	CPHCommander				*m_ph_commander_scripts;
	CPHCommander				*m_ph_commander_physics_worldstep;
	// Local events
	EVENT						eChangeRP;
	EVENT						eDemoPlay;
	EVENT						eChangeTrack;
	EVENT						eEnvironment;
	EVENT						eEntitySpawn;
	//---------------------------------------------
	CStatGraph					*pStatGraphS;
	u32							m_dwSPC;	//SendedPacketsCount
	u32							m_dwSPS;	//SendedPacketsSize
	CStatGraph					*pStatGraphR;
	u32							m_dwRPC;	//ReceivedPacketsCount
	u32							m_dwRPS;	//ReceivedPacketsSize
	//---------------------------------------------
	
public:
#ifdef DEBUG
	// level debugger
	CLevelDebug					*m_level_debug;
#endif

public:
	////////////// network ////////////////////////
	u32							GetInterpolationSteps	();
	void						SetInterpolationSteps	(u32 InterpSteps);
	bool						InterpolationDisabled	();
	void						ReculcInterpolationSteps();
	u32							GetNumCrSteps			() const	{return m_dwNumSteps; };
	void						SetNumCrSteps			( u32 NumSteps );
	static void 				PhisStepsCallback		( u32 Time0, u32 Time1 );
	bool						In_NetCorrectionPrediction	() {return m_bIn_CrPr;};

	virtual void				OnMessage				(void* data, u32 size);
	virtual void				OnInvalidHost			();
	virtual void				OnInvalidPassword		();
	virtual void				OnSessionFull			();
	virtual void				OnConnectRejected		();

private:
			
			void				OnSecureMessage			(NET_Packet & P);
			void				OnSecureKeySync			(NET_Packet & P);
			void				SecureSend				(NET_Packet& P, u32 dwFlags=DPNSEND_GUARANTEED, u32 dwTimeout=0);
			
	secure_messaging::key_t		m_secret_key;
private:
	BOOL						m_bNeed_CrPr;
	u32							m_dwNumSteps;
	bool						m_bIn_CrPr;

	DEF_VECTOR					(OBJECTS_LIST, CGameObject*);

	OBJECTS_LIST				pObjects4CrPr;
	OBJECTS_LIST				pActors4CrPr;

	CObject*					pCurrentControlEntity;
	xrServer::EConnect			m_connect_server_err;
public:
	void						AddObject_To_Objects4CrPr	(CGameObject* pObj);
	void						AddActor_To_Actors4CrPr		(CGameObject* pActor);

	void						RemoveObject_From_4CrPr		(CGameObject* pObj);	

	CObject*					CurrentControlEntity	( void ) const		{ return pCurrentControlEntity; }
	void						SetControlEntity		( CObject* O  )		{ pCurrentControlEntity=O; }
private:
	
	void						make_NetCorrectionPrediction	();

	u32							m_dwDeltaUpdate ;
	u32							m_dwLastNetUpdateTime;
	void						UpdateDeltaUpd					( u32 LastTime );
	void						BlockCheatLoad					()				;

	BOOL						Connect2Server					(LPCSTR options);
	void						SendClientDigestToServer		();
	shared_str					m_client_digest;	//for screenshots
public:
	shared_str const			get_cdkey_digest() const { return m_client_digest; };
private:
	bool						m_bConnectResultReceived;
	bool						m_bConnectResult;
	xr_string					m_sConnectResult;
public:	
	void						OnGameSpyChallenge				(NET_Packet* P);
	void						OnBuildVersionChallenge			();
	void						OnConnectResult					(NET_Packet* P);
public:
	//////////////////////////////////////////////	
	// static particles
	DEFINE_VECTOR				(CParticlesObject*,POVec,POIt);
	POVec						m_StaticParticles;

	game_cl_GameState			*game;
	BOOL						m_bGameConfigStarted;
	BOOL						game_configured;
	NET_Queue_Event				*game_events;
	xr_deque<CSE_Abstract*>		game_spawn_queue;
	xrServer*					Server;
	GlobalFeelTouch				m_feel_deny;
	
	CZoneList*					hud_zones_list;
	CZoneList*					create_hud_zones_list();

private:
	// preload sounds registry
	DEFINE_MAP					(shared_str,ref_sound,SoundRegistryMap,SoundRegistryMapIt);
	SoundRegistryMap			sound_registry;

public:
	void						PrefetchSound (LPCSTR name);

protected:
	BOOL						net_start_result_total;
	BOOL						connected_to_server;
	
	BOOL						deny_m_spawn;		//only for debug...
	
	BOOL						sended_request_connection_data;
		
	void						MakeReconnect();
	
	LevelMapSyncData			map_data;
	bool						synchronize_map_data	();
	bool						synchronize_client		();

	bool	xr_stdcall			net_start1				();
	bool	xr_stdcall			net_start2				();
	bool	xr_stdcall			net_start3				();
	bool	xr_stdcall			net_start4				();
	bool	xr_stdcall			net_start5				();
	bool	xr_stdcall			net_start6				();

	bool	xr_stdcall			net_start_client1				();
	bool	xr_stdcall			net_start_client2				();
	bool	xr_stdcall			net_start_client3				();
	bool	xr_stdcall			net_start_client4				();
	bool	xr_stdcall			net_start_client5				();
	bool	xr_stdcall			net_start_client6				();

	void						net_OnChangeSelfName			(NET_Packet* P);

	void						CalculateLevelCrc32		();
public:
	bool						IsChecksumsEqual		(u32 check_sum) const;

	// sounds
	xr_vector<ref_sound*>		static_Sounds;

	// startup options
	shared_str					m_caServerOptions;
	shared_str					m_caClientOptions;

	// Starting/Loading
	virtual BOOL				net_Start				( LPCSTR op_server, LPCSTR op_client);
	virtual void				net_Load				( LPCSTR name );
	virtual void				net_Save				( LPCSTR name );
	virtual void				net_Stop				( );
	virtual BOOL				net_Start_client		( LPCSTR name );
	virtual void				net_Update				( );


	virtual BOOL				Load_GameSpecific_Before( );
	virtual BOOL				Load_GameSpecific_After ( );
	virtual void				Load_GameSpecific_CFORM	( CDB::TRI* T, u32 count );

	// Events
	virtual void				OnEvent					( EVENT E, u64 P1, u64 P2 );
	virtual void	_BCL		OnFrame					( void );
	virtual void				OnRender				( );

	virtual	shared_str			OpenDemoFile			(LPCSTR demo_file_name);
	virtual void				net_StartPlayDemo		();

	void						cl_Process_Event		(u16 dest, u16 type, NET_Packet& P);
	void						cl_Process_Spawn		(NET_Packet& P);
	void						ProcessGameEvents		( );
	void						ProcessGameSpawns		( );
	void						ProcessCompressedUpdate	(NET_Packet& P, u8 const compression_type);

	// Input
	virtual	void				IR_OnKeyboardPress		( int btn );
	virtual void				IR_OnKeyboardRelease	( int btn );
	virtual void				IR_OnKeyboardHold		( int btn );
	virtual void				IR_OnMousePress			( int btn );
	virtual void				IR_OnMouseRelease		( int btn );
	virtual void				IR_OnMouseHold			( int btn );
	virtual void				IR_OnMouseMove			( int, int);
	virtual void				IR_OnMouseStop			( int, int);
	virtual void				IR_OnMouseWheel			( int direction);
	virtual void				IR_OnActivate			(void);
	
			int					get_RPID				(LPCSTR name);


	// Game
	void						InitializeClientGame	(NET_Packet& P);
	void						ClientReceive			();
	void						ClientSend				();
	void						ClientSendProfileData	();
	void						ClientSave				();
			u32					Objects_net_Save		(NET_Packet* _Packet, u32 start, u32 count);
	virtual	void				Send					(NET_Packet& P, u32 dwFlags=DPNSEND_GUARANTEED, u32 dwTimeout=0);
	
	void						g_cl_Spawn				(LPCSTR name, u8 rp, u16 flags, Fvector pos);	// only ask server
	void						g_sv_Spawn				(CSE_Abstract* E);					// server reply/command spawning
	
	// Save/Load/State
	void						SLS_Load				(LPCSTR name);		// Game Load
	void						SLS_Default				();					// Default/Editor Load
	
	IC CSpaceRestrictionManager		&space_restriction_manager	();
	IC CSeniorityHierarchyHolder	&seniority_holder			();
	IC CClientSpawnManager			&client_spawn_manager		();
	IC CAutosaveManager				&autosave_manager			();
#ifdef DEBUG
	IC CDebugRenderer				&debug_renderer				();
#endif
	void	__stdcall				script_gc					();			// GC-cycle

	IC CPHCommander					&ph_commander				();
	IC CPHCommander					&ph_commander_scripts		();
	IC CPHCommander					&ph_commander_physics_worldstep();

	// C/D
	CLevel();
	virtual ~CLevel();

	//названияе текущего уровня
	virtual shared_str			name					() const;
			shared_str			version					() const { return map_data.m_map_version.c_str(); } //this method can be used ONLY from CCC_ChangeGameType

	virtual void				GetLevelInfo		( CServerInfo* si );

	//gets the time from the game simulation
	
	//возвращает время в милисекундах относительно начала игры
	ALife::_TIME_ID		GetStartGameTime		();
	ALife::_TIME_ID		GetGameTime				();
	//возвращает время для энвайронмента в милисекундах относительно начала игры
	ALife::_TIME_ID		GetEnvironmentGameTime	();
	//игровое время в отформатированном виде
	void				GetGameDateTime			(u32& year, u32& month, u32& day, u32& hours, u32& mins, u32& secs, u32& milisecs);

	float				GetGameTimeFactor		();
	void				SetGameTimeFactor		(const float fTimeFactor);
	void				SetGameTimeFactor		(ALife::_TIME_ID GameTime, const float fTimeFactor);
	virtual void		SetEnvironmentGameTimeFactor(u64 const& GameTime, float const& fTimeFactor);

	// gets current daytime [0..23]
	u8					GetDayTime				();
	u32					GetGameDayTimeMS		();
	float				GetGameDayTimeSec		();
	float				GetEnvironmentGameDayTimeSec();

protected:
//	CFogOfWarMngr*		m_pFogOfWarMngr;
protected:	
	CMapManager *			m_map_manager;
	CGameTaskManager*		m_game_task_manager;

public:
	CMapManager&			MapManager					() const 	{return *m_map_manager;}
	CGameTaskManager&		GameTaskManager				() const	{return *m_game_task_manager;}
	void					OnAlifeSimulatorLoaded		();
	void					OnAlifeSimulatorUnLoaded	();
	//работа с пулями
protected:	
	CBulletManager*		m_pBulletManager;
public:
	IC CBulletManager&	BulletManager() {return	*m_pBulletManager;}

	//by Mad Max 
			bool			IsServer					();
			bool			IsClient					();
			CSE_Abstract	*spawn_item					(LPCSTR section, const Fvector &position, u32 level_vertex_id, u16 parent_id, bool return_item = false);
			
protected:
	u32		m_dwCL_PingDeltaSend;
	u32		m_dwCL_PingLastSendTime;
	u32		m_dwRealPing;
public:
	virtual	u32				GetRealPing					() { return m_dwRealPing; };

public:
			void			remove_objects				();
			virtual void	OnSessionTerminate		(LPCSTR reason);
			
			file_transfer::client_site*					m_file_transfer;
			
	compression::ppmd_trained_stream*			m_trained_stream;
	compression::lzo_dictionary_buffer			m_lzo_dictionary;
	//alligned to 16 bytes m_lzo_working_buffer
	u8*											m_lzo_working_memory;
	u8*											m_lzo_working_buffer;
	
	void			init_compression			();
	void			deinit_compression			();
			

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CLevel)
#undef script_type_list
#define script_type_list save_type_list(CLevel)

IC CLevel&				Level()		{ return *((CLevel*) g_pGameLevel);			}
IC game_cl_GameState&	Game()		{ return *Level().game;					}
	u32					GameID();


#ifdef DEBUG
IC CLevelDebug&			DBG()		{return *((CLevelDebug*)Level().m_level_debug);}
#endif


IC CSpaceRestrictionManager	&CLevel::space_restriction_manager()
{
	VERIFY				(m_space_restriction_manager);
	return				(*m_space_restriction_manager);
}

IC CSeniorityHierarchyHolder &CLevel::seniority_holder()
{
	VERIFY				(m_seniority_hierarchy_holder);
	return				(*m_seniority_hierarchy_holder);
}

IC CClientSpawnManager &CLevel::client_spawn_manager()
{
	VERIFY				(m_client_spawn_manager);
	return				(*m_client_spawn_manager);
}

IC CAutosaveManager &CLevel::autosave_manager()
{
	VERIFY				(m_autosave_manager);
	return				(*m_autosave_manager);
}

#ifdef DEBUG
IC CDebugRenderer &CLevel::debug_renderer()
{
	VERIFY				(m_debug_renderer);
	return				(*m_debug_renderer);
}
#endif

IC CPHCommander	& CLevel::ph_commander()
{
	VERIFY(m_ph_commander);
	return *m_ph_commander;
}
IC CPHCommander & CLevel::ph_commander_scripts()
{
	VERIFY(m_ph_commander_scripts);
	return *m_ph_commander_scripts;
}
IC CPHCommander & CLevel::ph_commander_physics_worldstep()
{
	VERIFY(m_ph_commander_scripts);
	return *m_ph_commander_physics_worldstep;
}
//by Mad Max 
IC bool		OnServer()			{ return Level().IsServer();}
IC bool		OnClient()			{ return Level().IsClient();}
IC bool		IsGameTypeSingle()	{ return (g_pGamePersistent->GameType() == eGameIDSingle);};

//class  CPHWorld;
//extern CPHWorld*				ph_world;
extern BOOL						g_bDebugEvents;

// -------------------------------------------------------------------------------------------------

#endif // !defined(AFX_LEVEL_H__38F63863_DB0C_494B_AFAB_C495876EC671__INCLUDED_)
