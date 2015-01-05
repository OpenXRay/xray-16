#pragma once

#include "WeaponMagazined.h"

class CWeaponAutoPistol : public CWeaponMagazined
{
private:
    typedef CWeaponMagazined inherited;
public:
    CWeaponAutoPistol();
    virtual			~CWeaponAutoPistol();
    //virtual	int		GetCurrentFireMode	() { return 1; };
protected:
    virtual void FireEnd();
    virtual void switch2_Fire();
    virtual void PlayAnimReload();
};
