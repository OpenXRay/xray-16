#include "StdAfx.h"
#include "WeaponCustomPistolAuto.h"

CWeaponCustomPistolAuto::CWeaponCustomPistolAuto() : CWeaponMagazined(SOUND_TYPE_WEAPON_PISTOL) {}

CWeaponCustomPistolAuto::~CWeaponCustomPistolAuto() {}

void CWeaponCustomPistolAuto::switch2_Fire()
{
    m_bFireSingleShot = true;
    //bWorking = false;
    m_iShotNum = 0;
    m_bStopedAfterQueueFired = false;
}

void CWeaponCustomPistolAuto::FireEnd()
{
    //if (fShotTimeCounter <= 0)
    {
        //SetPending(false);
        inherited::FireEnd();
    }
}

void CWeaponCustomPistolAuto::PlayAnimReload()
{
    inherited::PlayAnimReload();
} 
