#include "StdAfx.h"
#include "weaponBM16.h"

CWeaponBM16::~CWeaponBM16() {}
void CWeaponBM16::Load(LPCSTR section)
{
    inherited::Load(section);
    m_sounds.LoadSound(section, "snd_reload_1", "sndReload1", true, m_eSoundShot);
}

void CWeaponBM16::PlayReloadSound()
{
    if (m_magazine.size() == 1)
        PlaySound("sndReload1", get_LastFP());
    else
        PlaySound("sndReload", get_LastFP());
}

void CWeaponBM16::PlayAnimShoot()
{
    switch (m_magazine.size())
    {
    case 1: PlayHUDMotion("anm_shot_1", FALSE, this, GetState()); break;
    case 2: PlayHUDMotion("anm_shot_2", FALSE, this, GetState()); break;
    }
}

void CWeaponBM16::PlayAnimShow()
{
    switch (m_magazine.size())
    {
    case 0: PlayHUDMotion("anm_show_0", TRUE, this, GetState()); break;
    case 1: PlayHUDMotion("anm_show_1", TRUE, this, GetState()); break;
    case 2: PlayHUDMotion("anm_show_2", TRUE, this, GetState()); break;
    }
}

void CWeaponBM16::PlayAnimHide()
{
    switch (m_magazine.size())
    {
    case 0: PlayHUDMotion("anm_hide_0", TRUE, this, GetState()); break;
    case 1: PlayHUDMotion("anm_hide_1", TRUE, this, GetState()); break;
    case 2: PlayHUDMotion("anm_hide_2", TRUE, this, GetState()); break;
    }
}

void CWeaponBM16::PlayAnimBore()
{
    switch (m_magazine.size())
    {
    case 0: PlayHUDMotion("anm_bore_0", TRUE, this, GetState()); break;
    case 1: PlayHUDMotion("anm_bore_1", TRUE, this, GetState()); break;
    case 2: PlayHUDMotion("anm_bore_2", TRUE, this, GetState()); break;
    }
}

void CWeaponBM16::PlayAnimReload()
{
    bool b_both = HaveCartridgeInInventory(2);

    VERIFY(GetState() == eReload);

    if ((m_magazine.size() == 1 || !b_both) &&
        (m_set_next_ammoType_on_reload == undefined_ammo_type || m_ammoType == m_set_next_ammoType_on_reload))
        PlayHUDMotion("anm_reload_1", TRUE, this, GetState());
    else
        PlayHUDMotion("anm_reload_2", TRUE, this, GetState());
}

void CWeaponBM16::PlayAnimIdleMoving()
{
    switch (m_magazine.size())
    {
    case 0: PlayHUDMotion("anm_idle_moving_0", TRUE, this, GetState()); break;
    case 1: PlayHUDMotion("anm_idle_moving_1", TRUE, this, GetState()); break;
    case 2: PlayHUDMotion("anm_idle_moving_2", TRUE, this, GetState()); break;
    }
}

void CWeaponBM16::PlayAnimIdleSprint()
{
    switch (m_magazine.size())
    {
    case 0: PlayHUDMotion("anm_idle_sprint_0", TRUE, this, GetState()); break;
    case 1: PlayHUDMotion("anm_idle_sprint_1", TRUE, this, GetState()); break;
    case 2: PlayHUDMotion("anm_idle_sprint_2", TRUE, this, GetState()); break;
    }
}

void CWeaponBM16::PlayAnimIdle()
{
    if (TryPlayAnimIdle())
        return;

    if (IsZoomed())
    {
        switch (m_magazine.size())
        {
        case 0: { PlayHUDMotion("anm_idle_aim_0", TRUE, NULL, GetState());
        }
        break;
        case 1: { PlayHUDMotion("anm_idle_aim_1", TRUE, NULL, GetState());
        }
        break;
        case 2: { PlayHUDMotion("anm_idle_aim_2", TRUE, NULL, GetState());
        }
        break;
        };
    }
    else
    {
        switch (m_magazine.size())
        {
        case 0: { PlayHUDMotion("anm_idle_0", TRUE, NULL, GetState());
        }
        break;
        case 1: { PlayHUDMotion("anm_idle_1", TRUE, NULL, GetState());
        }
        break;
        case 2: { PlayHUDMotion("anm_idle_2", TRUE, NULL, GetState());
        }
        break;
        };
    }
}
