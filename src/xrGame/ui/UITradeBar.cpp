#include "StdAfx.h"
#include "UITradeBar.h"
#include "UIHelper.h"

void CUITradeBar::init_from_xml(CUIXml& uiXml, pcstr path)
{
    const XML_NODE stored_root = uiXml.GetLocalRoot();

    CUIXmlInit::InitStatic(uiXml, path, 0, this);

    uiXml.SetLocalRoot(uiXml.NavigateToNode(path, 0));

    if (!CallOfPripyatMode)
    {
        CUITextWnd* m_TradeCaption = UIHelper::CreateTextWnd(uiXml, "trade_caption", this, false);
        if (m_TradeCaption)
            m_TradeCaption->AdjustWidthToText();
    }
    m_TradePrice = UIHelper::CreateTextWnd(uiXml, "trade_price", this);
    m_TradeWeightMax = UIHelper::CreateTextWnd(uiXml, "trade_weight_max", this);

    uiXml.SetLocalRoot(stored_root);
}

void CUITradeBar::UpdateData(u32 price, float weight) const
{
    string64 buf;

    if (m_TradePrice)
    {
        xr_sprintf(buf, "%d RU", price);
        m_TradePrice->SetText(buf);
        m_TradePrice->AdjustWidthToText();
    }

    if (m_TradeWeightMax)
    {
        pcstr kg_str = StringTable().translate("st_kg").c_str();
        xr_sprintf(buf, "(%.1f %s)", weight, kg_str);
        m_TradeWeightMax->SetText(buf);
    }

    if (m_TradePrice && m_TradeWeightMax)
    {
        Fvector2 pos = m_TradePrice->GetWndPos();
        pos.x = m_TradeWeightMax->GetWndPos().x - m_TradePrice->GetWndSize().x - 5.0f;
        m_TradePrice->SetWndPos(pos);
        if (m_TradeCaption)
        {
            pos.x = pos.x - m_TradeCaption->GetWndSize().x - 5.0f;
            m_TradeCaption->SetWndPos(pos);
        }
    }
}
