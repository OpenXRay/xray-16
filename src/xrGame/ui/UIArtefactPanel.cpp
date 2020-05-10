#include "StdAfx.h"
#include "UIArtefactPanel.h"
#include "UIInventoryUtilities.h"
#include "UIXmlInit.h"

#include "../Artefact.h"

CUIArtefactPanel::CUIArtefactPanel()
{
    m_cell_size.set(50.0f, 50.0f);
    m_fScale = 0.5f;
}

CUIArtefactPanel::~CUIArtefactPanel()
{
}

void CUIArtefactPanel::InitFromXML(CUIXml& xml, pcstr path, int index)
{
    CUIXmlInit::InitWindow(xml, path, index, this);

    m_cell_size.x = xml.ReadAttribFlt(path, index, "cell_width", 50.0f);
    m_cell_size.y = xml.ReadAttribFlt(path, index, "cell_height", 50.0f);
    m_fScale = xml.ReadAttribFlt(path, index, "scale", 0.5f);
}

void CUIArtefactPanel::InitIcons(const xr_vector<const CArtefact*>& artefacts)
{
    m_StaticItem.SetShader(InventoryUtilities::GetEquipmentIconsShader());
    m_vRects.clear();
    
    for (const CArtefact* artefact : artefacts)
    {
        const shared_str& sectionName = artefact->cNameSect();
        Frect texture_rect;
        texture_rect.x1 = pSettings->read<float>(sectionName, "inv_grid_x") * INV_GRID_WIDTH;
        texture_rect.y1 = pSettings->read<float>(sectionName, "inv_grid_y") * INV_GRID_HEIGHT;
        texture_rect.x2 = pSettings->read<float>(sectionName, "inv_grid_width") * INV_GRID_WIDTH;
        texture_rect.y2 = pSettings->read<float>(sectionName, "inv_grid_height") * INV_GRID_HEIGHT;
        texture_rect.rb.add(texture_rect.lt);
        m_vRects.push_back(texture_rect);
    }
}

void CUIArtefactPanel::Draw()
{
    const float iIndent = 1.0f;
    float x = 0.0f;
    float y = 0.0f;

    Frect rect;
    GetAbsoluteRect(rect);
    x = rect.left;
    y = rect.top;    
    
    float _s = m_cell_size.x/m_cell_size.y;

    for (const Frect& r : m_vRects)
    {
        Fvector2 size;
        size.x = m_fScale*(r.bottom - r.top);
        size.y = _s*m_fScale*(r.right - r.left);

        m_StaticItem.SetTextureRect(r);
        m_StaticItem.SetSize(size);
        m_StaticItem.SetPos(x, y);
        x = x + iIndent + size.x;

        m_StaticItem.Render();
    }

    CUIWindow::Draw();
}
