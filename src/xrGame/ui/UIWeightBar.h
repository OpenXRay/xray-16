#pragma once

class CUIWeightBar final : public CUIWindow
{
    CUIStatic* m_BottomInfo{};
    CUIStatic* m_Weight{};
    CUIStatic* m_WeightMax{};
    float m_Weight_end_x{};

public:
    CUIStatic* m_BagWnd{};
    CUIStatic* m_BagWnd2{};

public:
    CUIWeightBar() : CUIWindow("CUIWeightBar") {}
    void init_from_xml(CUIXml& uiXml, pcstr path);
    void UpdateData(float weight);
    void UpdateData(CInventoryOwner* pInvOwner);
    pcstr GetDebugType() override { return "CUIWeightBar"; }
};
