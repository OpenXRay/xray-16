#include "pch.hpp"
#include "UIXmlInitBase.h"
#include "xrEngine/StringTable/IStringTable.h"
#include "Windows/UIFrameWindow.h"
#include "Windows/UITextFrameLineWnd.h"
#include "Buttons/UICheckButton.h"
#include "SpinBox/UICustomSpin.h"
#include "Buttons/UIRadioButton.h"
#include "ProgressBar/UIProgressBar.h"
#include "ProgressBar/UIProgressShape.h"
#include "TabControl/UITabControl.h"
#include "Static/UIAnimatedStatic.h"
#include "ListWnd/UIListWnd.h"
#include "ListBox/UIListBox.h"
#include "ComboBox/UIComboBox.h"
#include "TrackBar/UITrackBar.h"

#include "UITextureMaster.h"
#include "Lines/UILines.h"
#include "xrEngine/xr_input_xinput.h"

#define ARIAL_FONT_NAME "arial"

#define MEDIUM_FONT_NAME "medium"
#define SMALL_FONT_NAME "small"

#define GRAFFITI19_FONT_NAME "graffiti19"
#define GRAFFITI22_FONT_NAME "graffiti22"
#define GRAFFITI32_FONT_NAME "graffiti32"
#define GRAFFITI50_FONT_NAME "graffiti50"

#define LETTERICA16_FONT_NAME "letterica16"
#define LETTERICA18_FONT_NAME "letterica18"
#define LETTERICA25_FONT_NAME "letterica25"

#define DI_FONT_NAME "di"

//////////////////////////////////////////////////////////////////////////

constexpr pcstr COLOR_DEFINITIONS = "color_defs.xml";
CUIXmlInitBase::ColorDefs* CUIXmlInitBase::m_pColorDefs = nullptr;

//////////////////////////////////////////////////////////////////////////

CUIXmlInitBase::CUIXmlInitBase() { InitColorDefs(); }
//////////////////////////////////////////////////////////////////////////

CUIXmlInitBase::~CUIXmlInitBase() {}
//////////////////////////////////////////////////////////////////////////

Frect CUIXmlInitBase::GetFRect(CUIXml& xml_doc, LPCSTR path, int index)
{
    R_ASSERT4(xml_doc.NavigateToNode(path, index), "XML node not found", path, xml_doc.m_xml_file_name);
    Frect rect;
    rect.set(0, 0, 0, 0);
    rect.x1 = xml_doc.ReadAttribFlt(path, index, "x");
    rect.y1 = xml_doc.ReadAttribFlt(path, index, "y");
    rect.x2 = rect.x1 + xml_doc.ReadAttribFlt(path, index, "width");
    rect.y2 = rect.y1 + xml_doc.ReadAttribFlt(path, index, "height");

    return rect;
}

bool CUIXmlInitBase::InitWindow(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pWnd, bool fatal /*= true*/)
{
    const bool nodeExist = xml_doc.NavigateToNode(path, index);
    if (!nodeExist)
    {
        R_ASSERT4(!fatal, "XML node not found", path, xml_doc.m_xml_file_name);
        return false;
    }

    Fvector2 pos, size;
    pos.x = xml_doc.ReadAttribFlt(path, index, "x");
    pos.y = xml_doc.ReadAttribFlt(path, index, "y");
    InitAlignment(xml_doc, path, index, pos.x, pos.y, pWnd);
    size.x = xml_doc.ReadAttribFlt(path, index, "width");
    size.y = xml_doc.ReadAttribFlt(path, index, "height");
    pWnd->SetWndPos(pos);
    pWnd->SetWndSize(size);

    string512 buf;

    strconcat(sizeof(buf), buf, path, ":window_name");
    if (xml_doc.NavigateToNode(buf, index))
        pWnd->SetWindowName(xml_doc.Read(buf, index, NULL));

    InitAutoStaticGroup(xml_doc, path, index, pWnd);
    //.	InitAutoFrameLineGroup		(xml_doc, path, index, pWnd);

    return true;
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInitBase::InitFrameWindow(CUIXml& xml_doc, LPCSTR path, int index, CUIFrameWindow* pWnd, bool fatal /*= true*/)
{
    bool result = InitWindow(xml_doc, path, index, pWnd, fatal);
    result &= InitTexture(xml_doc, path, index, pWnd, fatal);

    return result;
}

bool CUIXmlInitBase::InitOptionsItem(CUIXml& xml_doc, LPCSTR path, int index, CUIOptionsItem* pWnd)
{
    string256 buf;
    strconcat(sizeof(buf), buf, path, ":options_item");

    if (xml_doc.NavigateToNode(buf, index))
    {
        shared_str entry = xml_doc.ReadAttrib(buf, index, "entry");
        shared_str group = xml_doc.ReadAttrib(buf, index, "group");
        pWnd->AssignProps(entry, group);

        LPCSTR depends = xml_doc.ReadAttrib(buf, index, "depend", NULL);
        if (depends)
        {
            CUIOptionsItem::ESystemDepends d = CUIOptionsItem::sdNothing;

            if (0 == xr_stricmp(depends, "vid"))
                d = CUIOptionsItem::sdVidRestart;
            else if (0 == xr_stricmp(depends, "snd"))
                d = CUIOptionsItem::sdSndRestart;
            else if (0 == xr_stricmp(depends, "ui"))
                d = CUIOptionsItem::sdUIRestart;
            else if (0 == xr_stricmp(depends, "restart"))
                d = CUIOptionsItem::sdSystemRestart;
            else if (0 == xr_stricmp(depends, "runtime"))
                d = CUIOptionsItem::sdApplyOnChange;
            else
                Msg("! unknown param [%s] in optionsItem [%s]", depends, entry.c_str());

            pWnd->SetSystemDepends(d);
        }
        return true;
    }

    return false;
}

bool CUIXmlInitBase::InitStatic(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd, bool fatal /*= true*/)
{
    if (!InitWindow(xml_doc, path, index, pWnd, fatal))
        return false;

    string256 buf;
    InitText(xml_doc, strconcat(sizeof(buf), buf, path, ":text"), index, pWnd);
    InitTexture(xml_doc, path, index, pWnd);
    InitTextureOffset(xml_doc, path, index, pWnd);

    int flag = xml_doc.ReadAttribInt(path, index, "heading", 0);
    pWnd->EnableHeading((flag) ? true : false);

    float heading_angle = xml_doc.ReadAttribFlt(path, index, "heading_angle", 0.0f);
    if (!fis_zero(heading_angle))
    {
        pWnd->EnableHeading(true);
        pWnd->SetConstHeading(true);
        pWnd->SetHeading(deg2rad(heading_angle));
    }

    LPCSTR str_flag = xml_doc.ReadAttrib(path, index, "light_anim", "");
    int flag_cyclic = xml_doc.ReadAttribInt(path, index, "la_cyclic", 1);
    int flag_text = xml_doc.ReadAttribInt(path, index, "la_text", 1);
    int flag_texture = xml_doc.ReadAttribInt(path, index, "la_texture", 1);
    int flag_alpha = xml_doc.ReadAttribInt(path, index, "la_alpha", 0);

    u8 flags = 0;
    if (flag_cyclic)
        flags |= LA_CYCLIC;
    if (flag_alpha)
        flags |= LA_ONLYALPHA;
    if (flag_text)
        flags |= LA_TEXTCOLOR;
    if (flag_texture)
        flags |= LA_TEXTURECOLOR;

    pWnd->SetColorAnimation(str_flag, flags);

    str_flag = xml_doc.ReadAttrib(path, index, "xform_anim", "");
    flag_cyclic = xml_doc.ReadAttribInt(path, index, "xform_anim_cyclic", 1);

    pWnd->SetXformLightAnim(str_flag, (flag_cyclic) ? true : false);

    bool bComplexMode = xml_doc.ReadAttribInt(path, index, "complex_mode", 0) ? true : false;
    if (bComplexMode)
        pWnd->TextItemControl()->SetTextComplexMode(bComplexMode);

    pWnd->m_stat_hint_text = xml_doc.ReadAttrib(path, index, "hint", "");

    return true;
}

bool CUIXmlInitBase::InitTextWnd(CUIXml& xml_doc, LPCSTR path, int index, CUITextWnd* pWnd, bool fatal /*= true*/)
{
    if (!InitWindow(xml_doc, path, index, pWnd, fatal))
        return false;

    string256 buf;
    InitText(xml_doc, strconcat(sizeof(buf), buf, path, ":text"), index, &pWnd->TextItemControl());

    LPCSTR str_flag = xml_doc.ReadAttrib(path, index, "light_anim", "");
    int flag_cyclic = xml_doc.ReadAttribInt(path, index, "la_cyclic", 1);
    int flag_alpha = xml_doc.ReadAttribInt(path, index, "la_alpha", 0);

    u8 flags = LA_TEXTCOLOR;
    if (flag_cyclic)
        flags |= LA_CYCLIC;
    if (flag_alpha)
        flags |= LA_ONLYALPHA;
    pWnd->SetColorAnimation(str_flag, flags);

    bool bComplexMode = xml_doc.ReadAttribInt(path, index, "complex_mode", 0) ? true : false;
    if (bComplexMode)
        pWnd->SetTextComplexMode(bComplexMode);

    strconcat(sizeof(buf), buf, path, ":texture");
    R_ASSERT3(NULL == xml_doc.NavigateToNode(buf, index), xml_doc.m_xml_file_name, buf);

    R_ASSERT(pWnd->GetChildWndList().size() == 0);
    return true;

}

bool CUIXmlInitBase::InitCheck(CUIXml& xml_doc, LPCSTR path, int index, CUICheckButton* pWnd, bool fatal /*= true*/)
{
    if (!InitStatic(xml_doc, path, index, pWnd, fatal))
        return false;

    string256 buf;
    strconcat(sizeof(buf), buf, path, ":texture");
    LPCSTR texture = xml_doc.Read(buf, index, "ui_checker");

    pWnd->InitCheckButton(pWnd->GetWndPos(), pWnd->GetWndSize(), texture);

    u32 color;
    strconcat(sizeof(buf), buf, path, ":text_color:e");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Enabled);
    }

    strconcat(sizeof(buf), buf, path, ":text_color:d");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Disabled);
    }

    strconcat(sizeof(buf), buf, path, ":text_color:t");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Touched);
    }

    strconcat(sizeof(buf), buf, path, ":text_color:h");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Highlighted);
    }

    InitOptionsItem(xml_doc, path, index, pWnd);

    return true;
}

bool CUIXmlInitBase::InitSpin(CUIXml& xml_doc, LPCSTR path, int index, CUICustomSpin* pWnd, bool fatal /*= true*/)
{
    if (!InitWindow(xml_doc, path, index, pWnd, fatal))
        return false;

    InitOptionsItem(xml_doc, path, index, pWnd);
    pWnd->InitSpin(pWnd->GetWndPos(), pWnd->GetWndSize());

    string256 foo;
    u32 color;
    strconcat(sizeof(foo), foo, path, ":text_color:e");
    if (xml_doc.NavigateToNode(foo, index))
    {
        color = GetColor(xml_doc, foo, index, 0x00);
        pWnd->SetTextColor(color);
    }
    strconcat(sizeof(foo), foo, path, ":text_color:d");
    if (xml_doc.NavigateToNode(foo, index))
    {
        color = GetColor(xml_doc, foo, index, 0x00);
        pWnd->SetTextColorD(color);
    }

    return true;
}

bool CUIXmlInitBase::InitText(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd)
{
    if (!xml_doc.NavigateToNode(path, index))
        return false;

    return InitText(xml_doc, path, index, pWnd->TextItemControl());
}

bool CUIXmlInitBase::InitText(CUIXml& xml_doc, LPCSTR path, int index, CUILines* pLines)
{
    if (!xml_doc.NavigateToNode(path, index))
        return false;

    u32 color;
    CGameFont* pTmpFont = NULL;
    InitFont(xml_doc, path, index, color, pTmpFont);
    pLines->SetTextColor(color);
    R_ASSERT(pTmpFont);
    pLines->SetFont(pTmpFont);

    // Load font alignment
    shared_str al = xml_doc.ReadAttrib(path, index, "align");
    if (0 == xr_strcmp(al, "c"))
        pLines->SetTextAlignment(CGameFont::alCenter);
    else if (0 == xr_strcmp(al, "r"))
        pLines->SetTextAlignment(CGameFont::alRight);
    else if (0 == xr_strcmp(al, "l"))
        pLines->SetTextAlignment(CGameFont::alLeft);

    al = xml_doc.ReadAttrib(path, index, "vert_align", "");

    if (0 == xr_strcmp(al, "c"))
        pLines->SetVTextAlignment(valCenter);
    else if (0 == xr_strcmp(al, "b"))
        pLines->SetVTextAlignment(valBotton);
    else if (0 == xr_strcmp(al, "t"))
        pLines->SetVTextAlignment(valTop);

    pLines->SetTextComplexMode(xml_doc.ReadAttribInt(path, index, "complex_mode", 0) ? true : false);

    // Text coordinates
    float text_x = xml_doc.ReadAttribFlt(path, index, "x", 0);
    float text_y = xml_doc.ReadAttribFlt(path, index, "y", 0);

    pLines->m_TextOffset.set(text_x, text_y);

    shared_str text = xml_doc.Read(path, index, NULL);
    if (text.size())
        pLines->SetText(gStringTable->translate(text).c_str());

    return true;
}

bool CUIXmlInitBase::Init3tButton(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd, bool fatal /*= true*/)
{
    const bool nodeExist = xml_doc.NavigateToNode(path, index);
    if (!nodeExist)
    {
        R_ASSERT4(!fatal, "XML node not found", path, xml_doc.m_xml_file_name);
        return false;
    }

    pWnd->m_frameline_mode = (xml_doc.ReadAttribInt(path, index, "frame_mode", 0) == 1) ? true : false;

    pWnd->vertical = (xml_doc.ReadAttribInt(path, index, "vertical", 0) == 1) ? true : false;

    InitWindow(xml_doc, path, index, pWnd);
    pWnd->InitButton(pWnd->GetWndPos(), pWnd->GetWndSize());

    string256 buf;
    InitText(xml_doc, strconcat(sizeof(buf), buf, path, ":text"), index, pWnd);
    u32 color;

    strconcat(sizeof(buf), buf, path, ":text_color:e");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Enabled);
    }

    strconcat(sizeof(buf), buf, path, ":text_color:d");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Disabled);
    }

    strconcat(sizeof(buf), buf, path, ":text_color:t");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Touched);
    }

    strconcat(sizeof(buf), buf, path, ":text_color:h");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Highlighted);
    }

    InitMultiTexture(xml_doc, path, index, pWnd);
    InitTextureOffset(xml_doc, path, index, pWnd);
    InitSound(xml_doc, path, index, pWnd);

    LPCSTR accel = xml_doc.ReadAttrib(path, index, "accel", NULL);
    if (accel)
    {
        int acc = KeynameToDik(accel);
        pWnd->SetAccelerator(acc, 0);
    }
    accel = xml_doc.ReadAttrib(path, index, "accel_ext", NULL);
    if (accel)
    {
        int acc = KeynameToDik(accel);
        pWnd->SetAccelerator(acc, 1);
    }

    LPCSTR text_hint = xml_doc.ReadAttrib(path, index, "hint", NULL);
    if (text_hint)
        pWnd->m_hint_text = gStringTable->translate(text_hint);

    return true;
}

bool CUIXmlInitBase::InitSound(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd)
{
    string256 sound_h;
    string256 sound_t;
    strconcat(sizeof(sound_h), sound_h, path, ":sound_h");
    strconcat(sizeof(sound_t), sound_t, path, ":sound_t");

    shared_str sound_h_result = xml_doc.Read(sound_h, index, "");
    shared_str sound_t_result = xml_doc.Read(sound_t, index, "");

    if (xr_strlen(sound_h_result) != 0)
        pWnd->InitSoundH(*sound_h_result);

    if (xr_strlen(sound_t_result) != 0)
        pWnd->InitSoundT(*sound_t_result);

    return true;
}

bool CUIXmlInitBase::InitProgressBar(CUIXml& xml_doc, LPCSTR path, int index, CUIProgressBar* pWnd, bool fatal /*= true*/)
{
    const bool nodeExist = xml_doc.NavigateToNode(path, index);
    if (!nodeExist)
    {
        R_ASSERT4(!fatal, "XML node not found", path, xml_doc.m_xml_file_name);
        return false;
    }

    InitAutoStaticGroup(xml_doc, path, index, pWnd);

    string256 buf;
    Fvector2 pos, size;
    pos.x = xml_doc.ReadAttribFlt(path, index, "x");
    pos.y = xml_doc.ReadAttribFlt(path, index, "y");

    InitAlignment(xml_doc, path, index, pos.x, pos.y, pWnd);

    size.x = xml_doc.ReadAttribFlt(path, index, "width");
    size.y = xml_doc.ReadAttribFlt(path, index, "height");

    CUIProgressBar::EOrientMode mode = CUIProgressBar::om_vert;
    int mode_horz = xml_doc.ReadAttribInt(path, index, "horz", 0);
    LPCSTR mode_str = xml_doc.ReadAttrib(path, index, "mode");
    if (mode_horz == 1) // om_horz
    {
        mode = CUIProgressBar::om_horz;
    }
    else if (xr_stricmp(mode_str, "horz") == 0)
    {
        mode = CUIProgressBar::om_horz;
    }
    else if (xr_stricmp(mode_str, "vert") == 0)
    {
        mode = CUIProgressBar::om_vert;
    }
    else if (xr_stricmp(mode_str, "back") == 0)
    {
        mode = CUIProgressBar::om_back;
    }
    else if (xr_stricmp(mode_str, "down") == 0)
    {
        mode = CUIProgressBar::om_down;
    }
    else if (xr_stricmp(mode_str, "from_center") == 0)
    {
        mode = CUIProgressBar::om_fromcenter;
    }
    else if (xr_stricmp(mode_str, "vert_from_center") == 0)
    {
        mode = CUIProgressBar::om_vfromcenter;
    }

    pWnd->InitProgressBar(pos, size, mode);

    float min = xml_doc.ReadAttribFlt(path, index, "min");
    float max = xml_doc.ReadAttribFlt(path, index, "max");
    float ppos = xml_doc.ReadAttribFlt(path, index, "pos");

    pWnd->SetRange(min, max);
    pWnd->SetProgressPos(ppos);
    pWnd->m_inertion = xml_doc.ReadAttribFlt(path, index, "inertion", 0.0f);

    // progress
    strconcat(sizeof(buf), buf, path, ":progress");

    if (!xml_doc.NavigateToNode(buf, index))
        return false;

    InitStatic(xml_doc, buf, index, &pWnd->m_UIProgressItem);

    pWnd->m_UIProgressItem.SetWndSize(pWnd->GetWndSize());

    // background
    strconcat(sizeof(buf), buf, path, ":background");

    if (xml_doc.NavigateToNode(buf, index))
    {
        InitStatic(xml_doc, buf, index, &pWnd->m_UIBackgroundItem);
        pWnd->m_bBackgroundPresent = true;
        pWnd->m_UIBackgroundItem.SetWndSize(pWnd->GetWndSize());
    }

    strconcat(sizeof(buf), buf, path, ":min_color");

    if (xml_doc.NavigateToNode(buf, index))
    {
        pWnd->m_bUseColor = true;

        pWnd->m_minColor = GetColor(xml_doc, buf, index, 0xff);

        strconcat(sizeof(buf), buf, path, ":middle_color");
        pWnd->m_middleColor = GetColor(xml_doc, buf, index, 0xff);

        strconcat(sizeof(buf), buf, path, ":max_color");
        pWnd->m_maxColor = GetColor(xml_doc, buf, index, 0xff);
    }

    return true;
}

bool CUIXmlInitBase::InitProgressShape(CUIXml& xml_doc, LPCSTR path, int index, CUIProgressShape* pWnd, bool fatal /*= true*/)
{
    if (!InitStatic(xml_doc, path, index, pWnd, fatal))
        return false;

    if (xml_doc.ReadAttribInt(path, index, "text"))
        pWnd->SetTextVisible(true);

    string256 _path;

    strconcat(sizeof(_path), _path, path, ":back");
    if (xml_doc.NavigateToNode(_path, index))
    {
        pWnd->m_pBackground = xr_new<CUIStatic>();
        pWnd->m_pBackground->SetAutoDelete(true);
        pWnd->AttachChild(pWnd->m_pBackground);
        InitStatic(xml_doc, _path, index, pWnd->m_pBackground);
    }

    strconcat(sizeof(_path), _path, path, ":front");
    if (xml_doc.NavigateToNode(_path, index))
    {
        pWnd->m_pTexture = xr_new<CUIStatic>();
        pWnd->m_pTexture->SetAutoDelete(true);
        pWnd->AttachChild(pWnd->m_pTexture);
        InitStatic(xml_doc, _path, index, pWnd->m_pTexture);
    }

    pWnd->m_sectorCount = xml_doc.ReadAttribInt(path, index, "sector_count", 8);
    pWnd->m_bClockwise = xml_doc.ReadAttribInt(path, index, "clockwise") ? true : false;

    pWnd->m_blend = (xml_doc.ReadAttribInt(path, index, "blend", 1) == 1) ? true : false;
    pWnd->m_angle_begin = xml_doc.ReadAttribFlt(path, index, "begin_angle", 0.0f);
    pWnd->m_angle_end = xml_doc.ReadAttribFlt(path, index, "end_angle", PI_MUL_2);

    return true;
}

void CUIXmlInitBase::InitAutoStaticGroup(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pParentWnd)
{
    XML_NODE _stored_root = xml_doc.GetLocalRoot();
    xml_doc.SetLocalRoot(xml_doc.NavigateToNode(path, index));

    XML_NODE curr_root = xml_doc.GetLocalRoot();
    if (!curr_root)
        curr_root = xml_doc.GetRoot();

    XML_NODE node = curr_root->FirstChild();
    int cnt_static = 0;
    int cnt_frameline = 0;
    int cnt_text = 0;
    string512 buff;

    while (node)
    {
        LPCSTR node_name = node->Value();
        if (0 == xr_stricmp(node_name, "auto_static"))
        {
            CUIStatic* pUIStatic = xr_new<CUIStatic>();
            InitStatic(xml_doc, "auto_static", cnt_static, pUIStatic);
            xr_sprintf(buff, "auto_static_%d", cnt_static);
            pUIStatic->SetWindowName(buff);
            pUIStatic->SetAutoDelete(true);
            pParentWnd->AttachChild(pUIStatic);

            ++cnt_static;
        }
        else if (0 == xr_stricmp(node_name, "auto_frameline"))
        {
            CUIFrameLineWnd* pUIFrameline = xr_new<CUIFrameLineWnd>();
            InitFrameLine(xml_doc, "auto_frameline", cnt_frameline, pUIFrameline);
            xr_sprintf(buff, "auto_frameline_%d", cnt_frameline);
            pUIFrameline->SetWindowName(buff);
            pUIFrameline->SetAutoDelete(true);
            pParentWnd->AttachChild(pUIFrameline);

            ++cnt_frameline;
        }
        else if (0 == xr_stricmp(node_name, "auto_text"))
        {
            ++cnt_text;
        }
        node = node->NextSibling();
    }
    /*
        CUIStatic* pUIStatic				= NULL;
        string64							sname;
        for(int i=0; i<items_num; i++)
        {
            pUIStatic						= xr_new<CUIStatic>();
            InitStatic						(xml_doc, "auto_static", i, pUIStatic);
            xr_sprintf						(sname,"auto_static_%d", i);
            pUIStatic->SetWindowName		(sname);
            pUIStatic->SetAutoDelete		(true);
            pParentWnd->AttachChild			(pUIStatic);
            pUIStatic						= NULL;
        }
    */
    xml_doc.SetLocalRoot(_stored_root);
}

void CUIXmlInitBase::InitAutoFrameLineGroup(CUIXml& xml_doc, LPCSTR path, int index, CUIWindow* pParentWnd)
{
    int items_num = xml_doc.GetNodesNum(path, index, "auto_frameline");
    if (items_num == 0)
    {
        return;
    }
    XML_NODE _stored_root = xml_doc.GetLocalRoot();
    xml_doc.SetLocalRoot(xml_doc.NavigateToNode(path, index));

    CUIFrameLineWnd* pUIFL = NULL;
    string64 sname;
    for (int i = 0; i < items_num; ++i)
    {
        pUIFL = xr_new<CUIFrameLineWnd>();
        InitFrameLine(xml_doc, "auto_frameline", i, pUIFL);
        xr_sprintf(sname, "auto_frameline_%d", i);
        pUIFL->SetWindowName(sname);
        pUIFL->SetAutoDelete(true);
        pParentWnd->AttachChild(pUIFL);
        pUIFL = NULL;
    }

    xml_doc.SetLocalRoot(_stored_root);
}

bool CUIXmlInitBase::InitFont(CUIXml& xml_doc, LPCSTR path, int index, u32& color, CGameFont*& pFnt)
{
    color = GetColor(xml_doc, path, index, 0xff);

    LPCSTR font_name = xml_doc.ReadAttrib(path, index, "font", NULL);
    if (!font_name)
    {
        pFnt = NULL;
        return false;
    }
    else
    {
        if (!xr_strcmp(font_name, GRAFFITI19_FONT_NAME))
        {
            pFnt = UI().Font().pFontGraffiti19Russian;
        }
        else if (!xr_strcmp(font_name, GRAFFITI22_FONT_NAME))
        {
            pFnt = UI().Font().pFontGraffiti22Russian;
        }
        else if (!xr_strcmp(font_name, GRAFFITI32_FONT_NAME))
        {
            pFnt = UI().Font().pFontGraffiti32Russian;
        }
        else if (!xr_strcmp(font_name, GRAFFITI50_FONT_NAME))
        {
            pFnt = UI().Font().pFontGraffiti50Russian;
        }
        else if (!xr_strcmp(font_name, "arial_14"))
        {
            pFnt = UI().Font().pFontArial14;
        }
        else if (!xr_strcmp(font_name, MEDIUM_FONT_NAME))
        {
            pFnt = UI().Font().pFontMedium;
        }
        else if (!xr_strcmp(font_name, SMALL_FONT_NAME))
        {
            pFnt = UI().Font().pFontStat;
        }
        else if (!xr_strcmp(font_name, LETTERICA16_FONT_NAME))
        {
            pFnt = UI().Font().pFontLetterica16Russian;
        }
        else if (!xr_strcmp(font_name, LETTERICA18_FONT_NAME))
        {
            pFnt = UI().Font().pFontLetterica18Russian;
        }
        else if (!xr_strcmp(font_name, LETTERICA25_FONT_NAME))
        {
            pFnt = UI().Font().pFontLetterica25;
        }
        else if (!xr_strcmp(font_name, DI_FONT_NAME))
        {
            pFnt = UI().Font().pFontDI;
        }
        else
        {
            R_ASSERT3(0, "unknown font", font_name);
            pFnt = NULL;
        }
    }
    return true;
}

bool CUIXmlInitBase::InitTabControl(CUIXml& xml_doc, LPCSTR path,
    int index, CUITabControl* pWnd, bool fatal /*= true*/, bool defaultIdsAllowed /*= false*/)
{
    const bool nodeExist = xml_doc.NavigateToNode(path, index);
    if (!nodeExist)
    {
        R_ASSERT4(!fatal, "XML node not found", path, xml_doc.m_xml_file_name);
        return false;
    }

    bool status = true;

    status &= InitWindow(xml_doc, path, index, pWnd);
    InitOptionsItem(xml_doc, path, index, pWnd);
    int tabsCount = xml_doc.GetNodesNum(path, index, "button");
    int radio = xml_doc.ReadAttribInt(path, index, "radio");

    XML_NODE tab_node = xml_doc.NavigateToNode(path, index);
    xml_doc.SetLocalRoot(tab_node);

    CUITabButton* newButton;

    for (int i = 0; i < tabsCount; ++i)
    {
        newButton = radio ? xr_new<CUIRadioButton>() : xr_new<CUITabButton>();
        status &= Init3tButton(xml_doc, "button", i, newButton);
        newButton->m_btn_id = xml_doc.ReadAttrib("button", i, "id");
        if (!newButton->m_btn_id.size())
        {
            R_ASSERT4(defaultIdsAllowed, "Tab control tab doesn't have 'id' assigned.", xml_doc.m_xml_file_name, path);
            Msg("~ [%s] doesn't have `id` tag in file [%s]", xml_doc.m_xml_file_name, path);
            string32 temp;
            xr_sprintf(temp, "%d", i);
            newButton->m_btn_id = temp;
            newButton->m_btn_id_default_assigned = true;
        }
        pWnd->AddItem(newButton);
    }

    xml_doc.SetLocalRoot(xml_doc.GetRoot());

    return status;
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInitBase::InitFrameLine(CUIXml& xml_doc, LPCSTR path, int index, CUIFrameLineWnd* pWnd, bool fatal /*= true*/)
{
    const bool nodeExist = xml_doc.NavigateToNode(path, index);
    if (!nodeExist)
    {
        R_ASSERT4(!fatal, "XML node not found", path, xml_doc.m_xml_file_name);
        return false;
    }

    string256 buf;

    bool stretch_flag = xml_doc.ReadAttribInt(path, index, "stretch");
    if (stretch_flag)
    {
        Msg("~ [%s] stretch attribute is unsupported for [%s]", xml_doc.m_xml_file_name, path);
        //.	pWnd->SetStretchTexture( stretch_flag );
    }

    Fvector2 pos, size;
    pos.x = xml_doc.ReadAttribFlt(path, index, "x");
    pos.y = xml_doc.ReadAttribFlt(path, index, "y");

    InitAlignment(xml_doc, path, index, pos.x, pos.y, pWnd);

    size.x = xml_doc.ReadAttribFlt(path, index, "width");
    size.y = xml_doc.ReadAttribFlt(path, index, "height");
    bool vertical = !!xml_doc.ReadAttribInt(path, index, "vertical");

    strconcat(sizeof(buf), buf, path, ":texture");
    shared_str base_name = xml_doc.Read(buf, index, NULL);

    VERIFY(base_name);

    u32 color = GetColor(xml_doc, buf, index, 0xff);
    pWnd->SetTextureColor(color);

    InitWindow(xml_doc, path, index, pWnd);
    
    return pWnd->InitFrameLineWnd(*base_name, pos, size, !vertical, fatal);
}

bool CUIXmlInitBase::InitTextFrameLine(CUIXml& xml_doc, LPCSTR path, int index, CUITextFrameLineWnd* pWnd, bool fatal /*= true*/)
{
    string256 buf;
    strconcat(sizeof(buf), buf, path, ":title");
    InitStatic(xml_doc, buf, index, &pWnd->m_title, false);

    return InitFrameLine(xml_doc, path, index, &pWnd->m_frameline, fatal);
}

bool CUIXmlInitBase::InitCustomEdit(CUIXml& xml_doc, LPCSTR path, int index, CUICustomEdit* pWnd, bool fatal /*= true*/)
{
    if (!InitStatic(xml_doc, path, index, pWnd, fatal))
        return false;

    pWnd->InitCustomEdit(pWnd->GetWndPos(), pWnd->GetWndSize());

    string256 foo;
    u32 color;
    strconcat(sizeof(foo), foo, path, ":text_color:e");
    if (xml_doc.NavigateToNode(foo, index))
    {
        color = GetColor(xml_doc, foo, index, 0x00);
        pWnd->TextItemControl()->SetTextColor(color);
    }

    int max_count = xml_doc.ReadAttribInt(path, index, "max_symb_count", 0);
    bool num_only = (xml_doc.ReadAttribInt(path, index, "num_only", 0) == 1);
    bool read_only = (xml_doc.ReadAttribInt(path, index, "read_only", 0) == 1);
    bool file_name_mode = (xml_doc.ReadAttribInt(path, index, "file_name_mode", 0) == 1);

    if (file_name_mode || read_only || num_only || 0 < max_count)
    {
        if (max_count <= 0)
        {
            max_count = 32;
        }
        pWnd->Init(max_count, num_only, read_only, file_name_mode);
    }

    if (xml_doc.ReadAttribInt(path, index, "password", 0))
    {
        pWnd->SetPasswordMode();
    }
    return true;
}

bool CUIXmlInitBase::InitEditBox(CUIXml& xml_doc, LPCSTR path, int index, CUIEditBox* pWnd, bool fatal /*= true*/)
{
    if (!InitCustomEdit(xml_doc, path, index, pWnd))
        return false;

    InitTexture(xml_doc, path, index, pWnd);
    InitOptionsItem(xml_doc, path, index, pWnd);

    return true;
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInitBase::InitAnimatedStatic(CUIXml& xml_doc, LPCSTR path, int index, CUIAnimatedStatic* pWnd, bool fatal /*= true*/)
{
    if (!InitStatic(xml_doc, path, index, pWnd, fatal))
        return false;

    float x = xml_doc.ReadAttribFlt(path, index, "x_offset", 0);
    float y = xml_doc.ReadAttribFlt(path, index, "y_offset", 0);
    u32 framesCount = static_cast<u32>(xml_doc.ReadAttribInt(path, index, "frames", 0));
    u32 animDuration = static_cast<u32>(xml_doc.ReadAttribInt(path, index, "duration", 0));
    u32 animCols = static_cast<u32>(xml_doc.ReadAttribInt(path, index, "columns", 0));
    float frameWidth = xml_doc.ReadAttribFlt(path, index, "frame_width", 0);
    float frameHeight = xml_doc.ReadAttribFlt(path, index, "frame_height", 0);
    bool cyclic = !!xml_doc.ReadAttribInt(path, index, "cyclic", 0);
    bool play = !!xml_doc.ReadAttribInt(path, index, "autoplay", 0);

    pWnd->SetFrameDimentions(frameWidth, frameHeight);
    pWnd->SetFramesCount(framesCount);
    pWnd->m_bCyclic = cyclic;
    pWnd->SetAnimCols(animCols);
    pWnd->SetAnimationDuration(animDuration);
    pWnd->SetOffset(x, y);
    pWnd->SetAnimPos(0.0f);
    if (play)
        pWnd->Play();

    return true;
}

bool CUIXmlInitBase::InitTexture(CUIXml& xml_doc, LPCSTR path, int index, ITextureOwner* pWnd, bool fatal /*= true*/)
{
    bool result = true;

    string256 buf;
    LPCSTR texture = NULL;
    LPCSTR shader = NULL;
    strconcat(sizeof(buf), buf, path, ":texture");
    if (xml_doc.NavigateToNode(buf))
    {
        texture = xml_doc.Read(buf, index, NULL);
        shader = xml_doc.ReadAttrib(buf, index, "shader", NULL);
    }
    if (texture)
    {
        if (shader)
            result = pWnd->InitTextureEx(texture, shader, fatal);
        else
            result = pWnd->InitTexture(texture, fatal);
    }
    //--------------------
    Frect rect;
    rect.x1 = xml_doc.ReadAttribFlt(buf, index, "x", 0);
    rect.y1 = xml_doc.ReadAttribFlt(buf, index, "y", 0);
    rect.x2 = rect.x1 + xml_doc.ReadAttribFlt(buf, index, "width", 0);
    rect.y2 = rect.y1 + xml_doc.ReadAttribFlt(buf, index, "height", 0);

    bool stretch_flag = xml_doc.ReadAttribInt(path, index, "stretch") ? true : false;
    pWnd->SetStretchTexture(stretch_flag);

    u32 color = GetColor(xml_doc, buf, index, 0xff);
    pWnd->SetTextureColor(color);

    if (rect.width() != 0 && rect.height() != 0)
        pWnd->SetTextureRect(rect);

    return result;
}

bool CUIXmlInitBase::InitTextureOffset(CUIXml& xml_doc, LPCSTR path, int index, CUIStatic* pWnd)
{
    string256 textureOffset;
    if (0 == xr_strcmp(path, ""))
        xr_strcpy(textureOffset, "texture_offset");
    else
        strconcat(sizeof(textureOffset), textureOffset, path, ":texture_offset");

    float x = xml_doc.ReadAttribFlt(textureOffset, index, "x");
    float y = xml_doc.ReadAttribFlt(textureOffset, index, "y");

    pWnd->SetTextureOffset(x, y);

    return true;
}

bool CUIXmlInitBase::InitMultiTexture(CUIXml& xml_doc, LPCSTR path, int index, CUI3tButton* pWnd)
{
    string256 buff;
    bool success = false;

    strconcat(sizeof(buff), buff, path, ":texture");
    shared_str texture = xml_doc.Read(buff, index, NULL);

    if (texture.size() > 0)
    {
        pWnd->InitTexture(*texture);
        return true;
    }

    strconcat(sizeof(buff), buff, path, ":texture_e");
    texture = xml_doc.Read(buff, index, NULL);
    if (texture.size())
    {
        if (pWnd->m_background)
        {
            pWnd->m_background->InitState(S_Enabled, texture.c_str());
        }
        else if (pWnd->m_back_frameline)
        {
            pWnd->m_back_frameline->InitState(S_Enabled, texture.c_str());
            pWnd->m_back_frameline->Get(S_Enabled)->SetHorizontal(!(pWnd->vertical));
        }
        success = true;
    }

    strconcat(sizeof(buff), buff, path, ":texture_t");
    texture = xml_doc.Read(buff, index, NULL);
    if (texture.size())
    {
        if (pWnd->m_background)
        {
            pWnd->m_background->InitState(S_Touched, texture.c_str());
        }
        else if (pWnd->m_back_frameline)
        {
            pWnd->m_back_frameline->InitState(S_Touched, texture.c_str());
            pWnd->m_back_frameline->Get(S_Touched)->SetHorizontal(!(pWnd->vertical));
        }
        success = true;
    }

    strconcat(sizeof(buff), buff, path, ":texture_d");
    texture = xml_doc.Read(buff, index, NULL);
    if (texture.size())
    {
        if (pWnd->m_background)
        {
            pWnd->m_background->InitState(S_Disabled, texture.c_str());
        }
        else if (pWnd->m_back_frameline)
        {
            pWnd->m_back_frameline->InitState(S_Disabled, texture.c_str());
            pWnd->m_back_frameline->Get(S_Disabled)->SetHorizontal(!(pWnd->vertical));
        }
        success = true;
    }

    strconcat(sizeof(buff), buff, path, ":texture_h");
    texture = xml_doc.Read(buff, index, NULL);
    if (texture.size())
    {
        if (pWnd->m_background)
        {
            pWnd->m_background->InitState(S_Highlighted, texture.c_str());
        }
        else if (pWnd->m_back_frameline)
        {
            pWnd->m_back_frameline->InitState(S_Highlighted, texture.c_str());
            pWnd->m_back_frameline->Get(S_Highlighted)->SetHorizontal(!(pWnd->vertical));
        }
        success = true;
    }

    if (success)
        pWnd->TextureOn();

    return success;
}

float CUIXmlInitBase::ApplyAlignX(float coord, u32 align) { return coord; }
//////////////////////////////////////////////////////////////////////////

float CUIXmlInitBase::ApplyAlignY(float coord, u32 align) { return coord; }
//////////////////////////////////////////////////////////////////////////

void CUIXmlInitBase::ApplyAlign(float& x, float& y, u32 align)
{
    x = ApplyAlignX(x, align);
    y = ApplyAlignY(y, align);
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInitBase::InitAlignment(CUIXml& xml_doc, const char* path, int index, float& x, float& y, CUIWindow* pWnd)
{
    xr_string wnd_alignment = xml_doc.ReadAttrib(path, index, "alignment", "");

    if (strchr(wnd_alignment.c_str(), 'c'))
        pWnd->SetAlignment(waCenter);

    // Alignment: right: "r", bottom: "b". Top, left - useless
    shared_str alignStr = xml_doc.ReadAttrib(path, index, "align", "");

    bool result = false;

    if (strchr(*alignStr, 'r'))
    {
        x = ApplyAlignX(x, alRight);
        result = true;
    }
    if (strchr(*alignStr, 'b'))
    {
        y = ApplyAlignY(y, alBottom);
        result = true;
    }
    if (strchr(*alignStr, 'c'))
    {
        ApplyAlign(x, y, alCenter);
        result = true;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////

void CUIXmlInitBase::InitColorDefs()
{
    if (!m_pColorDefs)
        m_pColorDefs = xr_new<ColorDefs>();

    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, COLOR_DEFINITIONS);

    const int num = uiXml.GetNodesNum("colors", 0, "color");

    shared_str name;
    int r, b, g, a;

    for (int i = 0; i < num; ++i)
    {
        name = uiXml.ReadAttrib("color", i, "name", "");
        r = uiXml.ReadAttribInt("color", i, "r", 0);
        g = uiXml.ReadAttribInt("color", i, "g", 0);
        b = uiXml.ReadAttribInt("color", i, "b", 0);
        a = uiXml.ReadAttribInt("color", i, "a", 255);

        (*m_pColorDefs)[name] = color_argb(a, r, g, b);
    }
}

bool CUIXmlInitBase::InitScrollView(CUIXml& xml_doc, LPCSTR path, int index, CUIScrollView* pWnd, bool fatal /*= true*/)
{
    if (!InitWindow(xml_doc, path, index, pWnd, fatal))
        return false;

    pWnd->SetRightIndention(xml_doc.ReadAttribFlt(path, index, "right_ident", 0.0f));
    pWnd->SetLeftIndention(xml_doc.ReadAttribFlt(path, index, "left_ident", 0.0f));
    pWnd->SetUpIndention(xml_doc.ReadAttribFlt(path, index, "top_indent", 0.0f));
    pWnd->SetDownIndention(xml_doc.ReadAttribFlt(path, index, "bottom_indent", 0.0f));

    float vi = xml_doc.ReadAttribFlt(path, index, "vert_interval", 0.0f);
    pWnd->m_vertInterval = (vi);

    bool bInverseDir = (1 == xml_doc.ReadAttribInt(path, index, "inverse_dir", 0));
    pWnd->m_flags.set(CUIScrollView::eInverseDir, bInverseDir);

    pWnd->SetScrollBarProfile(xml_doc.ReadAttrib(path, index, "scroll_profile", "default"));

    pWnd->InitScrollView();

    bool bVertFlip = (1 == xml_doc.ReadAttribInt(path, index, "flip_vert", 0));
    pWnd->SetVertFlip(bVertFlip);

    bool b = (1 == xml_doc.ReadAttribInt(path, index, "always_show_scroll", 1));

    pWnd->SetFixedScrollBar(b);

    b = (1 == xml_doc.ReadAttribInt(path, index, "can_select", 0));

    pWnd->m_flags.set(CUIScrollView::eItemsSelectabe, b);

    /////////////////////////////////////////////////////////////////////
    int tabsCount = xml_doc.GetNodesNum(path, index, "text");

    XML_NODE _stored_root = xml_doc.GetLocalRoot();
    xml_doc.SetLocalRoot(xml_doc.NavigateToNode(path, index));

    for (int i = 0; i < tabsCount; ++i)
    {
        CUITextWnd* newText = xr_new<CUITextWnd>();
        InitText(xml_doc, "text", i, &newText->TextItemControl());
        newText->SetTextComplexMode(true);
        newText->SetWidth(pWnd->GetDesiredChildWidth());
        newText->AdjustHeightToText();
        pWnd->AddWindow(newText, true);
    }
    xml_doc.SetLocalRoot(_stored_root);
    return true;
}

bool CUIXmlInitBase::InitListWnd(CUIXml& xml_doc, pcstr path, int index, CUIListWnd* pWnd, bool fatal /*= true*/)
{
    const bool nodeExist = xml_doc.NavigateToNode(path, index);
    if (!nodeExist)
    {
        R_ASSERT4(!fatal, "XML node not found", path, xml_doc.m_xml_file_name);
        return false;
    }

    Fvector2 pos, size;
    pos.x = xml_doc.ReadAttribFlt(path, index, "x");
    pos.y = xml_doc.ReadAttribFlt(path, index, "y");

    InitAlignment(xml_doc, path, index, pos.x, pos.y, pWnd);

    size.x = xml_doc.ReadAttribFlt(path, index, "width");
    size.y = xml_doc.ReadAttribFlt(path, index, "height");
    float item_height = xml_doc.ReadAttribFlt(path, index, "item_height");
    int active_background = xml_doc.ReadAttribInt(path, index, "active_bg");

    // Init font from xml config file
    string256 buf;
    CGameFont* LocalFont = nullptr;
    u32 cl;

    shared_str text_path = strconcat(sizeof(buf), buf, path, ":font");
    InitFont(xml_doc, *text_path, index, cl, LocalFont);
    if (LocalFont)
    {
        pWnd->SetFont(LocalFont);
        pWnd->SetTextColor(cl);
    }

    pWnd->SetScrollBarProfile(xml_doc.ReadAttrib(path, index, "scroll_profile", "default"));
    pWnd->InitListWnd(pos, size, item_height);
    pWnd->EnableActiveBackground(!!active_background);

    if (xml_doc.ReadAttribInt(path, index, "always_show_scroll"))
    {
        pWnd->SetAlwaysShowScroll(true);
        pWnd->EnableAlwaysShowScroll(true);
        pWnd->EnableScrollBar(true);
    }

    if (xml_doc.ReadAttribInt(path, index, "always_hide_scroll"))
    {
        pWnd->SetAlwaysShowScroll(false);
        pWnd->EnableAlwaysShowScroll(true);
    }


    bool bVertFlip = (1 == xml_doc.ReadAttribInt(path, index, "flip_vert", 0));
    pWnd->SetVertFlip(bVertFlip);

    return true;
}

bool CUIXmlInitBase::InitListBox(CUIXml& xml_doc, LPCSTR path, int index, CUIListBox* pWnd, bool fatal /*= true*/)
{
    if (!InitScrollView(xml_doc, path, index, pWnd))
        return false;

    string512 _path;
    u32 t_color;
    CGameFont* pFnt;
    strconcat(sizeof(_path), _path, path, ":font");
    InitFont(xml_doc, _path, index, t_color, pFnt);

    pWnd->SetTextColor(t_color);
    pWnd->SetFont(pFnt);

    float h = xml_doc.ReadAttribFlt(path, index, "item_height", 20.0f);
    pWnd->SetItemHeight(h);
    return true;
}

bool CUIXmlInitBase::InitTrackBar(CUIXml& xml_doc, LPCSTR path, int index, CUITrackBar* pWnd, bool fatal /*= true*/)
{
    if (!InitWindow(xml_doc, path, 0, pWnd, fatal))
        return false;

    pWnd->InitTrackBar(pWnd->GetWndPos(), pWnd->GetWndSize());
    int is_integer = xml_doc.ReadAttribInt(path, index, "is_integer", 0);
    pWnd->SetType(!is_integer);
    InitOptionsItem(xml_doc, path, 0, pWnd);

    int invert = xml_doc.ReadAttribInt(path, index, "invert", 0);
    pWnd->SetInvert(!!invert);
    float step = xml_doc.ReadAttribFlt(path, index, "step", 0.1f);
    pWnd->SetStep(step);

    bool is_float = !is_integer;
    if (is_float)
    {
        float fmin = xml_doc.ReadAttribFlt(path, index, "min", 0.0f);
        float fmax = xml_doc.ReadAttribFlt(path, index, "max", 0.0f);
        
        if (fmin != fmax)
        {
            pWnd->SetOptFBounds(fmin, fmax);
            pWnd->SetBoundReady(true);
        }
    }
    else
    {
        int imin = xml_doc.ReadAttribInt(path, index, "min", 0);
        int imax = xml_doc.ReadAttribInt(path, index, "max", 0);

        if (imin != imax)
        {
            pWnd->SetOptIBounds(imin, imax);
            pWnd->SetBoundReady(true);
        }
    }

    string512 buf;
    strconcat(sizeof(buf), buf, path, ":output_wnd");
    if (xml_doc.NavigateToNode(buf, index))
    {
        InitStatic(xml_doc, buf, index, pWnd->m_static);
        pWnd->m_static_format = xml_doc.ReadAttrib(buf, index, "format", nullptr);
        pWnd->m_static->Enable(true);
    }

    return true;
}

bool CUIXmlInitBase::InitComboBox(CUIXml& xml_doc, LPCSTR path, int index, CUIComboBox* pWnd)
{
    u32 color;
    CGameFont* pFont;

    pWnd->SetListLength(xml_doc.ReadAttribInt(path, index, "list_length", 4));

    InitWindow(xml_doc, path, index, pWnd);
    pWnd->InitComboBox(pWnd->GetWndPos(), pWnd->GetWidth());
    InitOptionsItem(xml_doc, path, index, pWnd);

    bool b = (1 == xml_doc.ReadAttribInt(path, index, "always_show_scroll", 1));

    pWnd->m_list_box.SetFixedScrollBar(b);

    string512 _path;
    strconcat(sizeof(_path), _path, path, ":list_font");
    InitFont(xml_doc, _path, index, color, pFont);
    //.	pWnd->SetFont				(pFont);
    pWnd->m_list_box.SetFont(pFont);
    pWnd->m_list_box.SetTextColor(color);

    strconcat(sizeof(_path), _path, path, ":text_color:e");
    if (xml_doc.NavigateToNode(_path, index))
    {
        color = GetColor(xml_doc, _path, index, 0x00);
        pWnd->SetTextColor(color);
    }

    strconcat(sizeof(_path), _path, path, ":text_color:d");
    if (xml_doc.NavigateToNode(_path, index))
    {
        color = GetColor(xml_doc, _path, index, 0x00);
        pWnd->SetTextColorD(color);
    }

    return true;
}

void CUIXmlInitBase::AssignColor(LPCSTR name, u32 clr) { (*m_pColorDefs)[name] = clr; }
u32 CUIXmlInitBase::GetColor(CUIXml& xml_doc, LPCSTR path, int index, u32 def_clr)
{
    LPCSTR clr_def = xml_doc.ReadAttrib(path, index, "color", NULL);
    if (clr_def)
    {
        VERIFY(GetColorDefs()->find(clr_def) != GetColorDefs()->end());
        return (*m_pColorDefs)[clr_def];
    }
    else
    {
        int r = xml_doc.ReadAttribInt(path, index, "r", def_clr);
        int g = xml_doc.ReadAttribInt(path, index, "g", def_clr);
        int b = xml_doc.ReadAttribInt(path, index, "b", def_clr);
        int a = xml_doc.ReadAttribInt(path, index, "a", 0xff);
        return color_argb(a, r, g, b);
    }
}
