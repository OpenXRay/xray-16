#pragma once

#include "xrUICore/Static/UIStatic.h"

class CActor;
class CUIMiniMap;

class CUIZoneMap : public CUIWindow
{
private:
    CUIMiniMap* m_activeMap{};

    CUIStatic* m_compass{};
    CUIStatic m_clipFrame{ "Clip frame" };
    CUIStatic m_Counter_text{ "Counter text" };
    CUIStatic* m_clock_wnd{};
    CUIStatic* m_pointerDistanceText{};

    u8 m_current_map_idx{ u8(-1) };

public:
    CUIZoneMap();

    void Init(bool motionIconAttached, CUIXml& maingameXml);

    void Update(const CActor* actor);

    bool ZoomIn();
    bool ZoomOut();

    CUIWindow& MapFrame() { return m_clipFrame; }
    void SetupCurrentMap();
    void OnSectorChanged(IRender_Sector::sector_id_t sector);
    void Counter_ResetClrAnimation();

    pcstr GetDebugType() override { return "CUIZoneMap"; }

    void FillDebugInfo() override;

private:
    void SetHeading(float angle) const;
    void UpdateRadar(Fvector pos);
};
