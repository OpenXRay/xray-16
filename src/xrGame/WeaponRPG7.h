#pragma once

#include "WeaponCustomPistol.h"
#include "RocketLauncher.h"

class CWeaponRPG7 : public CWeaponCustomPistol, public CRocketLauncher
{
    using inherited = CWeaponCustomPistol;

public:
    CWeaponRPG7();
    virtual ~CWeaponRPG7();

    virtual bool net_Spawn(CSE_Abstract* DC);
    virtual void OnStateSwitch(u32 S, u32 oldState);
    virtual void OnEvent(NET_Packet& P, u16 type);
    virtual void ReloadMagazine();
    virtual void Load(LPCSTR section);
    virtual void switch2_Fire();
    virtual void FireTrace(const Fvector& P, const Fvector& D);
    virtual void on_a_hud_attach();

    virtual void FireStart();
    virtual void SwitchState(u32 S);

    void UpdateMissileVisibility();
    virtual void UnloadMagazine(bool spawn_ammo = true);

    virtual void net_Import(NET_Packet& P); // import from server
protected:
    virtual bool AllowBore();
    virtual void PlayAnimReload();

    shared_str m_sRocketSection;
};
