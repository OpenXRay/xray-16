#include "StdAfx.h"

#include "UIArtefactPanel.h"
#include "UIInventoryUtilities.h"
#include "UIXmlInit.h"

void CUIArtefactPanel::InitFromXML(CUIXml& xml, pcstr path, int index)
{
    CUIXmlInit::InitWindow(xml, path, index, this);

    m_cell_size.x = xml.ReadAttribFlt(path, index, "cell_width", INV_GRID_WIDTH);
    m_cell_size.y = xml.ReadAttribFlt(path, index, "cell_height", INV_GRID_HEIGHT);
    m_fScale      = xml.ReadAttribFlt(path, index, "scale", 0.5f);
    m_bVert       = xml.ReadAttribInt(path, index, "vert") != 0;
    m_fIndent     = xml.ReadAttribFlt(path, index, "indent", 1.0f);
}

void CUIArtefactPanel::InitIcons(const buffer_vector<pcstr>& sections)
{
    m_StaticItem.SetShader(InventoryUtilities::GetEquipmentIconsShader());
    m_vRects.clear();
    m_vRects.reserve(sections.size());

    for (cpcstr section_name : sections)
    {
        Frect texture_rect
        {
            .x1 = pSettings->r_float(section_name, "inv_grid_x") * INV_GRID_WIDTH,
            .y1 = pSettings->r_float(section_name, "inv_grid_y") * INV_GRID_HEIGHT,
            .x2 = pSettings->r_float(section_name, "inv_grid_width") * INV_GRID_WIDTH,
            .y2 = pSettings->r_float(section_name, "inv_grid_height") * INV_GRID_HEIGHT,
        };
        texture_rect.rb.add(texture_rect.lt);
        m_vRects.emplace_back(texture_rect);
    }
}

void CUIArtefactPanel::Clear()
{
    m_StaticItem.SetShader({});
    m_vRects.clear();
    m_vRects.shrink_to_fit();
}

void CUIArtefactPanel::Draw()
{
    Frect rect;
    GetAbsoluteRect(rect);
    float x = rect.left;
    float y = rect.top;

    const float _s = m_cell_size.x / m_cell_size.y;

    for (const Frect& r : m_vRects)
    {
        Fvector2 size;
        size.x = m_fScale * (r.bottom - r.top);
        size.y = _s * m_fScale *(r.right - r.left);

        m_StaticItem.SetTextureRect(r);
        m_StaticItem.SetSize(size);
        m_StaticItem.SetPos(x, y);
        if (!m_bVert)
            x = x + m_fIndent + size.x;
        else
            y = y + m_fIndent + size.y;

        m_StaticItem.Render();
    }

    CUIWindow::Draw();
}
