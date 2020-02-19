#include "StdAfx.h"
#include "ui_af_params.h"
#include "xrUICore/Static/UIStatic.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "Common/object_broker.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "string_table.h"

u32 const red_clr = color_argb(255, 210, 50, 50);
u32 const green_clr = color_argb(255, 170, 170, 170);

CUIArtefactParams::~CUIArtefactParams()
{
    delete_data(m_immunity_item);
    delete_data(m_restore_item);
    xr_delete(m_additional_weight);
    xr_delete(m_Prop_line);
}

LPCSTR af_immunity_section_names[] = // ALife::EInfluenceType
{
    "radiation_immunity", // infl_rad=0
    "burn_immunity", // infl_fire=1
    "chemical_burn_immunity", // infl_acid=2
    "telepatic_immunity", // infl_psi=3
    "shock_immunity", // infl_electra=4

    //"strike_immunity",
    //Alundaio: Uncommented
    "wound_immunity",
    "explosion_immunity",
    "fire_wound_immunity",
};

LPCSTR af_restore_section_names[] = // ALife::EConditionRestoreType
    {
        "health_restore_speed", // eHealthRestoreSpeed=0
        "satiety_restore_speed", // eSatietyRestoreSpeed=1
        "power_restore_speed", // ePowerRestoreSpeed=2
        "bleeding_restore_speed", // eBleedingRestoreSpeed=3
        "radiation_restore_speed", // eRadiationRestoreSpeed=4
};

LPCSTR af_immunity_caption[] = // ALife::EInfluenceType
{
    "ui_inv_outfit_radiation_protection", // "(radiation_imm)",
    "ui_inv_outfit_burn_protection", // "(burn_imm)",
    "ui_inv_outfit_chemical_burn_protection", // "(chemical_burn_imm)",
    "ui_inv_outfit_telepatic_protection", // "(telepatic_imm)",
    "ui_inv_outfit_shock_protection", // "(shock_imm)",

    //"ui_inv_outfit_strike_protection",	 // "(strike_imm)",

    //Alundaio: Uncommented
    "ui_inv_outfit_wound_protection", // "(wound_imm)",
    "ui_inv_outfit_explosion_protection", // "(explosion_imm)",
    "ui_inv_outfit_fire_wound_protection", // "(fire_wound_imm)",
};

LPCSTR af_restore_caption[] = // ALife::EConditionRestoreType
{
    "ui_inv_health", "ui_inv_satiety", "ui_inv_power", "ui_inv_bleeding", "ui_inv_radiation",
};

/*
LPCSTR af_actor_param_names[]=
{
    "satiety_health_v",
    "radiation_v",
    "satiety_v",
    "satiety_power_v",
    "wound_incarnation_v",
};
*/

bool CUIArtefactParams::InitFromXml(CUIXml& xml)
{
    LPCSTR base = "af_params";

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE base_node = xml.NavigateToNode(base, 0);
    if (!base_node)
    {
        return false;
    }
    CUIXmlInit::InitWindow(xml, base, 0, this);
    xml.SetLocalRoot(base_node);

    if ((m_Prop_line = UIHelper::CreateStatic(xml, "prop_line", this, false)))
        m_Prop_line->SetAutoDelete(false);

    for (u32 i = 0; i < ALife::infl_max_count; ++i)
    {
        m_immunity_item[i] = CreateItem(xml, af_immunity_section_names[i], af_immunity_caption[i]);
    }
    for (u32 i = 0; i < ALife::eRestoreTypeMax; ++i)
    {
        m_restore_item[i] = CreateItem(xml, af_restore_section_names[i], af_restore_caption[i]);
    }
    m_additional_weight = CreateItem(xml, "additional_weight", "ui_inv_weight", "ui_inv_outfit_additional_weight");

    xml.SetLocalRoot(stored_root);
    return true;
}

UIArtefactParamItem* CUIArtefactParams::CreateItem(CUIXml& uiXml, pcstr section,
    shared_str translationId, shared_str translationId2 /*= nullptr*/)
{
    UIArtefactParamItem* item = new UIArtefactParamItem();

    item->Init(uiXml, section);
    item->SetAutoDelete(false);

    // use either translationId or translationId2
    // but set translationId if both unavailable
    shared_str name = StringTable().translate(translationId);
    shared_str name2 = translationId2 != nullptr ? StringTable().translate(translationId2) : nullptr;

    if (name != translationId && name2 != translationId2)
        item->SetCaption(name2.c_str());
    else
        item->SetCaption(name.c_str());

    return item;
}

bool CUIArtefactParams::Check(const shared_str& af_section)
{
    return !!pSettings->line_exist(af_section, "af_actor_properties");
}

void CUIArtefactParams::SetInfo(shared_str const& af_section)
{
    DetachAll();
    if (m_Prop_line)
        AttachChild(m_Prop_line);

    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
    {
        return;
    }

    float val = 0.0f, max_val = 1.0f, h = 0.0f;
    if (m_Prop_line)
        h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

    const auto setValue = [&](UIArtefactParamItem* item)
    {
        item->SetValue(val);

        Fvector2 pos = item->GetWndPos();
        pos.y = h;
        item->SetWndPos(pos);

        h += item->GetWndSize().y;
        AttachChild(item);
    };

    for (u32 i = 0; i < ALife::infl_max_count; ++i)
    {
        shared_str const& sect = pSettings->r_string(af_section, "hit_absorbation_sect");
        val = pSettings->r_float(sect, af_immunity_section_names[i]);
        if (fis_zero(val))
        {
            continue;
        }
        max_val = actor->conditions().GetZoneMaxPower((ALife::EInfluenceType)i);
        val /= max_val;
        setValue(m_immunity_item[i]);
    }

    {
        val = pSettings->r_float(af_section, "additional_inventory_weight");
        if (!fis_zero(val))
        {
            setValue(m_additional_weight);
        }
    }

    for (u32 i = 0; i < ALife::eRestoreTypeMax; ++i)
    {
        val = pSettings->r_float(af_section, af_restore_section_names[i]);
        if (fis_zero(val))
        {
            continue;
        }
        setValue(m_restore_item[i]);
    }

    SetHeight(h);
}

/// ----------------------------------------------------------------

UIArtefactParamItem::UIArtefactParamItem()
    : m_magnitude(1.0f), m_sign_inverse(false), m_unit_str(""),
      m_texture_minus(""), m_texture_plus("") {}

void UIArtefactParamItem::Init(CUIXml& xml, LPCSTR section)
{
    CUIXmlInit::InitWindow(xml, section, 0, this);

    const XML_NODE base_node = xml.GetLocalRoot();
    xml.SetLocalRoot(xml.NavigateToNode(section));

    m_caption = UIHelper::CreateStatic(xml, "caption", this);
    m_value = UIHelper::CreateTextWnd(xml, "value", this);
    m_magnitude = xml.ReadAttribFlt("value", 0, "magnitude", 1.0f);
    m_sign_inverse = (xml.ReadAttribInt("value", 0, "sign_inverse", 0) == 1);

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
    xml.SetLocalRoot(base_node);
}

void UIArtefactParamItem::SetCaption(LPCSTR name) { m_caption->TextItemControl()->SetText(name); }
void UIArtefactParamItem::SetValue(float value)
{
    value *= m_magnitude;
    string32 buf;
    xr_sprintf(buf, "%+.0f", value);

    LPSTR str;
    if (m_unit_str.size())
    {
        STRCONCAT(str, buf, " ", m_unit_str.c_str());
    }
    else // = ""
    {
        STRCONCAT(str, buf);
    }
    m_value->SetText(str);

    bool positive = (value >= 0.0f);
    positive = (m_sign_inverse) ? !positive : positive;
    u32 color = (positive) ? green_clr : red_clr;
    m_value->SetTextColor(color);

    if (m_texture_minus.size())
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
