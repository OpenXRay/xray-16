#pragma once

class CUITradeBar final : public CUIStatic
{
    CUITextWnd* m_TradeCaption{};
    CUITextWnd* m_TradePrice{};
    CUITextWnd* m_TradeWeightMax{};

public:
    CUITradeBar() : CUIStatic("CUITradeBar") {}
    void init_from_xml(CUIXml& uiXml, pcstr path);
    void UpdateData(u32 price, float weight) const;
};
