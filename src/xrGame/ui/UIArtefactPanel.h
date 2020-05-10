#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Static/UIStaticItem.h"

class CUIXml;
class CArtefact;
class CUIStaticItem;

class CUIArtefactPanel : public CUIWindow
{
protected:
    float m_fScale;
    Fvector2 m_cell_size;
    xr_vector<Frect> m_vRects;
    CUIStaticItem m_StaticItem;
public:
    CUIArtefactPanel();
    ~CUIArtefactPanel();

    void InitIcons(const xr_vector<const CArtefact*>& artefacts);
    void InitFromXML(CUIXml& xml, pcstr path, int index);
    virtual void Draw() override;
};
