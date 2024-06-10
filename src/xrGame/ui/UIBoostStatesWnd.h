#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "EntityCondition.h"


class CUIStatic;
class CUIXml;

class CUIBoostStatesWnd final : public CUIWindow
{
public:
    CUIBoostStatesWnd();
    void InitFromXml(CUIXml& xml, LPCSTR path);
    void DrawBoosterIndicators();
    void UpdateBoosterIndicators(const CEntityCondition::BOOSTER_MAP& influences);

private:
    bool bHorizontal, bInverse;
    float dx, dy;
    u8 max_item;
    xr_vector<EBoostParams> m_ind_boost_pos;
    xr_map<EBoostParams, CUIStatic*> m_ind_boost_state;
};
