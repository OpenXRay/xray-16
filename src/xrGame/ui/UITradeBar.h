#pragma once

class CUITradeBar final : public CUIStatic
{
    CUIStatic* m_TradeCaption{};
    CUIStatic* m_TradePrice{};
    CUIStatic* m_TradeWeightMax{};

public:
    CUITradeBar() : CUIStatic("Trade Bar") {}
    void init_from_xml(CUIXml& uiXml, pcstr path);
    void UpdateData(u32 price, float weight) const;
    pcstr GetDebugType() override { return "CUITradeBar"; }
};
