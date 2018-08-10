#include "stdafx.h"
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
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "UISleepStatic.h"
#include "uixmlinit.h"
#include "xrUICore/ListBox/UIListBox.h"
#include "xrUICore/ComboBox/UIComboBox.h"
#include "xrUICore/TrackBar/UITrackBar.h"
#include "game_base_space.h"

#include "xrUICore/XML/UITextureMaster.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "xrUICore/Lines/UILines.h"

CUIXmlInit::CUIXmlInit() : baseClass() {}
CUIXmlInit::~CUIXmlInit() {}

Frect CUIXmlInit::GetFRect(CUIXml& xml_doc, LPCSTR path, int index)
{
    return baseClass::GetFRect(xml_doc, path, index);
}

bool CUIXmlInit::InitWindow(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pWnd)
{
    return baseClass::InitWindow(xml_doc, path, index, pWnd);
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInit::InitFrameWindow(CUIXml& xml_doc, LPCSTR path, int index, CUIFrameWindow* pWnd)
{
    return baseClass::InitFrameWindow(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitOptionsItem(CUIXml& xml_doc, LPCSTR path, int index, CUIOptionsItem* pWnd)
{
    return baseClass::InitOptionsItem(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitStatic(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd)
{
    return baseClass::InitStatic(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitTextWnd(CUIXml& xml_doc, LPCSTR path, int index, CUITextWnd* pWnd)
{
    return baseClass::InitTextWnd(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitCheck(CUIXml& xml_doc, LPCSTR path, int index, CUICheckButton* pWnd)
{
    return baseClass::InitCheck(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitSpin(CUIXml& xml_doc, LPCSTR path, int index, CUICustomSpin* pWnd)
{
    return baseClass::InitSpin(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitText(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd)
{
    return baseClass::InitText(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitText(CUIXml& xml_doc, LPCSTR path, int index, CUILines* pLines)
{
    return baseClass::InitText(xml_doc, path, index, pLines);
}

////////////////////////////////////////////////////////////////////////////////////////////

bool CUIXmlInit::Init3tButton(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd)
{
    return baseClass::Init3tButton(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitSound(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd)
{
    return baseClass::InitSound(xml_doc, path, index, pWnd);
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

bool CUIXmlInit::InitProgressBar(CUIXml& xml_doc, LPCSTR path, int index, CUIProgressBar* pWnd)
{
    return baseClass::InitProgressBar(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitProgressShape(CUIXml& xml_doc, LPCSTR path, int index, CUIProgressShape* pWnd)
{
    return baseClass::InitProgressShape(xml_doc, path, index, pWnd);
}

void CUIXmlInit::InitAutoStaticGroup(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pParentWnd)
{
    baseClass::InitAutoStaticGroup(xml_doc, path, index, pParentWnd);
}

void CUIXmlInit::InitAutoFrameLineGroup(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pParentWnd)
{
    baseClass::InitAutoFrameLineGroup(xml_doc, path, index, pParentWnd);
}

bool CUIXmlInit::InitFont(CUIXml& xml_doc, LPCSTR path, int index, u32& color, CGameFont*& pFnt)
{
    return baseClass::InitFont(xml_doc, path, index, color, pFnt);
}

bool CUIXmlInit::InitTabControl(CUIXml& xml_doc, LPCSTR path, int index, CUITabControl* pWnd)
{
    return baseClass::InitTabControl(xml_doc, path, index, pWnd);
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInit::InitFrameLine(CUIXml& xml_doc, LPCSTR path, int index, CUIFrameLineWnd* pWnd)
{
    return baseClass::InitFrameLine(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitCustomEdit(CUIXml& xml_doc, LPCSTR path, int index, CUICustomEdit* pWnd)
{
    return baseClass::InitCustomEdit(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitEditBox(CUIXml& xml_doc, LPCSTR path, int index, CUIEditBox* pWnd)
{
    return baseClass::InitEditBox(xml_doc, path, index, pWnd);
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInit::InitAnimatedStatic(CUIXml& xml_doc, LPCSTR path, int index, CUIAnimatedStatic* pWnd)
{
    return baseClass::InitAnimatedStatic(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitSleepStatic(CUIXml& xml_doc, LPCSTR path, int index, CUISleepStatic* pWnd)
{
    R_ASSERT4(xml_doc.NavigateToNode(path, index), "XML node not found", path, xml_doc.m_xml_file_name);

    InitStatic(xml_doc, path, index, pWnd);

    return true;
}

bool CUIXmlInit::InitTexture(CUIXml& xml_doc, LPCSTR path, int index, ITextureOwner* pWnd)
{
    return baseClass::InitTexture(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitTextureOffset(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd)
{
    return baseClass::InitTextureOffset(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitMultiTexture(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd)
{
    return baseClass::InitMultiTexture(xml_doc, path, index, pWnd);
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInit::InitAlignment(CUIXml& xml_doc, const char* path, int index, float& x, float& y, CUIWindow* pWnd)
{
    return baseClass::InitAlignment(xml_doc, path, index, x, y, pWnd);
}

//////////////////////////////////////////////////////////////////////////

void CUIXmlInit::InitColorDefs() { baseClass::InitColorDefs(); }

bool CUIXmlInit::InitScrollView(CUIXml& xml_doc, LPCSTR path, int index, CUIScrollView* pWnd)
{
    return baseClass::InitScrollView(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitListBox(CUIXml& xml_doc, LPCSTR path, int index, CUIListBox* pWnd)
{
    return baseClass::InitListBox(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitTrackBar(CUIXml& xml_doc, LPCSTR path, int index, CUITrackBar* pWnd)
{
    return baseClass::InitTrackBar(xml_doc, path, index, pWnd);
}

bool CUIXmlInit::InitComboBox(CUIXml& xml_doc, LPCSTR path, int index, CUIComboBox* pWnd)
{
    return baseClass::InitComboBox(xml_doc, path, index, pWnd);
}

void CUIXmlInit::AssignColor(LPCSTR name, u32 clr) { baseClass::AssignColor(name, clr); }
u32 CUIXmlInit::GetColor(CUIXml& xml_doc, LPCSTR path, int index, u32 def_clr)
{
    return baseClass::GetColor(xml_doc, path, index, def_clr);
}
