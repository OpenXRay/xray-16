#include "StdAfx.h"
#include "UIOutfitInfo.h"
#include "UIXmlInit.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/ProgressBar/UIDoubleProgressBar.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "player_hud.h"
#include "UIHelper.h"

constexpr std::tuple<ALife::EHitType, cpcstr, cpcstr> immunity_names[] =
{
    // { hit type,                 "immunity",               "immunity text" }
    { ALife::eHitTypeBurn,         "burn_immunity",          "ui_inv_outfit_burn_protection" },
    { ALife::eHitTypeShock,        "shock_immunity",         "ui_inv_outfit_shock_protection" },
    { ALife::eHitTypeChemicalBurn, "chemical_burn_immunity", "ui_inv_outfit_chemical_burn_protection" },
    { ALife::eHitTypeRadiation,    "radiation_immunity",     "ui_inv_outfit_radiation_protection" },
    { ALife::eHitTypeTelepatic,    "telepatic_immunity",     "ui_inv_outfit_telepatic_protection" },
    { ALife::eHitTypeStrike,       "strike_immunity",        "ui_inv_outfit_strike_protection" },
    { ALife::eHitTypeWound,        "wound_immunity",         "ui_inv_outfit_wound_protection" },
    { ALife::eHitTypeExplosion,    "explosion_immunity",     "ui_inv_outfit_explosion_protection" },
    { ALife::eHitTypeFireWound,    "fire_wound_immunity",    "ui_inv_outfit_fire_wound_protection" },
};

CUIOutfitImmunity::CUIOutfitImmunity()
    : CUIWindow("CUIOutfitImmunity"), m_name("Name"), m_value("Value")
{
    AttachChild(&m_name);
    AttachChild(&m_progress);
    AttachChild(&m_value);
    m_magnitude = 1.0f;
}

bool CUIOutfitImmunity::InitFromXml(CUIXml& xml_doc, pcstr base_str, pcstr immunity, pcstr immunity_text)
{
    CUIXmlInit::InitWindow(xml_doc, base_str, 0, this);

    string256 buf;

    strconcat(sizeof(buf), buf, base_str, ":", immunity);
    if (!CUIXmlInit::InitWindow(xml_doc, buf, 0, this, false))
        return false;

    CUIXmlInit::InitStatic(xml_doc, buf, 0, &m_name);
    m_name.TextItemControl()->SetTextST(immunity_text);

    strconcat(sizeof(buf), buf, base_str, ":", immunity, ":progress_immunity");
    m_progress.InitFromXml(xml_doc, buf);

    strconcat(sizeof(buf), buf, base_str, ":", immunity, ":static_value");
    if (xml_doc.NavigateToNode(buf, 0) && !CallOfPripyatMode)
    {
        CUIXmlInit::InitStatic(xml_doc, buf, 0, &m_value);
        m_value.Show(true);
    }
    else
    {
        m_value.Show(false);
    }

    m_magnitude = xml_doc.ReadAttribFlt(buf, 0, "magnitude", 1.0f);
    return true;
}

void CUIOutfitImmunity::SetProgressValue(float cur, float comp)
{
    cur *= m_magnitude;
    comp *= m_magnitude;
    m_progress.SetTwoPos(cur, comp);
    string32 buf;
    //	xr_sprintf( buf, sizeof(buf), "%d %%", (int)cur );
    xr_sprintf(buf, sizeof(buf), "%.0f", cur);
    m_value.SetText(buf);
}

// ===========================================================================================
void CUIOutfitInfo::InitFromXml(CUIXml& xml_doc)
{
    const LPCSTR base_str = "outfit_info";

    CUIXmlInit::InitWindow(xml_doc, base_str, 0, this);

    string128 buf;

    strconcat(sizeof(buf), buf, base_str, ":caption");
    m_caption = UIHelper::CreateStatic(xml_doc, buf, this, false);

    strconcat(sizeof(buf), buf, base_str, ":", "prop_line");
    m_Prop_line = UIHelper::CreateStatic(xml_doc, buf, this, false);

    Fvector2 pos{};

    if (m_Prop_line)
        pos.set(0.0f, m_Prop_line->GetWndPos().y + m_Prop_line->GetWndSize().y);
    else if (m_caption)
        pos.set(0.0f, m_caption->GetWndSize().y);

    for (const auto [hit_type, immunity, immunity_text] : immunity_names)
    {
        auto item = xr_new<CUIOutfitImmunity>();
        if (!item->InitFromXml(xml_doc, base_str, immunity, immunity_text))
        {
            xr_delete(item);
            continue;
        }
        item->SetAutoDelete(true);
        AttachChild(item);
        item->SetWndPos(pos);
        pos.y += item->GetWndSize().y;
        m_items[hit_type] = item;
    }

    pos.x = GetWndSize().x;
    SetWndSize(pos);
}

void CUIOutfitInfo::UpdateInfo(CCustomOutfit* cur_outfit, CCustomOutfit* slot_outfit)
{
    const CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor || !cur_outfit)
    {
        return;
    }

    for (auto& [hit_type, item] : m_items)
    {
        if (!item)
            continue;

        if (hit_type == ALife::eHitTypeFireWound)
            continue;

        const float max_power = actor->conditions().GetZoneMaxPower(hit_type);

        float cur = cur_outfit->GetDefHitTypeProtection(hit_type);
        cur /= max_power; // = 0..1
        float slot = cur;

        if (slot_outfit)
        {
            slot = slot_outfit->GetDefHitTypeProtection(hit_type);
            slot /= max_power; //  = 0..1
        }
        item->SetProgressValue(cur, slot);
    }

    if (const auto& fireWoundItem = m_items[ALife::eHitTypeFireWound])
    {
        IKinematics* ikv = smart_cast<IKinematics*>(actor->Visual());
        VERIFY(ikv);
        u16 spine_bone = ikv->LL_BoneID("bip01_spine");

        float cur = cur_outfit->GetBoneArmor(spine_bone) * cur_outfit->GetCondition();
        // if(!cur_outfit->bIsHelmetAvaliable)
        //{
        //	spine_bone = ikv->LL_BoneID("bip01_head");
        //	cur += cur_outfit->GetBoneArmor(spine_bone);
        //}
        float slot = cur;
        if (slot_outfit)
        {
            spine_bone = ikv->LL_BoneID("bip01_spine");
            slot = slot_outfit->GetBoneArmor(spine_bone) * slot_outfit->GetCondition();
            // if(!slot_outfit->bIsHelmetAvaliable)
            //{
            //	spine_bone = ikv->LL_BoneID("bip01_head");
            //	slot += slot_outfit->GetBoneArmor(spine_bone);
            //}
        }
        const float max_power = actor->conditions().GetMaxFireWoundProtection();
        cur /= max_power;
        slot /= max_power;
        fireWoundItem->SetProgressValue(cur, slot);
    }
}

void CUIOutfitInfo::UpdateInfo(CHelmet* cur_helmet, CHelmet* slot_helmet)
{
    const CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor || !cur_helmet)
    {
        return;
    }

    for (auto& [hit_type, item] : m_items)
    {
        if (!item)
            continue;

        if (hit_type == ALife::eHitTypeFireWound)
            continue;

        const float max_power = actor->conditions().GetZoneMaxPower(hit_type);

        float cur = cur_helmet->GetDefHitTypeProtection(hit_type);
        cur /= max_power; // = 0..1
        float slot = cur;

        if (slot_helmet)
        {
            slot = slot_helmet->GetDefHitTypeProtection(hit_type);
            slot /= max_power; //  = 0..1
        }
        item->SetProgressValue(cur, slot);
    }

    if (const auto& fireWoundItem = m_items[ALife::eHitTypeFireWound])
    {
        IKinematics* ikv = smart_cast<IKinematics*>(actor->Visual());
        VERIFY(ikv);
        const u16 spine_bone = ikv->LL_BoneID("bip01_head");

        const float cur = cur_helmet->GetBoneArmor(spine_bone) * cur_helmet->GetCondition();
        const float slot = (slot_helmet) ? slot_helmet->GetBoneArmor(spine_bone) * slot_helmet->GetCondition() : cur;

        fireWoundItem->SetProgressValue(cur, slot);
    }
}
