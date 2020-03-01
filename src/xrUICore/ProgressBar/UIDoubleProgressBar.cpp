#include "pch.hpp"
#include "UIDoubleProgressBar.h"
#include "XML/UIXmlInitBase.h"

CUIDoubleProgressBar::CUIDoubleProgressBar()
{
    AttachChild(&m_progress_one);
    AttachChild(&m_progress_two);
}

CUIDoubleProgressBar::~CUIDoubleProgressBar() {}
void CUIDoubleProgressBar::InitFromXml(CUIXml& xml_doc, LPCSTR path)
{
    CUIXmlInitBase::InitProgressBar(xml_doc, path, 0, &m_progress_one);
    CUIXmlInitBase::InitProgressBar(xml_doc, path, 0, &m_progress_two);

    string256 buf;
    strconcat(sizeof(buf), buf, path, ":color_less");
    m_less_color = CUIXmlInitBase::GetColor(xml_doc, buf, 0, color_rgba(255, 0, 0, 255));
    strconcat(sizeof(buf), buf, path, ":color_more");
    m_more_color = CUIXmlInitBase::GetColor(xml_doc, buf, 0, color_rgba(0, 255, 0, 255));

    m_progress_one.SetRange(0.0f, 100.0f);
    m_progress_two.SetRange(0.0f, 100.0f);

    m_progress_one.ShowBackground(true);
    m_progress_two.ShowBackground(false);
}

void CUIDoubleProgressBar::SetTwoPos(float cur_value, float compare_value)
{
    if (cur_value < compare_value) // red
    {
        m_progress_one.m_UIProgressItem.SetTextureColor(m_less_color);
        m_progress_one.SetProgressPos(compare_value);
        m_progress_two.SetProgressPos(cur_value);
    }
    else if (cur_value > compare_value) // green
    {
        m_progress_one.m_UIProgressItem.SetTextureColor(m_more_color);
        m_progress_one.SetProgressPos(cur_value);
        m_progress_two.SetProgressPos(compare_value);
    }
    else
    {
        m_progress_one.m_UIProgressItem.SetTextureColor(m_progress_two.m_UIProgressItem.GetTextureColor());
        m_progress_one.SetProgressPos(cur_value);
        m_progress_two.SetProgressPos(cur_value);
    }
}
