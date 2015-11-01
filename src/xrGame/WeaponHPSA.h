#ifndef __XR_WEAPON_HPSA_H__
#define __XR_WEAPON_HPSA_H__

#pragma once

#include "WeaponPistol.h"
 
class CWeaponHPSA: public CWeaponPistol
{
private:
	typedef CWeaponPistol inherited;
protected:
public:
					CWeaponHPSA			();
	virtual			~CWeaponHPSA		();
};

#endif //__XR_WEAPON_HPSA_H__
