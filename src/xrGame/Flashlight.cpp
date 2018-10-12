#include "stdafx.h"
#include "Flashlight.h"
#include "inventory.h"
#include "player_hud.h"
#include "weapon.h"
#include "hudsound.h"
#include "ai_sounds.h"
#include "../xrEngine/LightAnimLibrary.h"
#include "../xrEngine/camerabase.h"
#include "../xrengine/xr_collide_form.h"

//ENGINE_API int g_current_renderer;

CFlashlight::CFlashlight()
{
	m_bFastAnimMode = false;
	m_bNeedActivation = false;
    
	light_render = GEnv.Render->light_create();
	light_render->set_type(IRender_Light::SPOT);
	light_render->set_shadow(true);
    light_omni = GEnv.Render->light_create();
	light_omni->set_type(IRender_Light::POINT);
	light_omni->set_shadow(false);

	m_switched_on = false;
    glow_render = GEnv.Render->glow_create();
	lanim = 0;
	fBrightness = 1.f;

	m_light_section = "torch_definition";
}

CFlashlight::~CFlashlight()
{
	Switch(false,false);
	light_render.destroy();
	light_omni.destroy();
	glow_render.destroy();
}

bool CFlashlight::CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate)
{
	if (itm == NULL)
		return true;

	CInventoryItem& iitm = itm->item();
	u32 slot = iitm.BaseSlot();
	bool bres = (slot == INV_SLOT_2 || slot == KNIFE_SLOT || slot == BOLT_SLOT);
	if (!bres && slot_to_activate)
	{
		*slot_to_activate = NO_ACTIVE_SLOT;
		if (m_pInventory->ItemFromSlot(BOLT_SLOT))
			*slot_to_activate = BOLT_SLOT;

		if (m_pInventory->ItemFromSlot(KNIFE_SLOT))
			*slot_to_activate = KNIFE_SLOT;

		if (m_pInventory->ItemFromSlot(INV_SLOT_3) && m_pInventory->ItemFromSlot(INV_SLOT_3)->BaseSlot() != INV_SLOT_3)
			*slot_to_activate = INV_SLOT_3;

		if (m_pInventory->ItemFromSlot(INV_SLOT_2) && m_pInventory->ItemFromSlot(INV_SLOT_2)->BaseSlot() != INV_SLOT_3)
			*slot_to_activate = INV_SLOT_2;

		if (*slot_to_activate != NO_ACTIVE_SLOT)
			bres = true;
	}

	if (itm->GetState() != CHUDState::eShowing)
		bres = bres && !itm->IsPending();

	if (bres)
	{
		CWeapon* W = smart_cast<CWeapon*>(itm);
		if (W)
			bres = bres && (W->GetState() != CHUDState::eBore) && (W->GetState() != CWeapon::eReload) && (W->GetState() != CWeapon::eSwitch) /*&& !W->IsZoomed()*/;
	}
	return bres;
}

bool  CFlashlight::CheckCompatibility(CHudItem* itm)
{
	if (!inherited::CheckCompatibility(itm))
		return false;

	if (!CheckCompatibilityInt(itm, NULL))
	{
		HideDevice(true);
		return			false;
	}
	return true;
}

void CFlashlight::HideDevice(bool bFastMode)
{
	if (GetState() == eIdle)
		ToggleDevice(bFastMode);
}

void CFlashlight::ShowDevice(bool bFastMode)
{
	if (GetState() == eHidden)
		ToggleDevice(bFastMode);
}

void CFlashlight::ToggleDevice(bool bFastMode)
{
	m_bNeedActivation = false;
	m_bFastAnimMode = bFastMode;

	if (GetState() == eHidden)
	{
		PIItem iitem = m_pInventory->ActiveItem();
		CHudItem* itm = (iitem) ? iitem->cast_hud_item() : NULL;
		u16 slot_to_activate = NO_ACTIVE_SLOT;

		if (CheckCompatibilityInt(itm, &slot_to_activate))
		{
			if (slot_to_activate != NO_ACTIVE_SLOT)
			{
				m_pInventory->Activate(slot_to_activate);
				m_bNeedActivation = true;
			}
			else
			{
				SwitchState(eShowing);
			}
		}
	}
	else
		if (GetState() == eIdle)
			SwitchState(eSwitchOff);

}

void CFlashlight::OnStateSwitch(u32 S, u32 oldState)
{
	inherited::OnStateSwitch(S, oldState);

	switch (S)
	{
		case eShowing:
		{
			g_player_hud->attach_item(this);
			m_sounds.PlaySound("sndShow", Fvector().set(0, 0, 0), this, true, false);
			PlayHUDMotion(m_bFastAnimMode ? "anm_show_fast" : "anm_show", FALSE/*TRUE*/, this, GetState());
			SetPending(TRUE);
		}break;
		case eHiding:
		{
			if (oldState != eHiding)
			{
				m_sounds.PlaySound("sndHide", Fvector().set(0, 0, 0), this, true, false);
				PlayHUDMotion(m_bFastAnimMode ? "anm_hide_fast" : "anm_hide", FALSE/*TRUE*/, this, GetState());
				SetPending(TRUE);
			}
		}break;
		case eIdle:
		{
			PlayAnimIdle();
			SetPending(FALSE);
		}break;
		case eToggle:
		{
			CActor* pActor = smart_cast<CActor*>(H_Parent());
			if (pActor)
			{
				if (!m_switched_on)
					m_sounds.PlaySound("SndTurnOn", pActor->Position(), NULL, !!pActor->HUDview());
				else
					m_sounds.PlaySound("SndTurnOff", pActor->Position(), NULL, !!pActor->HUDview());
			}
			PlayHUDMotion("anm_toggle", TRUE, this, GetState());
			SetPending(TRUE);
		}break;
		case eSwitchOn:
		{
			CActor* pActor = smart_cast<CActor*>(H_Parent());
			if (pActor)
				m_sounds.PlaySound("SndTurnOn", pActor->Position(), NULL, !!pActor->HUDview());
			PlayHUDMotion("anm_toggle", TRUE, this, GetState());
			SetPending(TRUE);
		}break;
		case eSwitchOff:
		{
			CActor* pActor = smart_cast<CActor*>(H_Parent());
			if (pActor)
				m_sounds.PlaySound("SndTurnOff", pActor->Position(), NULL, !!pActor->HUDview());
			PlayHUDMotion("anm_toggle", TRUE, this, GetState());
			SetPending(TRUE);
		}break;
		case eIdleZoom:
		{
			PlayHUDMotion("anm_zoom", TRUE, NULL, GetState());
			SetPending(FALSE);
		}break;
	}
}

void CFlashlight::OnAnimationEnd(u32 state)
{
	inherited::OnAnimationEnd(state);
	switch (state)
	{
		case eShowing:
		{
			SwitchState(eSwitchOn);
		} break;
		case eHiding:
		{
			SwitchState(eHidden);
			g_player_hud->detach_item(this);
		} break;
		case eToggle:
		{
			Switch(!m_switched_on,false);
			SwitchState(eIdle);
		} break;
		case eSwitchOn:
		{
			Switch(true,false);
			SwitchState(eIdle);
		} break;
		case eSwitchOff:
		{
			Switch(false,false);
			SwitchState(eHiding);
		} break;
	}
}

void CFlashlight::UpdateXForm()
{
	CInventoryItem::UpdateXForm();
}

void CFlashlight::OnActiveItem()
{
	return;
}

void CFlashlight::OnHiddenItem()
{
}

BOOL CFlashlight::net_Spawn(CSE_Abstract* DC)
{
	Switch(false, false);

	if (!inherited::net_Spawn(DC))
		return FALSE;

	bool b_r2 = !!psDeviceFlags.test(rsR2);
	b_r2 |= !!psDeviceFlags.test(rsR3);
	b_r2 |= !!psDeviceFlags.test(rsR4);

	lanim = LALib.FindItem(pSettings->r_string(m_light_section, "color_animator"));

	Fcolor clr = pSettings->r_fcolor(m_light_section, (b_r2) ? "color_r2" : "color");
	fBrightness = clr.intensity();
	float range = pSettings->r_float(m_light_section, (b_r2) ? "range_r2" : "range");
	light_render->set_color(clr);
	light_render->set_range(range);

	Fcolor clr_o = pSettings->r_fcolor(m_light_section, (b_r2) ? "omni_color_r2" : "omni_color");
	float range_o = pSettings->r_float(m_light_section, (b_r2) ? "omni_range_r2" : "omni_range");
	light_omni->set_color(clr_o);
	light_omni->set_range(range_o);

	light_render->set_cone(deg2rad(pSettings->r_float(m_light_section, "spot_angle")));
	light_render->set_texture(READ_IF_EXISTS(pSettings, r_string, m_light_section, "spot_texture", (0)));

	glow_render->set_texture(pSettings->r_string(m_light_section, "glow_texture"));
	glow_render->set_color(clr);
	glow_render->set_radius(pSettings->r_float(m_light_section, "glow_radius"));

	light_render->set_volumetric(!!READ_IF_EXISTS(pSettings, r_bool, m_light_section, "volumetric", 0));
	light_render->set_volumetric_quality(READ_IF_EXISTS(pSettings, r_float, m_light_section, "volumetric_quality", 1.f));
	light_render->set_volumetric_intensity(READ_IF_EXISTS(pSettings, r_float, m_light_section, "volumetric_intensity", 1.f));
	light_render->set_volumetric_distance(READ_IF_EXISTS(pSettings, r_float, m_light_section, "volumetric_distance", 1.f));
	light_render->set_type((IRender_Light::LT)(READ_IF_EXISTS(pSettings, r_u8, m_light_section, "type", 2)));
	light_omni->set_type((IRender_Light::LT)(READ_IF_EXISTS(pSettings, r_u8, m_light_section, "omni_type", 1)));

	return TRUE;
}

void CFlashlight::Load(LPCSTR section)
{
	inherited::Load(section);
	m_sounds.LoadSound(section, "snd_draw", "sndShow");
	m_sounds.LoadSound(section, "snd_holster", "sndHide");
	m_sounds.LoadSound(section, "snd_turn_on", "sndTurnOn", false, SOUND_TYPE_ITEM_USING);
	m_sounds.LoadSound(section, "snd_turn_off", "sndTurnOff", false, SOUND_TYPE_ITEM_USING);

	light_trace_bone = READ_IF_EXISTS(pSettings, r_string, section, "light_trace_bone", "");

	m_light_section = READ_IF_EXISTS(pSettings, r_string, section, "light_section", "torch_definition");
}


void CFlashlight::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);
}


bool CFlashlight::IsWorking()
{
	return m_switched_on && H_Parent() && H_Parent() == Level().CurrentViewEntity();
}

void CFlashlight::UpdateVisibility()
{
	//check visibility
	attachable_hud_item* i0 = g_player_hud->attached_item(1);
	if (i0 && HudItemData())
	{
		if (light_trace_bone.size())
		{
			u16 bone_id = HudItemData()->m_model->LL_BoneID(light_trace_bone);
			if (bone_id != BI_NONE)
			{
				bool visi = HudItemData()->m_model->LL_GetBoneVisible(bone_id);
				if (visi != m_switched_on)
					HudItemData()->m_model->LL_SetBoneVisible(bone_id, m_switched_on, TRUE);
			}
		}

		bool bClimb = ((Actor()->MovingState()&mcClimb) != 0);
		if (bClimb)
		{
			HideDevice(true);
			m_bNeedActivation = true;
		}
		else
		{
			CWeapon* wpn = smart_cast<CWeapon*>(i0->m_parent_hud_item);
			if (wpn)
			{
				u32 state = wpn->GetState();
				if (state == CWeapon::eReload || state == CWeapon::eSwitch)
				{
					HideDevice(true);
					m_bNeedActivation = true;
				}
				else if (wpn->IsZoomed())
				{
					if (GetState() != eIdleZoom)
						SwitchState(eIdleZoom);
				}
				else if (GetState() == eIdleZoom && GetState() != eIdle)
				{
					SwitchState(eIdle);
				}
			}
		}
	}
	else
		if (m_bNeedActivation)
		{
			attachable_hud_item* i0 = g_player_hud->attached_item(1);
			bool bClimb = ((Actor()->MovingState()&mcClimb) != 0);
			if (!bClimb)
			{
				CHudItem* huditem = (i0) ? i0->m_parent_hud_item : NULL;
				bool bChecked = !huditem || CheckCompatibilityInt(huditem, 0);

				if (bChecked)
					ShowDevice(true);
			}
		}
}

void CFlashlight::UpdateCL()
{
	inherited::UpdateCL();

	if (H_Parent() != Level().CurrentEntity())			
		return;

	CActor* actor = smart_cast<CActor*>(H_Parent());
	if (!actor)
		return;

	UpdateVisibility();

	if (!IsWorking())
		return;

	if (!HudItemData())
	{
		Switch(false, false);
		return;
	}

	firedeps dep;
	HudItemData()->setup_firedeps(dep);

	light_render->set_position(dep.vLastFP);
	light_omni->set_position(dep.vLastFP);
	glow_render->set_position(dep.vLastFP);

	light_render->set_rotation(dep.m_FireParticlesXForm.k, dep.m_FireParticlesXForm.i);
	light_omni->set_rotation(dep.m_FireParticlesXForm.k, dep.m_FireParticlesXForm.i);
	glow_render->set_direction(dep.m_FireParticlesXForm.k);

	// calc color animator
	if (!lanim)							
		return;

	int frame;

	u32 clr = lanim->CalculateBGR(Device.fTimeGlobal, frame);

	Fcolor fclr;
	fclr.set((float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f);
	fclr.mul_rgb(fBrightness / 255.f);
	if (can_use_dynamic_lights())
	{
		light_render->set_color(fclr);
		light_omni->set_color(fclr);
	}
	glow_render->set_color(fclr);
}

void CFlashlight::OnH_A_Chield()
{
	inherited::OnH_A_Chield();
}

void CFlashlight::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);

	if (GetState() != eHidden)
	{
		// Detaching hud item and animation stop in OnH_A_Independent
		Switch(false, false);
		SwitchState(eHidden);
	}
}

void CFlashlight::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CFlashlight::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CFlashlight::setup_physic_shell()
{
	CPhysicsShellHolder::setup_physic_shell();
}

void CFlashlight::OnMoveToRuck(const SInvItemPlace& prev)
{
	inherited::OnMoveToRuck(prev);
	if (prev.type == eItemPlaceSlot)
	{
		SwitchState(eHidden);
		g_player_hud->detach_item(this);
	}
	Switch(false);
	StopCurrentAnimWithoutCallback();
}

void CFlashlight::OnMoveToSlot(const SInvItemPlace& prev)
{
	inherited::OnMoveToSlot(prev);
}

inline bool CFlashlight::can_use_dynamic_lights()
{
	if (!H_Parent())
		return				(true);

	CInventoryOwner			*owner = smart_cast<CInventoryOwner*>(H_Parent());
	if (!owner)
		return				(true);

	return					(owner->can_use_dynamic_lights());
}

void CFlashlight::Switch()
{
	Switch(!m_switched_on);
}

void CFlashlight::Switch(bool light_on, bool b_play_sound)
{
	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (pActor)
	{
		if (light_on && !m_switched_on)
		{
			if (b_play_sound && m_sounds.FindSoundItem("SndTurnOn", false))
				m_sounds.PlaySound("SndTurnOn", pActor->Position(), NULL, !!pActor->HUDview());
		}
		else if (!light_on && m_switched_on)
		{
			if (b_play_sound && m_sounds.FindSoundItem("SndTurnOff", false))
				m_sounds.PlaySound("SndTurnOff", pActor->Position(), NULL, !!pActor->HUDview());
		}
	}

	m_switched_on = light_on;
	if (can_use_dynamic_lights())
	{
		light_render->set_active(light_on);
		light_omni->set_active(light_on);
	}
	glow_render->set_active(light_on);
}
bool CFlashlight::torch_active() const
{
	return m_switched_on;
}

void CFlashlight::ToggleSwitch()
{
	if (!IsPending())
	{
		u32 state = GetState();
		if (state == eIdle || state == eIdleZoom)
		{
			SwitchState(eToggle);
		}
	}
}