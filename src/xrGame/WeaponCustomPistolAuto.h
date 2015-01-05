#pragma once

#include "WeaponMagazined.h"

class CWeaponCustomPistolAuto: public CWeaponMagazined
{
    using inherited = CWeaponMagazined;

public:
    CWeaponCustomPistolAuto();
    virtual ~CWeaponCustomPistolAuto();
    //int GetCurrentFireMode() override { return 1; };

protected:
    void FireEnd() override;
    void switch2_Fire() override;
    void PlayAnimReload() override;
};
