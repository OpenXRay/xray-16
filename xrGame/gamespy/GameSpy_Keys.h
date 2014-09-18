#pragma once

#define MAX_REGISTERED_KEYS 254
#define NUM_RESERVED_KEYS 50


#define HOSTNAME_KEY	1
#define GAMENAME_KEY	2
#define GAMEVER_KEY		3
#define HOSTPORT_KEY	4
#define MAPNAME_KEY		5
#define GAMETYPE_KEY	6
#define GAMEVARIANT_KEY	7
#define NUMPLAYERS_KEY	8
#define NUMTEAMS_KEY	9
#define MAXPLAYERS_KEY	10
#define GAMEMODE_KEY	11
#define TEAMPLAY_KEY	12
#define FRAGLIMIT_KEY	13
#define TEAMFRAGLIMIT_KEY	14
#define TIMEELAPSED_KEY	15
#define TIMELIMIT_KEY	16
#define ROUNDTIME_KEY	17
#define ROUNDELAPSED_KEY	18
#define PASSWORD_KEY	19
#define GROUPID_KEY		20
#define PLAYER__KEY		21
#define SCORE__KEY		22
#define SKILL__KEY		23
#define PING__KEY		24
#define TEAM__KEY		25
#define DEATHS__KEY		26
#define PID__KEY		27
#define TEAM_T_KEY		28
#define SCORE_T_KEY		29

//----- ADDITIONAL KEYS ---------------------
//---- Game Keys
//---- Global Keys ----
#define GAMETYPE_NAME_KEY						100
#define	DEDICATED_KEY							101
//---- game_sv_base ---							
#define	G_MAP_ROTATION_KEY						102
#define	G_VOTING_ENABLED_KEY					103
//---- game sv mp ----							
#define	G_SPECTATOR_MODES_KEY					104
//---- game_sv_deathmatch ----					
#define	G_FRAG_LIMIT_KEY						105
#define	G_TIME_LIMIT_KEY						106
#define	G_DAMAGE_BLOCK_TIME_KEY					107
#define		G_DAMAGE_BLOCK_INDICATOR_KEY		108
#define	G_ANOMALIES_ENABLED_KEY					109
#define		G_ANOMALIES_TIME_KEY				110
#define	G_WARM_UP_TIME_KEY						111
#define	G_FORCE_RESPAWN_KEY						112
//---- game_sv_teamdeathmatch ----				
#define	G_AUTO_TEAM_BALANCE_KEY					113
#define	G_AUTO_TEAM_SWAP_KEY					114
#define	G_FRIENDLY_INDICATORS_KEY				115
#define	G_FRIENDLY_NAMES_KEY					116
#define	G_FRIENDLY_FIRE_KEY						117
//---- game_sv_artefacthunt ----				
#define	G_ARTEFACTS_COUNT_KEY					118
#define	G_ARTEFACT_STAY_TIME_KEY				119
#define	G_ARTEFACT_RESPAWN_TIME_KEY				120
#define	G_REINFORCEMENT_KEY						121
#define	G_SHIELDED_BASES_KEY					122
#define	G_RETURN_PLAYERS_KEY					123
#define	G_BEARER_CANT_SPRINT_KEY				124
												
//---- Player keys								
//#define P_NAME__KEY								125
//#define P_FRAGS__KEY							126
//#define P_DEATH__KEY							127
//#define P_RANK__KEY								128
//#define P_TEAM__KEY								129
#define P_SPECTATOR__KEY						130
#define P_ARTEFACTS__KEY						131

//---- Team keys
//#define T_NAME_KEY							
#define T_SCORE_T_KEY							132

#define	G_MAX_PING_KEY							133
#define	G_BATTLEYE_KEY							134
#define	G_USER_PASSWORD_KEY						135
#define SERVER_UP_TIME_KEY						136
