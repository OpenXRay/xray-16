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

CUIOutfitImmunity::CUIOutfitImmunity(pcstr immunity_name)
    : CUIStatic(immunity_name), m_value("static_value")
{
    AttachChild(&m_progress);
    AttachChild(&m_value);
    m_magnitude = 1.0f;
}

bool CUIOutfitImmunity::InitFromXml(CUIXml& xml_doc, pcstr base_str, pcstr immunity, pcstr immunity_text)
{
    float default_magnitude = 1.0f;

    string256 buf;
    strconcat(buf, base_str, ":", immunity);
    if (!CUIXmlInit::InitStatic(xml_doc, buf, 0, this, false))
        return false;

    SetTextST(immunity_text);

    strconcat(buf, base_str, ":", immunity, ":progress_immunity");
    if (xml_doc.NavigateToNode(buf, 0))
    {
        m_progress.InitFromXml(xml_doc, buf);
        m_progress.Show(true);
    }
    else
    {
        m_progress.Show(false);
    }

    strconcat(buf, base_str, ":", immunity, ":static_value");
    if (xml_doc.NavigateToNode(buf, 0) && !CallOfPripyatMode)
    {
        CUIXmlInit::InitStatic(xml_doc, buf, 0, &m_value);
        m_value.Show(true);
    }
    else if (!m_progress.IsShown()) // SOC
    {
        strconcat(buf, base_str, ":", immunity);
        CUIXmlInit::InitText(xml_doc, buf, 0, m_value.TextItemControl());

        const auto font = GetFont();
        float text_len  = font->SizeOf_(GetText());
        float space_len = font->SizeOf_(" ");
        UI().ClientToScreenScaledWidth(text_len);
        UI().ClientToScreenScaledWidth(space_len);

        auto rect = GetWndRect();
        rect.x1 += TextItemControl()->m_TextOffset.x + text_len + space_len;
        m_value.SetWndRect(rect);

        m_value.Show(true);
        default_magnitude = 100.0f;
    }
    else
    {
        m_value.Show(false);
    }

    m_magnitude = xml_doc.ReadAttribFlt(buf, 0, "magnitude", default_magnitude);
    return true;
}

void CUIOutfitImmunity::SetProgressValue(float cur, float comp, float add /*= 0.0f*/)
{
    string128 buf;

    cur *= m_magnitude;
    comp *= m_magnitude;

    if (m_progress.IsShown())
    {
        if (fsimilar(cur, comp) && !fis_zero(add))
            comp = add;
        m_progress.SetTwoPos(cur, comp);

        const auto sz = xr_sprintf(buf, "%.0f", cur);
        if (!fis_zero(add))
            xr_sprintf(buf + sz, sizeof(buf) - sz, "+ %.0f%", add * m_magnitude);
    }
    else // SOC
    {
        const auto sz = xr_sprintf(buf, "%s %+3.0f%%", cur > 0.0f ? "%c[green]" : "%c[red]", cur);
        if (!fis_zero(add))
            xr_sprintf(buf + sz, sizeof(buf) - sz, "%s %+3.0f%%", add > 0.0f ? "%c[green]" : "%c[red]", add * m_magnitude);
    }

    m_value.SetText(buf);
}

// ===========================================================================================
void CUIOutfitInfo::InitFromXml(CUIXml& xml_doc)
{
    constexpr pcstr base_str = "outfit_info";

    CUIXmlInit::InitWindow(xml_doc, base_str, 0, this);

    string128 buf;

    strconcat(buf, base_str, ":caption");
    const auto caption = UIHelper::CreateStatic(xml_doc, buf, this, false);

    strconcat(buf, base_str, ":prop_line");
    const auto prop_line = UIHelper::CreateStatic(xml_doc, buf, this, false);

    if (prop_line)
        m_start_pos = { 0.0f, prop_line->GetWndPos().y + prop_line->GetWndSize().y };
    else if (caption)
        m_start_pos = { 0.0f, caption->GetWndSize().y };
    else // SOC
    {
        strconcat(buf, base_str, ":scroll_view");
        if (const auto node = xml_doc.NavigateToNode(buf))
        {
            const auto x = xml_doc.ReadAttribFlt(node, "x", 0.0f);
            const auto y = xml_doc.ReadAttribFlt(node, "y", 0.0f);
            m_start_pos = { x, y };
        }
    }

    for (const auto [hit_type, immunity, immunity_text] : immunity_names)
    {
        auto item = xr_new<CUIOutfitImmunity>(immunity);

        strconcat(buf, "static_", immunity); // SOC
        if (item->InitFromXml(xml_doc, base_str, immunity, immunity_text) ||
            item->InitFromXml(xml_doc, base_str, buf, immunity_text))
        {
            item->SetAutoDelete(true);
            AttachChild(item);
            m_items[hit_type] = item;
            continue;
        }
        xr_delete(item);
    }

    AdjustElements();
}

void CUIOutfitInfo::AdjustElements()
{
    auto pos = m_start_pos;

    for (const auto [hit_type, _, __] : immunity_names)
    {
        auto item = m_items[hit_type];
        if (!item || !item->IsShown())
            continue;
        item->SetWndPos(pos);
        pos.y += item->GetWndSize().y;
    }
    pos.x = GetWndSize().x;
    SetWndSize(pos);
}

void CUIOutfitInfo::UpdateInfo(CCustomOutfit* cur_outfit, CCustomOutfit* slot_outfit /*= nullptr*/,
    bool add_artefact_info /*= false*/, bool hide_if_zero_immunity /*= false*/)
{
    const CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!actor || !cur_outfit)
    {
        if (hide_if_zero_immunity)
        {
            for (auto& [hit_type, item] : m_items)
            {
                if (item)
                    item->Show(false);
            }
        }
        return;
    }

    const bool is_cs_cop = cur_outfit->GetHitFracType() != SBoneProtections::HitFraction;

    for (auto& [hit_type, item] : m_items)
    {
        if (!item)
            continue;

        if (hit_type == ALife::eHitTypeFireWound && is_cs_cop)
            continue;

        float cur  = cur_outfit->GetDefHitTypeProtection(hit_type);
        float slot = slot_outfit ? slot_outfit->GetDefHitTypeProtection(hit_type) : cur;
        float add  = add_artefact_info ? actor->GetProtection_ArtefactsOnBelt(hit_type) : 0.0f;

        if (is_cs_cop)
        {
            const float max_power = actor->conditions().GetZoneMaxPower(hit_type);
            cur /= max_power;  // = 0..1
            slot /= max_power; //  = 0..1
        }

        const bool hide = hide_if_zero_immunity && fis_zero(cur) && fis_zero(slot);
        item->Show(!hide);

        if (!hide)
            item->SetProgressValue(cur, slot, add);
    }

    if (const auto& fireWoundItem = m_items[ALife::eHitTypeFireWound];
        fireWoundItem && is_cs_cop)
    {
        IKinematics* ikv = smart_cast<IKinematics*>(actor->Visual());
        VERIFY(ikv);
        u16 spine_bone = ikv->LL_BoneID("bip01_spine");

        float cur = cur_outfit->GetBoneArmor(spine_bone) * cur_outfit->GetCondition();

        float slot = cur;
        if (slot_outfit)
        {
            spine_bone = ikv->LL_BoneID("bip01_spine");
            slot = slot_outfit->GetBoneArmor(spine_bone) * slot_outfit->GetCondition();

        }
        const float max_power = actor->conditions().GetMaxFireWoundProtection();
        cur /= max_power;
        slot /= max_power;

        const bool hide = hide_if_zero_immunity && fis_zero(cur) && fis_zero(slot);
        fireWoundItem->Show(!hide);

        if (!hide)
            fireWoundItem->SetProgressValue(cur, slot);
    }

    AdjustElements();
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
