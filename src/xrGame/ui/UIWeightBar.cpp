#include "StdAfx.h"
#include "UIWeightBar.h"
#include "UIHelper.h"
#include "UIInventoryUtilities.h"

void CUIWeightBar::init_from_xml(CUIXml& uiXml, pcstr path)
{
    string64 buf;

    xr_sprintf(buf, "%s_weight_caption", path);
    m_BottomInfo = UIHelper::CreateStatic(uiXml, buf, this);

    xr_sprintf(buf, "%s_weight", path);
    m_Weight = UIHelper::CreateTextWnd(uiXml, buf, this);

    xr_sprintf(buf, "%s_weight_max", path);
    m_WeightMax = UIHelper::CreateTextWnd(uiXml, buf, this, false);
    {
        CUITextWnd* weightLabel = m_WeightMax ? m_WeightMax : m_Weight;
        m_Weight_end_x = weightLabel->GetWndPos().x;
    }
    m_BottomInfo->AdjustWidthToText();
}

void CUIWeightBar::UpdateData(float weight)
{
    if (!m_Weight || !m_BottomInfo)
        return;

    LPCSTR kg_str = StringTable().translate("st_kg").c_str();

    string64 buf;
    xr_sprintf(buf, "%.1f %s", weight, kg_str);
    m_Weight->SetText(buf);
    m_Weight->AdjustWidthToText();
    m_BottomInfo->AdjustWidthToText();

    Fvector2 pos = m_Weight->GetWndPos();
    pos.x = m_Weight_end_x - m_Weight->GetWndSize().x - 5.0f;
    m_Weight->SetWndPos(pos);
    pos.x = pos.x - m_BottomInfo->GetWndSize().x - 5.0f;
    m_BottomInfo->SetWndPos(pos);
}

void CUIWeightBar::UpdateData(CInventoryOwner* pInvOwner)
{
    if (!m_Weight || !m_WeightMax || !m_BottomInfo)
        return;
    InventoryUtilities::UpdateWeightStr(*m_Weight, *m_WeightMax, pInvOwner);

    m_Weight->AdjustWidthToText();
    m_WeightMax->AdjustWidthToText();
    m_BottomInfo->AdjustWidthToText();

    Fvector2 pos = m_Weight->GetWndPos();
    pos.x = m_WeightMax->GetWndPos().x - m_Weight->GetWndSize().x - 5.0f;
    m_Weight->SetWndPos(pos);
    pos.x = pos.x - m_BottomInfo->GetWndSize().x - 5.0f;
    m_BottomInfo->SetWndPos(pos);
}
