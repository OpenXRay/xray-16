#pragma once

#include "WeaponPistol.h"
#include "script_export_space.h"

class CWeaponFORT: public CWeaponPistol
{
private:
	typedef CWeaponPistol inherited;
protected:
public:
					CWeaponFORT			();
	virtual			~CWeaponFORT		();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponFORT)
#undef script_type_list
#define script_type_list save_type_list(CWeaponFORT)
