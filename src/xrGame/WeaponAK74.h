#pragma once

#include "WeaponMagazinedWGrenade.h"

class CWeaponAK74: public CWeaponMagazinedWGrenade
{
private:
	typedef CWeaponMagazinedWGrenade inherited;
public:
					CWeaponAK74		(ESoundTypes eSoundType=SOUND_TYPE_WEAPON_SUBMACHINEGUN);
	virtual			~CWeaponAK74	();
};
