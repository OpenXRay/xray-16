#include "pch.hpp"
#include "UIXmlInitBase.h"
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

constexpr pcstr ARIAL_FONT_NAME       = "arial";

constexpr pcstr MEDIUM_FONT_NAME      = "medium";
constexpr pcstr SMALL_FONT_NAME       = "small";

constexpr pcstr GRAFFITI19_FONT_NAME  = "graffiti19";
constexpr pcstr GRAFFITI22_FONT_NAME  = "graffiti22";
constexpr pcstr GRAFFITI32_FONT_NAME  = "graffiti32";
constexpr pcstr GRAFFITI50_FONT_NAME  = "graffiti50";

constexpr pcstr LETTERICA16_FONT_NAME = "letterica16";
constexpr pcstr LETTERICA18_FONT_NAME = "letterica18";
constexpr pcstr LETTERICA25_FONT_NAME = "letterica25";

constexpr pcstr DI_FONT_NAME          = "di";

//////////////////////////////////////////////////////////////////////////

constexpr pcstr COLOR_DEFINITIONS = "color_defs.xml";
CUIXmlInitBase::ColorDefs* CUIXmlInitBase::m_pColorDefs = nullptr;

//////////////////////////////////////////////////////////////////////////

CUIXmlInitBase::CUIXmlInitBase() { InitColorDefs(); }
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

Frect CUIXmlInitBase::GetFRect(const CUIXml& xml_doc, pcstr path, int index)
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

bool CUIXmlInitBase::InitWindow(CUIXml& xml_doc, pcstr path, int index, CUIWindow* pWnd, bool fatal /*= true*/)
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
    strconcat(buf, path, ":window_name");
    if (xml_doc.NavigateToNode(buf, index))
        pWnd->SetWindowName(xml_doc.Read(buf, index, nullptr));

    InitAutoStaticGroup(xml_doc, path, index, pWnd);
    //.	InitAutoFrameLineGroup		(xml_doc, path, index, pWnd);

    return true;
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInitBase::InitFrameWindow(CUIXml& xml_doc, pcstr path, int index, CUIFrameWindow* pWnd, bool fatal /*= true*/)
{
    bool result = InitWindow(xml_doc, path, index, pWnd, fatal);
    result &= InitTexture(xml_doc, path, index, pWnd, fatal);

    return result;
}

bool CUIXmlInitBase::InitOptionsItem(const CUIXml& xml_doc, pcstr path, int index, CUIOptionsItem* pWnd)
{
    string256 buf;
    strconcat(buf, path, ":options_item");

    if (xml_doc.NavigateToNode(buf, index))
    {
        cpcstr entry = xml_doc.ReadAttrib(buf, index, "entry");
        cpcstr group = xml_doc.ReadAttrib(buf, index, "group");
        pWnd->AssignProps(entry, group);

        if (cpcstr depends = xml_doc.ReadAttrib(buf, index, "depend", nullptr))
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
                Msg("! unknown param [%s] in optionsItem [%s]", depends, entry);

            pWnd->SetSystemDepends(d);
        }
        return true;
    }

    return false;
}

bool CUIXmlInitBase::InitStatic(CUIXml& xml_doc, pcstr path, int index, CUIStatic* pWnd, bool fatal /*= true*/, bool textWnd /*= false*/)
{
    if (!InitWindow(xml_doc, path, index, pWnd, fatal))
        return false;

    string256 buf;
    InitText(xml_doc, strconcat(buf, path, ":text"), index, pWnd->TextItemControl());
    InitTexture(xml_doc, path, index, pWnd);
    InitTextureOffset(xml_doc, path, index, pWnd);

    const int flag = xml_doc.ReadAttribInt(path, index, "heading", 0);
    pWnd->EnableHeading((flag) ? true : false);

    const float heading_angle = xml_doc.ReadAttribFlt(path, index, "heading_angle", 0.0f);
    if (!fis_zero(heading_angle))
    {
        pWnd->EnableHeading(true);
        pWnd->SetConstHeading(true);
        pWnd->SetHeading(deg2rad(heading_angle));
    }

    pcstr str_flag = xml_doc.ReadAttrib(path, index, "light_anim", "");
    int flag_cyclic = xml_doc.ReadAttribInt(path, index, "la_cyclic", 1);
    const int flag_text = xml_doc.ReadAttribInt(path, index, "la_text", 1);
    const int flag_texture = xml_doc.ReadAttribInt(path, index, "la_texture", 1);
    const int flag_alpha = xml_doc.ReadAttribInt(path, index, "la_alpha", 0);

    u8 flags = 0;
    if (flag_cyclic)
        flags |= LA_CYCLIC;
    if (flag_alpha)
        flags |= LA_ONLYALPHA;
    if (flag_text || textWnd)
        flags |= LA_TEXTCOLOR;
    if (flag_texture)
        flags |= LA_TEXTURECOLOR;

    pWnd->SetColorAnimation(str_flag, flags);

    str_flag = xml_doc.ReadAttrib(path, index, "xform_anim", "");
    flag_cyclic = xml_doc.ReadAttribInt(path, index, "xform_anim_cyclic", 1);

    pWnd->SetXformLightAnim(str_flag, flag_cyclic ? true : false);

    if (xml_doc.ReadAttribInt(path, index, "complex_mode", 0) ? true : false)
        pWnd->TextItemControl()->SetTextComplexMode(true);

    pWnd->m_stat_hint_text = xml_doc.ReadAttrib(path, index, "hint", "");

    if (textWnd)
    {
        strconcat(buf, path, ":texture");
        R_ASSERT3(nullptr == xml_doc.NavigateToNode(buf, index), xml_doc.m_xml_file_name, buf);

        R_ASSERT2(pWnd->GetChildWndList().empty(),
            "CUITextWnd should have no children. "
            "Use InitStatic from Lua, if you want your UI element to have children.");
    }
    return true;
}

bool CUIXmlInitBase::InitCheck(CUIXml& xml_doc, pcstr path, int index, CUICheckButton* pWnd, bool fatal /*= true*/)
{
    if (!InitStatic(xml_doc, path, index, pWnd, fatal))
        return false;

    string256 buf;
    strconcat(buf, path, ":texture");
    cpcstr texture = xml_doc.Read(buf, index, "ui_checker");

    pWnd->InitCheckButton(pWnd->GetWndPos(), pWnd->GetWndSize(), texture);

    u32 color;
    strconcat(buf, path, ":text_color:e");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Enabled);
    }

    strconcat(buf, path, ":text_color:d");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Disabled);
    }

    strconcat(buf, path, ":text_color:t");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Touched);
    }

    strconcat(buf, path, ":text_color:h");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Highlighted);
    }

    InitOptionsItem(xml_doc, path, index, pWnd);

    return true;
}

bool CUIXmlInitBase::InitSpin(CUIXml& xml_doc, pcstr path, int index, CUICustomSpin* pWnd, bool fatal /*= true*/)
{
    if (!InitWindow(xml_doc, path, index, pWnd, fatal))
        return false;

    InitOptionsItem(xml_doc, path, index, pWnd);
    pWnd->InitSpin(pWnd->GetWndPos(), pWnd->GetWndSize());

    string256 foo;
    u32 color;
    strconcat(foo, path, ":text_color:e");
    if (xml_doc.NavigateToNode(foo, index))
    {
        color = GetColor(xml_doc, foo, index, 0x00);
        pWnd->SetTextColor(color);
    }
    strconcat(foo, path, ":text_color:d");
    if (xml_doc.NavigateToNode(foo, index))
    {
        color = GetColor(xml_doc, foo, index, 0x00);
        pWnd->SetTextColorD(color);
    }

    return true;
}

bool CUIXmlInitBase::InitText(CUIXml& xml_doc, pcstr path, int index, CUILines* pLines)
{
    if (!xml_doc.NavigateToNode(path, index))
        return false;

    u32 color;
    CGameFont* pTmpFont = nullptr;
    InitFont(xml_doc, path, index, color, pTmpFont);
    pLines->SetTextColor(color);
    if (pTmpFont)
        pLines->SetFont(pTmpFont);
    else
    {
#ifndef MASTER_GOLD
        Msg("~ Missing 'font' attribute in node [%s] in file [%s]", path, xml_doc.m_xml_file_name);
#endif
    }

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
    // m_TextOffset can be already set during parent element creation (e.g. 3tButton)
    // so reuse it as default.
    const float text_x = xml_doc.ReadAttribFlt(path, index, "x", pLines->m_TextOffset.x);
    const float text_y = xml_doc.ReadAttribFlt(path, index, "y", pLines->m_TextOffset.y);

    pLines->m_TextOffset = { text_x, text_y };

    if (cpcstr text = xml_doc.Read(path, index, nullptr))
        pLines->SetText(StringTable().translate(text).c_str());

    return true;
}

bool CUIXmlInitBase::Init3tButton(CUIXml& xml_doc, pcstr path, int index, CUI3tButton* pWnd, bool fatal /*= true*/)
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
    InitText(xml_doc, strconcat(buf, path, ":text"), index, pWnd->TextItemControl());
    u32 color;

    strconcat(buf, path, ":text_color:e");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Enabled);
    }

    strconcat(buf, path, ":text_color:d");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Disabled);
    }

    strconcat(buf, path, ":text_color:t");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Touched);
    }

    strconcat(buf, path, ":text_color:h");
    if (xml_doc.NavigateToNode(buf, index))
    {
        color = GetColor(xml_doc, buf, index, 0x00);
        pWnd->SetStateTextColor(color, S_Highlighted);
    }

    InitMultiTexture(xml_doc, path, index, pWnd);
    InitTextureOffset(xml_doc, path, index, pWnd);
    InitSound(xml_doc, path, index, pWnd);

    if (cpcstr accel = xml_doc.ReadAttrib(path, index, "accel", nullptr))
    {
        const int acc = KeynameToDik(accel);
        pWnd->SetAccelerator(acc, 0);
    }

    if (cpcstr accel = xml_doc.ReadAttrib(path, index, "accel_ext", nullptr))
    {
        const int acc = KeynameToDik(accel);
        pWnd->SetAccelerator(acc, 1);
    }

    if (cpcstr text_hint = xml_doc.ReadAttrib(path, index, "hint", nullptr))
        pWnd->m_hint_text = StringTable().translate(text_hint);

    return true;
}

bool CUIXmlInitBase::InitSound(const CUIXml& xml_doc, pcstr path, int index, CUI3tButton* pWnd)
{
    string256 sound_h;
    string256 sound_t;
    strconcat(sound_h, path, ":sound_h");
    strconcat(sound_t, path, ":sound_t");

    cpcstr sound_h_result = xml_doc.Read(sound_h, index, "");
    cpcstr sound_t_result = xml_doc.Read(sound_t, index, "");

    if (xr_strlen(sound_h_result) != 0)
        pWnd->InitSoundH(sound_h_result);

    if (xr_strlen(sound_t_result) != 0)
        pWnd->InitSoundT(sound_t_result);

    return true;
}

bool CUIXmlInitBase::InitProgressBar(CUIXml& xml_doc, pcstr path, int index, CUIProgressBar* pWnd, bool fatal /*= true*/)
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
    const int mode_horz = xml_doc.ReadAttribInt(path, index, "horz", 0);
    const pcstr mode_str = xml_doc.ReadAttrib(path, index, "mode");
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

    const float min = xml_doc.ReadAttribFlt(path, index, "min");
    const float max = xml_doc.ReadAttribFlt(path, index, "max");
    const float ppos = xml_doc.ReadAttribFlt(path, index, "pos");

    pWnd->SetRange(min, max);
    pWnd->SetProgressPos(ppos);
    pWnd->m_inertion = xml_doc.ReadAttribFlt(path, index, "inertion", 0.0f);

    // progress
    strconcat(buf, path, ":progress");

    if (!xml_doc.NavigateToNode(buf, index))
        return false;

    InitStatic(xml_doc, buf, index, &pWnd->m_UIProgressItem);

    pWnd->m_UIProgressItem.SetWndSize(pWnd->GetWndSize());

    // background
    strconcat(buf, path, ":background");

    if (xml_doc.NavigateToNode(buf, index))
    {
        InitStatic(xml_doc, buf, index, &pWnd->m_UIBackgroundItem);
        pWnd->m_bBackgroundPresent = true;
        pWnd->m_UIBackgroundItem.SetWndSize(pWnd->GetWndSize());
    }

    strconcat(buf, path, ":min_color");

    if (xml_doc.NavigateToNode(buf, index))
    {
        pWnd->m_bUseColor = true;

        pWnd->m_minColor = GetColor(xml_doc, buf, index, 0xff);

        strconcat(buf, path, ":middle_color");
        if (xml_doc.NavigateToNode(buf, 0))
        {
            pWnd->m_middleColor = GetColor(xml_doc, buf, index, 0xff);
            pWnd->m_bUseMiddleColor = true;
        }

        strconcat(buf, path, ":max_color");
        pWnd->m_maxColor = GetColor(xml_doc, buf, index, 0xff);
    }

    return true;
}

bool CUIXmlInitBase::InitProgressShape(CUIXml& xml_doc, pcstr path, int index, CUIProgressShape* pWnd, bool fatal /*= true*/)
{
    if (!InitStatic(xml_doc, path, index, pWnd, fatal))
        return false;

    if (xml_doc.ReadAttribInt(path, index, "text"))
        pWnd->SetTextVisible(true);

    string256 _path;

    strconcat(_path, path, ":back");
    if (xml_doc.NavigateToNode(_path, index))
    {
        pWnd->m_pBackground = xr_new<CUIStatic>("Background");
        pWnd->m_pBackground->SetAutoDelete(true);
        pWnd->AttachChild(pWnd->m_pBackground);
        InitStatic(xml_doc, _path, index, pWnd->m_pBackground);
    }

    strconcat(_path, path, ":front");
    if (xml_doc.NavigateToNode(_path, index))
    {
        pWnd->m_pTexture = xr_new<CUIStatic>("Forefround");
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

void CUIXmlInitBase::InitAutoStaticGroup(CUIXml& xml_doc, pcstr path, int index, CUIWindow* pParentWnd)
{
    const XML_NODE _stored_root = xml_doc.GetLocalRoot();
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
        cpcstr node_name = node->Value();
        if (0 == xr_stricmp(node_name, "auto_static"))
        {
            xr_sprintf(buff, "auto_static_%d", cnt_static);
            CUIStatic* pUIStatic = xr_new<CUIStatic>(buff);

            // some code (e.g. CUISequenceSimpleItem) relies on window name
            // but it can get overriden during initialization, so set name again.
            InitStatic(xml_doc, "auto_static", cnt_static, pUIStatic);
            pUIStatic->SetWindowName(buff);

            pUIStatic->SetAutoDelete(true);
            pParentWnd->AttachChild(pUIStatic);

            ++cnt_static;
        }
        else if (0 == xr_stricmp(node_name, "auto_frameline"))
        {
            xr_sprintf(buff, "auto_frameline_%d", cnt_frameline);
            CUIFrameLineWnd* pUIFrameline = xr_new<CUIFrameLineWnd>(buff);

            // some code relies on window name
            // but it can get overriden during initialization, so set name again.
            InitFrameLine(xml_doc, "auto_frameline", cnt_frameline, pUIFrameline);
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

void CUIXmlInitBase::InitAutoFrameLineGroup(CUIXml& xml_doc, pcstr path, int index, CUIWindow* pParentWnd)
{
    const int items_num = (int)xml_doc.GetNodesNum(path, index, "auto_frameline");
    if (items_num == 0)
        return;

    const XML_NODE _stored_root = xml_doc.GetLocalRoot();
    xml_doc.SetLocalRoot(xml_doc.NavigateToNode(path, index));

    string64 sname;
    for (int i = 0; i < items_num; ++i)
    {
        xr_sprintf(sname, "auto_frameline_%d", i);
        auto* pUIFL = xr_new<CUIFrameLineWnd>(sname);
        InitFrameLine(xml_doc, "auto_frameline", i, pUIFL);
        pUIFL->SetAutoDelete(true);
        pParentWnd->AttachChild(pUIFL);
    }

    xml_doc.SetLocalRoot(_stored_root);
}

bool CUIXmlInitBase::InitFont(const CUIXml& xml_doc, pcstr path, int index, u32& color, CGameFont*& pFnt)
{
    color = GetColor(xml_doc, path, index, 0xff);

    cpcstr font_name = xml_doc.ReadAttrib(path, index, "font", nullptr);
    if (!font_name)
    {
        pFnt = nullptr;
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
            pFnt = nullptr;
        }
    }
    return true;
}

bool CUIXmlInitBase::InitTabControl(CUIXml& xml_doc, pcstr path,
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
    const auto tabsCount = (int)xml_doc.GetNodesNum(path, index, "button");
    const int radio = xml_doc.ReadAttribInt(path, index, "radio");

    const XML_NODE tab_node = xml_doc.NavigateToNode(path, index);
    xml_doc.SetLocalRoot(tab_node);

    for (int i = 0; i < tabsCount; ++i)
    {
        CUITabButton* newButton = radio ? xr_new<CUIRadioButton>() : xr_new<CUITabButton>();
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

bool CUIXmlInitBase::InitFrameLine(CUIXml& xml_doc, pcstr path, int index, CUIFrameLineWnd* pWnd, bool fatal /*= true*/)
{
    const bool nodeExist = xml_doc.NavigateToNode(path, index);
    if (!nodeExist)
    {
        R_ASSERT4(!fatal, "XML node not found", path, xml_doc.m_xml_file_name);
        return false;
    }

    string256 buf;

    if (xml_doc.ReadAttribInt(path, index, "stretch"))
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
    const bool vertical = !!xml_doc.ReadAttribInt(path, index, "vertical");

    strconcat(buf, path, ":texture");
    const shared_str base_name = xml_doc.Read(buf, index, nullptr);

#ifdef DEBUG
    VERIFY(base_name);
#endif

    const u32 color = GetColor(xml_doc, buf, index, 0xff);
    pWnd->SetTextureColor(color);

    InitWindow(xml_doc, path, index, pWnd);

    return pWnd->InitFrameLineWnd(*base_name, pos, size, !vertical, fatal);
}

bool CUIXmlInitBase::InitTextFrameLine(CUIXml& xml_doc, pcstr path, int index, CUITextFrameLineWnd* pWnd, bool fatal /*= true*/)
{
    string256 buf;
    strconcat(buf, path, ":title");
    InitStatic(xml_doc, buf, index, &pWnd->m_title, false);

    return InitFrameLine(xml_doc, path, index, &pWnd->m_frameline, fatal);
}

bool CUIXmlInitBase::InitCustomEdit(CUIXml& xml_doc, pcstr path, int index, CUICustomEdit* pWnd, bool fatal /*= true*/)
{
    if (!InitStatic(xml_doc, path, index, pWnd, fatal))
        return false;

    pWnd->InitCustomEdit(pWnd->GetWndPos(), pWnd->GetWndSize());

    string256 foo;
    strconcat(foo, path, ":text_color:e");
    if (xml_doc.NavigateToNode(foo, index))
    {
        const u32 color = GetColor(xml_doc, foo, index, 0x00);
        pWnd->TextItemControl()->SetTextColor(color);
    }

    int max_count = xml_doc.ReadAttribInt(path, index, "max_symb_count", 0);
    const bool num_only = (xml_doc.ReadAttribInt(path, index, "num_only", 0) == 1);
    const bool read_only = (xml_doc.ReadAttribInt(path, index, "read_only", 0) == 1);
    const bool file_name_mode = (xml_doc.ReadAttribInt(path, index, "file_name_mode", 0) == 1);

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

bool CUIXmlInitBase::InitEditBox(CUIXml& xml_doc, pcstr path, int index, CUIEditBox* pWnd, bool fatal /*= true*/)
{
    if (!InitCustomEdit(xml_doc, path, index, pWnd))
        return false;

    InitTexture(xml_doc, path, index, pWnd);
    InitOptionsItem(xml_doc, path, index, pWnd);

    return true;
}

//////////////////////////////////////////////////////////////////////////

bool CUIXmlInitBase::InitAnimatedStatic(CUIXml& xml_doc, pcstr path, int index, CUIAnimatedStatic* pWnd, bool fatal /*= true*/)
{
    if (!InitStatic(xml_doc, path, index, pWnd, fatal))
        return false;

    const float x = xml_doc.ReadAttribFlt(path, index, "x_offset", 0);
    const float y = xml_doc.ReadAttribFlt(path, index, "y_offset", 0);
    const u32 framesCount = static_cast<u32>(xml_doc.ReadAttribInt(path, index, "frames", 0));
    const u32 animDuration = static_cast<u32>(xml_doc.ReadAttribInt(path, index, "duration", 0));
    const u32 animCols = static_cast<u32>(xml_doc.ReadAttribInt(path, index, "columns", 0));
    const float frameWidth = xml_doc.ReadAttribFlt(path, index, "frame_width", 0);
    const float frameHeight = xml_doc.ReadAttribFlt(path, index, "frame_height", 0);
    const bool cyclic = !!xml_doc.ReadAttribInt(path, index, "cyclic", 0);
    const bool play = !!xml_doc.ReadAttribInt(path, index, "autoplay", 0);

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

bool CUIXmlInitBase::InitTexture(const CUIXml& xml_doc, pcstr path, int index, ITextureOwner* pWnd, bool fatal /*= true*/)
{
    bool result = true;

    string256 buf;
    pcstr texture = nullptr;
    pcstr shader = nullptr;
    strconcat(buf, path, ":texture");
    if (xml_doc.NavigateToNode(buf))
    {
        texture = xml_doc.Read(buf, index, nullptr);
        shader = xml_doc.ReadAttrib(buf, index, "shader", nullptr);
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

    const bool stretch_flag = xml_doc.ReadAttribInt(path, index, "stretch") ? true : false;
    pWnd->SetStretchTexture(stretch_flag);

    const u32 color = GetColor(xml_doc, buf, index, 0xff);
    pWnd->SetTextureColor(color);

    if (rect.width() != 0 && rect.height() != 0)
        pWnd->SetTextureRect(rect);

    return result;
}

bool CUIXmlInitBase::InitTextureOffset(const CUIXml& xml_doc, pcstr path, int index, CUIStatic* pWnd)
{
    string256 textureOffset;
    if (0 == xr_strcmp(path, ""))
        xr_strcpy(textureOffset, "texture_offset");
    else
        strconcat(textureOffset, path, ":texture_offset");

    const float x = xml_doc.ReadAttribFlt(textureOffset, index, "x");
    const float y = xml_doc.ReadAttribFlt(textureOffset, index, "y");

    pWnd->SetTextureOffset(x, y);

    return true;
}

bool CUIXmlInitBase::InitMultiTexture(const CUIXml& xml_doc, pcstr path, int index, CUI3tButton* pWnd)
{
    string256 buff;
    bool success = false;

    strconcat(buff, path, ":texture");
    shared_str texture = xml_doc.Read(buff, index, nullptr);

    if (texture.size() > 0)
    {
        pWnd->InitTexture(*texture);
        return true;
    }

    strconcat(buff, path, ":texture_e");
    texture = xml_doc.Read(buff, index, nullptr);
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

    strconcat(buff, path, ":texture_t");
    texture = xml_doc.Read(buff, index, nullptr);
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

    strconcat(buff, path, ":texture_d");
    texture = xml_doc.Read(buff, index, nullptr);
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

    strconcat(buff, path, ":texture_h");
    texture = xml_doc.Read(buff, index, nullptr);
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

bool CUIXmlInitBase::InitAlignment(const CUIXml& xml_doc, const char* path, int index, float& x, float& y, CUIWindow* pWnd)
{
    // Alignment: top: "t", right: "r", bottom: "b", left: "l", center: "c"
    const xr_string wnd_alignment = xml_doc.ReadAttrib(path, index, "alignment", "");

    switch (strhash(wnd_alignment))
    {
    case "r"_hash:
        pWnd->SetAlignment(waRight);
        break;
    case "l"_hash:
        pWnd->SetAlignment(waLeft);
        break;
    case "t"_hash:
        pWnd->SetAlignment(waTop);
        break;
    case "b"_hash:
        pWnd->SetAlignment(waBottom);
        break;
    case "c"_hash:
        pWnd->SetAlignment(waCenter);
        break;
    default:
        break;
    }

    // Alignment: right: "r", bottom: "b". Top, left - useless
    cpcstr alignStr = xml_doc.ReadAttrib(path, index, "align", "");

    bool result = false;

    switch (strhash(alignStr))
    {
    case "r"_hash:
        x = ApplyAlignX(x, alRight);
        result = true;
        break;
    case "b"_hash:
        y = ApplyAlignY(y, alBottom);
        result = true;
        break;
    case "c"_hash:
        ApplyAlign(x, y, alCenter);
        result = true;
        break;
    default:
        break;
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

    const auto num = uiXml.GetNodesNum("colors", 0, "color");

    for (size_t i = 0; i < num; ++i)
    {
        const shared_str name = uiXml.ReadAttrib("color", i, "name", "");
        const int r = uiXml.ReadAttribInt("color", i, "r", 0);
        const int g = uiXml.ReadAttribInt("color", i, "g", 0);
        const int b = uiXml.ReadAttribInt("color", i, "b", 0);
        const int a = uiXml.ReadAttribInt("color", i, "a", 255);

        (*m_pColorDefs)[name] = color_argb(a, r, g, b);
    }
}

bool CUIXmlInitBase::InitScrollView(CUIXml& xml_doc, pcstr path, int index, CUIScrollView* pWnd, bool fatal /*= true*/)
{
    if (!InitWindow(xml_doc, path, index, pWnd, fatal))
        return false;

    pWnd->SetRightIndention(xml_doc.ReadAttribFlt(path, index, "right_ident", 0.0f));
    pWnd->SetLeftIndention(xml_doc.ReadAttribFlt(path, index, "left_ident", 0.0f));
    pWnd->SetUpIndention(xml_doc.ReadAttribFlt(path, index, "top_indent", 0.0f));
    pWnd->SetDownIndention(xml_doc.ReadAttribFlt(path, index, "bottom_indent", 0.0f));

    const float vi = xml_doc.ReadAttribFlt(path, index, "vert_interval", 0.0f);
    pWnd->m_vertInterval = (vi);

    const bool bInverseDir = (1 == xml_doc.ReadAttribInt(path, index, "inverse_dir", 0));
    pWnd->m_flags.set(CUIScrollView::eInverseDir, bInverseDir);

    pWnd->SetScrollBarProfile(xml_doc.ReadAttrib(path, index, "scroll_profile", "default"));

    pWnd->InitScrollView();

    const bool bVertFlip = (1 == xml_doc.ReadAttribInt(path, index, "flip_vert", 0));
    pWnd->SetVertFlip(bVertFlip);

    bool b = (1 == xml_doc.ReadAttribInt(path, index, "always_show_scroll", 1));

    pWnd->SetFixedScrollBar(b);

    b = (1 == xml_doc.ReadAttribInt(path, index, "can_select", 0));

    pWnd->m_flags.set(CUIScrollView::eItemsSelectabe, b);

    /////////////////////////////////////////////////////////////////////
    const int tabsCount = (int)xml_doc.GetNodesNum(path, index, "text");

    const XML_NODE _stored_root = xml_doc.GetLocalRoot();
    xml_doc.SetLocalRoot(xml_doc.NavigateToNode(path, index));

    for (int i = 0; i < tabsCount; ++i)
    {
        auto* newText = xr_new<CUIStatic>("Text");
        InitText(xml_doc, "text", i, newText->TextItemControl());
        newText->SetTextComplexMode(true);
        newText->SetWidth(pWnd->GetDesiredChildWidth());
        newText->AdjustHeightToText();
        pWnd->AddWindow(newText, true);
    }
    xml_doc.SetLocalRoot(_stored_root);
    return true;
}

bool CUIXmlInitBase::InitListWnd(const CUIXml& xml_doc, pcstr path, int index, CUIListWnd* pWnd, bool fatal /*= true*/)
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
    const float item_height = xml_doc.ReadAttribFlt(path, index, "item_height");
    const int active_background = xml_doc.ReadAttribInt(path, index, "active_bg");

    // Init font from xml config file
    string256 buf;
    CGameFont* LocalFont = nullptr;
    u32 cl;

    const shared_str text_path = strconcat(buf, path, ":font");
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

    const bool bVertFlip = (1 == xml_doc.ReadAttribInt(path, index, "flip_vert", 0));
    pWnd->SetVertFlip(bVertFlip);

    return true;
}

bool CUIXmlInitBase::InitListBox(CUIXml& xml_doc, pcstr path, int index, CUIListBox* pWnd, bool fatal /*= true*/)
{
    if (!InitScrollView(xml_doc, path, index, pWnd))
        return false;

    string512 _path;
    u32 t_color;
    CGameFont* pFnt;
    strconcat(_path, path, ":font");
    InitFont(xml_doc, _path, index, t_color, pFnt);

    pWnd->SetTextColor(t_color);
    pWnd->SetFont(pFnt);

    const float h = xml_doc.ReadAttribFlt(path, index, "item_height", 20.0f);
    pWnd->SetItemHeight(h);
    return true;
}

bool CUIXmlInitBase::InitTrackBar(CUIXml& xml_doc, pcstr path, int index, CUITrackBar* pWnd, bool fatal /*= true*/)
{
    if (!InitWindow(xml_doc, path, 0, pWnd, fatal))
        return false;

    pWnd->InitTrackBar(pWnd->GetWndPos(), pWnd->GetWndSize());
    const int is_integer = xml_doc.ReadAttribInt(path, index, "is_integer", 0);
    pWnd->SetType(!is_integer);
    InitOptionsItem(xml_doc, path, 0, pWnd);

    const int invert = xml_doc.ReadAttribInt(path, index, "invert", 0);
    pWnd->SetInvert(!!invert);
    const float step = xml_doc.ReadAttribFlt(path, index, "step", 0.1f);
    pWnd->SetStep(step);

    if (!is_integer)
    {
        const float fmin = xml_doc.ReadAttribFlt(path, index, "min", 0.0f);
        const float fmax = xml_doc.ReadAttribFlt(path, index, "max", 0.0f);

        if (fmin != fmax)
        {
            pWnd->SetOptFBounds(fmin, fmax);
            pWnd->SetBoundReady(true);
        }
    }
    else
    {
        const int imin = xml_doc.ReadAttribInt(path, index, "min", 0);
        const int imax = xml_doc.ReadAttribInt(path, index, "max", 0);

        if (imin != imax)
        {
            pWnd->SetOptIBounds(imin, imax);
            pWnd->SetBoundReady(true);
        }
    }

    string512 buf;
    strconcat(buf, path, ":output_wnd");
    if (xml_doc.NavigateToNode(buf, index))
    {
        InitStatic(xml_doc, buf, index, pWnd->m_static);
        pWnd->m_static_format = xml_doc.ReadAttrib(buf, index, "format", nullptr);
        pWnd->m_static->Enable(true);
    }

    return true;
}

bool CUIXmlInitBase::InitComboBox(CUIXml& xml_doc, pcstr path, int index, CUIComboBox* pWnd)
{
    u32 color;
    CGameFont* pFont;

    pWnd->SetListLength(xml_doc.ReadAttribInt(path, index, "list_length", 4));

    InitWindow(xml_doc, path, index, pWnd);
    pWnd->InitComboBox(pWnd->GetWndPos(), pWnd->GetWidth());
    InitOptionsItem(xml_doc, path, index, pWnd);

    const bool b = (1 == xml_doc.ReadAttribInt(path, index, "always_show_scroll", 1));

    pWnd->m_list_box.SetFixedScrollBar(b);

    string512 _path;
    strconcat(_path, path, ":list_font");
    InitFont(xml_doc, _path, index, color, pFont);
    //.	pWnd->SetFont				(pFont);
    pWnd->m_list_box.SetFont(pFont);
    pWnd->m_list_box.SetTextColor(color);

    strconcat(_path, path, ":text_color:e");
    if (xml_doc.NavigateToNode(_path, index))
    {
        color = GetColor(xml_doc, _path, index, 0x00);
        pWnd->SetTextColor(color);
    }

    strconcat(_path, path, ":text_color:d");
    if (xml_doc.NavigateToNode(_path, index))
    {
        color = GetColor(xml_doc, _path, index, 0x00);
        pWnd->SetTextColorD(color);
    }

    return true;
}

void CUIXmlInitBase::AssignColor(pcstr name, u32 clr) { (*m_pColorDefs)[name] = clr; }
u32 CUIXmlInitBase::GetColor(const CUIXml& xml_doc, pcstr path, int index, u32 def_clr)
{
    if (cpcstr clr_def = xml_doc.ReadAttrib(path, index, "color", nullptr))
    {
        VERIFY(GetColorDefs()->find(clr_def) != GetColorDefs()->end());
        return (*m_pColorDefs)[clr_def];
    }
    else
    {
        const int r = xml_doc.ReadAttribInt(path, index, "r", def_clr);
        const int g = xml_doc.ReadAttribInt(path, index, "g", def_clr);
        const int b = xml_doc.ReadAttribInt(path, index, "b", def_clr);
        const int a = xml_doc.ReadAttribInt(path, index, "a", 0xff);
        return color_argb(a, r, g, b);
    }
}
