#pragma once

#include "weaponmagazinedwgrenade.h"
#include "script_export_space.h"

class CWeaponGroza :
	public CWeaponMagazinedWGrenade
{
	typedef CWeaponMagazinedWGrenade inherited;
public:
				CWeaponGroza();
	virtual		~CWeaponGroza();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponGroza)
#undef script_type_list
#define script_type_list save_type_list(CWeaponGroza)
