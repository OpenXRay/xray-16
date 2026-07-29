#include "StdAfx.h"
#include "ui_af_params.h"
#include "xrUICore/Static/UIStatic.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "inventory_item.h"
#include "Common/object_broker.h"
#include "UIXmlInit.h"
#include "UIHelper.h"

constexpr pcstr af_params = "af_params";

constexpr u32 green_clr_cop = color_argb(255, 170, 170, 170);
constexpr u32 red_clr_cop   = color_argb(255, 210, 50, 50);

constexpr u32 green_clr_cs  = color_argb(250, 50, 210, 50);
constexpr u32 red_clr_cs    = color_argb(250, 210, 50, 50);

CUIArtefactParams::CUIArtefactParams() : CUIWindow(af_params) {}

CUIArtefactParams::~CUIArtefactParams()
{
    delete_data(m_immunity_item);
    delete_data(m_restore_item);
    xr_delete(m_additional_weight);
    xr_delete(m_Prop_line);
}

constexpr std::tuple<ALife::EHitType, cpcstr, cpcstr, float, bool, cpcstr> af_immunity[] =
{
    //{ ALife::EHitType,           "section",                "caption",                                magnitude, sign_inverse, "unit" }
    { ALife::eHitTypeRadiation,    "radiation_immunity",     "ui_inv_outfit_radiation_protection",     100.0f,    false,        "%" },
    { ALife::eHitTypeBurn,         "burn_immunity",          "ui_inv_outfit_burn_protection",          100.0f,    false,        "%" },
    { ALife::eHitTypeChemicalBurn, "chemical_burn_immunity", "ui_inv_outfit_chemical_burn_protection", 100.0f,    false,        "%" },
    { ALife::eHitTypeTelepatic,    "telepatic_immunity",     "ui_inv_outfit_telepatic_protection",     100.0f,    false,        "%" },
    { ALife::eHitTypeShock,        "shock_immunity",         "ui_inv_outfit_shock_protection",         100.0f,    false,        "%" },
    { ALife::eHitTypeStrike,       "strike_immunity",        "ui_inv_outfit_strike_protection",        100.0f,    false,        "%" },
    { ALife::eHitTypeWound,        "wound_immunity",         "ui_inv_outfit_wound_protection",         100.0f,    false,        "%" },
    { ALife::eHitTypeExplosion,    "explosion_immunity",     "ui_inv_outfit_explosion_protection",     100.0f,    false,        "%" },
    { ALife::eHitTypeFireWound,    "fire_wound_immunity",    "ui_inv_outfit_fire_wound_protection",    100.0f,    false,        "%" },
};

constexpr std::tuple<ALife::EConditionRestoreType, cpcstr, cpcstr, cpcstr, float, bool, cpcstr> af_restore[] =
{
    //{ ALife::EConditionRestoreType, "section",                 "actor_condition",     "caption",          magnitude, sign_inverse, "unit" }
    { ALife::eHealthRestoreSpeed,     "health_restore_speed",    "satiety_health_v",    "ui_inv_health",    100.0f,    false,        "%" },
    { ALife::eSatietyRestoreSpeed,    "satiety_restore_speed",   "satiety_v",           "ui_inv_satiety",   100.0f,    false,        "%" },
    { ALife::ePowerRestoreSpeed,      "power_restore_speed",     "satiety_power_v",     "ui_inv_power",     1.0f,      false,        nullptr },
    { ALife::eBleedingRestoreSpeed,   "bleeding_restore_speed",  "wound_incarnation_v", "ui_inv_bleeding", -100.0f,    true,         "%" },
    { ALife::eRadiationRestoreSpeed,  "radiation_restore_speed", "radiation_v",         "ui_inv_radiation", 1.0f,      true,         nullptr },
};
static_assert(std::size(af_restore) == ALife::eRestoreTypeMax,
    "All restore types should be listed in the tuple above.");

bool CUIArtefactParams::InitFromXml(CUIXml& xml)
{
    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE base_node = xml.NavigateToNode(af_params, 0);
    if (!base_node)
        return false;

    CUIXmlInit::InitWindow(xml, af_params, 0, this);
    xml.SetLocalRoot(base_node);

    if ((m_Prop_line = UIHelper::CreateStatic(xml, "prop_line", this, false)))
        m_Prop_line->SetAutoDelete(false);

    const u32 positive_color = m_Prop_line ? green_clr_cop : green_clr_cs;
    const u32 negative_color = m_Prop_line ? red_clr_cop : red_clr_cs;

    const auto create_item = [&](pcstr section, pcstr caption,
        float magnitude = 1.0f, bool is_sign_inverse = false, pcstr unit = "")
    {
        UIArtefactParamItem* item = xr_new<UIArtefactParamItem>(section);

        string256 buf;
        strconcat(buf, "static_", section);

        if (item->Init(xml, section, caption, positive_color, negative_color, magnitude, is_sign_inverse, unit) ||
            item->Init(xml, buf, caption, positive_color, negative_color, magnitude, is_sign_inverse, unit))
        {
            item->SetAutoDelete(false);
            return item;
        }
        xr_delete(item);
        return item;
    };

    //Alundaio: Show AF Condition
    m_disp_condition = create_item("condition", "ui_inv_af_condition");
    //-Alundaio

    for (auto [id, section, actor_condition, caption_id, magnitude, sign_inverse, unit] : af_restore)
    {
        const auto caption = StringTable().translate(caption_id).c_str();
        m_restore_item[id] = create_item(section, caption, magnitude, sign_inverse, unit);
    }
    for (auto [id, section, caption_id, magnitude, sign_inverse, unit] : af_immunity)
    {
        const auto caption = StringTable().translate(caption_id).c_str();
        m_immunity_item[id] = create_item(section, caption, magnitude, sign_inverse, unit);
    }

    const auto weight = StringTable().translate("ui_inv_weight", "ui_inv_outfit_additional_weight");
    m_additional_weight = create_item("additional_weight", weight.c_str());

    xml.SetLocalRoot(stored_root);
    return true;
}

bool CUIArtefactParams::Check(const shared_str& af_section) const
{
    return pSettings->line_exist(af_section, "af_actor_properties");
}

void CUIArtefactParams::SetInfo(const CInventoryItem& pInvItem)
{
    DetachAll();
    if (m_Prop_line)
        AttachChild(m_Prop_line);

    const CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
        return;

    const auto& af_section = pInvItem.object().cNameSect().c_str();
    const auto& actor_sect = actor->cNameSect().c_str();
    const auto& condition_sect = pSettings->read_if_exists<pcstr>(actor_sect, "condition_sect", actor_sect);
    const auto& hit_absorbation_sect = pSettings->r_string(af_section, "hit_absorbation_sect");

    float h = 0.0f;
    if (m_Prop_line)
        h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

    const auto setValue = [&](UIArtefactParamItem* item, const float val)
    {
        item->SetValue(val);

        Fvector2 pos = item->GetWndPos();
        pos.y = h;
        item->SetWndPos(pos);

        h += item->GetWndSize().y;
        AttachChild(item);
    };

    //Alundaio: Show AF Condition
    if (m_disp_condition)
        setValue(m_disp_condition, pInvItem.GetCondition());
    //-Alundaio

    const bool is_soc = GMLib.GetLibraryVersion() <= GAMEMTL_VERSION_SOC;

    for (auto [id, restore_section, actor_condition, restore_caption, magnitude, sign_inverse, unit] : af_restore)
    {
        if (!m_restore_item[id])
            continue;

        float val = pSettings->r_float(af_section, restore_section);
        if (fis_zero(val))
            continue;

        val = val * pInvItem.GetCondition();
        if (is_soc)
        {
            const float max_val = pSettings->r_float(condition_sect, actor_condition);
            val /= max_val;
        }
        setValue(m_restore_item[id], val);
    }

    CHitImmunity immunities;
    immunities.LoadImmunities(hit_absorbation_sect, pSettings, is_soc);

    for (auto [id, immunity_section, immunity_caption, magnitude, sign_inverse, unit] : af_immunity)
    {
        if (!m_immunity_item[id])
            continue;

        float val = immunities.GetHitImmunity(id);
        if (fis_zero(val))
            continue;

        val *= pInvItem.GetCondition();
        if (!is_soc)
        {
            const float max_val = actor->conditions().GetZoneMaxPower(id);
            val /= max_val;
        }
        setValue(m_immunity_item[id], val);
    }

    if (m_additional_weight)
    {
        float val = pSettings->r_float(af_section, "additional_inventory_weight");
        if (!fis_zero(val))
        {
            val *= pInvItem.GetCondition();
            setValue(m_additional_weight, val);
        }
    }

    SetHeight(h);
}

/// ----------------------------------------------------------------

UIArtefactParamItem::UIArtefactParamItem(pcstr param_name)
    : CUIStatic(param_name), m_positive_color(green_clr_cop), m_negative_color(red_clr_cop)
{
    AttachChild(&m_value);
}

bool UIArtefactParamItem::Init(CUIXml& xml, pcstr section, pcstr caption,
    u32 positive_color, u32 negative_color,
    float magnitude /*= 1.0f*/, bool is_sign_inverse /*= false*/, pcstr unit /*= ""*/)
{
    if (!CUIXmlInit::InitStatic(xml, section, 0, this, false))
        return false;

    const XML_NODE base_node = xml.GetLocalRoot();
    xml.SetLocalRoot(xml.NavigateToNode(section));

    m_caption = UIHelper::CreateStatic(xml, "caption", this, false);
    if (m_caption)
        m_caption->SetText(caption);
    else
        SetText(caption);

    m_positive_color = positive_color;
    m_negative_color = negative_color;

    if (!CUIXmlInit::InitStatic(xml, "value", 0, &m_value, m_caption))
    {
        CUIXmlInit::InitText(xml, section, 0, m_value.TextItemControl());

        const auto font = GetFont();
        float text_len = font->SizeOf_(GetText());
        float space_len = font->SizeOf_("  ");
        UI().ClientToScreenScaledWidth(text_len);
        UI().ClientToScreenScaledWidth(space_len);

        auto rect = GetWndRect();
        rect.x1 += TextItemControl()->m_TextOffset.x + text_len + space_len;
        m_value.SetWndRect(rect);

        m_magnitude = magnitude;
        m_sign_inverse = is_sign_inverse;
        m_unit_str = unit;
        CUIXmlInit::GetColor("green", m_positive_color);
        CUIXmlInit::GetColor("red", m_negative_color);
    }

    m_magnitude = xml.ReadAttribFlt("value", 0, "magnitude", m_magnitude);
    m_sign_inverse = xml.ReadAttribInt("value", 0, "sign_inverse", m_sign_inverse) == 1;

    if (cpcstr unit_str = xml.ReadAttrib("value", 0, "unit_str", m_unit_str.c_str()))
        m_unit_str = StringTable().translate(unit_str);

    m_positive_color = CUIXmlInit::GetColor(xml, "value:positive_color", 0, m_positive_color);
    m_negative_color = CUIXmlInit::GetColor(xml, "value:negative_color", 0, m_negative_color);

    pcstr texture_minus = xml.Read("texture_minus", 0, nullptr);
    if (texture_minus && texture_minus[0])
    {
        m_texture_minus = texture_minus;

        pcstr texture_plus = xml.Read("caption:texture", 0, nullptr);
        m_texture_plus = texture_plus;
        VERIFY(m_texture_plus.size());
    }
    xml.SetLocalRoot(base_node);
    return true;
}

void UIArtefactParamItem::SetValue(float value)
{
    value *= m_magnitude;
    string32 buf;
    xr_sprintf(buf, "%+.0f", value);

    pstr str;
    if (m_unit_str.size())
    {
        STRCONCAT(str, buf, " ", m_unit_str.c_str());
    }
    else // = ""
    {
        STRCONCAT(str, buf);
    }
    m_value.SetText(str);

    bool positive = value >= 0.0f;
    positive = m_sign_inverse ? !positive : positive;
    u32 color = positive ? m_positive_color : m_negative_color;
    m_value.SetTextColor(color);

    if (m_texture_minus.size() && m_caption)
    {
        if (positive)
        {
            m_caption->InitTexture(m_texture_plus.c_str());
        }
        else
        {
            m_caption->InitTexture(m_texture_minus.c_str());
        }
    }
}
