#include "stdafx.h"

#include "Entity.h"
#include "WeaponCustomAuto.h"

CWeaponAutoPistol::CWeaponAutoPistol() : CWeaponMagazined(SOUND_TYPE_WEAPON_PISTOL)
{}

CWeaponAutoPistol::~CWeaponAutoPistol()
{}

void CWeaponAutoPistol::switch2_Fire()
{
    m_bFireSingleShot = true;
    //bWorking					= false;
    m_iShotNum = 0;
    m_bStopedAfterQueueFired = false;
}

void CWeaponAutoPistol::FireEnd()
{
    //if(fShotTimeCounter<=0)
    //{
    //SetPending			(FALSE);
    inherited::FireEnd();
    //}
}

void CWeaponAutoPistol::PlayAnimReload()
{
    inherited::PlayAnimReload();
}