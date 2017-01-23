#pragma once

#include "weaponpistol.h"

class CWeaponWalther :
	public CWeaponPistol
{
	typedef CWeaponPistol inherited;
public:
	CWeaponWalther(void);
	virtual ~CWeaponWalther(void);
};
