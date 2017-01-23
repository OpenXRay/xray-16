#include "stdafx.h"
#include "weaponpistol.h"
#include "ParticlesObject.h"
#include "actor.h"

CWeaponPistol::CWeaponPistol()
{
	m_eSoundClose		= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
	SetPending			(FALSE);
}

CWeaponPistol::~CWeaponPistol(void)
{
}

void CWeaponPistol::net_Destroy()
{
	inherited::net_Destroy();
}


void CWeaponPistol::Load	(LPCSTR section)
{
	inherited::Load		(section);

	m_sounds.LoadSound(section, "snd_close", "sndClose", false, m_eSoundClose);
}

void CWeaponPistol::OnH_B_Chield		()
{
	inherited::OnH_B_Chield		();
}

void CWeaponPistol::PlayAnimShow	()
{
	VERIFY(GetState()==eShowing);

	if(iAmmoElapsed==0)
		PlayHUDMotion("anm_show_empty", FALSE, this, GetState());
	else
		inherited::PlayAnimShow();
}

void CWeaponPistol::PlayAnimBore()
{
	if(iAmmoElapsed==0)
		PlayHUDMotion	("anm_bore_empty", TRUE, this, GetState());
	else
		inherited::PlayAnimBore();
}

void CWeaponPistol::PlayAnimIdleSprint()
{
	if(iAmmoElapsed==0)
	{
		PlayHUDMotion("anm_idle_sprint_empty", TRUE, NULL, GetState());
	}else{
		inherited::PlayAnimIdleSprint();
	}
}

void CWeaponPistol::PlayAnimIdleMoving()
{
	if(iAmmoElapsed==0)
	{
		PlayHUDMotion("anm_idle_moving_empty", TRUE, NULL, GetState());
	}else{
		inherited::PlayAnimIdleMoving();
	}
}


void CWeaponPistol::PlayAnimIdle()
{
	if (TryPlayAnimIdle()) return;

	if(iAmmoElapsed==0)
	{
		PlayHUDMotion("anm_idle_empty", TRUE, NULL, GetState());
	}else{
		inherited::PlayAnimIdle		();
	}
}

void CWeaponPistol::PlayAnimAim()
{
	if(iAmmoElapsed==0)
		PlayHUDMotion("anm_idle_aim_empty", TRUE, NULL, GetState());
	else
		inherited::PlayAnimAim();
}

void CWeaponPistol::PlayAnimReload()
{	
	VERIFY(GetState()==eReload);
	if(iAmmoElapsed==0)
	{
		PlayHUDMotion("anm_reload_empty", TRUE, this, GetState());
	}else{
		PlayHUDMotion("anm_reload", TRUE, this, GetState());
	}
}


void CWeaponPistol::PlayAnimHide()
{
	VERIFY(GetState()==eHiding);
	if(iAmmoElapsed==0) 
	{
		PlaySound			("sndClose", get_LastFP());
		PlayHUDMotion		("anm_hide_empty" , TRUE, this, GetState());
	} 
	else 
		inherited::PlayAnimHide();
}

void CWeaponPistol::PlayAnimShoot	()
{
	VERIFY(GetState()==eFire);
	if(iAmmoElapsed > 1) 
	{
		PlayHUDMotion("anm_shots" , FALSE, this, GetState());
	}
	else 
	{
		PlayHUDMotion("anm_shot_l", FALSE, this, GetState()); 
	}
}


void CWeaponPistol::switch2_Reload()
{
	inherited::switch2_Reload();
}

void CWeaponPistol::OnAnimationEnd(u32 state)
{
	inherited::OnAnimationEnd(state);
}

void CWeaponPistol::OnShot		()
{
	PlaySound		(m_sSndShotCurrent.c_str(),get_LastFP());

	AddShotEffector	();
	
	PlayAnimShoot	();

	// Shell Drop
	Fvector vel; 
	PHGetLinearVell(vel);
	OnShellDrop					(get_LastSP(),  vel);

	// ќгонь из ствола
	
	StartFlameParticles	();
	R_ASSERT2(!m_pFlameParticles || !m_pFlameParticles->IsLooped(),
			  "can't set looped particles system for shoting with pistol");
	
	//дым из ствола
	StartSmokeParticles	(get_LastFP(), vel);
}

void CWeaponPistol::UpdateSounds()
{
	inherited::UpdateSounds();
	m_sounds.SetPosition("sndClose", get_LastFP());
}