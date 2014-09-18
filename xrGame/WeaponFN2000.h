#ifndef __XR_WEAPON_FN2000_H__
#define __XR_WEAPON_FN2000_H__

#pragma once

#include "WeaponMagazined.h"
#include "script_export_space.h"

class CWeaponFN2000: public CWeaponMagazined
{
private:
	typedef CWeaponMagazined inherited;
public:
					CWeaponFN2000	();
	virtual			~CWeaponFN2000	();
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponFN2000)
#undef script_type_list
#define script_type_list save_type_list(CWeaponFN2000)

#endif //__XR_WEAPON_FN2000_H__
