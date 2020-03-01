#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"

class XRUICORE_API CUIDoubleProgressBar : public CUIWindow
{
public: // func
    CUIDoubleProgressBar();
    virtual ~CUIDoubleProgressBar();

    void InitFromXml(CUIXml& xml_doc, LPCSTR path);
    void SetTwoPos(float cur_value, float compare_value);

protected:
    CUIProgressBar m_progress_one;
    CUIProgressBar m_progress_two;

    u32 m_less_color; // red
    u32 m_more_color; // green

}; // class CUIDoubleProgressBar
