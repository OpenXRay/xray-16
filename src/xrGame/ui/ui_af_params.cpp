#include "stdafx.h"
#include "ui_af_params.h"
#include "UIStatic.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "Common/object_broker.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "string_table.h"
#include "Inventory_Item.h"
#include "Artefact.h"

#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "ActorBackpack.h"


u32 const red_clr = color_argb(255, 210, 50, 50);
u32 const green_clr = color_argb(255, 170, 170, 170);

CUIArtefactParams::CUIArtefactParams()
{
    for (u32 i = 0; i < af_immunity_count; ++i)
    {
        m_immunity_item[i] = nullptr;
    }
    for (u32 i = 0; i < ALife::eRestoreTypeMax; ++i)
    {
        m_restore_item[i] = nullptr;
    }
    m_additional_weight = nullptr;
	m_disp_condition = nullptr;
	m_fJumpSpeed = nullptr;
	m_fWalkAccel = nullptr;
	m_fOverweightWalkAccel = nullptr;
	m_Prop_line = nullptr;
}

CUIArtefactParams::~CUIArtefactParams()
{
	delete_data	( m_immunity_item );
	delete_data	( m_restore_item );
	xr_delete	( m_additional_weight );
	xr_delete(m_disp_condition);
	xr_delete(m_fJumpSpeed);
	xr_delete(m_fWalkAccel);
	xr_delete(m_fOverweightWalkAccel);
	xr_delete	( m_Prop_line );
}

constexpr pcstr af_immunity_section_names[] = // ALife::EInfluenceType
{
    "radiation_immunity", // infl_rad=0
    "burn_immunity", // infl_fire=1
    "chemical_burn_immunity", // infl_acid=2
    "telepatic_immunity", // infl_psi=3
    "shock_immunity", // infl_electra=4
    "wound_immunity",
    "fire_wound_immunity",
    "explosion_immunity",
    "strike_immunity",
};

constexpr pcstr af_restore_section_names[] = // ALife::EConditionRestoreType
{
        "health_restore_speed", // eHealthRestoreSpeed=0
        "satiety_restore_speed", // eSatietyRestoreSpeed=1
        "power_restore_speed", // ePowerRestoreSpeed=2
        "bleeding_restore_speed", // eBleedingRestoreSpeed=3
        "radiation_restore_speed", // eRadiationRestoreSpeed=4
};

constexpr pcstr af_immunity_caption[] = // ALife::EInfluenceType
{
    "ui_inv_outfit_radiation_protection", // "(radiation_imm)",
    "ui_inv_outfit_burn_protection", // "(burn_imm)",
    "ui_inv_outfit_chemical_burn_protection", // "(chemical_burn_imm)",
    "ui_inv_outfit_telepatic_protection", // "(telepatic_imm)",
    "ui_inv_outfit_shock_protection", // "(shock_imm)",
    "ui_inv_outfit_wound_protection", // "(wound_imm)",
    "ui_inv_outfit_fire_wound_protection", // "(fire_wound_imm)",
    "ui_inv_outfit_explosion_protection", // "(explosion_imm)",
    "ui_inv_outfit_strike_protection",	 // "(strike_imm)",
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

void CUIArtefactParams::InitFromXml(CUIXml& xml)
{
    LPCSTR base = "af_params";

    XML_NODE stored_root = xml.GetLocalRoot();
    XML_NODE base_node = xml.NavigateToNode(base, 0);
    if (!base_node) { return; }
    CUIXmlInit::InitWindow(xml, base, 0, this);
    xml.SetLocalRoot(base_node);

    m_Prop_line = new CUIStatic();
    AttachChild(m_Prop_line);
    m_Prop_line->SetAutoDelete(false);
    CUIXmlInit::InitStatic(xml, "prop_line", 0, m_Prop_line);

    //Alundaio: Show AF Condition
    m_disp_condition = new UIArtefactParamItem();
    m_disp_condition->Init(xml, "condition");
    m_disp_condition->SetAutoDelete(false);
    pcstr name = CStringTable().translate("ui_inv_af_condition").c_str();
    m_disp_condition->SetCaption(name);
    xml.SetLocalRoot(base_node);
    //-Alundaio

    for (u32 i = 0; i < af_immunity_count; ++i)
    {
        m_immunity_item[i] = new UIArtefactParamItem();
        m_immunity_item[i]->Init(xml, af_immunity_section_names[i]);
        m_immunity_item[i]->SetAutoDelete(false);

        LPCSTR name = CStringTable().translate(af_immunity_caption[i]).c_str();
        m_immunity_item[i]->SetCaption(name);

        xml.SetLocalRoot(base_node);
    }

    for (u32 i = 0; i < ALife::eRestoreTypeMax; ++i)
    {
        m_restore_item[i] = new UIArtefactParamItem();
        m_restore_item[i]->Init(xml, af_restore_section_names[i]);
        m_restore_item[i]->SetAutoDelete(false);

        LPCSTR name = CStringTable().translate(af_restore_caption[i]).c_str();
        m_restore_item[i]->SetCaption(name);

        xml.SetLocalRoot(base_node);
    }

    // Alundaio: AF movement modifiers
    {
        m_fJumpSpeed = new UIArtefactParamItem();
        m_fJumpSpeed->Init(xml, "jump_speed");
        m_fJumpSpeed->SetAutoDelete(false);
        LPCSTR name = CStringTable().translate("ui_inv_af_jump_speed").c_str();
        m_fJumpSpeed->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }
    {
        m_fWalkAccel = new UIArtefactParamItem();
        m_fWalkAccel->Init(xml, "walk_accel");
        m_fWalkAccel->SetAutoDelete(false);
        LPCSTR name = CStringTable().translate("ui_inv_af_walk_accel").c_str();
        m_fWalkAccel->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_fOverweightWalkAccel = new UIArtefactParamItem();
        m_fOverweightWalkAccel->Init(xml, "overweight_walk_accel");
        m_fOverweightWalkAccel->SetAutoDelete(false);
        LPCSTR name = CStringTable().translate("ui_inv_af_overweight_walk_accel").c_str();
        m_fOverweightWalkAccel->SetCaption(name);
        xml.SetLocalRoot(base_node);
    }

    {
        m_additional_weight = new UIArtefactParamItem();
        m_additional_weight->Init(xml, "additional_weight");
        m_additional_weight->SetAutoDelete(false);

        LPCSTR name = CStringTable().translate("ui_inv_weight").c_str();
        m_additional_weight->SetCaption(name);

        // xml.SetLocalRoot( base_node );
    }

    xml.SetLocalRoot(stored_root);
}

bool CUIArtefactParams::Check(const shared_str& af_section)
{
    return !!pSettings->line_exist(af_section, "af_actor_properties");
}

void CUIArtefactParams::SetInfo(const CArtefact* pInvItem)
{
    DetachAll();
    AttachChild(m_Prop_line);

    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor)
        return;

    const shared_str& af_section = pInvItem->cNameSect();

    float val = 0.0f, max_val = 1.0f;
    Fvector2 pos;
    float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

	m_disp_condition->SetValue(pInvItem->GetCondition());
    pos.set(m_disp_condition->GetWndPos());
    pos.y = h;
    m_disp_condition->SetWndPos(pos);
    h += m_disp_condition->GetWndSize().y;
    AttachChild(m_disp_condition);

    for (u32 i = 0; i < af_immunity_count; ++i)
    {
        shared_str const& sect = pSettings->r_string(af_section, "hit_absorbation_sect");
        val = READ_IF_EXISTS(pSettings, r_float, sect, af_immunity_section_names[i], 0.f);
        if (fis_zero(val))
            continue;

        val *= pInvItem->GetCondition();
        max_val = actor->conditions().GetZoneMaxPower((ALife::EInfluenceType)i);
        val /= max_val;
        m_immunity_item[i]->SetValue(val);

        pos.set(m_immunity_item[i]->GetWndPos());
        pos.y = h;
        m_immunity_item[i]->SetWndPos(pos);

        h += m_immunity_item[i]->GetWndSize().y;
        AttachChild(m_immunity_item[i]);
    }

	{
		float val = pInvItem->m_fJumpSpeed;
		if (val != 1.f)
		{
			m_fJumpSpeed->SetValue(val*pInvItem->GetCondition());
			pos.set(m_fJumpSpeed->GetWndPos());
			pos.y = h;
			m_fJumpSpeed->SetWndPos(pos);
			h += m_fJumpSpeed->GetWndSize().y;
			AttachChild(m_fJumpSpeed);
		}
		val = pInvItem->m_fWalkAccel;
		if (val != 1.f)
		{
			m_fWalkAccel->SetValue(val*pInvItem->GetCondition());
			pos.set(m_fWalkAccel->GetWndPos());
			pos.y = h;
			m_fWalkAccel->SetWndPos(pos);
			h += m_fWalkAccel->GetWndSize().y;
			AttachChild(m_fWalkAccel);
		}
	}

    {
        val = pSettings->r_float(af_section, "additional_inventory_weight");
        if (!fis_zero(val))
        {
            // val *= pInvItem->GetCondition();
            m_additional_weight->SetValue(val);

            pos.set(m_additional_weight->GetWndPos());
            pos.y = h;
            m_additional_weight->SetWndPos(pos);

            h += m_additional_weight->GetWndSize().y;
            AttachChild(m_additional_weight);
        }
    }

    for (u32 i = 0; i < ALife::eRestoreTypeMax; ++i)
    {
        val = pSettings->r_float(af_section, af_restore_section_names[i]);
        if (fis_zero(val))
            continue;

        val *= pInvItem->GetCondition();
        m_restore_item[i]->SetValue(val);

        pos.set(m_restore_item[i]->GetWndPos());
        pos.y = h;
        m_restore_item[i]->SetWndPos(pos);

        h += m_restore_item[i]->GetWndSize().y;
        AttachChild(m_restore_item[i]);
    }

    SetHeight(h);
}

void CUIArtefactParams::SetInfo(const CCustomOutfit* pInvItem)
{
	DetachAll();
	AttachChild(m_Prop_line);

	CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!actor)
	{
		return;
	}

	const shared_str& af_section = pInvItem->cNameSect();

	float val = 0.0f;
	Fvector2 pos;
	float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

	{
		float val = pInvItem->m_fJumpSpeed;
		if (val != 1.f)
		{
			m_fJumpSpeed->SetValue(val);
			pos.set(m_fJumpSpeed->GetWndPos());
			pos.y = h;
			m_fJumpSpeed->SetWndPos(pos);
			h += m_fJumpSpeed->GetWndSize().y;
			AttachChild(m_fJumpSpeed);
		}
		val = pInvItem->m_fWalkAccel;
		if (val != 1.f)
		{
			m_fWalkAccel->SetValue(val - 1.f);
			pos.set(m_fWalkAccel->GetWndPos());
			pos.y = h;
			m_fWalkAccel->SetWndPos(pos);
			h += m_fWalkAccel->GetWndSize().y;
			AttachChild(m_fWalkAccel);
		}
		val = pInvItem->m_fOverweightWalkK;
		if (val != 1.f)
		{
			m_fOverweightWalkAccel->SetValue(val - 1.f);
			pos.set(m_fOverweightWalkAccel->GetWndPos());
			pos.y = h;
			m_fOverweightWalkAccel->SetWndPos(pos);
			h += m_fOverweightWalkAccel->GetWndSize().y;
			AttachChild(m_fOverweightWalkAccel);
		}
	}

	{
		val = pSettings->r_float(af_section, "additional_inventory_weight");
		if (!fis_zero(val))
		{
			//val *= pInvItem->GetCondition();
			m_additional_weight->SetValue(val);

			pos.set(m_additional_weight->GetWndPos());
			pos.y = h;
			m_additional_weight->SetWndPos(pos);

			h += m_additional_weight->GetWndSize().y;
			AttachChild(m_additional_weight);
		}
	}

	SetHeight(h);
}

void CUIArtefactParams::SetInfo(const CHelmet* pInvItem) { DetachAll(); }
void CUIArtefactParams::SetInfo(const CBackpack* pInvItem)
{
	DetachAll();
	AttachChild(m_Prop_line);

	CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!actor)
	{
		return;
	}

	const shared_str& af_section = pInvItem->cNameSect();

	float val = 0.0f, max_val = 1.0f;
	Fvector2 pos;
	float h = m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y;

	{
		float val = pInvItem->m_fJumpSpeed;
		if (val != 1.f)
		{
			m_fJumpSpeed->SetValue(val);
			pos.set(m_fJumpSpeed->GetWndPos());
			pos.y = h;
			m_fJumpSpeed->SetWndPos(pos);
			h += m_fJumpSpeed->GetWndSize().y;
			AttachChild(m_fJumpSpeed);
		}
		val = pInvItem->m_fWalkAccel;
		if (val != 1.f)
		{
			m_fWalkAccel->SetValue(val - 1.f);
			pos.set(m_fWalkAccel->GetWndPos());
			pos.y = h;
			m_fWalkAccel->SetWndPos(pos);
			h += m_fWalkAccel->GetWndSize().y;
			AttachChild(m_fWalkAccel);
		}
		val = pInvItem->m_fOverweightWalkK;
		if (val != 1.f)
		{
			m_fOverweightWalkAccel->SetValue(val - 1.f);
			pos.set(m_fOverweightWalkAccel->GetWndPos());
			pos.y = h;
			m_fOverweightWalkAccel->SetWndPos(pos);
			h += m_fOverweightWalkAccel->GetWndSize().y;
			AttachChild(m_fOverweightWalkAccel);
		}
	}

	{
		val = pSettings->r_float(af_section, "additional_inventory_weight");
		if (!fis_zero(val))
		{
			//val *= pInvItem->GetCondition();
			m_additional_weight->SetValue(val);

			pos.set(m_additional_weight->GetWndPos());
			pos.y = h;
			m_additional_weight->SetWndPos(pos);

			h += m_additional_weight->GetWndSize().y;
			AttachChild(m_additional_weight);
		}
	}

	SetHeight(h);
}
/// ----------------------------------------------------------------

UIArtefactParamItem::UIArtefactParamItem()
{
    m_caption = nullptr;
    m_value = nullptr;
    m_magnitude = 1.0f;
    m_sign_inverse = false;

    m_unit_str._set("");
    m_texture_minus._set("");
    m_texture_plus._set("");
}

UIArtefactParamItem::~UIArtefactParamItem() {}
void UIArtefactParamItem::Init(CUIXml& xml, LPCSTR section)
{
    CUIXmlInit::InitWindow(xml, section, 0, this);
    xml.SetLocalRoot(xml.NavigateToNode(section));

    m_caption = UIHelper::CreateStatic(xml, "caption", this);
    m_value = UIHelper::CreateTextWnd(xml, "value", this);
    m_magnitude = xml.ReadAttribFlt("value", 0, "magnitude", 1.0f);
    m_sign_inverse = (xml.ReadAttribInt("value", 0, "sign_inverse", 0) == 1);

    LPCSTR unit_str = xml.ReadAttrib("value", 0, "unit_str", "");
    m_unit_str._set(CStringTable().translate(unit_str));

    LPCSTR texture_minus = xml.Read("texture_minus", 0, "");
    if (texture_minus && xr_strlen(texture_minus))
    {
        m_texture_minus._set(texture_minus);

        LPCSTR texture_plus = xml.Read("caption:texture", 0, "");
        m_texture_plus._set(texture_plus);
        VERIFY(m_texture_plus.size());
    }
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
