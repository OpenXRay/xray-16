#include "StdAfx.h"
#include "WeaponPistol.h"
#include "ParticlesObject.h"
#include "Actor.h"

CWeaponPistol::CWeaponPistol()
{
    m_eSoundClose = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
    SetPending(FALSE);
}

CWeaponPistol::~CWeaponPistol(void) {}
void CWeaponPistol::net_Destroy() { inherited::net_Destroy(); }
void CWeaponPistol::Load(LPCSTR section)
{
    inherited::Load(section);

    m_sounds.LoadSound(section, "snd_close", "sndClose", false, m_eSoundClose);
}

void CWeaponPistol::OnH_B_Chield() { inherited::OnH_B_Chield(); }
void CWeaponPistol::PlayAnimShow()
{
    VERIFY(GetState() == eShowing);

    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_show_empty", FALSE, this, GetState());
    else
        inherited::PlayAnimShow();
}

void CWeaponPistol::PlayAnimBore()
{
    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_bore_empty", TRUE, this, GetState());
    else
        inherited::PlayAnimBore();
}

void CWeaponPistol::PlayAnimIdleSprint()
{
    if (iAmmoElapsed == 0)
    {
        PlayHUDMotion("anm_idle_sprint_empty", TRUE, NULL, GetState());
    }
    else
    {
        inherited::PlayAnimIdleSprint();
    }
}

void CWeaponPistol::PlayAnimIdleMoving()
{
    if (iAmmoElapsed == 0)
    {
        PlayHUDMotion("anm_idle_moving_empty", TRUE, NULL, GetState());
    }
    else
    {
        inherited::PlayAnimIdleMoving();
    }
}

void CWeaponPistol::PlayAnimIdle()
{
    if (TryPlayAnimIdle())
        return;

    if (iAmmoElapsed == 0)
    {
        PlayHUDMotion("anm_idle_empty", TRUE, NULL, GetState());
    }
    else
    {
        inherited::PlayAnimIdle();
    }
}

void CWeaponPistol::PlayAnimAim()
{
    if (iAmmoElapsed == 0)
        PlayHUDMotion("anm_idle_aim_empty", TRUE, NULL, GetState());
    else
        inherited::PlayAnimAim();
}

void CWeaponPistol::PlayAnimReload()
{
    inherited::PlayAnimReload(); //AVO: refactored to use grand-parent (CWeaponMagazined) function
}

void CWeaponPistol::PlayAnimHide()
{
    VERIFY(GetState() == eHiding);
    if (iAmmoElapsed == 0)
    {
        PlaySound("sndClose", get_LastFP());
        PlayHUDMotion("anm_hide_empty", TRUE, this, GetState());
    }
    else
        inherited::PlayAnimHide();
}

void CWeaponPistol::PlayAnimShoot()
{
    VERIFY(GetState() == eFire);
    if (iAmmoElapsed > 1)
    {
        PlayHUDMotion("anm_shots", FALSE, this, GetState());
    }
    else
    {
        PlayHUDMotion("anm_shot_l", FALSE, this, GetState());
    }
}

void CWeaponPistol::switch2_Reload() { inherited::switch2_Reload(); }
void CWeaponPistol::OnAnimationEnd(u32 state) { inherited::OnAnimationEnd(state); }
void CWeaponPistol::OnShot()
{
    inherited::OnShot(); //Alundaio: not changed from inherited, so instead of copying changes from weaponmagazined, we just do this
}

void CWeaponPistol::UpdateSounds()
{
    inherited::UpdateSounds();
    m_sounds.SetPosition("sndClose", get_LastFP());
}
