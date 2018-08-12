#include "StdAfx.h"
#include "WeaponRevolver.h"
#include "ParticlesObject.h"
#include "Actor.h"

CWeaponRevolver::CWeaponRevolver()
{
    m_eSoundClose = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
    SetPending(false);
}

CWeaponRevolver::~CWeaponRevolver() {}

void CWeaponRevolver::net_Destroy()
{
    inherited::net_Destroy();
}


void CWeaponRevolver::Load(LPCSTR section)
{
    inherited::Load(section);

    m_sounds.LoadSound(section, "snd_close", "sndClose", false, m_eSoundClose);
}

void CWeaponRevolver::OnH_B_Chield()
{
    inherited::OnH_B_Chield();
}

void CWeaponRevolver::PlayAnimShow()
{
    VERIFY(GetState()==eShowing);

    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_show_empty", false, this, GetState());
    else
        inherited::PlayAnimShow();
}

void CWeaponRevolver::PlayAnimBore()
{
    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_bore_empty", true, this, GetState());
    else
        inherited::PlayAnimBore();
}

void CWeaponRevolver::PlayAnimIdleSprint()
{
    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_idle_sprint_empty", true, nullptr, GetState());
    else
        inherited::PlayAnimIdleSprint();
}

void CWeaponRevolver::PlayAnimIdleMoving()
{
    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_idle_moving_empty", true, nullptr, GetState());
    else
        inherited::PlayAnimIdleMoving();
}


void CWeaponRevolver::PlayAnimIdle()
{
    if (TryPlayAnimIdle()) return;

    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_idle_empty", true, nullptr, GetState());
    else
        inherited::PlayAnimIdle();
}

void CWeaponRevolver::PlayAnimAim()
{
    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_idle_aim_empty", true, nullptr, GetState());
    else
        inherited::PlayAnimAim();
}

void CWeaponRevolver::PlayAnimReload()
{
    auto state = GetState();
    VERIFY(state  == eReload);
    if (iAmmoElapsed == 1)
        PlayHUDMotion("anm_reload_1", true, this, state);
    else if (iAmmoElapsed == 2)
        PlayHUDMotion("anm_reload_2", true, this, state);
    else if (iAmmoElapsed == 3)
        PlayHUDMotion("anm_reload_3", true, this, state);
    else if (iAmmoElapsed == 4)
        PlayHUDMotion("anm_reload_4", true, this, state);
    else if (iAmmoElapsed == 5)
        PlayHUDMotion("anm_reload_5", true, this, state);
    else
        PlayHUDMotion("anm_reload", true, this, state);

}


void CWeaponRevolver::PlayAnimHide()
{
    VERIFY(GetState()==eHiding);
    if (iAmmoElapsed == 0)
    {
        PlaySound("sndClose", get_LastFP());
        PlayHUDMotion("anm_hide_empty", true, this, GetState());
    }
    else
        inherited::PlayAnimHide();
}

void CWeaponRevolver::PlayAnimShoot()
{
    VERIFY(GetState()==eFire);
    if (iAmmoElapsed > 1)
        PlayHUDMotion("anm_shots", false, this, GetState());
    else
        PlayHUDMotion("anm_shot_l", false, this, GetState());
}


void CWeaponRevolver::switch2_Reload()
{
    inherited::switch2_Reload();
}

void CWeaponRevolver::OnAnimationEnd(u32 state)
{
    inherited::OnAnimationEnd(state);
}

void CWeaponRevolver::OnShot()
{
    inherited::OnShot(); //Alundaio: not changed from inherited, so instead of copying changes from weaponmagazined, we just do this
}

void CWeaponRevolver::UpdateSounds()
{
    inherited::UpdateSounds();
    m_sounds.SetPosition("sndClose", get_LastFP());
}
