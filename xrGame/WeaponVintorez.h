#pragma once

#include "weaponmagazined.h"
#include "script_export_space.h"

class CWeaponVintorez :
	public CWeaponMagazined
{
	typedef CWeaponMagazined inherited;
public:
	CWeaponVintorez(void);
	virtual ~CWeaponVintorez(void);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponVintorez)
#undef script_type_list
#define script_type_list save_type_list(CWeaponVintorez)
