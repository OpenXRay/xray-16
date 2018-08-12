#pragma once

#include "WeaponCustomPistol.h"

class CWeaponShotgun : public CWeaponCustomPistol
{
    typedef CWeaponCustomPistol inherited;

public:
    CWeaponShotgun();
    virtual ~CWeaponShotgun();

    virtual void Load(LPCSTR section);

    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);

    virtual void Reload();
    virtual void switch2_Fire();
    void switch2_StartReload();
    void switch2_AddCartgidge();
    void switch2_EndReload();

    virtual void PlayAnimOpenWeapon();
    virtual void PlayAnimAddOneCartridgeWeapon();
    void PlayAnimCloseWeapon();

    virtual bool Action(u16 cmd, u32 flags);

protected:
    virtual void OnAnimationEnd(u32 state);
    void TriStateReload();
    virtual void OnStateSwitch(u32 S, u32 oldState);

    bool HaveCartridgeInInventory(u8 cnt);
    virtual u8 AddCartridge(u8 cnt);

    ESoundTypes m_eSoundOpen;
    ESoundTypes m_eSoundAddCartridge;
    ESoundTypes m_eSoundClose;
};
