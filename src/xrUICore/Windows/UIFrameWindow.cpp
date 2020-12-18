#include "pch.hpp"
#include "UIFrameWindow.h"
#include "XML/UITextureMaster.h"
#include "Static/UIStatic.h"

void draw_rect(Fvector2 LTp, Fvector2 RBp, Fvector2 LTt, Fvector2 RBt, u32 clr, Fvector2 const& ts);

CUIFrameWindow::CUIFrameWindow()
    : m_bTextureVisible(false), m_texture_color(color_argb(255, 255, 255, 255))
{
}
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

bool CUIFrameWindow::InitTextureEx(pcstr texture, pcstr shader, bool fatal /*= true*/)
{
    dbg_tex_name = texture;
    string256 buf;

    const bool back_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_back"), shader, m_shader, m_tex_rect[fmBK]);
    const bool left_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_l"), shader, m_shader, m_tex_rect[fmL]);
    const bool right_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_r"), shader, m_shader, m_tex_rect[fmR]);
    const bool top_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_t"), shader, m_shader, m_tex_rect[fmT]);
    const bool bottom_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_b"), shader, m_shader, m_tex_rect[fmB]);

    const bool leftTop_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_lt"), shader, m_shader, m_tex_rect[fmLT]);
    const bool rightBottom_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_rb"), shader, m_shader, m_tex_rect[fmRB]);
    const bool rightTop_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_rt"), shader, m_shader, m_tex_rect[fmRT]);
    const bool leftBottom_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_lb"), shader, m_shader, m_tex_rect[fmLB]);

    bool failed = false;

    if (fatal)
    {
        R_ASSERT2(back_exist, texture);
        R_ASSERT2(left_exist, texture);
        R_ASSERT2(right_exist, texture);
        R_ASSERT2(top_exist, texture);
        R_ASSERT2(bottom_exist, texture);

        R_ASSERT2(leftTop_exist, texture);
        R_ASSERT2(rightBottom_exist, texture);
        R_ASSERT2(rightTop_exist, texture);
        R_ASSERT2(leftBottom_exist, texture);
    }
    /*else*/ // Always set failed flag to be able to play in debug
    {
        failed |= !back_exist;
        failed |= !left_exist;
        failed |= !right_exist;
        failed |= !top_exist;
        failed |= !bottom_exist;

        failed |= !leftTop_exist;
        failed |= !rightBottom_exist;
        failed |= !rightTop_exist;
        failed |= !leftBottom_exist;
    }

    const bool LT_and_T_are_similar_by_height = fsimilar(m_tex_rect[fmLT].height(), m_tex_rect[fmT].height());
    const bool LT_and_RT_are_similar_by_height = fsimilar(m_tex_rect[fmLT].height(), m_tex_rect[fmRT].height());

    //const bool L_and_BK_are_similar_by_height = fsimilar(m_tex_rect[fmL].height(), m_tex_rect[fmBK].height());
    const bool L_and_R_are_similar_by_height = fsimilar(m_tex_rect[fmL].height(), m_tex_rect[fmR].height());
    
    const bool LB_and_B_are_similar_by_height = fsimilar(m_tex_rect[fmLB].height(), m_tex_rect[fmB].height());
    const bool LB_and_RB_are_similar_by_height = fsimilar(m_tex_rect[fmLB].height(), m_tex_rect[fmRB].height());

    const bool LT_and_L_are_similar_by_width = fsimilar(m_tex_rect[fmLT].width(), m_tex_rect[fmL].width());
    const bool LT_and_LB_are_similar_by_width = fsimilar(m_tex_rect[fmLT].width(), m_tex_rect[fmLB].width());

    //const bool T_and_BK_are_similar_by_width = fsimilar(m_tex_rect[fmT].width(), m_tex_rect[fmBK].width());
    const bool T_and_B_are_similar_by_width = fsimilar(m_tex_rect[fmT].width(), m_tex_rect[fmB].width());

    const bool RT_and_R_are_similar_by_width = fsimilar(m_tex_rect[fmRT].width(), m_tex_rect[fmR].width());
    const bool RT_and_RB_are_similar_by_width = fsimilar(m_tex_rect[fmRT].width(), m_tex_rect[fmRB].width());

    if (fatal)
    {
        R_ASSERT2(LT_and_T_are_similar_by_height, texture);
        R_ASSERT2(LT_and_RT_are_similar_by_height, texture);

        //R_ASSERT2(L_and_BK_are_similar_by_height, texture);
        R_ASSERT2(L_and_R_are_similar_by_height, texture);

        R_ASSERT2(LB_and_B_are_similar_by_height, texture);
        R_ASSERT2(LB_and_RB_are_similar_by_height, texture);

        R_ASSERT2(LT_and_L_are_similar_by_width, texture);
        R_ASSERT2(LT_and_LB_are_similar_by_width, texture);

        //R_ASSERT2(T_and_BK_are_similar_by_width, texture);
        R_ASSERT2(T_and_B_are_similar_by_width, texture);

        R_ASSERT2(RT_and_R_are_similar_by_width, texture);
        R_ASSERT2(RT_and_RB_are_similar_by_width, texture);
    }
    else
    {
        if (!LT_and_T_are_similar_by_height && leftTop_exist && top_exist)
            Msg("! textures %s_lt and %s_t are not similar by height", texture, texture);
        if (!LT_and_RT_are_similar_by_height && leftTop_exist && rightTop_exist)
            Msg("! textures %s_lt and %s_rt are not similar by height", texture, texture);

        if (!L_and_R_are_similar_by_height && left_exist && right_exist)
            Msg("! textures %s_l and %s_r are not similar by height", texture, texture);

        if (!LB_and_B_are_similar_by_height && leftBottom_exist && bottom_exist)
            Msg("! textures %s_lb and %s_b are not similar by height", texture, texture);
        if (!LB_and_RB_are_similar_by_height && leftBottom_exist && rightBottom_exist)
            Msg("! textures %s_lb and %s_rb are not similar by height", texture, texture);

        if (!LT_and_L_are_similar_by_width && leftTop_exist && left_exist)
            Msg("! textures %s_lt and %s_l are not similar by width", texture, texture);
        if (!LT_and_LB_are_similar_by_width && leftTop_exist && leftBottom_exist)
            Msg("! textures %s_lt and %s_lb are not similar by width", texture, texture);

        if (!T_and_B_are_similar_by_width && top_exist && bottom_exist)
            Msg("! textures %s_t and %s_b are not similar by width", texture, texture);

        if (!RT_and_R_are_similar_by_width && rightTop_exist && right_exist)
            Msg("! textures %s_rt and %s_r are not similar by width", texture, texture);
        if (!RT_and_RB_are_similar_by_width && rightTop_exist && rightBottom_exist)
            Msg("! textures %s_rt and %s_rb are not similar by width", texture, texture);
    }

    m_bTextureVisible = !failed;
    return !failed;
}

bool CUIFrameWindow::InitTexture(pcstr texture, bool fatal)
{
    return InitTextureEx(texture, "hud" DELIMITER "default", fatal);
}

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
