#include "StdAfx.h"
#include "UIXmlInit.h"
#include "Level.h"
#include "string_table.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Buttons/UICheckButton.h"
#include "xrUICore/SpinBox/UICustomSpin.h"
#include "xrUICore/Buttons/UIRadioButton.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"
#include "xrUICore/TabControl/UITabControl.h"
#include "UILabel.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "UISleepStatic.h"
#include "UIXmlInit.h"
#include "xrUICore/ListBox/UIListBox.h"
#include "xrUICore/ComboBox/UIComboBox.h"
#include "xrUICore/TrackBar/UITrackBar.h"
#include "game_base_space.h"

#include "xrUICore/XML/UITextureMaster.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UITabButtonMP.h"
#include "xrUICore/Lines/UILines.h"

CUIXmlInit::CUIXmlInit() : CUIXmlInitBase() {}
CUIXmlInit::~CUIXmlInit() {}

bool CUIXmlInit::InitTabButtonMP(CUIXml& xml_doc, LPCSTR path, int index, CUITabButtonMP* pWnd)
{
    Init3tButton(xml_doc, path, index, pWnd);

    string256 buff;
    strconcat(sizeof(buff), buff, path, ":idention");

    if (xml_doc.NavigateToNode(buff, index))
    {
        pWnd->m_text_ident_cursor_over.x = xml_doc.ReadAttribFlt(buff, index, "over_x", 0);
        pWnd->m_text_ident_cursor_over.y = xml_doc.ReadAttribFlt(buff, index, "over_y", 0);

        pWnd->m_text_ident_normal.x = xml_doc.ReadAttribFlt(buff, index, "normal_x", 0);
        pWnd->m_text_ident_normal.y = xml_doc.ReadAttribFlt(buff, index, "normal_y", 0);
    }

    strconcat(sizeof(buff), buff, path, ":hint");

    if (xml_doc.NavigateToNode(buff, index))
    {
        pWnd->CreateHint();
        InitStatic(xml_doc, buff, index, pWnd->m_hint);
    }

    return true;
}

bool CUIXmlInit::InitDragDropListEx(CUIXml& xml_doc, LPCSTR path, int index, CUIDragDropListEx* pWnd)
{
    R_ASSERT4(xml_doc.NavigateToNode(path, index), "XML node not found", path, xml_doc.m_xml_file_name);

    Fvector2 pos, size;
    pos.x = xml_doc.ReadAttribFlt(path, index, "x");
    pos.y = xml_doc.ReadAttribFlt(path, index, "y");
    size.x = xml_doc.ReadAttribFlt(path, index, "width");
    size.y = xml_doc.ReadAttribFlt(path, index, "height");

    InitAlignment(xml_doc, path, index, pos.x, pos.y, pWnd);

    pWnd->InitDragDropList(pos, size);

    Ivector2 w_cell_sz, w_cells, w_cell_sp;

    w_cell_sz.x = xml_doc.ReadAttribInt(path, index, "cell_width");
    w_cell_sz.y = xml_doc.ReadAttribInt(path, index, "cell_height");
    w_cells.y = xml_doc.ReadAttribInt(path, index, "rows_num");
    w_cells.x = xml_doc.ReadAttribInt(path, index, "cols_num");

    w_cell_sp.x = xml_doc.ReadAttribInt(path, index, "cell_sp_x");
    w_cell_sp.y = xml_doc.ReadAttribInt(path, index, "cell_sp_y");

    pWnd->SetCellSize(w_cell_sz);
    pWnd->SetCellsSpacing(w_cell_sp);
    pWnd->SetStartCellsCapacity(w_cells);

    int tmp = xml_doc.ReadAttribInt(path, index, "unlimited", 0);
    pWnd->SetAutoGrow(tmp != 0);
    tmp = xml_doc.ReadAttribInt(path, index, "group_similar", 0);
    pWnd->SetGrouping(tmp != 0);
    tmp = xml_doc.ReadAttribInt(path, index, "custom_placement", 1);
    pWnd->SetCustomPlacement(tmp != 0);

    tmp = xml_doc.ReadAttribInt(path, index, "vertical_placement", 0);
    pWnd->SetVerticalPlacement(tmp != 0);

    tmp = xml_doc.ReadAttribInt(path, index, "always_show_scroll", 0);
    pWnd->SetAlwaysShowScroll(tmp != 0);

    tmp = xml_doc.ReadAttribInt(path, index, "condition_progress_bar", 0);
    pWnd->SetConditionProgBarVisibility(tmp != 0);

    tmp = xml_doc.ReadAttribInt(path, index, "virtual_cells", 0);
    pWnd->SetVirtualCells(tmp != 0);

    if (tmp != 0)
    {
        xr_string vc_vert_align = xml_doc.ReadAttrib(path, index, "vc_vert_align", "");
        pWnd->SetCellsVertAlignment(vc_vert_align);
        xr_string vc_horiz_align = xml_doc.ReadAttrib(path, index, "vc_horiz_align", "");
        pWnd->SetCellsHorizAlignment(vc_horiz_align);
    }

    pWnd->back_color = GetColor(xml_doc, path, index, 0xFFFFFFFF);

    return true;
}

bool CUIXmlInit::InitSleepStatic(CUIXml& xml_doc, LPCSTR path, int index, CUISleepStatic* pWnd)
{
    R_ASSERT4(xml_doc.NavigateToNode(path, index), "XML node not found", path, xml_doc.m_xml_file_name);

    InitStatic(xml_doc, path, index, pWnd);

    return true;
}

bool CUIXmlInit::InitHintWindow(CUIXml& xml_doc, LPCSTR path, int index, UIHintWindow* pWnd)
{
    VERIFY(pWnd);
    InitWindow(xml_doc, path, index, pWnd);
    LPCSTR hint_text = xml_doc.Read(path, index, "no hint");
    pWnd->set_hint_text_ST(hint_text);

    pWnd->set_hint_delay((u32)xml_doc.ReadAttribInt(path, index, "delay"));
    return true;
}
