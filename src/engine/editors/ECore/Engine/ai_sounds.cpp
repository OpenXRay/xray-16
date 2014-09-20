#include "stdafx.h"
#pragma hdrstop

#include "ai_sounds.h"

xr_token anomaly_type_token[]={
	{ "undefined",				sg_Undefined						},
	{ "Item picking up",		SOUND_TYPE_ITEM_PICKING_UP			},
	{ "Item dropping",			SOUND_TYPE_ITEM_DROPPING			},
	{ "Item taking",			SOUND_TYPE_ITEM_TAKING				},
	{ "Item hiding",			SOUND_TYPE_ITEM_HIDING				},
	{ "Item using",				SOUND_TYPE_ITEM_USING				},
	{ "Weapon shooting",		SOUND_TYPE_WEAPON_SHOOTING			},
	{ "Weapon empty clicking",	SOUND_TYPE_WEAPON_EMPTY_CLICKING	},
	{ "Weapon bullet hit",		SOUND_TYPE_WEAPON_BULLET_HIT		},
	{ "Weapon recharging",		SOUND_TYPE_WEAPON_RECHARGING		},
	{ "NPC dying",				SOUND_TYPE_MONSTER_DYING			},
	{ "NPC injuring",			SOUND_TYPE_MONSTER_INJURING			},
	{ "NPC step",				SOUND_TYPE_MONSTER_STEP				},
	{ "NPC talking",			SOUND_TYPE_MONSTER_TALKING			},
	{ "NPC attacking",			SOUND_TYPE_MONSTER_ATTACKING		},
	{ "NPC eating",				SOUND_TYPE_MONSTER_EATING			},
	{ "Anomaly idle",			SOUND_TYPE_ANOMALY_IDLE				},
	{ "Object breaking",		SOUND_TYPE_WORLD_OBJECT_BREAKING	},
	{ "Object colliding",		SOUND_TYPE_WORLD_OBJECT_COLLIDING	},
	{ "Object exploding",		SOUND_TYPE_WORLD_OBJECT_EXPLODING	},
	{ "World ambient",			SOUND_TYPE_WORLD_AMBIENT			},
	{ 0,						0}
};
