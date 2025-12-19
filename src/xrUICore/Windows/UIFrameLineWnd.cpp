#include "pch.hpp"
#include "UIFrameLineWnd.h"
#include "XML/UITextureMaster.h"
#include "xrEngine/editor_helper.h"

CUIFrameLineWnd::CUIFrameLineWnd(pcstr window_name) : CUIWindow(window_name) {}

bool CUIFrameLineWnd::InitFrameLineWnd(pcstr base_name, Fvector2 pos, Fvector2 size, bool horizontal, bool fatal /*= true*/)
{
    InitFrameLineWnd(pos, size, horizontal);
    return InitTexture(base_name, fatal);
}

void CUIFrameLineWnd::InitFrameLineWnd(Fvector2 pos, Fvector2 size, bool horizontal)
{
    inherited::SetWndPos(pos);
    inherited::SetWndSize(size);

    bHorizontal = horizontal;
}

bool CUIFrameLineWnd::InitTexture(pcstr texture, bool fatal /*= true*/)
{
    return InitTextureEx(texture, "hud" DELIMITER "default", fatal);
}

bool CUIFrameLineWnd::InitTextureEx(pcstr texture, pcstr shader, bool fatal /*= true*/)
{
    string256 buf;

    const bool back_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_back"), shader, m_shader, m_tex_rect[flBack]);
    const bool b_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_b"), shader, m_shader, m_tex_rect[flFirst]);
    const bool e_exist = CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture, "_e"), shader, m_shader, m_tex_rect[flSecond]);

    bool failed = false;

    if (fatal)
    {
        R_ASSERT2(back_exist, texture);
        R_ASSERT2(b_exist, texture);
        R_ASSERT2(e_exist, texture);
    }
    /*else*/ // Always set failed flag to be able to play in debug
    {
        failed |= !back_exist;
        failed |= !b_exist;
        failed |= !e_exist;
    }

    const bool B_and_E_are_similar_by_height = fsimilar(m_tex_rect[flFirst].height(), m_tex_rect[flSecond].height());
    const bool B_and_Back_are_similar_by_height = fsimilar(m_tex_rect[flFirst].height(), m_tex_rect[flBack].height());
    const bool B_and_E_are_similar_by_width = fsimilar(m_tex_rect[flFirst].width(), m_tex_rect[flSecond].width());
    const bool B_and_Back_are_similar_by_width = fsimilar(m_tex_rect[flFirst].width(), m_tex_rect[flBack].width());

    if (fatal)
    {
        if (bHorizontal)
        {
            R_ASSERT2(B_and_E_are_similar_by_height, texture);
            R_ASSERT2(B_and_Back_are_similar_by_height, texture);
        }
        else
        {
            R_ASSERT2(B_and_E_are_similar_by_width, texture);
            R_ASSERT2(B_and_Back_are_similar_by_width, texture);
        }
    }
    else
    {
        if (bHorizontal)
        {
            if (!B_and_E_are_similar_by_height && b_exist && e_exist)
                Msg("! Textures %s_b and %s_e are not similar by height", texture, texture);
            if (!B_and_Back_are_similar_by_height && b_exist && back_exist)
                Msg("! Textures %s_b and %s_back are not similar by height", texture, texture);
        }
        else
        {
            if (!B_and_E_are_similar_by_width && b_exist && e_exist)
                Msg("! Textures %s_b and %s_e are not similar by width", texture, texture);
            if (!B_and_Back_are_similar_by_width && b_exist && back_exist)
                Msg("! Textures %s_b and %s_back are not similar by width", texture, texture);
        }
    }

    m_bTextureVisible = !failed;
    return !failed;
}

void CUIFrameLineWnd::Draw()
{
    if (m_bTextureVisible)
        DrawElements();

    inherited::Draw();
}

constexpr Fvector2 pt_offset = { -0.5f, -0.5f };

ICF void draw_rect(Fvector2 LTp, Fvector2 RBp, Fvector2 LTt, Fvector2 RBt, u32 clr, Fvector2 const& ts)
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

void CUIFrameLineWnd::DrawElements() const
{
    GEnv.UIRender->SetShader(*m_shader);

    Fvector2 ts{};
    m_shader->GetBaseTextureResolution(ts);

    Frect rect;
    GetAbsoluteRect(rect);
    UI().ClientToScreenScaled(rect.lt);
    UI().ClientToScreenScaled(rect.rb);

    u32 total_tiles{};
    u32 back_tiles{};
    float back_remainder{};

    const float line_len   = bHorizontal ? rect.width() : rect.height();

    const float first_len  = bHorizontal ? m_tex_rect[flFirst].width()  : m_tex_rect[flFirst].height();
    const float second_len = bHorizontal ? m_tex_rect[flSecond].width() : m_tex_rect[flSecond].height();
    const float back_len   = bHorizontal ? m_tex_rect[flBack].width()   : m_tex_rect[flBack].height();

    const float back_available_len = line_len - first_len - second_len;

    if (back_available_len > 0.0f && back_len > 0.0f)
    {
        const float back_tiles_full = back_available_len / back_len;
        total_tiles   += iCeil(back_tiles_full);
        back_tiles     = iFloor(back_tiles_full);
        back_remainder = back_tiles_full - float(back_tiles);
    }
    else if (back_available_len < 0.0f)
    {
        if (bHorizontal)
            rect.x2 -= back_len;
        else
            rect.y2 -= back_len;
    }

    total_tiles += 2; // first & second

    GEnv.UIRender->StartPrimitive(6 * total_tiles, IUIRender::ptTriList, UI().m_currentPointType);

    float cursor{};

    const auto draw_tile = [&](const float length, const Frect tex_rect)
    {
        Fvector2 lt, rb;
        if (bHorizontal)
        {
            lt = { rect.lt.x + cursor, rect.lt.y };
            rb = { lt.x      + length, rect.rb.y };
        }
        else
        {
            lt = { rect.lt.x, rect.lt.y + cursor };
            rb = { rect.rb.x, lt.y      + length };
        }

        draw_rect(lt, rb, tex_rect.lt, tex_rect.rb, m_texture_color, ts);
        cursor += length;
    };

    // first
    draw_tile(first_len, m_tex_rect[flFirst]);

    // back
    for (u32 i = 0; i < back_tiles; ++i)
    {
        draw_tile(back_len, m_tex_rect[flBack]);
    }

    // back remainder
    if (back_remainder > 0.0f)
    {
        Frect remainder_tc = m_tex_rect[flBack];
        if (bHorizontal)
            remainder_tc.rb.x = remainder_tc.lt.x + remainder_tc.width() * back_remainder;
        else
            remainder_tc.rb.y = remainder_tc.lt.y + remainder_tc.height() * back_remainder;

        draw_tile(back_len * back_remainder, remainder_tc);
    }

    // second
    draw_tile(second_len, m_tex_rect[flSecond]);

    GEnv.UIRender->FlushPrimitive();
}

bool CUIFrameLineWnd::FillDebugTree(const CUIDebugState& debugState)
{
    return CUIWindow::FillDebugTree(debugState);
}

void CUIFrameLineWnd::FillDebugInfo()
{
#ifndef MASTER_GOLD
    CUIWindow::FillDebugInfo();

    if (!ImGui::CollapsingHeader(CUIFrameLineWnd::GetDebugType()))
        return;

    ImGui::Checkbox("Enable texture", &m_bTextureVisible);

    ImGui::SameLine();
    ImGui::Checkbox("Horizontal", &bHorizontal);

    xray::imgui::ColorEdit4("Texture color", m_texture_color);

    Fvector2 ts{};
    m_shader->GetBaseTextureResolution(ts);

    const auto showRectAdjust = [ts](pcstr label, Frect& tex_rect)
    {
        Frect rect =
        {
            .lt = tex_rect.lt,
            .rb = { tex_rect.width(), tex_rect.height() }
        };
        if (ImGui::DragFloat4(label, reinterpret_cast<float*>(&rect), 1.0f, 0.0f, std::max(ts.x, ts.y)))
        {
            tex_rect.lt = rect.lt;
            tex_rect.rb = rect.lt.add(rect.rb);
            clamp(tex_rect.lt.x, 0.0f, ts.x);
            clamp(tex_rect.lt.y, 0.0f, ts.y);
            clamp(tex_rect.rb.x, 0.0f, ts.x);
            clamp(tex_rect.rb.y, 0.0f, ts.y);
        }
    };

    showRectAdjust("Texture rect: first", m_tex_rect[flFirst]);
    showRectAdjust("Texture rect: middle", m_tex_rect[flBack]);
    showRectAdjust("Texture rect: second", m_tex_rect[flSecond]);
#endif
}
