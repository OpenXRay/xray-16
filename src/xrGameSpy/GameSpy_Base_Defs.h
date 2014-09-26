#pragma once

#include "GameSpy_Keys.h"

#ifdef DEMO_BUILD
	#define	GAMESPY_GAMENAME		"stalkerscd"
	#define GAMESPY_GAMEID			1567
	#define GAMESPY_PRODUCTID		10954
#else
	#define	GAMESPY_GAMENAME		"stalkersc"
	#define GAMESPY_GAMEID			1067
	#define GAMESPY_PRODUCTID		10953
#endif

#define GAMESPY_MAX_UPDATES			20


#define GAMESPY_PATCHING_VERSIONUNIQUE_ID	"test_version_1"
#define GAMESPY_PATCHING_DISTRIBUTION_ID	0

#define	GAMESPY_BASEPORT			5447
#define START_PORT					5445
#define END_PORT					START_PORT + 100//GameSpy only process 500 ports

static unsigned char Fields_Of_Interest[] = 
{ 
	HOSTNAME_KEY, 
		NUMPLAYERS_KEY, 
		MAXPLAYERS_KEY, 
		MAPNAME_KEY, 
		GAMETYPE_KEY, 
		PASSWORD_KEY
};











