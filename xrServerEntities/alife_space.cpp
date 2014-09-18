#include "stdafx.h"
#include "alife_space.h"

namespace ALife {

xr_token							hit_types_token							[ ]={
	{ "burn",						eHitTypeBurn								},
	{ "shock",						eHitTypeShock								},
	{ "strike",						eHitTypeStrike								},
	{ "wound",						eHitTypeWound								},
	{ "radiation",					eHitTypeRadiation							},
	{ "telepatic",					eHitTypeTelepatic							},
	{ "fire_wound",					eHitTypeFireWound							},
	{ "chemical_burn",				eHitTypeChemicalBurn						},
	{ "explosion",					eHitTypeExplosion							},
	{ "wound_2",					eHitTypeWound_2								},
//	{ "physic_strike",				eHitTypePhysicStrike						},
	{ "light_burn",					eHitTypeLightBurn							},
	{ 0,							0											}
};

};