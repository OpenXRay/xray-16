#include "stdafx.h"

#include "Entity.h"
#include "WeaponCustomPistol.h"

CWeaponCustomPistol::CWeaponCustomPistol() : CWeaponMagazined(SOUND_TYPE_WEAPON_PISTOL)
{
}

CWeaponCustomPistol::~CWeaponCustomPistol()
{
}
void CWeaponCustomPistol::switch2_Fire	()
{
	m_bFireSingleShot			= true;
	bWorking					= false;
	m_iShotNum					= 0;
	m_bStopedAfterQueueFired	= false;
}



void CWeaponCustomPistol::FireEnd() 
{
	if(fShotTimeCounter<=0) 
	{
		SetPending			(FALSE);
		inherited::FireEnd	();
	}
}