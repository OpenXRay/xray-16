#ifndef _INCDEF_XRMESSAGES_H_
#define _INCDEF_XRMESSAGES_H_

#pragma once

// CL	== client 2 server message
// SV	== server 2 client message

enum {
	M_UPDATE			= 0,	// DUAL: Update state
	M_SPAWN,					// DUAL: Spawning, full state

	M_SV_CONFIG_NEW_CLIENT,
	M_SV_CONFIG_GAME,
	M_SV_CONFIG_FINISHED,

	M_MIGRATE_DEACTIVATE,		// TO:   Changing server, just deactivate
	M_MIGRATE_ACTIVATE,			// TO:   Changing server, full state

	M_CHAT,						// DUAL:

	M_EVENT,					// Game Event
	M_CL_INPUT,					// Client Input Data
	//----------- for E3 -----------------------------
	M_CL_UPDATE,
	M_UPDATE_OBJECTS,
	//-------------------------------------------------
	M_CLIENTREADY,				// Client has finished to load level and are ready to play
	
	M_CHANGE_LEVEL,				// changing level
	M_LOAD_GAME,
	M_RELOAD_GAME,
	M_SAVE_GAME,
	M_SAVE_PACKET,

	M_SWITCH_DISTANCE,
	M_GAMEMESSAGE,					// Game Message
	M_EVENT_PACK,					// Pack of M_EVENT

	//-----------------------------------------------------
	M_GAMESPY_CDKEY_VALIDATION_CHALLENGE,
	M_GAMESPY_CDKEY_VALIDATION_CHALLENGE_RESPOND,
	M_CLIENT_CONNECT_RESULT,
	M_CLIENT_REQUEST_CONNECTION_DATA,

	M_CHAT_MESSAGE,
	M_CLIENT_WARN,
	M_CHANGE_LEVEL_GAME,
	//-----------------------------------------------------
	M_CL_PING_CHALLENGE,
	M_CL_PING_CHALLENGE_RESPOND,
	//-----------------------------------------------------
	M_AUTH_CHALLENGE,
	M_CL_AUTH,
	M_BULLET_CHECK_RESPOND,
	//-----------------------------------------------------
	M_STATISTIC_UPDATE,
	M_STATISTIC_UPDATE_RESPOND,
	//-----------------------------------------------------
	M_PLAYER_FIRE,
	//-----------------------------------------------------
	M_MOVE_PLAYERS,
	M_MOVE_ARTEFACTS,
	M_MOVE_PLAYERS_RESPOND,
	//-----------------------------------------------------
	M_CHANGE_SELF_NAME,
	M_REMOTE_CONTROL_AUTH,
	M_REMOTE_CONTROL_CMD,
	M_BATTLEYE,

	M_SV_MAP_NAME,				//request and responce
	M_SV_DIGEST,				//request and responce

	M_FILE_TRANSFER,
	M_MAKE_SCREENSHOT,
	M_SECURE_KEY_SYNC,
	M_SECURE_MESSAGE,
	M_CREATE_PLAYER_STATE,
	M_COMPRESSED_UPDATE_OBJECTS,

	MSG_FORCEDWORD				= u32(-1)
};

enum {
	GE_RESPAWN,
	GE_OWNERSHIP_TAKE,			// DUAL: Client request for ownership of an item
	GE_OWNERSHIP_TAKE_MP_FORCED,
	GE_OWNERSHIP_REJECT,		// DUAL: Client request ownership rejection
	GE_TRANSFER_AMMO,			// DUAL: Take ammo out of weapon for our weapon
	GE_HIT,						//
	GE_DIE,						//
	GE_ASSIGN_KILLER,			//
	GE_DESTROY,					// authorative client request for entity-destroy
	GE_DESTROY_REJECT,			// GE_DESTROY + GE_OWNERSHIP_REJECT
	GE_TELEPORT_OBJECT,

	GE_ADD_RESTRICTION,
	GE_REMOVE_RESTRICTION,
	GE_REMOVE_ALL_RESTRICTIONS,

	GE_BUY,


	GE_INFO_TRANSFER,			//transfer _new_ info on PDA
	
	GE_TRADE_SELL,
	GE_TRADE_BUY,

	GE_WPN_AMMO_ADD,
	GE_WPN_STATE_CHANGE,

	GE_ADDON_ATTACH,
	GE_ADDON_DETACH,
	GE_ADDON_CHANGE,
	GE_INSTALL_UPGRADE,
	
	GE_GRENADE_EXPLODE,
	GE_INV_ACTION,				//a action beign taken on inventory
	GE_INV_BOX_STATUS,
	GE_INV_OWNER_STATUS,

	GE_ZONE_STATE_CHANGE,

	GE_MOVE_ACTOR,				//move actor to desired position instantly
	GE_ACTOR_JUMPING,			//actor press jump key
	GE_ACTOR_MAX_POWER,
	GE_ACTOR_MAX_HEALTH,

	GE_CHANGE_POS,

	GE_GAME_EVENT,

	GE_CHANGE_VISUAL,
	GE_MONEY,

	GEG_PLAYER_ACTIVATE_SLOT,
	GEG_PLAYER_ITEM2SLOT,
	GEG_PLAYER_ITEM2BELT,
	GEG_PLAYER_ITEM2RUCK,
	GEG_PLAYER_ITEM_EAT,
	GEG_PLAYER_ITEM_SELL,
	GEG_PLAYER_ACTIVATEARTEFACT,

	GEG_PLAYER_WEAPON_HIDE_STATE,
	GEG_PLAYER_DISABLE_SPRINT,
	

	GEG_PLAYER_ATTACH_HOLDER,
	GEG_PLAYER_DETACH_HOLDER,

	GEG_PLAYER_PLAY_HEADSHOT_PARTICLE,
	//-------------------------------------
	GE_HIT_STATISTIC,
	//-------------------------------------
	GE_KILL_SOMEONE,

	GE_FREEZE_OBJECT,
	GE_LAUNCH_ROCKET,
	
	GEG_PLAYER_USE_BOOSTER,
	GE_REQUEST_PLAYERS_INFO,

	GE_FORCEDWORD				= u32(-1)
};


enum EGameMessages {  //game_cl <----> game_sv messages
	GAME_EVENT_PLAYER_READY,	
	GAME_EVENT_PLAYER_KILL,			//player wants to die
	GAME_EVENT_PLAYER_BUY_FINISHED,	//player end to buy items
	GAME_EVENT_PLAYER_BUYMENU_OPEN,
	GAME_EVENT_PLAYER_BUYMENU_CLOSE,
	GAME_EVENT_PLAYER_BUY_SPAWN,

	GAME_EVENT_PLAYER_GAME_MENU,
	GAME_EVENT_PLAYER_GAME_MENU_RESPOND,	

	GAME_EVENT_PLAYER_CONNECTED	,
	GAME_EVENT_PLAYER_DISCONNECTED	,
	GAME_EVENT_PLAYER_ENTERED_GAME	,
	
	GAME_EVENT_PLAYER_KILLED			,//////!!!!!
	GAME_EVENT_PLAYER_HITTED,

	GAME_EVENT_PLAYER_JOIN_TEAM		,
	GAME_EVENT_ROUND_STARTED		,
	GAME_EVENT_ROUND_END		,
	
	GAME_EVENT_ARTEFACT_SPAWNED		,
	GAME_EVENT_ARTEFACT_DESTROYED		,
	GAME_EVENT_ARTEFACT_TAKEN,
	GAME_EVENT_ARTEFACT_DROPPED		,
	GAME_EVENT_ARTEFACT_ONBASE		,
	
	GAME_EVENT_PLAYER_ENTER_TEAM_BASE	,
	GAME_EVENT_PLAYER_LEAVE_TEAM_BASE	,

	GAME_EVENT_BUY_MENU_CLOSED		,
	GAME_EVENT_TEAM_MENU_CLOSED		,
	GAME_EVENT_SKIN_MENU_CLOSED		,

	GAME_EVENT_CREATE_CLIENT,
	GAME_EVENT_ON_HIT,
	GAME_EVENT_ON_TOUCH,

	GAME_EVENT_VOTE_START,
	GAME_EVENT_VOTE_YES,
	GAME_EVENT_VOTE_NO,
	GAME_EVENT_VOTE_STOP,
	GAME_EVENT_VOTE_END,
	GAME_EVENT_GET_ACTIVE_VOTE,
//	GAME_EVENT_VOTE_PLAYER_VOTED,

	GAME_EVENT_PLAYER_AUTH,
	GAME_EVENT_PLAYER_NAME,

	GAME_EVENT_SPEECH_MESSAGE,

	//-----------------------------------------
	GAME_EVENT_PLAYERS_MONEY_CHANGED,
	GAME_EVENT_SERVER_STRING_MESSAGE,
	GAME_EVENT_SERVER_DIALOG_MESSAGE,

	GAME_EVENT_PLAYER_STARTED,
	GAME_EVENT_MAKE_DATA,
	GAME_EVENT_RECEIVE_SERVER_LOGO,
	GAME_EVENT_CREATE_PLAYER_STATE,
	GAME_EVENT_PLAYERS_INFO_REPLY,

	//-----------------------------------------
	GAME_EVENT_SCRIPT_BEGINS_FROM,		// don't add messages after this
	GAME_EVENT_FORCEDWORD				= u32(-1)
};

enum
{
	M_SPAWN_OBJECT_LOCAL		= (1<<0),	// after spawn it becomes local (authorative)
	M_SPAWN_OBJECT_HASUPDATE	= (1<<2),	// after spawn info it has update inside message
	M_SPAWN_OBJECT_ASPLAYER		= (1<<3),	// after spawn it must become viewable
	M_SPAWN_OBJECT_PHANTOM		= (1<<4),	// after spawn it must become viewable
	M_SPAWN_VERSION				= (1<<5),	// control version
	M_SPAWN_UPDATE				= (1<<6),	// + update packet
	M_SPAWN_TIME				= (1<<7),	// + spawn time
	M_SPAWN_DENIED				= (1<<8),	// don't spawn entity with this flag

	M_SPAWN_OBJECT_FORCEDWORD	= u32(-1)
};

enum enum_connection_results
{
	ecr_data_verification_failed		=	0x00,
	ecr_cdkey_validation_failed,
	ecr_password_verification_failed,
	ecr_have_been_banned,
	ecr_profile_error,
};//enum enum_connection_results


#endif /*_INCDEF_XRMESSAGES_H_*/