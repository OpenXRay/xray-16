#include "pch.hpp"
#include "UIDoubleProgressBar.h"
#include "XML/UIXmlInitBase.h"

CUIDoubleProgressBar::CUIDoubleProgressBar() : CUIWindow("CUIDoubleProgressBar")
{
    AttachChild(&m_progress_one);
    AttachChild(&m_progress_two);
}

void CUIDoubleProgressBar::InitFromXml(CUIXml& xml_doc, LPCSTR path)
{
    CUIXmlInitBase::InitProgressBar(xml_doc, path, 0, &m_progress_one);
    CUIXmlInitBase::InitProgressBar(xml_doc, path, 0, &m_progress_two);

    u32 less_color = color_rgba(255, 0, 0, 255);
    u32 more_color = color_rgba(0, 255, 0, 255);

    if (m_progress_one.m_bUseColor)
    {
        less_color = m_progress_one.m_minColor.get();
        more_color = m_progress_one.m_maxColor.get();
    }

    string256 buf;
    strconcat(sizeof(buf), buf, path, ":color_less");
    m_less_color = CUIXmlInitBase::GetColor(xml_doc, buf, 0, less_color);
    strconcat(sizeof(buf), buf, path, ":color_more");
    m_more_color = CUIXmlInitBase::GetColor(xml_doc, buf, 0, more_color);

    m_progress_one.SetRange(0.0f, 100.0f);
    m_progress_two.SetRange(0.0f, 100.0f);

    m_progress_one.ShowBackground(true);
    m_progress_two.ShowBackground(false);
}

void CUIDoubleProgressBar::SetTwoPos(float cur_value, float compare_value)
{
    if (cur_value < compare_value) // red
    {
        m_progress_one.SetProgressPos(compare_value);
        m_progress_two.SetProgressPos(cur_value);
        m_progress_one.m_UIProgressItem.SetTextureColor(m_less_color);
    }
    else if (cur_value > compare_value) // green
    {
        m_progress_one.SetProgressPos(cur_value);
        m_progress_two.SetProgressPos(compare_value);
        m_progress_one.m_UIProgressItem.SetTextureColor(m_more_color);
    }
    else
    {
        m_progress_one.m_UIProgressItem.SetTextureColor(m_progress_two.m_UIProgressItem.GetTextureColor());
        m_progress_one.SetProgressPos(cur_value);
        m_progress_two.SetProgressPos(cur_value);
    }
}

void CUIDoubleProgressBar::FillDebugInfo()
{
#ifndef MASTER_GOLD
    CUIWindow::FillDebugInfo();

    if (!ImGui::CollapsingHeader(CUIDoubleProgressBar::GetDebugType()))
        return;

    Fcolor color = m_less_color;
    if (ImGui::ColorEdit4("Less color", reinterpret_cast<float*>(&color)))
        m_less_color = color.get();

    color = m_more_color;
    if (ImGui::ColorEdit4("More color", reinterpret_cast<float*>(&color)))
        m_less_color = color.get();
#endif
}
