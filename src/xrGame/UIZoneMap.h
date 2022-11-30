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
    CUIWindow m_clipFrame{ "Clip frame" };
    CUIStatic m_Counter{ "Counter" };
    CUITextWnd m_Counter_text{};
    CUIStatic* m_clock_wnd{};
    CUIStatic* m_pointerDistanceText{};

    u8 m_current_map_idx{ u8(-1) };

public:
    virtual ~CUIZoneMap() = default;

    void Init();

    void Render();
    void Update();

    bool ZoomIn();
    bool ZoomOut();

    CUIStatic& Background() { return m_background; };
    CUIWindow& MapFrame() { return m_clipFrame; };
    void SetupCurrentMap();
    void OnSectorChanged(int sector);
    void Counter_ResetClrAnimation();

private:
    void SetHeading(float angle);
    void UpdateRadar(Fvector pos);
};
