#ifndef __XR_WEAPON_PM_H__
#define __XR_WEAPON_PM_H__

#pragma once

#include "WeaponPistol.h"

class CWeaponPM: public CWeaponPistol
{
private:
	typedef CWeaponPistol inherited;
protected:
public:
					CWeaponPM			();
	virtual			~CWeaponPM		();
};

#endif //__XR_WEAPON_PM_H__
