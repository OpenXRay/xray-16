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
    void UpdateBoosterPosition(const CEntityCondition::BOOSTER_MAP& influences);

private:
    bool bHorizontal, bInverse;
    float dx, dy;
    u8 maxItem;
    xr_vector<EBoostParams> indBoostPos;
    xr_map<EBoostParams, CUIStatic*> indBoostState;
};
