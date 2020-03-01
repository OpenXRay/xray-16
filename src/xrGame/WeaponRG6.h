#pragma once

#include "RocketLauncher.h"
#include "WeaponShotgun.h"

class CWeaponRG6 : public CRocketLauncher, public CWeaponShotgun
{
    typedef CRocketLauncher inheritedRL;
    typedef CWeaponShotgun inheritedSG;

public:
    virtual ~CWeaponRG6();
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void Load(LPCSTR section);
    virtual void OnEvent(NET_Packet& P, u16 type);

protected:
    virtual void FireStart();
    virtual u8 AddCartridge(u8 cnt);
};
