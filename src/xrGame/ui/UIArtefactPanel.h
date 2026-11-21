#pragma once

#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Static/UIStaticItem.h"

class CUIXml;
class CUIStaticItem;

class CUIArtefactPanel final : public CUIWindow
{
protected:
    bool m_bVert{ false };
    float m_fIndent{ 1.0f };
    float m_fScale{ 0.5f };
    Fvector2 m_cell_size{ INV_GRID_WIDTH, INV_GRID_HEIGHT };
    xr_vector<Frect> m_vRects;
    CUIStaticItem m_StaticItem;

public:
    CUIArtefactPanel() : CUIWindow(CUIArtefactPanel::GetDebugType()) {}

    void InitFromXML(CUIXml& xml, pcstr path, int index);
    void InitIcons(const buffer_vector<pcstr>& sections);
    void Clear();
    void Draw() override;

    pcstr GetDebugType() override { return "CUIArtefactPanel"; }
};
