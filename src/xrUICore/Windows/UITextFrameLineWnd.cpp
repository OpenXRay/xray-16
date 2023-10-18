#include "pch.hpp"
#include "UITextFrameLineWnd.h"
#include "XML/UITextureMaster.h"

CUITextFrameLineWnd::CUITextFrameLineWnd()
    : CUIWindow(CUITextFrameLineWnd::GetDebugType()),
      bHorizontal(true), m_frameline("Frameline"), m_title("Title")
{
    AttachChild(&m_title);
    AttachChild(&m_frameline);
}

void CUITextFrameLineWnd::Init(pcstr baseTexture, Fvector2 pos, Fvector2 size, bool horizontal)
{
    m_frameline.InitFrameLineWnd(baseTexture, pos, size, horizontal);

    m_title.SetWndPos({ 0.0f, 0.0f });
    if (horizontal)
        m_title.SetWndSize({ size.x, 50 });
    else
        m_title.SetWndSize({ 50, size.y });
}

void CUITextFrameLineWnd::InitTexture(pcstr texture, bool horizontal)
{
    bHorizontal = horizontal;
    m_frameline.InitFrameLineWnd(texture, GetWndPos(), GetWndSize(), horizontal);
}
