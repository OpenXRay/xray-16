#include "pch.hpp"
#include "UIFrameLineWnd.h"
#include "XML/UITextureMaster.h"

CUIFrameLineWnd::CUIFrameLineWnd() : bHorizontal(true), m_bTextureVisible(false)
{
    m_texture_color = color_argb(255, 255, 255, 255);
}

void CUIFrameLineWnd::InitFrameLineWnd(LPCSTR base_name, Fvector2 pos, Fvector2 size, bool horizontal)
{
    InitFrameLineWnd(pos, size, horizontal);
    InitTexture(base_name, "hud" DELIMITER "default");
}

void CUIFrameLineWnd::InitFrameLineWnd(Fvector2 pos, Fvector2 size, bool horizontal)
{
    inherited::SetWndPos(pos);
    inherited::SetWndSize(size);

    bHorizontal = horizontal;
}

void CUIFrameLineWnd::InitTexture(LPCSTR texture, LPCSTR sh_name)
{
    m_bTextureVisible = true;
    dbg_tex_name = texture;
    string256 buf;
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_back"), sh_name, m_shader, m_tex_rect[flBack]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_b"), sh_name, m_shader, m_tex_rect[flFirst]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_e"), sh_name, m_shader, m_tex_rect[flSecond]);
    if (bHorizontal)
    {
        R_ASSERT2(fsimilar(m_tex_rect[flFirst].height(), m_tex_rect[flSecond].height()), texture);
        R_ASSERT2(fsimilar(m_tex_rect[flFirst].height(), m_tex_rect[flBack].height()), texture);
    }
    else
    {
        R_ASSERT2(fsimilar(m_tex_rect[flFirst].width(), m_tex_rect[flSecond].width()), texture);
        R_ASSERT2(fsimilar(m_tex_rect[flFirst].width(), m_tex_rect[flBack].width()), texture);
    }
}

void CUIFrameLineWnd::Draw()
{
    if (m_bTextureVisible)
        DrawElements();

    inherited::Draw();
}

static Fvector2 pt_offset = {-0.5f, -0.5f};

void draw_rect(Fvector2 LTp, Fvector2 RBp, Fvector2 LTt, Fvector2 RBt, u32 clr, Fvector2 const& ts)
{
    UI().AlignPixel(LTp.x);
    UI().AlignPixel(LTp.y);
    LTp.add(pt_offset);
    UI().AlignPixel(RBp.x);
    UI().AlignPixel(RBp.y);
    RBp.add(pt_offset);
    LTt.div(ts);
    RBt.div(ts);

    GEnv.UIRender->PushPoint(LTp.x, LTp.y, 0, clr, LTt.x, LTt.y);
    GEnv.UIRender->PushPoint(RBp.x, RBp.y, 0, clr, RBt.x, RBt.y);
    GEnv.UIRender->PushPoint(LTp.x, RBp.y, 0, clr, LTt.x, RBt.y);

    GEnv.UIRender->PushPoint(LTp.x, LTp.y, 0, clr, LTt.x, LTt.y);
    GEnv.UIRender->PushPoint(RBp.x, LTp.y, 0, clr, RBt.x, LTt.y);
    GEnv.UIRender->PushPoint(RBp.x, RBp.y, 0, clr, RBt.x, RBt.y);
}

void CUIFrameLineWnd::DrawElements()
{
    GEnv.UIRender->SetShader(*m_shader);

    Fvector2 ts;
    GEnv.UIRender->GetActiveTextureResolution(ts);

    Frect rect;
    GetAbsoluteRect(rect);
    UI().ClientToScreenScaled(rect.lt);
    UI().ClientToScreenScaled(rect.rb);

    float back_len = 0.0f;
    u32 prim_count = 6 * 2; // first&second
    if (bHorizontal)
    {
        back_len = rect.width() - m_tex_rect[flFirst].width() - m_tex_rect[flSecond].width();
        if (back_len < 0.0f)
            rect.x2 -= back_len;

        if (back_len > 0.0f)
            prim_count += 6 * iCeil(back_len / m_tex_rect[flBack].width());
    }
    else
    {
        back_len = rect.height() - m_tex_rect[flFirst].height() - m_tex_rect[flSecond].height();
        if (back_len < 0)
            rect.y2 -= back_len;

        if (back_len > 0.0f)
            prim_count += 6 * iCeil(back_len / m_tex_rect[flBack].height());
    }

    GEnv.UIRender->StartPrimitive(prim_count, IUIRender::ptTriList, UI().m_currentPointType);

    for (int i = 0; i < flMax; ++i)
    {
        Fvector2 LTt, RBt;
        Fvector2 LTp, RBp;
        int counter = 0;

        while (inc_pos(rect, counter, i, LTp, RBp, LTt, RBt))
        {
            draw_rect(LTp, RBp, LTt, RBt, m_texture_color, ts);
            ++counter;
        };
    }
    GEnv.UIRender->FlushPrimitive();
}

bool CUIFrameLineWnd::inc_pos(
    Frect& rect, int counter, int i, Fvector2& LTp, Fvector2& RBp, Fvector2& LTt, Fvector2& RBt)
{
    if (i == flFirst || i == flSecond)
    {
        if (counter != 0)
            return false;

        LTt = m_tex_rect[i].lt;
        RBt = m_tex_rect[i].rb;

        LTp = rect.lt;

        RBp = rect.lt;
        RBp.x += m_tex_rect[i].width();
        RBp.y += m_tex_rect[i].height();
    }
    else // i==flBack
    {
        if ((bHorizontal && rect.lt.x + m_tex_rect[flSecond].width() + EPS_L >= rect.rb.x) ||
            (!bHorizontal && rect.lt.y + m_tex_rect[flSecond].height() + EPS_L >= rect.rb.y))
            return false;

        LTt = m_tex_rect[i].lt;
        LTp = rect.lt;

        bool b_draw_reminder = (bHorizontal) ?
            (rect.lt.x + m_tex_rect[flBack].width() > rect.rb.x - m_tex_rect[flSecond].width()) :
            (rect.lt.y + m_tex_rect[flBack].height() > rect.rb.y - m_tex_rect[flSecond].height());
        if (b_draw_reminder)
        { // draw reminder
            float rem_len = (bHorizontal) ? rect.rb.x - m_tex_rect[flSecond].width() - rect.lt.x :
                                            rect.rb.y - m_tex_rect[flSecond].height() - rect.lt.y;

            if (bHorizontal)
            {
                RBt.y = m_tex_rect[i].rb.y;
                RBt.x = m_tex_rect[i].lt.x + rem_len;

                RBp = rect.lt;
                RBp.x += rem_len;
                RBp.y += m_tex_rect[i].height();
            }
            else
            {
                RBt.y = m_tex_rect[i].lt.y + rem_len;
                RBt.x = m_tex_rect[i].rb.x;

                RBp = rect.lt;
                RBp.x += m_tex_rect[i].width();
                RBp.y += rem_len;
            }
        }
        else
        { // draw full element
            RBt = m_tex_rect[i].rb;

            RBp = rect.lt;
            RBp.x += m_tex_rect[i].width();
            RBp.y += m_tex_rect[i].height();
        }
    }

    // stretch always
    if (bHorizontal)
        RBp.y = rect.rb.y;
    else
        RBp.x = rect.rb.x;

    if (bHorizontal)
        rect.lt.x = RBp.x;
    else
        rect.lt.y = RBp.y;
    return true;
}
