#include "StdAfx.h"
#include "UIHudStatesWnd.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "EntityCondition.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "Inventory.h"
#include "RadioactiveZone.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "xrUICore/arrow/ui_arrow.h"
#include "UIInventoryUtilities.h"
#include "CustomDetector.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "PDA.h"
#include "WeaponMagazinedWGrenade.h"
#include "xrUICore/XML/UITextureMaster.h"

CUIHudStatesWnd::CUIHudStatesWnd()
    : CUIWindow(CUIHudStatesWnd::GetDebugType()), m_b_force_update(true)
{
    m_health_blink = pSettings->read_if_exists<float>("actor_condition", "hud_health_blink", 0.f);
    clamp(m_health_blink, 0.0f, 1.0f);

    hud_zones_list.anomalies_always_visible = true;
}

void CUIHudStatesWnd::reset_ui()
{
    hud_zones_list.clear();
}

void CUIHudStatesWnd::InitFromXml(CUIXml& xml, LPCSTR path)
{
    ZoneScoped;

    if (!CUIXmlInit::InitWindow(xml, path, 0, this, false))
        SetWndSize(GetParent()->GetWndSize());

    XML_NODE stored_root = xml.GetLocalRoot();

    XML_NODE new_root = xml.NavigateToNode(path, 0);
    xml.SetLocalRoot(new_root);

    std::ignore = UIHelper::CreateStatic(xml, "back", this, false);
    std::ignore = UIHelper::CreateStatic(xml, "back_v", this, false);

    // XXX: replace with UIHelper
    if (xml.NavigateToNode("arrow"))
    {
        m_arrow = xr_new<UI_Arrow>();
        m_arrow->init_from_xml(xml, "arrow", this);
    }

    // XXX: replace with UIHelper
    if (xml.NavigateToNode("arrow_shadow"))
    {
        m_arrow_shadow = xr_new<UI_Arrow>();
        m_arrow_shadow->init_from_xml(xml, "arrow_shadow", this);
    }

    std::ignore = UIHelper::CreateStatic(xml, "back_over_arrow", this, false);
    m_static_health = UIHelper::CreateStatic(xml, "static_health", this, false);
    m_static_armor = UIHelper::CreateStatic(xml, "static_armor", this, false);
    m_static_weapon = UIHelper::CreateStatic(xml, "static_weapon", this, false);

    CUIWindow* healthBarParent = this;
    CUIWindow* armorBarParent = this;
    CUIWindow* weaponsParent = this;
    if (m_static_health && m_static_armor)
    {
        healthBarParent = m_static_health;
        armorBarParent = m_static_armor;
    }
    if (m_static_weapon)
    {
        weaponsParent = m_static_weapon;
    }

    m_ui_health_bar = UIHelper::CreateProgressBar(xml, "progress_bar_health", healthBarParent);
    m_ui_stamina_bar = UIHelper::CreateProgressBar(xml, "progress_bar_stamina", this, false);
    m_ui_armor_bar = UIHelper::CreateProgressBar(xml, "progress_bar_armor", armorBarParent, false);

    if (m_static_armor || m_ui_armor_bar)
    {
        R_ASSERT3(m_static_armor && m_ui_armor_bar,
            "Please, provide both [m_static_armor] and [m_ui_armor_bar] tags in xml file",
            xml.m_xml_file_name);
    }

    constexpr std::tuple<ALife::EInfluenceType, cpcstr, cpcstr> resistance_indicators[] =
    {
        { ALife::infl_rad,  "resist_back_rad",  "indik_rad"     },
        { ALife::infl_fire, "resist_back_fire", "indik_fire"    },
        { ALife::infl_acid, "resist_back_acid", "indik_acid"    },
        { ALife::infl_psi,  "resist_back_psi",  "indik_psi"     },
        // electra = no has CStatic!!
    };

    for (const auto& [type, back, indik] : resistance_indicators)
    {
        std::ignore = UIHelper::CreateStatic(xml, back, this, false);
        const auto indicator = UIHelper::CreateStatic(xml, indik, this, false);
        if (!indicator)
            continue;
        m_indik[type] = indicator;
        // Will crash if LALib contents are updated/reloaded during the game
        // because pointers might become invalid
        m_color_animation[type] = indicator->GetColorAnimation();
        indicator->SetColorAnimation(nullptr, 0);
    }

    m_ui_weapon_sign_ammo = UIHelper::CreateStatic(xml, "static_ammo", weaponsParent, false);
    //m_ui_weapon_sign_ammo->SetEllipsis( CUIStatic::eepEnd, 2 );

    m_ui_weapon_cur_ammo = UIHelper::CreateStatic(xml, "static_cur_ammo", this, false);
    m_ui_weapon_fmj_ammo = UIHelper::CreateStatic(xml, "static_fmj_ammo", this, false);
    m_ui_weapon_ap_ammo = UIHelper::CreateStatic(xml, "static_ap_ammo", this, false);
    m_ui_weapon_third_ammo = UIHelper::CreateStatic(xml, "static_third_ammo", this, false); //Alundaio: Option to display a third ammo type
    m_fire_mode = UIHelper::CreateStatic(xml, "static_fire_mode", this, false);
    m_ui_grenade = UIHelper::CreateStatic(xml, "static_grenade", this, false);

    m_ui_weapon_icon = UIHelper::CreateStatic(xml, "static_wpn_icon", weaponsParent);
    m_ui_weapon_icon->SetShader(InventoryUtilities::GetEquipmentIconsShader());
    //	m_ui_weapon_icon->Enable	( false );
    m_ui_weapon_icon_rect = m_ui_weapon_icon->GetWndRect();

    m_progress_self = UIHelper::CreateProgressShape(xml, "progress", this, false);

    if ((m_bleeding = UIHelper::CreateStatic(xml, "bleeding", this, false)))
    {
        m_bleeding->Show(false);
    }

    /*
        m_bleeding_lev1 = UIHelper::CreateStatic( xml, "bleeding_level_1", this );
        m_bleeding_lev1->Show( false );

        m_bleeding_lev2 = UIHelper::CreateStatic( xml, "bleeding_level_2", this );
        m_bleeding_lev2->Show( false );

        m_bleeding_lev3 = UIHelper::CreateStatic( xml, "bleeding_level_3", this );
        m_bleeding_lev3->Show( false );

        m_radiation_lev1 = UIHelper::CreateStatic( xml, "radiation_level_1", this );
        m_radiation_lev1->Show( false );

        m_radiation_lev2 = UIHelper::CreateStatic( xml, "radiation_level_2", this );
        m_radiation_lev2->Show( false );

        m_radiation_lev3 = UIHelper::CreateStatic( xml, "radiation_level_3", this );
        m_radiation_lev3->Show( false );
    */

    xml.SetLocalRoot(stored_root);
}

void CUIHudStatesWnd::on_connected() { Load_section(); }
void CUIHudStatesWnd::Load_section()
{
    //	m_actor_radia_factor = pSettings->r_float( "radiation_zone_detector", "actor_radia_factor" );

    float zone_feel_radius[ALife::infl_max_count];

    const auto Load_section_type = [this, &zone_feel_radius](ALife::EInfluenceType type, pcstr section)
    {
        zone_feel_radius[type] = pSettings->read_if_exists<float>(section, "zone_radius", 1.0f);
        if (zone_feel_radius[type] <= 0.0f)
        {
            zone_feel_radius[type] = 1.0f;
        }
        if (m_zone_feel_radius_max < zone_feel_radius[type])
        {
            m_zone_feel_radius_max = zone_feel_radius[type];
        }
        m_zone_threshold[type] = pSettings->read_if_exists<float>(section, "threshold", 0.05f);
    };

    Load_section_type(ALife::infl_rad, "radiation_zone_detector");
    Load_section_type(ALife::infl_fire, "fire_zone_detector");
    Load_section_type(ALife::infl_acid, "acid_zone_detector");
    Load_section_type(ALife::infl_psi, "psi_zone_detector");
    Load_section_type(ALife::infl_electra, "electra_zone_detector"); // no uistatic

    hud_zones_list.load("all_zone_detector", "zone", [this, zone_feel_radius](const shared_str& item_sect) -> ITEM_TYPE
    {
        const auto hit_type = pSettings->read_if_exists<pcstr>(item_sect, "hit_type", nullptr);
        if (!hit_type)
            return {};

        float radius = 5.0f;

        switch (const auto type = g_tfHitType2InfluenceType(ALife::g_tfString2HitType(hit_type)))
        {
        case ALife::infl_rad:
        case ALife::infl_fire:
        case ALife::infl_acid:
        case ALife::infl_psi:
        case ALife::infl_electra:
            radius = zone_feel_radius[type];
            break;
        }

        return
        {
            .radius = radius,
        };
    });
}

void CUIHudStatesWnd::Update()
{
    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
    {
        return;
    }

    UpdateHealth(actor);
    UpdateActiveItemInfo(actor);
    UpdateIndicators(actor);

    UpdateZones();

    inherited::Update();
}

void CUIHudStatesWnd::UpdateHealth(CActor* actor)
{
    //	if ( Device.dwTimeGlobal - m_timer_1sec > 1000 ) // 1 sec
    //	{
    //		m_timer_1sec = Device.dwTimeGlobal;
    //	}

    const float cur_health = actor->GetfHealth();
    m_ui_health_bar->SetProgressPos(iCeil(cur_health * 100.0f * 35.f) / 35.f);
    if (_abs(cur_health - m_last_health) > m_health_blink)
    {
        m_last_health = cur_health;
        m_ui_health_bar->m_UIProgressItem.ResetColorAnimation();
    }

    if (m_ui_stamina_bar)
    {
        const float cur_stamina = actor->conditions().GetPower();
        m_ui_stamina_bar->SetProgressPos(iCeil(cur_stamina * 100.0f * 35.f) / 35.f);
        if (!actor->conditions().IsCantSprint())
        {
            m_ui_stamina_bar->m_UIProgressItem.ResetColorAnimation();
        }
    }

    if (m_static_armor && m_ui_armor_bar)
    {
        CCustomOutfit* outfit = actor->GetOutfit();
        if (outfit)
        {
            m_static_armor->Show(true);
            m_ui_armor_bar->Show(true);
            m_ui_armor_bar->SetProgressPos(outfit->GetCondition() * 100.0f);
        }
        else
        {
            m_static_armor->Show(false);
            m_ui_armor_bar->Show(false);
        }
    }

    if (m_bleeding)
    {
        if (actor->conditions().BleedingSpeed() > 0.01f)
            m_bleeding->Show(true);
        else
            m_bleeding->Show(false);
    }
    /*
    float bleeding_speed = actor->conditions().BleedingSpeed();
    if(bleeding_speed > 0.01f)
        m_bleeding_lev1->Show(true);
    else
        m_bleeding_lev1->Show(false);

    if(bleeding_speed > 0.35f)
        m_bleeding_lev2->Show(true);
    else
        m_bleeding_lev2->Show(false);

    if(bleeding_speed > 0.7f)
        m_bleeding_lev3->Show(true);
    else
        m_bleeding_lev3->Show(false);


    if(m_radia_self > 0.01f)
        m_radiation_lev1->Show(true);
    else
        m_radiation_lev1->Show(false);

    if(m_radia_self > 0.35f)
        m_radiation_lev2->Show(true);
    else
        m_radiation_lev2->Show(false);

    if(m_radia_self > 0.7f)
        m_radiation_lev3->Show(true);
    else
        m_radiation_lev3->Show(false);
    */

    if (m_progress_self)
        m_progress_self->SetPos(m_radia_self);
}

void CUIHudStatesWnd::UpdateActiveItemInfo(CActor* actor)
{
    PIItem item = actor->inventory().ActiveItem();
    if (item)
    {
        if (m_b_force_update)
        {
            if (item->cast_weapon())
                item->cast_weapon()->ForceUpdateAmmo();
            m_b_force_update = false;
        }

        II_BriefInfo item_info;
        item->GetBriefInfo(item_info);

        if (m_fire_mode)
            m_fire_mode->SetText(item_info.fire_mode.c_str());
        SetAmmoIcon(item_info.icon.c_str());

        if (m_ui_weapon_cur_ammo)
        {
            m_ui_weapon_cur_ammo->Show(true);
            m_ui_weapon_cur_ammo->SetText(item_info.cur_ammo.c_str());
        }

        if (m_ui_weapon_fmj_ammo)
        {
            m_ui_weapon_fmj_ammo->Show(true);
            m_ui_weapon_fmj_ammo->SetText(item_info.fmj_ammo.c_str());
            m_ui_weapon_fmj_ammo->SetTextColor(color_rgba(238, 155, 23, 150));
        }

        if (m_ui_weapon_ap_ammo)
        {
            m_ui_weapon_ap_ammo->Show(true);
            m_ui_weapon_ap_ammo->SetText(item_info.ap_ammo.c_str());
            m_ui_weapon_ap_ammo->SetTextColor(color_rgba(238, 155, 23, 150));
        }

        //Alundaio: Third ammo type and also set text color for each ammo type
        if (m_ui_weapon_third_ammo)
        {
            m_ui_weapon_third_ammo->Show(true);
            m_ui_weapon_third_ammo->SetText(item_info.third_ammo.c_str());
            m_ui_weapon_third_ammo->SetTextColor(color_rgba(238, 155, 23, 150));
        }

        if (m_ui_weapon_sign_ammo)
        {
            if (item_info.cur_ammo.size() && item_info.total_ammo.size())
            {
                string64 temp;
                xr_sprintf(temp, "%s/%s", item_info.cur_ammo.c_str(), item_info.total_ammo.c_str());

                m_ui_weapon_sign_ammo->Show(true);
                m_ui_weapon_sign_ammo->SetText(temp);

                // hack ^ begin
                CGameFont* pFont32 = GEnv.UI->Font().pFontGraffiti32Russian;
                CGameFont* pFont22 = GEnv.UI->Font().pFontGraffiti22Russian;
                CGameFont* pFont = pFont32;

                if (UICore::is_widescreen())
                {
                    pFont = pFont22;
                }
                else
                {
                    if (xr_strlen(temp) > 5)
                    {
                        pFont = pFont22;
                    }
                }
                m_ui_weapon_sign_ammo->SetFont(pFont);
            }
            else
            {
                m_ui_weapon_sign_ammo->Show(false);
            }
        }

        if (m_fire_mode)
            m_fire_mode->Show(true);

        if (m_ui_grenade)
        {
            m_ui_grenade->Show(true);

            m_ui_grenade->SetText(item_info.grenade.c_str());

            CWeaponMagazinedWGrenade* wpn = smart_cast<CWeaponMagazinedWGrenade*>(item);
            if (wpn && wpn->m_bGrenadeMode)
                m_ui_grenade->SetTextColor(color_rgba(238, 155, 23, 255));
            else
                m_ui_grenade->SetTextColor(color_rgba(238, 155, 23, 150));
        }

        if (CWeaponMagazined* wpnm = smart_cast<CWeaponMagazined*>(item))
        {
            if (wpnm->m_ammoType == 0 && m_ui_weapon_fmj_ammo)
                m_ui_weapon_fmj_ammo->SetTextColor(color_rgba(238, 155, 23, 255));
            else if (wpnm->m_ammoType == 1 && m_ui_weapon_ap_ammo)
                m_ui_weapon_ap_ammo->SetTextColor(color_rgba(238, 155, 23, 255));
            else if (wpnm->m_ammoType == 2 && m_ui_weapon_third_ammo)
                m_ui_weapon_third_ammo->SetTextColor(color_rgba(238, 155, 23, 255));
        }
        //-Alundaio
    }
    else
    {
        m_ui_weapon_icon->Show(false);

        if (m_ui_weapon_cur_ammo)
            m_ui_weapon_cur_ammo->Show(false);

        if (m_ui_weapon_fmj_ammo)
            m_ui_weapon_fmj_ammo->Show(false);

        if (m_ui_weapon_ap_ammo)
            m_ui_weapon_ap_ammo->Show(false);

        if (m_ui_weapon_third_ammo)
            m_ui_weapon_third_ammo->Show(false);

        if (m_ui_weapon_sign_ammo)
            m_ui_weapon_sign_ammo->Show(false);

        if (m_fire_mode)
            m_fire_mode->Show(false);

        if (m_ui_grenade)
            m_ui_grenade->Show(false);
    }
}

void CUIHudStatesWnd::SetAmmoIcon(const shared_str& sect_name)
{
    if (!m_ui_weapon_icon)
        return;

    if (!sect_name.size())
    {
        m_ui_weapon_icon->Show(false);
        return;
    }
    m_ui_weapon_icon->Show(true);

    Frect texture_rect;
    texture_rect.x1 = pSettings->r_float(sect_name, "inv_grid_x") * INV_GRID_WIDTH;
    texture_rect.y1 = pSettings->r_float(sect_name, "inv_grid_y") * INV_GRID_HEIGHT;
    texture_rect.x2 = pSettings->r_float(sect_name, "inv_grid_width") * INV_GRID_WIDTH;
    texture_rect.y2 = pSettings->r_float(sect_name, "inv_grid_height") * INV_GRID_HEIGHT;
    texture_rect.rb.add(texture_rect.lt);
    m_ui_weapon_icon->GetUIStaticItem().SetTextureRect(texture_rect);
    m_ui_weapon_icon->SetStretchTexture(true);

    float w, h;
    if (ShadowOfChernobylMode)
    {
        h = texture_rect.height() * 0.8f;
        w = texture_rect.width() * (UI().is_widescreen() ? 0.7f : 0.8f);
        float posx_16 = 30.0f;
        float posx = 32.0f;
        if (texture_rect.width() > 2.01f * INV_GRID_WIDTH)
        {
            w = INV_GRID_WIDTH * 1.6f;
        }
        if (texture_rect.width() < 1.01f * INV_GRID_WIDTH)
        {
            m_ui_weapon_icon->SetTextureOffset(UI().is_widescreen() ? posx_16 : posx, 5.0f);
        }
        else
        {
            posx_16 = 12.f;
            posx = 14.f;
            m_ui_weapon_icon->SetTextureOffset(UI().is_widescreen() ? posx_16 : posx, 5.0f);
        }
        m_ui_weapon_icon->SetWidth(w);
    }
    else if (ClearSkyMode)
    {
        h = texture_rect.height() * 0.65f;
        w = texture_rect.width() * 0.65f;
        float posx_16 = 8.33f;
        float posx = 10.0f;
        if (texture_rect.width() > 2.01f * INV_GRID_WIDTH)
        {
            w = INV_GRID_WIDTH * 1.5f;
        }
        if (texture_rect.width() < 1.01f * INV_GRID_WIDTH)
        {
            m_ui_weapon_icon->SetTextureOffset(UI().is_widescreen() ? posx_16 : posx, 0.0f);
        }
        else
        {
            m_ui_weapon_icon->SetTextureOffset(0.0f, 0.0f);
        }
        m_ui_weapon_icon->SetWidth(UI().is_widescreen() ? w * 0.833f : w);
    }
    else
    {
        h = texture_rect.height() * 0.8f;
        w = texture_rect.width() * 0.8f;
        // now perform only width scale for ammo, which (W)size >2
        if (texture_rect.width() > 2.01f * INV_GRID_WIDTH)
            w = INV_GRID_WIDTH * 1.5f;
        m_ui_weapon_icon->SetWidth(w * UI().get_current_kx());
    }
    m_ui_weapon_icon->SetHeight(h);
}
// ------------------------------------------------------------------------------------------------
void CUIHudStatesWnd::UpdateZones()
{
    // float actor_radia = m_actor->conditions().GetRadiation() * m_actor_radia_factor;
    // m_radia_hit = _max( m_zone_cur_power[it_rad], actor_radia );

    const CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
        return;

    if (const CPda* pda = actor->GetPDA())
    {
        for (const auto& game_object : pda->feel_touch)
        {
            CBaseMonster* const monster = smart_cast<CBaseMonster*>(game_object);
            if (!monster || !monster->g_Alive())
                continue;

            monster->play_detector_sound();
        }
    }

    m_radia_self = actor->conditions().GetRadiation();

    float zone_max_power = actor->conditions().GetZoneMaxPower(ALife::infl_rad);
    float power = actor->conditions().GetInjuriousMaterialDamage();
    power = power / zone_max_power;
    clamp(power, 0.0f, 1.1f);
    if (hud_zones_list.zone_cur_power[ALife::infl_rad] < power)
    {
        hud_zones_list.zone_cur_power[ALife::infl_rad] = power;
    }
    m_radia_hit = hud_zones_list.zone_cur_power[ALife::infl_rad];

    if (m_arrow)
        m_arrow->SetNewValue(m_radia_hit);
    if (m_arrow_shadow)
        m_arrow_shadow->SetPos(m_arrow->GetPos());
    /*
        power = actor->conditions().GetPsy();
        clamp( power, 0.0f, 1.1f );
        if ( hud_zones_list.zone_cur_power[ALife::infl_psi] < power )
        {
            hud_zones_list.zone_cur_power[ALife::infl_psi] = power;
        }
    */

    Fvector pos = Device.vCameraPosition;
    hud_zones_list.feel_touch_update(pos, m_zone_feel_radius_max);

    pos.y -= 0.5f;
    hud_zones_list.scan(pos, nullptr);
}

void CUIHudStatesWnd::UpdateIndicators(CActor* actor)
{
    if (m_fake_indicators_update)
        return;

    for (int i = 0; i < ALife::infl_max_count - 1; ++i)
    {
        ALife::EInfluenceType type = static_cast<ALife::EInfluenceType>(i);
        UpdateIndicatorType(actor, type);
    }
}

void CUIHudStatesWnd::UpdateIndicatorType(CActor* actor, ALife::EInfluenceType type, float power /*= 0.0f*/)
{
    pcstr base_texture{};
    switch (type)
    {
    case ALife::infl_rad:  base_texture = "ui_inGame2_triangle_Radiation_"; break;
    case ALife::infl_fire: base_texture = "ui_inGame2_triangle_Fire_"; break;
    case ALife::infl_acid: base_texture = "ui_inGame2_triangle_Biological_"; break;
    case ALife::infl_psi:  base_texture = "ui_inGame2_triangle_Psy_"; break;

    default:
        VERIFY(!"Failed EIndicatorType for CStatic!");
        [[fallthrough]];

    case ALife::infl_electra:
        return;
    }

    const float hit_power = m_fake_indicators_update ? power : hud_zones_list.zone_cur_power[type];
    ALife::EHitType hit_type = g_tfInfluenceType2HitType(type);

    CCustomOutfit* outfit = actor->GetOutfit();
    CHelmet* helmet = smart_cast<CHelmet*>(actor->inventory().ItemFromSlot(HELMET_SLOT));
    float protect = (outfit) ? outfit->GetDefHitTypeProtection(hit_type) : 0.0f;
    protect += (helmet) ? helmet->GetDefHitTypeProtection(hit_type) : 0.0f;
    protect += actor->GetProtection_ArtefactsOnBelt(hit_type);

    const auto& cur_booster_influences = actor->conditions().GetCurBoosterInfluences();
    CEntityCondition::BOOSTER_MAP::const_iterator it;
    if (hit_type == ALife::eHitTypeChemicalBurn)
    {
        it = cur_booster_influences.find(eBoostChemicalBurnProtection);
        if (it != cur_booster_influences.end())
            protect += it->second.fBoostValue;
    }
    else if (hit_type == ALife::eHitTypeRadiation)
    {
        it = cur_booster_influences.find(eBoostRadiationProtection);
        if (it != cur_booster_influences.end())
            protect += it->second.fBoostValue;
    }
    else if (hit_type == ALife::eHitTypeTelepatic)
    {
        it = cur_booster_influences.find(eBoostTelepaticProtection);
        if (it != cur_booster_influences.end())
            protect += it->second.fBoostValue;
    }

    float max_power = actor->conditions().GetZoneMaxPower(hit_type);
    R_ASSERT1_CURE(_valid(max_power) && !fis_zero(max_power),
    {
        max_power = 1.0f;
    });

    bool green_condition = hit_power <= protect;
    if (m_fake_indicators_update || ClearSkyMode)
    {
        protect /= max_power; // = 0..1
        max_power = 1.0f;
        green_condition = hit_power < protect;
    }

    pcstr texture_color;
    u32 color;
    float zone_danger = 0.0f;
    bool light_anim = false;

    if (hit_power < EPS)
    {
        texture_color = "green";
        color = color_rgba(255, 255, 255, 255);
    }
    else if (green_condition)
    {
        texture_color = "green";
        color = color_rgba(0, 255, 0, 255);
    }
    else if (hit_power - protect < m_zone_threshold[type])
    {
        texture_color = "yellow";
        color = color_rgba(255, 255, 0, 255);
    }
    else
    {
        light_anim = true;
        texture_color = "red";
        color = color_rgba(255, 0, 0, 255);
        zone_danger = (hit_power - protect) / max_power;
    }

    actor->conditions().SetZoneDanger(zone_danger, type);

    CUIStatic* indicator = m_indik[type];
    if (!indicator)
        return;

    // Switch LA
    if (light_anim)
    {
        indicator->SetColorAnimation(m_color_animation[type]);
    }
    else
    {
        indicator->SetColorAnimation(nullptr, 0); // turn off
    }

    string256 final_texture;
    xr_sprintf(final_texture, "%s%s", base_texture, texture_color);
    if (CUITextureMaster::ItemExist(final_texture))
    {
        indicator->Show(hit_power >= EPS);
        indicator->InitTexture(final_texture);
    }
    else
    {
        indicator->Show(true);
        indicator->SetTextureColor(color);
    }

}

void CUIHudStatesWnd::DrawZoneIndicators()
{
    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
        return;

    UpdateIndicators(actor);

    for (const auto& [_, indicator] : m_indik)
    {
        if (indicator && indicator->IsShown())
            indicator->Draw();
    }
}

void CUIHudStatesWnd::EnableFakeIndicators(bool enable) { m_fake_indicators_update = enable; }
