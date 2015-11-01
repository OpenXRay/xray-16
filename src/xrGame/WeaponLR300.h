#ifndef __XR_WEAPON_LR300_H__
#define __XR_WEAPON_LR300_H__
#pragma once

#include "WeaponMagazined.h"

class CWeaponLR300: public CWeaponMagazined
{
private:
	typedef CWeaponMagazined inherited;
public:
	/*
	virtual	void	UpdateCL			();
	virtual void	renderable_Render	();
	virtual void	spatial_move		();
	virtual void	spatial_register	();
	virtual void	spatial_unregister	();
	*/

					CWeaponLR300		();
	virtual			~CWeaponLR300		();
};

#endif //__XR_WEAPON_LR300_H__
