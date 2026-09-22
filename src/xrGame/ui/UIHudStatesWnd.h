#pragma once

#include "xrUICore/Windows/UIWindow.h"

#include "xrServerEntities/alife_space.h"
#include "xrServerEntities/inventory_space.h"

#include "actor_defs.h"
#include "CustomDetector.h" // for CZonesList

class CUIStatic;
class CUIProgressBar;
class CUIProgressShape;
class CUIXml;
class UI_Arrow;
class CActor;

class CUIHudStatesWnd final : public CUIWindow
{
private:
    typedef CUIWindow inherited;
    //-	typedef ALife::EInfluenceType	EIndicatorType;

    CUIStatic* m_back;
    CUIStatic* m_back_v;
    CUIStatic* m_back_over_arrow;
    CUIStatic* m_static_health;
    CUIStatic* m_static_armor;
    CUIStatic* m_static_weapon;

    xr_map<ALife::EInfluenceType, CUIStatic*> m_indik;

    CUIStatic* m_ui_weapon_cur_ammo;
    CUIStatic* m_ui_weapon_fmj_ammo;
    CUIStatic* m_ui_weapon_ap_ammo;
    CUIStatic* m_ui_weapon_third_ammo; //Alundaio
    CUIStatic* m_fire_mode;
    CUIStatic* m_ui_grenade;
    II_BriefInfo m_item_info;

    CUIStatic* m_ui_weapon_sign_ammo;
    CUIStatic* m_ui_weapon_icon;
    Frect m_ui_weapon_icon_rect;

    CUIProgressBar* m_ui_health_bar;
    CUIProgressBar* m_ui_armor_bar;
    CUIProgressBar* m_ui_stamina_bar;

    CUIProgressShape* m_progress_self;
    CUIStatic* m_radia_damage;
    UI_Arrow* m_arrow{};
    UI_Arrow* m_arrow_shadow{};

    CUIStatic* m_bleeding;
    /*
        CUIStatic*			m_bleeding_lev1;
        CUIStatic*			m_bleeding_lev2;
        CUIStatic*			m_bleeding_lev3;

        CUIStatic*			m_radiation_lev1;
        CUIStatic*			m_radiation_lev2;
        CUIStatic*			m_radiation_lev3;
    */
    float m_last_health{};
    float m_health_blink;

    float m_radia_self{};
    //	float				m_actor_radia_factor;
    float m_radia_hit{};

    float m_zone_threshold[ALife::infl_max_count];
    color_animation m_color_animation[ALife::infl_max_count];

    float m_zone_feel_radius_max{};

    bool m_fake_indicators_update{};
    bool m_b_force_update;

    CZoneList hud_zones_list;

public:
    CUIHudStatesWnd();

    void InitFromXml(CUIXml& xml, LPCSTR path);
    void Load_section();
    virtual void Update();
    //	virtual void	Draw				();

    void on_connected();
    void reset_ui();
    void UpdateHealth(CActor* actor);
    void SetAmmoIcon(const shared_str& sect_name);
    void UpdateActiveItemInfo(CActor* actor);

    void UpdateZones();
    void UpdateIndicators(CActor* actor);
    void UpdateIndicatorType(CActor* actor, ALife::EInfluenceType type, float power = 0.0f);

    [[nodiscard]]
    float get_zone_cur_power(const ALife::EHitType hit_type) const
    {
        ALife::EInfluenceType iz_type = g_tfHitType2InfluenceType(hit_type);
        if (iz_type == ALife::infl_max_count)
            return 0.0f;
        return hud_zones_list.zone_cur_power[iz_type];
    }

    [[nodiscard]]
    float get_main_sensor_value() const
    {
        return m_radia_hit;
    }

    void DrawZoneIndicators();

    void EnableFakeIndicators(bool enable);

    pcstr GetDebugType() override { return "CUIHudStatesWnd"; }

}; // class CUIHudStatesWnd
