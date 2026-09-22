#include "StdAfx.h"
#include "CustomDetector.h"
#include "ui/ArtefactDetectorUI.h"
#include "HUDManager.h"
#include "Inventory.h"
#include "Level.h"
#include "map_manager.h"
#include "ActorEffector.h"
#include "Actor.h"
#include "xrUICore/Windows/UIWindow.h"
#include "player_hud.h"
#include "Weapon.h"

ITEM_INFO::~ITEM_INFO()
{
    if (pParticle)
        CParticlesObject::Destroy(pParticle);
}

bool CCustomDetector::CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate)
{
    if (itm == nullptr)
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
            bres = bres && (W->GetState() != CHUDState::eBore) && (W->GetState() != CWeapon::eReload) &&
                (W->GetState() != CWeapon::eSwitch) && !W->IsZoomed();
    }
    return bres;
}

bool CCustomDetector::CheckCompatibility(CHudItem* itm)
{
    if (!inherited::CheckCompatibility(itm))
        return false;

    if (!CheckCompatibilityInt(itm, NULL))
    {
        HideDetector(true);
        return false;
    }
    return true;
}

void CCustomDetector::HideDetector(bool bFastMode)
{
    if (GetState() == eIdle)
        ToggleDetector(bFastMode);
}

void CCustomDetector::ShowDetector(bool bFastMode)
{
    if (GetState() == eHidden)
        ToggleDetector(bFastMode);
}

void CCustomDetector::ToggleDetector(bool bFastMode)
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
                TurnDetectorInternal(true);
            }
        }
    }
    else if (GetState() == eIdle)
        SwitchState(eHiding);
}

void CCustomDetector::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);

    switch (S)
    {
    case eShowing:
    {
        g_player_hud->attach_item(this);
        m_sounds.PlaySound("sndShow", Fvector().set(0, 0, 0), this, true, false);
        PlayHUDMotion(m_bFastAnimMode ? "anm_show_fast" : "anm_show", "anim_show", FALSE /*TRUE*/, this, GetState());
        SetPending(TRUE);
    }
    break;
    case eHiding:
    {
        if (oldState != eHiding)
        {
            m_sounds.PlaySound("sndHide", Fvector().set(0, 0, 0), this, true, false);
            PlayHUDMotion(m_bFastAnimMode ? "anm_hide_fast" : "anm_hide", "anim_show", FALSE/*TRUE*/, this, GetState());
            SetPending(TRUE);
        }
    }
    break;
    case eIdle:
    {
        PlayAnimIdle();
        SetPending(FALSE);
    }
    break;
    }
}

void CCustomDetector::OnAnimationEnd(u32 state)
{
    inherited::OnAnimationEnd(state);
    switch (state)
    {
    case eShowing:
    {
        SwitchState(eIdle);
        if (IsUsingCondition() && m_fDecayRate > 0.f)
            this->SetCondition(-m_fDecayRate);
    }
    break;
    case eHiding:
    {
        SwitchState(eHidden);
        TurnDetectorInternal(false);
        g_player_hud->detach_item(this);
    }
    break;
    }
}

void CCustomDetector::UpdateXForm() { CInventoryItem::UpdateXForm(); }
void CCustomDetector::OnActiveItem() { return; }
void CCustomDetector::OnHiddenItem() {}

CCustomDetector::~CCustomDetector()
{
    TurnDetectorInternal(false);
    xr_delete(m_ui);
}

bool CCustomDetector::net_Spawn(CSE_Abstract* DC)
{
    TurnDetectorInternal(false);
    return (inherited::net_Spawn(DC));
}

void CCustomDetector::Load(LPCSTR section)
{
    m_animation_slot = 7;
    inherited::Load(section);

    m_fAfDetectRadius = pSettings->read_if_exists(section, "af_radius", 30.0f);
    m_fRadius = pSettings->read_if_exists(section, "radius", m_fAfDetectRadius);
    m_fAfVisRadius = pSettings->read_if_exists(section, "af_vis_radius", 3.0f);

    m_fDecayRate = pSettings->read_if_exists(section, "decay_rate", 0.0f); //Alundaio

    shared_str nightvision_particle;
    if (pSettings->line_exist(section, "night_vision_particle"))
        nightvision_particle = pSettings->r_string(section, "night_vision_particle");

    m_zones.load(section, "zone", [this, nightvision_particle](const shared_str& /*item_sect*/)
    {
        return ITEM_TYPE
        {
            .nightvision_particle = nightvision_particle,
            .radius = m_fRadius,
        };
    });
    m_artefacts.load(section, "af", [this, nightvision_particle](const shared_str& /*item_sect*/)
    {
        return ITEM_TYPE
        {
            .nightvision_particle = nightvision_particle,
            .radius = m_fAfDetectRadius,
        };
    });

    m_sounds.LoadSound(section, "snd_draw", "sndShow");
    m_sounds.LoadSound(section, "snd_holster", "sndHide");
}

void CCustomDetector::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    UpdateNightVisionMode(IsWorking());

    if (!IsWorking())
        return;

    Position().set(H_Parent()->Position());

    Fvector P = H_Parent()->Position();

    if (IsUsingCondition() && GetCondition() <= 0.01f)
        return;

    m_zones.feel_touch_update(P, m_fRadius);
    m_artefacts.feel_touch_update(P, m_fAfDetectRadius);
}

bool CCustomDetector::IsWorking()
{
    return m_bWorking && H_Parent() && H_Parent() == Level().CurrentViewEntity();
}

void CCustomDetector::UpdateVisibility()
{
    // check visibility
    attachable_hud_item* i0 = g_player_hud->attached_item(0);
    if (i0 && HudItemData())
    {
        bool bClimb = ((Actor()->MovingState() & mcClimb) != 0);
        if (bClimb)
        {
            HideDetector(true);
            m_bNeedActivation = true;
        }
        else
        {
            CWeapon* wpn = smart_cast<CWeapon*>(i0->m_parent_hud_item);
            if (wpn)
            {
                u32 state = wpn->GetState();
                if (wpn->IsZoomed() || state == CWeapon::eReload || state == CWeapon::eSwitch)
                {
                    HideDetector(true);
                    m_bNeedActivation = true;
                }
            }
        }
    }
    else if (m_bNeedActivation)
    {
        attachable_hud_item* i0 = g_player_hud->attached_item(0);
        bool bClimb = ((Actor()->MovingState() & mcClimb) != 0);
        if (!bClimb)
        {
            CHudItem* huditem = (i0) ? i0->m_parent_hud_item : NULL;
            bool bChecked = !huditem || CheckCompatibilityInt(huditem, 0);

            if (bChecked)
                ShowDetector(true);
        }
    }
}

void CCustomDetector::UpdateCL()
{
    inherited::UpdateCL();

    if (H_Parent() != Level().CurrentEntity())
        return;

    UpdateVisibility();
    if (!IsWorking())
        return;

    Scan();

    if (m_ui)
        m_ui->update();
}

void CCustomDetector::Scan()
{
    const Fvector& pos = Position();
    m_zones.scan(pos, this);
    m_artefacts.scan(pos, this, m_fAfVisRadius);
}

void CCustomDetector::OnH_A_Chield() { inherited::OnH_A_Chield(); }
void CCustomDetector::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    m_zones.clear();
    m_artefacts.clear();

	if (GetState() != eHidden)
	{
		// Detaching hud item and animation stop in OnH_A_Independent
		TurnDetectorInternal(false);
		SwitchState(eHidden);
	}
}

void CCustomDetector::OnMoveToRuck(const SInvItemPlace& prev)
{
    inherited::OnMoveToRuck(prev);
    if (prev.type == eItemPlaceSlot)
    {
        SwitchState(eHidden);
        g_player_hud->detach_item(this);
    }
    TurnDetectorInternal(false);
    StopCurrentAnimWithoutCallback();
}

void CCustomDetector::OnMoveToSlot(const SInvItemPlace& prev)
{
    inherited::OnMoveToSlot(prev);
    if (HudSection().empty())
        TurnDetectorInternal(true);
}

void CCustomDetector::OnMoveToBelt(const SInvItemPlace& prev)
{
    CHudItemObject::OnMoveToBelt(prev);
    if (HudSection().empty())
        TurnDetectorInternal(true);
}

void CCustomDetector::TurnDetectorInternal(bool on)
{
    m_bWorking = on;
    if (on && !m_ui)
    {
        CreateUI();
    }
    else
    {
        //.		xr_delete			(m_ui);
    }

    if (on)
        UpdateNightVisionMode(on);
}

void CCustomDetector::UpdateNightVisionMode(bool on)
{
    bool bNightVision = false;

    if (CActor* actor = smart_cast<CActor*>(H_Parent()))
        bNightVision  = actor->Cameras().GetPPEffector(EEffectorPPType(effNightvision));

    m_zones.update_night_vision(on && bNightVision);
}

bool CZoneList::feel_touch_contact(IGameObject* O)
{
    const auto it = m_TypesMap.find(O->cNameSect());
    if (it == m_TypesMap.end())
        return false;

    CCustomZone* zone = smart_cast<CCustomZone*>(O);
    return zone && zone->IsEnabled() && (zone->VisibleByDetector() || anomalies_always_visible);
}

void CZoneList::scan(const Fvector detector_position, const IGameObject* parent)
{
    for (float& power : zone_cur_power)
    {
        if (Device.fTimeDelta < 1.0f)
            power *= 0.9f * (1.0f - Device.fTimeDelta);

        if (power < 0.01f)
            power = 0.0f;
    }

    for (auto& [zone, zone_info] : m_ItemInfos)
    {
        ITEM_TYPE* zone_type = zone_info.curr_ref;

        float dist_to_zone = 0.0f;
        float rad_zone = 0.0f;
        zone->CalcDistanceTo(detector_position, dist_to_zone, rad_zone);
        clamp(dist_to_zone, 0.0f, flt_max * 0.5f);

        float fRelPow = dist_to_zone / (rad_zone + zone_type->radius + 0.1f) - 0.1f;

        float power = zone->Power(dist_to_zone, rad_zone);
        clamp(power, 0.0f, 1.1f);

        if (dist_to_zone < rad_zone + 0.9f * zone_type->radius)
        {
            fRelPow *= 0.6f;
            if (dist_to_zone < rad_zone)
            {
                fRelPow *= 0.3f;
                fRelPow *= (2.5f - 2.0f * power); // звук зависит от силы зоны
            }
        }
        clamp(fRelPow, 0.0f, 1.0f);

        update_sound(zone_info, parent, fRelPow, false);

        const auto hit_type = zone->GetHitType();
        const auto iz_type = g_tfHitType2InfluenceType(hit_type);
        if (iz_type == ALife::infl_max_count)
            continue;
        if (zone_cur_power[iz_type] < power)
            zone_cur_power[iz_type] = power;
    }
}

bool CAfList::feel_touch_contact(IGameObject* O)
{
    const auto it = m_TypesMap.find(O->cNameSect());
    if (it == m_TypesMap.end())
        return false;

    const auto artefact = smart_cast<CArtefact*>(O);
    return artefact && artefact->GetAfRank() <= m_af_rank;
}

std::pair<CArtefact*, float> CAfList::scan(const Fvector detector_position, const IGameObject* parent, const float visibility_radius)
{
    if (m_ItemInfos.empty())
        return {};

    auto it_b = m_ItemInfos.begin();
    auto it_e = m_ItemInfos.end();
    auto it = it_b;
    float min_dist = flt_max;

    for (; it_b != it_e; ++it_b) // only nearest
    {
        CArtefact* pAf = it_b->first;
        if (pAf->H_Parent())
            continue;

        float d = detector_position.distance_to(pAf->Position());
        if (d < min_dist)
        {
            min_dist = d;
            it = it_b;
        }

        if (pAf->CanBeInvisible() && d < visibility_radius)
            pAf->SwitchVisibility(true);
    }

    ITEM_INFO& af_info = it->second;
    ITEM_TYPE* item_type = af_info.curr_ref;

    float dist = min_dist;
    float fRelPow = dist / item_type->radius;
    clamp(fRelPow, 0.f, 1.f);

    update_sound(af_info, parent, fRelPow, true);

    return { it->first, fRelPow };
}
