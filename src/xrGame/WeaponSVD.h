#pragma once

#include "WeaponCustomPistol.h"

class CWeaponSVD : public CWeaponCustomPistol
{
    typedef CWeaponCustomPistol inherited;

protected:
    virtual void switch2_Fire();
    virtual void OnAnimationEnd(u32 state);

public:
    CWeaponSVD(void);
    virtual ~CWeaponSVD(void);
};
