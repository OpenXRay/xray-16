#ifndef __XR_WEAPON_FN2000_H__
#define __XR_WEAPON_FN2000_H__

#pragma once

#include "WeaponMagazined.h"


class CWeaponFN2000: public CWeaponMagazined
{
private:
	typedef CWeaponMagazined inherited;
public:
					CWeaponFN2000	();
	virtual			~CWeaponFN2000	();
};

#endif //__XR_WEAPON_FN2000_H__
