////////////////////////////////////////////////////////////////////////////
//	Module 		: UIActorStateInfo.cpp
//	Created 	: 15.02.2008
//	Author		: Evgeniy Sokolov
//	Description : UI actor state window class implementation
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UIActorStateInfo.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Static/UIStatic.h"
#include "UIXmlInit.h"
#include "Common/object_broker.h"
#include "UIHelper.h"
#include "xrUICore/arrow/ui_arrow.h"
#include "UIHudStatesWnd.h"
#include "Level.h"
#include "location_manager.h"
#include "player_hud.h"
#include "UIMainIngameWnd.h"
#include "UIGameCustom.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "EntityCondition.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "Inventory.h"
#include "Artefact.h"
#include "string_table.h"

ui_actor_state_wnd::~ui_actor_state_wnd() { delete_data(m_hint_wnd); }

void ui_actor_state_wnd::init_from_xml(CUIXml& xml)
{
    for (int i = 0; i < stt_count; ++i)
    {
        m_state[i] = new ui_actor_state_item();
        m_state[i]->SetAutoDelete(true);
        AttachChild(m_state[i]);
        m_state[i]->set_hint_wnd(m_hint_wnd);
    }
    m_state[stt_health]->init_from_xml_plain(xml, "progress_bar_health");
    m_state[stt_psi]->init_from_xml_plain(xml, "progress_bar_psy");
    m_state[stt_radia]->init_from_xml_plain(xml, "progress_bar_radiation");
}

void ui_actor_state_wnd::init_from_xml(CUIXml& xml, LPCSTR path)
{
    XML_NODE stored_root = xml.GetLocalRoot();
    CUIXmlInit::InitWindow(xml, path, 0, this);

    XML_NODE new_root = xml.NavigateToNode(path, 0);
    xml.SetLocalRoot(new_root);

    m_hint_wnd = UIHelper::CreateHint(xml, "hint_wnd");

    for (int i = 0; i < stt_count; ++i)
    {
        m_state[i] = new ui_actor_state_item();
        m_state[i]->SetAutoDelete(true);
        AttachChild(m_state[i]);
        m_state[i]->set_hint_wnd(m_hint_wnd);
    }
    m_state[stt_stamina]->init_from_xml(xml, "stamina_state", false);
    m_state[stt_health]->init_from_xml(xml, "health_state");
    m_state[stt_bleeding]->init_from_xml(xml, "bleeding_state", false);
    m_state[stt_radiation]->init_from_xml(xml, "radiation_state", false);
    m_state[stt_armor]->init_from_xml(xml, "armor_state", false);

    m_state[stt_main]->init_from_xml(xml, "main_sensor", false);
    m_state[stt_fire]->init_from_xml(xml, "fire_sensor");
    m_state[stt_radia]->init_from_xml(xml, "radia_sensor");
    m_state[stt_acid]->init_from_xml(xml, "acid_sensor");
    m_state[stt_psi]->init_from_xml(xml, "psi_sensor");
    m_state[stt_wound]->init_from_xml(xml, "wound_sensor", false);
    m_state[stt_fire_wound]->init_from_xml(xml, "fire_wound_sensor", false);
    m_state[stt_shock]->init_from_xml(xml, "shock_sensor", false);
    m_state[stt_power]->init_from_xml(xml, "power_sensor", false);

    xml.SetLocalRoot(stored_root);
}

void ui_actor_state_wnd::UpdateActorInfo(CInventoryOwner* owner)
{
    auto actor = smart_cast<CActor*>(owner);
    if (!actor)
        return;

    float value = 0.0f;

    const auto& conditions = actor->conditions();

    // show stamina icon
    value = conditions.GetPower();
    m_state[stt_stamina]->set_progress(value);

    value = actor->GetRestoreSpeed(ALife::ePowerRestoreSpeed);
    m_state[stt_stamina]->set_text(value); // 0..0.99

    // show health icon
    value = conditions.GetHealth();
    value = floor(value * 55) / 55; // number of sticks in progress bar
    m_state[stt_health]->set_progress(value);

    // show bleeding icon
    value = conditions.BleedingSpeed();
    m_state[stt_health]->show_static((value > 0.01f)); // Bleeding icon in Clear Sky
    
    m_state[stt_bleeding]->show_static(false, 1);
    m_state[stt_bleeding]->show_static(false, 2);
    m_state[stt_bleeding]->show_static(false, 3);

    if (!fis_zero(value, EPS))
    {
        if (value < 0.35f)
            m_state[stt_bleeding]->show_static(true, 1);
        else if (value < 0.7f)
            m_state[stt_bleeding]->show_static(true, 2);
        else
            m_state[stt_bleeding]->show_static(true, 3);
    }

    // show radiation icon
    value = conditions.GetRadiation();
    m_state[stt_radiation]->show_static(false, 1);
    m_state[stt_radiation]->show_static(false, 2);
    m_state[stt_radiation]->show_static(false, 3);

    if (!fis_zero(value, EPS))
    {
        if (value < 0.35f)
            m_state[stt_radiation]->show_static(true, 1);
        else if (value < 0.7f)
            m_state[stt_radiation]->show_static(true, 2);
        else
            m_state[stt_radiation]->show_static(true, 3);
    }

    CCustomOutfit* outfit = actor->GetOutfit();
    PIItem itm = actor->inventory().ItemFromSlot(HELMET_SLOT);
    CHelmet* helmet = smart_cast<CHelmet*>(itm);

    m_state[stt_fire]->set_progress(0.0f);
    m_state[stt_radia]->set_progress(0.0f);
    m_state[stt_acid]->set_progress(0.0f);
    m_state[stt_psi]->set_progress(0.0f);
    m_state[stt_wound]->set_progress(0.0f);
    m_state[stt_fire_wound]->set_progress(0.0f);
    m_state[stt_shock]->set_progress(0.0f);
    m_state[stt_power]->set_progress(0.0f);

    float burn_value = 0.0f;
    float radi_value = 0.0f;
    float cmbn_value = 0.0f;
    float tele_value = 0.0f;
    float woun_value = 0.0f;
    float shoc_value = 0.0f;
    float fwou_value = 0.0f;

    const auto& cur_booster_influences = conditions.GetCurBoosterInfluences();
    CEntityCondition::BOOSTER_MAP::const_iterator it;
    it = cur_booster_influences.find(eBoostRadiationProtection);
    if (it != cur_booster_influences.end())
        radi_value += it->second.fBoostValue;

    it = cur_booster_influences.find(eBoostChemicalBurnProtection);
    if (it != cur_booster_influences.end())
        cmbn_value += it->second.fBoostValue;

    it = cur_booster_influences.find(eBoostTelepaticProtection);
    if (it != cur_booster_influences.end())
        tele_value += it->second.fBoostValue;

    if (outfit)
    {
        value = outfit->GetCondition();
        m_state[stt_armor]->set_progress(value);

        burn_value += outfit->GetDefHitTypeProtection(ALife::eHitTypeBurn);
        radi_value += outfit->GetDefHitTypeProtection(ALife::eHitTypeRadiation);
        cmbn_value += outfit->GetDefHitTypeProtection(ALife::eHitTypeChemicalBurn);
        tele_value += outfit->GetDefHitTypeProtection(ALife::eHitTypeTelepatic);
        woun_value += outfit->GetDefHitTypeProtection(ALife::eHitTypeWound);
        shoc_value += outfit->GetDefHitTypeProtection(ALife::eHitTypeShock);

        IKinematics* ikv = smart_cast<IKinematics*>(actor->Visual());
        VERIFY(ikv);
        const auto spine_bone = ikv->LL_BoneID("bip01_spine");

        value = outfit->GetBoneArmor(spine_bone);
        m_state[stt_armor]->set_text(value);

        fwou_value += value * outfit->GetCondition();
        if (!outfit->bIsHelmetAvaliable)
        {
            const auto head_bone = ikv->LL_BoneID("bip01_head");
            fwou_value += outfit->GetBoneArmor(head_bone) * outfit->GetCondition();
        }
    }
    else
    {
        m_state[stt_armor]->set_progress(0.0f);
        m_state[stt_armor]->set_text(0.0f);
    }

    if (helmet)
    {
        burn_value += helmet->GetDefHitTypeProtection(ALife::eHitTypeBurn);
        radi_value += helmet->GetDefHitTypeProtection(ALife::eHitTypeRadiation);
        cmbn_value += helmet->GetDefHitTypeProtection(ALife::eHitTypeChemicalBurn);
        tele_value += helmet->GetDefHitTypeProtection(ALife::eHitTypeTelepatic);
        woun_value += helmet->GetDefHitTypeProtection(ALife::eHitTypeWound);
        shoc_value += helmet->GetDefHitTypeProtection(ALife::eHitTypeShock);

        IKinematics* ikv = smart_cast<IKinematics*>(actor->Visual());
        VERIFY(ikv);
        const auto head_bone = ikv->LL_BoneID("bip01_head");
        fwou_value += helmet->GetBoneArmor(head_bone) * helmet->GetCondition();
    }

    const auto getProtection = [&](float& valueRef, ALife::EHitType hitType) -> float
    {
        valueRef += actor->GetProtection_ArtefactsOnBelt(hitType);
        return conditions.GetZoneMaxPower(hitType);
    };

    // fire burn protection progress bar
    {
        const float max_power = getProtection(burn_value, ALife::eHitTypeBurn);
        update_round_states(stt_fire, burn_value, max_power);
    }
    // radiation protection progress bar
    {
        const float max_power = getProtection(radi_value, ALife::eHitTypeRadiation);
        update_round_states(stt_radia, radi_value, max_power);
    }
    // chemical burn protection progress bar
    {
        const float max_power = getProtection(cmbn_value, ALife::eHitTypeChemicalBurn);
        update_round_states(stt_acid, cmbn_value, max_power);
    }
    // telepathic protection progress bar
    {
        const float max_power = getProtection(tele_value, ALife::eHitTypeTelepatic);
        update_round_states(stt_psi, tele_value, max_power);
    }
    // wound protection progress bar
    {
        const float max_power = conditions.GetMaxWoundProtection();
        update_round_states(stt_wound, woun_value, max_power);
    }
    // shock protection progress bar
    {
        const float max_power = getProtection(shoc_value, ALife::eHitTypeShock);
        update_round_states(stt_shock, shoc_value, max_power);
    }
    // fire wound protection progress bar
    {
        const float max_power = conditions.GetMaxFireWoundProtection();
        update_round_states(stt_fire_wound, fwou_value, max_power);
    }
    // power restore speed progress bar
    {
        value = actor->GetRestoreSpeed(ALife::ePowerRestoreSpeed) / conditions.GetMaxPowerRestoreSpeed();
        update_round_states(stt_power, value, 1.f);
    }

    // -----------------------------------------------------------------------------------
    m_state[stt_main]->set_progress_shape(conditions.GetRadiation());
    // -----------------------------------------------------------------------------------

    UpdateHitZone();
}

void ui_actor_state_wnd::update_round_states(EStateType stt_type, float initial, float max_power)
{
    auto state = m_state[stt_type];

    const float progress = floor(initial / max_power * 31) / 31; // number of sticks in progress bar
    const float arrow = initial / max_power; //  = 0..1

    if (!state->set_progress(progress))
    {
        //state->set_progress_shape(arrow);
        state->set_arrow(arrow); // 0..1
        state->set_text(arrow); // 0..1
    }
}

void ui_actor_state_wnd::UpdateHitZone()
{
    CUIHudStatesWnd* wnd = CurrentGameUI()->UIMainIngameWnd->get_hud_states(); //некрасиво слишком
    VERIFY(wnd);
    if (!wnd)
    {
        return;
    }
    wnd->UpdateZones();
    if (m_state[stt_main])
        m_state[stt_main]->set_arrow(wnd->get_main_sensor_value());

    /*
    m_state[stt_fire]->set_arrow(wnd->get_zone_cur_power(ALife::eHitTypeBurn));
    m_state[stt_radia]->set_arrow(nd->get_zone_cur_power(ALife::eHitTypeRadiation));
    m_state[stt_acid]->set_arrow(wnd->get_zone_cur_power(ALife::eHitTypeChemicalBurn));
    m_state[stt_psi]->set_arrow(wnd->get_zone_cur_power(ALife::eHitTypeTelepatic));
    */
}

void ui_actor_state_wnd::Draw()
{
    inherited::Draw();
    m_hint_wnd->Draw();
}

void ui_actor_state_wnd::Show(bool status)
{
    inherited::Show(status);
    ShowChildren(status);
}

/// =============================================================================================
ui_actor_state_item::ui_actor_state_item() : m_magnitude(1.0f) {}

void ui_actor_state_item::init_from_xml(CUIXml& xml, LPCSTR path, bool critical /*= true*/)
{
    if (!CUIXmlInit::InitWindow(xml, path, 0, this, critical))
        return;

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE new_root = xml.NavigateToNode(path, 0);
    xml.SetLocalRoot(new_root);

    LPCSTR hint_text = xml.Read("hint_text", 0, "no hint");
    set_hint_text_ST(hint_text);

    set_hint_delay((u32)xml.ReadAttribInt("hint_text", 0, "delay"));

    if (xml.NavigateToNode("state_progress", 0))
    {
        m_progress = UIHelper::CreateProgressBar(xml, "state_progress", this);
    }
    if (xml.NavigateToNode("progress_shape", 0))
    {
        m_sensor = new CUIProgressShape();
        AttachChild(m_sensor);
        m_sensor->SetAutoDelete(true);
        CUIXmlInit::InitProgressShape(xml, "progress_shape", 0, m_sensor);
    }
    if (xml.NavigateToNode("arrow", 0))
    {
        m_arrow = new UI_Arrow();
        m_arrow->init_from_xml(xml, "arrow", this);
    }
    if (xml.NavigateToNode("arrow_shadow", 0))
    {
        m_arrow_shadow = new UI_Arrow();
        m_arrow_shadow->init_from_xml(xml, "arrow_shadow", this);
    }
    if (xml.NavigateToNode("icon", 0))
    {
        m_static = UIHelper::CreateStatic(xml, "icon", this);
        m_magnitude = xml.ReadAttribFlt("icon", 0, "magnitude", 1.0f);
        m_static->TextItemControl()->SetText("");
    }
    if (xml.NavigateToNode("icon2", 0))
    {
        m_static2 = UIHelper::CreateStatic(xml, "icon2", this);
        m_magnitude = xml.ReadAttribFlt("icon2", 0, "magnitude", 1.0f);
        m_static2->TextItemControl()->SetText("");
    }
    if (xml.NavigateToNode("icon3", 0))
    {
        m_static3 = UIHelper::CreateStatic(xml, "icon3", this);
        m_magnitude = xml.ReadAttribFlt("icon3", 0, "magnitude", 1.0f);
        m_static3->TextItemControl()->SetText("");
    }
    set_arrow(0.0f);
    xml.SetLocalRoot(stored_root);
}

void ui_actor_state_item::init_from_xml_plain(CUIXml& xml, LPCSTR path)
{
    m_progress = UIHelper::CreateProgressBar(xml, path, this);
}

bool ui_actor_state_item::set_text(float value)
{
    if (!m_static)
        return false;

    int v = (int)(value * m_magnitude + 0.49f); // m_magnitude=100
    clamp(v, 0, 99);
    string32 text_res;
    xr_sprintf(text_res, sizeof(text_res), "%d", v);
    m_static->TextItemControl()->SetText(text_res);
    return true;
}

bool ui_actor_state_item::set_progress(float value)
{
    if (!m_progress)
        return false;

    m_progress->SetProgressPos(value);
    return true;
}

bool ui_actor_state_item::set_progress_shape(float value)
{
    if (!m_sensor)
        return false;

    m_sensor->SetPos(value);
    return true;
}

int ui_actor_state_item::set_arrow(float value)
{
    if (!m_arrow)
        return 0;

    m_arrow->SetNewValue(value);

    if (!m_arrow_shadow)
        return 1;

    m_arrow_shadow->SetPos(m_arrow->GetPos());
    return 2;
}

bool ui_actor_state_item::show_static(bool status, u8 number)
{
    switch (number)
    {
    case 1:
        if (!m_static)
            return false;
        m_static->Show(status);
        break;

    case 2:
        if (!m_static2)
            return false;
        m_static2->Show(status);
        break;

    case 3:
        if (!m_static3)
            return false;
        m_static3->Show(status);
        break;

    default: return false;
    }
    return true;
}
