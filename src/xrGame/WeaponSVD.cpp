#include "StdAfx.h"
#include "WeaponSVD.h"

CWeaponSVD::CWeaponSVD(void) {}
CWeaponSVD::~CWeaponSVD(void) {}
void CWeaponSVD::switch2_Fire()
{
    m_bFireSingleShot = true;
    bWorking = false;
    SetPending(TRUE);
    m_iShotNum = 0;
    m_bStopedAfterQueueFired = false;
}

void CWeaponSVD::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eFire: { SetPending(FALSE);
    }
    break; // End of reload animation
    }
    inherited::OnAnimationEnd(state);
}
