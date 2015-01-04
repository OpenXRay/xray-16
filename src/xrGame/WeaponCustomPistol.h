#pragma once
#include "WeaponMagazined.h"

class CWeaponCustomPistol : public CWeaponMagazined
{
    using inherited = CWeaponMagazined;

public:
    CWeaponCustomPistol();
    virtual ~CWeaponCustomPistol();
    int GetCurrentFireMode() override { return 1; }

protected:
    void FireEnd() override;
    void switch2_Fire() override;
};
