#pragma once
#include "WeaponCustomPistol.h"

class CWeaponRevolver : public CWeaponCustomPistol
{
    using inherited = CWeaponCustomPistol;

public:
    CWeaponRevolver();
    virtual ~CWeaponRevolver();

    void Load(LPCSTR section) override;

    void switch2_Reload() override;

    void OnShot() override;
    void OnAnimationEnd(u32 state) override;
    void net_Destroy() override;
    void OnH_B_Chield() override;

    //анимации
    void PlayAnimShow() override;
    void PlayAnimIdle() override;
    void PlayAnimIdleMoving() override;
    void PlayAnimIdleSprint() override;
    void PlayAnimHide() override;
    void PlayAnimReload() override;
    void PlayAnimShoot() override;
    void PlayAnimBore() override;
    void PlayAnimAim() override;
    //virtual void PlayReloadSound();
    void UpdateSounds() override;

protected:
    bool AllowFireWhileWorking() override { return true; }

    ESoundTypes m_eSoundClose;
    //ESoundTypes m_eSoundReloadEmpty;
};
