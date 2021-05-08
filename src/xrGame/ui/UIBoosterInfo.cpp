#include "StdAfx.h"
#include "UIBoosterInfo.h"
#include "xrUICore/Static/UIStatic.h"
#include "Common/object_broker.h"
#include "EntityCondition.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "string_table.h"

CUIBoosterInfo::CUIBoosterInfo()
{
    for (u32 i = 0; i < eBoostExplImmunity; ++i)
    {
        m_booster_items[i] = NULL;
    }
    m_booster_satiety = NULL;
    m_booster_anabiotic = NULL;
    m_booster_time = NULL;
    m_Prop_line = nullptr;
}

CUIBoosterInfo::~CUIBoosterInfo()
{
    delete_data(m_booster_items);
    xr_delete(m_booster_satiety);
    xr_delete(m_booster_anabiotic);
    xr_delete(m_booster_time);
    xr_delete(m_Prop_line);
}

LPCSTR boost_influence_caption[] = {"ui_inv_health", "ui_inv_power", "ui_inv_radiation", "ui_inv_bleeding",
    "ui_inv_outfit_additional_weight", "ui_inv_outfit_radiation_protection", "ui_inv_outfit_telepatic_protection",
    "ui_inv_outfit_chemical_burn_protection", "ui_inv_outfit_burn_immunity", "ui_inv_outfit_shock_immunity",
    "ui_inv_outfit_radiation_immunity", "ui_inv_outfit_telepatic_immunity", "ui_inv_outfit_chemical_burn_immunity"};

bool CUIBoosterInfo::InitFromXml(CUIXml& xml)
{
    LPCSTR base = "booster_params";
    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE base_node = xml.NavigateToNode(base, 0);
    if (!base_node)
        return false;

    CUIXmlInit::InitWindow(xml, base, 0, this);
    xml.SetLocalRoot(base_node);

    m_Prop_line = UIHelper::CreateStatic(xml, "prop_line", this, false);
    m_Prop_line->SetAutoDelete(false);

    for (u32 i = 0; i < eBoostExplImmunity; ++i)
    {
        m_booster_items[i] = xr_new<UIBoosterInfoItem>();
        m_booster_items[i]->Init(xml, ef_boosters_section_names[i]);
        m_booster_items[i]->SetAutoDelete(false);

        LPCSTR name = StringTable().translate(boost_influence_caption[i]).c_str();
        m_booster_items[i]->SetCaption(name);

        xml.SetLocalRoot(base_node);
    }

    m_booster_satiety = xr_new<UIBoosterInfoItem>();
    m_booster_satiety->Init(xml, "boost_satiety");
    m_booster_satiety->SetAutoDelete(false);
    LPCSTR name = StringTable().translate("ui_inv_satiety").c_str();
    m_booster_satiety->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_anabiotic = xr_new<UIBoosterInfoItem>();
    m_booster_anabiotic->Init(xml, "boost_anabiotic");
    m_booster_anabiotic->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_survive_surge").c_str();
    m_booster_anabiotic->SetCaption(name);
    xml.SetLocalRoot(base_node);

    m_booster_time = xr_new<UIBoosterInfoItem>();
    m_booster_time->Init(xml, "boost_time");
    m_booster_time->SetAutoDelete(false);
    name = StringTable().translate("ui_inv_effect_time").c_str();
    m_booster_time->SetCaption(name);

    xml.SetLocalRoot(stored_root);
    return true;
}

void CUIBoosterInfo::SetInfo(shared_str const& section)
{
    DetachAll();
    if (m_Prop_line)
        AttachChild(m_Prop_line);

    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
    {
        return;
    }

    //const auto& boosters = actor->conditions().GetCurBoosterInfluences();

    float val = 0.0f, max_val = 1.0f, h = 0.0f;
    Fvector2 pos;
    if (m_Prop_line)
        h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

    for (u32 i = 0; i < eBoostExplImmunity; ++i)
    {
        if (pSettings->line_exist(section.c_str(), ef_boosters_section_names[i]))
        {
            val = pSettings->r_float(section, ef_boosters_section_names[i]);
            if (fis_zero(val))
                continue;

            EBoostParams type = (EBoostParams)i;
            switch (type)
            {
            case eBoostHpRestore:
            case eBoostPowerRestore:
            case eBoostBleedingRestore:
            case eBoostMaxWeight: max_val = 1.0f; break;
            case eBoostRadiationRestore: max_val = -1.0f; break;
            case eBoostBurnImmunity: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_fire); break;
            case eBoostShockImmunity: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_electra); break;
            case eBoostRadiationImmunity:
            case eBoostRadiationProtection: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_rad); break;
            case eBoostTelepaticImmunity:
            case eBoostTelepaticProtection: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_psi); break;
            case eBoostChemicalBurnImmunity:
            case eBoostChemicalBurnProtection: max_val = actor->conditions().GetZoneMaxPower(ALife::infl_acid); break;
            }
            val /= max_val;
            m_booster_items[i]->SetValue(val);

            pos.set(m_booster_items[i]->GetWndPos());
            pos.y = h;
            m_booster_items[i]->SetWndPos(pos);

            h += m_booster_items[i]->GetWndSize().y;
            AttachChild(m_booster_items[i]);
        }
    }

    if (pSettings->line_exist(section.c_str(), "eat_satiety"))
    {
        val = pSettings->r_float(section, "eat_satiety");
        if (!fis_zero(val))
        {
            m_booster_satiety->SetValue(val);
            pos.set(m_booster_satiety->GetWndPos());
            pos.y = h;
            m_booster_satiety->SetWndPos(pos);

            h += m_booster_satiety->GetWndSize().y;
            AttachChild(m_booster_satiety);
        }
    }

    if (!xr_strcmp(section.c_str(), "drug_anabiotic"))
    {
        pos.set(m_booster_anabiotic->GetWndPos());
        pos.y = h;
        m_booster_anabiotic->SetWndPos(pos);

        h += m_booster_anabiotic->GetWndSize().y;
        AttachChild(m_booster_anabiotic);
    }

    if (pSettings->line_exist(section.c_str(), "boost_time"))
    {
        val = pSettings->r_float(section, "boost_time");
        if (!fis_zero(val))
        {
            m_booster_time->SetValue(val);
            pos.set(m_booster_time->GetWndPos());
            pos.y = h;
            m_booster_time->SetWndPos(pos);

            h += m_booster_time->GetWndSize().y;
            AttachChild(m_booster_time);
        }
    }
    SetHeight(h);
}

/// ----------------------------------------------------------------

UIBoosterInfoItem::UIBoosterInfoItem()
{
    m_caption = NULL;
    m_value = NULL;
    m_magnitude = 1.0f;
    m_show_sign = false;

    m_unit_str._set("");
    m_texture_minus._set("");
    m_texture_plus._set("");
}

UIBoosterInfoItem::~UIBoosterInfoItem() {}
void UIBoosterInfoItem::Init(CUIXml& xml, LPCSTR section)
{
    CUIXmlInit::InitWindow(xml, section, 0, this);
    xml.SetLocalRoot(xml.NavigateToNode(section));

    m_caption = UIHelper::CreateStatic(xml, "caption", this);
    m_value = UIHelper::CreateTextWnd(xml, "value", this);
    m_magnitude = xml.ReadAttribFlt("value", 0, "magnitude", 1.0f);
    m_show_sign = (xml.ReadAttribInt("value", 0, "show_sign", 1) == 1);

    LPCSTR unit_str = xml.ReadAttrib("value", 0, "unit_str", "");
    m_unit_str._set(StringTable().translate(unit_str));

    LPCSTR texture_minus = xml.Read("texture_minus", 0, "");
    if (texture_minus && xr_strlen(texture_minus))
    {
        m_texture_minus._set(texture_minus);

        LPCSTR texture_plus = xml.Read("caption:texture", 0, "");
        m_texture_plus._set(texture_plus);
        VERIFY(m_texture_plus.size());
    }
}

void UIBoosterInfoItem::SetCaption(LPCSTR name) { m_caption->TextItemControl()->SetText(name); }
void UIBoosterInfoItem::SetValue(float value)
{
    value *= m_magnitude;
    string32 buf;
    if (m_show_sign)
        xr_sprintf(buf, "%+.0f", value);
    else
        xr_sprintf(buf, "%.0f", value);

    pstr str;
    if (m_unit_str.size())
        STRCONCAT(str, buf, " ", m_unit_str.c_str());
    else
        STRCONCAT(str, buf);

    m_value->SetText(str);

    bool positive = (value >= 0.0f);
    m_value->SetTextColor(color_rgba(170, 170, 170, 255));

    if (m_texture_minus.size())
    {
        if (positive)
            m_caption->InitTexture(m_texture_plus.c_str());
        else
            m_caption->InitTexture(m_texture_minus.c_str());
    }
}
