#include "pch.hpp"
#include "UIEditBoxEx.h"
#include "Windows/UIFrameWindow.h"

CUIEditBoxEx::CUIEditBoxEx()
{
    m_pFrameWindow = new CUIFrameWindow();
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

void CUIEditBoxEx::InitTextureEx(LPCSTR texture, LPCSTR shader) { m_pFrameWindow->InitTextureEx(texture, shader); }
void CUIEditBoxEx::InitTexture(LPCSTR texture) { m_pFrameWindow->InitTexture(texture); }
