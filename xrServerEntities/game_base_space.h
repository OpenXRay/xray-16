#pragma once
/*
enum EGameIDs {
	eGameIDNoGame						= u32(0),
	eGameIDSingle						= u32(1) << 0,
	eGameIDDeathmatch					= u32(1) << 1,
	eGameIDTeamDeathmatch				= u32(1) << 2,
	eGameIDArtefactHunt					= u32(1) << 3,
	eGameIDCaptureTheArtefact			= u32(1) << 4,
	eGameIDDominationZone				= u32(1) << 5,
	eGameIDTeamDominationZone			= u32(1) << 6,
};*/

enum EGamePlayerFlags
{
	GAME_PLAYER_FLAG_LOCAL				= (1<<0),
	GAME_PLAYER_FLAG_READY				= (1<<1),
	GAME_PLAYER_FLAG_VERY_VERY_DEAD		= (1<<2),
	GAME_PLAYER_FLAG_SPECTATOR			= (1<<3),

	GAME_PLAYER_FLAG_SCRIPT_BEGINS_FROM	= (1<<4),
	GAME_PLAYER_FLAG_INVINCIBLE			= (1<<5),
	GAME_PLAYER_FLAG_ONBASE				= (1<<6),
	GAME_PLAYER_FLAG_SKIP				= (1<<7),
	GAME_PLAYER_HAS_ADMIN_RIGHTS		= (1<<8),

	GAME_PLAYER_FLAG_FORCEDWORD			= u32(-1)
};

enum EGamePhases
{
	GAME_PHASE_NONE						= 0,
	GAME_PHASE_INPROGRESS,
	GAME_PHASE_PENDING,
	GAME_PHASE_TEAM1_SCORES,
	GAME_PHASE_TEAM2_SCORES,
	GAME_PHASE_TEAM1_ELIMINATED,
	GAME_PHASE_TEAM2_ELIMINATED,
	GAME_PHASE_TEAMS_IN_A_DRAW,
	GAME_PHASE_PLAYER_SCORES,

	GAME_PHASE_SCRIPT_BEGINS_FROM,
	GAME_PHASE_FORCEDWORD				= u32(-1)
};
