#include "pch.hpp"
#include "UIEditBoxEx.h"
#include "Windows/UIFrameWindow.h"

CUIEditBoxEx::CUIEditBoxEx()
{
    m_pFrameWindow = xr_new<CUIFrameWindow>();
    AttachChild(m_pFrameWindow);

    TextItemControl()->SetTextComplexMode(true);
}

CUIEditBoxEx::~CUIEditBoxEx() { xr_delete(m_pFrameWindow); }
void CUIEditBoxEx::InitCustomEdit(Fvector2 pos, Fvector2 size)
{
    m_pFrameWindow->SetWndSize(size);
    m_pFrameWindow->SetWndPos(Fvector2().set(0, 0));
    CUICustomEdit::InitCustomEdit(pos, size);
}

bool CUIEditBoxEx::InitTextureEx(pcstr texture, pcstr shader, bool fatal /*= true*/)
{
    return m_pFrameWindow->InitTextureEx(texture, shader, fatal);
}
bool CUIEditBoxEx::InitTexture(pcstr texture, bool fatal /*= true*/)
{
    return m_pFrameWindow->InitTexture(texture, fatal);
}
