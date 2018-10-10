#include "pch.hpp"
#include "UIFrameWindow.h"
#include "XML/UITextureMaster.h"
#include "Static/UIStatic.h"

void draw_rect(Fvector2 LTp, Fvector2 RBp, Fvector2 LTt, Fvector2 RBt, u32 clr, Fvector2 const& ts);

CUIFrameWindow::CUIFrameWindow() : m_bTextureVisible(false) { m_texture_color = color_argb(255, 255, 255, 255); }
void CUIFrameWindow::SetWndSize(const Fvector2& sz)
{
    Fvector2 size = sz;
    Fvector2 size_test = sz;
    UI().ClientToScreenScaled(size_test);

    if (m_bTextureVisible)
    { // fit to min size
        Fvector2 min_size;
        min_size.x = m_tex_rect[fmLT].width() + m_tex_rect[fmRT].width();
        min_size.y = m_tex_rect[fmLT].height() + m_tex_rect[fmLB].height();

        if (size_test.x < min_size.x)
        {
            UI().ClientToScreenScaledWidth(min_size.x);
            size.x = min_size.x;
        }
        if (size_test.y < min_size.y)
        {
            UI().ClientToScreenScaledHeight(min_size.y);
            size.y = min_size.y;
        }
    }

    inherited::SetWndSize(size);
}

void CUIFrameWindow::InitTextureEx(LPCSTR texture, LPCSTR sh_name)
{
    dbg_tex_name = texture;
    m_bTextureVisible = true;
    string256 buf;
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_back"), sh_name, m_shader, m_tex_rect[fmBK]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_l"), sh_name, m_shader, m_tex_rect[fmL]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_r"), sh_name, m_shader, m_tex_rect[fmR]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_t"), sh_name, m_shader, m_tex_rect[fmT]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_b"), sh_name, m_shader, m_tex_rect[fmB]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_lt"), sh_name, m_shader, m_tex_rect[fmLT]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_rb"), sh_name, m_shader, m_tex_rect[fmRB]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_rt"), sh_name, m_shader, m_tex_rect[fmRT]);
    CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_lb"), sh_name, m_shader, m_tex_rect[fmLB]);

    R_ASSERT2(fsimilar(m_tex_rect[fmLT].height(), m_tex_rect[fmT].height()), texture);
    R_ASSERT2(fsimilar(m_tex_rect[fmLT].height(), m_tex_rect[fmRT].height()), texture);
    //	R_ASSERT2(fsimilar(m_tex_rect[fmL].height(), m_tex_rect[fmBK].height()),texture );
    R_ASSERT2(fsimilar(m_tex_rect[fmL].height(), m_tex_rect[fmR].height()), texture);
    R_ASSERT2(fsimilar(m_tex_rect[fmLB].height(), m_tex_rect[fmB].height()), texture);
    R_ASSERT2(fsimilar(m_tex_rect[fmLB].height(), m_tex_rect[fmRB].height()), texture);

    R_ASSERT2(fsimilar(m_tex_rect[fmLT].width(), m_tex_rect[fmL].width()), texture);
    R_ASSERT2(fsimilar(m_tex_rect[fmLT].width(), m_tex_rect[fmLB].width()), texture);

    //	R_ASSERT2(fsimilar(m_tex_rect[fmT].width(), m_tex_rect[fmBK].width()),texture );
    R_ASSERT2(fsimilar(m_tex_rect[fmT].width(), m_tex_rect[fmB].width()), texture);

    R_ASSERT2(fsimilar(m_tex_rect[fmRT].width(), m_tex_rect[fmR].width()), texture);
    R_ASSERT2(fsimilar(m_tex_rect[fmRT].width(), m_tex_rect[fmRB].width()), texture);
}

void CUIFrameWindow::InitTexture(LPCSTR texture) { InitTextureEx(texture, "hud" DELIMITER "default"); }
void CUIFrameWindow::Draw()
{
    if (m_bTextureVisible)
        DrawElements();

    inherited::Draw();
}

void CUIFrameWindow::DrawElements()
{
    GEnv.UIRender->SetShader(*m_shader);

    Fvector2 ts;
    GEnv.UIRender->GetActiveTextureResolution(ts);

    Frect rect;
    GetAbsoluteRect(rect);
    UI().ClientToScreenScaled(rect.lt);
    UI().ClientToScreenScaled(rect.rb);

    Fvector2 back_len = {0.0f, 0.0f};
    u32 rect_count = 4; // lt+rt+lb+rb
    back_len.x = rect.width() - m_tex_rect[fmLT].width() - m_tex_rect[fmRT].width();
    back_len.y = rect.height() - m_tex_rect[fmLT].height() - m_tex_rect[fmRB].height();
    R_ASSERT(back_len.x + EPS_L >= 0.0f && back_len.y + EPS_L >= 0.0f);

    u32 cnt = 0;
    if (back_len.x > 0.0f) // top+bottom
        cnt = 2 * iCeil(back_len.x / m_tex_rect[fmT].width());
    rect_count += cnt;

    if (back_len.y > 0.0f) // left+right
        cnt = 2 * iCeil(back_len.y / m_tex_rect[fmL].height());
    rect_count += cnt;

    if (back_len.x > 0.0f && back_len.y > 0.0f) // back
        cnt = iCeil(back_len.x / m_tex_rect[fmBK].width()) * iCeil(back_len.y / m_tex_rect[fmBK].height());

    rect_count += cnt;

    rect_count *= 6;

    GEnv.UIRender->StartPrimitive(rect_count, IUIRender::ptTriList, UI().m_currentPointType);

    Fvector2 LTt, RBt;
    Fvector2 LTp, RBp;

    Frect tmp = rect;
    get_points(tmp, fmLT, LTp, RBp, LTt, RBt);
    draw_rect(LTp, RBp, LTt, RBt, m_texture_color, ts);

    tmp.lt.x = rect.lt.x;
    tmp.lt.y = rect.rb.y - m_tex_rect[fmLB].height();
    get_points(tmp, fmLB, LTp, RBp, LTt, RBt);
    draw_rect(LTp, RBp, LTt, RBt, m_texture_color, ts);

    tmp.lt.x = rect.rb.x - m_tex_rect[fmRT].width();
    tmp.lt.y = rect.lt.y;
    ;
    get_points(tmp, fmRT, LTp, RBp, LTt, RBt);
    draw_rect(LTp, RBp, LTt, RBt, m_texture_color, ts);

    tmp.lt.x = rect.rb.x - m_tex_rect[fmRB].width();
    tmp.lt.y = rect.rb.y - m_tex_rect[fmRB].height();
    get_points(tmp, fmRB, LTp, RBp, LTt, RBt);
    draw_rect(LTp, RBp, LTt, RBt, m_texture_color, ts);

    if (back_len.x > 0.0f)
    {
        tmp.lt = rect.lt;
        tmp.lt.x += m_tex_rect[fmLT].width();
        tmp.rb.x = rect.rb.x - m_tex_rect[fmRT].width();
        tmp.rb.y = rect.lt.y + m_tex_rect[fmT].height();
        draw_tile_line(tmp, fmT, true, ts);

        tmp.lt.x = rect.lt.x + m_tex_rect[fmLT].width();
        tmp.lt.y = rect.rb.y - m_tex_rect[fmB].height();
        tmp.rb.x = rect.rb.x - m_tex_rect[fmRT].width();
        tmp.rb.y = rect.rb.y;
        draw_tile_line(tmp, fmB, true, ts);
    }

    if (back_len.y > 0.0f)
    {
        tmp.lt = rect.lt;
        tmp.lt.y += m_tex_rect[fmLT].height();
        tmp.rb.x = rect.lt.x + m_tex_rect[fmL].width();
        tmp.rb.y = rect.rb.y - m_tex_rect[fmLB].height();
        draw_tile_line(tmp, fmL, false, ts);

        tmp.lt.x = rect.rb.x - m_tex_rect[fmR].width();
        tmp.lt.y = rect.lt.y + m_tex_rect[fmRT].height();
        ;
        tmp.rb.x = rect.rb.x;
        tmp.rb.y = rect.rb.y - m_tex_rect[fmRB].height();
        draw_tile_line(tmp, fmR, false, ts);
    }

    if (back_len.x > 0.0f && back_len.y > 0.0f)
    {
        tmp.lt.x = rect.lt.x + m_tex_rect[fmLT].width();
        tmp.lt.y = rect.lt.y + m_tex_rect[fmLT].height();
        tmp.rb.x = rect.rb.x - m_tex_rect[fmRB].width();
        tmp.rb.y = rect.rb.y - m_tex_rect[fmRB].height();
        draw_tile_rect(tmp, fmBK, ts);
    }

    GEnv.UIRender->FlushPrimitive();
}

bool CUIFrameWindow::get_points(Frect const& r, int i, Fvector2& LTp, Fvector2& RBp, Fvector2& LTt, Fvector2& RBt)
{
    LTt = m_tex_rect[i].lt;
    RBt = m_tex_rect[i].rb;

    LTp = r.lt;
    RBp = r.lt;
    RBp.x += m_tex_rect[i].width();
    RBp.y += m_tex_rect[i].height();

    float rem_x = r.width() - m_tex_rect[i].width();
    float rem_y = r.height() - m_tex_rect[i].height();
    if (rem_x < 0.0f)
    {
        RBt.x += rem_x;
        RBp.x += rem_x;
    }
    if (rem_y < 0.0f)
    {
        RBt.y += rem_y;
        RBp.y += rem_y;
    }

    return true;
}

void CUIFrameWindow::draw_tile_line(Frect rect, int i, bool b_horz, Fvector2 const& ts)
{
    Fvector2 LTt, RBt;
    Fvector2 LTp, RBp;

    if (b_horz)
    {
        while (rect.lt.x + EPS_L < rect.rb.x)
        {
            get_points(rect, i, LTp, RBp, LTt, RBt);
            rect.lt.x = RBp.x;
            draw_rect(LTp, RBp, LTt, RBt, m_texture_color, ts);
        }
    }
    else
    {
        while (rect.lt.y + EPS_L < rect.rb.y)
        {
            get_points(rect, i, LTp, RBp, LTt, RBt);
            rect.lt.y = RBp.y;
            draw_rect(LTp, RBp, LTt, RBt, m_texture_color, ts);
        }
    }
}

void CUIFrameWindow::draw_tile_rect(Frect rect, int i, Fvector2 const& ts)
{
    Frect tmp = rect;
    while (rect.lt.x + EPS_L < rect.rb.x)
    {
        draw_tile_line(rect, i, false, ts);
        rect.lt.x += m_tex_rect[i].width();
        rect.lt.x = _min(rect.lt.x, tmp.rb.x);
    }
}
