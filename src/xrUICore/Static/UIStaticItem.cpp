#include "pch.hpp"
#include "UIStaticItem.h"

XRUICORE_API void CreateUIGeom() { GEnv.UIRender->CreateUIGeom(); }
XRUICORE_API void DestroyUIGeom() { GEnv.UIRender->DestroyUIGeom(); }
CUIStaticItem::CUIStaticItem()
{
    uFlags.zero();
    vSize.set(0, 0);
    TextureRect.set(0, 0, 0, 0);
    vHeadingPivot.set(0, 0);
    vHeadingOffset.set(0, 0);
    dwColor = 0xffffffff;
}

void CUIStaticItem::ResetHeadingPivot()
{
    uFlags.set(flValidHeadingPivot, FALSE);
    uFlags.set(flFixedLTWhileHeading, FALSE);
}

void CUIStaticItem::SetHeadingPivot(const Fvector2& p, const Fvector2& offset, bool fixedLT)
{
    vHeadingPivot = p;
    vHeadingOffset = offset;
    uFlags.set(flValidHeadingPivot, TRUE);
    if (fixedLT)
        uFlags.set(flFixedLTWhileHeading, TRUE);
    else
        uFlags.set(flFixedLTWhileHeading, FALSE);
}

void CUIStaticItem::RenderInternal(const Fvector2& in_pos)
{
    Fvector2 pos;
    UI().ClientToScreenScaled(pos, in_pos.x, in_pos.y);
    UI().AlignPixel(pos.x);
    UI().AlignPixel(pos.y);

    Fvector2 ts;
    GEnv.UIRender->GetActiveTextureResolution(ts);

    if (!uFlags.test(flValidSize))
        SetSize(ts);

    if (!uFlags.test(flValidTextureRect))
        SetTextureRect(Frect().set(0, 0, ts.x, ts.y));

    Fvector2 LTp, RBp;
    Fvector2 LTt, RBt;
    //координаты на экране в пикселях
    LTp.set(pos);

    UI().ClientToScreenScaled(RBp, vSize.x, vSize.y);
    RBp.add(pos);

    //текстурные координаты
    LTt.set(TextureRect.x1 / ts.x, TextureRect.y1 / ts.y);
    RBt.set(TextureRect.x2 / ts.x, TextureRect.y2 / ts.y);

    float offset = -0.5f;
    if (UI().m_currentPointType == IUIRender::pttLIT)
        offset = 0.0f;

    // clip poly
    sPoly2D S;
    S.resize(4);

    LTp.x += offset;
    LTp.y += offset;
    RBp.x += offset;
    RBp.y += offset;

    S[0].set(LTp.x, LTp.y, LTt.x, LTt.y); // LT
    S[1].set(RBp.x, LTp.y, RBt.x, LTt.y); // RT
    S[2].set(RBp.x, RBp.y, RBt.x, RBt.y); // RB
    S[3].set(LTp.x, RBp.y, LTt.x, RBt.y); // LB

    sPoly2D D;
    sPoly2D* R = NULL;

    if (UI().m_currentPointType != IUIRender::pttLIT)
        R = UI().ScreenFrustum().ClipPoly(S, D);
    else
    {
        R = UI().ScreenFrustumLIT().ClipPoly(S, D);
    }

    if (R && R->size())
    {
        for (u32 k = 0; k < R->size() - 2; ++k)
        {
            GEnv.UIRender->PushPoint(
                (*R)[0 + 0].pt.x, (*R)[0 + 0].pt.y, 0, dwColor, (*R)[0 + 0].uv.x, (*R)[0 + 0].uv.y);
            GEnv.UIRender->PushPoint(
                (*R)[k + 1].pt.x, (*R)[k + 1].pt.y, 0, dwColor, (*R)[k + 1].uv.x, (*R)[k + 1].uv.y);
            GEnv.UIRender->PushPoint(
                (*R)[k + 2].pt.x, (*R)[k + 2].pt.y, 0, dwColor, (*R)[k + 2].uv.x, (*R)[k + 2].uv.y);
        }
    }
}

void CUIStaticItem::RenderInternal(float angle)
{
    Fvector2 ts;
    Fvector2 hp;

    GEnv.UIRender->GetActiveTextureResolution(ts);
    hp.set(0.5f / ts.x, 0.5f / ts.y);

    if (!uFlags.test(flValidSize))
        SetSize(ts);

    if (!uFlags.test(flValidTextureRect))
        SetTextureRect(Frect().set(0, 0, ts.x, ts.y));

    Fvector2 pivot, offset, SZ;
    SZ.set(vSize);

    float cosA = _cos(angle);
    float sinA = _sin(angle);

    // Rotation
    if (!uFlags.test(flValidHeadingPivot))
        pivot.set(vSize.x / 2.f, vSize.y / 2.f);
    else
        pivot.set(vHeadingPivot.x, vHeadingPivot.y);

    offset.set(vPos);
    offset.add(vHeadingOffset);

    Fvector2 LTt, RBt;
    LTt.set(TextureRect.x1 / ts.x + hp.x, TextureRect.y1 / ts.y + hp.y);
    RBt.set(TextureRect.x2 / ts.x + hp.x, TextureRect.y2 / ts.y + hp.y);

    float kx = UI().get_current_kx();

    // clip poly
    sPoly2D S;
    S.resize(4);
    // LT
    S[0].set(0.f, 0.f, LTt.x, LTt.y);
    S[0].rotate_pt(pivot, cosA, sinA, kx);
    S[0].pt.add(offset);

    // RT
    S[1].set(SZ.x, 0.f, RBt.x, LTt.y);
    S[1].rotate_pt(pivot, cosA, sinA, kx);
    S[1].pt.add(offset);
    // RB
    S[2].set(SZ.x, SZ.y, RBt.x, RBt.y);
    S[2].rotate_pt(pivot, cosA, sinA, kx);
    S[2].pt.add(offset);
    // LB
    S[3].set(0.f, SZ.y, LTt.x, RBt.y);
    S[3].rotate_pt(pivot, cosA, sinA, kx);
    S[3].pt.add(offset);

    for (int i = 0; i < 4; ++i)
        UI().ClientToScreenScaled(S[i].pt);

    sPoly2D D;
    sPoly2D* R = UI().ScreenFrustum().ClipPoly(S, D);
    if (R && R->size())
    {
        for (u32 k = 0; k < R->size() - 2; k++)
        {
            GEnv.UIRender->PushPoint(
                (*R)[0 + 0].pt.x, (*R)[0 + 0].pt.y, 0, dwColor, (*R)[0 + 0].uv.x, (*R)[0 + 0].uv.y);
            GEnv.UIRender->PushPoint(
                (*R)[k + 1].pt.x, (*R)[k + 1].pt.y, 0, dwColor, (*R)[k + 1].uv.x, (*R)[k + 1].uv.y);
            GEnv.UIRender->PushPoint(
                (*R)[k + 2].pt.x, (*R)[k + 2].pt.y, 0, dwColor, (*R)[k + 2].uv.x, (*R)[k + 2].uv.y);
        }
    }
}

//---from static-item

void CUIStaticItem::Render()
{
    VERIFY(g_bRendering);
    GEnv.UIRender->SetShader(*hShader);
    GEnv.UIRender->StartPrimitive(8, IUIRender::ptTriList, UI().m_currentPointType);
    RenderInternal(vPos);
    GEnv.UIRender->FlushPrimitive();
}

void CUIStaticItem::Render(float angle)
{
    VERIFY(g_bRendering);

    GEnv.UIRender->SetShader(*hShader);
    GEnv.UIRender->StartPrimitive(32, IUIRender::ptTriList, UI().m_currentPointType);
    RenderInternal(angle);
    GEnv.UIRender->FlushPrimitive();
}

void CUIStaticItem::CreateShader(LPCSTR tex, LPCSTR sh)
{
    hShader->create(sh, tex);

#ifdef DEBUG
    dbg_tex_name = tex;
#endif
    uFlags.set(flValidSize, FALSE);
    uFlags.set(flValidTextureRect, FALSE);
}

void CUIStaticItem::Init(LPCSTR tex, LPCSTR sh, float left, float top)
{
    uFlags.set(flValidSize, FALSE);
    CreateShader(tex, sh);
    SetPos(left, top);
}
