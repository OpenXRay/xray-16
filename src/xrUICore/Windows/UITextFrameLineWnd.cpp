#include "pch.hpp"
#include "UITextFrameLineWnd.h"

CUITextFrameLineWnd::CUITextFrameLineWnd()
    : CUIWindow(CUITextFrameLineWnd::GetDebugType()),
      m_frameline("Frameline"), m_title("Title")
{
    AttachChild(&m_title);
    AttachChild(&m_frameline);
}
