#pragma once

#include "xrUICore/Static/UIStatic.h"

class CActor;
class CUIMiniMap;

class CUIZoneMap
{
public:
    bool visible{ true };

private:
    CUIMiniMap* m_activeMap{};

    CUIStatic m_background{ "Background" };
    CUIStatic m_center{ "Center" };
    CUIStatic m_compass{ "Compass" };
    CUIStatic m_clipFrame{ "Clip frame" };
    CUIStatic m_Counter{ "Counter" };
    CUIStatic m_Counter_text{ "Counter text" };
    CUIStatic* m_clock_wnd{};
    CUIStatic* m_pointerDistanceText{};

    u8 m_current_map_idx{ u8(-1) };

public:
    void Init(bool motionIconAttached);

    void Render();
    void Update();

    bool ZoomIn();
    bool ZoomOut();

    CUIStatic& Background() { return m_background; }
    CUIWindow& MapFrame() { return m_clipFrame; }
    void SetupCurrentMap();
    void OnSectorChanged(IRender_Sector::sector_id_t sector);
    void Counter_ResetClrAnimation();

private:
    void SetHeading(float angle);
    void UpdateRadar(Fvector pos);
};
