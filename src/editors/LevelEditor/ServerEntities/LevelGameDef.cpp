#include "stdafx.h"
#include "Common/LevelGameDef.h"

xr_token rpoint_type[] =
	{
		{"Actor Spawn", rptActorSpawn},
		{"Artefact Spawn", rptArtefactSpawn},
		{"Item Spawn", rptItemSpawn},
		{0, 0}};

xr_token rpoint_type_soc[] =
	{
		{"Actor Spawn", rptActorSpawn},
		{"Artefact Spawn", rptArtefactSpawn},
		{"TeamBase Particle", rptTeamBaseParticle},
		{0, 0}};
